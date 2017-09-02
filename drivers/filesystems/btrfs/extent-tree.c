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

typedef struct {
    UINT8 type;
    
    union {
        EXTENT_DATA_REF edr;
        SHARED_DATA_REF sdr;
        TREE_BLOCK_REF tbr;
        SHARED_BLOCK_REF sbr;
    };
    
    UINT64 hash;
    LIST_ENTRY list_entry;
} extent_ref;

static __inline ULONG get_extent_data_len(UINT8 type) {
    switch (type) {
        case TYPE_TREE_BLOCK_REF:
            return sizeof(TREE_BLOCK_REF);
            
        case TYPE_EXTENT_DATA_REF:
            return sizeof(EXTENT_DATA_REF);
            
        case TYPE_EXTENT_REF_V0:
            return sizeof(EXTENT_REF_V0);
            
        case TYPE_SHARED_BLOCK_REF:
            return sizeof(SHARED_BLOCK_REF);
            
        case TYPE_SHARED_DATA_REF:
            return sizeof(SHARED_DATA_REF);
            
        default:
            return 0;
    }
}

static __inline UINT64 get_extent_data_refcount(UINT8 type, void* data) {
    switch (type) {
        case TYPE_TREE_BLOCK_REF:
            return 1;
            
        case TYPE_EXTENT_DATA_REF:
        {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
            return edr->count;
        }
        
        case TYPE_EXTENT_REF_V0:
        {
            EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)data;
            return erv0->count;
        }
        
        case TYPE_SHARED_BLOCK_REF:
            return 1;
        
        case TYPE_SHARED_DATA_REF:
        {
            SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
            return sdr->count;
        }
            
        default:
            return 0;
    }
}

UINT64 get_extent_data_ref_hash2(UINT64 root, UINT64 objid, UINT64 offset) {
    UINT32 high_crc = 0xffffffff, low_crc = 0xffffffff;

    high_crc = calc_crc32c(high_crc, (UINT8*)&root, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&objid, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&offset, sizeof(UINT64));
    
    return ((UINT64)high_crc << 31) ^ (UINT64)low_crc;
}

static __inline UINT64 get_extent_data_ref_hash(EXTENT_DATA_REF* edr) {
    return get_extent_data_ref_hash2(edr->root, edr->objid, edr->offset);
}

static UINT64 get_extent_hash(UINT8 type, void* data) {
    if (type == TYPE_EXTENT_DATA_REF) {
        return get_extent_data_ref_hash((EXTENT_DATA_REF*)data);
    } else if (type == TYPE_SHARED_BLOCK_REF) {
        SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;
        return sbr->offset;
    } else if (type == TYPE_SHARED_DATA_REF) {
        SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
        return sdr->offset;
    } else if (type == TYPE_TREE_BLOCK_REF) {
        TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;
        return tbr->offset;
    } else {
        ERR("unhandled extent type %x\n", type);
        return 0;
    }
}

static void free_extent_refs(LIST_ENTRY* extent_refs) {
    while (!IsListEmpty(extent_refs)) {
        LIST_ENTRY* le = RemoveHeadList(extent_refs);
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        
        ExFreePool(er);
    }
}

