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
#include "btrfsioctl.h"
#include <ntddstor.h>

typedef struct {
    UINT64 address;
    UINT64 new_address;
    tree_header* data;
    EXTENT_ITEM* ei;
    tree* t;
    BOOL system;
    LIST_ENTRY refs;
    LIST_ENTRY list_entry;
} metadata_reloc;

typedef struct {
    UINT8 type;
    UINT64 hash;

    union {
        TREE_BLOCK_REF tbr;
        SHARED_BLOCK_REF sbr;
    };

    metadata_reloc* parent;
    BOOL top;
    LIST_ENTRY list_entry;
} metadata_reloc_ref;

typedef struct {
    UINT64 address;
    UINT64 size;
    UINT64 new_address;
    chunk* newchunk;
    EXTENT_ITEM* ei;
    LIST_ENTRY refs;
    LIST_ENTRY list_entry;
} data_reloc;

typedef struct {
    UINT8 type;
    UINT64 hash;

    union {
        EXTENT_DATA_REF edr;
        SHARED_DATA_REF sdr;
    };

    metadata_reloc* parent;
    LIST_ENTRY list_entry;
} data_reloc_ref;

#ifndef __REACTOS__
#ifndef _MSC_VER // not in mingw yet
#define DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED 0x80000000
#endif
#endif

#define BALANCE_UNIT 0x100000 // only read 1 MB at a time

static NTSTATUS add_metadata_reloc(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* items, traverse_ptr* tp,
                                   BOOL skinny, metadata_reloc** mr2, chunk* c, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    metadata_reloc* mr;
    EXTENT_ITEM* ei;
    UINT16 len;
    UINT64 inline_rc;
    UINT8* ptr;

    mr = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc), ALLOC_TAG);
    if (!mr) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mr->address = tp->item->key.obj_id;
    mr->data = NULL;
    mr->ei = (EXTENT_ITEM*)tp->item->data;
    mr->system = FALSE;
    InitializeListHead(&mr->refs);

    Status = delete_tree_item(Vcb, tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08x\n", Status);
        ExFreePool(mr);
        return Status;
    }

    if (!c)
        c = get_chunk_from_address(Vcb, tp->item->key.obj_id);

    if (c) {
        acquire_chunk_lock(c, Vcb);

        c->used -= Vcb->superblock.node_size;

        space_list_add(c, tp->item->key.obj_id, Vcb->superblock.node_size, rollback);

        release_chunk_lock(c, Vcb);
    }

    ei = (EXTENT_ITEM*)tp->item->data;
    inline_rc = 0;

    len = tp->item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)tp->item->data + sizeof(EXTENT_ITEM);
    if (!skinny) {
        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }

    while (len > 0) {
        UINT8 secttype = *ptr;
        UINT16 sectlen = secttype == TYPE_TREE_BLOCK_REF ? sizeof(TREE_BLOCK_REF) : (secttype == TYPE_SHARED_BLOCK_REF ? sizeof(SHARED_BLOCK_REF) : 0);
        metadata_reloc_ref* ref;

        len--;

        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }

        ref = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc_ref), ALLOC_TAG);
        if (!ref) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (secttype == TYPE_TREE_BLOCK_REF) {
            ref->type = TYPE_TREE_BLOCK_REF;
            RtlCopyMemory(&ref->tbr, ptr + sizeof(UINT8), sizeof(TREE_BLOCK_REF));
            inline_rc++;
        } else if (secttype == TYPE_SHARED_BLOCK_REF) {
            ref->type = TYPE_SHARED_BLOCK_REF;
            RtlCopyMemory(&ref->sbr, ptr + sizeof(UINT8), sizeof(SHARED_BLOCK_REF));
            inline_rc++;
        } else {
            ERR("unexpected tree type %x\n", secttype);
            ExFreePool(ref);
            return STATUS_INTERNAL_ERROR;
        }

        ref->parent = NULL;
        ref->top = FALSE;
        InsertTailList(&mr->refs, &ref->list_entry);

        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
    }

    if (inline_rc < ei->refcount) { // look for non-inline entries
        traverse_ptr tp2 = *tp, next_tp;

        while (find_next_item(Vcb, &tp2, &next_tp, FALSE, NULL)) {
            tp2 = next_tp;

            if (tp2.item->key.obj_id == tp->item->key.obj_id) {
                if (tp2.item->key.obj_type == TYPE_TREE_BLOCK_REF) {
                    metadata_reloc_ref* ref = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    ref->type = TYPE_TREE_BLOCK_REF;
                    ref->tbr.offset = tp2.item->key.offset;
                    ref->parent = NULL;
                    ref->top = FALSE;
                    InsertTailList(&mr->refs, &ref->list_entry);

                    Status = delete_tree_item(Vcb, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }
                } else if (tp2.item->key.obj_type == TYPE_SHARED_BLOCK_REF) {
                    metadata_reloc_ref* ref = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    ref->type = TYPE_SHARED_BLOCK_REF;
                    ref->sbr.offset = tp2.item->key.offset;
                    ref->parent = NULL;
                    ref->top = FALSE;
                    InsertTailList(&mr->refs, &ref->list_entry);

                    Status = delete_tree_item(Vcb, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }
                }
            } else
                break;
        }
    }

    InsertTailList(items, &mr->list_entry);

    if (mr2)
        *mr2 = mr;

    return STATUS_SUCCESS;
}

