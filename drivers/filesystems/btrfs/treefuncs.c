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

// #define DEBUG_TREE_LOCKS

enum read_tree_status {
    ReadTreeStatus_Pending,
    ReadTreeStatus_Success,
    ReadTreeStatus_Cancelling,
    ReadTreeStatus_Cancelled,
    ReadTreeStatus_Error,
    ReadTreeStatus_CRCError,
    ReadTreeStatus_MissingDevice
};

struct read_tree_context;

typedef struct {
    struct read_tree_context* context;
    UINT8* buf;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_tree_status status;
} read_tree_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
//     UINT8* buf;
    UINT32 buflen;
    UINT64 num_stripes;
    LONG stripes_left;
    UINT64 type;
    read_tree_stripe* stripes;
} read_tree_context;

enum rollback_type {
    ROLLBACK_INSERT_ITEM,
    ROLLBACK_DELETE_ITEM
};

typedef struct {
    enum rollback_type type;
    void* ptr;
    LIST_ENTRY list_entry;
} rollback_item;

static NTSTATUS STDCALL read_tree_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_tree_stripe* stripe = conptr;
    read_tree_context* context = (read_tree_context*)stripe->context;
    UINT64 i;
    
    if (stripe->status == ReadTreeStatus_Cancelling) {
        stripe->status = ReadTreeStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        tree_header* th = (tree_header*)stripe->buf;
        UINT32 crc32;
        
        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
        
        if (crc32 == *((UINT32*)th->csum)) {
            stripe->status = ReadTreeStatus_Success;
            
            for (i = 0; i < context->num_stripes; i++) {
                if (context->stripes[i].status == ReadTreeStatus_Pending) {
                    context->stripes[i].status = ReadTreeStatus_Cancelling;
                    IoCancelIrp(context->stripes[i].Irp);
                }
            }
            
            goto end;
        } else
            stripe->status = ReadTreeStatus_CRCError;
    } else {
        stripe->status = ReadTreeStatus_Error;
    }
    
end:
    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, FALSE);
    
//     return STATUS_SUCCESS;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS STDCALL read_tree(device_extension* Vcb, UINT64 addr, UINT8* buf) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_tree_context* context;
    UINT64 i/*, type*/, offset;
    NTSTATUS Status;
    device** devices;
    
    // FIXME - make this work with RAID
    
    if (Vcb->log_to_phys_loaded) {
        chunk* c = get_chunk_from_address(Vcb, addr);
        
        if (!c) {
            ERR("get_chunk_from_address failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        ci = c->chunk_item;
        offset = c->offset;
        devices = c->devices;
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
    }
    
//     if (ci->type & BLOCK_FLAG_DUPLICATE) {
//         type = BLOCK_FLAG_DUPLICATE;
//     } else if (ci->type & BLOCK_FLAG_RAID0) {
//         FIXME("RAID0 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID1) {
//         FIXME("RAID1 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID10) {
//         FIXME("RAID10 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID5) {
//         FIXME("RAID5 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID6) {
//         FIXME("RAID6 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else { // SINGLE
//         type = 0;
//     }

    cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_tree_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context, sizeof(read_tree_context));
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    context->stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_tree_stripe) * ci->num_stripes, ALLOC_TAG);
    if (!context->stripes) {
        ERR("out of memory\n");
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context->stripes, sizeof(read_tree_stripe) * ci->num_stripes);
    
    context->buflen = Vcb->superblock.node_size;
    context->num_stripes = ci->num_stripes;
    context->stripes_left = context->num_stripes;
