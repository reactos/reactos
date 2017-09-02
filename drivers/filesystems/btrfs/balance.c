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
#include "btrfsioctl.h"

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
    
    union {
        EXTENT_DATA_REF edr;
        SHARED_DATA_REF sdr;
    };
    
    metadata_reloc* parent;
    LIST_ENTRY list_entry;
} data_reloc_ref;

extern LIST_ENTRY volumes;
extern ERESOURCE volumes_lock;

static NTSTATUS add_metadata_reloc(device_extension* Vcb, LIST_ENTRY* items, traverse_ptr* tp, BOOL skinny, metadata_reloc** mr2, chunk* c, LIST_ENTRY* rollback) {
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
    
    delete_tree_item(Vcb, tp, rollback);
    
    if (!c)
        c = get_chunk_from_address(Vcb, tp->item->key.obj_id);
        
    if (c) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        decrease_chunk_usage(c, Vcb->superblock.node_size);
        
        space_list_add(Vcb, c, TRUE, tp->item->key.obj_id, Vcb->superblock.node_size, rollback);
        
        ExReleaseResourceLite(&c->lock);
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
        ULONG sectlen = secttype == TYPE_TREE_BLOCK_REF ? sizeof(TREE_BLOCK_REF) : (secttype == TYPE_SHARED_BLOCK_REF ? sizeof(SHARED_BLOCK_REF) : 0);
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
                if (tp2.item->key.obj_type == TYPE_TREE_BLOCK_REF && tp2.item->size >= sizeof(TREE_BLOCK_REF)) {
                    metadata_reloc_ref* ref = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    ref->type = TYPE_TREE_BLOCK_REF;
                    RtlCopyMemory(&ref->tbr, tp2.item->data, sizeof(TREE_BLOCK_REF));
                    ref->parent = NULL;
                    ref->top = FALSE;
                    InsertTailList(&mr->refs, &ref->list_entry);
                    
                    delete_tree_item(Vcb, &tp2, rollback);
                } else if (tp2.item->key.obj_type == TYPE_SHARED_BLOCK_REF && tp2.item->size >= sizeof(SHARED_BLOCK_REF)) {
                    metadata_reloc_ref* ref = ExAllocatePoolWithTag(PagedPool, sizeof(metadata_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    ref->type = TYPE_SHARED_BLOCK_REF;
                    RtlCopyMemory(&ref->sbr, tp2.item->data, sizeof(SHARED_BLOCK_REF));
                    ref->parent = NULL;
                    ref->top = FALSE;
                    InsertTailList(&mr->refs, &ref->list_entry);
                    
                    delete_tree_item(Vcb, &tp2, rollback);
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

static NTSTATUS add_metadata_reloc_parent(device_extension* Vcb, LIST_ENTRY* items, UINT64 address, metadata_reloc** mr2, LIST_ENTRY* rollback) {
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

static NTSTATUS add_metadata_reloc_extent_item(device_extension* Vcb, metadata_reloc* mr, LIST_ENTRY* rollback) {
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
    
    le = mr->refs.Flink;
    while (le != &mr->refs) {
        metadata_reloc_ref* ref = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);
        ULONG extlen = 0;
        
        rc++;
        
        if (ref->type == TYPE_TREE_BLOCK_REF)
            extlen += sizeof(TREE_BLOCK_REF);
        else if (ref->type == TYPE_SHARED_BLOCK_REF)
            extlen += sizeof(SHARED_BLOCK_REF);

        if (all_inline) {
            if (inline_len + 1 + extlen > Vcb->superblock.node_size / 4) {
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
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        if (!insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_METADATA_ITEM, mr->data->level, ei, inline_len, NULL, NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        if (!insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, ei, inline_len, NULL, NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    if (!all_inline) {
        le = &first_noninline->list_entry;
        
        while (le != &mr->refs) {
            metadata_reloc_ref* ref = CONTAINING_RECORD(le, metadata_reloc_ref, list_entry);
            
            if (ref->type == TYPE_TREE_BLOCK_REF) {
                TREE_BLOCK_REF* tbr;
                
                tbr = ExAllocatePoolWithTag(PagedPool, sizeof(TREE_BLOCK_REF), ALLOC_TAG);
                if (!tbr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                tbr->offset = ref->tbr.offset;
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_TREE_BLOCK_REF, tbr->offset, tbr, sizeof(TREE_BLOCK_REF), NULL, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
                }
            } else if (ref->type == TYPE_SHARED_BLOCK_REF) {
                SHARED_BLOCK_REF* sbr;
                
                sbr = ExAllocatePoolWithTag(PagedPool, sizeof(SHARED_BLOCK_REF), ALLOC_TAG);
                if (!sbr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                sbr->offset = ref->parent->new_address;
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, mr->new_address, TYPE_SHARED_BLOCK_REF, sbr->offset, sbr, sizeof(SHARED_BLOCK_REF), NULL, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
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
                    NTSTATUS Status;
                    SHARED_BLOCK_REF sbr;
                    
                    sbr.offset = mr->new_address;
                    
                    Status = increase_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                      NULL, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }
        
                    sbr.offset = mr->address;
                    
                    Status = decrease_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                      sbr.offset, FALSE, NULL, rollback);
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
                            UINT64 sdrrc = find_extent_shared_data_refcount(Vcb, ed2->address, mr->address, NULL);
                            
                            if (sdrrc > 0) {
                                NTSTATUS Status;
                                SHARED_DATA_REF sdr;
                                chunk* c;
                                
                                sdr.offset = mr->new_address;
                                sdr.count = sdrrc;
                                
                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0,
                                                                  NULL, rollback);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("increase_extent_refcount returned %08x\n", Status);
                                    return Status;
                                }
                                
                                sdr.offset = mr->address;
                                
                                Status = decrease_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0,
                                                                  sdr.offset, FALSE, NULL, rollback);
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

static NTSTATUS write_metadata_items(device_extension* Vcb, LIST_ENTRY* items, LIST_ENTRY* data_items, chunk* c, LIST_ENTRY* rollback) {
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
        
//         ERR("address %llx\n", mr->address);
        
        mr->data = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!mr->data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        Status = read_data(Vcb, mr->address, Vcb->superblock.node_size, NULL, TRUE, (UINT8*)mr->data,
                           c && mr->address >= c->offset && mr->address < c->offset + c->chunk_item->size ? c : NULL, &pc, NULL, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data returned %08x\n", Status);
            return Status;
        }
        
        if (pc->chunk_item->type & BLOCK_FLAG_SYSTEM)
            mr->system = TRUE;
        
        if (data_items && mr->data->level == 0) {
            LIST_ENTRY* le2 = data_items->Flink;
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
                    ExAcquireResourceExclusiveLite(&newchunk->lock, TRUE);
                    
                    if (newchunk->chunk_item->type == flags && find_metadata_address_in_chunk(Vcb, newchunk, &mr->new_address)) {
                        increase_chunk_usage(newchunk, Vcb->superblock.node_size);
                        space_list_subtract(Vcb, newchunk, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                        done = TRUE;
                    }
                    
                    ExReleaseResourceLite(&newchunk->lock);
                }
                
                if (!done) {
                    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
                    le2 = Vcb->chunks.Flink;
                    while (le2 != &Vcb->chunks) {
                        chunk* c2 = CONTAINING_RECORD(le2, chunk, list_entry);
                        
                        if (!c2->readonly && !c2->reloc && c2 != newchunk && c2->chunk_item->type == flags) {
                            ExAcquireResourceExclusiveLite(&c2->lock, TRUE);
                            
                            if ((c2->chunk_item->size - c2->used) >= Vcb->superblock.node_size) {
                                if (find_metadata_address_in_chunk(Vcb, c2, &mr->new_address)) {
                                    increase_chunk_usage(c2, Vcb->superblock.node_size);
                                    space_list_subtract(Vcb, c2, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                                    ExReleaseResourceLite(&c2->lock);
                                    newchunk = c2;
                                    done = TRUE;
                                    break;
                                }
                            }
                            
                            ExReleaseResourceLite(&c2->lock);
                        }

                        le2 = le2->Flink;
                    }
                    
                    // allocate new chunk if necessary
                    if (!done) {
                        newchunk = alloc_chunk(Vcb, flags);
                        
                        if (!newchunk) {
                            ERR("could not allocate new chunk\n");
                            ExReleaseResourceLite(&Vcb->chunk_lock);
                            Status = STATUS_DISK_FULL;
                            goto end;
                        }
                        
                        ExAcquireResourceExclusiveLite(&newchunk->lock, TRUE);
                        
                        if (!find_metadata_address_in_chunk(Vcb, newchunk, &mr->new_address)) {
                            ExReleaseResourceLite(&newchunk->lock);
                            ERR("could not find address in new chunk\n");
                            Status = STATUS_DISK_FULL;
                            goto end;
                        } else {
                            increase_chunk_usage(newchunk, Vcb->superblock.node_size);
                            space_list_subtract(Vcb, newchunk, FALSE, mr->new_address, Vcb->superblock.node_size, rollback);
                        }
                        
                        ExReleaseResourceLite(&newchunk->lock);
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
                                
                                delete_tree_item(Vcb, &tp, rollback);
                                
                                if (!insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, NULL, rollback)) {
                                    ERR("insert_tree_item failed\n");
                                    Status = STATUS_INTERNAL_ERROR;
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
                tw->overlap = FALSE;
                
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
    
    le = items->Flink;
    while (le != items) {
        metadata_reloc* mr = CONTAINING_RECORD(le, metadata_reloc, list_entry);
        
        Status = add_metadata_reloc_extent_item(Vcb, mr, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_metadata_reloc_extent_item returned %08x\n", Status);
            goto end;
        }
        
        le = le->Flink;
    }
    
    Status = do_tree_writes(Vcb, &tree_writes, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("do_tree_writes returned %08x\n", Status);
        goto end;
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
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);
    
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

static NTSTATUS add_data_reloc(device_extension* Vcb, LIST_ENTRY* items, LIST_ENTRY* metadata_items, traverse_ptr* tp, chunk* c, LIST_ENTRY* rollback) {
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
    
    delete_tree_item(Vcb, tp, rollback);
    
    if (!c)
        c = get_chunk_from_address(Vcb, tp->item->key.obj_id);
        
    if (c) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        decrease_chunk_usage(c, tp->item->key.offset);
        
        space_list_add(Vcb, c, TRUE, tp->item->key.obj_id, tp->item->key.offset, rollback);
        
        ExReleaseResourceLite(&c->lock);
    }
    
    ei = (EXTENT_ITEM*)tp->item->data;
    inline_rc = 0;
    
    len = tp->item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)tp->item->data + sizeof(EXTENT_ITEM);
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = secttype == TYPE_EXTENT_DATA_REF ? sizeof(EXTENT_DATA_REF) : (secttype == TYPE_SHARED_DATA_REF ? sizeof(SHARED_DATA_REF) : 0);
        data_reloc_ref* ref;
        NTSTATUS Status;
        metadata_reloc* mr;
        
        len--;
        
        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }
        
        ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
        if (!ref) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (secttype == TYPE_EXTENT_DATA_REF) {
            LIST_ENTRY* le;
            KEY searchkey;
            traverse_ptr tp3;
            root* r = NULL;
            
            ref->type = TYPE_EXTENT_DATA_REF;
            RtlCopyMemory(&ref->edr, ptr + sizeof(UINT8), sizeof(EXTENT_DATA_REF));
            inline_rc += ref->edr.count;
            
            le = Vcb->roots.Flink;
            while (le != &Vcb->roots) {
                root* r2 = CONTAINING_RECORD(le, root, list_entry);
                
                if (r2->id == ref->edr.root) {
                    r = r2;
                    break;
                }
                
                le = le->Flink;
            }
            
            if (!r) {
                ERR("could not find subvol %llx\n", ref->edr.count);
                ExFreePool(ref);
                return STATUS_INTERNAL_ERROR;
            }
            
            searchkey.obj_id = ref->edr.objid;
            searchkey.obj_type = TYPE_EXTENT_DATA;
            searchkey.offset = ref->edr.offset;
            
            Status = find_item(Vcb, r, &tp3, &searchkey, FALSE, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("find_item returned %08x\n", Status);
                ExFreePool(ref);
                return Status;
            }
            
            if (keycmp(tp3.item->key, searchkey)) {
                ERR("could not find (%llx,%x,%llx) in root %llx\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset, r->id);
                ExFreePool(ref);
                return STATUS_INTERNAL_ERROR;
            }
            
            Status = add_metadata_reloc_parent(Vcb, metadata_items, tp3.tree->header.address, &mr, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_metadata_reloc_parent returned %08x\n", Status);
                ExFreePool(ref);
                return Status;
            }
            
            ref->parent = mr;
        } else if (secttype == TYPE_SHARED_DATA_REF) {
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
        } else {
            ERR("unexpected tree type %x\n", secttype);
            ExFreePool(ref);
            return STATUS_INTERNAL_ERROR;
        }
        
        InsertTailList(&dr->refs, &ref->list_entry);
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
    }
    
    if (inline_rc < ei->refcount) { // look for non-inline entries
        traverse_ptr tp2 = *tp, next_tp;
        
        while (find_next_item(Vcb, &tp2, &next_tp, FALSE, NULL)) {
            metadata_reloc* mr;
            NTSTATUS Status;
            
            tp2 = next_tp;
            
            if (tp2.item->key.obj_id == tp->item->key.obj_id) {
                if (tp2.item->key.obj_type == TYPE_EXTENT_DATA_REF && tp2.item->size >= sizeof(EXTENT_DATA_REF)) {
                    data_reloc_ref* ref;
                    LIST_ENTRY* le;
                    KEY searchkey;
                    traverse_ptr tp3;
                    root* r = NULL;

                    ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    ref->type = TYPE_EXTENT_DATA_REF;
                    RtlCopyMemory(&ref->edr, tp2.item->data, sizeof(EXTENT_DATA_REF));
                    
                    le = Vcb->roots.Flink;
                    while (le != &Vcb->roots) {
                        root* r2 = CONTAINING_RECORD(le, root, list_entry);
                        
                        if (r2->id == ref->edr.root) {
                            r = r2;
                            break;
                        }
                        
                        le = le->Flink;
                    }
                    
                    if (!r) {
                        ERR("could not find subvol %llx\n", ref->edr.count);
                        ExFreePool(ref);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    searchkey.obj_id = ref->edr.objid;
                    searchkey.obj_type = TYPE_EXTENT_DATA;
                    searchkey.offset = ref->edr.offset;
                    
                    Status = find_item(Vcb, r, &tp3, &searchkey, FALSE, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("find_item returned %08x\n", Status);
                        ExFreePool(ref);
                        return Status;
                    }
                    
                    if (!keycmp(tp3.item->key, searchkey)) {
                        ERR("could not find (%llx,%x,%llx) in root %llx\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset, r->id);
                        ExFreePool(ref);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    Status = add_metadata_reloc_parent(Vcb, metadata_items, tp3.tree->header.address, &mr, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_metadata_reloc_parent returned %08x\n", Status);
                        ExFreePool(ref);
                        return Status;
                    }
                    
                    ref->parent = mr;
                    InsertTailList(&dr->refs, &ref->list_entry);
                    
                    delete_tree_item(Vcb, &tp2, rollback);
                } else if (tp2.item->key.obj_type == TYPE_SHARED_DATA_REF && tp2.item->size >= sizeof(SHARED_DATA_REF)) {
                    data_reloc_ref* ref = ExAllocatePoolWithTag(PagedPool, sizeof(data_reloc_ref), ALLOC_TAG);
                    if (!ref) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    ref->type = TYPE_SHARED_DATA_REF;
                    RtlCopyMemory(&ref->sdr, tp2.item->data, sizeof(SHARED_DATA_REF));
                    
                    Status = add_metadata_reloc_parent(Vcb, metadata_items, ref->sdr.offset, &mr, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_metadata_reloc_parent returned %08x\n", Status);
                        ExFreePool(ref);
                        return Status;
                    }
                    
                    ref->parent = mr;
                    InsertTailList(&dr->refs, &ref->list_entry);
                    
                    delete_tree_item(Vcb, &tp2, rollback);
                }
            } else
                break;
        }
    }
    
    InsertTailList(items, &dr->list_entry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_data_reloc_extent_item(device_extension* Vcb, data_reloc* dr, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    UINT64 rc = 0;
    UINT16 inline_len;
    BOOL all_inline = TRUE;
    data_reloc_ref* first_noninline = NULL;
    EXTENT_ITEM* ei;
    UINT8* ptr;
    
    inline_len = sizeof(EXTENT_ITEM);
    
    le = dr->refs.Flink;
    while (le != &dr->refs) {
        data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);
        ULONG extlen = 0;
        
        rc++;
        
        if (ref->type == TYPE_EXTENT_DATA_REF)
            extlen += sizeof(EXTENT_DATA_REF);
        else if (ref->type == TYPE_SHARED_DATA_REF)
            extlen += sizeof(SHARED_DATA_REF);

        if (all_inline) {
            if (inline_len + 1 + extlen > Vcb->superblock.node_size / 4) {
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
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_EXTENT_ITEM, dr->size, ei, inline_len, NULL, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (!all_inline) {
        le = &first_noninline->list_entry;
        
        while (le != &dr->refs) {
            data_reloc_ref* ref = CONTAINING_RECORD(le, data_reloc_ref, list_entry);
            
            if (ref->type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* edr;
                UINT64 off;
                
                edr = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA_REF), ALLOC_TAG);
                if (!edr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(edr, &ref->edr, sizeof(EXTENT_DATA_REF));
                
                off = get_extent_data_ref_hash2(ref->edr.root, ref->edr.objid, ref->edr.offset);
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_EXTENT_DATA_REF, off, edr, sizeof(EXTENT_DATA_REF), NULL, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
                }
            } else if (ref->type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sdr;
                
                sdr = ExAllocatePoolWithTag(PagedPool, sizeof(SHARED_DATA_REF), ALLOC_TAG);
                if (!sdr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                sdr->offset = ref->parent->new_address;
                sdr->count = ref->sdr.count;
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, dr->new_address, TYPE_SHARED_DATA_REF, sdr->offset, sdr, sizeof(SHARED_DATA_REF), NULL, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
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
    
    data = ExAllocatePoolWithTag(PagedPool, 0x100000, ALLOC_TAG);
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
        UINT64 off;
        
        if (newchunk) {
            ExAcquireResourceExclusiveLite(&newchunk->lock, TRUE);
            
            if (find_data_address_in_chunk(Vcb, newchunk, dr->size, &dr->new_address)) {
                increase_chunk_usage(newchunk, dr->size);
                space_list_subtract(Vcb, newchunk, FALSE, dr->new_address, dr->size, &rollback);
                done = TRUE;
            }
            
            ExReleaseResourceLite(&newchunk->lock);
        }
        
        if (!done) {
            ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

            le2 = Vcb->chunks.Flink;
            while (le2 != &Vcb->chunks) {
                chunk* c2 = CONTAINING_RECORD(le2, chunk, list_entry);
                
                if (!c2->readonly && !c2->reloc && c2 != newchunk && c2->chunk_item->type == Vcb->data_flags) {
                    ExAcquireResourceExclusiveLite(&c2->lock, TRUE);
                    
                    if ((c2->chunk_item->size - c2->used) >= dr->size) {
                        if (find_data_address_in_chunk(Vcb, c2, dr->size, &dr->new_address)) {
                            increase_chunk_usage(c2, dr->size);
                            space_list_subtract(Vcb, c2, FALSE, dr->new_address, dr->size, &rollback);
                            ExReleaseResourceLite(&c2->lock);
                            newchunk = c2;
                            done = TRUE;
                            break;
                        }
                    }
                    
                    ExReleaseResourceLite(&c2->lock);
                }

                le2 = le2->Flink;
            }
            
            // allocate new chunk if necessary
            if (!done) {
                newchunk = alloc_chunk(Vcb, Vcb->data_flags);
                
                if (!newchunk) {
                    ERR("could not allocate new chunk\n");
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    Status = STATUS_DISK_FULL;
                    goto end;
                }
                
                ExAcquireResourceExclusiveLite(&newchunk->lock, TRUE);
                
                if (!find_data_address_in_chunk(Vcb, newchunk, dr->size, &dr->new_address)) {
                    ExReleaseResourceLite(&newchunk->lock);
                    ERR("could not find address in new chunk\n");
                    Status = STATUS_DISK_FULL;
                    goto end;
                } else {
                    increase_chunk_usage(newchunk, dr->size);
                    space_list_subtract(Vcb, newchunk, FALSE, dr->new_address, dr->size, &rollback);
                }
                
                ExReleaseResourceLite(&newchunk->lock);
            }
            
            ExReleaseResourceLite(&Vcb->chunk_lock);
        }
        
        dr->newchunk = newchunk;
        
        csum = ExAllocatePoolWithTag(PagedPool, dr->size * sizeof(UINT32) / Vcb->superblock.sector_size, ALLOC_TAG);
        if (!csum) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        Status = load_csum(Vcb, csum, dr->address, dr->size / Vcb->superblock.sector_size, NULL);

        if (NT_SUCCESS(Status)) {
            add_checksum_entry(Vcb, dr->new_address, dr->size / Vcb->superblock.sector_size, csum, NULL, &rollback);
            add_checksum_entry(Vcb, dr->address, dr->size / Vcb->superblock.sector_size, NULL, NULL, &rollback);
        }

        ExFreePool(csum);
        
        off = 0;
        
        while (off < dr->size) {
            ULONG ds = min(dr->size - off, 0x100000);
            
            Status = read_data(Vcb, dr->address + off, ds, NULL, FALSE, data, c, NULL, NULL, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("read_data returned %08x\n", Status);
                goto end;
            }
            
            Status = write_data_complete(Vcb, dr->new_address + off, data, ds, NULL, newchunk);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data_complete returned %08x\n", Status);
                goto end;
            }
            
            off += ds;
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
        
        Status = add_data_reloc_extent_item(Vcb, dr, &rollback);
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
    
    // update open FCBs
    // FIXME - speed this up
    
    ExAcquireResourceSharedLite(&Vcb->fcb_lock, TRUE);
    
    le = Vcb->all_fcbs.Flink;
    while (le != &Vcb->all_fcbs) {
        struct _fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_all);
        LIST_ENTRY* le2;

        ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
        
        le2 = fcb->extents.Flink;
        while (le2 != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
            
            if (!ext->ignore) {
                if (ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                    
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
    
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    Status = STATUS_SUCCESS;
    
    Vcb->need_write = TRUE;
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);
    
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
            if (cis[i].offset >= opts->drange_start && cis[i].offset + physsize < opts->drange_end) {
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
            args->limit_start = opts->limit_start;
            args->limit_end = opts->limit_end;
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
    LIST_ENTRY rollback;
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BALANCE_ITEM* bi;
    
    InitializeListHead(&rollback);
    
    searchkey.obj_id = BALANCE_ITEM_ID;
    searchkey.obj_type = TYPE_TEMP_ITEM;
    searchkey.offset = 0;
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!keycmp(tp.item->key, searchkey))
        delete_tree_item(Vcb, &tp, &rollback);
    
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
    
    if (!insert_tree_item(Vcb, Vcb->root_root, BALANCE_ITEM_ID, TYPE_TEMP_ITEM, 0, bi, sizeof(BALANCE_ITEM), NULL, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    if (NT_SUCCESS(Status)) {
        do_write(Vcb, NULL, &rollback);
        free_trees(Vcb);
        
        clear_rollback(Vcb, &rollback);
    } else
        do_rollback(Vcb, &rollback);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static NTSTATUS remove_balance_item(device_extension* Vcb) {
    LIST_ENTRY rollback;
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    InitializeListHead(&rollback);
    
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
        delete_tree_item(Vcb, &tp, &rollback);
        
        do_write(Vcb, NULL, &rollback);
        free_trees(Vcb);
    }

    Status = STATUS_SUCCESS;
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);
    
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
        opts->usage_end = args->usage;
    } else if (args->flags & BALANCE_ARGS_FLAGS_USAGE_RANGE) {
        opts->flags |= BTRFS_BALANCE_OPTS_USAGE;
        
        opts->usage_start = args->usage_start;
        opts->usage_end = args->usage_end;
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
        
        opts->stripes_start = args->stripes_start;
        opts->stripes_end = args->stripes_end;
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
    
    while (superblock_addrs[i] > 0 && dev->length >= superblock_addrs[i] + sizeof(superblock)) {
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

static NTSTATUS replace_mount_dev(device_extension* Vcb, device* dev, PDEVICE_OBJECT mountmgr, BOOL part0) {
    NTSTATUS Status;
    MOUNTDEV_NAME mdn, *mdn2 = NULL, *mdn3 = NULL;
    ULONG mdnsize, mmpsize;
    MOUNTMGR_MOUNT_POINT* mmp = NULL;
    MOUNTMGR_MOUNT_POINTS mmps, *mmps2 = NULL;
    ULONG i;
    UNICODE_STRING us;

    // get old device name
    
    Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), TRUE, NULL);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
        return Status;
    }
    
    mdnsize = offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;
    
    mdn2 = ExAllocatePoolWithTag(PagedPool, mdnsize, ALLOC_TAG);
    if (!mdn2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, mdnsize, TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
        goto end;
    }
    
    // get new device name
    
    Status = dev_ioctl(first_device(Vcb)->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), TRUE, NULL);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
        goto end2;
    }
    
    mdnsize = offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;
    
    mdn3 = ExAllocatePoolWithTag(PagedPool, mdnsize, ALLOC_TAG);
    if (!mdn3) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end2;
    }
    
    Status = dev_ioctl(first_device(Vcb)->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn3, mdnsize, TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
        goto end2;
    }
    
    // query and delete existing mount points
    
    mmpsize = sizeof(MOUNTMGR_MOUNT_POINT) + mdn2->NameLength;
    
    mmp = ExAllocatePoolWithTag(PagedPool, mmpsize, ALLOC_TAG);
    if (!mmp) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end2;
    }
    
    RtlZeroMemory(mmp, sizeof(MOUNTMGR_MOUNT_POINT));
    mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mmp->DeviceNameLength = mdn2->NameLength;
    RtlCopyMemory(&mmp[1], mdn2->Name, mdn2->NameLength);
    
    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_QUERY_POINTS, mmp, mmpsize, &mmps, mmpsize, TRUE, NULL);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTMGR_QUERY_POINTS returned %08x\n", Status);
        goto end2;
    }
    
    mmps2 = ExAllocatePoolWithTag(PagedPool, mmps.Size, ALLOC_TAG);
    if (!mmps2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end2;
    }
    
    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_DELETE_POINTS, mmp, mmpsize, mmps2, mmps.Size, TRUE, NULL);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTMGR_DELETE_POINTS returned %08x\n", Status);
        goto end2;
    }
    
    // re-create mount points
    
    for (i = 0; i < mmps2->NumberOfMountPoints; i++) {
        if (mmps2->MountPoints[i].SymbolicLinkNameOffset != 0) {
            ULONG mcpilen;
            MOUNTMGR_CREATE_POINT_INPUT* mcpi;
            
            mcpilen = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + mmps2->MountPoints[i].SymbolicLinkNameLength + mdn3->NameLength;
            
            mcpi = ExAllocatePoolWithTag(PagedPool, mcpilen, ALLOC_TAG);
            if (!mcpi) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end2;
            }
            
            mcpi->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
            mcpi->SymbolicLinkNameLength = mmps2->MountPoints[i].SymbolicLinkNameLength;
            mcpi->DeviceNameOffset = mcpi->SymbolicLinkNameOffset + mcpi->SymbolicLinkNameLength;
            mcpi->DeviceNameLength = mdn3->NameLength;
            
            RtlCopyMemory((UINT8*)mcpi + mcpi->SymbolicLinkNameOffset, (UINT8*)mmps2 + mmps2->MountPoints[i].SymbolicLinkNameOffset,
                          mcpi->SymbolicLinkNameLength);
            RtlCopyMemory((UINT8*)mcpi + mcpi->DeviceNameOffset, mdn3->Name, mdn3->NameLength);

            Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_CREATE_POINT, mcpi, mcpilen, NULL, 0, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("IOCTL_MOUNTMGR_CREATE_POINT returned %08x\n", Status);
                ExFreePool(mcpi);
                goto end2;
            }
            
            ExFreePool(mcpi);
        }
    }
    
    Status = STATUS_SUCCESS;
    
end2:
    // re-add old device back to mountmgr
    
    if (!part0) {
        us.Buffer = mdn2->Name;
        us.Length = us.MaximumLength = mdn2->NameLength;
        
        add_volume(mountmgr, &us);
    }
    
end:
    if (mdn2)
        ExFreePool(mdn2);
    
    if (mdn3)
        ExFreePool(mdn3);
    
    if (mmp)
        ExFreePool(mmp);
    
    if (mmps2)
        ExFreePool(mmps2);

    return Status;
}

static NTSTATUS finish_removing_device(device_extension* Vcb, device* dev) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    LIST_ENTRY rollback, *le;
    BOOL first_dev, part0 = FALSE;
    
    InitializeListHead(&rollback);
    
    if (Vcb->need_write)
        do_write(Vcb, NULL, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    // remove entry in chunk tree

    searchkey.obj_id = 1;
    searchkey.obj_type = TYPE_DEV_ITEM;
    searchkey.offset = dev->devitem.dev_id;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key))
        delete_tree_item(Vcb, &tp, &rollback);
    
    // remove stats entry in device tree
    
    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = dev->devitem.dev_id;
    
    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key))
        delete_tree_item(Vcb, &tp, &rollback);
    
    // update superblock
    
    Vcb->superblock.num_devices--;
    Vcb->superblock.total_bytes -= dev->devitem.num_bytes;
    Vcb->devices_loaded--;
    
    first_dev = first_device(Vcb) == dev;
    
    RemoveEntryList(&dev->list_entry);
    
    // flush
    
    do_write(Vcb, NULL, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    if (!dev->readonly) {
        Status = remove_superblocks(dev);
        if (!NT_SUCCESS(Status))
            WARN("remove_superblocks returned %08x\n", Status);
    }
    
    // remove entry in volume list
    
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    
    le = volumes.Flink;
    while (le != &volumes) {
        volume* v = CONTAINING_RECORD(le, volume, list_entry);
        
        if (RtlCompareMemory(&Vcb->superblock.uuid, &v->fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID) &&
            RtlCompareMemory(&dev->devitem.device_uuid, &v->devuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            PFILE_OBJECT FileObject;
            PDEVICE_OBJECT mountmgr;
            UNICODE_STRING mmdevpath;
        
            RemoveEntryList(&v->list_entry);
        
            // re-add entry to mountmgr

            if (!first_dev && v->part_num != 0) {
                RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
                Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
                if (!NT_SUCCESS(Status))
                    ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
                else {
                    add_volume(mountmgr, &v->devpath);
                    ObDereferenceObject(FileObject);
                }
            }
            
            part0 = v->part_num == 0 ? TRUE : FALSE;
        
            if (v->devpath.Buffer)
                ExFreePool(v->devpath.Buffer);
            
            ExFreePool(v);
            break;
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&volumes_lock);
    
    if (first_dev) {
        PDEVICE_OBJECT DeviceObject, olddev;
        device* newfirstdev;
        PFILE_OBJECT FileObject;
        UNICODE_STRING mmdevpath;
        PDEVICE_OBJECT mountmgr;
        
        DeviceObject = Vcb->Vpb->DeviceObject;
        
        olddev = DeviceObject->Vpb->RealDevice;
        newfirstdev = first_device(Vcb);
        
        ObReferenceObject(newfirstdev->devobj);
        DeviceObject->Vpb->RealDevice = newfirstdev->devobj;
        ObDereferenceObject(olddev);
        
        RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
        Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
        if (!NT_SUCCESS(Status))
            ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        else {
            Status = replace_mount_dev(Vcb, dev, mountmgr, part0);
            if (!NT_SUCCESS(Status))
                ERR("replace_mount_dev returned %08x\n", Status);
            
            ObDereferenceObject(FileObject);
        }
        
    }
    
    // free dev
    
    ObDereferenceObject(dev->devobj);
    
    while (!IsListEmpty(&dev->space)) {
        LIST_ENTRY* le2 = RemoveHeadList(&dev->space);
        space* s = CONTAINING_RECORD(le2, space, list_entry);
        
        ExFreePool(s);
    }
    
    ExFreePool(dev);
    
    return STATUS_SUCCESS;
}

#ifndef __REACTOS__
static void balance_thread(void* context) {
#else
static void NTAPI balance_thread(void* context) {
#endif
    device_extension* Vcb = (device_extension*)context;
    LIST_ENTRY chunks;
    LIST_ENTRY* le;
    UINT64 num_chunks[3];
    NTSTATUS Status;
    
    Vcb->balance.stopping = FALSE;
    Vcb->balance.cancelling = FALSE;
    KeInitializeEvent(&Vcb->balance.finished, NotificationEvent, FALSE);
    
    if (Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_CONVERT)
        Vcb->data_flags = BLOCK_FLAG_DATA | (Vcb->balance.opts[BALANCE_OPTS_DATA].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_DATA].convert);
    
    if (Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_CONVERT)
        Vcb->metadata_flags = BLOCK_FLAG_METADATA | (Vcb->balance.opts[BALANCE_OPTS_METADATA].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_METADATA].convert);
    
    if (Vcb->balance.opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_ENABLED && Vcb->balance.opts[BALANCE_OPTS_SYSTEM].flags & BTRFS_BALANCE_OPTS_CONVERT)
        Vcb->system_flags = BLOCK_FLAG_SYSTEM | (Vcb->balance.opts[BALANCE_OPTS_SYSTEM].convert == BLOCK_FLAG_SINGLE ? 0 : Vcb->balance.opts[BALANCE_OPTS_SYSTEM].convert);
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS) {
        if (Vcb->balance.opts[BALANCE_OPTS_DATA].flags & BTRFS_BALANCE_OPTS_ENABLED)
            RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_METADATA], &Vcb->balance.opts[BALANCE_OPTS_DATA], sizeof(btrfs_balance_opts));
        else if (Vcb->balance.opts[BALANCE_OPTS_METADATA].flags & BTRFS_BALANCE_OPTS_ENABLED)
            RtlCopyMemory(&Vcb->balance.opts[BALANCE_OPTS_DATA], &Vcb->balance.opts[BALANCE_OPTS_METADATA], sizeof(btrfs_balance_opts));
    }
    
    // FIXME - what are we supposed to do with limit_start?
    
    if (!Vcb->readonly) {
        if (!Vcb->balance.removing) {
            Status = add_balance_item(Vcb);
            if (!NT_SUCCESS(Status)) {
                ERR("add_balance_item returned %08x\n", Status);
                goto end;
            }
        } else {
            if (Vcb->need_write) {
                LIST_ENTRY rollback;
                
                InitializeListHead(&rollback);
                do_write(Vcb, NULL, &rollback);
                free_trees(Vcb);
                
                clear_rollback(Vcb, &rollback);
            }
        }
    }
    
    num_chunks[0] = num_chunks[1] = num_chunks[2] = 0;
    Vcb->balance.total_chunks = 0;
    
    InitializeListHead(&chunks);
    
    KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);
    
    if (Vcb->balance.stopping)
        goto end;
    
    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
        UINT8 sort;
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if (c->chunk_item->type & BLOCK_FLAG_DATA)
            sort = BALANCE_OPTS_DATA;
        else if (c->chunk_item->type & BLOCK_FLAG_METADATA)
            sort = BALANCE_OPTS_METADATA;
        else if (c->chunk_item->type & BLOCK_FLAG_SYSTEM)
            sort = BALANCE_OPTS_SYSTEM;
        else {
            ERR("unexpected chunk type %llx\n", c->chunk_item->type);
            ExReleaseResourceLite(&c->lock);
            break;
        }
        
        if ((!(Vcb->balance.opts[sort].flags & BTRFS_BALANCE_OPTS_LIMIT) || num_chunks[sort] < Vcb->balance.opts[sort].limit_end) &&
            should_balance_chunk(Vcb, sort, c)) {
            c->reloc = TRUE;
            
            InsertTailList(&chunks, &c->list_entry_balance);
            
            num_chunks[sort]++;
            Vcb->balance.total_chunks++;
        }
        
        ExReleaseResourceLite(&c->lock);
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&Vcb->chunk_lock);
    
    Vcb->balance.chunks_left = Vcb->balance.total_chunks;
    
    // do data chunks before metadata
    le = chunks.Flink;
    while (le != &chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_balance);
        LIST_ENTRY* le2 = le->Flink;
        
        if (c->chunk_item->type & BLOCK_FLAG_DATA) {
            NTSTATUS Status;
            BOOL changed;
            
            do {
                changed = FALSE;
                
                FsRtlEnterFileSystem();
                
                Status = balance_data_chunk(Vcb, c, &changed);
                
                FsRtlExitFileSystem();
                
                if (!NT_SUCCESS(Status)) {
                    ERR("balance_data_chunk returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                }
                
                KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);
                
                if (Vcb->balance.stopping)
                    break;
            } while (changed);
        
            if (!c->list_entry_changed.Flink)
                InsertTailList(&Vcb->chunks_changed, &c->list_entry_changed);
        }
            
        if (Vcb->balance.stopping) {
            while (le != &chunks) {
                c = CONTAINING_RECORD(le, chunk, list_entry_balance);
                c->reloc = FALSE;
                
                le = le->Flink;
            }
            goto end;
        }
        
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
        NTSTATUS Status;
        BOOL changed;
        
        le = RemoveHeadList(&chunks);
        c = CONTAINING_RECORD(le, chunk, list_entry_balance);
        
        if (c->chunk_item->type & BLOCK_FLAG_METADATA || c->chunk_item->type & BLOCK_FLAG_SYSTEM) {
            do {
                FsRtlEnterFileSystem();
                
                Status = balance_metadata_chunk(Vcb, c, &changed);
                
                FsRtlExitFileSystem();
                
                if (!NT_SUCCESS(Status)) {
                    ERR("balance_metadata_chunk returned %08x\n", Status);
                    Vcb->balance.status = Status;
                    goto end;
                }
                
                KeWaitForSingleObject(&Vcb->balance.event, Executive, KernelMode, FALSE, NULL);
                
                if (Vcb->balance.stopping)
                    break;
            } while (changed);
            
            if (!c->list_entry_changed.Flink)
                InsertTailList(&Vcb->chunks_changed, &c->list_entry_changed);
        }
        
        if (Vcb->balance.stopping) {
            while (le != &chunks) {
                c = CONTAINING_RECORD(le, chunk, list_entry_balance);
                c->reloc = FALSE;
                
                le = le->Flink;
                c->list_entry_balance.Flink = NULL;
            }
            break;
        }
        
        c->list_entry_balance.Flink = NULL;
        
        Vcb->balance.chunks_left--;
    }
    
end:
    if (!Vcb->readonly) {
        if (!Vcb->balance.removing) {
            FsRtlEnterFileSystem();
            Status = remove_balance_item(Vcb);
            FsRtlExitFileSystem();
            
            if (!NT_SUCCESS(Status)) {
                ERR("remove_balance_item returned %08x\n", Status);
                goto end;
            }
        } else {
            device* dev = NULL;
            
            FsRtlEnterFileSystem();
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
            FsRtlExitFileSystem();
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
    Vcb->balance.status = STATUS_SUCCESS;
    KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);
    
    Status = PsCreateSystemThread(&Vcb->balance.thread, 0, NULL, NULL, NULL, balance_thread, Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        return Status;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS look_for_balance_item(device_extension* Vcb) {
    LIST_ENTRY rollback;
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BALANCE_ITEM* bi;
    int i;
    
    InitializeListHead(&rollback);
    
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
    Vcb->balance.cancelling = TRUE;
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
    KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);
    
    Status = PsCreateSystemThread(&Vcb->balance.thread, 0, NULL, NULL, NULL, balance_thread, Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        dev->reloc = FALSE;
        return Status;
    }
    
    return STATUS_SUCCESS;
}