static NTSTATUS add_metadata_reloc_parent(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* items,
                                          UINT64 address, metadata_reloc** mr2, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    KEY searchkey;
    traverse_ptr tp;
    BOOL skinny = FALSE;
    NTSTATUS Status;

    le = items->Flink;
    while (le != items) {
        metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);

        if (mr->address == address) {
            *mr2 = mr;
            return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_METADATA_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM))
        skinny = TRUE;
    else if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset == Vcb->superblock.node_size &&
             tp.item->size >= sizeof(EXTENT_ITEM)) {
        EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;

        if (!(ei->flags & EXTENT_ITEM_TREE_BLOCK)) {
            ERR("EXTENT_ITEM for %llx found, but tree flag not set\n", address);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        ERR("could not find valid EXTENT_ITEM for address %llx\n", address);
        return STATUS_INTERNAL_ERROR;
    }

    Status = add_metadata_reloc(Vcb, items, &tp, skinny, mr2, NULL, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_metadata_reloc returned %08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static void sort_metadata_reloc_refs(metadata_reloc* mr) {
    LIST_ENTRY newlist, *le;

    if (mr->refs.Flink == mr->refs.Blink) // 0 or 1 items
        return;

    // insertion sort

    InitializeListHead(&newlist);

    while (!IsListEmpty(&mr->refs)) {
        metadata_reloc_ref* ref = CONTAINING_RECORD(RemoveHeadList(&mr->refs), metadata_reloc_ref, list_entry);
        BOOL inserted = FALSE;

        if (ref->type == TYPE_TREE_BLOCK_REF)
            ref->hash = ref->tbr.offset;
        else if (ref->type == TYPE_SHARED_BLOCK_REF)
            ref->hash = ref->parent->new_address;

        le = newlist.Flink;
        while (le != &newlist) {
            metadata_reloc_ref* ref2 = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);

            if (ref->type < ref2->type || (ref->type == ref2->type && ref->hash > ref2->hash)) {
                InsertHeadList(le->Blink, &ref->list_entry);
                inserted = TRUE;
                break;
            }

            le = le->Flink;
        }

        if (!inserted)
            InsertTailList(&newlist, &ref->list_entry);
    }

    newlist.Flink->Blink = &mr->refs;
    newlist.Blink->Flink = &mr->refs;
    mr->refs.Flink = newlist.Flink;
    mr->refs.Blink = newlist.Blink;
}

static NTSTATUS add_metadata_reloc_extent_item(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, metadata_reloc* mr) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    UINT64 rc = 0;
    UINT16 inline_len;
    BOOL all_inline = TRUE;
    metadata_reloc_ref* first_noninline = NULL;
    EXTENT_ITEM* ei;
    UINT8* ptr;

    inline_len = sizeof(EXTENT_ITEM);
    if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA))
        inline_len += sizeof(EXTENT_ITEM2);

    sort_metadata_reloc_refs(mr);

    le = mr->refs.Flink;
    while (le != &mr->refs) {
        metadata_reloc_ref* ref = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);
        UINT16 extlen = 0;

        rc++;

        if (ref->type == TYPE_TREE_BLOCK_REF)
            extlen += sizeof(TREE_BLOCK_REF);
        else if (ref->type == TYPE_SHARED_BLOCK_REF)
            extlen += sizeof(SHARED_BLOCK_REF);

        if (all_inline) {
            if ((ULONG)(inline_len + 1 + extlen) > (Vcb->superblock.node_size >> 2)) {
                all_inline = FALSE;
                first_noninline = ref;
            } else
                inline_len += extlen + 1;
        }

        le = le->Flink;
    }

    ei = ExAllocatePoolWithTag(PagedPool, inline_len, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ei->refcount = rc;
    ei->generation = mr->ei->generation;
    ei->flags = mr->ei->flags;
    ptr = (UINT8*)&ei[1];

    if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
        EXTENT_ITEM2* ei2 = (EXTENT_ITEM2*)ptr;

        ei2->firstitem = *(KEY*)&mr->data[1];
        ei2->level = mr->data->level;

        ptr += sizeof(EXTENT_ITEM2);
    }

    le = mr->refs.Flink;
    while (le != &mr->refs) {
        metadata_reloc_ref* ref = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);

        if (ref == first_noninline)
            break;

        *ptr = ref->type;
        ptr++;

        if (ref->type == TYPE_TREE_BLOCK_REF) {
            TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)ptr;

            tbr->offset = ref->tbr.offset;

            ptr += sizeof(TREE_BLOCK_REF);
        } else if (ref->type == TYPE_SHARED_BLOCK_REF) {
            SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)ptr;

            sbr->offset = ref->parent->new_address;

            ptr += sizeof(SHARED_BLOCK_REF);
        }

        le = le->Flink;
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)
        Status = insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_METADATA_ITEM, mr->data->level, ei, inline_len, NULL, NULL);
    else
        Status = insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, ei, inline_len, NULL, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(ei);
        return Status;
    }

    if (!all_inline) {
        le = &first_noninline->list_entry;

        while (le != &mr->refs) {
            metadata_reloc_ref* ref = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);

            if (ref->type == TYPE_TREE_BLOCK_REF) {
                Status = insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_TREE_BLOCK_REF, ref->tbr.offset, NULL, 0, NULL, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    return Status;
                }
            } else if (ref->type == TYPE_SHARED_BLOCK_REF) {
                Status = insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_SHARED_BLOCK_REF, ref->parent->new_address, NULL, 0, NULL, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    return Status;
                }
            }

            le = le->Flink;
        }
    }

    if (ei->flags & EXTENT_ITEM_SHARED_BACKREFS || mr->data->flags & HEADER_FLAG_SHARED_BACKREF || !(mr->data->flags & HEADER_FLAG_MIXED_BACKREF)) {
        if (mr->data->level > 0) {
            UINT16 i;
            internal_node* in = (internal_node*)&mr->data[1];

            for (i = 0; i < mr->data->num_items; i++) {
                UINT64 sbrrc = find_extent_shared_tree_refcount(Vcb, in[i].address, mr->address, NULL);

                if (sbrrc > 0) {
                    SHARED_BLOCK_REF sbr;

                    sbr.offset = mr->new_address;

                    Status = increase_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }

                    sbr.offset = mr->address;

                    Status = decrease_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                      sbr.offset, FALSE, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("decrease_extent_refcount returned %08x\n", Status);
                        return Status;
                    }
                }
            }
        } else {
            UINT16 i;
            leaf_node* ln = (leaf_node*)&mr->data[1];

            for (i = 0; i < mr->data->num_items; i++) {
                if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)((UINT8*)mr->data + sizeof(tree_header) + ln[i].offset);

                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                        if (ed2->size > 0) { // not sparse
                            UINT32 sdrrc = find_extent_shared_data_refcount(Vcb, ed2->address, mr->address, NULL);

                            if (sdrrc > 0) {
                                SHARED_DATA_REF sdr;
                                chunk* c;

                                sdr.offset = mr->new_address;
                                sdr.count = sdrrc;

                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0, NULL);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("increase_extent_refcount returned %08x\n", Status);
                                    return Status;
                                }

                                sdr.offset = mr->address;

                                Status = decrease_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0,
                                                                  sdr.offset, FALSE, NULL);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("decrease_extent_refcount returned %08x\n", Status);
                                    return Status;
                                }

                                c = get_chunk_from_address(Vcb, ed2->address);

                                if (c) {
                                    // check changed_extents

                                    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, TRUE);

                                    le = c->changed_extents.Flink;

                                    while (le != &c->changed_extents) {
                                        changed_extent* ce = CONTAINING_RECORD(le, changed_extent, list_entry);

                                        if (ce->address == ed2->address) {
                                            LIST_ENTRY* le2;

                                            le2 = ce->refs.Flink;
                                            while (le2 != &ce->refs) {
                                                changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                                                if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == mr->address) {
                                                    cer->sdr.offset = mr->new_address;
                                                    break;
                                                }

                                                le2 = le2->Flink;
                                            }

                                            le2 = ce->old_refs.Flink;
                                            while (le2 != &ce->old_refs) {
                                                changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                                                if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == mr->address) {
                                                    cer->sdr.offset = mr->new_address;
                                                    break;
                                                }

                                                le2 = le2->Flink;
                                            }

                                            break;
                                        }

                                        le = le->Flink;
                                    }

                                    ExReleaseResourceLite(&c->changed_extents_lock);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS write_metadata_items(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* items,
                                     LIST_ENTRY* data_items, chunk* c, LIST_ENTRY* rollback) {
    LIST_ENTRY tree_writes, *le;
    NTSTATUS Status;
    traverse_ptr tp;
    UINT8 level, max_level = 0;
    chunk* newchunk = NULL;

    InitializeListHead(&tree_writes);

    le = items->Flink;
    while (le != items) {
        metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);
        LIST_ENTRY* le2;
        chunk* pc;

        mr->data = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!mr->data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = read_data(Vcb, mr->address, Vcb->superblock.node_size, NULL, TRUE, (UINT8*)mr->data,
                           c && mr->address >= c->offset && mr->address < c->offset + c->chunk_item->size ? c : NULL, &pc, NULL, 0, FALSE, NormalPagePriority);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data returned %08x\n", Status);
            return Status;
        }

        if (pc->chunk_item->type & BLOCK_FLAG_SYSTEM)
            mr->system = TRUE;

        if (data_items && mr->data->level == 0) {
            le2 = data_items->Flink;
            while (le2 != data_items) {
                data_reloc* dr = CONTAINING_RECORD(le2, data_reloc, list_entry);
                leaf_node* ln = (leaf_node*)&mr->data[1];
                UINT16 i;

                for (i = 0; i < mr->data->num_items; i++) {
                    if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                        EXTENT_DATA* ed = (EXTENT_DATA*)((UINT8*)mr->data + sizeof(tree_header) + ln[i].offset);

                        if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                            if (ed2->address == dr->address)
                                ed2->address = dr->new_address;
                        }
                    }
                }

                le2 = le2->Flink;
            }
        }

        if (mr->data->level > max_level)
            max_level = mr->data->level;

        le2 = mr->refs.Flink;
        while (le2 != &mr->refs) {
            metadata_reloc_ref* ref = CONTAINING_RECORD(le2, metadata_reloc_ref, list_entry);

            if (ref->type == TYPE_TREE_BLOCK_REF) {
                KEY* firstitem;
                root* r = NULL;
                LIST_ENTRY* le3;
                tree* t;

                firstitem = (KEY*)&mr->data[1];

                le3 = Vcb->roots.Flink;
                while (le3 != &Vcb->roots) {
                    root* r2 = CONTAINING_RECORD(le3, root, list_entry);

                    if (r2->id == ref->tbr.offset) {
                        r = r2;
                        break;
                    }

                    le3 = le3->Flink;
                }

                if (!r) {
                    ERR("could not find subvol with id %llx\n", ref->tbr.offset);
                    return STATUS_INTERNAL_ERROR;
                }

                Status = find_item_to_level(Vcb, r, &tp, firstitem, FALSE, mr->data->level + 1, NULL);
                if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
                    ERR("find_item_to_level returned %08x\n", Status);
                    return Status;
                }

                t = tp.tree;
                while (t && t->header.level < mr->data->level + 1) {
                    t = t->parent;
                }

                if (!t)
                    ref->top = TRUE;
                else {
                    metadata_reloc* mr2;

                    Status = add_metadata_reloc_parent(Vcb, items, t->header.address, &mr2, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_metadata_reloc_parent returned %08x\n", Status);
                        return Status;
                    }

                    ref->parent = mr2;
                }
            } else if (ref->type == TYPE_SHARED_BLOCK_REF) {
                metadata_reloc* mr2;

                Status = add_metadata_reloc_parent(Vcb, items, ref->sbr.offset, &mr2, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_metadata_reloc_parent returned %08x\n", Status);
                    return Status;
                }

                ref->parent = mr2;
            }

            le2 = le2->Flink;
        }

        le = le->Flink;
    }

    le = items->Flink;
    while (le != items) {
        metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);
        LIST_ENTRY* le2;
        UINT32 hash;

        mr->t = NULL;

        hash = calc_crc32c(0xffffffff, (UINT8*)&mr->address, sizeof(UINT64));

        le2 = Vcb->trees_ptrs[hash >> 24];

        if (le2) {
            while (le2 != &Vcb->trees_hash) {
                tree* t = CONTAINING_RECORD(le2, tree, list_entry_hash);

                if (t->header.address == mr->address) {
                    mr->t = t;
                    break;
                } else if (t->hash > hash)
                    break;

                le2 = le2->Flink;
            }
        }

        le = le->Flink;
    }

    for (level = 0; level <= max_level; level++) {
        le = items->Flink;
        while (le != items) {
            metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);

            if (mr->data->level == level) {
                BOOL done = FALSE;
                LIST_ENTRY* le2;
                tree_write* tw;
                UINT64 flags;
                tree* t3;

                if (mr->system)
                    flags = Vcb->system_flags;
                else if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS)
                    flags = Vcb->data_flags;
                else
                    flags = Vcb->metadata_flags;

                if (newchunk) {
                    acquire_chunk_lock(newchunk, Vcb);

                    if (newchunk->chunk_item->type == flags && find_metadata_address_in_chunk(Vcb, newchunk, &mr->new_address)) {
                        newchunk->used += Vcb->superblock.node_size;
                        space_list_subtract(newchunk, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                        done = TRUE;
                    }

                    release_chunk_lock(newchunk, Vcb);
                }

                if (!done) {
                    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

                    le2 = Vcb->chunks.Flink;
                    while (le2 != &Vcb->chunks) {
                        chunk* c2 = CONTAINING_RECORD(le2, chunk, list_entry);

                        if (!c2->readonly && !c2->reloc && c2 != newchunk && c2->chunk_item->type == flags) {
                            acquire_chunk_lock(c2, Vcb);

                            if ((c2->chunk_item->size - c2->used) >= Vcb->superblock.node_size) {
                                if (find_metadata_address_in_chunk(Vcb, c2, &mr->new_address)) {
                                    c2->used += Vcb->superblock.node_size;
                                    space_list_subtract(c2, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                                    release_chunk_lock(c2, Vcb);
                                    newchunk = c2;
                                    done = TRUE;
                                    break;
                                }
                            }

                            release_chunk_lock(c2, Vcb);
                        }

                        le2 = le2->Flink;
                    }

                    // allocate new chunk if necessary
                    if (!done) {
                        Status = alloc_chunk(Vcb, flags, &newchunk, FALSE);

                        if (!NT_SUCCESS(Status)) {
                            ERR("alloc_chunk returned %08x\n", Status);
                            ExReleaseResourceLite(&Vcb->chunk_lock);
                            goto end;
                        }

                        acquire_chunk_lock(newchunk, Vcb);

                        newchunk->balance_num = Vcb->balance.balance_num;

                        if (!find_metadata_address_in_chunk(Vcb, newchunk, &mr->new_address)) {
                            release_chunk_lock(newchunk, Vcb);
                            ExReleaseResourceLite(&Vcb->chunk_lock);
                            ERR("could not find address in new chunk\n");
                            Status = STATUS_DISK_FULL;
                            goto end;
                        } else {
                            newchunk->used += Vcb->superblock.node_size;
                            space_list_subtract(newchunk, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                        }

                        release_chunk_lock(newchunk, Vcb);
                    }

                    ExReleaseResourceLite(&Vcb->chunk_lock);
                }

                // update parents
                le2 = mr->refs.Flink;
                while (le2 != &mr->refs) {
                    metadata_reloc_ref* ref = CONTAINING_RECORD(le2, metadata_reloc_ref, list_entry);

                    if (ref->parent) {
                        UINT16 i;
                        internal_node* in = (internal_node*)&ref->parent->data[1];

                        for (i = 0; i < ref->parent->data->num_items; i++) {
                            if (in[i].address == mr->address) {
                                in[i].address = mr->new_address;
                                break;
                            }
                        }

                        if (ref->parent->t) {
                            LIST_ENTRY* le3;

                            le3 = ref->parent->t->itemlist.Flink;
                            while (le3 != &ref->parent->t->itemlist) {
                                tree_data* td = CONTAINING_RECORD(le3, tree_data, list_entry);

                                if (!td->inserted && td->treeholder.address == mr->address)
                                    td->treeholder.address = mr->new_address;

                                le3 = le3->Flink;
                            }
                        }
                    } else if (ref->top && ref->type == TYPE_TREE_BLOCK_REF) {
                        LIST_ENTRY* le3;
                        root* r = NULL;

                        // alter ROOT_ITEM

                        le3 = Vcb->roots.Flink;
                        while (le3 != &Vcb->roots) {
                            root* r2 = CONTAINING_RECORD(le3, root, list_entry);

                            if (r2->id == ref->tbr.offset) {
                                r = r2;
                                break;
                            }

                            le3 = le3->Flink;
                        }

                        if (r) {
                            r->treeholder.address = mr->new_address;

                            if (r == Vcb->root_root)
                                Vcb->superblock.root_tree_addr = mr->new_address;
                            else if (r == Vcb->chunk_root)
                                Vcb->superblock.chunk_tree_addr = mr->new_address;
                            else if (r->root_item.block_number == mr->address) {
                                KEY searchkey;
                                ROOT_ITEM* ri;

                                r->root_item.block_number = mr->new_address;

                                searchkey.obj_id = r->id;
                                searchkey.obj_type = TYPE_ROOT_ITEM;
                                searchkey.offset = 0xffffffffffffffff;

                                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, NULL);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("find_item returned %08x\n", Status);
                                    goto end;
                                }

                                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                                    Status = STATUS_INTERNAL_ERROR;
                                    goto end;
                                }

                                ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
                                if (!ri) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    goto end;
                                }

                                RtlCopyMemory(ri, &r->root_item, sizeof(ROOT_ITEM));

                                Status = delete_tree_item(Vcb, &tp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("delete_tree_item returned %08x\n", Status);
                                    goto end;
                                }

                                Status = insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, NULL);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("insert_tree_item returned %08x\n", Status);
                                    goto end;
                                }
                            }
                        }
                    }

                    le2 = le2->Flink;
                }

                mr->data->address = mr->new_address;

                t3 = mr->t;

                while (t3) {
                    UINT8 h;
                    BOOL inserted;
                    tree* t4 = NULL;

                    // check if tree loaded more than once
                    if (t3->list_entry.Flink != &Vcb->trees_hash) {
                        tree* nt = CONTAINING_RECORD(t3->list_entry_hash.Flink, tree, list_entry_hash);

                        if (nt->header.address == t3->header.address)
                            t4 = nt;
                    }

                    t3->header.address = mr->new_address;

                    h = t3->hash >> 24;

                    if (Vcb->trees_ptrs[h] == &t3->list_entry_hash) {
                        if (t3->list_entry_hash.Flink == &Vcb->trees_hash)
                            Vcb->trees_ptrs[h] = NULL;
                        else {
                            tree* t2 = CONTAINING_RECORD(t3->list_entry_hash.Flink, tree, list_entry_hash);

                            if (t2->hash >> 24 == h)
                                Vcb->trees_ptrs[h] = &t2->list_entry_hash;
                            else
                                Vcb->trees_ptrs[h] = NULL;
                        }
                    }

                    RemoveEntryList(&t3->list_entry_hash);

                    t3->hash = calc_crc32c(0xffffffff, (UINT8*)&t3->header.address, sizeof(UINT64));
                    h = t3->hash >> 24;

                    if (!Vcb->trees_ptrs[h]) {
                        UINT8 h2 = h;

                        le2 = Vcb->trees_hash.Flink;

                        if (h2 > 0) {
                            h2--;
                            do {
                                if (Vcb->trees_ptrs[h2]) {
                                    le2 = Vcb->trees_ptrs[h2];
                                    break;
                                }

                                h2--;
                            } while (h2 > 0);
                        }
                    } else
                        le2 = Vcb->trees_ptrs[h];

                    inserted = FALSE;
                    while (le2 != &Vcb->trees_hash) {
                        tree* t2 = CONTAINING_RECORD(le2, tree, list_entry_hash);

                        if (t2->hash >= t3->hash) {
                            InsertHeadList(le2->Blink, &t3->list_entry_hash);
                            inserted = TRUE;
                            break;
                        }

                        le2 = le2->Flink;
                    }

                    if (!inserted)
                        InsertTailList(&Vcb->trees_hash, &t3->list_entry_hash);

                    if (!Vcb->trees_ptrs[h] || t3->list_entry_hash.Flink == Vcb->trees_ptrs[h])
                        Vcb->trees_ptrs[h] = &t3->list_entry_hash;

                    if (data_items && level == 0) {
                        le2 = data_items->Flink;

                        while (le2 != data_items) {
                            data_reloc* dr = CONTAINING_RECORD(le2, data_reloc, list_entry);
                            LIST_ENTRY* le3 = t3->itemlist.Flink;

                            while (le3 != &t3->itemlist) {
                                tree_data* td = CONTAINING_RECORD(le3, tree_data, list_entry);

                                if (!td->inserted && td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;

                                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                                        if (ed2->address == dr->address)
                                            ed2->address = dr->new_address;
                                    }
                                }

                                le3 = le3->Flink;
                            }

                            le2 = le2->Flink;
                        }
                    }

                    t3 = t4;
                }

                *((UINT32*)mr->data) = ~calc_crc32c(0xffffffff, (UINT8*)&mr->data->fs_uuid, Vcb->superblock.node_size - sizeof(mr->data->csum));

                tw = ExAllocatePoolWithTag(PagedPool, sizeof(tree_write), ALLOC_TAG);
                if (!tw) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                tw->address = mr->new_address;
                tw->length = Vcb->superblock.node_size;
                tw->data = (UINT8*)mr->data;

                if (IsListEmpty(&tree_writes))
                    InsertTailList(&tree_writes, &tw->list_entry);
                else {
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
    }

    Status = do_tree_writes(Vcb, &tree_writes, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("do_tree_writes returned %08x\n", Status);
        goto end;
    }

    le = items->Flink;
    while (le != items) {
        metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);

        Status = add_metadata_reloc_extent_item(Vcb, mr);
        if (!NT_SUCCESS(Status)) {
            ERR("add_metadata_reloc_extent_item returned %08x\n", Status);
            goto end;
        }

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    while (!IsListEmpty(&tree_writes)) {
        tree_write* tw = CONTAINING_RECORD(RemoveHeadList(&tree_writes), tree_write, list_entry);
        ExFreePool(tw);
    }

    return Status;
}