static NTSTATUS add_shared_data_extent_ref(LIST_ENTRY* extent_refs, UINT64 parent, UINT32 count) {
    extent_ref* er2;
    LIST_ENTRY* le;
    
    if (!IsListEmpty(extent_refs)) {
        le = extent_refs->Flink;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            
            if (er->type == TYPE_SHARED_DATA_REF && er->sdr.offset == parent) {
                er->sdr.count += count;
                return STATUS_SUCCESS;
            }
            
            le = le->Flink;
        }
    }
    
    er2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
    if (!er2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    er2->type = TYPE_SHARED_DATA_REF;
    er2->sdr.offset = parent;
    er2->sdr.count = count;
    
    InsertTailList(extent_refs, &er2->list_entry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_shared_block_extent_ref(LIST_ENTRY* extent_refs, UINT64 parent) {
    extent_ref* er2;
    LIST_ENTRY* le;
    
    if (!IsListEmpty(extent_refs)) {
        le = extent_refs->Flink;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            
            if (er->type == TYPE_SHARED_BLOCK_REF && er->sbr.offset == parent)
                return STATUS_SUCCESS;
            
            le = le->Flink;
        }
    }
    
    er2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
    if (!er2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    er2->type = TYPE_SHARED_BLOCK_REF;
    er2->sbr.offset = parent;
    
    InsertTailList(extent_refs, &er2->list_entry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_tree_block_extent_ref(LIST_ENTRY* extent_refs, UINT64 root) {
    extent_ref* er2;
    LIST_ENTRY* le;
    
    if (!IsListEmpty(extent_refs)) {
        le = extent_refs->Flink;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            
            if (er->type == TYPE_TREE_BLOCK_REF && er->tbr.offset == root)
                return STATUS_SUCCESS;
            
            le = le->Flink;
        }
    }
    
    er2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
    if (!er2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    er2->type = TYPE_TREE_BLOCK_REF;
    er2->tbr.offset = root;
    
    InsertTailList(extent_refs, &er2->list_entry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS construct_extent_item(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 flags, LIST_ENTRY* extent_refs,
                                      KEY* firstitem, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *next_le;
    UINT64 refcount;
    ULONG inline_len;
    BOOL all_inline = TRUE;
    extent_ref* first_noninline;
    EXTENT_ITEM* ei;
    UINT8* siptr;
    
    // FIXME - write skinny extents if is tree and incompat flag set
    
    if (IsListEmpty(extent_refs)) {
        WARN("no extent refs found\n");
        return STATUS_SUCCESS;
    }
    
    refcount = 0;
    inline_len = sizeof(EXTENT_ITEM);
    
    if (flags & EXTENT_ITEM_TREE_BLOCK)
        inline_len += sizeof(EXTENT_ITEM2);
    
    le = extent_refs->Flink;
    while (le != extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        UINT64 rc;
        
        next_le = le->Flink;
        
        rc = get_extent_data_refcount(er->type, &er->edr);
        
        if (rc == 0) {
            RemoveEntryList(&er->list_entry);
            
            ExFreePool(er);
        } else {
            ULONG extlen = get_extent_data_len(er->type);
            
            refcount += rc;
            
            er->hash = get_extent_hash(er->type, &er->edr);
            
            if (all_inline) {
                if (inline_len + 1 + extlen > Vcb->superblock.node_size / 4) {
                    all_inline = FALSE;
                    first_noninline = er;
                } else
                    inline_len += extlen + 1;
            }
        }
        
        le = next_le;
    }
    
    ei = ExAllocatePoolWithTag(PagedPool, inline_len, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ei->refcount = refcount;
    ei->generation = Vcb->superblock.generation;
    ei->flags = flags;
    
    if (flags & EXTENT_ITEM_TREE_BLOCK) {
        EXTENT_ITEM2* ei2 = (EXTENT_ITEM2*)&ei[1];
        
        if (firstitem) {
            ei2->firstitem.obj_id = firstitem->obj_id;
            ei2->firstitem.obj_type = firstitem->obj_type;
            ei2->firstitem.offset = firstitem->offset;
        } else {
            ei2->firstitem.obj_id = 0;
            ei2->firstitem.obj_type = 0;
            ei2->firstitem.offset = 0;
        }
        
        ei2->level = level;
        
        siptr = (UINT8*)&ei2[1];
    } else
        siptr = (UINT8*)&ei[1];
    
    // Do we need to sort the inline extent refs? The Linux driver doesn't seem to bother.
    
    le = extent_refs->Flink;
    while (le != extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        ULONG extlen = get_extent_data_len(er->type);
        
        if (!all_inline && er == first_noninline)
            break;
        
        *siptr = er->type;
        siptr++;
        
        if (extlen > 0) {
            RtlCopyMemory(siptr, &er->edr, extlen);
            siptr += extlen;
        }
         
        le = le->Flink;
    }
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, inline_len, NULL, Irp, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(ei);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (!all_inline) {
        le = &first_noninline->list_entry;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            ULONG len = get_extent_data_len(er->type);
            UINT8* data;
            
            if (len > 0) {
                data = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
                
                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(data, &er->edr, len);
            } else
                data = NULL;
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, er->type, er->hash, data, len, NULL, Irp, rollback)) {
                ERR("error - failed to insert item\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            le = le->Flink;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS convert_old_extent(device_extension* Vcb, UINT64 address, BOOL tree, KEY* firstitem, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    LIST_ENTRY extent_refs;
    UINT64 size;
    
    InitializeListHead(&extent_refs);
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("old-style extent %llx not found\n", address);
        return STATUS_INTERNAL_ERROR;
    }
    
    size = tp.item->key.offset;
    
    delete_tree_item(Vcb, &tp, rollback);
    
    while (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
        tp = next_tp;
        
        if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_EXTENT_REF_V0 && tp.item->size >= sizeof(EXTENT_REF_V0)) {
            EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)tp.item->data;
            
            if (tree) {
                if (tp.item->key.offset == tp.item->key.obj_id) { // top of the tree
                    Status = add_tree_block_extent_ref(&extent_refs, erv0->root);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_tree_block_extent_ref returned %08x\n", Status);
                        goto end;
                    }
                } else {
                    Status = add_shared_block_extent_ref(&extent_refs, tp.item->key.offset);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_shared_block_extent_ref returned %08x\n", Status);
                        goto end;
                    }
                }
            } else {
                Status = add_shared_data_extent_ref(&extent_refs, tp.item->key.offset, erv0->count);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_shared_data_extent_ref returned %08x\n", Status);
                    goto end;
                }
            }
            
            delete_tree_item(Vcb, &tp, rollback);
        }

        if (tp.item->key.obj_id > address || tp.item->key.obj_type > TYPE_EXTENT_REF_V0)
            break;
    }

    Status = construct_extent_item(Vcb, address, size, tree ? (EXTENT_ITEM_TREE_BLOCK | EXTENT_ITEM_SHARED_BACKREFS) : EXTENT_ITEM_DATA,
                                   &extent_refs, firstitem, level, Irp, rollback);
    if (!NT_SUCCESS(Status))
        ERR("construct_extent_item returned %08x\n", Status);

end:
    free_extent_refs(&extent_refs);
    
    return Status;
}

NTSTATUS increase_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    ULONG datalen = get_extent_data_len(type), len, max_extent_item_size;
    EXTENT_ITEM* ei;
    UINT8* ptr;
    UINT64 inline_rc, offset;
    UINT8* data2;
    EXTENT_ITEM* newei;
    BOOL skinny;
    BOOL is_tree = type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF;
    
    if (datalen == 0) {
        ERR("unrecognized extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    // If entry doesn't exist yet, create new inline extent item
    
    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        ULONG eisize;
        EXTENT_ITEM* ei;
        UINT8* ptr;
        
        eisize = sizeof(EXTENT_ITEM);
        if (is_tree && !(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) eisize += sizeof(EXTENT_ITEM2);
        eisize += sizeof(UINT8);
        eisize += datalen;
        
        ei = ExAllocatePoolWithTag(PagedPool, eisize, ALLOC_TAG);
        if (!ei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        ei->refcount = get_extent_data_refcount(type, data);
        ei->generation = Vcb->superblock.generation;
        ei->flags = is_tree ? EXTENT_ITEM_TREE_BLOCK : EXTENT_ITEM_DATA;
        ptr = (UINT8*)&ei[1];
        
        if (is_tree && !(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
            EXTENT_ITEM2* ei2 = (EXTENT_ITEM2*)ptr;
            ei2->firstitem = *firstitem;
            ei2->level = level;
            ptr = (UINT8*)&ei2[1];
        }
        
        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);
        
        if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && is_tree) {
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, ei, eisize, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        } else {
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, eisize, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        }

        return STATUS_SUCCESS;
    } else if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset != size) {
        ERR("extent %llx exists, but with size %llx rather than %llx expected\n", tp.item->key.obj_id, tp.item->key.offset, size);
        return STATUS_INTERNAL_ERROR;
    }

    skinny = tp.item->key.obj_type == TYPE_METADATA_ITEM;

    if (tp.item->size == sizeof(EXTENT_ITEM_V0) && !skinny) {
        Status = convert_old_extent(Vcb, address, is_tree, firstitem, level, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_extent returned %08x\n", Status);
            return Status;
        }

        return increase_extent_refcount(Vcb, address, size, type, data, firstitem, level, Irp, rollback);
    }
        
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return STATUS_INTERNAL_ERROR;
        }
        
        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }
    
    inline_rc = 0;
    
    // Loop through existing inline extent entries
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
        
        len--;
        
        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }
        
        // If inline extent already present, increase refcount and return
        
        if (secttype == type) {
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
                
                if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
                    UINT32 rc = get_extent_data_refcount(type, data);
                    EXTENT_DATA_REF* sectedr2;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newei, tp.item->data, tp.item->size);
                    
                    newei->refcount += rc;
                    
                    sectedr2 = (EXTENT_DATA_REF*)((UINT8*)newei + ((UINT8*)sectedr - tp.item->data));
                    sectedr2->count += rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_TREE_BLOCK_REF) {
                TREE_BLOCK_REF* secttbr = (TREE_BLOCK_REF*)(ptr + sizeof(UINT8));
                TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;
                
                if (secttbr->offset == tbr->offset) {
                    TRACE("trying to increase refcount of non-shared tree extent\n");
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_BLOCK_REF) {
                SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(UINT8));
                SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;
                
                if (sectsbr->offset == sbr->offset)
                    return STATUS_SUCCESS;
            } else if (type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(UINT8));
                SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
                
                if (sectsdr->offset == sdr->offset) {
                    UINT32 rc = get_extent_data_refcount(type, data);
                    SHARED_DATA_REF* sectsdr2;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newei, tp.item->data, tp.item->size);
                    
                    newei->refcount += rc;
                    
                    sectsdr2 = (SHARED_DATA_REF*)((UINT8*)newei + ((UINT8*)sectsdr - tp.item->data));
                    sectsdr2->count += rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
        inline_rc += sectcount;
    }
    
    offset = get_extent_hash(type, data);
    
    max_extent_item_size = (Vcb->superblock.node_size >> 4) - sizeof(leaf_node);
    
    // If we can, add entry as inline extent item
    
    if (inline_rc == ei->refcount && tp.item->size + sizeof(UINT8) + datalen < max_extent_item_size) {
        len = tp.item->size - sizeof(EXTENT_ITEM);
        ptr = (UINT8*)&ei[1];
        
        if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
            len -= sizeof(EXTENT_ITEM2);
            ptr += sizeof(EXTENT_ITEM2);
        }

        while (len > 0) {
            UINT8 secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);
            
            if (secttype > type)
                break;
            
            if (secttype == type) {
                UINT64 sectoff = get_extent_hash(secttype, ptr + 1);
                
                if (sectoff > offset)
                    break;
            }
            
            len -= sectlen + sizeof(UINT8);
            ptr += sizeof(UINT8) + sectlen;
        }
        
        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size + sizeof(UINT8) + datalen, ALLOC_TAG);
        RtlCopyMemory(newei, tp.item->data, ptr - tp.item->data);
        
        newei->refcount += get_extent_data_refcount(type, data);
        
        if (len > 0)
            RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data) + sizeof(UINT8) + datalen, ptr, len);
        
        ptr = (ptr - tp.item->data) + (UINT8*)newei;
        
        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size + sizeof(UINT8) + datalen, NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        return STATUS_SUCCESS;
    }
    
    // Look for existing non-inline entry, and increase refcount if found
    
    if (inline_rc != ei->refcount) {
        traverse_ptr tp2;
        
        searchkey.obj_id = address;
        searchkey.obj_type = type;
        searchkey.offset = offset;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(tp2.item->key, searchkey)) {
            if (tp2.item->size < datalen) {
                ERR("(%llx,%x,%llx) was %x bytes, expecting %x\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp2.item->size, datalen);
                return STATUS_INTERNAL_ERROR;
            }
            
            data2 = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);
            RtlCopyMemory(data2, tp2.item->data, tp2.item->size);
            
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data2;
                
                edr->count += get_extent_data_refcount(type, data);
            } else if (type == TYPE_TREE_BLOCK_REF) {
                TRACE("trying to increase refcount of non-shared tree extent\n");
                return STATUS_SUCCESS;
            } else if (type == TYPE_SHARED_BLOCK_REF)
                return STATUS_SUCCESS;
            else if (type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data2;
                
                sdr->count += get_extent_data_refcount(type, data);
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, data2, tp2.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            RtlCopyMemory(newei, tp.item->data, tp.item->size);
            
            newei->refcount += get_extent_data_refcount(type, data);
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        }
    }
    
    // Otherwise, add new non-inline entry
    
    data2 = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
    RtlCopyMemory(data2, data, datalen);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, type, offset, data2, datalen, NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
    RtlCopyMemory(newei, tp.item->data, tp.item->size);
    
    newei->refcount += get_extent_data_refcount(type, data);
    
    delete_tree_item(Vcb, &tp, rollback);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS increase_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT64 inode, UINT64 offset, UINT32 refcount, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_DATA_REF edr;
    
    edr.root = root;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;
    
    return increase_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp, rollback);
}

