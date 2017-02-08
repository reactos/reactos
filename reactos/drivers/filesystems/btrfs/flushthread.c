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

typedef struct {
    KEVENT Event;
    IO_STATUS_BLOCK iosb;
} write_context;

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

static NTSTATUS create_chunk(device_extension* Vcb, chunk* c, PIRP Irp, LIST_ENTRY* rollback);
static NTSTATUS update_tree_extents(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback);
static BOOL insert_tree_item_batch(LIST_ENTRY* batchlist, device_extension* Vcb, root* r, UINT64 objid, UINT64 objtype, UINT64 offset,
                                   void* data, UINT16 datalen, enum batch_operation operation, PIRP Irp, LIST_ENTRY* rollback);

static NTSTATUS STDCALL write_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    write_context* context = conptr;
    
    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);
    
//     return STATUS_SUCCESS;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS STDCALL write_data_phys(PDEVICE_OBJECT device, UINT64 address, void* data, UINT32 length) {
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

static NTSTATUS add_parents(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
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
                } else if (t->root != Vcb->root_root && t->root != Vcb->chunk_root) {
                    KEY searchkey;
                    traverse_ptr tp;
                    NTSTATUS Status;
                    
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
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, delete and create new entry
                        ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
                        
                        if (!ri) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        RtlCopyMemory(ri, &t->root->root_item, sizeof(ROOT_ITEM));
                        
                        delete_tree_item(Vcb, &tp, rollback);
                        
                        if (!insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, Irp, rollback)) {
                            ERR("insert_tree_item failed\n");
                            return STATUS_INTERNAL_ERROR;
                        }
                    }
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

BOOL find_metadata_address_in_chunk(device_extension* Vcb, chunk* c, UINT64* address) {
    LIST_ENTRY* le;
    space* s;
    
    TRACE("(%p, %llx, %p)\n", Vcb, c->offset, address);
    
    if (IsListEmpty(&c->space_size))
        return FALSE;
    
    if (!c->last_alloc_set) {
        s = CONTAINING_RECORD(c->space.Blink, space, list_entry);
        
        c->last_alloc = s->address;
        c->last_alloc_set = TRUE;
        
        if (s->size >= Vcb->superblock.node_size) {
            *address = s->address;
            c->last_alloc += Vcb->superblock.node_size;
            return TRUE;
        }
    }
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->address <= c->last_alloc && s->address + s->size >= c->last_alloc + Vcb->superblock.node_size) {
            *address = c->last_alloc;
            c->last_alloc += Vcb->superblock.node_size;
            return TRUE;
        }
        
        le = le->Flink;
    }
    
    le = c->space_size.Flink;
    while (le != &c->space_size) {
        s = CONTAINING_RECORD(le, space, list_entry_size);
        
        if (s->size == Vcb->superblock.node_size) {
            *address = s->address;
            c->last_alloc = s->address + Vcb->superblock.node_size;
            return TRUE;
        } else if (s->size < Vcb->superblock.node_size) {
            if (le == c->space_size.Flink)
                return FALSE;
            
            s = CONTAINING_RECORD(le->Blink, space, list_entry_size);
            
            *address = s->address;
            c->last_alloc = s->address + Vcb->superblock.node_size;
            
            return TRUE;
        }
        
        le = le->Flink;
    }
    
    s = CONTAINING_RECORD(c->space_size.Blink, space, list_entry_size);
    
    if (s->size > Vcb->superblock.node_size) {
        *address = s->address;
        c->last_alloc = s->address + Vcb->superblock.node_size;
        return TRUE;
    }
    
    return FALSE;
}