static NTSTATUS balance_metadata_chunk(device_extension* Vcb, chunk* c, BOOL* changed) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BOOL b;
    LIST_ENTRY items, rollback;
    UINT32 loaded = 0;

    TRACE("chunk %llx\n", c->offset);

    InitializeListHead(&rollback);
    InitializeListHead(&items);

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    searchkey.obj_id = c->offset;
    searchkey.obj_type = TYPE_METADATA_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }

    do {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id >= c->offset + c->chunk_item->size)
            break;

        if (tp.item->key.obj_id >= c->offset && (tp.item->key.obj_type == TYPE_EXTENT_ITEM || tp.item->key.obj_type == TYPE_METADATA_ITEM)) {
            BOOL tree = FALSE, skinny = FALSE;

            if (tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
                tree = TRUE;
                skinny = TRUE;
            } else if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset == Vcb->superblock.node_size &&
                       tp.item->size >= sizeof(EXTENT_ITEM)) {
                EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;

                if (ei->flags & EXTENT_ITEM_TREE_BLOCK)
                    tree = TRUE;
            }

            if (tree) {
                Status = add_metadata_reloc(Vcb, &items, &tp, skinny, NULL, c, &rollback);

                if (!NT_SUCCESS(Status)) {
                    ERR("add_metadata_reloc returned %08x\n", Status);
                    goto end;
                }

                loaded++;

                if (loaded >= 64) // only do 64 at a time
                    break;
            }
        }

        b = find_next_item(Vcb, &tp, &next_tp, FALSE, NULL);

        if (b)
            tp = next_tp;
    } while (b);

    if (IsListEmpty(&items)) {
        *changed = FALSE;
        Status = STATUS_SUCCESS;
        goto end;
    } else
        *changed = TRUE;

    Status = write_metadata_items(Vcb, &items, NULL, c, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("write_metadata_items returned %08x\n", Status);
        goto end;
    }

    Status = STATUS_SUCCESS;

    Vcb->need_write = TRUE;

end:
    if (NT_SUCCESS(Status)) {
        Status = do_write(Vcb, NULL);
        if (!NT_SUCCESS(Status))
            ERR("do_write returned %08x\n", Status);
    }

    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    free_trees(Vcb);

    ExReleaseResourceLite(&Vcb->tree_lock);

    while (!IsListEmpty(&items)) {
        metadata_reloc* mr = CONTAINING_RECORD(RemoveHeadList(&items), metadata_reloc, list_entry);

        while (!IsListEmpty(&mr->refs)) {
            metadata_reloc_ref* ref = CONTAINING_RECORD(RemoveHeadList(&mr->refs), metadata_reloc_ref, list_entry);

            ExFreePool(ref);
        }

        ExFreePool(mr);
    }

    return Status;
}

static NTSTATUS data_reloc_add_tree_edr(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* metadata_items,
                                        data_reloc* dr, EXTENT_DATA_REF* edr, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    KEY searchkey;
    traverse_ptr tp;
    root* r = NULL;
    metadata_reloc* mr;
    UINT64 last_tree = 0;
    data_reloc_ref* ref;

    le = Vcb->roots.Flink;
    while (le != &Vcb->roots) {
        root* r2 = CONTAINING_RECORD(le, root, list_entry);

        if (r2->id == edr->root) {
            r = r2;
            break;
        }

        le = le->Flink;
    }

    if (!r) {
        ERR("could not find subvol %llx\n", edr->count);
        return STATUS_INTERNAL_ERROR;
    }

    searchkey.obj_id = edr->objid;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;

    Status = find_item(Vcb, r, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id < searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type < searchkey.obj_type)) {
        traverse_ptr tp2;

        if (find_next_item(Vcb, &tp, &tp2, FALSE, NULL))
            tp = tp2;
        else {
            ERR("could not find EXTENT_DATA for inode %llx in root %llx\n", searchkey.obj_id, r->id);
            return STATUS_INTERNAL_ERROR;
        }
    }

    ref = NULL;

    while (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        traverse_ptr tp2;

        if (tp.item->size >= sizeof(EXTENT_DATA)) {
            EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;

            if ((ed->type == EXTENT_TYPE_PREALLOC || ed->type == EXTENT_TYPE_REGULAR) && tp.item->size >= offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2)) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                if (ed2->address == dr->address && ed2->size == dr->size && tp.item->key.offset - ed2->offset == edr->offset) {
                    if (ref && last_tree == tp.tree->header.address)
                        ref->edr.count++;
                    else {
                        ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
                        if (!ref) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        ref->type = TYPE_EXTENT_DATA_REF;
                        RtlCopyMemory(&ref->edr, edr, sizeof(EXTENT_DATA_REF));
                        ref->edr.count = 1;

                        Status = add_metadata_reloc_parent(Vcb, metadata_items, tp.tree->header.address, &mr, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("add_metadata_reloc_parent returned %08x\n", Status);
                            ExFreePool(ref);
                            return Status;
                        }

                        last_tree = tp.tree->header.address;
                        ref->parent = mr;

                        InsertTailList(&dr->refs, &ref->list_entry);
                    }
                }
            }
        }

        if (find_next_item(Vcb, &tp, &tp2, FALSE, NULL))
            tp = tp2;
        else
            break;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS add_data_reloc(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* items, LIST_ENTRY* metadata_items,
                               traverse_ptr* tp, chunk* c, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    data_reloc* dr;
    EXTENT_ITEM* ei;
    UINT16 len;
    UINT64 inline_rc;
    UINT8* ptr;

    dr = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc), ALLOC_TAG);
    if (!dr) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dr->address = tp->item->key.obj_id;
    dr->size = tp->item->key.offset;
    dr->ei = (EXTENT_ITEM*)tp->item->data;
    InitializeListHead(&dr->refs);

    Status = delete_tree_item(Vcb, tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08x\n", Status);
        return Status;
    }

    if (!c)
        c = get_chunk_from_address(Vcb, tp->item->key.obj_id);

    if (c) {
        acquire_chunk_lock(c, Vcb);

        c->used -= tp->item->key.offset;

        space_list_add(c, tp->item->key.obj_id, tp->item->key.offset, rollback);

        release_chunk_lock(c, Vcb);
    }

    ei = (EXTENT_ITEM*)tp->item->data;
    inline_rc = 0;

    len = tp->item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)tp->item->data + sizeof(EXTENT_ITEM);

    while (len > 0) {
        UINT8 secttype = *ptr;
        UINT16 sectlen = secttype == TYPE_EXTENT_DATA_REF ? sizeof(EXTENT_DATA_REF) : (secttype == TYPE_SHARED_DATA_REF ? sizeof(SHARED_DATA_REF) : 0);

        len--;

        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }

        if (secttype == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));

            inline_rc += edr->count;

            Status = data_reloc_add_tree_edr(Vcb, metadata_items, dr, edr, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("data_reloc_add_tree_edr returned %08x\n", Status);
                return Status;
            }
        } else if (secttype == TYPE_SHARED_DATA_REF) {
            metadata_reloc* mr;
            data_reloc_ref* ref;

            ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
            if (!ref) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ref->type = TYPE_SHARED_DATA_REF;
            RtlCopyMemory(&ref->sdr, ptr + sizeof(UINT8), sizeof(SHARED_DATA_REF));
            inline_rc += ref->sdr.count;

            Status = add_metadata_reloc_parent(Vcb, metadata_items, ref->sdr.offset, &mr, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_metadata_reloc_parent returned %08x\n", Status);
                ExFreePool(ref);
                return Status;
            }

            ref->parent = mr;

            InsertTailList(&dr->refs, &ref->list_entry);
        } else {
            ERR("unexpected tree type %x\n", secttype);
            return STATUS_INTERNAL_ERROR;
        }


        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
    }

    if (inline_rc < ei->refcount) { // look for non-inline entries
        traverse_ptr tp2 = *tp, next_tp;

        while (find_next_item(Vcb, &tp2, &next_tp, FALSE, NULL)) {
            tp2 = next_tp;

            if (tp2.item->key.obj_id == tp->item->key.obj_id) {
                if (tp2.item->key.obj_type == TYPE_EXTENT_DATA_REF && tp2.item->size >= sizeof(EXTENT_DATA_REF)) {
                    Status = data_reloc_add_tree_edr(Vcb, metadata_items, dr, (EXTENT_DATA_REF*)tp2.item->data, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("data_reloc_add_tree_edr returned %08x\n", Status);
                        return Status;
                    }

                    Status = delete_tree_item(Vcb, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }
                } else if (tp2.item->key.obj_type == TYPE_SHARED_DATA_REF && tp2.item->size >= sizeof(UINT32)) {
                    metadata_reloc* mr;
                    data_reloc_ref* ref;

                    ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    ref->type = TYPE_SHARED_DATA_REF;
                    ref->sdr.offset = tp2.item->key.offset;
                    ref->sdr.count = *((UINT32*)tp2.item->data);

                    Status = add_metadata_reloc_parent(Vcb, metadata_items, ref->sdr.offset, &mr, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_metadata_reloc_parent returned %08x\n", Status);
                        ExFreePool(ref);
                        return Status;
                    }

                    ref->parent = mr;
                    InsertTailList(&dr->refs, &ref->list_entry);

                    Status = delete_tree_item(Vcb, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }
                }
            } else
                break;
        }
    }

    InsertTailList(items, &dr->list_entry);

    return STATUS_SUCCESS;
}

static void sort_data_reloc_refs(data_reloc* dr) {
    LIST_ENTRY newlist, *le;

    if (IsListEmpty(&dr->refs))
        return;

    // insertion sort

    InitializeListHead(&newlist);

    while (!IsListEmpty(&dr->refs)) {
        data_reloc_ref* ref = CONTAINING_RECORD(RemoveHeadList(&dr->refs), data_reloc_ref, list_entry);
        BOOL inserted = FALSE;

        if (ref->type == TYPE_EXTENT_DATA_REF)
            ref->hash = get_extent_data_ref_hash2(ref->edr.root, ref->edr.objid, ref->edr.offset);
        else if (ref->type == TYPE_SHARED_DATA_REF)
            ref->hash = ref->parent->new_address;

        le = newlist.Flink;
        while (le != &newlist) {
            data_reloc_ref* ref2 = CONTAINING_RECORD(le, data_reloc_ref, list_entry);

            if (ref->type < ref2->type || (ref->type == ref2->type && ref->hash > ref2->hash)) {
                InsertHeadList(le->Blink, &ref->list_entry);
                inserted = TRUE;
                break;
            }

            le = le->Flink;
        }

        if (!inserted)
            InsertTailList(&newlist, &ref->list_entry);
    }

    le = newlist.Flink;
    while (le != &newlist) {
        data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);

        if (le->Flink != &newlist) {
            data_reloc_ref* ref2 = CONTAINING_RECORD(le->Flink, data_reloc_ref, list_entry);

            if (ref->type == TYPE_EXTENT_DATA_REF && ref2->type == TYPE_EXTENT_DATA_REF && ref->edr.root == ref2->edr.root &&
                ref->edr.objid == ref2->edr.objid && ref->edr.offset == ref2->edr.offset) {
                RemoveEntryList(&ref2->list_entry);
                ref->edr.count += ref2->edr.count;
                ExFreePool(ref2);
                continue;
            }
        }

        le = le->Flink;
    }

    newlist.Flink->Blink = &dr->refs;
    newlist.Blink->Flink = &dr->refs;
    dr->refs.Flink = newlist.Flink;
    dr->refs.Blink = newlist.Blink;
}