void decrease_chunk_usage(chunk* c, UINT64 delta) {
    c->used -= delta;
    
    TRACE("decreasing size of chunk %llx by %llx\n", c->offset, delta);
}

NTSTATUS decrease_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem,
                                  UINT8 level, UINT64 parent, BOOL superseded, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    NTSTATUS Status;
    traverse_ptr tp, tp2;
    EXTENT_ITEM* ei;
    ULONG len;
    UINT64 inline_rc;
    UINT8* ptr;
    UINT32 rc = data ? get_extent_data_refcount(type, data) : 1;
    ULONG datalen = get_extent_data_len(type);
    BOOL is_tree = (type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF), skinny = FALSE;
    
    if (is_tree && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            skinny = TRUE;
    }
    
    if (!skinny) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_EXTENT_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("could not find EXTENT_ITEM for address %llx\n", address);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->key.offset != size) {
            ERR("extent %llx had length %llx, not %llx as expected\n", address, tp.item->key.offset, size);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
            Status = convert_old_extent(Vcb, address, is_tree, firstitem, level, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("convert_old_extent returned %08x\n", Status);
                return Status;
            }

            return decrease_extent_refcount(Vcb, address, size, type, data, firstitem, level, parent, superseded, Irp, rollback);
        }
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return STATUS_INTERNAL_ERROR;
        }
        
        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }
    
    if (ei->refcount < rc) {
        ERR("error - extent has refcount %llx, trying to reduce by %x\n", ei->refcount, rc);
        return STATUS_INTERNAL_ERROR;
    }
    
    inline_rc = 0;
    
    // Loop through inline extent entries
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
        
        len--;
        
        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (secttype == type) {
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
                ULONG neweilen;
                EXTENT_ITEM* newei;
                
                if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
                    if (ei->refcount == edr->count) {
                        delete_tree_item(Vcb, &tp, rollback);
                        
                        if (!superseded)
                            add_checksum_entry(Vcb, address, size / Vcb->superblock.sector_size, NULL, Irp, rollback);
                        
                        return STATUS_SUCCESS;
                    }
                    
                    if (sectedr->count < edr->count) {
                        ERR("error - extent section has refcount %x, trying to reduce by %x\n", sectedr->count, edr->count);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    if (sectedr->count > edr->count)    // reduce section refcount
                        neweilen = tp.item->size;
                    else                                // remove section entirely
                        neweilen = tp.item->size - sizeof(UINT8) - sectlen;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    if (sectedr->count > edr->count) {
                        EXTENT_DATA_REF* newedr = (EXTENT_DATA_REF*)((UINT8*)newei + ((UINT8*)sectedr - tp.item->data));
                        
                        RtlCopyMemory(newei, ei, neweilen);
                        
                        newedr->count -= rc;
                    } else {
                        RtlCopyMemory(newei, ei, ptr - tp.item->data);
                        
                        if (len > sectlen)
                            RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(UINT8), len - sectlen);
                    }
                    
                    newei->refcount -= rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(UINT8));
                SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
                ULONG neweilen;
                EXTENT_ITEM* newei;
                
                if (sectsdr->offset == sdr->offset) {
                    if (ei->refcount == sectsdr->count) {
                        delete_tree_item(Vcb, &tp, rollback);
                        
                        if (!superseded)
                            add_checksum_entry(Vcb, address, size / Vcb->superblock.sector_size, NULL, Irp, rollback);
                        
                        return STATUS_SUCCESS;
                    }
                    
                    if (sectsdr->count < sdr->count) {
                        ERR("error - SHARED_DATA_REF has refcount %x, trying to reduce by %x\n", sectsdr->count, sdr->count);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    if (sectsdr->count > sdr->count)    // reduce section refcount
                        neweilen = tp.item->size;
                    else                                // remove section entirely
                        neweilen = tp.item->size - sizeof(UINT8) - sectlen;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    if (sectsdr->count > sdr->count) {
                        SHARED_DATA_REF* newsdr = (SHARED_DATA_REF*)((UINT8*)newei + ((UINT8*)sectsdr - tp.item->data));
                        
                        RtlCopyMemory(newei, ei, neweilen);
                        
                        newsdr->count -= rc;
                    } else {
                        RtlCopyMemory(newei, ei, ptr - tp.item->data);
                        
                        if (len > sectlen)
                            RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(UINT8), len - sectlen);
                    }

                    newei->refcount -= rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_TREE_BLOCK_REF) {
                TREE_BLOCK_REF* secttbr = (TREE_BLOCK_REF*)(ptr + sizeof(UINT8));
                TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;
                ULONG neweilen;
                EXTENT_ITEM* newei;
                
                if (secttbr->offset == tbr->offset) {
                    if (ei->refcount == 1) {
                        delete_tree_item(Vcb, &tp, rollback);
                        return STATUS_SUCCESS;
                    }

                    neweilen = tp.item->size - sizeof(UINT8) - sectlen;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newei, ei, ptr - tp.item->data);
                    
                    if (len > sectlen)
                        RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(UINT8), len - sectlen);
                    
                    newei->refcount--;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_BLOCK_REF) {
                SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(UINT8));
                SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;
                ULONG neweilen;
                EXTENT_ITEM* newei;
                
                if (sectsbr->offset == sbr->offset) {
                    if (ei->refcount == 1) {
                        delete_tree_item(Vcb, &tp, rollback);
                        return STATUS_SUCCESS;
                    }
                    
                    neweilen = tp.item->size - sizeof(UINT8) - sectlen;
                    
                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newei, ei, ptr - tp.item->data);
                    
                    if (len > sectlen)
                        RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(UINT8), len - sectlen);
                    
                    newei->refcount--;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
        inline_rc += sectcount;
    }
    
    if (inline_rc == ei->refcount) {
        ERR("entry not found in inline extent item for address %llx\n", address);
        return STATUS_INTERNAL_ERROR;
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = type;
    searchkey.offset = (type == TYPE_SHARED_DATA_REF || type == TYPE_EXTENT_REF_V0) ? parent : get_extent_hash(type, data);
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(tp2.item->key, searchkey)) {
        ERR("(%llx,%x,%llx) not found\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp2.item->size < datalen) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, datalen);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (type == TYPE_EXTENT_DATA_REF) {
        EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)tp2.item->data;
        EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
        EXTENT_ITEM* newei;
        
        if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
            if (ei->refcount == edr->count) {
                delete_tree_item(Vcb, &tp, rollback);
                delete_tree_item(Vcb, &tp2, rollback);
                
                if (!superseded)
                    add_checksum_entry(Vcb, address, size / Vcb->superblock.sector_size, NULL, Irp, rollback);
                
                return STATUS_SUCCESS;
            }
            
            if (sectedr->count < edr->count) {
                ERR("error - extent section has refcount %x, trying to reduce by %x\n", sectedr->count, edr->count);
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            if (sectedr->count > edr->count) {
                EXTENT_DATA_REF* newedr = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);
                
                if (!newedr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(newedr, sectedr, tp2.item->size);
                
                newedr->count -= edr->count;
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, newedr, tp2.item->size, NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
                }
            }
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        } else {
            ERR("error - hash collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_SHARED_DATA_REF) {
        SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)tp2.item->data;
        SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
        EXTENT_ITEM* newei;
        
        if (sectsdr->offset == sdr->offset) {
            if (ei->refcount == sdr->count) {
                delete_tree_item(Vcb, &tp, rollback);
                delete_tree_item(Vcb, &tp2, rollback);
                
                if (!superseded)
                    add_checksum_entry(Vcb, address, size / Vcb->superblock.sector_size, NULL, Irp, rollback);
                
                return STATUS_SUCCESS;
            }
            
            if (sectsdr->count < sdr->count) {
                ERR("error - extent section has refcount %x, trying to reduce by %x\n", sectsdr->count, sdr->count);
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            if (sectsdr->count > sdr->count) {
                SHARED_DATA_REF* newsdr = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);
                
                if (!newsdr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(newsdr, sectsdr, tp2.item->size);
                
                newsdr->count -= sdr->count;
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, newsdr, tp2.item->size, NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    return STATUS_INTERNAL_ERROR;
                }
            }
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        } else {
            ERR("error - collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_SHARED_BLOCK_REF) {
        SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)tp2.item->data;
        SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;
        EXTENT_ITEM* newei;
        
        if (sectsbr->offset == sbr->offset) {
            if (ei->refcount == 1) {
                delete_tree_item(Vcb, &tp, rollback);
                delete_tree_item(Vcb, &tp2, rollback);
                return STATUS_SUCCESS;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        } else {
            ERR("error - collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_TREE_BLOCK_REF) {
        TREE_BLOCK_REF* secttbr = (TREE_BLOCK_REF*)tp2.item->data;
        TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;
        EXTENT_ITEM* newei;
        
        if (secttbr->offset == tbr->offset) {
            if (ei->refcount == 1) {
                delete_tree_item(Vcb, &tp, rollback);
                delete_tree_item(Vcb, &tp2, rollback);
                return STATUS_SUCCESS;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        } else {
            ERR("error - collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_EXTENT_REF_V0) {
        EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)tp2.item->data;
        EXTENT_ITEM* newei;
        
        if (ei->refcount == erv0->count) {
            delete_tree_item(Vcb, &tp, rollback);
            delete_tree_item(Vcb, &tp2, rollback);
            
            if (!superseded)
                add_checksum_entry(Vcb, address, size / Vcb->superblock.sector_size, NULL, Irp, rollback);
            
            return STATUS_SUCCESS;
        }
        
        delete_tree_item(Vcb, &tp2, rollback);
        
        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
        if (!newei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(newei, tp.item->data, tp.item->size);

        newei->refcount -= rc;
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        return STATUS_SUCCESS;
    } else {
        ERR("unhandled extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }
}

NTSTATUS decrease_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT64 inode,
                                       UINT64 offset, UINT32 refcount, BOOL superseded, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_DATA_REF edr;
    
    edr.root = root;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;
    
    return decrease_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, 0, superseded, Irp, rollback);
}

NTSTATUS decrease_extent_refcount_tree(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root,
                                       UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    TREE_BLOCK_REF tbr;
    
    tbr.offset = root;
    
    return decrease_extent_refcount(Vcb, address, size, TYPE_TREE_BLOCK_REF, &tbr, NULL/*FIXME*/, level, 0, FALSE, Irp, rollback);
}

static UINT64 find_extent_data_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_DATA_REF* edr;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        TRACE("could not find address %llx in extent tree\n", address);
        return 0;
    }
    
    if (tp.item->key.offset != size) {
        ERR("extent %llx had size %llx, not %llx as expected\n", address, tp.item->key.offset, size);
        return 0;
    }
    
    if (tp.item->size >= sizeof(EXTENT_ITEM)) {
        EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;
        UINT32 len = tp.item->size - sizeof(EXTENT_ITEM);
        UINT8* ptr = (UINT8*)&ei[1];
        
        while (len > 0) {
            UINT8 secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);
            UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
            
            len--;
            
            if (sectlen > len) {
                ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
                return 0;
            }

            if (sectlen == 0) {
                ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
                return 0;
            }
            
            if (secttype == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));
                
                if (sectedr->root == root && sectedr->objid == objid && sectedr->offset == offset)
                    return sectcount;
            }
            
            len -= sectlen;
            ptr += sizeof(UINT8) + sectlen;
        }
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_DATA_REF;
    searchkey.offset = get_extent_data_ref_hash2(root, objid, offset);
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (!keycmp(searchkey, tp.item->key)) {    
        if (tp.item->size < sizeof(EXTENT_DATA_REF))
            ERR("(%llx,%x,%llx) has size %u, not %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA_REF));
        else {    
            edr = (EXTENT_DATA_REF*)tp.item->data;
            
            return edr->count;
        }
    }
    
    return 0;
}

UINT64 get_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;
    
        return ei->refcount;
    }
    
    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find (%llx,%x,%llx) in extent tree\n", address, TYPE_EXTENT_ITEM, size);
        return 0;
    } else if (tp.item->key.offset != size) {
        ERR("extent %llx had size %llx, not %llx as expected\n", address, tp.item->key.offset, size);
        return 0;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        EXTENT_ITEM_V0* eiv0 = (EXTENT_ITEM_V0*)tp.item->data;
        
        return eiv0->refcount;
    } else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %x bytes, expected at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                       tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    return ei->refcount;
}