static BOOL insert_tree_extent(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64* new_address, PIRP Irp, LIST_ENTRY* rollback) {
    UINT64 address;
    EXTENT_ITEM_TREE2* eit2;
    traverse_ptr insert_tp;
    
    TRACE("(%p, %x, %llx, %p, %p, %p, %p)\n", Vcb, level, root_id, c, new_address, rollback);
    
    if (!find_metadata_address_in_chunk(Vcb, c, &address))
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
    UINT64 flags, addr;
    
    if (t->root->id == BTRFS_ROOT_CHUNK)
        flags = Vcb->system_flags;
    else
        flags = Vcb->metadata_flags;
    
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
        
        if (!origchunk->readonly && !origchunk->reloc && origchunk->chunk_item->type == flags &&
            insert_tree_extent(Vcb, t->header.level, t->root->id, origchunk, &addr, Irp, rollback)) {
            t->new_address = addr;
            t->has_new_address = TRUE;
            return STATUS_SUCCESS;
        }
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (!c->readonly && !c->reloc) {
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c != origchunk && c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
                if (insert_tree_extent(Vcb, t->header.level, t->root->id, c, &addr, Irp, rollback)) {
                    ExReleaseResourceLite(&c->lock);
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    t->new_address = addr;
                    t->has_new_address = TRUE;
                    return STATUS_SUCCESS;
                }
            }
            
            ExReleaseResourceLite(&c->lock);
        }

        le = le->Flink;
    }
    
    // allocate new chunk if necessary
    if ((c = alloc_chunk(Vcb, flags))) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t->header.level, t->root->id, c, &addr, Irp, rollback)) {
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

static NTSTATUS reduce_tree_extent(device_extension* Vcb, UINT64 address, tree* t, UINT64 parent_root, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 rc, root;
    
    TRACE("(%p, %llx, %p)\n", Vcb, address, t);

    rc = get_extent_refcount(Vcb, address, Vcb->superblock.node_size, Irp);
    if (rc == 0) {
        ERR("error - refcount for extent %llx was 0\n", address);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (!t || t->parent)
        root = parent_root;
    else
        root = t->header.tree_id;
    
    Status = decrease_extent_refcount_tree(Vcb, address, Vcb->superblock.node_size, root, level, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("decrease_extent_refcount_tree returned %08x\n", Status);
        return Status;
    }

    if (rc == 1) {
        chunk* c = get_chunk_from_address(Vcb, address);
        
        if (c) {
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            decrease_chunk_usage(c, Vcb->superblock.node_size);
            
            space_list_add(Vcb, c, TRUE, address, Vcb->superblock.node_size, rollback);
            
            ExReleaseResourceLite(&c->lock);
        } else
            ERR("could not find chunk for address %llx\n", address);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_changed_extent_ref_edr(changed_extent* ce, EXTENT_DATA_REF* edr, BOOL old) {
    LIST_ENTRY *le2, *list;
    changed_extent_ref* cer;
    
    list = old ? &ce->old_refs : &ce->refs;
    
    le2 = list->Flink;
    while (le2 != list) {
        cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
        
        if (cer->type == TYPE_EXTENT_DATA_REF && cer->edr.root == edr->root && cer->edr.objid == edr->objid && cer->edr.offset == edr->offset) {
            cer->edr.count += edr->count;
            goto end;
        }
        
        le2 = le2->Flink;
    }
    
    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    if (!cer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    cer->type = TYPE_EXTENT_DATA_REF;
    RtlCopyMemory(&cer->edr, edr, sizeof(EXTENT_DATA_REF));
    InsertTailList(list, &cer->list_entry);
    
end:
    if (old)
        ce->old_count += edr->count;
    else
        ce->count += edr->count;
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_changed_extent_ref_sdr(changed_extent* ce, SHARED_DATA_REF* sdr, BOOL old) {
    LIST_ENTRY *le2, *list;
    changed_extent_ref* cer;
    
    list = old ? &ce->old_refs : &ce->refs;
    
    le2 = list->Flink;
    while (le2 != list) {
        cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
        
        if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr->offset) {
            cer->sdr.count += sdr->count;
            goto end;
        }
        
        le2 = le2->Flink;
    }
    
    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    if (!cer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    cer->type = TYPE_SHARED_DATA_REF;
    RtlCopyMemory(&cer->sdr, sdr, sizeof(SHARED_DATA_REF));
    InsertTailList(list, &cer->list_entry);
    
end:
    if (old)
        ce->old_count += sdr->count;
    else
        ce->count += sdr->count;

    return STATUS_SUCCESS;
}

static BOOL shared_tree_is_unique(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    if (!t->updated_extents && t->has_address) {
        Status = update_tree_extents(Vcb, t, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return FALSE;
        }
    }
    
    searchkey.obj_id = t->header.address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (tp.item->key.obj_id == t->header.address && (tp.item->key.obj_type == TYPE_METADATA_ITEM || tp.item->key.obj_type == TYPE_EXTENT_ITEM))
        return FALSE;
    else
        return TRUE;
}

static NTSTATUS update_tree_extents(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 rc = get_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, Irp);
    UINT64 flags = get_extent_flags(Vcb, t->header.address, Irp);
    
    if (rc == 0) {
        ERR("refcount for extent %llx was 0\n", t->header.address);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (flags & EXTENT_ITEM_SHARED_BACKREFS || t->header.flags & HEADER_FLAG_SHARED_BACKREF || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
        TREE_BLOCK_REF tbr;
        BOOL unique = rc > 1 ? FALSE : (t->parent ? shared_tree_is_unique(Vcb, t->parent, Irp, rollback) : FALSE);
        
        if (t->header.level == 0) {
            LIST_ENTRY* le;
            
            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (!td->inserted && td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                    
                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                            
                        if (ed2->size > 0) {
                            EXTENT_DATA_REF edr;
                            changed_extent* ce = NULL;
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);
                            
                            if (c) {
                                LIST_ENTRY* le2;
                                
                                le2 = c->changed_extents.Flink;
                                while (le2 != &c->changed_extents) {
                                    changed_extent* ce2 = CONTAINING_RECORD(le2, changed_extent, list_entry);
                                    
                                    if (ce2->address == ed2->address) {
                                        ce = ce2;
                                        break;
                                    }

                                    le2 = le2->Flink;
                                }
                            }
                                    
                            edr.root = t->root->id;
                            edr.objid = td->key.obj_id;
                            edr.offset = td->key.offset - ed2->offset;
                            edr.count = 1;
                            
                            if (ce) {
                                Status = add_changed_extent_ref_edr(ce, &edr, TRUE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                    return Status;
                                }
                                
                                Status = add_changed_extent_ref_edr(ce, &edr, FALSE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                    return Status;
                                }
                            }
                            
                            Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp, rollback);
                            if (!NT_SUCCESS(Status)) {
                                ERR("increase_extent_refcount returned %08x\n", Status);
                                return Status;
                            }
                            
                            if ((flags & EXTENT_ITEM_SHARED_BACKREFS && unique) || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
                                UINT64 sdrrc = find_extent_shared_data_refcount(Vcb, ed2->address, t->header.address, Irp);

                                if (sdrrc > 0) {
                                    SHARED_DATA_REF sdr;
                                    
                                    sdr.offset = t->header.address;
                                    sdr.count = 1;
                                    
                                    Status = decrease_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0,
                                                                      t->header.address, ce->superseded, Irp, rollback);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("decrease_extent_refcount returned %08x\n", Status);
                                        return Status;
                                    }
                                    
                                    if (ce) {
                                        LIST_ENTRY* le2;
                                        
                                        le2 = ce->refs.Flink;
                                        while (le2 != &ce->refs) {
                                            changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
                                            
                                            if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr.offset) {
                                                ce->count--;
                                                cer->sdr.count--;
                                                break;
                                            }
                                            
                                            le2 = le2->Flink;
                                        }
                                        
                                        le2 = ce->old_refs.Flink;
                                        while (le2 != &ce->old_refs) {
                                            changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
                                            
                                            if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr.offset) {
                                                ce->old_count--;
                                                
                                                if (cer->sdr.count > 1)
                                                    cer->sdr.count--;
                                                else {
                                                    RemoveEntryList(&cer->list_entry);
                                                    ExFreePool(cer);
                                                }
                                                
                                                break;
                                            }
                                            
                                            le2 = le2->Flink;
                                        }
                                    }
                                }
                            }
                            
                            // FIXME - clear shared flag if unique?
                        }
                    }
                }
                
                le = le->Flink;
            }
        } else {
            LIST_ENTRY* le;
            
            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (!td->inserted) {
                    TREE_BLOCK_REF tbr;
                    
                    tbr.offset = t->root->id;
                    
                    Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF,
                                                      &tbr, &td->key, t->header.level - 1, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }
                    
                    if (unique || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
                        UINT64 sbrrc = find_extent_shared_tree_refcount(Vcb, td->treeholder.address, t->header.address, Irp);

                        if (sbrrc > 0) {
                            SHARED_BLOCK_REF sbr;
                
                            sbr.offset = t->header.address;
                            
                            Status = decrease_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                              t->header.address, FALSE, Irp, rollback);
                            if (!NT_SUCCESS(Status)) {
                                ERR("decrease_extent_refcount returned %08x\n", Status);
                                return Status;
                            }
                        }
                    }
                            
                    // FIXME - clear shared flag if unique?
                }
                
                le = le->Flink;
            }
        }
        
        if (unique) {
            UINT64 sbrrc = find_extent_shared_tree_refcount(Vcb, t->header.address, t->parent->header.address, Irp);
            
            if (sbrrc == 1) {
                SHARED_BLOCK_REF sbr;
                
                sbr.offset = t->parent->header.address;
                
                Status = decrease_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                  t->parent->header.address, FALSE, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("decrease_extent_refcount returned %08x\n", Status);
                    return Status;
                }
            }
        }
        
        if (t->parent)
            tbr.offset = t->parent->header.tree_id;
        else
            tbr.offset = t->header.tree_id;
        
        Status = increase_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr,
                                          t->parent ? &t->paritem->key : NULL, t->header.level, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("increase_extent_refcount returned %08x\n", Status);
            return Status;
        }
        
        // FIXME - clear shared flag if unique?
        
        t->header.flags &= ~HEADER_FLAG_SHARED_BACKREF;
    }
    
    Status = reduce_tree_extent(Vcb, t->header.address, t, t->parent ? t->parent->header.tree_id : t->header.tree_id, t->header.level, Irp, rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("reduce_tree_extent returned %08x\n", Status);
        return Status;
    }
    
    t->has_address = FALSE;
    
    if (rc > 1 && !(flags & EXTENT_ITEM_SHARED_BACKREFS)) {
        if (t->header.tree_id == t->root->id) {
            flags |= EXTENT_ITEM_SHARED_BACKREFS;
            update_extent_flags(Vcb, t->header.address, flags, Irp);
        }
        
        if (t->header.level > 0) {
            LIST_ENTRY* le;
            
            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (!td->inserted) {
                    if (t->header.tree_id == t->root->id) {
                        SHARED_BLOCK_REF sbr;
                        
                        sbr.offset = t->header.address;
                        
                        Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, &td->key, t->header.level - 1, Irp, rollback);
                    } else {
                        TREE_BLOCK_REF tbr;
                        
                        tbr.offset = t->root->id;
                        
                        Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr, &td->key, t->header.level - 1, Irp, rollback);
                    }
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }
                }
                
                le = le->Flink;
            }
        } else {
            LIST_ENTRY* le;
            
            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (!td->inserted && td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                    
                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        if (ed2->size > 0) {
                            changed_extent* ce = NULL;
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);
                            
                            if (c) {
                                LIST_ENTRY* le2;
                                
                                le2 = c->changed_extents.Flink;
                                while (le2 != &c->changed_extents) {
                                    changed_extent* ce2 = CONTAINING_RECORD(le2, changed_extent, list_entry);
                                    
                                    if (ce2->address == ed2->address) {
                                        ce = ce2;
                                        break;
                                    }

                                    le2 = le2->Flink;
                                }
                            }
                            
                            if (t->header.tree_id == t->root->id) {
                                SHARED_DATA_REF sdr;
                                
                                sdr.offset = t->header.address;
                                sdr.count = 1;
                                
                                if (ce) {
                                    Status = add_changed_extent_ref_sdr(ce, &sdr, TRUE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                    
                                    Status = add_changed_extent_ref_sdr(ce, &sdr, FALSE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                }
                                
                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0, Irp, rollback);
                            } else {
                                EXTENT_DATA_REF edr;
                                
                                edr.root = t->root->id;
                                edr.objid = td->key.obj_id;
                                edr.offset = td->key.offset - ed2->offset;
                                edr.count = 1;
                                
                                if (ce) {
                                    Status = add_changed_extent_ref_edr(ce, &edr, TRUE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                    
                                    Status = add_changed_extent_ref_edr(ce, &edr, FALSE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                }
                                
                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp, rollback);
                            }
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("increase_extent_refcount returned %08x\n", Status);
                                return Status;
                            }
                        }
                    }
                }
                
                le = le->Flink;
            }
        }
    }
    
    t->updated_extents = TRUE;
    t->header.tree_id = t->root->id;
    
    return STATUS_SUCCESS;
}

static NTSTATUS allocate_tree_extents(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    BOOL changed = FALSE;
    UINT8 max_level = 0, level;
    
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
            
            c = get_chunk_from_address(Vcb, t->new_address);
            
            if (c) {
                increase_chunk_usage(c, Vcb->superblock.node_size);
            } else {
                ERR("could not find chunk for address %llx\n", t->new_address);
                return STATUS_INTERNAL_ERROR;
            }
            
            changed = TRUE;
            
            if (t->header.level > max_level)
                max_level = t->header.level;
        }
        
        le = le->Flink;
    }
    
    if (!changed)
        return STATUS_SUCCESS;
    
    level = max_level;
    do {
        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            
            if (t->write && !t->updated_extents && t->has_address && t->header.level == level) {
                Status = update_tree_extents(Vcb, t, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("update_tree_extents returned %08x\n", Status);
                    return Status;
                }
            }
            
            le = le->Flink;
        }
        
        if (level == 0)
            break;
        
        level--;
    } while (TRUE);
    
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
                
                // item is guaranteed to be at least sizeof(ROOT_ITEM), due to add_parents

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