static NTSTATUS add_data_reloc_extent_item(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, data_reloc* dr) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    UINT64 rc = 0;
    UINT16 inline_len;
    BOOL all_inline = TRUE;
    data_reloc_ref* first_noninline = NULL;
    EXTENT_ITEM* ei;
    UINT8* ptr;

    inline_len = sizeof(EXTENT_ITEM);

    sort_data_reloc_refs(dr);

    le = dr->refs.Flink;
    while (le != &dr->refs) {
        data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);
        UINT16 extlen = 0;

        if (ref->type == TYPE_EXTENT_DATA_REF) {
            extlen += sizeof(EXTENT_DATA_REF);
            rc += ref->edr.count;
        } else if (ref->type == TYPE_SHARED_DATA_REF) {
            extlen += sizeof(SHARED_DATA_REF);
            rc++;
        }

        if (all_inline) {
            if ((ULONG)(inline_len + 1 + extlen) > (Vcb->superblock.node_size >> 2)) {
                all_inline = FALSE;
                first_noninline = ref;
            } else
                inline_len += extlen + 1;
        }

        le = le->Flink;
    }

    ei = ExAllocatePoolWithTag(PagedPool, inline_len, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ei->refcount = rc;
    ei->generation = dr->ei->generation;
    ei->flags = dr->ei->flags;
    ptr = (UINT8*)&ei[1];

    le = dr->refs.Flink;
    while (le != &dr->refs) {
        data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);

        if (ref == first_noninline)
            break;

        *ptr = ref->type;
        ptr++;

        if (ref->type == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)ptr;

            RtlCopyMemory(edr, &ref->edr, sizeof(EXTENT_DATA_REF));

            ptr += sizeof(EXTENT_DATA_REF);
        } else if (ref->type == TYPE_SHARED_DATA_REF) {
            SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)ptr;

            sdr->offset = ref->parent->new_address;
            sdr->count = ref->sdr.count;

            ptr += sizeof(SHARED_DATA_REF);
        }

        le = le->Flink;
    }

    Status = insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_EXTENT_ITEM, dr->size, ei, inline_len, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        return Status;
    }

    if (!all_inline) {
        le = &first_noninline->list_entry;

        while (le != &dr->refs) {
            data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);

            if (ref->type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* edr;

                edr = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA_REF), ALLOC_TAG);
                if (!edr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(edr, &ref->edr, sizeof(EXTENT_DATA_REF));

                Status = insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_EXTENT_DATA_REF, ref->hash, edr, sizeof(EXTENT_DATA_REF), NULL, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    return Status;
                }
            } else if (ref->type == TYPE_SHARED_DATA_REF) {
                UINT32* sdr;

                sdr = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32), ALLOC_TAG);
                if (!sdr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                *sdr = ref->sdr.count;

                Status = insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_SHARED_DATA_REF, ref->parent->new_address, sdr, sizeof(UINT32), NULL, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    return Status;
                }
            }

            le = le->Flink;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS balance_data_chunk(device_extension* Vcb, chunk* c, BOOL* changed) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BOOL b;
    LIST_ENTRY items, metadata_items, rollback, *le;
    UINT64 loaded = 0, num_loaded = 0;
    chunk* newchunk = NULL;
    UINT8* data = NULL;

    TRACE("chunk %llx\n", c->offset);

    InitializeListHead(&rollback);
    InitializeListHead(&items);
    InitializeListHead(&metadata_items);

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    searchkey.obj_id = c->offset;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }

    do {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id >= c->offset + c->chunk_item->size)
            break;

        if (tp.item->key.obj_id >= c->offset && tp.item->key.obj_type == TYPE_EXTENT_ITEM) {
            BOOL tree = FALSE;

            if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
                EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;

                if (ei->flags & EXTENT_ITEM_TREE_BLOCK)
                    tree = TRUE;
            }

            if (!tree) {
                Status = add_data_reloc(Vcb, &items, &metadata_items, &tp, c, &rollback);

                if (!NT_SUCCESS(Status)) {
                    ERR("add_data_reloc returned %08x\n", Status);
                    goto end;
                }

                loaded += tp.item->key.offset;
                num_loaded++;

                if (loaded >= 0x1000000 || num_loaded >= 100) // only do so much at a time, so we don't block too obnoxiously
                    break;
            }
        }

        b = find_next_item(Vcb, &tp, &next_tp, FALSE, NULL);

        if (b)
            tp = next_tp;
    } while (b);

    if (IsListEmpty(&items)) {
        *changed = FALSE;
        Status = STATUS_SUCCESS;
        goto end;
    } else
        *changed = TRUE;

    data = ExAllocatePoolWithTag(PagedPool, BALANCE_UNIT, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    le = items.Flink;
    while (le != &items) {
        data_reloc* dr = CONTAINING_RECORD(le, data_reloc, list_entry);
        BOOL done = FALSE;
        LIST_ENTRY* le2;
        UINT32* csum;
        RTL_BITMAP bmp;
        ULONG* bmparr;
        ULONG runlength, index, lastoff;

        if (newchunk) {
            acquire_chunk_lock(newchunk, Vcb);

            if (find_data_address_in_chunk(Vcb, newchunk, dr->size, &dr->new_address)) {
                newchunk->used += dr->size;
                space_list_subtract(newchunk, FALSE, dr->new_address, dr->size, &rollback);
                done = TRUE;
            }

            release_chunk_lock(newchunk, Vcb);
        }

        if (!done) {
            ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

            le2 = Vcb->chunks.Flink;
            while (le2 != &Vcb->chunks) {
                chunk* c2 = CONTAINING_RECORD(le2, chunk, list_entry);

                if (!c2->readonly && !c2->reloc && c2 != newchunk && c2->chunk_item->type == Vcb->data_flags) {
                    acquire_chunk_lock(c2, Vcb);

                    if ((c2->chunk_item->size - c2->used) >= dr->size) {
                        if (find_data_address_in_chunk(Vcb, c2, dr->size, &dr->new_address)) {
                            c2->used += dr->size;
                            space_list_subtract(c2, FALSE, dr->new_address, dr->size, &rollback);
                            release_chunk_lock(c2, Vcb);
                            newchunk = c2;
                            done = TRUE;
                            break;
                        }
                    }

                    release_chunk_lock(c2, Vcb);
                }

                le2 = le2->Flink;
            }

            // allocate new chunk if necessary
            if (!done) {
                Status = alloc_chunk(Vcb, Vcb->data_flags, &newchunk, FALSE);

                if (!NT_SUCCESS(Status)) {
                    ERR("alloc_chunk returned %08x\n", Status);
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    goto end;
                }

                acquire_chunk_lock(newchunk, Vcb);

                newchunk->balance_num = Vcb->balance.balance_num;

                if (!find_data_address_in_chunk(Vcb, newchunk, dr->size, &dr->new_address)) {
                    release_chunk_lock(newchunk, Vcb);
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    ERR("could not find address in new chunk\n");
                    Status = STATUS_DISK_FULL;
                    goto end;
                } else {
                    newchunk->used += dr->size;
                    space_list_subtract(newchunk, FALSE, dr->new_address, dr->size, &rollback);
                }

                release_chunk_lock(newchunk, Vcb);
            }

            ExReleaseResourceLite(&Vcb->chunk_lock);
        }

        dr->newchunk = newchunk;

        bmparr = ExAllocatePoolWithTag(PagedPool, (ULONG)sector_align((dr->size / Vcb->superblock.sector_size) + 1, sizeof(ULONG)), ALLOC_TAG);
        if (!bmparr) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        csum = ExAllocatePoolWithTag(PagedPool, (ULONG)(dr->size * sizeof(UINT32) / Vcb->superblock.sector_size), ALLOC_TAG);
        if (!csum) {
            ERR("out of memory\n");
            ExFreePool(bmparr);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlInitializeBitMap(&bmp, bmparr, (ULONG)(dr->size / Vcb->superblock.sector_size));
        RtlSetAllBits(&bmp); // 1 = no csum, 0 = csum

        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = dr->address;

        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
            ERR("find_item returned %08x\n", Status);
            ExFreePool(csum);
            ExFreePool(bmparr);
            goto end;
        }

        if (Status != STATUS_NOT_FOUND) {
            do {
                traverse_ptr next_tp;

                if (tp.item->key.obj_type == TYPE_EXTENT_CSUM) {
                    if (tp.item->key.offset >= dr->address + dr->size)
                        break;
                    else if (tp.item->size >= sizeof(UINT32) && tp.item->key.offset + (tp.item->size * Vcb->superblock.sector_size / sizeof(UINT32)) >= dr->address) {
                        UINT64 cs = max(dr->address, tp.item->key.offset);
                        UINT64 ce = min(dr->address + dr->size, tp.item->key.offset + (tp.item->size * Vcb->superblock.sector_size / sizeof(UINT32)));

                        RtlCopyMemory(csum + ((cs - dr->address) / Vcb->superblock.sector_size),
                                      tp.item->data + ((cs - tp.item->key.offset) * sizeof(UINT32) / Vcb->superblock.sector_size),
                                      (ULONG)((ce - cs) * sizeof(UINT32) / Vcb->superblock.sector_size));

                        RtlClearBits(&bmp, (ULONG)((cs - dr->address) / Vcb->superblock.sector_size), (ULONG)((ce - cs) / Vcb->superblock.sector_size));

                        if (ce == dr->address + dr->size)
                            break;
                    }
                }

                if (find_next_item(Vcb, &tp, &next_tp, FALSE, NULL))
                    tp = next_tp;
                else
                    break;
            } while (TRUE);
        }

        lastoff = 0;
        runlength = RtlFindFirstRunClear(&bmp, &index);

        while (runlength != 0) {
            if (index > lastoff) {
                ULONG off = lastoff;
                ULONG size = index - lastoff;

                // handle no csum run
                do {
                    ULONG rl;

                    if (size * Vcb->superblock.sector_size > BALANCE_UNIT)
                        rl = BALANCE_UNIT / Vcb->superblock.sector_size;
                    else
                        rl = size;

                    Status = read_data(Vcb, dr->address + (off * Vcb->superblock.sector_size), rl * Vcb->superblock.sector_size, NULL, FALSE, data,
                                       c, NULL, NULL, 0, FALSE, NormalPagePriority);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_data returned %08x\n", Status);
                        ExFreePool(csum);
                        ExFreePool(bmparr);
                        goto end;
                    }

                    Status = write_data_complete(Vcb, dr->new_address + (off * Vcb->superblock.sector_size), data, rl * Vcb->superblock.sector_size,
                                                 NULL, newchunk, FALSE, 0, NormalPagePriority);
                    if (!NT_SUCCESS(Status)) {
                        ERR("write_data_complete returned %08x\n", Status);
                        ExFreePool(csum);
                        ExFreePool(bmparr);
                        goto end;
                    }

                    size -= rl;
                    off += rl;
                } while (size > 0);
            }

            add_checksum_entry(Vcb, dr->new_address + (index * Vcb->superblock.sector_size), runlength, &csum[index], NULL);
            add_checksum_entry(Vcb, dr->address + (index * Vcb->superblock.sector_size), runlength, NULL, NULL);

            // handle csum run
            do {
                ULONG rl;

                if (runlength * Vcb->superblock.sector_size > BALANCE_UNIT)
                    rl = BALANCE_UNIT / Vcb->superblock.sector_size;
                else
                    rl = runlength;

                Status = read_data(Vcb, dr->address + (index * Vcb->superblock.sector_size), rl * Vcb->superblock.sector_size, &csum[index], FALSE, data,
                                   c, NULL, NULL, 0, FALSE, NormalPagePriority);
                if (!NT_SUCCESS(Status)) {
                    ERR("read_data returned %08x\n", Status);
                    ExFreePool(csum);
                    ExFreePool(bmparr);
                    goto end;
                }

                Status = write_data_complete(Vcb, dr->new_address + (index * Vcb->superblock.sector_size), data, rl * Vcb->superblock.sector_size,
                                             NULL, newchunk, FALSE, 0, NormalPagePriority);
                if (!NT_SUCCESS(Status)) {
                    ERR("write_data_complete returned %08x\n", Status);
                    ExFreePool(csum);
                    ExFreePool(bmparr);
                    goto end;
                }

                runlength -= rl;
                index += rl;
            } while (runlength > 0);

            lastoff = index;
            runlength = RtlFindNextForwardRunClear(&bmp, index, &index);
        }

        ExFreePool(csum);
        ExFreePool(bmparr);

        // handle final nocsum run
        if (lastoff < dr->size / Vcb->superblock.sector_size) {
            ULONG off = lastoff;
            ULONG size = (ULONG)((dr->size / Vcb->superblock.sector_size) - lastoff);

            do {
                ULONG rl;

                if (size * Vcb->superblock.sector_size > BALANCE_UNIT)
                    rl = BALANCE_UNIT / Vcb->superblock.sector_size;
                else
                    rl = size;

                Status = read_data(Vcb, dr->address + (off * Vcb->superblock.sector_size), rl * Vcb->superblock.sector_size, NULL, FALSE, data,
                                   c, NULL, NULL, 0, FALSE, NormalPagePriority);
                if (!NT_SUCCESS(Status)) {
                    ERR("read_data returned %08x\n", Status);
                    goto end;
                }

                Status = write_data_complete(Vcb, dr->new_address + (off * Vcb->superblock.sector_size), data, rl * Vcb->superblock.sector_size,
                                             NULL, newchunk, FALSE, 0, NormalPagePriority);
                if (!NT_SUCCESS(Status)) {
                    ERR("write_data_complete returned %08x\n", Status);
                    goto end;
                }

                size -= rl;
                off += rl;
            } while (size > 0);
        }

        le = le->Flink;
    }

    ExFreePool(data);
    data = NULL;

    Status = write_metadata_items(Vcb, &metadata_items, &items, NULL, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("write_metadata_items returned %08x\n", Status);
        goto end;
    }

    le = items.Flink;
    while (le != &items) {
        data_reloc* dr = CONTAINING_RECORD(le, data_reloc, list_entry);

        Status = add_data_reloc_extent_item(Vcb, dr);
        if (!NT_SUCCESS(Status)) {
            ERR("add_data_reloc_extent_item returned %08x\n", Status);
            goto end;
        }

        le = le->Flink;
    }

    le = c->changed_extents.Flink;
    while (le != &c->changed_extents) {
        LIST_ENTRY *le2, *le3;
        changed_extent* ce = CONTAINING_RECORD(le, changed_extent, list_entry);

        le3 = le->Flink;

        le2 = items.Flink;
        while (le2 != &items) {
            data_reloc* dr = CONTAINING_RECORD(le2, data_reloc, list_entry);

            if (ce->address == dr->address) {
                ce->address = dr->new_address;
                RemoveEntryList(&ce->list_entry);
                InsertTailList(&dr->newchunk->changed_extents, &ce->list_entry);
                break;
            }

            le2 = le2->Flink;
        }

        le = le3;
    }

    Status = STATUS_SUCCESS;

    Vcb->need_write = TRUE;

