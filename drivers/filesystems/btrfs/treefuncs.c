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

typedef struct {
    enum rollback_type type;
    void* ptr;
    LIST_ENTRY list_entry;
} rollback_item;

NTSTATUS STDCALL _load_tree(device_extension* Vcb, UINT64 addr, root* r, tree** pt, tree* parent, PIRP Irp, const char* func, const char* file, unsigned int line) {
    UINT8* buf;
    NTSTATUS Status;
    tree_header* th;
    tree* t;
    tree_data* td;
    chunk* c;
    shared_data* sd;
    
    TRACE("(%p, %llx)\n", Vcb, addr);
    
    buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_data(Vcb, addr, Vcb->superblock.node_size, NULL, TRUE, buf, &c, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("read_data returned 0x%08x\n", Status);
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
    t->has_address = TRUE;
    t->Vcb = Vcb;
    t->parent = NULL;
    t->root = r;
//     t->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    t->paritem = NULL;
    t->size = 0;
    t->new_address = 0;
    t->has_new_address = FALSE;
    t->write = FALSE;
    
    if (c)
        t->flags = c->chunk_item->type;
    else
        t->flags = 0;
    
//     ExInitializeResourceLite(&t->nonpaged->load_tree_lock);
    
//     t->items = ExAllocatePoolWithTag(PagedPool, num_items * sizeof(tree_data), ALLOC_TAG);
    InitializeListHead(&t->itemlist);
    
    if (t->header.flags & HEADER_FLAG_SHARED_BACKREF || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
        sd = ExAllocatePoolWithTag(NonPagedPool, sizeof(shared_data), ALLOC_TAG);
        if (!sd) {
            ERR("out of memory\n");
            ExFreePool(buf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        sd->address = addr;
        sd->parent = parent ? parent->header.address : addr;
        InitializeListHead(&sd->entries);
        
        ExInterlockedInsertTailList(&Vcb->shared_extents, &sd->list_entry, &Vcb->shared_extents_lock);
    }
    
    if (t->header.level == 0) { // leaf node
        leaf_node* ln = (leaf_node*)(buf + sizeof(tree_header));
        unsigned int i;
        
        if ((t->header.num_items * sizeof(leaf_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %llx has more items than expected (%x)\n", t->header.num_items);
            ExFreePool(buf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
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
            
            if ((t->header.flags & HEADER_FLAG_SHARED_BACKREF || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) &&
                ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA)) {
                EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                
                if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                    
                    if (ed2->size != 0) {
                        LIST_ENTRY* le;
                        BOOL found = FALSE;
                        
                        TRACE("shared extent %llx,%llx\n", ed2->address, ed2->size);
                        
                        le = sd->entries.Flink;
                        while (le != &sd->entries) {
                            shared_data_entry* sde = CONTAINING_RECORD(le, shared_data_entry, list_entry);
                            
                            if (sde->address == ed2->address && sde->size == ed2->size && sde->edr.root == t->header.tree_id &&
                                sde->edr.objid == ln[i].key.obj_id && sde->edr.offset == ln[i].key.offset - ed2->offset) {
                                sde->edr.count++;
                                found = TRUE;
                                break;
                            }
                            
                            le = le->Flink;
                        }
                        
                        if (!found) {
                            shared_data_entry* sde = ExAllocatePoolWithTag(PagedPool, sizeof(shared_data_entry), ALLOC_TAG);
                            
                            if (!sde) {
                                ERR("out of memory\n");
                                ExFreePool(buf);
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }
                            
                            sde->address = ed2->address;
                            sde->size = ed2->size;
                            sde->edr.root = t->header.tree_id;
                            sde->edr.objid = ln[i].key.obj_id;
                            sde->edr.offset = ln[i].key.offset - ed2->offset;
                            sde->edr.count = 1;
                            
                            InsertTailList(&sd->entries, &sde->list_entry);
                        }
                    }
                }
            }
            
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
        
        if ((t->header.num_items * sizeof(internal_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %llx has more items than expected (%x)\n", t->header.num_items);
            ExFreePool(buf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
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
    LIST_ENTRY* le;
    tree_data* td;
    tree* par;
    root* r = t->root;
    
    par = t->parent;
    
//     if (par) ExAcquireResourceExclusiveLite(&par->nonpaged->load_tree_lock, TRUE);
    
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
}

NTSTATUS STDCALL _do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, BOOL* loaded, PIRP Irp,
                               const char* func, const char* file, unsigned int line) {
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
        
        Status = _load_tree(Vcb, th->address, r, &th->tree, t, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("load_tree returned %08x\n", Status);
            ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
            return Status;
        }
        
        th->tree->parent = t;
        th->tree->paritem = td;
        
        ret = TRUE;
    } else
        ret = FALSE;
    
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

static NTSTATUS STDCALL find_item_in_tree(device_extension* Vcb, tree* t, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, PIRP Irp,
                                          const char* func, const char* file, unsigned int line) {
    int cmp;
    tree_data *td, *lasttd;
    
    TRACE("(%p, %p, %p, %p, %u)\n", Vcb, t, tp, searchkey, ignore);
    
    cmp = 1;
    td = first_item(t);
    lasttd = NULL;
    
    if (!td) return STATUS_NOT_FOUND;
    
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
            
            while (_find_prev_item(Vcb, &oldtp, tp, TRUE, Irp, func, file, line)) {
                if (!tp->item->ignore)
                    return STATUS_SUCCESS;
                
                oldtp = *tp;
            }
            
            // if no valid entries before where item should be, look afterwards instead
            
            oldtp.tree = t;
            oldtp.item = td;
            
            while (_find_next_item(Vcb, &oldtp, tp, TRUE, Irp, func, file, line)) {
                if (!tp->item->ignore)
                    return STATUS_SUCCESS;
                
                oldtp = *tp;
            }
            
            return STATUS_NOT_FOUND;
        } else {
            tp->tree = t;
            tp->item = td;
        }
        
        return STATUS_SUCCESS;
    } else {
        NTSTATUS Status;
        BOOL loaded;
        
        while (td && td->treeholder.tree && IsListEmpty(&td->treeholder.tree->itemlist)) {
            td = prev_item(t, td);
        }
        
        if (!td)
            return STATUS_NOT_FOUND;
        
//         if (i > 0)
//             TRACE("entering tree from (%x,%x,%x) to (%x,%x,%x) (%p)\n", (UINT32)t->items[i].key.obj_id, t->items[i].key.obj_type, (UINT32)t->items[i].key.offset, (UINT32)t->items[i+1].key.obj_id, t->items[i+1].key.obj_type, (UINT32)t->items[i+1].key.offset, t->items[i].tree);
        
        Status = _do_load_tree(Vcb, &td->treeholder, t->root, t, td, &loaded, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
        
        Status = find_item_in_tree(Vcb, td->treeholder.tree, tp, searchkey, ignore, Irp, func, file, line);
        
        return Status;
    }
}

NTSTATUS STDCALL _find_item(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, PIRP Irp, const char* func, const char* file, unsigned int line) {
    NTSTATUS Status;
    BOOL loaded;
//     KIRQL irql;
    
    TRACE("(%p, %p, %p, %p)\n", Vcb, r, tp, searchkey);
    
    if (!r->treeholder.tree) {
        Status = _do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, &loaded, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
    }

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, Irp, func, file, line);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
        ERR("find_item_in_tree returned %08x\n", Status);
    }
    
// #ifdef DEBUG_PARANOID
//     if (b && !ignore && tp->item->ignore) {
//         ERR("error - returning ignored item\n");
//         int3;
//     }
// #endif
    
    return Status;
}

BOOL STDCALL _find_next_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* next_tp, BOOL ignore, PIRP Irp,
                             const char* func, const char* file, unsigned int line) {
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
    
    Status = _do_load_tree(Vcb, &td->treeholder, t->parent->root, t->parent, td, &loaded, Irp, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return FALSE;
    }
    
    t = td->treeholder.tree;
    
    while (t->header.level != 0) {
        tree_data* fi;
       
        fi = first_item(t);
        
        Status = _do_load_tree(Vcb, &fi->treeholder, t->parent->root, t, fi, &loaded, Irp, func, file, line);
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
        
        while ((b = _find_next_item(Vcb, next_tp, &ntp2, TRUE, Irp, func, file, line))) {
            *next_tp = ntp2;
            
            if (!next_tp->item->ignore)
                break;
        }
        
        if (!b)
            return FALSE;
    }
    
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

BOOL STDCALL _find_prev_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* prev_tp, BOOL ignore, PIRP Irp,
                             const char* func, const char* file, unsigned int line) {
    tree* t;
    tree_data* td;
    NTSTATUS Status;
    BOOL loaded;
    
    // FIXME - support ignore flag
    if (prev_item(tp->tree, tp->item)) {
        prev_tp->tree = tp->tree;
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
    
    Status = _do_load_tree(Vcb, &td->treeholder, t->parent->root, t, td, &loaded, Irp, func, file, line);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return FALSE;
    }
    
    t = td->treeholder.tree;
    
    while (t->header.level != 0) {
        tree_data* li;
        
        li = last_item(t);
        
        Status = _do_load_tree(Vcb, &li->treeholder, t->parent->root, t, li, &loaded, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return FALSE;
        }
        
        t = li->treeholder.tree;
    }
    
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

void free_trees_root(device_extension* Vcb, root* r) {
    LIST_ENTRY* le;
    UINT8 level;
    
    for (level = 0; level <= 255; level++) {
        BOOL empty = TRUE;
        
        le = Vcb->trees.Flink;
        
        while (le != &Vcb->trees) {
            LIST_ENTRY* nextle = le->Flink;
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            
            if (t->root == r) {
                if (t->header.level == level) {
                    BOOL top = !t->paritem;
                    
                    empty = FALSE;
                    
                    free_tree2(t, funcname, __FILE__, __LINE__);
                    if (top && r->treeholder.tree == t)
                        r->treeholder.tree = NULL;
                    
                    if (IsListEmpty(&Vcb->trees))
                        return;
                } else if (t->header.level > level)
                    empty = FALSE;
            }
            
            le = nextle;
        }
        
        if (empty)
            break;
    }
}

void STDCALL free_trees(device_extension* Vcb) {
    LIST_ENTRY* le;
    UINT8 level;
    
    for (level = 0; level <= 255; level++) {
        BOOL empty = TRUE;
        
        le = Vcb->trees.Flink;
        
        while (le != &Vcb->trees) {
            LIST_ENTRY* nextle = le->Flink;
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            root* r = t->root;
            
            if (t->header.level == level) {
                BOOL top = !t->paritem;
                
                empty = FALSE;
                
                free_tree2(t, funcname, __FILE__, __LINE__);
                if (top && r->treeholder.tree == t)
                    r->treeholder.tree = NULL;
                
                if (IsListEmpty(&Vcb->trees))
                    goto free_shared;
            } else if (t->header.level > level)
                empty = FALSE;
            
            le = nextle;
        }
        
        if (empty)
            break;
    }
    
free_shared:
    while (!IsListEmpty(&Vcb->shared_extents)) {
        shared_data* sd;
        
        le = RemoveHeadList(&Vcb->shared_extents);
        sd = CONTAINING_RECORD(le, shared_data, list_entry);
        
        while (!IsListEmpty(&sd->entries)) {
            LIST_ENTRY* le2 = RemoveHeadList(&sd->entries);
            shared_data_entry* sde = CONTAINING_RECORD(le2, shared_data_entry, list_entry);
            
            ExFreePool(sde);
        }
        
        ExFreePool(sd);
    }
}

void add_rollback(LIST_ENTRY* rollback, enum rollback_type type, void* ptr) {
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

BOOL STDCALL insert_tree_item(device_extension* Vcb, root* r, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, UINT32 size, traverse_ptr* ptp, PIRP Irp, LIST_ENTRY* rollback) {
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
    
#ifdef DEBUG_PARANOID
    if (!ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
        ERR("ERROR - tree_lock not held exclusively\n");
        int3;
    }
#endif
    
    searchkey.obj_id = obj_id;
    searchkey.obj_type = obj_type;
    searchkey.offset = offset;
    
    Status = find_item(Vcb, r, &tp, &searchkey, TRUE, Irp);
    if (Status == STATUS_NOT_FOUND) {
        if (r) {
            if (!r->treeholder.tree) {
                BOOL loaded;
                
                Status = do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, &loaded, Irp);
                
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
    } else if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }
    
    TRACE("tp.item = %p\n", tp.item);
    
    if (tp.item) {
        TRACE("tp.item->key = %p\n", &tp.item->key);
        cmp = keycmp(&searchkey, &tp.item->key);
        
        if (cmp == 0 && !tp.item->ignore) { // FIXME - look for all items of the same key to make sure none are non-ignored
            ERR("error: key (%llx,%x,%llx) already present\n", obj_id, obj_type, offset);
            int3;
            goto end;
        }
    } else
        cmp = -1;
    
    td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
    if (!td) {
        ERR("out of memory\n");
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
    
    if (!tp.tree->write) {
        tp.tree->write = TRUE;
        Vcb->need_write = TRUE;
    }
    
    if (ptp)
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
//     if (!ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
//         ERR("ERROR - tree_lock not held exclusively\n");
//         int3;
//     }

    if (tp->item->ignore) {
        ERR("trying to delete already-deleted item %llx,%x,%llx\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
        int3;
    }
#endif

    tp->item->ignore = TRUE;
    
    if (!tp->tree->write) {
        tp->tree->write = TRUE;
        Vcb->need_write = TRUE;
    }
    
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
            case ROLLBACK_ADD_SPACE:
            case ROLLBACK_SUBTRACT_SPACE:
            case ROLLBACK_INSERT_EXTENT:
            case ROLLBACK_DELETE_EXTENT:
                ExFreePool(ri->ptr);
                break;

            default:
                break;
        }
        
        ExFreePool(ri);
    }
}

void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    rollback_item* ri;
    
    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveTailList(rollback);
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
            
            case ROLLBACK_INSERT_EXTENT:
            {
                rollback_extent* re = ri->ptr;
                
                re->ext->ignore = TRUE;
                
                if (re->ext->data->type == EXTENT_TYPE_REGULAR || re->ext->data->type == EXTENT_TYPE_PREALLOC) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)re->ext->data->data;
                    
                    if (ed2->size != 0) {
                        chunk* c = get_chunk_from_address(Vcb, ed2->address);
                        
                        if (c) {
                            Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, re->fcb->subvol->id,
                                                               re->fcb->inode, re->ext->offset - ed2->offset, -1,
                                                               re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, NULL);
                            
                            if (!NT_SUCCESS(Status))
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                        }
                        
                        re->fcb->inode_item.st_blocks -= ed2->num_bytes;
                    }
                }
                
                ExFreePool(re);
                break;
            }
            
            case ROLLBACK_DELETE_EXTENT:
            {
                rollback_extent* re = ri->ptr;
                
                re->ext->ignore = FALSE;
                
                if (re->ext->data->type == EXTENT_TYPE_REGULAR || re->ext->data->type == EXTENT_TYPE_PREALLOC) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)re->ext->data->data;
                    
                    if (ed2->size != 0) {
                        chunk* c = get_chunk_from_address(Vcb, ed2->address);
                        
                        if (c) {
                            Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, re->fcb->subvol->id,
                                                               re->fcb->inode, re->ext->offset - ed2->offset, 1,
                                                               re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, NULL);
                            
                            if (!NT_SUCCESS(Status))
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                        }
                        
                        re->fcb->inode_item.st_blocks += ed2->num_bytes;
                    }
                }
                
                ExFreePool(re);
                break;
            }

            case ROLLBACK_ADD_SPACE:
            case ROLLBACK_SUBTRACT_SPACE:
            {
                rollback_space* rs = ri->ptr;
                
                if (rs->chunk)
                    ExAcquireResourceExclusiveLite(&rs->chunk->lock, TRUE);
                
                if (ri->type == ROLLBACK_ADD_SPACE)
                    space_list_subtract2(rs->list, rs->list_size, rs->address, rs->length, NULL);
                else
                    space_list_add2(rs->list, rs->list_size, rs->address, rs->length, NULL);
                
                if (rs->chunk) {
                    LIST_ENTRY* le2 = le->Blink;
                    
                    while (le2 != rollback) {
                        LIST_ENTRY* le3 = le2->Blink;
                        rollback_item* ri2 = CONTAINING_RECORD(le2, rollback_item, list_entry);
                        
                        if (ri2->type == ROLLBACK_ADD_SPACE || ri2->type == ROLLBACK_SUBTRACT_SPACE) {
                            rollback_space* rs2 = ri2->ptr;
                            
                            if (rs2->chunk == rs->chunk) {
                                if (ri2->type == ROLLBACK_ADD_SPACE)
                                    space_list_subtract2(rs2->list, rs2->list_size, rs2->address, rs2->length, NULL);
                                else
                                    space_list_add2(rs2->list, rs2->list_size, rs2->address, rs2->length, NULL);
                                
                                ExFreePool(rs2);
                                RemoveEntryList(&ri2->list_entry);
                                ExFreePool(ri2);
                            }
                        }
                        
                        le2 = le3;
                    }
                    
                    ExReleaseResourceLite(&rs->chunk->lock);
                }
                    
                ExFreePool(rs);
                
                break;
            }
        }
        
        ExFreePool(ri);
    }
}