NTSTATUS do_tree_writes(device_extension* Vcb, LIST_ENTRY* tree_writes, PIRP Irp) {
    chunk* c;
    LIST_ENTRY* le;
    tree_write* tw;
    NTSTATUS Status;
    write_data_context* wtc;
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    KeInitializeEvent(&wtc->Event, NotificationEvent, FALSE);
    InitializeListHead(&wtc->stripes);
    wtc->tree = TRUE;
    wtc->stripes_left = 0;
    
    // merge together runs
    c = NULL;
    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);
        
        if (!c || tw->address < c->offset || tw->address >= c->offset + c->chunk_item->size)
            c = get_chunk_from_address(Vcb, tw->address);
        else {
            tree_write* tw2 = CONTAINING_RECORD(le->Blink, tree_write, list_entry);
            
            if (tw->address == tw2->address + tw2->length) {
                UINT8* data = ExAllocatePoolWithTag(NonPagedPool, tw2->length + tw->length, ALLOC_TAG);
                
                if (!data) {
                    ERR("out of memory\n");
                    ExFreePool(wtc);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(data, tw2->data, tw2->length);
                RtlCopyMemory(&data[tw2->length], tw->data, tw->length);
                
                ExFreePool(tw2->data);
                tw2->data = data;
                tw2->length += tw->length;
                
                ExFreePool(tw->data);
                RemoveEntryList(&tw->list_entry);
                ExFreePool(tw);
                
                le = tw2->list_entry.Flink;
                continue;
            }
        }
        
        le = le->Flink;
    }
    
    // mark RAID5/6 overlaps so we can do them one by one
    c = NULL;
    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);
        
        if (!c || tw->address < c->offset || tw->address >= c->offset + c->chunk_item->size)
            c = get_chunk_from_address(Vcb, tw->address);
        else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
            tree_write* tw2 = CONTAINING_RECORD(le->Blink, tree_write, list_entry);
            UINT64 last_stripe, this_stripe;
            
            last_stripe = (tw2->address + tw2->length - 1 - c->offset) / (c->chunk_item->stripe_length * (c->chunk_item->num_stripes - 1));
            this_stripe = (tw->address - c->offset) / (c->chunk_item->stripe_length * (c->chunk_item->num_stripes - 1));
            
            if (last_stripe == this_stripe)
                tw->overlap = TRUE;
        } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
            tree_write* tw2 = CONTAINING_RECORD(le->Blink, tree_write, list_entry);
            UINT64 last_stripe, this_stripe;
            
            last_stripe = (tw2->address + tw2->length - 1 - c->offset) / (c->chunk_item->stripe_length * (c->chunk_item->num_stripes - 2));
            this_stripe = (tw->address - c->offset) / (c->chunk_item->stripe_length * (c->chunk_item->num_stripes - 2));
            
            if (last_stripe == this_stripe)
                tw->overlap = TRUE;
        }
        
        le = le->Flink;
    }
    
    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);
        
        if (!tw->overlap) {
            TRACE("address: %llx, size: %x, overlap = %u\n", tw->address, tw->length, tw->overlap);
            
            Status = write_data(Vcb, tw->address, tw->data, TRUE, tw->length, wtc, NULL, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data returned %08x\n", Status);
                ExFreePool(wtc);
                return Status;
            }
        }
        
        le = le->Flink;
    }
    
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
    
    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);
        
        if (tw->overlap) {
            TRACE("address: %llx, size: %x, overlap = %u\n", tw->address, tw->length, tw->overlap);
            
            Status = write_data_complete(Vcb, tw->address, tw->data, tw->length, Irp, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data_complete returned %08x\n", Status);
                ExFreePool(wtc);
                return Status;
            }
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS write_trees(device_extension* Vcb, PIRP Irp) {
    UINT8 level;
    UINT8 *data, *body;
    UINT32 crc32;
    NTSTATUS Status;
    LIST_ENTRY* le;
    LIST_ENTRY tree_writes;
    tree_write* tw;
    
    TRACE("(%p)\n", Vcb);
    
    InitializeListHead(&tree_writes);

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
                    
                    if (keycmp(searchkey, tp.item->key)) {
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
            t->header.tree_id = t->root->id;
            t->header.flags |= HEADER_FLAG_MIXED_BACKREF;
            t->header.fs_uuid = Vcb->superblock.uuid;
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
            
            tw = ExAllocatePoolWithTag(PagedPool, sizeof(tree_write), ALLOC_TAG);
            if (!tw) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            tw->address = t->new_address;
            tw->length = Vcb->superblock.node_size;
            tw->data = data;
            tw->overlap = FALSE;
            
            if (IsListEmpty(&tree_writes))
                InsertTailList(&tree_writes, &tw->list_entry);
            else {
                LIST_ENTRY* le2;
                BOOL inserted = FALSE;
                
                le2 = tree_writes.Flink;
                while (le2 != &tree_writes) {
                    tree_write* tw2 = CONTAINING_RECORD(le2, tree_write, list_entry);
                    
                    if (tw2->address > tw->address) {
                        InsertHeadList(le2->Blink, &tw->list_entry);
                        inserted = TRUE;
                        break;
                    }
                    
                    le2 = le2->Flink;
                }
                
                if (!inserted)
                    InsertTailList(&tree_writes, &tw->list_entry);
            }
        }

        le = le->Flink;
    }
    
    Status = do_tree_writes(Vcb, &tree_writes, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("do_tree_writes returned %08x\n", Status);
        goto end;
    }
    
    Status = STATUS_SUCCESS;

end:
    while (!IsListEmpty(&tree_writes)) {
        le = RemoveHeadList(&tree_writes);
        tw = CONTAINING_RECORD(le, tree_write, list_entry);
        
        ExFreePool(tw);
    }
    
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

static NTSTATUS STDCALL write_superblock(device_extension* Vcb, device* device) {
    NTSTATUS Status;
    unsigned int i = 0;
    UINT32 crc32;
#ifdef __REACTOS__
    Status = STATUS_INTERNAL_ERROR;
#endif
    
    RtlCopyMemory(&Vcb->superblock.dev_item, &device->devitem, sizeof(DEV_ITEM));
    
    // All the documentation says that the Linux driver only writes one superblock
    // if it thinks a disk is an SSD, but this doesn't seem to be the case!
    
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
    
    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        
        if (dev->devobj && !dev->readonly) {
            Status = write_superblock(Vcb, dev);
            if (!NT_SUCCESS(Status)) {
                ERR("write_superblock returned %08x\n", Status);
                return Status;
            }
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS flush_changed_extent(device_extension* Vcb, chunk* c, changed_extent* ce, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    UINT64 old_size;
    
    if (ce->count == 0 && ce->old_count == 0) {
        while (!IsListEmpty(&ce->refs)) {
            changed_extent_ref* cer = CONTAINING_RECORD(RemoveHeadList(&ce->refs), changed_extent_ref, list_entry);
            ExFreePool(cer);
        }
        
        while (!IsListEmpty(&ce->old_refs)) {
            changed_extent_ref* cer = CONTAINING_RECORD(RemoveHeadList(&ce->old_refs), changed_extent_ref, list_entry);
            ExFreePool(cer);
        }
        
        goto end;
    }
    
    le = ce->refs.Flink;
    while (le != &ce->refs) {
        changed_extent_ref* cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        LIST_ENTRY* le3 = le->Flink;
        UINT64 old_count = 0;
        
        if (cer->type == TYPE_EXTENT_DATA_REF) {
            le2 = ce->old_refs.Flink;
            while (le2 != &ce->old_refs) {
                changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
                
                if (cer2->type == TYPE_EXTENT_DATA_REF && cer2->edr.root == cer->edr.root && cer2->edr.objid == cer->edr.objid && cer2->edr.offset == cer->edr.offset) {
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
                                                       old_count - cer->edr.count, ce->superseded, Irp, rollback);
                
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
                
                if (keycmp(searchkey, tp.item->key)) {
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
        } else if (cer->type == TYPE_SHARED_DATA_REF) {
            le2 = ce->old_refs.Flink;
            while (le2 != &ce->old_refs) {
                changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
                
                if (cer2->type == TYPE_SHARED_DATA_REF && cer2->sdr.offset == cer->sdr.offset) {
//                     old_count = cer2->edr.count;
                    
                    RemoveEntryList(&cer2->list_entry);
                    ExFreePool(cer2);
                    break;
                }
                
                le2 = le2->Flink;
            }            
        }
       
        RemoveEntryList(&cer->list_entry);
        ExFreePool(cer);
        
        le = le3;
    }
    
#ifdef DEBUG_PARANOID
    if (!IsListEmpty(&ce->old_refs))
        WARN("old_refs not empty\n");
#endif
    
end:
    if (ce->count == 0 && !ce->superseded) {
        decrease_chunk_usage(c, ce->size);
        space_list_add(Vcb, c, TRUE, ce->address, ce->size, rollback);
    }

    RemoveEntryList(&ce->list_entry);
    ExFreePool(ce);
    
    return STATUS_SUCCESS;
}

void add_checksum_entry(device_extension* Vcb, UINT64 address, ULONG length, UINT32* csum, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    UINT32* data;
    NTSTATUS Status;
    UINT64 startaddr, endaddr;
    ULONG len;
    UINT32* checksums;
    RTL_BITMAP bmp;
    ULONG* bmparr;
    ULONG runlength, index;
    
    searchkey.obj_id = EXTENT_CSUM_ID;
    searchkey.obj_type = TYPE_EXTENT_CSUM;
    searchkey.offset = address;
    
    // FIXME - create checksum_root if it doesn't exist at all
    
    Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
    if (Status == STATUS_NOT_FOUND) { // tree is completely empty
        if (csum) { // not deleted
            ULONG length2 = length;
            UINT64 off = address;
            UINT32* data = csum;
            
            do {
                ULONG il = min(length2, MAX_CSUM_SIZE / sizeof(UINT32));
                
                checksums = ExAllocatePoolWithTag(PagedPool, il * sizeof(UINT32), ALLOC_TAG);
                if (!checksums) {
                    ERR("out of memory\n");
                    return;
                }
                
                RtlCopyMemory(checksums, data, il * sizeof(UINT32));
                
                if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, off, checksums,
                                        il * sizeof(UINT32), NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(checksums);
                    return;
                }
                
                length2 -= il;
                
                if (length2 > 0) {
                    off += il * Vcb->superblock.sector_size;
                    data += il;
                }
            } while (length2 > 0);
        }
    } else if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return;
    } else {
        UINT32 tplen;
        
        // FIXME - check entry is TYPE_EXTENT_CSUM?
        
        if (tp.item->key.offset < address && tp.item->key.offset + (tp.item->size * Vcb->superblock.sector_size / sizeof(UINT32)) >= address)
            startaddr = tp.item->key.offset;
        else
            startaddr = address;
        
        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = address + (length * Vcb->superblock.sector_size);
        
        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return;
        }
        
        tplen = tp.item->size / sizeof(UINT32);
        
        if (tp.item->key.offset + (tplen * Vcb->superblock.sector_size) >= address + (length * Vcb->superblock.sector_size))
            endaddr = tp.item->key.offset + (tplen * Vcb->superblock.sector_size);
        else
            endaddr = address + (length * Vcb->superblock.sector_size);
        
        TRACE("cs starts at %llx (%x sectors)\n", address, length);
        TRACE("startaddr = %llx\n", startaddr);
        TRACE("endaddr = %llx\n", endaddr);
        
        len = (endaddr - startaddr) / Vcb->superblock.sector_size;
        
        checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * len, ALLOC_TAG);
        if (!checksums) {
            ERR("out of memory\n");
            return;
        }
        
        bmparr = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) * ((len/8)+1), ALLOC_TAG);
        if (!bmparr) {
            ERR("out of memory\n");
            ExFreePool(checksums);
            return;
        }
            
        RtlInitializeBitMap(&bmp, bmparr, len);
        RtlSetAllBits(&bmp);
        
        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = address;
        
        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return;
        }
        
        // set bit = free space, cleared bit = allocated sector
        
        while (tp.item->key.offset < endaddr) {
            if (tp.item->key.offset >= startaddr) {
                if (tp.item->size > 0) {
                    ULONG itemlen = min((len - (tp.item->key.offset - startaddr) / Vcb->superblock.sector_size) * sizeof(UINT32), tp.item->size);
                    
                    RtlCopyMemory(&checksums[(tp.item->key.offset - startaddr) / Vcb->superblock.sector_size], tp.item->data, itemlen);
                    RtlClearBits(&bmp, (tp.item->key.offset - startaddr) / Vcb->superblock.sector_size, itemlen / sizeof(UINT32));
                }
                
                delete_tree_item(Vcb, &tp, rollback);
            }
            
            if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
                tp = next_tp;
            } else
                break;
        }
        
        if (!csum) { // deleted
            RtlSetBits(&bmp, (address - startaddr) / Vcb->superblock.sector_size, length);
        } else {
            RtlCopyMemory(&checksums[(address - startaddr) / Vcb->superblock.sector_size], csum, length * sizeof(UINT32));
            RtlClearBits(&bmp, (address - startaddr) / Vcb->superblock.sector_size, length);
        }
        
        runlength = RtlFindFirstRunClear(&bmp, &index);
        
        while (runlength != 0) {
            do {
                ULONG rl;
                UINT64 off;
                
                if (runlength * sizeof(UINT32) > MAX_CSUM_SIZE)
                    rl = MAX_CSUM_SIZE / sizeof(UINT32);
                else
                    rl = runlength;
                
                data = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * rl, ALLOC_TAG);
                if (!data) {
                    ERR("out of memory\n");
                    ExFreePool(bmparr);
                    ExFreePool(checksums);
                    return;
                }
                
                RtlCopyMemory(data, &checksums[index], sizeof(UINT32) * rl);
                
                off = startaddr + UInt32x32To64(index, Vcb->superblock.sector_size);
                
                if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, off, data, sizeof(UINT32) * rl, NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(data);
                    ExFreePool(bmparr);
                    ExFreePool(checksums);
                    return;
                }
                
                runlength -= rl;
                index += rl;
            } while (runlength > 0);
            
            runlength = RtlFindNextForwardRunClear(&bmp, index, &index);
        }
        
        ExFreePool(bmparr);
        ExFreePool(checksums);
    }
}