end:
    if (NT_SUCCESS(Status)) {
        // update extents in cache inodes before we flush
        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);

            if (c2->cache) {
                LIST_ENTRY* le2;

                ExAcquireResourceExclusiveLite(c2->cache->Header.Resource, TRUE);

                le2 = c2->cache->extents.Flink;
                while (le2 != &c2->cache->extents) {
                    extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                    if (!ext->ignore) {
                        if (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                            if (ed2->size > 0 && ed2->address >= c->offset && ed2->address < c->offset + c->chunk_item->size) {
                                LIST_ENTRY* le3 = items.Flink;
                                while (le3 != &items) {
                                    data_reloc* dr = CONTAINING_RECORD(le3, data_reloc, list_entry);

                                    if (ed2->address == dr->address) {
                                        ed2->address = dr->new_address;
                                        break;
                                    }

                                    le3 = le3->Flink;
                                }
                            }
                        }
                    }

                    le2 = le2->Flink;
                }

                ExReleaseResourceLite(c2->cache->Header.Resource);
            }

            le = le->Flink;
        }

        Status = do_write(Vcb, NULL);
        if (!NT_SUCCESS(Status))
            ERR("do_write returned %08x\n", Status);
    }

    if (NT_SUCCESS(Status)) {
        clear_rollback(&rollback);

        // update open FCBs
        // FIXME - speed this up(?)

        le = Vcb->all_fcbs.Flink;
        while (le != &Vcb->all_fcbs) {
            struct _fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_all);
            LIST_ENTRY* le2;

            ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);

            le2 = fcb->extents.Flink;
            while (le2 != &fcb->extents) {
                extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                if (!ext->ignore) {
                    if (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                        if (ed2->size > 0 && ed2->address >= c->offset && ed2->address < c->offset + c->chunk_item->size) {
                            LIST_ENTRY* le3 = items.Flink;
                            while (le3 != &items) {
                                data_reloc* dr = CONTAINING_RECORD(le3, data_reloc, list_entry);

                                if (ed2->address == dr->address) {
                                    ed2->address = dr->new_address;
                                    break;
                                }

                                le3 = le3->Flink;
                            }
                        }
                    }
                }

                le2 = le2->Flink;
            }

            ExReleaseResourceLite(fcb->Header.Resource);

            le = le->Flink;
        }
    } else
        do_rollback(Vcb, &rollback);

    free_trees(Vcb);

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (data)
        ExFreePool(data);

    while (!IsListEmpty(&items)) {
        data_reloc* dr = CONTAINING_RECORD(RemoveHeadList(&items), data_reloc, list_entry);

        while (!IsListEmpty(&dr->refs)) {
            data_reloc_ref* ref = CONTAINING_RECORD(RemoveHeadList(&dr->refs), data_reloc_ref, list_entry);

            ExFreePool(ref);
        }

        ExFreePool(dr);
    }

    while (!IsListEmpty(&metadata_items)) {
        metadata_reloc* mr = CONTAINING_RECORD(RemoveHeadList(&metadata_items), metadata_reloc, list_entry);

        while (!IsListEmpty(&mr->refs)) {
            metadata_reloc_ref* ref = CONTAINING_RECORD(RemoveHeadList(&mr->refs), metadata_reloc_ref, list_entry);

            ExFreePool(ref);
        }

        ExFreePool(mr);
    }

    return Status;
}

static __inline UINT64 get_chunk_dup_type(chunk* c) {
    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
        return BLOCK_FLAG_RAID0;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID1)
        return BLOCK_FLAG_RAID1;
    else if (c->chunk_item->type & BLOCK_FLAG_DUPLICATE)
        return BLOCK_FLAG_DUPLICATE;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
        return BLOCK_FLAG_RAID10;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
        return BLOCK_FLAG_RAID5;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
        return BLOCK_FLAG_RAID6;
    else
        return BLOCK_FLAG_SINGLE;
}

static BOOL should_balance_chunk(device_extension* Vcb, UINT8 sort, chunk* c) {
    btrfs_balance_opts* opts;

    opts = &Vcb->balance.opts[sort];

    if (!(opts->flags & BTRFS_BALANCE_OPTS_ENABLED))
        return FALSE;

    if (opts->flags & BTRFS_BALANCE_OPTS_PROFILES) {
        UINT64 type = get_chunk_dup_type(c);

        if (!(type & opts->profiles))
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_DEVID) {
        UINT16 i;
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
        BOOL b = FALSE;

        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            if (cis[i].dev_id == opts->devid) {
                b = TRUE;
                break;
            }
        }

        if (!b)
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_DRANGE) {
        UINT16 i, factor;
        UINT64 physsize;
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
        BOOL b = FALSE;

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

        physsize = c->chunk_item->size / factor;

        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            if (cis[i].offset < opts->drange_end && cis[i].offset + physsize >= opts->drange_start &&
                (!(opts->flags & BTRFS_BALANCE_OPTS_DEVID) || cis[i].dev_id == opts->devid)) {
                b = TRUE;
                break;
            }
        }

        if (!b)
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_VRANGE) {
        if (c->offset + c->chunk_item->size <= opts->vrange_start || c->offset > opts->vrange_end)
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_STRIPES) {
        if (c->chunk_item->num_stripes < opts->stripes_start || c->chunk_item->num_stripes < opts->stripes_end)
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_USAGE) {
        UINT64 usage = c->used * 100 / c->chunk_item->size;

        // usage == 0 should mean completely empty, not just that usage rounds to 0%
        if (c->used > 0 && usage == 0)
            usage = 1;

        if (usage < opts->usage_start || usage > opts->usage_end)
            return FALSE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_CONVERT && opts->flags & BTRFS_BALANCE_OPTS_SOFT) {
        UINT64 type = get_chunk_dup_type(c);

        if (type == opts->convert)
            return FALSE;
    }

    return TRUE;
}

static void copy_balance_args(btrfs_balance_opts* opts, BALANCE_ARGS* args) {
    if (opts->flags & BTRFS_BALANCE_OPTS_PROFILES) {
        args->profiles = opts->profiles;
        args->flags |= BALANCE_ARGS_FLAGS_PROFILES;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_USAGE) {
        if (args->usage_start == 0) {
            args->flags |= BALANCE_ARGS_FLAGS_USAGE_RANGE;
            args->usage_start = opts->usage_start;
            args->usage_end = opts->usage_end;
        } else {
            args->flags |= BALANCE_ARGS_FLAGS_USAGE;
            args->usage = opts->usage_end;
        }
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_DEVID) {
        args->devid = opts->devid;
        args->flags |= BALANCE_ARGS_FLAGS_DEVID;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_DRANGE) {
        args->drange_start = opts->drange_start;
        args->drange_end = opts->drange_end;
        args->flags |= BALANCE_ARGS_FLAGS_DRANGE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_VRANGE) {
        args->vrange_start = opts->vrange_start;
        args->vrange_end = opts->vrange_end;
        args->flags |= BALANCE_ARGS_FLAGS_VRANGE;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_CONVERT) {
        args->convert = opts->convert;
        args->flags |= BALANCE_ARGS_FLAGS_CONVERT;

        if (opts->flags & BTRFS_BALANCE_OPTS_SOFT)
            args->flags |= BALANCE_ARGS_FLAGS_SOFT;
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_LIMIT) {
        if (args->limit_start == 0) {
            args->flags |= BALANCE_ARGS_FLAGS_LIMIT_RANGE;
            args->limit_start = (UINT32)opts->limit_start;
            args->limit_end = (UINT32)opts->limit_end;
        } else {
            args->flags |= BALANCE_ARGS_FLAGS_LIMIT;
            args->limit = opts->limit_end;
        }
    }

    if (opts->flags & BTRFS_BALANCE_OPTS_STRIPES) {
        args->stripes_start = opts->stripes_start;
        args->stripes_end = opts->stripes_end;
        args->flags |= BALANCE_ARGS_FLAGS_STRIPES_RANGE;
    }
}

static NTSTATUS add_balance_item(device_extension* Vcb) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BALANCE_ITEM* bi;

    searchkey.obj_id = BALANCE_ITEM_ID;
    searchkey.obj_type = TYPE_TEMP_ITEM;
    searchkey.offset = 0;

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }

    if (!keycmp(tp.item->key, searchkey)) {
        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            goto end;
        }
    }

    bi = ExAllocatePoolWithTag(PagedPool, sizeof(BALANCE_ITEM), ALLOC_TAG);
    if (!bi) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(bi, sizeof(BALANCE_ITEM));

    if (Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED) {
        bi->flags |= BALANCE_FLAGS_DATA;
        copy_balance_args(&Vcb->balance.opts[BALANCE_OPTS_DATA], &bi->data);
    }

    if (Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED) {
        bi->flags |= BALANCE_FLAGS_METADATA;
        copy_balance_args(&Vcb->balance.opts[BALANCE_OPTS_METADATA], &bi->metadata);
    }

    if (Vcb->balance.opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_ENABLED) {
        bi->flags |= BALANCE_FLAGS_SYSTEM;
        copy_balance_args(&Vcb->balance.opts[BALANCE_OPTS_SYSTEM], &bi->system);
    }

    Status = insert_tree_item(Vcb, Vcb->root_root, BALANCE_ITEM_ID, TYPE_TEMP_ITEM, 0, bi, sizeof(BALANCE_ITEM), NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(bi);
        goto end;
    }

    Status = STATUS_SUCCESS;

end:
    if (NT_SUCCESS(Status)) {
        Status = do_write(Vcb, NULL);
        if (!NT_SUCCESS(Status))
            ERR("do_write returned %08x\n", Status);
    }

    free_trees(Vcb);

    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS remove_balance_item(device_extension* Vcb) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    searchkey.obj_id = BALANCE_ITEM_ID;
    searchkey.obj_type = TYPE_TEMP_ITEM;
    searchkey.offset = 0;

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }

    if (!keycmp(tp.item->key, searchkey)) {
        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            goto end;
        }

        Status = do_write(Vcb, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write returned %08x\n", Status);
            goto end;
        }

        free_trees(Vcb);
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static void load_balance_args(btrfs_balance_opts* opts, BALANCE_ARGS* args) {
    opts->flags = BTRFS_BALANCE_OPTS_ENABLED;

    if (args->flags & BALANCE_ARGS_FLAGS_PROFILES) {
        opts->flags |= BTRFS_BALANCE_OPTS_PROFILES;
        opts->profiles = args->profiles;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_USAGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_USAGE;

        opts->usage_start = 0;
        opts->usage_end = (UINT8)args->usage;
    } else if (args->flags & BALANCE_ARGS_FLAGS_USAGE_RANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_USAGE;

        opts->usage_start = (UINT8)args->usage_start;
        opts->usage_end = (UINT8)args->usage_end;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_DEVID) {
        opts->flags |= BTRFS_BALANCE_OPTS_DEVID;
        opts->devid = args->devid;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_DRANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_DRANGE;
        opts->drange_start = args->drange_start;
        opts->drange_end = args->drange_end;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_VRANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_VRANGE;
        opts->vrange_start = args->vrange_start;
        opts->vrange_end = args->vrange_end;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_LIMIT) {
        opts->flags |= BTRFS_BALANCE_OPTS_LIMIT;

        opts->limit_start = 0;
        opts->limit_end = args->limit;
    } else if (args->flags & BALANCE_ARGS_FLAGS_LIMIT_RANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_LIMIT;

        opts->limit_start = args->limit_start;
        opts->limit_end = args->limit_end;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_STRIPES_RANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_STRIPES;

        opts->stripes_start = (UINT16)args->stripes_start;
        opts->stripes_end = (UINT16)args->stripes_end;
    }

    if (args->flags & BALANCE_ARGS_FLAGS_CONVERT) {
        opts->flags |= BTRFS_BALANCE_OPTS_CONVERT;
        opts->convert = args->convert;

        if (args->flags & BALANCE_ARGS_FLAGS_SOFT)
            opts->flags |= BTRFS_BALANCE_OPTS_SOFT;
    }
}

