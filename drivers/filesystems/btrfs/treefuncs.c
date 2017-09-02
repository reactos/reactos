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

NTSTATUS STDCALL _load_tree(device_extension* Vcb, UINT64 addr, root* r, tree** pt, tree* parent, PIRP Irp, const char* func, const char* file, unsigned int line) {
    UINT8* buf;
    NTSTATUS Status;
    tree_header* th;
    tree* t;
    tree_data* td;
    chunk* c;
    UINT8 h;
    BOOL inserted;
    LIST_ENTRY* le;
    
    TRACE("(%p, %llx)\n", Vcb, addr);
    
    buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_data(Vcb, addr, Vcb->superblock.node_size, NULL, TRUE, buf, NULL, &c, Irp, FALSE);
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
    t->hash = calc_crc32c(0xffffffff, (UINT8*)&addr, sizeof(UINT64));
    t->has_address = TRUE;
    t->Vcb = Vcb;
    t->parent = NULL;
    t->root = r;
//     t->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    t->paritem = NULL;
    t->size = 0;
    t->new_address = 0;
    t->has_new_address = FALSE;
    t->updated_extents = FALSE;
    t->write = FALSE;
    
//     ExInitializeResourceLite(&t->nonpaged->load_tree_lock);
    
//     t->items = ExAllocatePoolWithTag(PagedPool, num_items * sizeof(tree_data), ALLOC_TAG);
    InitializeListHead(&t->itemlist);
    
    if (t->header.level == 0) { // leaf node
        leaf_node* ln = (leaf_node*)(buf + sizeof(tree_header));
        unsigned int i;
        
        if ((t->header.num_items * sizeof(leaf_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %llx has more items than expected (%x)\n", t->header.num_items);
            ExFreePool(buf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
        
        if ((t->header.num_items * sizeof(internal_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %llx has more items than expected (%x)\n", t->header.num_items);
            ExFreePool(buf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
    
    h = t->hash >> 24;
    
    if (!Vcb->trees_ptrs[h]) {
        UINT8 h2 = h;
        
        le = Vcb->trees_hash.Flink;
        
        if (h2 > 0) {
            h2--;
            do {
                if (Vcb->trees_ptrs[h2]) {
                    le = Vcb->trees_ptrs[h2];
                    break;
                }
                    
                h2--;
            } while (h2 > 0);
        }
    } else
        le = Vcb->trees_ptrs[h];
    
    inserted = FALSE;
    while (le != &Vcb->trees_hash) {
        tree* t2 = CONTAINING_RECORD(le, tree, list_entry_hash);
        
        if (t2->hash >= t->hash) {
            InsertHeadList(le->Blink, &t->list_entry_hash);
            inserted = TRUE;
            break;
        }
        
        le = le->Flink;
    }

    if (!inserted)
        InsertTailList(&Vcb->trees_hash, &t->list_entry_hash);

    if (!Vcb->trees_ptrs[h] || t->list_entry_hash.Flink == Vcb->trees_ptrs[h])
        Vcb->trees_ptrs[h] = &t->list_entry_hash;
    
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
            
        ExFreeToPagedLookasideList(&t->Vcb->tree_data_lookaside, td);
    }
    
    InterlockedDecrement(&t->Vcb->open_trees);
    RemoveEntryList(&t->list_entry);
    
    if (r) {
        r->treeholder.tree = NULL;
//             ExReleaseResourceLite(&r->nonpaged->load_tree_lock);
//             FsRtlExitFileSystem();
    }
    
    if (t->list_entry_hash.Flink) {
        UINT8 h = t->hash >> 24;
        if (t->Vcb->trees_ptrs[h] == &t->list_entry_hash) {
            if (t->list_entry_hash.Flink != &t->Vcb->trees_hash) {
                tree* t2 = CONTAINING_RECORD(t->list_entry_hash.Flink, tree, list_entry_hash);
                
                if ((t2->hash >> 24) == h)
                    t->Vcb->trees_ptrs[h] = &t2->list_entry_hash;
                else
                    t->Vcb->trees_ptrs[h] = NULL;
            } else
                t->Vcb->trees_ptrs[h] = NULL;
        }
        
        RemoveEntryList(&t->list_entry_hash);
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
        
#ifdef DEBUG_PARANOID
        if (t && t->header.level <= th->tree->header.level) int3;
#endif
        
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

static NTSTATUS STDCALL find_item_in_tree(device_extension* Vcb, tree* t, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, UINT8 level, PIRP Irp,
                                          const char* func, const char* file, unsigned int line) {
    int cmp;
    tree_data *td, *lasttd;
    KEY key2;
    
    TRACE("(%p, %p, %p, %p, %u)\n", Vcb, t, tp, searchkey, ignore);
    
    cmp = 1;
    td = first_item(t);
    lasttd = NULL;
    
    if (!td) return STATUS_NOT_FOUND;
    
    key2 = *searchkey;
    
    do {
        cmp = keycmp(key2, td->key);
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
                cmp = keycmp(key2, td->key);
                
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
        
        if (t->header.level <= level) {
            tp->tree = t;
            tp->item = td;
            return STATUS_SUCCESS;
        }
        
//         if (i > 0)
//             TRACE("entering tree from (%x,%x,%x) to (%x,%x,%x) (%p)\n", (UINT32)t->items[i].key.obj_id, t->items[i].key.obj_type, (UINT32)t->items[i].key.offset, (UINT32)t->items[i+1].key.obj_id, t->items[i+1].key.obj_type, (UINT32)t->items[i+1].key.offset, t->items[i].tree);
        
        Status = _do_load_tree(Vcb, &td->treeholder, t->root, t, td, &loaded, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
        
        Status = find_item_in_tree(Vcb, td->treeholder.tree, tp, searchkey, ignore, level, Irp, func, file, line);
        
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

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, 0, Irp, func, file, line);
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

NTSTATUS STDCALL _find_item_to_level(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, UINT8 level,
                                     PIRP Irp, const char* func, const char* file, unsigned int line) {
    NTSTATUS Status;
    BOOL loaded;
    
    TRACE("(%p, %p, %p, %p)\n", Vcb, r, tp, searchkey);
    
    if (!r->treeholder.tree) {
        Status = _do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, &loaded, Irp, func, file, line);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
    }

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, level, Irp, func, file, line);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
        ERR("find_item_in_tree returned %08x\n", Status);
    }
    
    if (Status == STATUS_NOT_FOUND) {
        tp->tree = r->treeholder.tree;
        tp->item = NULL;
    }
    
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
    
    Status = _do_load_tree(Vcb, &td->treeholder, t->parent->root, t->parent, td, &loaded, Irp, func, file, line);
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
                    return;
            } else if (t->header.level > level)
                empty = FALSE;
            
            le = nextle;
        }
        
        if (empty)
            break;
    }
}

void add_rollback(device_extension* Vcb, LIST_ENTRY* rollback, enum rollback_type type, void* ptr) {
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
    
// #ifdef DEBUG_PARANOID
//     if (!ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
//         ERR("ERROR - tree_lock not held exclusively\n");
//         int3;
//     }
// #endif
    
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
        cmp = keycmp(searchkey, tp.item->key);
        
        if (cmp == 0 && !tp.item->ignore) { // FIXME - look for all items of the same key to make sure none are non-ignored
            ERR("error: key (%llx,%x,%llx) already present\n", obj_id, obj_type, offset);
            int3;
            goto end;
        }
    } else
        cmp = -1;
    
    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
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
            if (!keycmp(paritem->key, tp.item->key)) {
                paritem->key = searchkey;
            } else
                break;
            
            paritem = paritem->treeholder.tree->paritem;
        }
    } else if (cmp == 0)
        InsertHeadList(tp.item->list_entry.Blink, &td->list_entry); // make sure non-deleted item is before deleted ones
    else
        InsertHeadList(&tp.item->list_entry, &td->list_entry);
    
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
    
    tp2 = ExAllocateFromPagedLookasideList(&Vcb->traverse_ptr_lookaside);
    if (!tp2) {
        ERR("out of memory\n");
        goto end;
    }
    
    tp2->tree = tp.tree;
    tp2->item = td;
    
    add_rollback(Vcb, rollback, ROLLBACK_INSERT_ITEM, tp2);
    
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
    
    tp2 = ExAllocateFromPagedLookasideList(&Vcb->traverse_ptr_lookaside);
    if (!tp2) {
        ERR("out of memory\n");
        return;
    }
    
    tp2->tree = tp->tree;
    tp2->item = tp->item;

    add_rollback(Vcb, rollback, ROLLBACK_DELETE_ITEM, tp2);
}

void clear_rollback(device_extension* Vcb, LIST_ENTRY* rollback) {
    rollback_item* ri;
    
    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveHeadList(rollback);
        ri = CONTAINING_RECORD(le, rollback_item, list_entry);
        
        switch (ri->type) {
            case ROLLBACK_INSERT_ITEM:
            case ROLLBACK_DELETE_ITEM:
                ExFreeToPagedLookasideList(&Vcb->traverse_ptr_lookaside, ri->ptr);
                break;
                
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
                
                ExFreeToPagedLookasideList(&Vcb->traverse_ptr_lookaside, tp);
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
                
                ExFreeToPagedLookasideList(&Vcb->traverse_ptr_lookaside, tp);
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
                                                               re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, FALSE, NULL);
                            
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
                                                               re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, FALSE, NULL);
                            
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
                    space_list_subtract2(Vcb, rs->list, rs->list_size, rs->address, rs->length, NULL);
                else
                    space_list_add2(Vcb, rs->list, rs->list_size, rs->address, rs->length, NULL);
                
                if (rs->chunk) {
                    LIST_ENTRY* le2 = le->Blink;
                    
                    while (le2 != rollback) {
                        LIST_ENTRY* le3 = le2->Blink;
                        rollback_item* ri2 = CONTAINING_RECORD(le2, rollback_item, list_entry);
                        
                        if (ri2->type == ROLLBACK_ADD_SPACE || ri2->type == ROLLBACK_SUBTRACT_SPACE) {
                            rollback_space* rs2 = ri2->ptr;
                            
                            if (rs2->chunk == rs->chunk) {
                                if (ri2->type == ROLLBACK_ADD_SPACE)
                                    space_list_subtract2(Vcb, rs2->list, rs2->list_size, rs2->address, rs2->length, NULL);
                                else
                                    space_list_add2(Vcb, rs2->list, rs2->list_size, rs2->address, rs2->length, NULL);
                                
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

static void find_tree_end(tree* t, KEY* tree_end, BOOL* no_end) {
    tree* p;
    
    p = t;
    do {
        tree_data* pi;
        
        if (!p->parent) {
            *no_end = TRUE;
            return;
        }
        
        pi = p->paritem;
        
        if (pi->list_entry.Flink != &p->parent->itemlist) {
            tree_data* td = CONTAINING_RECORD(pi->list_entry.Flink, tree_data, list_entry);
            
            *tree_end = td->key;
            *no_end = FALSE;
            return;
        }
        
        p = p->parent;
    } while (p);
}

void clear_batch_list(device_extension* Vcb, LIST_ENTRY* batchlist) {
    while (!IsListEmpty(batchlist)) {
        LIST_ENTRY* le = RemoveHeadList(batchlist);
        batch_root* br = CONTAINING_RECORD(le, batch_root, list_entry);
        
        while (!IsListEmpty(&br->items)) {
            LIST_ENTRY* le2 = RemoveHeadList(&br->items);
            batch_item* bi = CONTAINING_RECORD(le2, batch_item, list_entry);
            
            ExFreeToPagedLookasideList(&Vcb->batch_item_lookaside, bi);
        }
        
        ExFreePool(br);
    }
}

static void add_delete_inode_extref(device_extension* Vcb, batch_item* bi, LIST_ENTRY* listhead) {
    batch_item* bi2;
    LIST_ENTRY* le;
    INODE_REF* delir = (INODE_REF*)bi->data;
    INODE_EXTREF* ier;
    
    TRACE("entry in INODE_REF not found, adding Batch_DeleteInodeExtRef entry\n");
    
    bi2 = ExAllocateFromPagedLookasideList(&Vcb->batch_item_lookaside);
    if (!bi2) {
        ERR("out of memory\n");
        return;
    }
    
    ier = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_EXTREF) - 1 + delir->n, ALLOC_TAG);
    if (!ier) {
        ERR("out of memory\n");
        return;
    }
    
    ier->dir = bi->key.offset;
    ier->index = delir->index;
    ier->n = delir->n;
    RtlCopyMemory(ier->name, delir->name, delir->n);
    
    bi2->key.obj_id = bi->key.obj_id;
    bi2->key.obj_type = TYPE_INODE_EXTREF;
    bi2->key.offset = calc_crc32c((UINT32)bi->key.offset, (UINT8*)ier->name, ier->n);
    bi2->data = ier;
    bi2->datalen = sizeof(INODE_EXTREF) - 1 + ier->n;
    bi2->operation = Batch_DeleteInodeExtRef;
    
    le = bi->list_entry.Flink;
    while (le != listhead) {
        batch_item* bi3 = CONTAINING_RECORD(le, batch_item, list_entry);
        
        if (keycmp(bi3->key, bi2->key) != -1) {
            InsertHeadList(le->Blink, &bi2->list_entry);
            return;
        }
        
        le = le->Flink;
    }
    
    InsertTailList(listhead, &bi2->list_entry);
}

static BOOL handle_batch_collision(device_extension* Vcb, batch_item* bi, tree* t, tree_data* td, tree_data* newtd, LIST_ENTRY* listhead, LIST_ENTRY* rollback) {
    if (bi->operation == Batch_Delete || bi->operation == Batch_SetXattr || bi->operation == Batch_DirItem || bi->operation == Batch_InodeRef ||
        bi->operation == Batch_InodeExtRef || bi->operation == Batch_DeleteDirItem || bi->operation == Batch_DeleteInodeRef ||
        bi->operation == Batch_DeleteInodeExtRef) {
        UINT16 maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
        
        switch (bi->operation) {
            case Batch_SetXattr: {
                if (td->size < sizeof(DIR_ITEM)) {
                    ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset, td->size, sizeof(DIR_ITEM));
                } else {
                    UINT8* newdata;
                    ULONG size = td->size;
                    DIR_ITEM* newxa = (DIR_ITEM*)bi->data;
                    DIR_ITEM* xa = (DIR_ITEM*)td->data;
                    
                    while (TRUE) {
                        ULONG oldxasize;
                        
                        if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                            ERR("(%llx,%x,%llx) was truncated\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);
                            break;
                        }
                        
                        oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                        
                        if (xa->n == newxa->n && RtlCompareMemory(newxa->name, xa->name, xa->n) == xa->n) {
                            UINT64 pos;
                            
                            // replace
                            
                            if (td->size + bi->datalen - oldxasize > maxlen)
                                ERR("DIR_ITEM would be over maximum size, truncating (%u + %u - %u > %u)\n", td->size, bi->datalen, oldxasize, maxlen);
                            
                            newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen - oldxasize, ALLOC_TAG);
                            if (!newdata) {
                                ERR("out of memory\n");
                                return TRUE;
                            }
                            
                            pos = (UINT8*)xa - td->data;
                            if (pos + oldxasize < td->size) { // copy after changed xattr
                                RtlCopyMemory(newdata + pos + bi->datalen, td->data + pos + oldxasize, td->size - pos - oldxasize);
                            }
                            
                            if (pos > 0) { // copy before changed xattr
                                RtlCopyMemory(newdata, td->data, pos);
                                xa = (DIR_ITEM*)(newdata + pos);
                            } else
                                xa = (DIR_ITEM*)newdata;
                            
                            RtlCopyMemory(xa, bi->data, bi->datalen);
                            
                            bi->datalen = min(td->size + bi->datalen - oldxasize, maxlen);
                            
                            ExFreePool(bi->data);
                            bi->data = newdata;
                            
                            break;
                        }
                        
                        if ((UINT8*)xa - (UINT8*)td->data + oldxasize >= size) {
                            // not found, add to end of data
                            
                            if (td->size + bi->datalen > maxlen)
                                ERR("DIR_ITEM would be over maximum size, truncating (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                            
                            newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                            if (!newdata) {
                                ERR("out of memory\n");
                                return TRUE;
                            }
                            
                            RtlCopyMemory(newdata, td->data, td->size);
                            
                            xa = (DIR_ITEM*)((UINT8*)newdata + td->size);
                            RtlCopyMemory(xa, bi->data, bi->datalen);
                            
                            bi->datalen = min(bi->datalen + td->size, maxlen);
                            
                            ExFreePool(bi->data);
                            bi->data = newdata;

                            break;
                        } else {
                            xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
                            size -= oldxasize;
                        }
                    }
                }
                break;
            }
            
            case Batch_DirItem: {
                UINT8* newdata;
                
                if (td->size + bi->datalen > maxlen) {
                    ERR("DIR_ITEM would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                    return TRUE;
                }
                
                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return TRUE;
                }
                
                RtlCopyMemory(newdata, td->data, td->size);
                
                RtlCopyMemory(newdata + td->size, bi->data, bi->datalen);

                bi->datalen += td->size;
                
                ExFreePool(bi->data);
                bi->data = newdata;
                
                break;
            }
        
            case Batch_InodeRef: {
                UINT8* newdata;
                
                if (td->size + bi->datalen > maxlen) {
                    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                        INODE_REF* ir = (INODE_REF*)bi->data;
                        INODE_EXTREF* ier;
                        ULONG ierlen;
                        batch_item* bi2;
                        LIST_ENTRY* le;
                        BOOL inserted = FALSE;
                        
                        TRACE("INODE_REF would be too long, adding INODE_EXTREF instead\n");

                        ierlen = sizeof(INODE_EXTREF) - 1 + ir->n;
                        
                        ier = ExAllocatePoolWithTag(PagedPool, ierlen, ALLOC_TAG);
                        if (!ier) {
                            ERR("out of memory\n");
                            return TRUE;
                        }
                        
                        ier->dir = bi->key.offset;
                        ier->index = ir->index;
                        ier->n = ir->n;
                        RtlCopyMemory(ier->name, ir->name, ier->n);
                        
                        bi2 = ExAllocateFromPagedLookasideList(&Vcb->batch_item_lookaside);
                        if (!bi2) {
                            ERR("out of memory\n");
                            ExFreePool(ier);
                            return TRUE;
                        }
                        
                        bi2->key.obj_id = bi->key.obj_id;
                        bi2->key.obj_type = TYPE_INODE_EXTREF;
                        bi2->key.offset = calc_crc32c((UINT32)ier->dir, (UINT8*)ier->name, ier->n);
                        bi2->data = ier;
                        bi2->datalen = ierlen;
                        bi2->operation = Batch_InodeExtRef;
                        
                        le = bi->list_entry.Flink;
                        while (le != listhead) {
                            batch_item* bi3 = CONTAINING_RECORD(le, batch_item, list_entry);
                            
                            if (keycmp(bi3->key, bi2->key) != -1) {
                                InsertHeadList(le->Blink, &bi2->list_entry);
                                inserted = TRUE;
                            }
                            
                            le = le->Flink;
                        }
                        
                        if (!inserted)
                            InsertTailList(listhead, &bi2->list_entry);
                        
                        return TRUE;
                    } else {
                        ERR("INODE_REF would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                        return TRUE;
                    }
                }
                
                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return TRUE;
                }
                
                RtlCopyMemory(newdata, td->data, td->size);
                
                RtlCopyMemory(newdata + td->size, bi->data, bi->datalen);

                bi->datalen += td->size;
                
                ExFreePool(bi->data);
                bi->data = newdata;
                
                break;
            }
        
            case Batch_InodeExtRef: {
                UINT8* newdata;
                
                if (td->size + bi->datalen > maxlen) {
                    ERR("INODE_EXTREF would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                    return TRUE;
                }
                
                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return TRUE;
                }
                
                RtlCopyMemory(newdata, td->data, td->size);
                
                RtlCopyMemory(newdata + td->size, bi->data, bi->datalen);

                bi->datalen += td->size;
                
                ExFreePool(bi->data);
                bi->data = newdata;
                
                break;
            }
        
            case Batch_DeleteDirItem: {
                if (td->size < sizeof(DIR_ITEM)) {
                    WARN("DIR_ITEM was %u bytes, expected at least %u\n", td->size, sizeof(DIR_ITEM));
                    return TRUE;
                } else {
                    DIR_ITEM *di, *deldi;
                    LONG len;
                    
                    deldi = (DIR_ITEM*)bi->data;
                    di = (DIR_ITEM*)td->data;
                    len = td->size;
                    
                    do {
                        if (di->m == deldi->m && di->n == deldi->n && RtlCompareMemory(di->name, deldi->name, di->n + di->m) == di->n + di->m) {
                            ULONG newlen = td->size - (sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m);
                            
                            if (newlen == 0) {
                                TRACE("deleting DIR_ITEM\n");
                            } else {
                                UINT8 *newdi = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *dioff;
                                tree_data* td2;
                                
                                if (!newdi) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                TRACE("modifying DIR_ITEM\n");

                                if ((UINT8*)di > td->data) {
                                    RtlCopyMemory(newdi, td->data, (UINT8*)di - td->data);
                                    dioff = newdi + ((UINT8*)di - td->data);
                                } else {
                                    dioff = newdi;
                                }
                                
                                if ((UINT8*)&di->name[di->n + di->m] - td->data < td->size)
                                    RtlCopyMemory(dioff, &di->name[di->n + di->m], td->size - ((UINT8*)&di->name[di->n + di->m] - td->data));
                                
                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newdi;
                                td2->ignore = FALSE;
                                td2->inserted = TRUE;
                                
                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);
                                
                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = TRUE;
                            }
                            
                            break;
                        }
                        
                        len -= sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m;
                        di = (DIR_ITEM*)&di->name[di->n + di->m];
                        
                        if (len == 0) {
                            TRACE("could not find DIR_ITEM to delete\n");
                            return TRUE;
                        }
                    } while (len > 0);
                }
                break;
            }
        
            case Batch_DeleteInodeRef: {
                if (td->size < sizeof(INODE_REF)) {
                    WARN("INODE_REF was %u bytes, expected at least %u\n", td->size, sizeof(INODE_REF));
                    return TRUE;
                } else {
                    INODE_REF *ir, *delir;
                    ULONG len;
                    BOOL changed = FALSE;
                    
                    delir = (INODE_REF*)bi->data;
                    ir = (INODE_REF*)td->data;
                    len = td->size;
                    
                    do {
                        ULONG itemlen;
                        
                        if (len < sizeof(INODE_REF) || len < sizeof(INODE_REF) - 1 + ir->n) {
                            ERR("INODE_REF was truncated\n");
                            break;
                        }
                        
                        itemlen = sizeof(INODE_REF) - sizeof(char) + ir->n;
                        
                        if (ir->n == delir->n && RtlCompareMemory(ir->name, delir->name, ir->n) == ir->n) {
                            ULONG newlen = td->size - itemlen;
                            
                            changed = TRUE;
                            
                            if (newlen == 0)
                                TRACE("deleting INODE_REF\n");
                            else {
                                UINT8 *newir = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *iroff;
                                tree_data* td2;
                                
                                if (!newir) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                TRACE("modifying INODE_REF\n");

                                if ((UINT8*)ir > td->data) {
                                    RtlCopyMemory(newir, td->data, (UINT8*)ir - td->data);
                                    iroff = newir + ((UINT8*)ir - td->data);
                                } else {
                                    iroff = newir;
                                }
                                
                                if ((UINT8*)&ir->name[ir->n] - td->data < td->size)
                                    RtlCopyMemory(iroff, &ir->name[ir->n], td->size - ((UINT8*)&ir->name[ir->n] - td->data));
                                
                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newir;
                                td2->ignore = FALSE;
                                td2->inserted = TRUE;
                                
                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);
                                
                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = TRUE;
                            }
                            
                            break;
                        }
                        
                        if (len > itemlen) {
                            len -= itemlen;
                            ir = (INODE_REF*)&ir->name[ir->n];
                        } else
                            break;
                    } while (len > 0);
                    
                    if (!changed) {
                        if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                            TRACE("entry in INODE_REF not found, adding Batch_DeleteInodeExtRef entry\n");
                            
                            add_delete_inode_extref(Vcb, bi, listhead);
                            
                            return TRUE;
                        } else
                            WARN("entry not found in INODE_REF\n");
                    }
                }
                
                break;
            }
        
            case Batch_DeleteInodeExtRef: {
                if (td->size < sizeof(INODE_EXTREF)) {
                    WARN("INODE_EXTREF was %u bytes, expected at least %u\n", td->size, sizeof(INODE_EXTREF));
                    return TRUE;
                } else {
                    INODE_EXTREF *ier, *delier;
                    ULONG len;
                    
                    delier = (INODE_EXTREF*)bi->data;
                    ier = (INODE_EXTREF*)td->data;
                    len = td->size;
                    
                    do {
                        ULONG itemlen;
                        
                        if (len < sizeof(INODE_EXTREF) || len < sizeof(INODE_EXTREF) - 1 + ier->n) {
                            ERR("INODE_REF was truncated\n");
                            break;
                        }
                        
                        itemlen = sizeof(INODE_EXTREF) - sizeof(char) + ier->n;
                        
                        if (ier->dir == delier->dir && ier->n == delier->n && RtlCompareMemory(ier->name, delier->name, ier->n) == ier->n) {
                            ULONG newlen = td->size - itemlen;
                            
                            if (newlen == 0)
                                TRACE("deleting INODE_EXTREF\n");
                            else {
                                UINT8 *newier = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *ieroff;
                                tree_data* td2;
                                
                                if (!newier) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                TRACE("modifying INODE_EXTREF\n");

                                if ((UINT8*)ier > td->data) {
                                    RtlCopyMemory(newier, td->data, (UINT8*)ier - td->data);
                                    ieroff = newier + ((UINT8*)ier - td->data);
                                } else {
                                    ieroff = newier;
                                }
                                
                                if ((UINT8*)&ier->name[ier->n] - td->data < td->size)
                                    RtlCopyMemory(ieroff, &ier->name[ier->n], td->size - ((UINT8*)&ier->name[ier->n] - td->data));
                                
                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    return TRUE;
                                }
                                
                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newier;
                                td2->ignore = FALSE;
                                td2->inserted = TRUE;
                                
                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);
                                
                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = TRUE;
                            }
                            
                            break;
                        }
                        
                        if (len > itemlen) {
                            len -= itemlen;
                            ier = (INODE_EXTREF*)&ier->name[ier->n];
                        } else
                            break;
                    } while (len > 0);
                }
                break;
            }
            
            case Batch_Delete:
                break;
            
            default:
                ERR("unexpected batch operation type\n");
                int3;
                break;
        }
        
        // delete old item
        if (!td->ignore) {
            traverse_ptr* tp2;
            
            td->ignore = TRUE;
        
            t->header.num_items--;
            t->size -= sizeof(leaf_node) + td->size;
            t->write = TRUE;
            
            if (rollback) {
                tp2 = ExAllocateFromPagedLookasideList(&Vcb->traverse_ptr_lookaside);
                if (!tp2) {
                    ERR("out of memory\n");
                    return FALSE;
                }
                
                tp2->tree = t;
                tp2->item = td;
    
                add_rollback(Vcb, rollback, ROLLBACK_DELETE_ITEM, tp2);
            }
        }

        if (newtd) {
            newtd->data = bi->data;
            newtd->size = bi->datalen;
            InsertHeadList(&td->list_entry, &newtd->list_entry);
        }
    } else {
        ERR("(%llx,%x,%llx) already exists\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);
        int3;
    }
    
    return FALSE;
}