//     context->type = type;
    
    // FIXME - for RAID, check beforehand whether there's enough devices to satisfy request
    
    for (i = 0; i < ci->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        if (!devices[i]) {
            context->stripes[i].status = ReadTreeStatus_MissingDevice;
            context->stripes[i].buf = NULL;
            context->stripes_left--;
        } else {
            context->stripes[i].context = (struct read_tree_context*)context;
            context->stripes[i].buf = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
            
            if (!context->stripes[i].buf) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            context->stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, FALSE);
            
            if (!context->stripes[i].Irp) {
                ERR("IoAllocateIrp failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;
            
            if (devices[i]->devobj->Flags & DO_BUFFERED_IO) {
                FIXME("FIXME - buffered IO\n");
            } else if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                context->stripes[i].Irp->MdlAddress = IoAllocateMdl(context->stripes[i].buf, Vcb->superblock.node_size, FALSE, FALSE, NULL);
                if (!context->stripes[i].Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
                
                MmProbeAndLockPages(context->stripes[i].Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                context->stripes[i].Irp->UserBuffer = context->stripes[i].buf;
            }

            IrpSp->Parameters.Read.Length = Vcb->superblock.node_size;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = addr - offset + cis[i].offset;
            
            context->stripes[i].Irp->UserIosb = &context->stripes[i].iosb;
            
            IoSetCompletionRoutine(context->stripes[i].Irp, read_tree_completion, &context->stripes[i], TRUE, TRUE, TRUE);

            context->stripes[i].status = ReadTreeStatus_Pending;
        }
    }
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status != ReadTreeStatus_MissingDevice) {
            IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
        }
    }

    KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
    
    // FIXME - if checksum error, write good data over bad
    
    // check if any of the stripes succeeded
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadTreeStatus_Success) {
            RtlCopyMemory(buf, context->stripes[i].buf, Vcb->superblock.node_size);
            Status = STATUS_SUCCESS;
            goto exit;
        }
    }
    
    // if not, see if we got a checksum error
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadTreeStatus_CRCError) {
#ifdef _DEBUG
            tree_header* th = (tree_header*)context->stripes[i].buf;
            UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
//             UINT64 j;
            
            WARN("stripe %llu had a checksum error\n", i);
            WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
#endif
            
//             for (j = 0; j < ci->num_stripes; j++) {
//                 WARN("stripe %llu: device = %p, status = %u\n", j, c->devices[j], context->stripes[j].status);
//             }
//             int3;
            
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            goto exit;
        }
    }
    
    // failing that, return the first error we encountered
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadTreeStatus_Error) {
            Status = context->stripes[i].iosb.Status;
            goto exit;
        }
    }
    
    // if we somehow get here, return STATUS_INTERNAL_ERROR
    
    Status = STATUS_INTERNAL_ERROR;

//     for (i = 0; i < ci->num_stripes; i++) {
//         ERR("%llx: status = %u, NTSTATUS = %08x\n", i, context->stripes[i].status, context->stripes[i].iosb.Status);
//     }
exit:

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