BOOL is_extent_unique(device_extension* Vcb, UINT64 address, UINT64 size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    UINT64 rc, rcrun, root = 0, inode = 0;
    UINT32 len;
    EXTENT_ITEM* ei;
    UINT8* ptr;
    BOOL b;
    
    rc = get_extent_refcount(Vcb, address, size, Irp);

    if (rc == 1)
        return TRUE;
    
    if (rc == 0)
        return FALSE;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        WARN("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (keycmp(tp.item->key, searchkey)) {
        WARN("could not find (%llx,%x,%llx)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return FALSE;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return FALSE;
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return FALSE;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (ei->flags & EXTENT_ITEM_TREE_BLOCK) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return FALSE;
        }
        
        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }
    
    rcrun = 0;
    
    // Loop through inline extent entries
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
        
        len--;
        
        if (sectlen > len) {
            WARN("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return FALSE;
        }

        if (sectlen == 0) {
            WARN("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return FALSE;
        }
        
        if (secttype == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));
            
            if (root == 0 && inode == 0) {
                root = sectedr->root;
                inode = sectedr->objid;
            } else if (root != sectedr->root || inode != sectedr->objid)
                return FALSE;
        } else
            return FALSE;
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
        rcrun += sectcount;
    }
    
    if (rcrun == rc)
        return TRUE;

    // Loop through non-inlines if some refs still unaccounted for
    
    do {
        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)tp.item->data;
            
            if (tp.item->size < sizeof(EXTENT_DATA_REF)) {
                WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                     tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
                return FALSE;
            }
            
            if (root == 0 && inode == 0) {
                root = edr->root;
                inode = edr->objid;
            } else if (root != edr->root || inode != edr->objid)
                return FALSE;
            
            rcrun += edr->count;
        }
        
        if (rcrun == rc)
            return TRUE;
        
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id)
                break;
        }
    } while (b);
    
    // If we reach this point, there's still some refs unaccounted for somewhere.
    // Return FALSE in case we mess things up elsewhere.
    
    return FALSE;
}