static void commit_batch_list_root(device_extension* Vcb, batch_root* br, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    TRACE("root: %llx\n", br->r->id);
    
    le = br->items.Flink;
    while (le != &br->items) {
        batch_item* bi = CONTAINING_RECORD(le, batch_item, list_entry);
        LIST_ENTRY *le2, *listhead;
        traverse_ptr tp, *tp2;
        KEY tree_end;
        BOOL no_end;
        tree_data* td;
        int cmp;
        tree* t;
        BOOL ignore = FALSE;
        
        TRACE("(%llx,%x,%llx)\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);
        
        Status = find_item(Vcb, br->r, &tp, &bi->key, FALSE, Irp);
        if (!NT_SUCCESS(Status)) { // FIXME - handle STATUS_NOT_FOUND
            ERR("find_item returned %08x\n", Status);
            return;
        }
        
        find_tree_end(tp.tree, &tree_end, &no_end);
        
        if (bi->operation == Batch_DeleteInode) {
            if (tp.item->key.obj_id == bi->key.obj_id) {
                BOOL ended = FALSE;
                
                td = tp.item;
                
                if (!tp.item->ignore) {
                    tp.item->ignore = TRUE;
                    tp.tree->header.num_items--;
                    tp.tree->size -= tp.item->size + sizeof(leaf_node);
                    tp.tree->write = TRUE;
                }
                
                le2 = tp.item->list_entry.Flink;
                while (le2 != &tp.tree->itemlist) {
                    td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    
                    if (td->key.obj_id == bi->key.obj_id) {
                        if (!td->ignore) {
                            td->ignore = TRUE;
                            tp.tree->header.num_items--;
                            tp.tree->size -= td->size + sizeof(leaf_node);
                            tp.tree->write = TRUE;
                        }
                    } else {
                        ended = TRUE;
                        break;
                    }
                    
                    le2 = le2->Flink;
                }
                
                while (!ended) {
                    traverse_ptr next_tp;
                    
                    tp.item = td;
                    
                    if (!find_next_item(Vcb, &tp, &next_tp, FALSE, Irp))
                        break;
                    
                    tp = next_tp;
                    
                    le2 = &tp.item->list_entry;
                    while (le2 != &tp.tree->itemlist) {
                        td = CONTAINING_RECORD(le2, tree_data, list_entry);
                        
                        if (td->key.obj_id == bi->key.obj_id) {
                            if (!td->ignore) {
                                td->ignore = TRUE;
                                tp.tree->header.num_items--;
                                tp.tree->size -= td->size + sizeof(leaf_node);
                                tp.tree->write = TRUE;
                            }
                        } else {
                            ended = TRUE;
                            break;
                        }
                        
                        le2 = le2->Flink;
                    }
                }
            }
        } else {
            if (bi->operation == Batch_Delete || bi->operation == Batch_DeleteDirItem ||
                bi->operation == Batch_DeleteInodeRef || bi->operation == Batch_DeleteInodeExtRef)
                td = NULL;
            else {
                td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                if (!td) {
                    ERR("out of memory\n");
                    return;
                }
                
                td->key = bi->key;
                td->size = bi->datalen;
                td->data = bi->data;
                td->ignore = FALSE;
                td->inserted = TRUE;
            }
            
            cmp = keycmp(bi->key, tp.item->key);
            
            if (cmp == -1) { // very first key in root
                if (td) {
                    tree_data* paritem;
                    
                    InsertHeadList(&tp.tree->itemlist, &td->list_entry);

                    paritem = tp.tree->paritem;
                    while (paritem) {
                        if (!keycmp(paritem->key, tp.item->key)) {
                            paritem->key = bi->key;
                        } else
                            break;
                        
                        paritem = paritem->treeholder.tree->paritem;
                    }
                }
            } else if (cmp == 0) { // item already exists
                ignore = handle_batch_collision(Vcb, bi, tp.tree, tp.item, td, &br->items, rollback);
            } else if (td) {
                InsertHeadList(&tp.item->list_entry, &td->list_entry);
            }
            
            if (bi->operation == Batch_DeleteInodeRef && cmp != 0 && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                add_delete_inode_extref(Vcb, bi, &br->items);
            }
            
            if (!ignore && td) {
                tp.tree->header.num_items++;
                tp.tree->size += bi->datalen + sizeof(leaf_node);
                tp.tree->write = TRUE;
                
                if (rollback) {
                    // FIXME - free this correctly
                    tp2 = ExAllocateFromPagedLookasideList(&Vcb->traverse_ptr_lookaside);
                    if (!tp2) {
                        ERR("out of memory\n");
                        return;
                    }
                    
                    tp2->tree = tp.tree;
                    tp2->item = td;

                    add_rollback(Vcb, rollback, ROLLBACK_INSERT_ITEM, tp2);
                }
                
                listhead = &td->list_entry;
            } else {
                listhead = &tp.item->list_entry;
                
                if (!td && tp.item->ignore && tp.item->list_entry.Blink != &tp.tree->itemlist) {
                    tree_data* prevtd = CONTAINING_RECORD(tp.item->list_entry.Blink, tree_data, list_entry);
                    
                    if (!prevtd->ignore && !keycmp(prevtd->key, tp.item->key))
                        listhead = &prevtd->list_entry;
                }
            }
            
            le2 = le->Flink;
            while (le2 != &br->items) {
                batch_item* bi2 = CONTAINING_RECORD(le2, batch_item, list_entry);
                
                if (bi2->operation == Batch_DeleteInode)
                    break;
                
                if (no_end || keycmp(bi2->key, tree_end) == -1) {
                    LIST_ENTRY* le3;
                    BOOL inserted = FALSE;
                    
                    ignore = FALSE;
                    
                    if (bi2->operation == Batch_Delete || bi2->operation == Batch_DeleteDirItem ||
                        bi2->operation == Batch_DeleteInodeRef || bi2->operation == Batch_DeleteInodeExtRef)
                        td = NULL;
                    else {
                        td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                        if (!td) {
                            ERR("out of memory\n");
                            return;
                        }
                        
                        td->key = bi2->key;
                        td->size = bi2->datalen;
                        td->data = bi2->data;
                        td->ignore = FALSE;
                        td->inserted = TRUE;
                    }
                    
                    le3 = listhead;
                    while (le3 != &tp.tree->itemlist) {
                        tree_data* td2 = CONTAINING_RECORD(le3, tree_data, list_entry);
                        
                        if (!td2->ignore) {
                            cmp = keycmp(bi2->key, td2->key);

                            if (cmp == 0) {
                                ignore = handle_batch_collision(Vcb, bi2, tp.tree, td2, td, &br->items, rollback);
                                inserted = TRUE;
                                break;
                            } else if (cmp == -1) {
                                if (td) {
                                    InsertHeadList(le3->Blink, &td->list_entry);
                                    inserted = TRUE;
                                } else if (bi2->operation == Batch_DeleteInodeRef && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                                    add_delete_inode_extref(Vcb, bi2, &br->items);
                                }
                                break;
                            }
                        }
                        
                        le3 = le3->Flink;
                    }
                    
                    if (td) {
                        if (!inserted)
                            InsertTailList(&tp.tree->itemlist, &td->list_entry);
                        
                        if (!ignore) {
                            tp.tree->header.num_items++;
                            tp.tree->size += bi2->datalen + sizeof(leaf_node);
                            
                            if (rollback) {
                                // FIXME - free this correctly
                                tp2 = ExAllocateFromPagedLookasideList(&Vcb->traverse_ptr_lookaside);
                                if (!tp2) {
                                    ERR("out of memory\n");
                                    return;
                                }
                                
                                tp2->tree = tp.tree;
                                tp2->item = td;
                                
                                add_rollback(Vcb, rollback, ROLLBACK_INSERT_ITEM, tp2);
                            }
                            
                            listhead = &td->list_entry;
                        }
                    } else if (!inserted && bi2->operation == Batch_DeleteInodeRef && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                        add_delete_inode_extref(Vcb, bi2, &br->items);
                    }
                    
                    le = le2;
                } else
                    break;
                
                le2 = le2->Flink;
            }
            
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
        }
        
        le = le->Flink;
    }
    
    // FIXME - remove as we are going along
    while (!IsListEmpty(&br->items)) {
        LIST_ENTRY* le = RemoveHeadList(&br->items);
        batch_item* bi = CONTAINING_RECORD(le, batch_item, list_entry);
        
        if ((bi->operation == Batch_DeleteDirItem || bi->operation == Batch_DeleteInodeRef || bi->operation == Batch_DeleteInodeExtRef) && bi->data)
            ExFreePool(bi->data);
        
        ExFreeToPagedLookasideList(&Vcb->batch_item_lookaside, bi);
    }
}

void commit_batch_list(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    while (!IsListEmpty(batchlist)) {
        LIST_ENTRY* le = RemoveHeadList(batchlist);
        batch_root* br2 = CONTAINING_RECORD(le, batch_root, list_entry);
        
        commit_batch_list_root(Vcb, br2, Irp, rollback);
        
        ExFreePool(br2);
    }
}