static NTSTATUS remove_superblocks(device* dev) {
    NTSTATUS Status;
    superblock* sb;
    int i = 0;

    sb = ExAllocatePoolWithTag(PagedPool, sizeof(superblock), ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(sb, sizeof(superblock));

    while (superblock_addrs[i] > 0 && dev->devitem.num_bytes >= superblock_addrs[i] + sizeof(superblock)) {
        Status = write_data_phys(dev->devobj, superblock_addrs[i], sb, sizeof(superblock));

        if (!NT_SUCCESS(Status)) {
            ExFreePool(sb);
            return Status;
        }

        i++;
    }

    ExFreePool(sb);

    return STATUS_SUCCESS;
}

static NTSTATUS finish_removing_device(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, device* dev) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    LIST_ENTRY* le;
    volume_device_extension* vde;

    if (Vcb->need_write) {
        Status = do_write(Vcb, NULL);

        if (!NT_SUCCESS(Status))
            ERR("do_write returned %08x\n", Status);
    } else
        Status = STATUS_SUCCESS;

    free_trees(Vcb);

    if (!NT_SUCCESS(Status))
        return Status;

    // remove entry in chunk tree

    searchkey.obj_id = 1;
    searchkey.obj_type = TYPE_DEV_ITEM;
    searchkey.offset = dev->devitem.dev_id;

    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        Status = delete_tree_item(Vcb, &tp);

        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            return Status;
        }
    }

    // remove stats entry in device tree

    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = dev->devitem.dev_id;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        Status = delete_tree_item(Vcb, &tp);

        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            return Status;
        }
    }

    // update superblock

    Vcb->superblock.num_devices--;
    Vcb->superblock.total_bytes -= dev->devitem.num_bytes;
    Vcb->devices_loaded--;

    RemoveEntryList(&dev->list_entry);

    // flush

    Status = do_write(Vcb, NULL);
    if (!NT_SUCCESS(Status))
        ERR("do_write returned %08x\n", Status);

    free_trees(Vcb);

    if (!NT_SUCCESS(Status))
        return Status;

    if (!dev->readonly && dev->devobj) {
        Status = remove_superblocks(dev);
        if (!NT_SUCCESS(Status))
            WARN("remove_superblocks returned %08x\n", Status);
    }

    // remove entry in volume list

    vde = Vcb->vde;

    if (dev->devobj) {
        pdo_device_extension* pdode = vde->pdode;

        ExAcquireResourceExclusiveLite(&pdode->child_lock, TRUE);

        le = pdode->children.Flink;
        while (le != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

            if (RtlCompareMemory(&dev->devitem.device_uuid, &vc->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                PFILE_OBJECT FileObject;
                PDEVICE_OBJECT mountmgr;
                UNICODE_STRING mmdevpath;

                pdode->children_loaded--;

                if (vc->had_drive_letter) { // re-add entry to mountmgr
                    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
                    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
                    if (!NT_SUCCESS(Status))
                        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
                    else {
                        MOUNTDEV_NAME mdn;

                        Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), TRUE, NULL);
                        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
                            ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                        else {
                            MOUNTDEV_NAME* mdn2;
                            ULONG mdnsize = (ULONG)offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

                            mdn2 = ExAllocatePoolWithTag(PagedPool, mdnsize, ALLOC_TAG);
                            if (!mdn2)
                                ERR("out of memory\n");
                            else {
                                Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, mdnsize, TRUE, NULL);
                                if (!NT_SUCCESS(Status))
                                    ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                                else {
                                    UNICODE_STRING name;

                                    name.Buffer = mdn2->Name;
                                    name.Length = name.MaximumLength = mdn2->NameLength;

                                    Status = mountmgr_add_drive_letter(mountmgr, &name);
                                    if (!NT_SUCCESS(Status))
                                        WARN("mountmgr_add_drive_letter returned %08x\n", Status);
                                }

                                ExFreePool(mdn2);
                            }
                        }

                        ObDereferenceObject(FileObject);
                    }
                }

                ExFreePool(vc->pnp_name.Buffer);
                RemoveEntryList(&vc->list_entry);
                ExFreePool(vc);

                ObDereferenceObject(vc->fileobj);

                break;
            }

            le = le->Flink;
        }

        if (pdode->children_loaded > 0 && vde->device->Characteristics & FILE_REMOVABLE_MEDIA) {
            vde->device->Characteristics &= ~FILE_REMOVABLE_MEDIA;

            le = pdode->children.Flink;
            while (le != &pdode->children) {
                volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

                if (vc->devobj->Characteristics & FILE_REMOVABLE_MEDIA) {
                    vde->device->Characteristics |= FILE_REMOVABLE_MEDIA;
                    break;
                }

                le = le->Flink;
            }
        }

        pdode->num_children = Vcb->superblock.num_devices;

        ExReleaseResourceLite(&pdode->child_lock);

        // free dev

        if (dev->trim && !dev->readonly && !Vcb->options.no_trim)
            trim_whole_device(dev);
    }

    while (!IsListEmpty(&dev->space)) {
        LIST_ENTRY* le2 = RemoveHeadList(&dev->space);
        space* s = CONTAINING_RECORD(le2, space, list_entry);

        ExFreePool(s);
    }

    ExFreePool(dev);

    if (Vcb->trim) {
        Vcb->trim = FALSE;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev2 = CONTAINING_RECORD(le, device, list_entry);

            if (dev2->trim) {
                Vcb->trim = TRUE;
                break;
            }

            le = le->Flink;
        }
    }

    FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_CHANGE_SIZE);

    return STATUS_SUCCESS;
}

static void trim_unalloc_space(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, device* dev) {
    DEVICE_MANAGE_DATA_SET_ATTRIBUTES* dmdsa;
    DEVICE_DATA_SET_RANGE* ranges;
    ULONG datalen, i;
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BOOL b;
    UINT64 lastoff = 0x100000; // don't TRIM the first megabyte, in case someone has been daft enough to install GRUB there
    LIST_ENTRY* le;

    dev->num_trim_entries = 0;

    searchkey.obj_id = dev->devitem.dev_id;
    searchkey.obj_type = TYPE_DEV_EXTENT;
    searchkey.offset = 0;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return;
    }

    do {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id == dev->devitem.dev_id && tp.item->key.obj_type == TYPE_DEV_EXTENT) {
            if (tp.item->size >= sizeof(DEV_EXTENT)) {
                DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;

                if (tp.item->key.offset > lastoff)
                    add_trim_entry_avoid_sb(Vcb, dev, lastoff, tp.item->key.offset - lastoff);

                lastoff = tp.item->key.offset + de->length;
            } else {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DEV_EXTENT));
                return;
            }
        }

        b = find_next_item(Vcb, &tp, &next_tp, FALSE, NULL);

        if (b) {
            tp = next_tp;
            if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
                break;
        }
    } while (b);

    if (lastoff < dev->devitem.num_bytes)
        add_trim_entry_avoid_sb(Vcb, dev, lastoff, dev->devitem.num_bytes - lastoff);

    if (dev->num_trim_entries == 0)
        return;

    datalen = (ULONG)sector_align(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), sizeof(UINT64)) + (dev->num_trim_entries * sizeof(DEVICE_DATA_SET_RANGE));

    dmdsa = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
    if (!dmdsa) {
        ERR("out of memory\n");
        goto end;
    }

    dmdsa->Size = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES);
    dmdsa->Action = DeviceDsmAction_Trim;
    dmdsa->Flags = DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED;
    dmdsa->ParameterBlockOffset = 0;
    dmdsa->ParameterBlockLength = 0;
    dmdsa->DataSetRangesOffset = (ULONG)sector_align(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), sizeof(UINT64));
    dmdsa->DataSetRangesLength = dev->num_trim_entries * sizeof(DEVICE_DATA_SET_RANGE);

    ranges = (DEVICE_DATA_SET_RANGE*)((UINT8*)dmdsa + dmdsa->DataSetRangesOffset);

    i = 0;
    le = dev->trim_list.Flink;
    while (le != &dev->trim_list) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        ranges[i].StartingOffset = s->address;
        ranges[i].LengthInBytes = s->size;
        i++;

        le = le->Flink;
    }

    Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES, dmdsa, datalen, NULL, 0, TRUE, NULL);
    if (!NT_SUCCESS(Status))
        WARN("IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES returned %08x\n", Status);

    ExFreePool(dmdsa);

end:
    while (!IsListEmpty(&dev->trim_list)) {
        space* s = CONTAINING_RECORD(RemoveHeadList(&dev->trim_list), space, list_entry);
        ExFreePool(s);
    }

    dev->num_trim_entries = 0;
}

static NTSTATUS try_consolidation(device_extension* Vcb, UINT64 flags, chunk** newchunk) {
    NTSTATUS Status;
    BOOL changed;
    LIST_ENTRY* le;
    chunk* rc;

    // FIXME - allow with metadata chunks?

    while (TRUE) {
        rc = NULL;

        ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

        ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);

        // choose the least-used chunk we haven't looked at yet
        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

            // FIXME - skip full-size chunks over e.g. 90% full?
            if (c->chunk_item->type & BLOCK_FLAG_DATA && !c->readonly && c->balance_num != Vcb->balance.balance_num && (!rc || c->used < rc->used))
                rc = c;

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->chunk_lock);

        if (!rc) {
            ExReleaseResourceLite(&Vcb->tree_lock);
            break;
        }

        if (rc->list_entry_balance.Flink) {
            RemoveEntryList(&rc->list_entry_balance);
            Vcb->balance.chunks_left--;
        }

        rc->list_entry_balance.Flink = (LIST_ENTRY*)1; // so it doesn't get dropped
        rc->reloc = TRUE;

        ExReleaseResourceLite(&Vcb->tree_lock);

        do {
            changed = FALSE;

            Status = balance_data_chunk(Vcb, rc, &changed);
            if (!NT_SUCCESS(Status)) {
                ERR("balance_data_chunk returned %08x\n", Status);
                Vcb->balance.status = Status;
                rc->list_entry_balance.Flink = NULL;
                rc->reloc = FALSE;
                return Status;
            }

            KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);

            if (Vcb->readonly)
                Vcb->balance.stopping = TRUE;

            if (Vcb->balance.stopping)
                return STATUS_SUCCESS;
        } while (changed);

        rc->list_entry_balance.Flink = NULL;

        rc->changed = TRUE;
        rc->space_changed = TRUE;
        rc->balance_num = Vcb->balance.balance_num;

        Status = do_write(Vcb, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write returned %08x\n", Status);
            return Status;
        }

        free_trees(Vcb);
    }

    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

    Status = alloc_chunk(Vcb, flags, &rc, TRUE);

    ExReleaseResourceLite(&Vcb->chunk_lock);

    if (NT_SUCCESS(Status)) {
        *newchunk = rc;
        return Status;
    } else {
        ERR("alloc_chunk returned %08x\n", Status);
        return Status;
    }
}

