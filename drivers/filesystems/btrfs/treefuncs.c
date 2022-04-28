/* Copyright (c) Mark Harmstone 2016-17
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
#include "crc32c.h"

__attribute__((nonnull(1,3,4,5)))
NTSTATUS load_tree(device_extension* Vcb, uint64_t addr, uint8_t* buf, root* r, tree** pt) {
    tree_header* th;
    tree* t;
    tree_data* td;
    uint8_t h;
    bool inserted;
    LIST_ENTRY* le;

    th = (tree_header*)buf;

    t = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!t) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (th->level > 0) {
        t->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
        if (!t->nonpaged) {
            ERR("out of memory\n");
            ExFreePool(t);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ExInitializeFastMutex(&t->nonpaged->mutex);
    } else
        t->nonpaged = NULL;

    RtlCopyMemory(&t->header, th, sizeof(tree_header));
    t->hash = calc_crc32c(0xffffffff, (uint8_t*)&addr, sizeof(uint64_t));
    t->has_address = true;
    t->Vcb = Vcb;
    t->parent = NULL;
    t->root = r;
    t->paritem = NULL;
    t->size = 0;
    t->new_address = 0;
    t->has_new_address = false;
    t->updated_extents = false;
    t->write = false;
    t->uniqueness_determined = false;

    InitializeListHead(&t->itemlist);

    if (t->header.level == 0) { // leaf node
        leaf_node* ln = (leaf_node*)(buf + sizeof(tree_header));
        unsigned int i;

        if ((t->header.num_items * sizeof(leaf_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %I64x has more items than expected (%x)\n", addr, t->header.num_items);
            ExFreePool(t);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
            if (!td) {
                ERR("out of memory\n");
                ExFreePool(t);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            td->key = ln[i].key;

            if (ln[i].size > 0)
                td->data = buf + sizeof(tree_header) + ln[i].offset;
            else
                td->data = NULL;

            if (ln[i].size + sizeof(tree_header) + sizeof(leaf_node) > Vcb->superblock.node_size) {
                ERR("overlarge item in tree %I64x: %u > %Iu\n", addr, ln[i].size, Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node));
                ExFreeToPagedLookasideList(&t->Vcb->tree_data_lookaside, td);
                ExFreePool(t);
                return STATUS_INTERNAL_ERROR;
            }

            td->size = (uint16_t)ln[i].size;
            td->ignore = false;
            td->inserted = false;

            InsertTailList(&t->itemlist, &td->list_entry);

            t->size += ln[i].size;
        }

        t->size += t->header.num_items * sizeof(leaf_node);
        t->buf = buf;
    } else {
        internal_node* in = (internal_node*)(buf + sizeof(tree_header));
        unsigned int i;

        if ((t->header.num_items * sizeof(internal_node)) + sizeof(tree_header) > Vcb->superblock.node_size) {
            ERR("tree at %I64x has more items than expected (%x)\n", addr, t->header.num_items);
            ExFreePool(t);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < t->header.num_items; i++) {
            td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
            if (!td) {
                ERR("out of memory\n");
                ExFreePool(t);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            td->key = in[i].key;

            td->treeholder.address = in[i].address;
            td->treeholder.generation = in[i].generation;
            td->treeholder.tree = NULL;
            td->ignore = false;
            td->inserted = false;

            InsertTailList(&t->itemlist, &td->list_entry);
        }

        t->size = t->header.num_items * sizeof(internal_node);
        t->buf = NULL;
    }

    ExAcquireFastMutex(&Vcb->trees_list_mutex);

    InsertTailList(&Vcb->trees, &t->list_entry);

    h = t->hash >> 24;

    if (!Vcb->trees_ptrs[h]) {
        uint8_t h2 = h;

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

    inserted = false;
    while (le != &Vcb->trees_hash) {
        tree* t2 = CONTAINING_RECORD(le, tree, list_entry_hash);

        if (t2->hash >= t->hash) {
            InsertHeadList(le->Blink, &t->list_entry_hash);
            inserted = true;
            break;
        }

        le = le->Flink;
    }

    if (!inserted)
        InsertTailList(&Vcb->trees_hash, &t->list_entry_hash);

    if (!Vcb->trees_ptrs[h] || t->list_entry_hash.Flink == Vcb->trees_ptrs[h])
        Vcb->trees_ptrs[h] = &t->list_entry_hash;

    ExReleaseFastMutex(&Vcb->trees_list_mutex);

    TRACE("returning %p\n", t);

    *pt = t;

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2,3,4)))
static NTSTATUS do_load_tree2(device_extension* Vcb, tree_holder* th, uint8_t* buf, root* r, tree* t, tree_data* td) {
    if (!th->tree) {
        NTSTATUS Status;
        tree* nt;

        Status = load_tree(Vcb, th->address, buf, r, &nt);
        if (!NT_SUCCESS(Status)) {
            ERR("load_tree returned %08lx\n", Status);
            return Status;
        }

        nt->parent = t;

#ifdef DEBUG_PARANOID
        if (t && t->header.level <= nt->header.level) int3;
#endif

        nt->paritem = td;

        th->tree = nt;
    }

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2,3)))
NTSTATUS do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, PIRP Irp) {
    NTSTATUS Status;
    uint8_t* buf;
    chunk* c;

    buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = read_data(Vcb, th->address, Vcb->superblock.node_size, NULL, true, buf, NULL,
                       &c, Irp, th->generation, false, NormalPagePriority);
    if (!NT_SUCCESS(Status)) {
        ERR("read_data returned 0x%08lx\n", Status);
        ExFreePool(buf);
        return Status;
    }

    if (t)
        ExAcquireFastMutex(&t->nonpaged->mutex);
    else
        ExAcquireResourceExclusiveLite(&r->nonpaged->load_tree_lock, true);

    Status = do_load_tree2(Vcb, th, buf, r, t, td);

    if (t)
        ExReleaseFastMutex(&t->nonpaged->mutex);
    else
        ExReleaseResourceLite(&r->nonpaged->load_tree_lock);

    if (!th->tree || th->tree->buf != buf)
        ExFreePool(buf);

    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree2 returned %08lx\n", Status);
        return Status;
    }

    return Status;
}

__attribute__((nonnull(1)))
void free_tree(tree* t) {
    tree* par;
    root* r = t->root;

    // No need to acquire lock, as this is only ever called while Vcb->tree_lock held exclusively

    par = t->parent;

    if (r && r->treeholder.tree != t)
        r = NULL;

    if (par) {
        if (t->paritem)
            t->paritem->treeholder.tree = NULL;
    }

    while (!IsListEmpty(&t->itemlist)) {
        tree_data* td = CONTAINING_RECORD(RemoveHeadList(&t->itemlist), tree_data, list_entry);

        if (t->header.level == 0 && td->data && td->inserted)
            ExFreePool(td->data);

        ExFreeToPagedLookasideList(&t->Vcb->tree_data_lookaside, td);
    }

    RemoveEntryList(&t->list_entry);

    if (r)
        r->treeholder.tree = NULL;

    if (t->list_entry_hash.Flink) {
        uint8_t h = t->hash >> 24;
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

    if (t->buf)
        ExFreePool(t->buf);

    if (t->nonpaged)
        ExFreePool(t->nonpaged);

    ExFreePool(t);
}

__attribute__((nonnull(1)))
static __inline tree_data* first_item(tree* t) {
    LIST_ENTRY* le = t->itemlist.Flink;

    if (le == &t->itemlist)
        return NULL;

    return CONTAINING_RECORD(le, tree_data, list_entry);
}

__attribute__((nonnull(1,2)))
static __inline tree_data* prev_item(tree* t, tree_data* td) {
    LIST_ENTRY* le = td->list_entry.Blink;

    if (le == &t->itemlist)
        return NULL;

    return CONTAINING_RECORD(le, tree_data, list_entry);
}

__attribute__((nonnull(1,2)))
static __inline tree_data* next_item(tree* t, tree_data* td) {
    LIST_ENTRY* le = td->list_entry.Flink;

    if (le == &t->itemlist)
        return NULL;

    return CONTAINING_RECORD(le, tree_data, list_entry);
}

__attribute__((nonnull(1,2,3,4)))
static NTSTATUS next_item2(device_extension* Vcb, tree* t, tree_data* td, traverse_ptr* tp) {
    tree_data* td2 = next_item(t, td);
    tree* t2;

    if (td2) {
        tp->tree = t;
        tp->item = td2;
        return STATUS_SUCCESS;
    }

    t2 = t;

    do {
        td2 = t2->paritem;
        t2 = t2->parent;
    } while (td2 && !next_item(t2, td2));

    if (!td2)
        return STATUS_NOT_FOUND;

    td2 = next_item(t2, td2);

    return find_item_to_level(Vcb, t2->root, tp, &td2->key, false, t->header.level, NULL);
}

__attribute__((nonnull(1,2,3,4,5)))
NTSTATUS skip_to_difference(device_extension* Vcb, traverse_ptr* tp, traverse_ptr* tp2, bool* ended1, bool* ended2) {
    NTSTATUS Status;
    tree *t1, *t2;
    tree_data *td1, *td2;

    t1 = tp->tree;
    t2 = tp2->tree;

    do {
        td1 = t1->paritem;
        td2 = t2->paritem;
        t1 = t1->parent;
        t2 = t2->parent;
    } while (t1 && t2 && t1->header.address == t2->header.address);

    while (true) {
        traverse_ptr tp3, tp4;

        Status = next_item2(Vcb, t1, td1, &tp3);
        if (Status == STATUS_NOT_FOUND)
            *ended1 = true;
        else if (!NT_SUCCESS(Status)) {
            ERR("next_item2 returned %08lx\n", Status);
            return Status;
        }

        Status = next_item2(Vcb, t2, td2, &tp4);
        if (Status == STATUS_NOT_FOUND)
            *ended2 = true;
        else if (!NT_SUCCESS(Status)) {
            ERR("next_item2 returned %08lx\n", Status);
            return Status;
        }

        if (*ended1 || *ended2) {
            if (!*ended1) {
                Status = find_item(Vcb, t1->root, tp, &tp3.item->key, false, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_item returned %08lx\n", Status);
                    return Status;
                }
            } else if (!*ended2) {
                Status = find_item(Vcb, t2->root, tp2, &tp4.item->key, false, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_item returned %08lx\n", Status);
                    return Status;
                }
            }

            return STATUS_SUCCESS;
        }

        if (tp3.tree->header.address != tp4.tree->header.address) {
            Status = find_item(Vcb, t1->root, tp, &tp3.item->key, false, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("find_item returned %08lx\n", Status);
                return Status;
            }

            Status = find_item(Vcb, t2->root, tp2, &tp4.item->key, false, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("find_item returned %08lx\n", Status);
                return Status;
            }

            return STATUS_SUCCESS;
        }

        t1 = tp3.tree;
        td1 = tp3.item;
        t2 = tp4.tree;
        td2 = tp4.item;
    }
}

__attribute__((nonnull(1,2,3,4)))
static NTSTATUS find_item_in_tree(device_extension* Vcb, tree* t, traverse_ptr* tp, const KEY* searchkey, bool ignore, uint8_t level, PIRP Irp) {
    int cmp;
    tree_data *td, *lasttd;
    KEY key2;

    cmp = 1;
    td = first_item(t);
    lasttd = NULL;

    if (!td) return STATUS_NOT_FOUND;

    key2 = *searchkey;

    do {
        cmp = keycmp(key2, td->key);

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

            while (find_prev_item(Vcb, &oldtp, tp, Irp)) {
                if (!tp->item->ignore)
                    return STATUS_SUCCESS;

                oldtp = *tp;
            }

            // if no valid entries before where item should be, look afterwards instead

            oldtp.tree = t;
            oldtp.item = td;

            while (find_next_item(Vcb, &oldtp, tp, true, Irp)) {
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

        if (!td->treeholder.tree) {
            Status = do_load_tree(Vcb, &td->treeholder, t->root, t, td, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("do_load_tree returned %08lx\n", Status);
                return Status;
            }
        }

        Status = find_item_in_tree(Vcb, td->treeholder.tree, tp, searchkey, ignore, level, Irp);

        return Status;
    }
}

__attribute__((nonnull(1,2,3,4)))
NTSTATUS find_item(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _Out_ traverse_ptr* tp,
                   _In_ const KEY* searchkey, _In_ bool ignore, _In_opt_ PIRP Irp) {
    NTSTATUS Status;

    if (!r->treeholder.tree) {
        Status = do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08lx\n", Status);
            return Status;
        }
    }

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, 0, Irp);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
        ERR("find_item_in_tree returned %08lx\n", Status);
    }

    return Status;
}

__attribute__((nonnull(1,2,3,4)))
NTSTATUS find_item_to_level(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, bool ignore, uint8_t level, PIRP Irp) {
    NTSTATUS Status;

    if (!r->treeholder.tree) {
        Status = do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08lx\n", Status);
            return Status;
        }
    }

    Status = find_item_in_tree(Vcb, r->treeholder.tree, tp, searchkey, ignore, level, Irp);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
        ERR("find_item_in_tree returned %08lx\n", Status);
    }

    if (Status == STATUS_NOT_FOUND) {
        tp->tree = r->treeholder.tree;
        tp->item = NULL;
    }

    return Status;
}

__attribute__((nonnull(1,2,3)))
bool find_next_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* next_tp, bool ignore, PIRP Irp) {
    tree* t;
    tree_data *td = NULL, *next;
    NTSTATUS Status;

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

        return true;
    }

    if (!tp->tree->parent)
        return false;

    t = tp->tree;
    do {
        if (t->parent) {
            td = next_item(t->parent, t->paritem);

            if (td) break;
        }

        t = t->parent;
    } while (t);

    if (!t)
        return false;

    if (!td->treeholder.tree) {
        Status = do_load_tree(Vcb, &td->treeholder, t->parent->root, t->parent, td, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08lx\n", Status);
            return false;
        }
    }

    t = td->treeholder.tree;

    while (t->header.level != 0) {
        tree_data* fi;

        fi = first_item(t);

        if (!fi)
            return false;

        if (!fi->treeholder.tree) {
            Status = do_load_tree(Vcb, &fi->treeholder, t->parent->root, t, fi, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("do_load_tree returned %08lx\n", Status);
                return false;
            }
        }

        t = fi->treeholder.tree;
    }

    next_tp->tree = t;
    next_tp->item = first_item(t);

    if (!next_tp->item)
        return false;

    if (!ignore && next_tp->item->ignore) {
        traverse_ptr ntp2;
        bool b;

        while ((b = find_next_item(Vcb, next_tp, &ntp2, true, Irp))) {
            *next_tp = ntp2;

            if (!next_tp->item->ignore)
                break;
        }

        if (!b)
            return false;
    }

#ifdef DEBUG_PARANOID
    if (!ignore && next_tp->item->ignore) {
        ERR("error - returning ignored item\n");
        int3;
    }
#endif

    return true;
}

__attribute__((nonnull(1)))
static __inline tree_data* last_item(tree* t) {
    LIST_ENTRY* le = t->itemlist.Blink;

    if (le == &t->itemlist)
        return NULL;

    return CONTAINING_RECORD(le, tree_data, list_entry);
}

__attribute__((nonnull(1,2,3)))
bool find_prev_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* prev_tp, PIRP Irp) {
    tree* t;
    tree_data* td;
    NTSTATUS Status;

    // FIXME - support ignore flag
    if (prev_item(tp->tree, tp->item)) {
        prev_tp->tree = tp->tree;
        prev_tp->item = prev_item(tp->tree, tp->item);

        return true;
    }

    if (!tp->tree->parent)
        return false;

    t = tp->tree;
    while (t && (!t->parent || !prev_item(t->parent, t->paritem))) {
        t = t->parent;
    }

    if (!t)
        return false;

    td = prev_item(t->parent, t->paritem);

    if (!td->treeholder.tree) {
        Status = do_load_tree(Vcb, &td->treeholder, t->parent->root, t->parent, td, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08lx\n", Status);
            return false;
        }
    }

    t = td->treeholder.tree;

    while (t->header.level != 0) {
        tree_data* li;

        li = last_item(t);

        if (!li->treeholder.tree) {
            Status = do_load_tree(Vcb, &li->treeholder, t->parent->root, t, li, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("do_load_tree returned %08lx\n", Status);
                return false;
            }
        }

        t = li->treeholder.tree;
    }

    prev_tp->tree = t;
    prev_tp->item = last_item(t);

    return true;
}

__attribute__((nonnull(1,2)))
void free_trees_root(device_extension* Vcb, root* r) {
    LIST_ENTRY* le;
    ULONG level;

    for (level = 0; level <= 255; level++) {
        bool empty = true;

        le = Vcb->trees.Flink;

        while (le != &Vcb->trees) {
            LIST_ENTRY* nextle = le->Flink;
            tree* t = CONTAINING_RECORD(le, tree, list_entry);

            if (t->root == r) {
                if (t->header.level == level) {
                    bool top = !t->paritem;

                    empty = false;

                    free_tree(t);
                    if (top && r->treeholder.tree == t)
                        r->treeholder.tree = NULL;

                    if (IsListEmpty(&Vcb->trees))
                        return;
                } else if (t->header.level > level)
                    empty = false;
            }

            le = nextle;
        }

        if (empty)
            break;
    }
}

__attribute__((nonnull(1)))
void free_trees(device_extension* Vcb) {
    LIST_ENTRY* le;
    ULONG level;

    for (level = 0; level <= 255; level++) {
        bool empty = true;

        le = Vcb->trees.Flink;

        while (le != &Vcb->trees) {
            LIST_ENTRY* nextle = le->Flink;
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            root* r = t->root;

            if (t->header.level == level) {
                bool top = !t->paritem;

                empty = false;

                free_tree(t);
                if (top && r->treeholder.tree == t)
                    r->treeholder.tree = NULL;

                if (IsListEmpty(&Vcb->trees))
                    break;
            } else if (t->header.level > level)
                empty = false;

            le = nextle;
        }

        if (empty)
            break;
    }

    reap_filerefs(Vcb, Vcb->root_fileref);
    reap_fcbs(Vcb);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(suppress: 28194)
#endif
__attribute__((nonnull(1,3)))
void add_rollback(_In_ LIST_ENTRY* rollback, _In_ enum rollback_type type, _In_ __drv_aliasesMem void* ptr) {
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
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(suppress: 28194)
#endif
__attribute__((nonnull(1,2)))
NTSTATUS insert_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ uint64_t obj_id,
                          _In_ uint8_t obj_type, _In_ uint64_t offset, _In_reads_bytes_opt_(size) _When_(return >= 0, __drv_aliasesMem) void* data,
                          _In_ uint16_t size, _Out_opt_ traverse_ptr* ptp, _In_opt_ PIRP Irp) {
    traverse_ptr tp;
    KEY searchkey;
    int cmp;
    tree_data *td, *paritem;
    tree* t;
#ifdef _DEBUG
    LIST_ENTRY* le;
    KEY firstitem = {0xcccccccccccccccc,0xcc,0xcccccccccccccccc};
#endif
    NTSTATUS Status;

    TRACE("(%p, %p, %I64x, %x, %I64x, %p, %x, %p)\n", Vcb, r, obj_id, obj_type, offset, data, size, ptp);

    searchkey.obj_id = obj_id;
    searchkey.obj_type = obj_type;
    searchkey.offset = offset;

    Status = find_item(Vcb, r, &tp, &searchkey, true, Irp);
    if (Status == STATUS_NOT_FOUND) {
        if (!r->treeholder.tree) {
            Status = do_load_tree(Vcb, &r->treeholder, r, NULL, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("do_load_tree returned %08lx\n", Status);
                return Status;
            }
        }

        if (r->treeholder.tree && r->treeholder.tree->header.num_items == 0) {
            tp.tree = r->treeholder.tree;
            tp.item = NULL;
        } else {
            ERR("error: unable to load tree for root %I64x\n", r->id);
            return STATUS_INTERNAL_ERROR;
        }
    } else if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        return Status;
    }

    TRACE("tp.item = %p\n", tp.item);

    if (tp.item) {
        TRACE("tp.item->key = %p\n", &tp.item->key);
        cmp = keycmp(searchkey, tp.item->key);

        if (cmp == 0 && !tp.item->ignore) {
            ERR("error: key (%I64x,%x,%I64x) already present\n", obj_id, obj_type, offset);
#ifdef DEBUG_PARANOID
            int3;
#endif
            return STATUS_INTERNAL_ERROR;
        }
    } else
        cmp = -1;

    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
    if (!td) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    td->key = searchkey;
    td->size = size;
    td->data = data;
    td->ignore = false;
    td->inserted = true;

#ifdef _DEBUG
    le = tp.tree->itemlist.Flink;
    while (le != &tp.tree->itemlist) {
        tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);
        firstitem = td2->key;
        break;
    }

    TRACE("inserting %I64x,%x,%I64x into tree beginning %I64x,%x,%I64x (num_items %x)\n", obj_id, obj_type, offset, firstitem.obj_id, firstitem.obj_type, firstitem.offset, tp.tree->header.num_items);
#endif

    if (cmp == -1) { // very first key in root
        InsertHeadList(&tp.tree->itemlist, &td->list_entry);

        paritem = tp.tree->paritem;
        while (paritem) {
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

    if (!tp.tree->write) {
        tp.tree->write = true;
        Vcb->need_write = true;
    }

    if (ptp)
        *ptp = tp;

    t = tp.tree;
    while (t) {
        if (t->paritem && t->paritem->ignore) {
            t->paritem->ignore = false;
            t->parent->header.num_items++;
            t->parent->size += sizeof(internal_node);
        }

        t->header.generation = Vcb->superblock.generation;
        t = t->parent;
    }

    return STATUS_SUCCESS;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

__attribute__((nonnull(1,2)))
NTSTATUS delete_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _Inout_ traverse_ptr* tp) {
    tree* t;
    uint64_t gen;

    TRACE("deleting item %I64x,%x,%I64x (ignore = %s)\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, tp->item->ignore ? "true" : "false");

#ifdef DEBUG_PARANOID
    if (tp->item->ignore) {
        ERR("trying to delete already-deleted item %I64x,%x,%I64x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
        int3;
        return STATUS_INTERNAL_ERROR;
    }
#endif

    tp->item->ignore = true;

    if (!tp->tree->write) {
        tp->tree->write = true;
        Vcb->need_write = true;
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

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1)))
void clear_rollback(LIST_ENTRY* rollback) {
    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveHeadList(rollback);
        rollback_item* ri = CONTAINING_RECORD(le, rollback_item, list_entry);

        switch (ri->type) {
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

__attribute__((nonnull(1,2)))
void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    rollback_item* ri;

    while (!IsListEmpty(rollback)) {
        LIST_ENTRY* le = RemoveTailList(rollback);
        ri = CONTAINING_RECORD(le, rollback_item, list_entry);

        switch (ri->type) {
            case ROLLBACK_INSERT_EXTENT:
            {
                rollback_extent* re = ri->ptr;

                re->ext->ignore = true;

                switch (re->ext->extent_data.type) {
                    case EXTENT_TYPE_REGULAR:
                    case EXTENT_TYPE_PREALLOC: {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)re->ext->extent_data.data;

                        if (ed2->size != 0) {
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);

                            if (c) {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, re->fcb->subvol->id,
                                                                re->fcb->inode, re->ext->offset - ed2->offset, -1,
                                                                re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, NULL);

                                if (!NT_SUCCESS(Status))
                                    ERR("update_changed_extent_ref returned %08lx\n", Status);
                            }

                            re->fcb->inode_item.st_blocks -= ed2->num_bytes;
                        }

                        break;
                    }

                    case EXTENT_TYPE_INLINE:
                        re->fcb->inode_item.st_blocks -= re->ext->extent_data.decoded_size;
                    break;
                }

                ExFreePool(re);
                break;
            }

            case ROLLBACK_DELETE_EXTENT:
            {
                rollback_extent* re = ri->ptr;

                re->ext->ignore = false;

                switch (re->ext->extent_data.type) {
                    case EXTENT_TYPE_REGULAR:
                    case EXTENT_TYPE_PREALLOC: {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)re->ext->extent_data.data;

                        if (ed2->size != 0) {
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);

                            if (c) {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, re->fcb->subvol->id,
                                                                re->fcb->inode, re->ext->offset - ed2->offset, 1,
                                                                re->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, NULL);

                                if (!NT_SUCCESS(Status))
                                    ERR("update_changed_extent_ref returned %08lx\n", Status);
                            }

                            re->fcb->inode_item.st_blocks += ed2->num_bytes;
                        }

                        break;
                    }

                    case EXTENT_TYPE_INLINE:
                        re->fcb->inode_item.st_blocks += re->ext->extent_data.decoded_size;
                    break;
                }

                ExFreePool(re);
                break;
            }

            case ROLLBACK_ADD_SPACE:
            case ROLLBACK_SUBTRACT_SPACE:
            {
                rollback_space* rs = ri->ptr;

                if (rs->chunk)
                    acquire_chunk_lock(rs->chunk, Vcb);

                if (ri->type == ROLLBACK_ADD_SPACE)
                    space_list_subtract2(rs->list, rs->list_size, rs->address, rs->length, NULL, NULL);
                else
                    space_list_add2(rs->list, rs->list_size, rs->address, rs->length, NULL, NULL);

                if (rs->chunk) {
                    if (ri->type == ROLLBACK_ADD_SPACE)
                        rs->chunk->used += rs->length;
                    else
                        rs->chunk->used -= rs->length;
                }

                if (rs->chunk) {
                    LIST_ENTRY* le2 = le->Blink;

                    while (le2 != rollback) {
                        LIST_ENTRY* le3 = le2->Blink;
                        rollback_item* ri2 = CONTAINING_RECORD(le2, rollback_item, list_entry);

                        if (ri2->type == ROLLBACK_ADD_SPACE || ri2->type == ROLLBACK_SUBTRACT_SPACE) {
                            rollback_space* rs2 = ri2->ptr;

                            if (rs2->chunk == rs->chunk) {
                                if (ri2->type == ROLLBACK_ADD_SPACE) {
                                    space_list_subtract2(rs2->list, rs2->list_size, rs2->address, rs2->length, NULL, NULL);
                                    rs->chunk->used += rs2->length;
                                } else {
                                    space_list_add2(rs2->list, rs2->list_size, rs2->address, rs2->length, NULL, NULL);
                                    rs->chunk->used -= rs2->length;
                                }

                                ExFreePool(rs2);
                                RemoveEntryList(&ri2->list_entry);
                                ExFreePool(ri2);
                            }
                        }

                        le2 = le3;
                    }

                    release_chunk_lock(rs->chunk, Vcb);
                }

                ExFreePool(rs);

                break;
            }
        }

        ExFreePool(ri);
    }
}

__attribute__((nonnull(1,2,3)))
static NTSTATUS find_tree_end(tree* t, KEY* tree_end, bool* no_end) {
    tree* p;

    p = t;
    do {
        tree_data* pi;

        if (!p->parent) {
            tree_end->obj_id = 0xffffffffffffffff;
            tree_end->obj_type = 0xff;
            tree_end->offset = 0xffffffffffffffff;
            *no_end = true;
            return STATUS_SUCCESS;
        }

        pi = p->paritem;

        if (pi->list_entry.Flink != &p->parent->itemlist) {
            tree_data* td = CONTAINING_RECORD(pi->list_entry.Flink, tree_data, list_entry);

            *tree_end = td->key;
            *no_end = false;
            return STATUS_SUCCESS;
        }

        p = p->parent;
    } while (p);

    return STATUS_INTERNAL_ERROR;
}

__attribute__((nonnull(1,2)))
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

__attribute__((nonnull(1,2,3)))
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
        ExFreePool(bi2);
        return;
    }

    ier->dir = bi->key.offset;
    ier->index = delir->index;
    ier->n = delir->n;
    RtlCopyMemory(ier->name, delir->name, delir->n);

    bi2->key.obj_id = bi->key.obj_id;
    bi2->key.obj_type = TYPE_INODE_EXTREF;
    bi2->key.offset = calc_crc32c((uint32_t)bi->key.offset, (uint8_t*)ier->name, ier->n);
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

__attribute__((nonnull(1,2,3,4,6,7)))
static NTSTATUS handle_batch_collision(device_extension* Vcb, batch_item* bi, tree* t, tree_data* td, tree_data* newtd, LIST_ENTRY* listhead, bool* ignore) {
    if (bi->operation == Batch_Delete || bi->operation == Batch_SetXattr || bi->operation == Batch_DirItem || bi->operation == Batch_InodeRef ||
        bi->operation == Batch_InodeExtRef || bi->operation == Batch_DeleteDirItem || bi->operation == Batch_DeleteInodeRef ||
        bi->operation == Batch_DeleteInodeExtRef || bi->operation == Batch_DeleteXattr) {
        uint16_t maxlen = (uint16_t)(Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node));

        switch (bi->operation) {
            case Batch_SetXattr: {
                if (td->size < sizeof(DIR_ITEM)) {
                    ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset, td->size, sizeof(DIR_ITEM));
                } else {
                    uint8_t* newdata;
                    ULONG size = td->size;
                    DIR_ITEM* newxa = (DIR_ITEM*)bi->data;
                    DIR_ITEM* xa = (DIR_ITEM*)td->data;

                    while (true) {
                        ULONG oldxasize;

                        if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                            ERR("(%I64x,%x,%I64x) was truncated\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);
                            break;
                        }

                        oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;

                        if (xa->n == newxa->n && RtlCompareMemory(newxa->name, xa->name, xa->n) == xa->n) {
                            uint64_t pos;

                            // replace

                            if (td->size + bi->datalen - oldxasize > maxlen)
                                ERR("DIR_ITEM would be over maximum size, truncating (%u + %u - %lu > %u)\n", td->size, bi->datalen, oldxasize, maxlen);

                            newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen - oldxasize, ALLOC_TAG);
                            if (!newdata) {
                                ERR("out of memory\n");
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }

                            pos = (uint8_t*)xa - td->data;
                            if (pos + oldxasize < td->size) // copy after changed xattr
                                RtlCopyMemory(newdata + pos + bi->datalen, td->data + pos + oldxasize, (ULONG)(td->size - pos - oldxasize));

                            if (pos > 0) { // copy before changed xattr
                                RtlCopyMemory(newdata, td->data, (ULONG)pos);
                                xa = (DIR_ITEM*)(newdata + pos);
                            } else
                                xa = (DIR_ITEM*)newdata;

                            RtlCopyMemory(xa, bi->data, bi->datalen);

                            bi->datalen = (uint16_t)min(td->size + bi->datalen - oldxasize, maxlen);

                            ExFreePool(bi->data);
                            bi->data = newdata;

                            break;
                        }

                        if ((uint8_t*)xa - (uint8_t*)td->data + oldxasize >= size) {
                            // not found, add to end of data

                            if (td->size + bi->datalen > maxlen)
                                ERR("DIR_ITEM would be over maximum size, truncating (%u + %u > %u)\n", td->size, bi->datalen, maxlen);

                            newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                            if (!newdata) {
                                ERR("out of memory\n");
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }

                            RtlCopyMemory(newdata, td->data, td->size);

                            xa = (DIR_ITEM*)((uint8_t*)newdata + td->size);
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
                uint8_t* newdata;

                if (td->size + bi->datalen > maxlen) {
                    ERR("DIR_ITEM would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                    return STATUS_INTERNAL_ERROR;
                }

                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(newdata, td->data, td->size);

                RtlCopyMemory(newdata + td->size, bi->data, bi->datalen);

                bi->datalen += td->size;

                ExFreePool(bi->data);
                bi->data = newdata;

                break;
            }

            case Batch_InodeRef: {
                uint8_t* newdata;

                if (td->size + bi->datalen > maxlen) {
                    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                        INODE_REF* ir = (INODE_REF*)bi->data;
                        INODE_EXTREF* ier;
                        uint16_t ierlen;
                        batch_item* bi2;
                        LIST_ENTRY* le;
                        bool inserted = false;

                        TRACE("INODE_REF would be too long, adding INODE_EXTREF instead\n");

                        ierlen = (uint16_t)(offsetof(INODE_EXTREF, name[0]) + ir->n);

                        ier = ExAllocatePoolWithTag(PagedPool, ierlen, ALLOC_TAG);
                        if (!ier) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        ier->dir = bi->key.offset;
                        ier->index = ir->index;
                        ier->n = ir->n;
                        RtlCopyMemory(ier->name, ir->name, ier->n);

                        bi2 = ExAllocateFromPagedLookasideList(&Vcb->batch_item_lookaside);
                        if (!bi2) {
                            ERR("out of memory\n");
                            ExFreePool(ier);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        bi2->key.obj_id = bi->key.obj_id;
                        bi2->key.obj_type = TYPE_INODE_EXTREF;
                        bi2->key.offset = calc_crc32c((uint32_t)ier->dir, (uint8_t*)ier->name, ier->n);
                        bi2->data = ier;
                        bi2->datalen = ierlen;
                        bi2->operation = Batch_InodeExtRef;

                        le = bi->list_entry.Flink;
                        while (le != listhead) {
                            batch_item* bi3 = CONTAINING_RECORD(le, batch_item, list_entry);

                            if (keycmp(bi3->key, bi2->key) != -1) {
                                InsertHeadList(le->Blink, &bi2->list_entry);
                                inserted = true;
                            }

                            le = le->Flink;
                        }

                        if (!inserted)
                            InsertTailList(listhead, &bi2->list_entry);

                        *ignore = true;
                        return STATUS_SUCCESS;
                    } else {
                        ERR("INODE_REF would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                        return STATUS_INTERNAL_ERROR;
                    }
                }

                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(newdata, td->data, td->size);

                RtlCopyMemory(newdata + td->size, bi->data, bi->datalen);

                bi->datalen += td->size;

                ExFreePool(bi->data);
                bi->data = newdata;

                break;
            }

            case Batch_InodeExtRef: {
                uint8_t* newdata;

                if (td->size + bi->datalen > maxlen) {
                    ERR("INODE_EXTREF would be over maximum size (%u + %u > %u)\n", td->size, bi->datalen, maxlen);
                    return STATUS_INTERNAL_ERROR;
                }

                newdata = ExAllocatePoolWithTag(PagedPool, td->size + bi->datalen, ALLOC_TAG);
                if (!newdata) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
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
                    ERR("DIR_ITEM was %u bytes, expected at least %Iu\n", td->size, sizeof(DIR_ITEM));
                    return STATUS_INTERNAL_ERROR;
                } else {
                    DIR_ITEM *di, *deldi;
                    LONG len;

                    deldi = (DIR_ITEM*)bi->data;
                    di = (DIR_ITEM*)td->data;
                    len = td->size;

                    do {
                        if (di->m == deldi->m && di->n == deldi->n && RtlCompareMemory(di->name, deldi->name, di->n + di->m) == di->n + di->m) {
                            uint16_t newlen = td->size - (sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m);

                            if (newlen == 0) {
                                TRACE("deleting DIR_ITEM\n");
                            } else {
                                uint8_t *newdi = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *dioff;
                                tree_data* td2;

                                if (!newdi) {
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                TRACE("modifying DIR_ITEM\n");

                                if ((uint8_t*)di > td->data) {
                                    RtlCopyMemory(newdi, td->data, (uint8_t*)di - td->data);
                                    dioff = newdi + ((uint8_t*)di - td->data);
                                } else {
                                    dioff = newdi;
                                }

                                if ((uint8_t*)&di->name[di->n + di->m] < td->data + td->size)
                                    RtlCopyMemory(dioff, &di->name[di->n + di->m], td->size - ((uint8_t*)&di->name[di->n + di->m] - td->data));

                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    ExFreePool(newdi);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newdi;
                                td2->ignore = false;
                                td2->inserted = true;

                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);

                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = true;
                            }

                            break;
                        }

                        len -= sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m;
                        di = (DIR_ITEM*)&di->name[di->n + di->m];

                        if (len == 0) {
                            TRACE("could not find DIR_ITEM to delete\n");
                            *ignore = true;
                            return STATUS_SUCCESS;
                        }
                    } while (len > 0);
                }
                break;
            }

            case Batch_DeleteInodeRef: {
                if (td->size < sizeof(INODE_REF)) {
                    ERR("INODE_REF was %u bytes, expected at least %Iu\n", td->size, sizeof(INODE_REF));
                    return STATUS_INTERNAL_ERROR;
                } else {
                    INODE_REF *ir, *delir;
                    ULONG len;
                    bool changed = false;

                    delir = (INODE_REF*)bi->data;
                    ir = (INODE_REF*)td->data;
                    len = td->size;

                    do {
                        uint16_t itemlen;

                        if (len < sizeof(INODE_REF) || len < offsetof(INODE_REF, name[0]) + ir->n) {
                            ERR("INODE_REF was truncated\n");
                            break;
                        }

                        itemlen = (uint16_t)offsetof(INODE_REF, name[0]) + ir->n;

                        if (ir->n == delir->n && RtlCompareMemory(ir->name, delir->name, ir->n) == ir->n) {
                            uint16_t newlen = td->size - itemlen;

                            changed = true;

                            if (newlen == 0)
                                TRACE("deleting INODE_REF\n");
                            else {
                                uint8_t *newir = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *iroff;
                                tree_data* td2;

                                if (!newir) {
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                TRACE("modifying INODE_REF\n");

                                if ((uint8_t*)ir > td->data) {
                                    RtlCopyMemory(newir, td->data, (uint8_t*)ir - td->data);
                                    iroff = newir + ((uint8_t*)ir - td->data);
                                } else {
                                    iroff = newir;
                                }

                                if ((uint8_t*)&ir->name[ir->n] < td->data + td->size)
                                    RtlCopyMemory(iroff, &ir->name[ir->n], td->size - ((uint8_t*)&ir->name[ir->n] - td->data));

                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    ExFreePool(newir);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newir;
                                td2->ignore = false;
                                td2->inserted = true;

                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);

                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = true;
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

                            *ignore = true;
                            return STATUS_SUCCESS;
                        } else
                            WARN("entry not found in INODE_REF\n");
                    }
                }

                break;
            }

            case Batch_DeleteInodeExtRef: {
                if (td->size < sizeof(INODE_EXTREF)) {
                    ERR("INODE_EXTREF was %u bytes, expected at least %Iu\n", td->size, sizeof(INODE_EXTREF));
                    return STATUS_INTERNAL_ERROR;
                } else {
                    INODE_EXTREF *ier, *delier;
                    ULONG len;

                    delier = (INODE_EXTREF*)bi->data;
                    ier = (INODE_EXTREF*)td->data;
                    len = td->size;

                    do {
                        uint16_t itemlen;

                        if (len < sizeof(INODE_EXTREF) || len < offsetof(INODE_EXTREF, name[0]) + ier->n) {
                            ERR("INODE_REF was truncated\n");
                            break;
                        }

                        itemlen = (uint16_t)offsetof(INODE_EXTREF, name[0]) + ier->n;

                        if (ier->dir == delier->dir && ier->n == delier->n && RtlCompareMemory(ier->name, delier->name, ier->n) == ier->n) {
                            uint16_t newlen = td->size - itemlen;

                            if (newlen == 0)
                                TRACE("deleting INODE_EXTREF\n");
                            else {
                                uint8_t *newier = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *ieroff;
                                tree_data* td2;

                                if (!newier) {
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                TRACE("modifying INODE_EXTREF\n");

                                if ((uint8_t*)ier > td->data) {
                                    RtlCopyMemory(newier, td->data, (uint8_t*)ier - td->data);
                                    ieroff = newier + ((uint8_t*)ier - td->data);
                                } else {
                                    ieroff = newier;
                                }

                                if ((uint8_t*)&ier->name[ier->n] < td->data + td->size)
                                    RtlCopyMemory(ieroff, &ier->name[ier->n], td->size - ((uint8_t*)&ier->name[ier->n] - td->data));

                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    ExFreePool(newier);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newier;
                                td2->ignore = false;
                                td2->inserted = true;

                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);

                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = true;
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

            case Batch_DeleteXattr: {
                if (td->size < sizeof(DIR_ITEM)) {
                    ERR("XATTR_ITEM was %u bytes, expected at least %Iu\n", td->size, sizeof(DIR_ITEM));
                    return STATUS_INTERNAL_ERROR;
                } else {
                    DIR_ITEM *di, *deldi;
                    LONG len;

                    deldi = (DIR_ITEM*)bi->data;
                    di = (DIR_ITEM*)td->data;
                    len = td->size;

                    do {
                        if (di->n == deldi->n && RtlCompareMemory(di->name, deldi->name, di->n) == di->n) {
                            uint16_t newlen = td->size - ((uint16_t)offsetof(DIR_ITEM, name[0]) + di->n + di->m);

                            if (newlen == 0)
                                TRACE("deleting XATTR_ITEM\n");
                            else {
                                uint8_t *newdi = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *dioff;
                                tree_data* td2;

                                if (!newdi) {
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                TRACE("modifying XATTR_ITEM\n");

                                if ((uint8_t*)di > td->data) {
                                    RtlCopyMemory(newdi, td->data, (uint8_t*)di - td->data);
                                    dioff = newdi + ((uint8_t*)di - td->data);
                                } else
                                    dioff = newdi;

                                if ((uint8_t*)&di->name[di->n + di->m] < td->data + td->size)
                                    RtlCopyMemory(dioff, &di->name[di->n + di->m], td->size - ((uint8_t*)&di->name[di->n + di->m] - td->data));

                                td2 = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                                if (!td2) {
                                    ERR("out of memory\n");
                                    ExFreePool(newdi);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                td2->key = bi->key;
                                td2->size = newlen;
                                td2->data = newdi;
                                td2->ignore = false;
                                td2->inserted = true;

                                InsertHeadList(td->list_entry.Blink, &td2->list_entry);

                                t->header.num_items++;
                                t->size += newlen + sizeof(leaf_node);
                                t->write = true;
                            }

                            break;
                        }

                        len -= sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m;
                        di = (DIR_ITEM*)&di->name[di->n + di->m];

                        if (len == 0) {
                            TRACE("could not find DIR_ITEM to delete\n");
                            *ignore = true;
                            return STATUS_SUCCESS;
                        }
                    } while (len > 0);
                }
                break;
            }

            case Batch_Delete:
                break;

            default:
                ERR("unexpected batch operation type\n");
                return STATUS_INTERNAL_ERROR;
        }

        // delete old item
        if (!td->ignore) {
            td->ignore = true;

            t->header.num_items--;
            t->size -= sizeof(leaf_node) + td->size;
            t->write = true;
        }

        if (newtd) {
            newtd->data = bi->data;
            newtd->size = bi->datalen;
            InsertHeadList(td->list_entry.Blink, &newtd->list_entry);
        }
    } else {
        ERR("(%I64x,%x,%I64x) already exists\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);
        return STATUS_INTERNAL_ERROR;
    }

    *ignore = false;
    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2)))
static NTSTATUS commit_batch_list_root(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, batch_root* br, PIRP Irp) {
    LIST_ENTRY* le;
    NTSTATUS Status;

    TRACE("root: %I64x\n", br->r->id);

    le = br->items.Flink;
    while (le != &br->items) {
        batch_item* bi = CONTAINING_RECORD(le, batch_item, list_entry);
        LIST_ENTRY *le2;
        traverse_ptr tp;
        KEY tree_end;
        bool no_end;
        tree_data *td, *listhead;
        int cmp;
        tree* t;
        bool ignore = false;

        TRACE("(%I64x,%x,%I64x)\n", bi->key.obj_id, bi->key.obj_type, bi->key.offset);

        Status = find_item(Vcb, br->r, &tp, &bi->key, true, Irp);
        if (!NT_SUCCESS(Status)) { // FIXME - handle STATUS_NOT_FOUND
            ERR("find_item returned %08lx\n", Status);
            return Status;
        }

        Status = find_tree_end(tp.tree, &tree_end, &no_end);
        if (!NT_SUCCESS(Status)) {
            ERR("find_tree_end returned %08lx\n", Status);
            return Status;
        }

        if (bi->operation == Batch_DeleteInode) {
            if (tp.item->key.obj_id == bi->key.obj_id) {
                bool ended = false;

                td = tp.item;

                if (!tp.item->ignore) {
                    tp.item->ignore = true;
                    tp.tree->header.num_items--;
                    tp.tree->size -= tp.item->size + sizeof(leaf_node);
                    tp.tree->write = true;
                }

                le2 = tp.item->list_entry.Flink;
                while (le2 != &tp.tree->itemlist) {
                    td = CONTAINING_RECORD(le2, tree_data, list_entry);

                    if (td->key.obj_id == bi->key.obj_id) {
                        if (!td->ignore) {
                            td->ignore = true;
                            tp.tree->header.num_items--;
                            tp.tree->size -= td->size + sizeof(leaf_node);
                            tp.tree->write = true;
                        }
                    } else {
                        ended = true;
                        break;
                    }

                    le2 = le2->Flink;
                }

                while (!ended) {
                    traverse_ptr next_tp;

                    tp.item = td;

                    if (!find_next_item(Vcb, &tp, &next_tp, false, Irp))
                        break;

                    tp = next_tp;

                    le2 = &tp.item->list_entry;
                    while (le2 != &tp.tree->itemlist) {
                        td = CONTAINING_RECORD(le2, tree_data, list_entry);

                        if (td->key.obj_id == bi->key.obj_id) {
                            if (!td->ignore) {
                                td->ignore = true;
                                tp.tree->header.num_items--;
                                tp.tree->size -= td->size + sizeof(leaf_node);
                                tp.tree->write = true;
                            }
                        } else {
                            ended = true;
                            break;
                        }

                        le2 = le2->Flink;
                    }
                }
            }
        } else if (bi->operation == Batch_DeleteExtentData) {
            if (tp.item->key.obj_id < bi->key.obj_id || (tp.item->key.obj_id == bi->key.obj_id && tp.item->key.obj_type < bi->key.obj_type)) {
                traverse_ptr tp2;

                if (find_next_item(Vcb, &tp, &tp2, false, Irp)) {
                    if (tp2.item->key.obj_id == bi->key.obj_id && tp2.item->key.obj_type == bi->key.obj_type) {
                        tp = tp2;
                        Status = find_tree_end(tp.tree, &tree_end, &no_end);
                        if (!NT_SUCCESS(Status)) {
                            ERR("find_tree_end returned %08lx\n", Status);
                            return Status;
                        }
                    }
                }
            }

            if (tp.item->key.obj_id == bi->key.obj_id && tp.item->key.obj_type == bi->key.obj_type) {
                bool ended = false;

                td = tp.item;

                if (!tp.item->ignore) {
                    tp.item->ignore = true;
                    tp.tree->header.num_items--;
                    tp.tree->size -= tp.item->size + sizeof(leaf_node);
                    tp.tree->write = true;
                }

                le2 = tp.item->list_entry.Flink;
                while (le2 != &tp.tree->itemlist) {
                    td = CONTAINING_RECORD(le2, tree_data, list_entry);

                    if (td->key.obj_id == bi->key.obj_id && td->key.obj_type == bi->key.obj_type) {
                        if (!td->ignore) {
                            td->ignore = true;
                            tp.tree->header.num_items--;
                            tp.tree->size -= td->size + sizeof(leaf_node);
                            tp.tree->write = true;
                        }
                    } else {
                        ended = true;
                        break;
                    }

                    le2 = le2->Flink;
                }

                while (!ended) {
                    traverse_ptr next_tp;

                    tp.item = td;

                    if (!find_next_item(Vcb, &tp, &next_tp, false, Irp))
                        break;

                    tp = next_tp;

                    le2 = &tp.item->list_entry;
                    while (le2 != &tp.tree->itemlist) {
                        td = CONTAINING_RECORD(le2, tree_data, list_entry);

                        if (td->key.obj_id == bi->key.obj_id && td->key.obj_type == bi->key.obj_type) {
                            if (!td->ignore) {
                                td->ignore = true;
                                tp.tree->header.num_items--;
                                tp.tree->size -= td->size + sizeof(leaf_node);
                                tp.tree->write = true;
                            }
                        } else {
                            ended = true;
                            break;
                        }

                        le2 = le2->Flink;
                    }
                }
            }
        } else if (bi->operation == Batch_DeleteFreeSpace) {
            if (tp.item->key.obj_id >= bi->key.obj_id && tp.item->key.obj_id < bi->key.obj_id + bi->key.offset) {
                bool ended = false;

                td = tp.item;

                if (!tp.item->ignore) {
                    tp.item->ignore = true;
                    tp.tree->header.num_items--;
                    tp.tree->size -= tp.item->size + sizeof(leaf_node);
                    tp.tree->write = true;
                }

                le2 = tp.item->list_entry.Flink;
                while (le2 != &tp.tree->itemlist) {
                    td = CONTAINING_RECORD(le2, tree_data, list_entry);

                    if (td->key.obj_id >= bi->key.obj_id && td->key.obj_id < bi->key.obj_id + bi->key.offset) {
                        if (!td->ignore) {
                            td->ignore = true;
                            tp.tree->header.num_items--;
                            tp.tree->size -= td->size + sizeof(leaf_node);
                            tp.tree->write = true;
                        }
                    } else {
                        ended = true;
                        break;
                    }

                    le2 = le2->Flink;
                }

                while (!ended) {
                    traverse_ptr next_tp;

                    tp.item = td;

                    if (!find_next_item(Vcb, &tp, &next_tp, false, Irp))
                        break;

                    tp = next_tp;

                    le2 = &tp.item->list_entry;
                    while (le2 != &tp.tree->itemlist) {
                        td = CONTAINING_RECORD(le2, tree_data, list_entry);

                        if (td->key.obj_id >= bi->key.obj_id && td->key.obj_id < bi->key.obj_id + bi->key.offset) {
                            if (!td->ignore) {
                                td->ignore = true;
                                tp.tree->header.num_items--;
                                tp.tree->size -= td->size + sizeof(leaf_node);
                                tp.tree->write = true;
                            }
                        } else {
                            ended = true;
                            break;
                        }

                        le2 = le2->Flink;
                    }
                }
            }
        } else {
            if (bi->operation == Batch_Delete || bi->operation == Batch_DeleteDirItem || bi->operation == Batch_DeleteInodeRef ||
                bi->operation == Batch_DeleteInodeExtRef || bi->operation == Batch_DeleteXattr)
                td = NULL;
            else {
                td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                if (!td) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                td->key = bi->key;
                td->size = bi->datalen;
                td->data = bi->data;
                td->ignore = false;
                td->inserted = true;
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
                if (tp.item->ignore) {
                    if (td)
                        InsertHeadList(tp.item->list_entry.Blink, &td->list_entry);
                } else {
                    Status = handle_batch_collision(Vcb, bi, tp.tree, tp.item, td, &br->items, &ignore);
                    if (!NT_SUCCESS(Status)) {
                        ERR("handle_batch_collision returned %08lx\n", Status);
#ifdef _DEBUG
                        int3;
#endif

                        if (td)
                            ExFreeToPagedLookasideList(&Vcb->tree_data_lookaside, td);

                        return Status;
                    }
                }
            } else if (td) {
                InsertHeadList(&tp.item->list_entry, &td->list_entry);
            }

            if (bi->operation == Batch_DeleteInodeRef && cmp != 0 && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                add_delete_inode_extref(Vcb, bi, &br->items);
            }

            if (!ignore && td) {
                tp.tree->header.num_items++;
                tp.tree->size += bi->datalen + sizeof(leaf_node);
                tp.tree->write = true;

                listhead = td;
            } else
                listhead = tp.item;

            while (listhead->list_entry.Blink != &tp.tree->itemlist) {
                tree_data* prevtd = CONTAINING_RECORD(listhead->list_entry.Blink, tree_data, list_entry);

                if (!keycmp(prevtd->key, listhead->key))
                    listhead = prevtd;
                else
                    break;
            }

            le2 = le->Flink;
            while (le2 != &br->items) {
                batch_item* bi2 = CONTAINING_RECORD(le2, batch_item, list_entry);

                if (bi2->operation == Batch_DeleteInode || bi2->operation == Batch_DeleteExtentData || bi2->operation == Batch_DeleteFreeSpace)
                    break;

                if (no_end || keycmp(bi2->key, tree_end) == -1) {
                    LIST_ENTRY* le3;
                    bool inserted = false;

                    ignore = false;

                    if (bi2->operation == Batch_Delete || bi2->operation == Batch_DeleteDirItem || bi2->operation == Batch_DeleteInodeRef ||
                        bi2->operation == Batch_DeleteInodeExtRef || bi2->operation == Batch_DeleteXattr)
                        td = NULL;
                    else {
                        td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
                        if (!td) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        td->key = bi2->key;
                        td->size = bi2->datalen;
                        td->data = bi2->data;
                        td->ignore = false;
                        td->inserted = true;
                    }

                    le3 = &listhead->list_entry;
                    while (le3 != &tp.tree->itemlist) {
                        tree_data* td2 = CONTAINING_RECORD(le3, tree_data, list_entry);

                        cmp = keycmp(bi2->key, td2->key);

                        if (cmp == 0) {
                            if (td2->ignore) {
                                if (td) {
                                    InsertHeadList(le3->Blink, &td->list_entry);
                                    inserted = true;
                                } else if (bi2->operation == Batch_DeleteInodeRef && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                                    add_delete_inode_extref(Vcb, bi2, &br->items);
                                }
                            } else {
                                Status = handle_batch_collision(Vcb, bi2, tp.tree, td2, td, &br->items, &ignore);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("handle_batch_collision returned %08lx\n", Status);
#ifdef _DEBUG
                                    int3;
#endif
                                    return Status;
                                }
                            }

                            inserted = true;
                            break;
                        } else if (cmp == -1) {
                            if (td) {
                                InsertHeadList(le3->Blink, &td->list_entry);
                                inserted = true;
                            } else if (bi2->operation == Batch_DeleteInodeRef && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                                add_delete_inode_extref(Vcb, bi2, &br->items);
                            }
                            break;
                        }

                        le3 = le3->Flink;
                    }

                    if (td) {
                        if (!inserted)
                            InsertTailList(&tp.tree->itemlist, &td->list_entry);

                        if (!ignore) {
                            tp.tree->header.num_items++;
                            tp.tree->size += bi2->datalen + sizeof(leaf_node);

                            listhead = td;
                        }
                    } else if (!inserted && bi2->operation == Batch_DeleteInodeRef && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                        add_delete_inode_extref(Vcb, bi2, &br->items);
                    }

                    while (listhead->list_entry.Blink != &tp.tree->itemlist) {
                        tree_data* prevtd = CONTAINING_RECORD(listhead->list_entry.Blink, tree_data, list_entry);

                        if (!keycmp(prevtd->key, listhead->key))
                            listhead = prevtd;
                        else
                            break;
                    }

                    le = le2;
                } else
                    break;

                le2 = le2->Flink;
            }

            t = tp.tree;
            while (t) {
                if (t->paritem && t->paritem->ignore) {
                    t->paritem->ignore = false;
                    t->parent->header.num_items++;
                    t->parent->size += sizeof(internal_node);
                }

                t->header.generation = Vcb->superblock.generation;
                t = t->parent;
            }
        }

        le = le->Flink;
    }

    // FIXME - remove as we are going along
    while (!IsListEmpty(&br->items)) {
        batch_item* bi = CONTAINING_RECORD(RemoveHeadList(&br->items), batch_item, list_entry);

        if ((bi->operation == Batch_DeleteDirItem || bi->operation == Batch_DeleteInodeRef ||
            bi->operation == Batch_DeleteInodeExtRef || bi->operation == Batch_DeleteXattr) && bi->data)
            ExFreePool(bi->data);

        ExFreeToPagedLookasideList(&Vcb->batch_item_lookaside, bi);
    }

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2)))
NTSTATUS commit_batch_list(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp) {
    NTSTATUS Status;

    while (!IsListEmpty(batchlist)) {
        LIST_ENTRY* le = RemoveHeadList(batchlist);
        batch_root* br2 = CONTAINING_RECORD(le, batch_root, list_entry);

        Status = commit_batch_list_root(Vcb, br2, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("commit_batch_list_root returned %08lx\n", Status);
            return Status;
        }

        ExFreePool(br2);
    }

    return STATUS_SUCCESS;
}