UINT64 get_extent_flags(device_extension* Vcb, UINT64 address, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;
    
        return ei->flags;
    }
    
    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find %llx in extent tree\n", address);
        return 0;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return 0;
    else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %x bytes, expected at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                   tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    return ei->flags;
}

void update_extent_flags(device_extension* Vcb, UINT64 address, UINT64 flags, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return;
    }
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;
        ei->flags = flags;
        return;
    }
    
    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find %llx in extent tree\n", address);
        return;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return;
    else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %x bytes, expected at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                   tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
        return;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    ei->flags = flags;
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
    ce->superseded = FALSE;
    InitializeListHead(&ce->refs);
    InitializeListHead(&ce->old_refs);
    
    InsertTailList(&c->changed_extents, &ce->list_entry);
    
    return ce;
}

NTSTATUS update_changed_extent_ref(device_extension* Vcb, chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, signed long long count,
                                   BOOL no_csum, BOOL superseded, PIRP Irp) {
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
    
    le = ce->refs.Flink;
    while (le != &ce->refs) {
        cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        
        if (cer->type == TYPE_EXTENT_DATA_REF && cer->edr.root == root && cer->edr.objid == objid && cer->edr.offset == offset) {
            ce->count += count;
            cer->edr.count += count;
            Status = STATUS_SUCCESS;
            
            if (superseded)
                ce->superseded = TRUE;
            
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
        
        cer->type = TYPE_EXTENT_DATA_REF;
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
    
    cer->type = TYPE_EXTENT_DATA_REF;
    cer->edr.root = root;
    cer->edr.objid = objid;
    cer->edr.offset = offset;
    cer->edr.count = old_count + count;
    
    InsertTailList(&ce->refs, &cer->list_entry);
    
    ce->count += count;
    
    if (superseded)
        ce->superseded = TRUE;
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&c->changed_extents_lock);
    
    return Status;
}

void add_changed_extent_ref(chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, UINT32 count, BOOL no_csum) {
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
        
        if (cer->type == TYPE_EXTENT_DATA_REF && cer->edr.root == root && cer->edr.objid == objid && cer->edr.offset == offset) {
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
    
    cer->type = TYPE_EXTENT_DATA_REF;
    cer->edr.root = root;
    cer->edr.objid = objid;
    cer->edr.offset = offset;
    cer->edr.count = count;
    
    InsertTailList(&ce->refs, &cer->list_entry);
    
    ce->count += count;
}

UINT64 find_extent_shared_tree_refcount(device_extension* Vcb, UINT64 address, UINT64 parent, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 inline_rc;
    EXTENT_ITEM* ei;
    UINT32 len;
    UINT8* ptr;
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        TRACE("could not find address %llx in extent tree\n", address);
        return 0;
    }
    
    if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset != Vcb->superblock.node_size) {
        ERR("extent %llx had size %llx, not %llx as expected\n", address, tp.item->key.offset, Vcb->superblock.node_size);
        return 0;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx): size was %u, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    inline_rc = 0;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (searchkey.obj_type == TYPE_EXTENT_ITEM && ei->flags & EXTENT_ITEM_TREE_BLOCK) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%llx,%x,%llx): size was %u, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                                                                       tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return 0;
        }
        
        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
        
        len--;
        
        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return 0;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return 0;
        }
        
        if (secttype == TYPE_SHARED_BLOCK_REF) {
            SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(UINT8));
            
            if (sectsbr->offset == parent)
                return 1;
        }
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
        inline_rc += sectcount;
    }
    
    // FIXME - what if old?
    
    if (inline_rc == ei->refcount)
        return 0;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_SHARED_BLOCK_REF;
    searchkey.offset = parent;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (!keycmp(searchkey, tp.item->key)) {    
        if (tp.item->size < sizeof(SHARED_BLOCK_REF))
            ERR("(%llx,%x,%llx) has size %u, not %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(SHARED_BLOCK_REF));
        else
            return 1;
    }
    
    return 0;
}