static NTSTATUS regenerate_space_list(device_extension* Vcb, device* dev) {
    LIST_ENTRY* le;

    while (!IsListEmpty(&dev->space)) {
        space* s = CONTAINING_RECORD(RemoveHeadList(&dev->space), space, list_entry);

        ExFreePool(s);
    }

    // The Linux driver doesn't like to allocate chunks within the first megabyte of a device.

    space_list_add2(&dev->space, NULL, 0x100000, dev->devitem.num_bytes - 0x100000, NULL, NULL);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        UINT16 n;
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

        for (n = 0; n < c->chunk_item->num_stripes; n++) {
            UINT64 stripe_size = 0;

            if (cis[n].dev_id == dev->devitem.dev_id) {
                if (stripe_size == 0) {
                    UINT16 factor;

                    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
                        factor = c->chunk_item->num_stripes;
                    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
                        factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
                    else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
                        factor = c->chunk_item->num_stripes - 1;
                    else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
                        factor = c->chunk_item->num_stripes - 2;
                    else // SINGLE, DUP, RAID1
                        factor = 1;

                    stripe_size = c->chunk_item->size / factor;
                }

                space_list_subtract2(&dev->space, NULL, cis[n].offset, stripe_size, NULL, NULL);
            }
        }

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

_Function_class_(KSTART_ROUTINE)
#ifndef __REACTOS__
void balance_thread(void* context) {
#else
void NTAPI balance_thread(void* context) {
#endif
    device_extension* Vcb = (device_extension*)context;
    LIST_ENTRY chunks;
    LIST_ENTRY* le;
    UINT64 num_chunks[3], okay_metadata_chunks = 0, okay_data_chunks = 0, okay_system_chunks = 0;
    UINT64 old_data_flags = 0, old_metadata_flags = 0, old_system_flags = 0;
    NTSTATUS Status;

    Vcb->balance.balance_num++;

    Vcb->balance.stopping = FALSE;
    KeInitializeEvent(&Vcb->balance.finished, NotificationEvent, FALSE);

    if (Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_CONVERT) {
        old_data_flags = Vcb->data_flags;
        Vcb->data_flags = BLOCK_FLAG_DATA | (Vcb->balance.opts[BALANCE_OPTS_DATA].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_DATA].convert);

        FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_CHANGE_SIZE);
    }

    if (Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_CONVERT) {
        old_metadata_flags = Vcb->metadata_flags;
        Vcb->metadata_flags = BLOCK_FLAG_METADATA | (Vcb->balance.opts[BALANCE_OPTS_METADATA].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_METADATA].convert);
    }

    if (Vcb->balance.opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_CONVERT) {
        old_system_flags = Vcb->system_flags;
        Vcb->system_flags = BLOCK_FLAG_SYSTEM | (Vcb->balance.opts[BALANCE_OPTS_SYSTEM].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_SYSTEM].convert);
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS) {
        if (Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED)
            RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_METADATA], &Vcb->balance.opts[BALANCE_OPTS_DATA], sizeof(btrfs_balance_opts));
        else if (Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED)
            RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_DATA], &Vcb->balance.opts[BALANCE_OPTS_METADATA], sizeof(btrfs_balance_opts));
    }

    num_chunks[0] = num_chunks[1] = num_chunks[2] = 0;
    Vcb->balance.total_chunks = Vcb->balance.chunks_left = 0;

    InitializeListHead(&chunks);

    // FIXME - what are we supposed to do with limit_start?

    if (!Vcb->readonly) {
        if (!Vcb->balance.removing && !Vcb->balance.shrinking) {
            Status = add_balance_item(Vcb);
            if (!NT_SUCCESS(Status)) {
                ERR("add_balance_item returned %08x\n", Status);
                Vcb->balance.status = Status;
                goto end;
            }
        } else {
            if (Vcb->need_write) {
                Status = do_write(Vcb, NULL);

                free_trees(Vcb);

                if (!NT_SUCCESS(Status)) {
                    ERR("do_write returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                }
            }
        }
    }

    KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);

    if (Vcb->balance.stopping)
        goto end;

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
        UINT8 sort;

        acquire_chunk_lock(c, Vcb);

        if (c->chunk_item->type & BLOCK_FLAG_DATA)
            sort = BALANCE_OPTS_DATA;
        else if (c->chunk_item->type & BLOCK_FLAG_METADATA)
            sort = BALANCE_OPTS_METADATA;
        else if (c->chunk_item->type & BLOCK_FLAG_SYSTEM)
            sort = BALANCE_OPTS_SYSTEM;
        else {
            ERR("unexpected chunk type %llx\n", c->chunk_item->type);
            release_chunk_lock(c, Vcb);
            break;
        }

        if ((!(Vcb->balance.opts[sort].flags & BTRFS_BALANCE_OPTS_LIMIT) || num_chunks[sort] < Vcb->balance.opts[sort].limit_end) &&
            should_balance_chunk(Vcb, sort, c)) {
            InsertTailList(&chunks, &c->list_entry_balance);

            num_chunks[sort]++;
            Vcb->balance.total_chunks++;
            Vcb->balance.chunks_left++;
        } else if (sort == BALANCE_OPTS_METADATA)
            okay_metadata_chunks++;
        else if (sort == BALANCE_OPTS_DATA)
            okay_data_chunks++;
        else if (sort == BALANCE_OPTS_SYSTEM)
            okay_system_chunks++;

        if (!c->cache_loaded) {
            Status = load_cache_chunk(Vcb, c, NULL);

            if (!NT_SUCCESS(Status)) {
                ERR("load_cache_chunk returned %08x\n", Status);
                Vcb->balance.status = Status;
                release_chunk_lock(c, Vcb);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                goto end;
            }
        }

        release_chunk_lock(c, Vcb);

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    // If we're doing a full balance, try and allocate a new chunk now, before we mess things up
    if (okay_metadata_chunks == 0 || okay_data_chunks == 0 || okay_system_chunks == 0) {
        BOOL consolidated = FALSE;
        chunk* c;

        if (okay_metadata_chunks == 0) {
            ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

            Status = alloc_chunk(Vcb, Vcb->metadata_flags, &c, TRUE);
            if (NT_SUCCESS(Status))
                c->balance_num = Vcb->balance.balance_num;
            else if (Status != STATUS_DISK_FULL || consolidated) {
                ERR("alloc_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                Vcb->balance.status = Status;
                goto end;
            }

            ExReleaseResourceLite(&Vcb->chunk_lock);

            if (Status == STATUS_DISK_FULL) {
                Status = try_consolidation(Vcb, Vcb->metadata_flags, &c);
                if (!NT_SUCCESS(Status)) {
                    ERR("try_consolidation returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                } else
                    c->balance_num = Vcb->balance.balance_num;

                consolidated = TRUE;

                if (Vcb->balance.stopping)
                    goto end;
            }
        }

        if (okay_data_chunks == 0) {
            ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

            Status = alloc_chunk(Vcb, Vcb->data_flags, &c, TRUE);
            if (NT_SUCCESS(Status))
                c->balance_num = Vcb->balance.balance_num;
            else if (Status != STATUS_DISK_FULL || consolidated) {
                ERR("alloc_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                Vcb->balance.status = Status;
                goto end;
            }

            ExReleaseResourceLite(&Vcb->chunk_lock);

            if (Status == STATUS_DISK_FULL) {
                Status = try_consolidation(Vcb, Vcb->data_flags, &c);
                if (!NT_SUCCESS(Status)) {
                    ERR("try_consolidation returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                } else
                    c->balance_num = Vcb->balance.balance_num;

                consolidated = TRUE;

                if (Vcb->balance.stopping)
                    goto end;
            }
        }

        if (okay_system_chunks == 0) {
            ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

            Status = alloc_chunk(Vcb, Vcb->system_flags, &c, TRUE);
            if (NT_SUCCESS(Status))
                c->balance_num = Vcb->balance.balance_num;
            else if (Status != STATUS_DISK_FULL || consolidated) {
                ERR("alloc_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                Vcb->balance.status = Status;
                goto end;
            }

            ExReleaseResourceLite(&Vcb->chunk_lock);

            if (Status == STATUS_DISK_FULL) {
                Status = try_consolidation(Vcb, Vcb->system_flags, &c);
                if (!NT_SUCCESS(Status)) {
                    ERR("try_consolidation returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                } else
                    c->balance_num = Vcb->balance.balance_num;

                consolidated = TRUE;

                if (Vcb->balance.stopping)
                    goto end;
            }
        }
    }

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);

    le = chunks.Flink;
    while (le != &chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_balance);

        c->reloc = TRUE;

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    // do data chunks before metadata
    le = chunks.Flink;
    while (le != &chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_balance);
        LIST_ENTRY* le2 = le->Flink;

        if (c->chunk_item->type & BLOCK_FLAG_DATA) {
            BOOL changed;

            do {
                changed = FALSE;

                Status = balance_data_chunk(Vcb, c, &changed);
                if (!NT_SUCCESS(Status)) {
                    ERR("balance_data_chunk returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                }

                KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);

                if (Vcb->readonly)
                    Vcb->balance.stopping = TRUE;

                if (Vcb->balance.stopping)
                    break;
            } while (changed);

            c->changed = TRUE;
            c->space_changed = TRUE;
        }

        if (Vcb->balance.stopping)
            goto end;

        if (c->chunk_item->type & BLOCK_FLAG_DATA &&
            (!(Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED) || !(c->chunk_item->type & BLOCK_FLAG_METADATA))) {
            RemoveEntryList(&c->list_entry_balance);
            c->list_entry_balance.Flink = NULL;

            Vcb->balance.chunks_left--;
        }

        le = le2;
    }

    // do metadata chunks
    while (!IsListEmpty(&chunks)) {
        chunk* c;
        BOOL changed;

        le = RemoveHeadList(&chunks);
        c = CONTAINING_RECORD(le, chunk, list_entry_balance);

        if (c->chunk_item->type & BLOCK_FLAG_METADATA || c->chunk_item->type & BLOCK_FLAG_SYSTEM) {
            do {
                Status = balance_metadata_chunk(Vcb, c, &changed);
                if (!NT_SUCCESS(Status)) {
                    ERR("balance_metadata_chunk returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                }

                KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);

                if (Vcb->readonly)
                    Vcb->balance.stopping = TRUE;

                if (Vcb->balance.stopping)
                    break;
            } while (changed);

            c->changed = TRUE;
            c->space_changed = TRUE;
        }

        if (Vcb->balance.stopping)
            break;

        c->list_entry_balance.Flink = NULL;

        Vcb->balance.chunks_left--;
    }

end:
    if (!Vcb->readonly) {
        if (Vcb->balance.stopping || !NT_SUCCESS(Vcb->balance.status)) {
            le = chunks.Flink;
            while (le != &chunks) {
                chunk* c = CONTAINING_RECORD(le, chunk, list_entry_balance);
                c->reloc = FALSE;

                le = le->Flink;
                c->list_entry_balance.Flink = NULL;
            }

            if (old_data_flags != 0)
                Vcb->data_flags = old_data_flags;

            if (old_metadata_flags != 0)
                Vcb->metadata_flags = old_metadata_flags;

            if (old_system_flags != 0)
                Vcb->system_flags = old_system_flags;
        }

        if (Vcb->balance.removing) {
            device* dev = NULL;

            ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev2 = CONTAINING_RECORD(le, device, list_entry);

                if (dev2->devitem.dev_id == Vcb->balance.opts[0].devid) {
                    dev = dev2;
                    break;
                }

                le = le->Flink;
            }

            if (dev) {
                if (Vcb->balance.chunks_left == 0) {
                    Status = finish_removing_device(Vcb, dev);

                    if (!NT_SUCCESS(Status)) {
                        ERR("finish_removing_device returned %08x\n", Status);
                        dev->reloc = FALSE;
                    }
                } else
                    dev->reloc = FALSE;
            }

            ExReleaseResourceLite(&Vcb->tree_lock);
        } else if (Vcb->balance.shrinking) {
            device* dev = NULL;

            ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev2 = CONTAINING_RECORD(le, device, list_entry);

                if (dev2->devitem.dev_id == Vcb->balance.opts[0].devid) {
                    dev = dev2;
                    break;
                }

                le = le->Flink;
            }

            if (!dev) {
                ERR("could not find device %llx\n", Vcb->balance.opts[0].devid);
                Vcb->balance.status = STATUS_INTERNAL_ERROR;
            }

            if (Vcb->balance.stopping || !NT_SUCCESS(Vcb->balance.status)) {
                if (dev) {
                    Status = regenerate_space_list(Vcb, dev);
                    if (!NT_SUCCESS(Status))
                        WARN("regenerate_space_list returned %08x\n", Status);
                }
            } else {
                UINT64 old_size;

                old_size = dev->devitem.num_bytes;
                dev->devitem.num_bytes = Vcb->balance.opts[0].drange_start;

                Status = update_dev_item(Vcb, dev, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("update_dev_item returned %08x\n", Status);
                    dev->devitem.num_bytes = old_size;
                    Vcb->balance.status = Status;

                    Status = regenerate_space_list(Vcb, dev);
                    if (!NT_SUCCESS(Status))
                        WARN("regenerate_space_list returned %08x\n", Status);
                } else {
                    Vcb->superblock.total_bytes -= old_size - dev->devitem.num_bytes;

                    Status = do_write(Vcb, NULL);
                    if (!NT_SUCCESS(Status))
                        ERR("do_write returned %08x\n", Status);

                    free_trees(Vcb);
                }
            }

            ExReleaseResourceLite(&Vcb->tree_lock);

            if (!Vcb->balance.stopping && NT_SUCCESS(Vcb->balance.status))
                FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_CHANGE_SIZE);
        } else {
            Status = remove_balance_item(Vcb);
            if (!NT_SUCCESS(Status)) {
                ERR("remove_balance_item returned %08x\n", Status);
                goto end;
            }
        }

        if (Vcb->trim && !Vcb->options.no_trim) {
            ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev2 = CONTAINING_RECORD(le, device, list_entry);

                if (dev2->devobj && !dev2->readonly && dev2->trim)
                    trim_unalloc_space(Vcb, dev2);

                le = le->Flink;
            }

            ExReleaseResourceLite(&Vcb->tree_lock);
        }
    }

    ZwClose(Vcb->balance.thread);
    Vcb->balance.thread = NULL;

    KeSetEvent(&Vcb->balance.finished, 0, FALSE);
}

NTSTATUS start_balance(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode) {
    NTSTATUS Status;
    btrfs_start_balance* bsb = (btrfs_start_balance*)data;
    UINT8 i;

    if (length < sizeof(btrfs_start_balance) || !data)
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (Vcb->locked) {
        WARN("cannot start balance while locked\n");
        return STATUS_DEVICE_NOT_READY;
    }

    if (Vcb->scrub.thread) {
        WARN("cannot start balance while scrub running\n");
        return STATUS_DEVICE_NOT_READY;
    }

    if (Vcb->balance.thread) {
        WARN("balance already running\n");
        return STATUS_DEVICE_NOT_READY;
    }

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (!(bsb->opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED) &&
        !(bsb->opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED) &&
        !(bsb->opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_ENABLED))
        return STATUS_SUCCESS;

    for (i = 0; i < 3; i++) {
        if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_ENABLED) {
            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_PROFILES) {
                bsb->opts[i].profiles &= BLOCK_FLAG_RAID0 | BLOCK_FLAG_RAID1 | BLOCK_FLAG_DUPLICATE | BLOCK_FLAG_RAID10 |
                                         BLOCK_FLAG_RAID5 | BLOCK_FLAG_RAID6 | BLOCK_FLAG_SINGLE;

                if (bsb->opts[i].profiles == 0)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_DEVID) {
                if (bsb->opts[i].devid == 0)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_DRANGE) {
                if (bsb->opts[i].drange_start > bsb->opts[i].drange_end)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_VRANGE) {
                if (bsb->opts[i].vrange_start > bsb->opts[i].vrange_end)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_LIMIT) {
                bsb->opts[i].limit_start = max(1, bsb->opts[i].limit_start);
                bsb->opts[i].limit_end = max(1, bsb->opts[i].limit_end);

                if (bsb->opts[i].limit_start > bsb->opts[i].limit_end)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_STRIPES) {
                bsb->opts[i].stripes_start = max(1, bsb->opts[i].stripes_start);
                bsb->opts[i].stripes_end = max(1, bsb->opts[i].stripes_end);

                if (bsb->opts[i].stripes_start > bsb->opts[i].stripes_end)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_USAGE) {
                bsb->opts[i].usage_start = min(100, bsb->opts[i].stripes_start);
                bsb->opts[i].usage_end = min(100, bsb->opts[i].stripes_end);

                if (bsb->opts[i].stripes_start > bsb->opts[i].stripes_end)
                    return STATUS_INVALID_PARAMETER;
            }

            if (bsb->opts[i].flags & BTRFS_BALANCE_OPTS_CONVERT) {
                if (bsb->opts[i].convert != BLOCK_FLAG_RAID0 && bsb->opts[i].convert != BLOCK_FLAG_RAID1 &&
                    bsb->opts[i].convert != BLOCK_FLAG_DUPLICATE && bsb->opts[i].convert != BLOCK_FLAG_RAID10 &&
                    bsb->opts[i].convert != BLOCK_FLAG_RAID5 && bsb->opts[i].convert != BLOCK_FLAG_RAID6 &&
                    bsb->opts[i].convert != BLOCK_FLAG_SINGLE)
                    return STATUS_INVALID_PARAMETER;
            }
        }
    }

    RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_DATA], &bsb->opts[BALANCE_OPTS_DATA], sizeof(btrfs_balance_opts));
    RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_METADATA], &bsb->opts[BALANCE_OPTS_METADATA], sizeof(btrfs_balance_opts));
    RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_SYSTEM], &bsb->opts[BALANCE_OPTS_SYSTEM], sizeof(btrfs_balance_opts));

    Vcb->balance.paused = FALSE;
    Vcb->balance.removing = FALSE;
    Vcb->balance.shrinking = FALSE;
    Vcb->balance.status = STATUS_SUCCESS;
    KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);

    Status = PsCreateSystemThread(&Vcb->balance.thread, 0, NULL, NULL, NULL, balance_thread, Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS look_for_balance_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BALANCE_ITEM* bi;
    int i;

    searchkey.obj_id = BALANCE_ITEM_ID;
    searchkey.obj_type = TYPE_TEMP_ITEM;
    searchkey.offset = 0;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        TRACE("no balance item found\n");
        return STATUS_NOT_FOUND;
    }

    if (tp.item->size < sizeof(BALANCE_ITEM)) {
        WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
             tp.item->size, sizeof(BALANCE_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    bi = (BALANCE_ITEM*)tp.item->data;

    if (bi->flags & BALANCE_FLAGS_DATA)
        load_balance_args(&Vcb->balance.opts[BALANCE_OPTS_DATA], &bi->data);

    if (bi->flags & BALANCE_FLAGS_METADATA)
        load_balance_args(&Vcb->balance.opts[BALANCE_OPTS_METADATA], &bi->metadata);

    if (bi->flags & BALANCE_FLAGS_SYSTEM)
        load_balance_args(&Vcb->balance.opts[BALANCE_OPTS_SYSTEM], &bi->system);

    // do the heuristics that Linux driver does

    for (i = 0; i < 3; i++) {
        if (Vcb->balance.opts[i].flags & BTRFS_BALANCE_OPTS_ENABLED) {
            // if converting, don't redo chunks already done

            if (Vcb->balance.opts[i].flags & BTRFS_BALANCE_OPTS_CONVERT)
                Vcb->balance.opts[i].flags |= BTRFS_BALANCE_OPTS_SOFT;

            // don't balance chunks more than 90% filled - presumably these
            // have already been done

            if (!(Vcb->balance.opts[i].flags & BTRFS_BALANCE_OPTS_USAGE) &&
                !(Vcb->balance.opts[i].flags & BTRFS_BALANCE_OPTS_CONVERT)
            ) {
                Vcb->balance.opts[i].flags |= BTRFS_BALANCE_OPTS_USAGE;
                Vcb->balance.opts[i].usage_start = 0;
                Vcb->balance.opts[i].usage_end = 90;
            }
        }
    }

    if (Vcb->readonly || Vcb->options.skip_balance)
        Vcb->balance.paused = TRUE;
    else
        Vcb->balance.paused = FALSE;

    Vcb->balance.removing = FALSE;
    Vcb->balance.shrinking = FALSE;
    Vcb->balance.status = STATUS_SUCCESS;
    KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);

    Status = PsCreateSystemThread(&Vcb->balance.thread, 0, NULL, NULL, NULL, balance_thread, Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS query_balance(device_extension* Vcb, void* data, ULONG length) {
    btrfs_query_balance* bqb = (btrfs_query_balance*)data;

    if (length < sizeof(btrfs_query_balance) || !data)
        return STATUS_INVALID_PARAMETER;

    if (!Vcb->balance.thread) {
        bqb->status = BTRFS_BALANCE_STOPPED;

        if (!NT_SUCCESS(Vcb->balance.status)) {
            bqb->status |= BTRFS_BALANCE_ERROR;
            bqb->error = Vcb->balance.status;
        }

        return STATUS_SUCCESS;
    }

    bqb->status = Vcb->balance.paused ? BTRFS_BALANCE_PAUSED : BTRFS_BALANCE_RUNNING;

    if (Vcb->balance.removing)
        bqb->status |= BTRFS_BALANCE_REMOVAL;

    if (Vcb->balance.shrinking)
        bqb->status |= BTRFS_BALANCE_SHRINKING;

    if (!NT_SUCCESS(Vcb->balance.status))
        bqb->status |= BTRFS_BALANCE_ERROR;

    bqb->chunks_left = Vcb->balance.chunks_left;
    bqb->total_chunks = Vcb->balance.total_chunks;
    bqb->error = Vcb->balance.status;
    RtlCopyMemory(&bqb->data_opts, &Vcb->balance.opts[BALANCE_OPTS_DATA], sizeof(btrfs_balance_opts));
    RtlCopyMemory(&bqb->metadata_opts, &Vcb->balance.opts[BALANCE_OPTS_METADATA], sizeof(btrfs_balance_opts));
    RtlCopyMemory(&bqb->system_opts, &Vcb->balance.opts[BALANCE_OPTS_SYSTEM], sizeof(btrfs_balance_opts));

    return STATUS_SUCCESS;
}

NTSTATUS pause_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode) {
    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!Vcb->balance.thread)
        return STATUS_DEVICE_NOT_READY;

    if (Vcb->balance.paused)
        return STATUS_DEVICE_NOT_READY;

    Vcb->balance.paused = TRUE;
    KeClearEvent(&Vcb->balance.event);

    return STATUS_SUCCESS;
}