NTSTATUS STDCALL _load_tree(device_extension* Vcb, UINT64 addr, root* r, tree** pt, const char* func, const char* file, unsigned int line) {
    UINT8* buf;
    NTSTATUS Status;
    tree_header* th;
    tree* t;
    tree_data* td;
    chunk* c;
    
    TRACE("(%p, %llx)\n", Vcb, addr);
    
    buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_tree(Vcb, addr, buf);
    if (!NT_SUCCESS(Status)) {
        ERR("read_tree returned 0x%08x\n", Status);
        ExFreePool(buf);
        return Status;
    }
    
    th = (tree_header*)buf;
    
    t = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!t) {
        ERR("out of memory\n");
        ExFreePool(buf);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(&t->header, th, sizeof(tree_header));
//     t->address = addr;
//     t->level = th->level;
    t->refcount = 1;
    t->has_address = TRUE;
    t->Vcb = Vcb;
    t->parent = NULL;
    t->root = r;
//     t->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    t->paritem = NULL;
    t->size = 0;
    t->new_address = 0;
    t->has_new_address = FALSE;
#ifdef DEBUG_TREE_REFCOUNTS   
#ifdef DEBUG_LONG_MESSAGES
    _debug_message(func, file, line, "loaded tree %p (%llx)\n", t, addr);
#else
    _debug_message(func, "loaded tree %p (%llx)\n", t, addr);
#endif
#endif
    
    c = get_chunk_from_address(Vcb, addr);
    
    if (c)
        t->flags = c->chunk_item->type;
    else
        t->flags = 0;
    
//     ExInitializeResourceLite(&t->nonpaged->load_tree_lock);
    
//     t->items = ExAllocatePoolWithTag(PagedPool, num_items * sizeof(tree_data), ALLOC_TAG);
    InitializeListHead(&t->itemlist);
    
    if (t->header.level == 0) { // leaf node
        leaf_node* ln = (leaf_node*)(buf + sizeof(tree_header));
        unsigned int i;
        
        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
            if (!td) {
                ERR("out of memory\n");
                ExFreePool(buf);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            td->key = ln[i].key;
//             TRACE("load_tree: leaf item %u (%x,%x,%x)\n", i, (UINT32)ln[i].key.obj_id, ln[i].key.obj_type, (UINT32)ln[i].key.offset);
            
            if (ln[i].size > 0) {
                td->data = ExAllocatePoolWithTag(PagedPool, ln[i].size, ALLOC_TAG);
                if (!td->data) {
                    ERR("out of memory\n");
                    ExFreePool(buf);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(td->data, buf + sizeof(tree_header) + ln[i].offset, ln[i].size);
            } else
                td->data = NULL;
            
            td->size = ln[i].size;
            td->ignore = FALSE;
            td->inserted = FALSE;
            
            InsertTailList(&t->itemlist, &td->list_entry);
            
            t->size += ln[i].size;
        }
        
        t->size += t->header.num_items * sizeof(leaf_node);
    } else {
        internal_node* in = (internal_node*)(buf + sizeof(tree_header));
        unsigned int i;
        
        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
            if (!td) {
                ERR("out of memory\n");
                ExFreePool(buf);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            td->key = in[i].key;
//             TRACE("load_tree: internal item %u (%x,%x,%x)\n", i, (UINT32)in[i].key.obj_id, in[i].key.obj_type, (UINT32)in[i].key.offset);
            
            td->treeholder.address = in[i].address;
            td->treeholder.generation = in[i].generation;
            td->treeholder.tree = NULL;
            init_tree_holder(&td->treeholder);
//             td->treeholder.nonpaged->status = tree_holder_unloaded;
            td->ignore = FALSE;
            td->inserted = FALSE;
            
            InsertTailList(&t->itemlist, &td->list_entry);
        }
        
        t->size = t->header.num_items * sizeof(internal_node);
    }
    
    ExFreePool(buf);
    
    InterlockedIncrement(&Vcb->open_trees);
    InsertTailList(&Vcb->trees, &t->list_entry);
    
    TRACE("returning %p\n", t);
    
    *pt = t;
    
    return STATUS_SUCCESS;
}

static tree* free_tree2(tree* t, const char* func, const char* file, unsigned int line) {
    LONG rc;
    LIST_ENTRY* le;
    tree_data* td;
    tree* par;
    
#ifdef DEBUG_TREE_REFCOUNTS
    TRACE("(%p)\n", t);
#endif
    
    par = t->parent;
    
//     if (par) ExAcquireResourceExclusiveLite(&par->nonpaged->load_tree_lock, TRUE);
    
    rc = InterlockedDecrement(&t->refcount);
    
#ifdef DEBUG_TREE_REFCOUNTS
#ifdef DEBUG_LONG_MESSAGES
    _debug_message(func, file, line, "tree %p: refcount decreased to %i (free_tree2)\n", t, rc);
#else
    _debug_message(func, "tree %p: refcount decreased to %i (free_tree2)\n", t, rc);
#endif
#endif
    
    if (rc < 0) {
        ERR("error - negative refcount (%i)\n", rc);
        int3;
    }
    
    if (rc == 0) {
        root* r = t->root;
        if (r && r->treeholder.tree != t)
            r = NULL;
        
//         if (r) {
//             FsRtlEnterFileSystem();
//             ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, TRUE);
//         }
        
        if (par) {
            if (t->paritem)
                t->paritem->treeholder.tree = NULL;
            
//             ExReleaseResourceLite(&par->nonpaged->load_tree_lock);
        }
        
        if (t->parent)
            t->parent = free_tree2(t->parent, func, file, line);
        
//         ExDeleteResourceLite(&t->nonpaged->load_tree_lock);
        
//         ExFreePool(t->nonpaged);
        
        while (!IsListEmpty(&t->itemlist)) {
            le = RemoveHeadList(&t->itemlist);
            td = CONTAINING_RECORD(le, tree_data, list_entry);
            
            if (t->header.level == 0 && td->data)
                ExFreePool(td->data);
             
            ExFreePool(td);
        }
        
        InterlockedDecrement(&t->Vcb->open_trees);
        RemoveEntryList(&t->list_entry);
        
        if (r) {
            r->treeholder.tree = NULL;
//             ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
//             FsRtlExitFileSystem();
        }
        
        ExFreePool(t);

        return NULL;
    } else {
//         if (par) ExReleaseResourceLite(&par->nonpaged->load_tree_lock);
    }
    
    return t;
}

NTSTATUS STDCALL _do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, BOOL* loaded, const char* func, const char* file, unsigned int line) {
//     KIRQL irql;
//     tree_holder_nonpaged* thnp = th->nonpaged;
    BOOL ret;
    
//     ExAcquireResourceExclusiveLite(&thnp->lock, TRUE);
    ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, TRUE);
    