static NTSTATUS update_chunk_usage(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks.Flink, *le2;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;
    
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
            
            le2 = le3;
        }
        
        // This is usually done by update_chunks, but we have to check again in case any new chunks
        // have been allocated since.
        if (c->created) {
            Status = create_chunk(Vcb, c, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("create_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
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
            
            if (keycmp(searchkey, tp.item->key)) {
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
            
            Vcb->superblock.bytes_used += c->used - c->oldused;
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);
            
            c->oldused = c->used;
        }
        
        ExReleaseResourceLite(&c->lock);
        
        le = le->Flink;
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
    nt->header.flags = HEADER_FLAG_MIXED_BACKREF | HEADER_FLAG_WRITTEN;
    
    nt->has_address = FALSE;
    nt->Vcb = Vcb;
    nt->parent = t->parent;
    
#ifdef DEBUG_PARANOID
    if (nt->parent && nt->parent->header.level <= nt->header.level) int3;
#endif
    
    nt->root = t->root;
//     nt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    nt->new_address = 0;
    nt->has_new_address = FALSE;
    nt->updated_extents = FALSE;
    nt->list_entry_hash.Flink = NULL;
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
            
            if (td2->treeholder.tree) {
                td2->treeholder.tree->parent = nt;
#ifdef DEBUG_PARANOID
                if (td2->treeholder.tree->parent && td2->treeholder.tree->parent->header.level <= td2->treeholder.tree->header.level) int3;
#endif
            }
            
            le = le->Flink;
        }
    }
    
    if (nt->parent) {
        td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
    pt->header.flags = HEADER_FLAG_MIXED_BACKREF | HEADER_FLAG_WRITTEN;
    
    pt->has_address = FALSE;
    pt->Vcb = Vcb;
    pt->parent = NULL;
    pt->paritem = NULL;
    pt->root = t->root;
    pt->new_address = 0;
    pt->has_new_address = FALSE;
    pt->updated_extents = FALSE;
//     pt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    pt->size = pt->header.num_items * sizeof(internal_node);
    pt->list_entry_hash.Flink = NULL;
    InitializeListHead(&pt->itemlist);
    
//     ExInitializeResourceLite(&pt->nonpaged->load_tree_lock);
    
    InterlockedIncrement(&Vcb->open_trees);
    InsertTailList(&Vcb->trees, &pt->list_entry);
    
    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
    
    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
    
#ifdef DEBUG_PARANOID
    if (t->parent && t->parent->header.level <= t->header.level) int3;
    if (nt->parent && nt->parent->header.level <= nt->header.level) int3;
#endif
    
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
            
            if (numitems == 0 && ds > Vcb->superblock.node_size - sizeof(tree_header)) {
                ERR("(%llx,%x,%llx) in tree %llx is too large (%x > %x)\n",
                    td->key.obj_id, td->key.obj_type, td->key.offset, t->root->id,
                    ds, Vcb->superblock.node_size - sizeof(tree_header));
                int3;
            }
            
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

BOOL is_tree_unique(device_extension* Vcb, tree* t, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    do {
        EXTENT_ITEM* ei;
        UINT8* type;
        
        if (t->has_address) {
            searchkey.obj_id = t->header.address;
            searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
            searchkey.offset = 0xffffffffffffffff;
            
            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return FALSE;
            }
            
            if (tp.item->key.obj_id != t->header.address || (tp.item->key.obj_type != TYPE_METADATA_ITEM && tp.item->key.obj_type != TYPE_EXTENT_ITEM))
                return FALSE;
            
            if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->size == sizeof(EXTENT_ITEM_V0))
                return FALSE;
            
            if (tp.item->size < sizeof(EXTENT_ITEM))
                return FALSE;
            
            ei = (EXTENT_ITEM*)tp.item->data;
            
            if (ei->refcount > 1)
                return FALSE;
            
            if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && ei->flags & EXTENT_ITEM_TREE_BLOCK) {
                EXTENT_ITEM2* ei2;
                
                if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2))
                    return FALSE;
                
                ei2 = (EXTENT_ITEM2*)&ei[1];
                type = (UINT8*)&ei2[1];
            } else
                type = (UINT8*)&ei[1];
            
            if (type >= tp.item->data + tp.item->size || *type != TYPE_TREE_BLOCK_REF)
                return FALSE;
        }
        
        t = t->parent;
    } while (t);
    
    return TRUE;
}