NTSTATUS resume_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode) {
    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!Vcb->balance.thread)
        return STATUS_DEVICE_NOT_READY;

    if (!Vcb->balance.paused)
        return STATUS_DEVICE_NOT_READY;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    Vcb->balance.paused = FALSE;
    KeSetEvent(&Vcb->balance.event, 0, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS stop_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode) {
    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!Vcb->balance.thread)
        return STATUS_DEVICE_NOT_READY;

    Vcb->balance.paused = FALSE;
    Vcb->balance.stopping = TRUE;
    Vcb->balance.status = STATUS_SUCCESS;
    KeSetEvent(&Vcb->balance.event, 0, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS remove_device(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode) {
    UINT64 devid;
    LIST_ENTRY* le;
    device* dev = NULL;
    NTSTATUS Status;
    int i;
    UINT64 num_rw_devices;

    TRACE("(%p, %p, %x)\n", Vcb, data, length);

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (length < sizeof(UINT64))
        return STATUS_INVALID_PARAMETER;

    devid = *(UINT64*)data;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

    if (Vcb->readonly) {
        ExReleaseResourceLite(&Vcb->tree_lock);
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    num_rw_devices = 0;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);

        if (dev2->devitem.dev_id == devid)
            dev = dev2;

        if (!dev2->readonly)
            num_rw_devices++;

        le = le->Flink;
    }

    if (!dev) {
        ExReleaseResourceLite(&Vcb->tree_lock);
        WARN("device %llx not found\n", devid);
        return STATUS_NOT_FOUND;
    }

    if (!dev->readonly) {
        if (num_rw_devices == 1) {
            ExReleaseResourceLite(&Vcb->tree_lock);
            WARN("not removing last non-readonly device\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (num_rw_devices == 4 &&
            ((Vcb->data_flags & BLOCK_FLAG_RAID10 || Vcb->metadata_flags & BLOCK_FLAG_RAID10 || Vcb->system_flags & BLOCK_FLAG_RAID10) ||
             (Vcb->data_flags & BLOCK_FLAG_RAID6 || Vcb->metadata_flags & BLOCK_FLAG_RAID6 || Vcb->system_flags & BLOCK_FLAG_RAID6))
        ) {
            ExReleaseResourceLite(&Vcb->tree_lock);
            ERR("would not be enough devices to satisfy RAID requirement (RAID6/10)\n");
            return STATUS_CANNOT_DELETE;
        }

        if (num_rw_devices == 3 && (Vcb->data_flags & BLOCK_FLAG_RAID5 || Vcb->metadata_flags & BLOCK_FLAG_RAID5 || Vcb->system_flags & BLOCK_FLAG_RAID5)) {
            ExReleaseResourceLite(&Vcb->tree_lock);
            ERR("would not be enough devices to satisfy RAID requirement (RAID5)\n");
            return STATUS_CANNOT_DELETE;
        }

        if (num_rw_devices == 2 &&
            ((Vcb->data_flags & BLOCK_FLAG_RAID0 || Vcb->metadata_flags & BLOCK_FLAG_RAID0 || Vcb->system_flags & BLOCK_FLAG_RAID0) ||
             (Vcb->data_flags & BLOCK_FLAG_RAID1 || Vcb->metadata_flags & BLOCK_FLAG_RAID1 || Vcb->system_flags & BLOCK_FLAG_RAID1))
        ) {
            ExReleaseResourceLite(&Vcb->tree_lock);
            ERR("would not be enough devices to satisfy RAID requirement (RAID0/1)\n");
            return STATUS_CANNOT_DELETE;
        }
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (Vcb->balance.thread) {
        WARN("balance already running\n");
        return STATUS_DEVICE_NOT_READY;
    }

    dev->reloc = TRUE;

    RtlZeroMemory(Vcb->balance.opts, sizeof(btrfs_balance_opts) * 3);

    for (i = 0; i < 3; i++) {
        Vcb->balance.opts[i].flags = BTRFS_BALANCE_OPTS_ENABLED | BTRFS_BALANCE_OPTS_DEVID;
        Vcb->balance.opts[i].devid = devid;
    }

    Vcb->balance.paused = FALSE;
    Vcb->balance.removing = TRUE;
    Vcb->balance.shrinking = FALSE;
    Vcb->balance.status = STATUS_SUCCESS;
    KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);

    Status = PsCreateSystemThread(&Vcb->balance.thread, 0, NULL, NULL, NULL, balance_thread, Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        dev->reloc = FALSE;
        return Status;
    }

    return STATUS_SUCCESS;
}