//     KeAcquireSpinLock(&thnp->spin_lock, &irql);
//     
//     if (thnp->status == tree_header_loading) {
//         KeReleaseSpinLock(&thnp->spin_lock, irql);
//         
//         // FIXME - wait for Event
//     } else if (thnp->status == tree_header_unloaded || thnp->status == tree_header_unloading) {
//         if (thnp->status == tree_header_unloading) {
//             KeReleaseSpinLock(&thnp->spin_lock, irql);
//             // FIXME - wait for Event
//         }
//         
//         // FIXME - change status
//         thnp->status = tree_header_loading;
//         KeReleaseSpinLock(&thnp->spin_lock, irql);
//         
//         // FIXME - load
//         // FIXME - change status
//         // FIXME - trigger event
//     } else if (thnp->status == tree_header_loaded) {
//         _increase_tree_rc(th->tree, func, file, line);
//         KeReleaseSpinLock(&thnp->spin_lock, irql);
//         
//         ret = FALSE;
//     }

    if (!th->tree) {
        NTSTATUS Status;
        
        Status = _load_tree(Vcb, th->address, r, &th->tree, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("load_tree returned %08x\n", Status);
            ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
            return Status;
        }
        
        th->tree->parent = t;
        th->tree->paritem = td;
        
        ret = TRUE;
    } else {
        _increase_tree_rc(th->tree, func, file, line);
        
        ret = FALSE;
    }
    
//     KeReleaseSpinLock(&thnp->spin_lock, irql);
    
//     ExReleaseResourceLite(&thnp->lock);
    ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
    
    *loaded = ret;
    
    return STATUS_SUCCESS;
}

tree* STDCALL _free_tree(tree* t, const char* func, const char* file, unsigned int line) {
    tree* ret;
    root* r = t->root;
    
    ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, TRUE);

    ret = free_tree2(t, func, file, line);

    ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
    
    return ret;
}

static __inline tree_data* first_item(tree* t) {
    LIST_ENTRY* le = t->itemlist.Flink;
    
    if (le == &t->itemlist)
        return NULL;
    
    return CONTAINING_RECORD(le, tree_data, list_entry);
}

static __inline tree_data* prev_item(tree* t, tree_data* td) {
    LIST_ENTRY* le = td->list_entry.Blink;
    
    if (le == &t->itemlist)
        return NULL;
    
    return CONTAINING_RECORD(le, tree_data, list_entry);
}

static __inline tree_data* next_item(tree* t, tree_data* td) {
    LIST_ENTRY* le = td->list_entry.Flink;
    
    if (le == &t->itemlist)
        return NULL;
    
    return CONTAINING_RECORD(le, tree_data, list_entry);
}