static NTSTATUS try_tree_amalgamate(device_extension* Vcb, tree* t, BOOL* done, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    tree_data* nextparitem = NULL;
    NTSTATUS Status;
    tree *next_tree, *par;
    BOOL loaded;
    
    *done = FALSE;
    
    TRACE("trying to amalgamate tree in root %llx, level %x (size %u)\n", t->root->id, t->header.level, t->size);
    
    // FIXME - doesn't capture everything, as it doesn't ascend
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
    
    TRACE("nextparitem: key = %llx,%x,%llx\n", nextparitem->key.obj_id, nextparitem->key.obj_type, nextparitem->key.offset);
//     nextparitem = t->paritem;
    
//     ExAcquireResourceExclusiveLite(&t->parent->nonpaged->load_tree_lock, TRUE);
    
    Status = do_load_tree(Vcb, &nextparitem->treeholder, t->root, t->parent, nextparitem, &loaded, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return Status;
    }
    
    if (!is_tree_unique(Vcb, nextparitem->treeholder.tree, Irp))
        return STATUS_SUCCESS;
    
//     ExReleaseResourceLite(&t->parent->nonpaged->load_tree_lock);
    
    next_tree = nextparitem->treeholder.tree;
    
    if (!next_tree->updated_extents && next_tree->has_address) {
        Status = update_tree_extents(Vcb, next_tree, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return Status;
        }
    }
    
    if (t->size + next_tree->size <= Vcb->superblock.node_size - sizeof(tree_header)) {
        // merge two trees into one
        
        t->header.num_items += next_tree->header.num_items;
        t->size += next_tree->size;
        
        if (next_tree->header.level > 0) {
            le = next_tree->itemlist.Flink;
            
            while (le != &next_tree->itemlist) {
                tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (td2->treeholder.tree) {
                    td2->treeholder.tree->parent = t;
#ifdef DEBUG_PARANOID
                    if (td2->treeholder.tree->parent && td2->treeholder.tree->parent->header.level <= td2->treeholder.tree->header.level) int3;
#endif
                }
                
                le = le->Flink;
            }
        }
        
        le = next_tree->itemlist.Flink;
        while (le != &next_tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            
            td->inserted = TRUE;
            
            le = le->Flink;
        }
        
        t->itemlist.Blink->Flink = next_tree->itemlist.Flink;
        t->itemlist.Blink->Flink->Blink = t->itemlist.Blink;
        t->itemlist.Blink = next_tree->itemlist.Blink;
        t->itemlist.Blink->Flink = &t->itemlist;
        
        next_tree->itemlist.Flink = next_tree->itemlist.Blink = &next_tree->itemlist;
        
        next_tree->header.num_items = 0;
        next_tree->size = 0;
        
        if (next_tree->has_new_address) { // delete associated EXTENT_ITEM
            Status = reduce_tree_extent(Vcb, next_tree->new_address, next_tree, next_tree->parent->header.tree_id, next_tree->header.level, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        } else if (next_tree->has_address) {
            Status = reduce_tree_extent(Vcb, next_tree->header.address, next_tree, next_tree->parent->header.tree_id, next_tree->header.level, Irp, rollback);
            
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
        
        *done = TRUE;
    } else {
        // rebalance by moving items from second tree into first
        ULONG avg_size = (t->size + next_tree->size) / 2;
        KEY firstitem = {0, 0, 0};
        BOOL changed = FALSE;
        
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
                td->inserted = TRUE;
                
                if (next_tree->header.level > 0 && td->treeholder.tree) {
                    td->treeholder.tree->parent = t;
#ifdef DEBUG_PARANOID
                    if (td->treeholder.tree->parent && td->treeholder.tree->parent->header.level <= td->treeholder.tree->header.level) int3;
#endif
                }
                
                if (!td->ignore) {
                    next_tree->size -= size;
                    t->size += size;
                    next_tree->header.num_items--;
                    t->header.num_items++;
                }
                
                changed = TRUE;
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
            
        if (changed)
            *done = TRUE;
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
        
        if (!keycmp(tp.item->key, searchkey)) {
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

static NTSTATUS update_tree_extents_recursive(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    if (t->parent && !t->parent->updated_extents && t->parent->has_address) {
        Status = update_tree_extents_recursive(Vcb, t->parent, Irp, rollback);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    
    Status = update_tree_extents(Vcb, t, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_tree_extents returned %08x\n", Status);
        return Status;
    }
    
    return STATUS_SUCCESS;
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
                    if (!t->updated_extents && t->has_address) {
                        Status = update_tree_extents(Vcb, t, Irp, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("update_tree_extents returned %08x\n", Status);
                            return Status;
                        }
                    }

                    if (t->parent) {
                        done_deletions = TRUE;
            
                        TRACE("deleting tree in root %llx\n", t->root->id);
                        
                        t->root->root_item.bytes_used -= Vcb->superblock.node_size;
                        
                        if (t->has_new_address) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, t->new_address, t, t->parent->header.tree_id, t->header.level, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, t->parent->header.tree_id, t->header.level, Irp, rollback);
                            
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
                    
                    if (!t->updated_extents && t->has_address) {
                        Status = update_tree_extents_recursive(Vcb, t, Irp, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("update_tree_extents_recursive returned %08x\n", Status);
                            return Status;
                        }
                    }
                    
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
            
            if (t->write && t->header.level == level && t->header.num_items > 0 && t->parent && t->size < min_size && is_tree_unique(Vcb, t, Irp)) {
                BOOL done;
                
                do {
                    Status = try_tree_amalgamate(Vcb, t, &done, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("try_tree_amalgamate returned %08x\n", Status);
                        return Status;
                    }
                } while (done && t->size < min_size);
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
                            Status = reduce_tree_extent(Vcb, t->new_address, t, t->header.tree_id, t->header.level, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, t->header.tree_id, t->header.level, Irp, rollback);
                            
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

static NTSTATUS remove_root_extents(device_extension* Vcb, root* r, tree_holder* th, UINT8 level, tree* parent, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    if (!th->tree) {
        Status = load_tree(Vcb, th->address, r, &th->tree, NULL, NULL);
        
        if (!NT_SUCCESS(Status)) {
            ERR("load_tree(%llx) returned %08x\n", th->address, Status);
            return Status;
        }
    }
    
    if (level > 0) {
        LIST_ENTRY* le = th->tree->itemlist.Flink;
        
        while (le != &th->tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            
            if (!td->ignore) {
                Status = remove_root_extents(Vcb, r, &td->treeholder, th->tree->header.level - 1, th->tree, Irp, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("remove_root_extents returned %08x\n", Status);
                    return Status;
                }
            }
            
            le = le->Flink;
        }
    }
    
    if (th->tree && !th->tree->updated_extents && th->tree->has_address) {
        Status = update_tree_extents(Vcb, th->tree, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return Status;
        }
    }
    
    if (!th->tree || th->tree->has_address) {
        Status = reduce_tree_extent(Vcb, th->address, NULL, parent ? parent->header.tree_id : r->id, level, Irp, rollback);
        
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
    
    Status = remove_root_extents(Vcb, r, &r->treeholder, r->root_item.root_level, NULL, Irp, rollback);
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
                if (!keycmp(tp.item->key, searchkey))
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
    
    if (keycmp(tp.item->key, searchkey)) {
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
        
        if (keycmp(sc2->key, sc->key) == 1)
            break;
        
        le = le->Flink;
    }
    InsertTailList(le, &sc->list_entry);
    
    Vcb->superblock.n += sizeof(KEY) + size;
    
    regen_bootstrap(Vcb);
    
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
    else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
        factor = c->chunk_item->num_stripes - 1;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
        factor = c->chunk_item->num_stripes - 2;
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

static NTSTATUS STDCALL set_xattr(device_extension* Vcb, LIST_ENTRY* batchlist, root* subvol, UINT64 inode, char* name, UINT32 crc32,
                                  UINT8* data, UINT16 datalen, PIRP Irp, LIST_ENTRY* rollback) {
    ULONG xasize;
    DIR_ITEM* xa;
    
    TRACE("(%p, %llx, %llx, %s, %08x, %p, %u)\n", Vcb, subvol->id, inode, name, crc32, data, datalen);
    
    xasize = sizeof(DIR_ITEM) - 1 + (ULONG)strlen(name) + datalen;
    
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
    
    if (!insert_tree_item_batch(batchlist, Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, xa, xasize, Batch_SetXattr, Irp, rollback))
        return STATUS_INTERNAL_ERROR;
    
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
    
    if (!keycmp(tp.item->key, searchkey)) { // key exists
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

static BOOL insert_tree_item_batch(LIST_ENTRY* batchlist, device_extension* Vcb, root* r, UINT64 objid, UINT64 objtype, UINT64 offset,
                                   void* data, UINT16 datalen, enum batch_operation operation, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    batch_root* br = NULL;
    batch_item* bi;
    
    le = batchlist->Flink;
    while (le != batchlist) {
        batch_root* br2 = CONTAINING_RECORD(le, batch_root, list_entry);
        
        if (br2->r == r) {
            br = br2;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!br) {
        br = ExAllocatePoolWithTag(PagedPool, sizeof(batch_root), ALLOC_TAG);
        if (!br) {
            ERR("out of memory\n");
            return FALSE;
        }
        
        br->r = r;
        InitializeListHead(&br->items);
        InsertTailList(batchlist, &br->list_entry);
    }
    
    bi = ExAllocateFromPagedLookasideList(&Vcb->batch_item_lookaside);
    if (!bi) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    bi->key.obj_id = objid;
    bi->key.obj_type = objtype;
    bi->key.offset = offset;
    bi->data = data;
    bi->datalen = datalen;
    bi->operation = operation;
    
    le = br->items.Blink;
    while (le != &br->items) {
        batch_item* bi2 = CONTAINING_RECORD(le, batch_item, list_entry);
        
        if (keycmp(bi2->key, bi->key) != 1) {
            InsertHeadList(&bi2->list_entry, &bi->list_entry);
            return TRUE;
        }
        
        le = le->Blink;
    }
    
    InsertHeadList(&br->items, &bi->list_entry);
    
    return TRUE;
}

typedef struct {
    UINT64 address;
    UINT64 length;
    UINT64 offset;
    BOOL changed;
    chunk* chunk;
    UINT64 skip_start;
    UINT64 skip_end;
    LIST_ENTRY list_entry;
} extent_range;

static void rationalize_extents(fcb* fcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    LIST_ENTRY extent_ranges;
    extent_range* er;
    BOOL changed = FALSE, truncating = FALSE;
    UINT32 num_extents = 0;
    
    InitializeListHead(&extent_ranges);
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        
        if ((ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) && ext->data->compression == BTRFS_COMPRESSION_NONE && ext->unique) {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
            
            if (ed2->size != 0) {
                LIST_ENTRY* le2;
                
                le2 = extent_ranges.Flink;
                while (le2 != &extent_ranges) {
                    extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);
                    
                    if (er2->address == ed2->address) {
                        er2->skip_start = min(er2->skip_start, ed2->offset);
                        er2->skip_end = min(er2->skip_end, ed2->size - ed2->offset - ed2->num_bytes);
                        goto cont;
                    } else if (er2->address > ed2->address)
                        break;
                    
                    le2 = le2->Flink;
                }
                
                er = ExAllocatePoolWithTag(PagedPool, sizeof(extent_range), ALLOC_TAG); // FIXME - should be from lookaside?
                if (!er) {
                    ERR("out of memory\n");
                    goto end;
                }
                
                er->address = ed2->address;
                er->length = ed2->size;
                er->offset = ext->offset - ed2->offset;
                er->changed = FALSE;
                er->chunk = NULL;
                er->skip_start = ed2->offset;
                er->skip_end = ed2->size - ed2->offset - ed2->num_bytes;
                
                if (er->skip_start != 0 || er->skip_end != 0)
                    truncating = TRUE;
                
                InsertHeadList(le2->Blink, &er->list_entry);
                num_extents++;
            }
        }
        
cont:
        le = le->Flink;
    }
    
    if (num_extents == 0 || (num_extents == 1 && !truncating))
        goto end;
    
    le = extent_ranges.Flink;
    while (le != &extent_ranges) {
        er = CONTAINING_RECORD(le, extent_range, list_entry);
        
        if (!er->chunk) {
            LIST_ENTRY* le2;
            
            er->chunk = get_chunk_from_address(fcb->Vcb, er->address);
            
            if (!er->chunk) {
                ERR("get_chunk_from_address(%llx) failed\n", er->address);
                goto end;
            }
            
            le2 = le->Flink;
            while (le2 != &extent_ranges) {
                extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);
                
                if (!er2->chunk && er2->address >= er->chunk->offset && er2->address < er->chunk->offset + er->chunk->chunk_item->size)
                    er2->chunk = er->chunk;
                
                le2 = le2->Flink;
            }
        }
        
        le = le->Flink;
    }
    
    if (truncating) {
        // truncate beginning or end of extent if unused
        
        le = extent_ranges.Flink;
        while (le != &extent_ranges) {
            er = CONTAINING_RECORD(le, extent_range, list_entry);
            
            if (er->skip_start > 0) {
                LIST_ENTRY* le2 = fcb->extents.Flink;
                while (le2 != &fcb->extents) {
                    extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
                    
                    if ((ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) && ext->data->compression == BTRFS_COMPRESSION_NONE && ext->unique) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                
                        if (ed2->size != 0 && ed2->address == er->address) {
                            NTSTATUS Status;
                            
                            Status = update_changed_extent_ref(fcb->Vcb, er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                               -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                                goto end;
                            }
                            
                            ext->data->decoded_size -= er->skip_start;
                            ed2->size -= er->skip_start;
                            ed2->address += er->skip_start;
                            ed2->offset -= er->skip_start;
                            
                            add_changed_extent_ref(er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                   1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
                        }
                    }
                    
                    le2 = le2->Flink;
                }
                
                if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM))
                    add_checksum_entry(fcb->Vcb, er->address, er->skip_start / fcb->Vcb->superblock.sector_size, NULL, NULL, rollback);
                
                decrease_chunk_usage(er->chunk, er->skip_start);
                
                space_list_add(fcb->Vcb, er->chunk, TRUE, er->address, er->skip_start, NULL);
                
                er->address += er->skip_start;
                er->length -= er->skip_start;
            }
            
            if (er->skip_end > 0) {
                LIST_ENTRY* le2 = fcb->extents.Flink;
                while (le2 != &fcb->extents) {
                    extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
                    
                    if ((ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) && ext->data->compression == BTRFS_COMPRESSION_NONE && ext->unique) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                
                        if (ed2->size != 0 && ed2->address == er->address) {
                            NTSTATUS Status;
                            
                            Status = update_changed_extent_ref(fcb->Vcb, er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                               -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                                goto end;
                            }
                            
                            ext->data->decoded_size -= er->skip_end;
                            ed2->size -= er->skip_end;
                            
                            add_changed_extent_ref(er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                   1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
                        }
                    }
                    
                    le2 = le2->Flink;
                }
                
                if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM))
                    add_checksum_entry(fcb->Vcb, er->address + er->length - er->skip_end, er->skip_end / fcb->Vcb->superblock.sector_size, NULL, NULL, rollback);
                
                decrease_chunk_usage(er->chunk, er->skip_end);
                
                space_list_add(fcb->Vcb, er->chunk, TRUE, er->address + er->length - er->skip_end, er->skip_end, NULL);
                
                er->length -= er->skip_end;
            }
            
            le = le->Flink;
        }
    }
    
    if (num_extents < 2)
        goto end;
    
    // merge together adjacent extents
    le = extent_ranges.Flink;
    while (le != &extent_ranges) {
        er = CONTAINING_RECORD(le, extent_range, list_entry);
        
        if (le->Flink != &extent_ranges && er->length < MAX_EXTENT_SIZE) {
            extent_range* er2 = CONTAINING_RECORD(le->Flink, extent_range, list_entry);
            
            if (er->chunk == er2->chunk) {
                if (er2->address == er->address + er->length && er2->offset >= er->offset + er->length) {
                    if (er->length + er2->length <= MAX_EXTENT_SIZE) {
                        er->length += er2->length;
                        er->changed = TRUE;
                        
                        RemoveEntryList(&er2->list_entry);
                        ExFreePool(er2);
                        
                        changed = TRUE;
                        continue;
//                     } else { // FIXME - make changing of beginning of offset work
//                         er2->length = er2->address + er->length - er->address - MAX_EXTENT_SIZE;
//                         er2->address = er->address + MAX_EXTENT_SIZE;
//                         er->length = MAX_EXTENT_SIZE;
                    }
                }
            }
        }
        
        le = le->Flink;
    }
    
    if (!changed)
        goto end;
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        
        if ((ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) && ext->data->compression == BTRFS_COMPRESSION_NONE && ext->unique) {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
            
            if (ed2->size != 0) {
                LIST_ENTRY* le2;
                
                le2 = extent_ranges.Flink;
                while (le2 != &extent_ranges) {
                    extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);
                    
                    if (ed2->address >= er2->address && ed2->address + ed2->size <= er2->address + er2->length && er2->changed) {
                        NTSTATUS Status;
                        
                        Status = update_changed_extent_ref(fcb->Vcb, er2->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                           -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("update_changed_extent_ref returned %08x\n", Status);
                            goto end;
                        }
                        
                        ed2->offset += ed2->address - er2->address;
                        ed2->address = er2->address;
                        ed2->size = er2->length;
                        ext->data->decoded_size = ed2->size;
                        
                        add_changed_extent_ref(er2->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                               1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
                        
                        break;
                    }
                    
                    le2 = le2->Flink;
                }
            }
        }
        
        le = le->Flink;
    }
    
end:
    while (!IsListEmpty(&extent_ranges)) {
        le = RemoveHeadList(&extent_ranges);
        er = CONTAINING_RECORD(le, extent_range, list_entry);
        
        ExFreePool(er);
    }
}

void flush_fcb(fcb* fcb, BOOL cache, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    traverse_ptr tp;
    KEY searchkey;
    NTSTATUS Status;
    INODE_ITEM* ii;
    UINT64 ii_offset;
#ifdef DEBUG_PARANOID
    UINT64 old_size = 0;
    BOOL extents_changed;
#endif
    
    if (fcb->ads) {
        if (fcb->deleted)
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, Irp, rollback);
        else {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, (UINT8*)fcb->adsdata.Buffer, fcb->adsdata.Length, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        }
        goto end;
    }
    
    if (fcb->deleted) {
        if (!insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0xffffffffffffffff, NULL, 0, Batch_DeleteInode, Irp, rollback))
            ERR("insert_tree_item_batch failed\n");
        
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
                
                if (ext->csum)
                    ExFreePool(ext->csum);
                
                ExFreePool(ext->data);
                ExFreePool(ext);
            }
            
            le = le2;
        }
        
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            
            if (ext->inserted && ext->csum && ext->data->type == EXTENT_TYPE_REGULAR) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                
                if (ed2->size > 0) { // not sparse
                    if (ext->data->compression == BTRFS_COMPRESSION_NONE)
                        add_checksum_entry(fcb->Vcb, ed2->address + ed2->offset, ed2->num_bytes / fcb->Vcb->superblock.sector_size, ext->csum, Irp, rollback);
                    else
                        add_checksum_entry(fcb->Vcb, ed2->address, ed2->size / fcb->Vcb->superblock.sector_size, ext->csum, Irp, rollback);
                }
            }
            
            le = le->Flink;
        }
        
        if (!IsListEmpty(&fcb->extents)) {
            rationalize_extents(fcb, Irp, rollback);
            
            // merge together adjacent EXTENT_DATAs pointing to same extent
            
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
                        
                            if (ext->data->compression == BTRFS_COMPRESSION_NONE && ext->csum) {
                                ULONG len = (ed2->num_bytes + ned2->num_bytes) / fcb->Vcb->superblock.sector_size;
                                UINT32* csum;
                                
                                csum = ExAllocatePoolWithTag(NonPagedPool, len * sizeof(UINT32), ALLOC_TAG);
                                if (!csum) {
                                    ERR("out of memory\n");
                                    goto end;
                                }
                                
                                RtlCopyMemory(csum, ext->csum, ed2->num_bytes * sizeof(UINT32) / fcb->Vcb->superblock.sector_size);
                                RtlCopyMemory(&csum[ed2->num_bytes / fcb->Vcb->superblock.sector_size], nextext->csum,
                                              ned2->num_bytes * sizeof(UINT32) / fcb->Vcb->superblock.sector_size);
                                
                                ExFreePool(ext->csum);
                                ext->csum = csum;
                            }
                            
                            ext->data->generation = fcb->Vcb->superblock.generation;
                            ed2->num_bytes += ned2->num_bytes;
                        
                            RemoveEntryList(&nextext->list_entry);
                        
                            if (nextext->csum)
                                ExFreePool(nextext->csum);
                        
                            ExFreePool(nextext->data);
                            ExFreePool(nextext);
                        
                            c = get_chunk_from_address(fcb->Vcb, ed2->address);
                                
                            if (!c) {
                                ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, -1,
                                                                fcb->inode_item.flags & BTRFS_INODE_NODATASUM, FALSE, Irp);
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
        }
        
        if (!fcb->created) {
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
        }
        
        // add new EXTENT_DATAs
        
        last_end = 0;
        
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            EXTENT_DATA* ed;
            
            ext->inserted = FALSE;
            
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
            
            if (!insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, ext->offset,
                                ed, ext->datalen, Batch_Insert, Irp, rollback)) {
                ERR("insert_tree_item_batch failed\n");
                Status = STATUS_INTERNAL_ERROR;
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
        
        fcb->inode_item_changed = TRUE;
        
        fcb->extents_changed = FALSE;
    }
    
    if ((!fcb->created && fcb->inode_item_changed) || cache) {
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
                int3;
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
            
            if (keycmp(tp.item->key, searchkey)) {
                ERR("could not find INODE_ITEM for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                int3;
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
        
    if (!cache && fcb->inode_item_changed) {
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            goto end;
        }
        
        RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
        
        if (!insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, ii_offset, ii, sizeof(INODE_ITEM),
                                    Batch_Insert, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            goto end;
        }
        
        fcb->inode_item_changed = FALSE;
    }
    
    if (fcb->sd_dirty) {
        Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_NTACL, EA_NTACL_HASH, (UINT8*)fcb->sd, RtlLengthSecurityDescriptor(fcb->sd), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
        }
        
        fcb->sd_dirty = FALSE;
    }
    
    if (fcb->atts_changed) {
        if (!fcb->atts_deleted) {
            UINT8 val[16], *val2;
            ULONG atts = fcb->atts;
            
            TRACE("inserting new DOSATTRIB xattr\n");
            
            val2 = &val[sizeof(val) - 1];
            
            do {
                UINT8 c = atts % 16;
                *val2 = (c >= 0 && c <= 9) ? (c + '0') : (c - 0xa + 'a');
                
                val2--;
                atts >>= 4;
            } while (atts != 0);
            
            *val2 = 'x';
            val2--;
            *val2 = '0';
            
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, val2, val + sizeof(val) - val2, Irp, rollback);
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
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_REPARSE, EA_REPARSE_HASH, (UINT8*)fcb->reparse_xattr.Buffer, fcb->reparse_xattr.Length, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_REPARSE, EA_REPARSE_HASH, Irp, rollback);
        
        fcb->reparse_xattr_changed = FALSE;
    }
    
    if (fcb->ea_changed) {
        if (fcb->ea_xattr.Buffer && fcb->ea_xattr.Length > 0) {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_EA, EA_EA_HASH, (UINT8*)fcb->ea_xattr.Buffer, fcb->ea_xattr.Length, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_EA, EA_EA_HASH, Irp, rollback);
        
        fcb->ea_changed = FALSE;
    }
    