UINT64 find_extent_shared_data_refcount(device_extension* Vcb, UINT64 address, UINT64 parent, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 inline_rc;
    EXTENT_ITEM* ei;
    UINT32 len;
    UINT8* ptr;
    
    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        TRACE("could not find address %llx in extent tree\n", address);
        return 0;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx): size was %u, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    inline_rc = 0;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    while (len > 0) {
        UINT8 secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));
        
        len--;
        
        if (sectlen > len) {
            ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return 0;
        }

        if (sectlen == 0) {
            ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return 0;
        }
        
        if (secttype == TYPE_SHARED_DATA_REF) {
            SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(UINT8));
            
            if (sectsdr->offset == parent)
                return sectsdr->count;
        }
        
        len -= sectlen;
        ptr += sizeof(UINT8) + sectlen;
        inline_rc += sectcount;
    }
    
    // FIXME - what if old?
    
    if (inline_rc == ei->refcount)
        return 0;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_SHARED_DATA_REF;
    searchkey.offset = parent;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (!keycmp(searchkey, tp.item->key)) {    
        if (tp.item->size < sizeof(SHARED_DATA_REF))
            ERR("(%llx,%x,%llx) has size %u, not %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(SHARED_DATA_REF));
        else {
            SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)tp.item->data;
            return sdr->count;
        }
    }
    
    return 0;
}