static NTSTATUS STDCALL find_item_in_tree(device_extension* Vcb, tree* t, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, const char* func, const char* file, unsigned int line) {
    int cmp;
    tree_data *td, *lasttd;
    
    TRACE("(%p, %p, %p, %p, %u)\n", Vcb, t, tp, searchkey, ignore);
    
    cmp = 1;
    td = first_item(t);
    lasttd = NULL;
    
    if (!td) return STATUS_INTERNAL_ERROR;
    
    do {
        cmp = keycmp(searchkey, &td->key);
//         TRACE("(%u) comparing (%x,%x,%x) to (%x,%x,%x) - %i (ignore = %s)\n", t->header.level, (UINT32)searchkey->obj_id, searchkey->obj_type, (UINT32)searchkey->offset, (UINT32)td->key.obj_id, td->key.obj_type, (UINT32)td->key.offset, cmp, td->ignore ? "TRUE" : "FALSE");
        if (cmp == 1) {
            lasttd = td;
            td = next_item(t, td);
        }

        if (t->header.level == 0 && cmp == 0 && !ignore && td && td->ignore) {
            tree_data* origtd = td;
            
            while (td && td->ignore)
                td = next_item(t, td);
            
            if (td) {
                cmp = keycmp(searchkey, &td->key);
                
                if (cmp != 0) {
                    td = origtd;
                    cmp = 0;
                }
            } else
                td = origtd;
        }
    } while (td && cmp == 1);
    
    if ((cmp == -1 || !td) && lasttd)
        td = lasttd;
    
    if (t->header.level == 0) {
        if (td->ignore && !ignore) {
            traverse_ptr oldtp;
            
            oldtp.tree = t;
            oldtp.item = td;
            _increase_tree_rc(t, func, file, line);
            
            while (_find_prev_item(Vcb, &oldtp, tp, TRUE, func, file, line)) {
                if (!tp->item->ignore)
                    return STATUS_SUCCESS;
                
                free_traverse_ptr(&oldtp);
                oldtp = *tp;
            }
            
            // if no valid entries before where item should be, look afterwards instead
            
            oldtp.tree = t;
            oldtp.item = td;
            _increase_tree_rc(t, func, file, line);
            
            while (_find_next_item(Vcb, &oldtp, tp, TRUE, func, file, line)) {
                if (!tp->item->ignore)
                    return STATUS_SUCCESS;
                
                free_traverse_ptr(&oldtp);
                oldtp = *tp;
            }
            
            return STATUS_INTERNAL_ERROR;
        } else {
            tp->tree = t;
            _increase_tree_rc(t, func, file, line);
            tp->item = td;
            
            add_to_tree_cache(Vcb, t, FALSE);
        }
        
        return STATUS_SUCCESS;
    } else {
        NTSTATUS Status;
        BOOL loaded;
        
        while (td && td->treeholder.tree && IsListEmpty(&td->treeholder.tree->itemlist)) {
            td = prev_item(t, td);
        }
        
        if (!td)
            return STATUS_INTERNAL_ERROR;
        
//         if (i > 0)
//             TRACE("entering tree from (%x,%x,%x) to (%x,%x,%x) (%p)\n", (UINT32)t->items[i].key.obj_id, t->items[i].key.obj_type, (UINT32)t->items[i].key.offset, (UINT32)t->items[i+1].key.obj_id, t->items[i+1].key.obj_type, (UINT32)t->items[i+1].key.offset, t->items[i].tree);
        
        Status = _do_load_tree(Vcb, &td->treeholder, t->root, t, td, &loaded, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
        
        if (loaded)
            _increase_tree_rc(t, func, file, line);
        
        Status = find_item_in_tree(Vcb, td->treeholder.tree, tp, searchkey, ignore, func, file, line);
        
        td->treeholder.tree = _free_tree(td->treeholder.tree, func, file, line);
        TRACE("tree now %p\n", td->treeholder.tree);
        
        return Status;
    }
}

NTSTATUS STDCALL _find_item(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, const char* func, const char* file, unsigned int line) {
    NTSTATUS Status;
    BOOL loaded;
//     KIRQL irql;
    
    TRACE("(%p, %p, %p, %p)\n", Vcb, r, tp, searchkey);
    
    Status = _do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, &loaded, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return Status;
    }

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item_in_tree returned %08x\n", Status);
    }

    _free_tree(r->treeholder.tree, func, file, line);
    
// #ifdef DEBUG_PARANOID
//     if (b && !ignore && tp->item->ignore) {
//         ERR("error - returning ignored item\n");
//         int3;
//     }
// #endif
    
    return Status;
}

void STDCALL _free_traverse_ptr(traverse_ptr* tp, const char* func, const char* file, unsigned int line) {
    if (tp->tree) {
        tp->tree = free_tree2(tp->tree, func, file, line);
    }
}

BOOL STDCALL _find_next_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* next_tp, BOOL ignore, const char* func, const char* file, unsigned int line) {
    tree* t;
    tree_data *td, *next;
    NTSTATUS Status;
    BOOL loaded;
    
    next = next_item(tp->tree, tp->item);
    
    if (!ignore) {
        while (next && next->ignore)
            next = next_item(tp->tree, next);
    }
    
    if (next) {
        next_tp->tree = tp->tree;
        _increase_tree_rc(next_tp->tree, func, file, line);
        next_tp->item = next;
        
#ifdef DEBUG_PARANOID
        if (!ignore && next_tp->item->ignore) {
            ERR("error - returning ignored item\n");
            int3;
        }
#endif
        
        return TRUE;
    }
    
    if (!tp->tree->parent)
        return FALSE;
    
    t = tp->tree;
    do {
        if (t->parent) {
            td = next_item(t->parent, t->paritem);
            
            if (td) break;
        }
        
        t = t->parent;
    } while (t);
    
    if (!t)
        return FALSE;
    
    Status = _do_load_tree(Vcb, &td->treeholder, t->parent->root, t->parent, td, &loaded, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return FALSE;
    }
    
    if (loaded)
        _increase_tree_rc(t->parent, func, file, line);
    
    t = td->treeholder.tree;
    
    while (t->header.level != 0) {
        tree_data* fi;
       
        fi = first_item(t);
        
        Status = _do_load_tree(Vcb, &fi->treeholder, t->parent->root, t, fi, &loaded, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return FALSE;
        }
        
        t = fi->treeholder.tree;
    }
    
    next_tp->tree = t;
    next_tp->item = first_item(t);
    
    if (!ignore && next_tp->item->ignore) {
        traverse_ptr ntp2;
        BOOL b;
        
        while ((b = _find_next_item(Vcb, next_tp, &ntp2, TRUE, func, file, line))) {
            _free_traverse_ptr(next_tp, func, file, line);
            *next_tp = ntp2;
            
            if (!next_tp->item->ignore)
                break;
        }
        
        if (!b) {
            _free_traverse_ptr(next_tp, func, file, line);
            return FALSE;
        }
    }
    
    add_to_tree_cache(Vcb, t, FALSE);
    
#ifdef DEBUG_PARANOID
    if (!ignore && next_tp->item->ignore) {
        ERR("error - returning ignored item\n");
        int3;
    }
#endif
    
    return TRUE;
}