end:
    fcb->dirty = FALSE;
}

static NTSTATUS drop_chunk(device_extension* Vcb, chunk* c, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 i, factor;
    CHUNK_ITEM_STRIPE* cis;
    
    TRACE("dropping chunk %llx\n", c->offset);
    
    // remove free space cache
    if (c->cache) {
        c->cache->deleted = TRUE;
        
        Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08x\n", Status);
            return Status;
        }
        
        flush_fcb(c->cache, TRUE, batchlist, Irp, rollback);
        
        free_fcb(c->cache);
        
        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(tp.item->key, searchkey)) {
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
            
            if (!keycmp(tp.item->key, searchkey)) {
                delete_tree_item(Vcb, &tp, rollback);
                
                if (tp.item->size >= sizeof(DEV_EXTENT)) {
                    DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;
                    
                    c->devices[i]->devitem.bytes_used -= de->length;
                    
                    space_list_add2(Vcb, &c->devices[i]->space, NULL, cis[i].offset, de->length, rollback);
                }
            } else
                WARN("could not find (%llx,%x,%llx) in dev tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        } else {
            UINT64 len = c->chunk_item->size / factor;
            
            c->devices[i]->devitem.bytes_used -= len;
            space_list_add2(Vcb, &c->devices[i]->space, NULL, cis[i].offset, len, rollback);
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
            
            if (keycmp(tp.item->key, searchkey)) {
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
        
        if (!keycmp(tp.item->key, searchkey))
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
    
    // clear raid56 incompat flag if dropping last RAID5/6 chunk
    
    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6) {
        LIST_ENTRY* le;
        BOOL clear_flag = TRUE;
        
        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);
            
            if (c2->chunk_item->type & BLOCK_FLAG_RAID5 || c2->chunk_item->type & BLOCK_FLAG_RAID6) {
                clear_flag = FALSE;
                break;
            }
            
            le = le->Flink;
        }
        
        if (clear_flag)
            Vcb->superblock.incompat_flags &= ~BTRFS_INCOMPAT_FLAGS_RAID56;
    }
    
    Vcb->superblock.bytes_used -= c->oldused;
    
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