static __inline tree_data* last_item(tree* t) {
    LIST_ENTRY* le = t->itemlist.Blink;
    
    if (le == &t->itemlist)
        return NULL;
    
    return CONTAINING_RECORD(le, tree_data, list_entry);
}

BOOL STDCALL _find_prev_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* prev_tp, BOOL ignore, const char* func, const char* file, unsigned int line) {
    tree* t;
    tree_data* td;
    NTSTATUS Status;
    BOOL loaded;
    
    // FIXME - support ignore flag
    if (prev_item(tp->tree, tp->item)) {
        prev_tp->tree = tp->tree;
        _increase_tree_rc(prev_tp->tree, func, file, line);
        prev_tp->item = prev_item(tp->tree, tp->item);

        return TRUE;
    }
    
    if (!tp->tree->parent)
        return FALSE;
    
    t = tp->tree;
    while (t && (!t->parent || !prev_item(t->parent, t->paritem))) {
        t = t->parent;
    }
    
    if (!t)
        return FALSE;
    
    td = prev_item(t->parent, t->paritem);
    
    Status = _do_load_tree(Vcb, &td->treeholder, t->parent->root, t, td, &loaded, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return FALSE;
    }
    
    if (loaded)
        _increase_tree_rc(t->parent, func, file, line);
    
    t = td->treeholder.tree;
    
    while (t->header.level != 0) {
        tree_data* li;
        
        li = last_item(t);
        
        Status = _do_load_tree(Vcb, &li->treeholder, t->parent->root, t, li, &loaded, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return FALSE;
        }
        
        t = li->treeholder.tree;
    }
    
    add_to_tree_cache(Vcb, t, FALSE);
    
    prev_tp->tree = t;
    prev_tp->item = last_item(t);
    
    return TRUE;
}

// static void free_tree_holder(tree_holder* th) {
//     root* r = th->tree->root;
//     
// //     ExAcquireResourceExclusiveLite(&th->nonpaged->lock, TRUE);
//     ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, TRUE);
// 
//     free_tree2(th->tree, funcname, __FILE__, __LINE__);
// 
// //     ExReleaseResourceLite(&th->nonpaged->lock);
//     ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
// }

void STDCALL free_tree_cache(LIST_ENTRY* tc) {
    LIST_ENTRY* le;
    tree_cache* tc2;
    root* r;

    while (tc->Flink != tc) {
        le = tc->Flink;
        tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        r = tc2->tree->root;
        
        ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, TRUE);
        
        while (le != tc) {
            LIST_ENTRY* nextle = le->Flink;
            tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
            
            if (tc2->tree->root == r) {
                tree* nt;
                BOOL top = !tc2->tree->paritem;
                
                nt = free_tree2(tc2->tree, funcname, __FILE__, __LINE__);
                if (top && !nt && r->treeholder.tree == tc2->tree)
                    r->treeholder.tree = NULL;
                
                RemoveEntryList(&tc2->list_entry);
                ExFreePool(tc2);
            }
            
            le = nextle;
        }
        
        ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
    }
}

void STDCALL add_to_tree_cache(device_extension* Vcb, tree* t, BOOL write) {
    LIST_ENTRY* le;
    tree_cache* tc2;
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->tree == t) {
            if (write && !tc2->write) {
                Vcb->write_trees++;
                tc2->write = TRUE;
            }
            return;
        }
        
        le = le->Flink;
    }
    
    tc2 = ExAllocatePoolWithTag(PagedPool, sizeof(tree_cache), ALLOC_TAG);
    if (!tc2) {
        ERR("out of memory\n");
        return;
    }
    
    TRACE("adding %p to tree cache\n", t);
    
    tc2->tree = t;
    tc2->write = write;
    increase_tree_rc(t);
    InsertTailList(&Vcb->tree_cache, &tc2->list_entry);

//     print_trees(tc);
}

static void add_rollback(LIST_ENTRY* rollback, enum rollback_type type, void* ptr) {
    rollback_item* ri;
    
    ri = ExAllocatePoolWithTag(PagedPool, sizeof(rollback_item), ALLOC_TAG);
    if (!ri) {
        ERR("out of memory\n");
        return;
    }
    
    ri->type = type;
    ri->ptr = ptr;
    InsertTailList(rollback, &ri->list_entry);
}

BOOL STDCALL insert_tree_item(device_extension* Vcb, root* r, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, UINT32 size, traverse_ptr* ptp, LIST_ENTRY* rollback) {
    traverse_ptr tp;
    KEY searchkey;
    int cmp;
    tree_data *td, *paritem;
    tree* t;
#ifdef _DEBUG
    LIST_ENTRY* le;
    KEY firstitem = {0xcccccccccccccccc,0xcc,0xcccccccccccccccc};
#endif
    traverse_ptr* tp2;
    BOOL success = FALSE;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %llx, %x, %llx, %p, %x, %p, %p)\n", Vcb, r, obj_id, obj_type, offset, data, size, ptp, rollback);
    
    searchkey.obj_id = obj_id;
    searchkey.obj_type = obj_type;
    searchkey.offset = offset;
    
    Status = find_item(Vcb, r, &tp, &searchkey, TRUE);
    if (!NT_SUCCESS(Status)) {
        if (r) {
            if (!r->treeholder.tree) {
                BOOL loaded;
                
                Status = do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, &loaded);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("do_load_tree returned %08x\n", Status);
                    goto end;
                }
            }
            
            if (r->treeholder.tree && r->treeholder.tree->header.num_items == 0) {
                tp.tree = r->treeholder.tree;
                tp.item = NULL;
            } else {
                ERR("error: unable to load tree for root %llx\n", r->id);
                goto end;
            }
        } else {
            ERR("error: find_item returned %08x\n", Status);
            goto end;
        }
    }
    
    TRACE("tp.item = %p\n", tp.item);
    
    if (tp.item) {
        TRACE("tp.item->key = %p\n", &tp.item->key);
        cmp = keycmp(&searchkey, &tp.item->key);
        
        if (cmp == 0 && !tp.item->ignore) { // FIXME - look for all items of the same key to make sure none are non-ignored
            ERR("error: key (%llx,%x,%llx) already present\n", obj_id, obj_type, offset);
            free_traverse_ptr(&tp);
            goto end;
        }
    } else
        cmp = -1;
    
    td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
    if (!td) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        goto end;
    }
    
    td->key = searchkey;
    td->size = size;
    td->data = data;
    td->ignore = FALSE;
    td->inserted = TRUE;
    
#ifdef _DEBUG
    le = tp.tree->itemlist.Flink;
    while (le != &tp.tree->itemlist) {
        tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);
        firstitem = td2->key;
        break;
    }
    
    TRACE("inserting %llx,%x,%llx into tree beginning %llx,%x,%llx (num_items %x)\n", obj_id, obj_type, offset, firstitem.obj_id, firstitem.obj_type, firstitem.offset, tp.tree->header.num_items);
#endif
    
    if (cmp == -1) { // very first key in root
        InsertHeadList(&tp.tree->itemlist, &td->list_entry);

        paritem = tp.tree->paritem;
        while (paritem) {
//             ERR("paritem = %llx,%x,%llx, tp.item->key = %llx,%x,%llx\n", paritem->key.obj_id, paritem->key.obj_type, paritem->key.offset, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            if (!keycmp(&paritem->key, &tp.item->key)) {
                paritem->key = searchkey;
            } else
                break;
            
            paritem = paritem->treeholder.tree->paritem;
        }
        
    } else {          
        InsertAfter(&tp.tree->itemlist, &td->list_entry, &tp.item->list_entry); // FIXME - we don't need this
    }
    
    tp.tree->header.num_items++;
    tp.tree->size += size + sizeof(leaf_node);