static NTSTATUS update_chunks(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks_changed.Flink, *le2;
    NTSTATUS Status;
    UINT64 used_minus_cache;
    
    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
    // FIXME - do tree chunks before data chunks
    
    while (le != &Vcb->chunks_changed) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_changed);
        
        le2 = le->Flink;
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if (c->list_entry_balance.Flink) {
            ExReleaseResourceLite(&c->lock);
            le = le2;
            continue;
        }
        
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
            Status = drop_chunk(Vcb, c, batchlist, Irp, rollback);
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
    
    if (!keycmp(searchkey, tp.item->key)) {
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
    
    if (!keycmp(searchkey, tp.item->key)) {
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
    
    if (!keycmp(tp.item->key, searchkey) && tp.item->size > 0) {
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
    
    if (!keycmp(tp.item->key, searchkey))
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

static NTSTATUS flush_fileref(file_ref* fileref, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
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

        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX, fileref->index,
                                    di, disize, Batch_Insert, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM, crc32,
                                    di2, disize, Batch_DirItem, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF* ir;
            
            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + fileref->utf8.Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir->index = fileref->index;
            ir->n = fileref->utf8.Length;
            RtlCopyMemory(ir->name, fileref->utf8.Buffer, ir->n);
        
            if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                        ir, sizeof(INODE_REF) - 1 + ir->n, Batch_InodeRef, Irp, rollback)) {
                ERR("insert_tree_item_batch failed\n");
                return STATUS_INTERNAL_ERROR;
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
        ANSI_STRING* name;
        DIR_ITEM* di;
        
        if (fileref->oldutf8.Buffer)
            name = &fileref->oldutf8;
        else
            name = &fileref->utf8;

        crc32 = calc_crc32c(0xfffffffe, (UINT8*)name->Buffer, name->Length);

        TRACE("deleting %.*S\n", file_desc_fileref(fileref));
        
        di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + name->Length, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        di->m = 0;
        di->n = name->Length;
        RtlCopyMemory(di->name, name->Buffer, name->Length);
        
        // delete DIR_ITEM (0x54)
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM,
                                    crc32, di, sizeof(DIR_ITEM) - 1 + name->Length, Batch_DeleteDirItem, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF* ir;
            
            // delete INODE_REF (0xc)
            
            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + name->Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            ir->index = fileref->index;
            ir->n = name->Length;
            RtlCopyMemory(ir->name, name->Buffer, name->Length);

            if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF,
                                        fileref->parent->fcb->inode, ir, sizeof(INODE_REF) - 1 + name->Length, Batch_DeleteInodeRef, Irp, rollback)) {
                ERR("insert_tree_item_batch failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        } else { // subvolume
            Status = delete_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, name, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_root_ref returned %08x\n", Status);
                return Status;
            }
            
            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }
        
        // delete DIR_INDEX (0x60)
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                    fileref->index, NULL, 0, Batch_Delete, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    } else { // rename or change type
        PANSI_STRING oldutf8 = fileref->oldutf8.Buffer ? &fileref->oldutf8 : &fileref->utf8;
        UINT32 crc32, oldcrc32;
        ULONG disize;
        DIR_ITEM *olddi, *di, *di2;
        
        crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->utf8.Buffer, fileref->utf8.Length);
        
        if (!fileref->oldutf8.Buffer)
            oldcrc32 = crc32;
        else
            oldcrc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->oldutf8.Buffer, fileref->oldutf8.Length);
        
        olddi = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + oldutf8->Length, ALLOC_TAG);
        if (!olddi) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        olddi->m = 0;
        olddi->n = (UINT16)oldutf8->Length;
        RtlCopyMemory(olddi->name, oldutf8->Buffer, oldutf8->Length);
        
        // delete DIR_ITEM (0x54)
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM,
                                    oldcrc32, olddi, sizeof(DIR_ITEM) - 1 + oldutf8->Length, Batch_DeleteDirItem, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
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
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM, crc32,
                                    di, disize, Batch_DirItem, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF *ir, *ir2;
            
            // delete INODE_REF (0xc)
            
            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + oldutf8->Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir->index = fileref->index;
            ir->n = oldutf8->Length;
            RtlCopyMemory(ir->name, oldutf8->Buffer, ir->n);
        
            if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                        ir, sizeof(INODE_REF) - 1 + ir->n, Batch_DeleteInodeRef, Irp, rollback)) {
                ERR("insert_tree_item_batch failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            // add INODE_REF (0xc)
            
            ir2 = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + fileref->utf8.Length, ALLOC_TAG);
            if (!ir2) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir2->index = fileref->index;
            ir2->n = fileref->utf8.Length;
            RtlCopyMemory(ir2->name, fileref->utf8.Buffer, ir2->n);
        
            if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                        ir2, sizeof(INODE_REF) - 1 + ir2->n, Batch_InodeRef, Irp, rollback)) {
                ERR("insert_tree_item_batch failed\n");
                return STATUS_INTERNAL_ERROR;
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
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                    fileref->index, NULL, 0, Batch_Delete, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        // add DIR_INDEX (0x60)
        
        if (!insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                    fileref->index, di2, disize, Batch_Insert, Irp, rollback)) {
            ERR("insert_tree_item_batch failed\n");
            return STATUS_INTERNAL_ERROR;
        }

        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    }

    fileref->dirty = FALSE;
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL do_write(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY *le, batchlist;
    BOOL cache_changed = FALSE;
#ifdef DEBUG_FLUSH_TIMES
    UINT64 filerefs = 0, fcbs = 0;
    LARGE_INTEGER freq, time1, time2;
#endif
#ifdef DEBUG_WRITE_LOOPS
    UINT loops = 0;
#endif
    
    TRACE("(%p)\n", Vcb);
    
    InitializeListHead(&batchlist);

#ifdef DEBUG_FLUSH_TIMES
    time1 = KeQueryPerformanceCounter(&freq);
#endif
    
    while (!IsListEmpty(&Vcb->dirty_filerefs)) {
        dirty_fileref* dirt;
        
        le = RemoveHeadList(&Vcb->dirty_filerefs);
        
        dirt = CONTAINING_RECORD(le, dirty_fileref, list_entry);
        
        flush_fileref(dirt->fileref, &batchlist, Irp, rollback);
        free_fileref(dirt->fileref);
        ExFreePool(dirt);

#ifdef DEBUG_FLUSH_TIMES
        filerefs++;
#endif
    }
    
    commit_batch_list(Vcb, &batchlist, Irp, rollback);
    
#ifdef DEBUG_FLUSH_TIMES
    time2 = KeQueryPerformanceCounter(NULL);

    ERR("flushed %llu filerefs in %llu (freq = %llu)\n", filerefs, time2.QuadPart - time1.QuadPart, freq.QuadPart);

    time1 = KeQueryPerformanceCounter(&freq);
#endif

    // We process deleted streams first, so we don't run over our xattr
    // limit unless we absolutely have to.
    
    le = Vcb->dirty_fcbs.Flink;
    while (le != &Vcb->dirty_fcbs) {
        dirty_fcb* dirt;
        LIST_ENTRY* le2 = le->Flink;
        
        dirt = CONTAINING_RECORD(le, dirty_fcb, list_entry);
        
        if (dirt->fcb->deleted && dirt->fcb->ads) {
            RemoveEntryList(le);
            
            ExAcquireResourceExclusiveLite(dirt->fcb->Header.Resource, TRUE);
            flush_fcb(dirt->fcb, FALSE, &batchlist, Irp, rollback);
            ExReleaseResourceLite(dirt->fcb->Header.Resource);
            
            free_fcb(dirt->fcb);
            ExFreePool(dirt);

#ifdef DEBUG_FLUSH_TIMES
            fcbs++;
#endif
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
            
            ExAcquireResourceExclusiveLite(dirt->fcb->Header.Resource, TRUE);
            flush_fcb(dirt->fcb, FALSE, &batchlist, Irp, rollback);
            ExReleaseResourceLite(dirt->fcb->Header.Resource);
            free_fcb(dirt->fcb);
            ExFreePool(dirt);

#ifdef DEBUG_FLUSH_TIMES
            fcbs++;
#endif
        }
        
        le = le2;
    }
    
    commit_batch_list(Vcb, &batchlist, Irp, rollback);
    
#ifdef DEBUG_FLUSH_TIMES
    time2 = KeQueryPerformanceCounter(NULL);

    ERR("flushed %llu fcbs in %llu (freq = %llu)\n", filerefs, time2.QuadPart - time1.QuadPart, freq.QuadPart);
#endif

    if (!IsListEmpty(&Vcb->drop_roots)) {
        Status = drop_roots(Vcb, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("drop_roots returned %08x\n", Status);
            return Status;
        }
    }
    
    if (!IsListEmpty(&Vcb->chunks_changed)) {
        Status = update_chunks(Vcb, &batchlist, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("update_chunks returned %08x\n", Status);
            return Status;
        }
    }
    
    commit_batch_list(Vcb, &batchlist, Irp, rollback);
    
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
    
    // make sure we always update the extent tree
    Status = add_root_item_to_cache(Vcb, BTRFS_ROOT_EXTENT, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_root_item_to_cache returned %08x\n", Status);
        return Status;
    }
    
    do {
        Status = add_parents(Vcb, Irp, rollback);
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
    
#ifdef DEBUG_PARANOID
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
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
        
        le = le->Flink;
    }
#endif
    
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

#ifdef DEBUG_STATS
static void print_stats(device_extension* Vcb) {
    ERR("READ STATS:\n");
    ERR("number of reads: %llu\n", Vcb->stats.num_reads);
    ERR("data read: %llu bytes\n", Vcb->stats.data_read);
    ERR("total time taken: %llu\n", Vcb->stats.read_total_time);
    ERR("csum time taken: %llu\n", Vcb->stats.read_csum_time);
    ERR("disk time taken: %llu\n", Vcb->stats.read_disk_time);
    ERR("other time taken: %llu\n", Vcb->stats.read_total_time - Vcb->stats.read_csum_time - Vcb->stats.read_disk_time);
    
    ERR("OPEN STATS:\n");
    ERR("number of opens: %llu\n", Vcb->stats.num_opens);
    ERR("total time taken: %llu\n", Vcb->stats.open_total_time);
    ERR("number of overwrites: %llu\n", Vcb->stats.num_overwrites);
    ERR("total time taken: %llu\n", Vcb->stats.overwrite_total_time);
    ERR("number of creates: %llu\n", Vcb->stats.num_creates);
    ERR("total time taken: %llu\n", Vcb->stats.create_total_time);
    
    RtlZeroMemory(&Vcb->stats, sizeof(debug_stats));
}
#endif

static void do_flush(device_extension* Vcb) {
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    FsRtlEnterFileSystem();

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

#ifdef DEBUG_STATS
    print_stats(Vcb);
#endif

    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, NULL, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);

    ExReleaseResourceLite(&Vcb->tree_lock);

    FsRtlExitFileSystem();
}

void STDCALL flush_thread(void* context) {
    DEVICE_OBJECT* devobj = context;
    device_extension* Vcb = devobj->DeviceExtension;
    LARGE_INTEGER due_time;
    
    ObReferenceObject(devobj);
    
    KeInitializeTimer(&Vcb->flush_thread_timer);
    
    due_time.QuadPart = (UINT64)Vcb->options.flush_interval * -10000000;
    
    KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);
    
    while (TRUE) {
        KeWaitForSingleObject(&Vcb->flush_thread_timer, Executive, KernelMode, FALSE, NULL);

        if (!(devobj->Vpb->Flags & VPB_MOUNTED) || Vcb->removing)
            break;
        
        if (!Vcb->locked)
            do_flush(Vcb);
        
        KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);
    }
    
    ObDereferenceObject(devobj);
    KeCancelTimer(&Vcb->flush_thread_timer);
    
    KeSetEvent(&Vcb->flush_thread_finished, 0, FALSE);
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}