//     ERR("tree %p, num_items now %x\n", tp.tree, tp.tree->header.num_items);
//     ERR("size now %x\n", tp.tree->size);
    
    add_to_tree_cache(Vcb, tp.tree, TRUE);
    
    if (!ptp)
        free_traverse_ptr(&tp);
    else
        *ptp = tp;
    
    t = tp.tree;
    while (t) {
        if (t->paritem && t->paritem->ignore) {
            t->paritem->ignore = FALSE;
            t->parent->header.num_items++;
            t->parent->size += sizeof(internal_node);
            
            // FIXME - do we need to add a rollback entry here?
        }

        t->header.generation = Vcb->superblock.generation;
        t = t->parent;
    }
    
    // FIXME - free this correctly
    
    tp2 = ExAllocatePoolWithTag(PagedPool, sizeof(traverse_ptr), ALLOC_TAG);
    if (!tp2) {
        ERR("out of memory\n");
        goto end;
    }
    
    tp2->tree = tp.tree;
    tp2->item = td;
    
    add_rollback(rollback, ROLLBACK_INSERT_ITEM, tp2);
    
    success = TRUE;

end:
    return success;
}

static __inline tree_data* first_valid_item(tree* t) {
    LIST_ENTRY* le = t->itemlist.Flink;
    
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
        
        if (!td->ignore)
            return td;
        
        le = le->Flink;
    }
        
    return NULL;
}

void STDCALL delete_tree_item(device_extension* Vcb, traverse_ptr* tp, LIST_ENTRY* rollback) {
    tree* t;
    UINT64 gen;
    traverse_ptr* tp2;

    TRACE("deleting item %llx,%x,%llx (ignore = %s)\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, tp->item->ignore ? "TRUE" : "FALSE");
    
#ifdef DEBUG_PARANOID
    if (tp->item->ignore) {
        ERR("trying to delete already-deleted item %llx,%x,%llx\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
        int3;
    }
#endif

    tp->item->ignore = TRUE;
    
    add_to_tree_cache(Vcb, tp->tree, TRUE);
    
    tp->tree->header.num_items--;
    
    if (tp->tree->header.level == 0)
        tp->tree->size -= sizeof(leaf_node) + tp->item->size;
    else
        tp->tree->size -= sizeof(internal_node);
    
    gen = tp->tree->Vcb->superblock.generation;
    
    t = tp->tree;
    while (t) {
        t->header.generation = gen;
        t = t->parent;
    }
    
    tp2 = ExAllocatePoolWithTag(PagedPool, sizeof(traverse_ptr), ALLOC_TAG);
    if (!tp2) {
        ERR("out of memory\n");
        return;
    }
    
    tp2->tree = tp->tree;
    tp2->item = tp->item;

    add_rollback(rollback, ROLLBACK_DELETE_ITEM, tp2);
}

void clear_rollback(LIST_ENTRY* rollback) {
    rollback_item* ri;
    
    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveHeadList(rollback);
        ri = CONTAINING_RECORD(le, rollback_item, list_entry);
        
        switch (ri->type) {
            case ROLLBACK_INSERT_ITEM:
            case ROLLBACK_DELETE_ITEM:
                ExFreePool(ri->ptr);
                break;
        }
        
        ExFreePool(ri);
    }
}

void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback) {
    rollback_item* ri;
    
    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveHeadList(rollback);
        ri = CONTAINING_RECORD(le, rollback_item, list_entry);
        
        switch (ri->type) {
            case ROLLBACK_INSERT_ITEM:
            {
                traverse_ptr* tp = ri->ptr;
                
                if (!tp->item->ignore) {
                    tp->item->ignore = TRUE;
                    tp->tree->header.num_items--;
                
                    if (tp->tree->header.level == 0)
                        tp->tree->size -= sizeof(leaf_node) + tp->item->size;
                    else
                        tp->tree->size -= sizeof(internal_node);
                }
                
                ExFreePool(tp);
                break;
            }
                
            case ROLLBACK_DELETE_ITEM:
            {
                traverse_ptr* tp = ri->ptr;
                
                if (tp->item->ignore) {
                    tp->item->ignore = FALSE;
                    tp->tree->header.num_items++;
                
                    if (tp->tree->header.level == 0)
                        tp->tree->size += sizeof(leaf_node) + tp->item->size;
                    else
                        tp->tree->size += sizeof(internal_node);
                }
                
                ExFreePool(tp);
                break;
            }
        }
        
        ExFreePool(ri);
    }
}
