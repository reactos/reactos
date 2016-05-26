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

static __inline ULONG get_extent_data_len(UINT8 type) {
    switch (type) {
        case TYPE_TREE_BLOCK_REF:
            return sizeof(TREE_BLOCK_REF);
            
        case TYPE_EXTENT_DATA_REF:
            return sizeof(EXTENT_DATA_REF);
            
        // FIXME - TYPE_EXTENT_REF_V0
        // FIXME - TYPE_SHARED_BLOCK_REF
            
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
        
        // FIXME - TYPE_EXTENT_REF_V0
        // FIXME - TYPE_SHARED_BLOCK_REF
        
        case TYPE_SHARED_DATA_REF:
        {
            SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
            return sdr->count;
        }
            
        default:
            return 0;
    }
}

static UINT64 get_extent_data_ref_hash(EXTENT_DATA_REF* edr) {
    UINT32 high_crc = 0xffffffff, low_crc = 0xffffffff;

    high_crc = calc_crc32c(high_crc, (UINT8*)&edr->root, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&edr->objid, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&edr->offset, sizeof(UINT64));
    
    return ((UINT64)high_crc << 31) ^ (UINT64)low_crc;
}

static UINT64 get_extent_hash(UINT8 type, void* data) {
    if (type == TYPE_EXTENT_DATA_REF) {
        return get_extent_data_ref_hash((EXTENT_DATA_REF*)data);
    } else {
        ERR("unhandled extent type %x\n", type);
        return 0;
    }
}

static NTSTATUS increase_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem, UINT8 level, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    ULONG datalen = get_extent_data_len(type), len, max_extent_item_size;
    EXTENT_ITEM* ei;
    UINT8* ptr;
    UINT64 inline_rc, offset;
    UINT8* data2;
    EXTENT_ITEM* newei;
    
    // FIXME - handle A9s
    
    if (datalen == 0) {
        ERR("unrecognized extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    // If entry doesn't exist yet, create new inline extent item
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ULONG eisize;
        EXTENT_ITEM* ei;
        BOOL is_tree = type == TYPE_TREE_BLOCK_REF;
        UINT8* ptr;
        
        eisize = sizeof(EXTENT_ITEM);
        if (is_tree) eisize += sizeof(EXTENT_ITEM2);
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
        
        if (is_tree) {
            EXTENT_ITEM2* ei2 = (EXTENT_ITEM2*)ptr;
            ei2->firstitem = *firstitem;
            ei2->level = level;
            ptr = (UINT8*)&ei2[1];
        }
        
        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, eisize, NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        // FIXME - add to space list?

        return STATUS_SUCCESS;
    } else if (tp.item->key.offset != size) {
        ERR("extent %llx exists, but with size %llx rather than %llx expected\n", tp.item->key.obj_id, tp.item->key.offset, size);
        return STATUS_INTERNAL_ERROR;
    } else if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        TRACE("converting old-style extent at (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        Status = convert_old_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            return Status;
        }
        
        return increase_extent_refcount(Vcb, address, size, type, data, firstitem, level, rollback);
    } else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (ei->flags & EXTENT_ITEM_TREE_BLOCK) {
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
        
        if (secttype == TYPE_SHARED_DATA_REF) {
            TRACE("found shared data extent at %llx, converting\n", tp.item->key.obj_id);
            
            Status = convert_shared_data_extent(Vcb, address, size, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("convert_shared_data_extent returned %08x\n", Status);
                return Status;
            }
            
            return increase_extent_refcount(Vcb, address, size, type, data, firstitem, level, rollback);
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
                    
                    newei->generation = Vcb->superblock.generation;
                    newei->refcount += rc;
                    
                    sectedr2 = (EXTENT_DATA_REF*)((UINT8*)newei + ((UINT8*)sectedr - tp.item->data));
                    sectedr2->count += rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_TREE_BLOCK_REF) {
                ERR("trying to increase refcount of tree extent\n");
                return STATUS_INTERNAL_ERROR;
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
        
        if (ei->flags & EXTENT_ITEM_TREE_BLOCK) {
            len -= sizeof(EXTENT_ITEM2);
            ptr += sizeof(EXTENT_ITEM2);
        }

        while (len > 0) {
            UINT8 secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);
            
            if (secttype > type)
                break;
            
            len--;
            
            if (secttype == type) {
                UINT64 sectoff = get_extent_hash(secttype, ptr + 1);
                
                if (sectoff > offset)
                    break;
            }
            
            len -= sectlen;
            ptr += sizeof(UINT8) + sectlen;
        }
        
        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size + sizeof(UINT8) + datalen, ALLOC_TAG);
        RtlCopyMemory(newei, tp.item->data, ptr - tp.item->data);
        
        newei->generation = Vcb->superblock.generation;
        newei->refcount += get_extent_data_refcount(type, data);
        
        if (len > 0)
            RtlCopyMemory((UINT8*)newei + (ptr - tp.item->data) + sizeof(UINT8) + datalen, ptr, len + 1);
        
        ptr = (ptr - tp.item->data) + (UINT8*)newei;
        
        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size + sizeof(UINT8) + datalen, NULL, rollback)) {
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
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&tp.item->key, &searchkey)) {
            if (tp.item->size < datalen) {
                ERR("(%llx,%x,%llx) was %x bytes, expecting %x\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp.item->size, datalen);
                return STATUS_INTERNAL_ERROR;
            }
            
            data2 = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);
            RtlCopyMemory(data2, tp2.item->data, tp2.item->size);
            
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data2;
                
                edr->count += get_extent_data_refcount(type, data);
            } else if (type == TYPE_TREE_BLOCK_REF) {
                ERR("trying to increase refcount of tree extent\n");
                return STATUS_INTERNAL_ERROR;
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp2, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, data2, tp2.item->size, NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            RtlCopyMemory(newei, tp.item->data, tp.item->size);
            
            newei->generation = Vcb->superblock.generation;
            newei->refcount += get_extent_data_refcount(type, data);
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        }
    }
    
    // Otherwise, add new non-inline entry
    
    data2 = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
    RtlCopyMemory(data2, data, datalen);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, type, offset, data2, datalen, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
    RtlCopyMemory(newei, tp.item->data, tp.item->size);
    
    newei->generation = Vcb->superblock.generation;
    newei->refcount += get_extent_data_refcount(type, data);
    
    delete_tree_item(Vcb, &tp, rollback);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS increase_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, UINT32 refcount, LIST_ENTRY* rollback) {
    EXTENT_DATA_REF edr;
    
    edr.root = subvol->id;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;
    
    return increase_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, rollback);
}

void decrease_chunk_usage(chunk* c, UINT64 delta) {
    c->used -= delta;
    
    TRACE("decreasing size of chunk %llx by %llx\n", c->offset, delta);
}

static NTSTATUS remove_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* changed_sector_list) {
    chunk* c;
    LIST_ENTRY* le;
    
    if (changed_sector_list) {
        changed_sector* sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
        if (!sc) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        sc->ol.key = address;
        sc->checksums = NULL;
        sc->length = size / Vcb->superblock.sector_size;

        sc->deleted = TRUE;
        
        insert_into_ordered_list(changed_sector_list, &sc->ol);
    }
    
    c = NULL;
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (address >= c->offset && address + size < c->offset + c->chunk_item->size)
            break;
        
        le = le->Flink;
    }
    if (le == &Vcb->chunks) c = NULL;
    
    if (c) {
        decrease_chunk_usage(c, size);
        
        add_to_space_list(c, address, size, SPACE_TYPE_DELETING);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS decrease_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem,
                                         UINT8 level, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    NTSTATUS Status;
    traverse_ptr tp, tp2;
    EXTENT_ITEM* ei;
    ULONG len;
    UINT64 inline_rc, offset;
    UINT8* ptr;
    UINT32 rc = get_extent_data_refcount(type, data);
    ULONG datalen = get_extent_data_len(type);
    
    // FIXME - handle trees
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
        TRACE("converting old-style extent at (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        Status = convert_old_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            return Status;
        }
        
        return decrease_extent_refcount(Vcb, address, size, type, data, firstitem, level, changed_sector_list, rollback);
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (UINT8*)&ei[1];
    
    if (ei->flags & EXTENT_ITEM_TREE_BLOCK) {
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
        
        if (secttype == TYPE_SHARED_DATA_REF) {
            TRACE("found shared data extent at %llx, converting\n", tp.item->key.obj_id);
            
            Status = convert_shared_data_extent(Vcb, address, size, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("convert_shared_data_extent returned %08x\n", Status);
                return Status;
            }
            
            return decrease_extent_refcount(Vcb, address, size, type, data, firstitem, level, changed_sector_list, rollback);
        }
        
        if (secttype == type) {
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
                ULONG neweilen;
                EXTENT_ITEM* newei;
                
                if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
                    if (ei->refcount == edr->count) {
                        Status = remove_extent(Vcb, address, size, changed_sector_list);
                        if (!NT_SUCCESS(Status)) {
                            ERR("remove_extent returned %08x\n", Status);
                            return Status;
                        }
                        
                        delete_tree_item(Vcb, &tp, rollback);
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
                    
                    newei->generation = Vcb->superblock.generation;
                    newei->refcount -= rc;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    return STATUS_SUCCESS;
                }
//             } else if (type == TYPE_TREE_BLOCK_REF) {
//                 ERR("trying to increase refcount of tree extent\n");
//                 return STATUS_INTERNAL_ERROR;
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
    
    offset = get_extent_hash(type, data);
    
    searchkey.obj_id = address;
    searchkey.obj_type = type;
    searchkey.offset = offset;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp2.item->key, &searchkey)) {
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
                Status = remove_extent(Vcb, address, size, changed_sector_list);
                if (!NT_SUCCESS(Status)) {
                    ERR("remove_extent returned %08x\n", Status);
                    return Status;
                }
                
                delete_tree_item(Vcb, &tp, rollback);
                delete_tree_item(Vcb, &tp2, rollback);
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
                
                if (!insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, newedr, tp2.item->size, NULL, rollback)) {
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

            newei->generation = Vcb->superblock.generation;
            newei->refcount -= rc;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        } else {
            ERR("error - hash collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
//     } else if (type == TYPE_TREE_BLOCK_REF) {
//         ERR("trying to increase refcount of tree extent\n");
//         return STATUS_INTERNAL_ERROR;
    } else {
        ERR("unhandled extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }
}

NTSTATUS decrease_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode,
                                       UINT64 offset, UINT32 refcount, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    EXTENT_DATA_REF edr;
    
    edr.root = subvol->id;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;
    
    return decrease_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, changed_sector_list, rollback);
}

typedef struct {
    UINT8 type;
    void* data;
    BOOL allocated;
    UINT64 hash;
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

static NTSTATUS add_data_extent_ref(LIST_ENTRY* extent_refs, UINT64 tree_id, UINT64 obj_id, UINT64 offset) {
    extent_ref* er2;
    EXTENT_DATA_REF* edr;
    LIST_ENTRY* le;
    
    if (!IsListEmpty(extent_refs)) {
        le = extent_refs->Flink;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            
            if (er->type == TYPE_EXTENT_DATA_REF) {
                edr = (EXTENT_DATA_REF*)er->data;
                
                if (edr->root == tree_id && edr->objid == obj_id && edr->offset == offset) {
                    edr->count++;
                    return STATUS_SUCCESS;
                }
            }
            
            le = le->Flink;
        }
    }
    
    er2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
    if (!er2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    edr = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA_REF), ALLOC_TAG);
    if (!edr) {
        ERR("out of memory\n");
        ExFreePool(er2);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    edr->root = tree_id;
    edr->objid = obj_id;
    edr->offset = offset;
    edr->count = 1; // FIXME - not necessarily
    
    er2->type = TYPE_EXTENT_DATA_REF;
    er2->data = edr;
    er2->allocated = TRUE;
    
    InsertTailList(extent_refs, &er2->list_entry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS construct_extent_item(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 flags, LIST_ENTRY* extent_refs, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *next_le;
    UINT64 refcount;
    ULONG inline_len;
    BOOL all_inline = TRUE;
    extent_ref* first_noninline;
    EXTENT_ITEM* ei;
    UINT8* siptr;
    
    if (IsListEmpty(extent_refs)) {
        WARN("no extent refs found\n");
        return STATUS_SUCCESS;
    }
    
    refcount = 0;
    inline_len = sizeof(EXTENT_ITEM);
    
    le = extent_refs->Flink;
    while (le != extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        UINT64 rc;
        
        next_le = le->Flink;
        
        rc = get_extent_data_refcount(er->type, er->data);
        
        if (rc == 0) {
            if (er->allocated)
                ExFreePool(er->data);
            
            RemoveEntryList(&er->list_entry);
            
            ExFreePool(er);
        } else {
            ULONG extlen = get_extent_data_len(er->type);
            
            refcount += rc;
            
            if (er->type == TYPE_EXTENT_DATA_REF)
                er->hash = get_extent_data_ref_hash(er->data);
            else
                er->hash = 0;
            
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
    
    // Do we need to sort the inline extent refs? The Linux driver doesn't seem to bother.
    
    siptr = (UINT8*)&ei[1];
    le = extent_refs->Flink;
    while (le != extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        ULONG extlen = get_extent_data_len(er->type);
        
        if (!all_inline && er == first_noninline)
            break;
        
        *siptr = er->type;
        siptr++;
        
        if (extlen > 0) {
            RtlCopyMemory(siptr, er->data, extlen);
            siptr += extlen;
        }
         
        le = le->Flink;
    }
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, inline_len, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(ei);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (!all_inline) {
        le = &first_noninline->list_entry;
        
        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, er->type, er->hash, er->data, get_extent_data_len(er->type), NULL, rollback)) {
                ERR("error - failed to insert item\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            er->allocated = FALSE;
            
            le = le->Flink;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS populate_extent_refs_from_tree(device_extension* Vcb, UINT64 tree_address, UINT64 extent_address, LIST_ENTRY* extent_refs) {
    UINT8* buf;
    tree_header* th;
    NTSTATUS Status;
    
    buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = read_tree(Vcb, tree_address, buf);
    if (!NT_SUCCESS(Status)) {
        ERR("read_tree returned %08x\n", Status);
        ExFreePool(buf);
        return Status;
    }
    
    th = (tree_header*)buf;

    if (th->level == 0) {
        UINT32 i;
        leaf_node* ln = (leaf_node*)&th[1];
        
        for (i = 0; i < th->num_items; i++) {
            if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) && ln[i].offset + ln[i].size <= Vcb->superblock.node_size - sizeof(tree_header)) {
                EXTENT_DATA* ed = (EXTENT_DATA*)(((UINT8*)&th[1]) + ln[i].offset);
                
                if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];
                    
                    if (ed2->address == extent_address) {
                        Status = add_data_extent_ref(extent_refs, th->tree_id, ln[i].key.obj_id, ln[i].key.offset);
                        if (!NT_SUCCESS(Status)) {
                            ERR("add_data_extent_ref returned %08x\n", Status);
                            ExFreePool(buf);
                            return Status;
                        }
                    }
                }
            }
        }
    } else
        WARN("shared data ref pointed to tree of level %x\n", th->level);
    
    ExFreePool(buf);
    
    return STATUS_SUCCESS;
}

NTSTATUS convert_shared_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    LIST_ENTRY extent_refs;
    LIST_ENTRY *le, *next_le;
    EXTENT_ITEM* ei;
    UINT64 eiflags, inline_rc;
    UINT8* siptr;
    ULONG len;
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
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    len = tp.item->size - sizeof(EXTENT_ITEM);
    eiflags = ei->flags;
    
    InitializeListHead(&extent_refs);
    
    inline_rc = 0;
    siptr = (UINT8*)&ei[1];
    
    do {
        extent_ref* er;
        ULONG extlen;
        
        extlen = get_extent_data_len(*siptr);
        
        if (extlen == 0) {
            ERR("unrecognized extent subitem %x\n", *siptr);
            free_extent_refs(&extent_refs);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (extlen > len - 1) {
            ERR("extent %llx was truncated\n", address);
            free_extent_refs(&extent_refs);
            return STATUS_INTERNAL_ERROR;
        }
        
        er = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
        if (!er) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        er->type = *siptr;
        
        er->data = ExAllocatePoolWithTag(PagedPool, extlen, ALLOC_TAG);
        if (!er->data) {
            ERR("out of memory\n");
            ExFreePool(er);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(er->data, siptr+1, extlen);
        er->allocated = TRUE;
        
        InsertTailList(&extent_refs, &er->list_entry);
        
        siptr += extlen;
        len -= extlen + 1;
        
        inline_rc += get_extent_data_refcount(er->type, er->data);
    } while (len > 0);
    
    delete_tree_item(Vcb, &tp, rollback);
    
    if (inline_rc < ei->refcount) {
        BOOL b;
        traverse_ptr next_tp;
        
        do {
            b = find_next_item(Vcb, &tp, &next_tp, FALSE);
            
            if (tp.item->key.obj_id == address) {
                ULONG extlen;
                
                extlen = get_extent_data_len(tp.item->key.obj_type);
                
                if (extlen != 0 && tp.item->size >= extlen) {
                    extent_ref* er = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
                    if (!er) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    er->type = tp.item->key.obj_type;
                    
                    er->data = ExAllocatePoolWithTag(PagedPool, extlen, ALLOC_TAG);
                    if (!er->data) {
                        ERR("out of memory\n");
                        ExFreePool(er);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(er->data, siptr+1, extlen);
                    er->allocated = TRUE;
                    
                    InsertTailList(&extent_refs, &er->list_entry);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                }
            }
            
            if (b) {
                tp = next_tp;
                
                if (tp.item->key.obj_id > address)
                    break;
            }
        } while (b);
    }
    
    le = extent_refs.Flink;
    while (le != &extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        next_le = le->Flink;
        
        if (er->type == TYPE_SHARED_DATA_REF) {
            SHARED_DATA_REF* sdr = er->data;
            
            Status = populate_extent_refs_from_tree(Vcb, sdr->offset, address, &extent_refs);
            if (!NT_SUCCESS(Status)) {
                ERR("populate_extent_refs_from_tree returned %08x\n", Status);
                free_extent_refs(&extent_refs);
                return Status;
            }

            RemoveEntryList(&er->list_entry);
            
            if (er->allocated)
                ExFreePool(er->data);
            
            ExFreePool(er);
        }
        // FIXME - also do for SHARED_BLOCK_REF?

        le = next_le;
    }
    
    Status = construct_extent_item(Vcb, address, size, eiflags, &extent_refs, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("construct_extent_item returned %08x\n", Status);
        free_extent_refs(&extent_refs);
        return Status;
    }
    
    free_extent_refs(&extent_refs);
    
    return STATUS_SUCCESS;
}

NTSTATUS convert_old_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    LIST_ENTRY extent_refs;
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
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size != sizeof(EXTENT_ITEM_V0)) {
        TRACE("extent does not appear to be old - returning STATUS_SUCCESS\n");
        return STATUS_SUCCESS;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_REF_V0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    InitializeListHead(&extent_refs);
    
    do {
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            Status = populate_extent_refs_from_tree(Vcb, tp.item->key.offset, address, &extent_refs);
            if (!NT_SUCCESS(Status)) {
                ERR("populate_extent_refs_from_tree returned %08x\n", Status);
                return Status;
            }
            
            delete_tree_item(Vcb, &tp, rollback);
        }
        
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    Status = construct_extent_item(Vcb, address, size, EXTENT_ITEM_DATA, &extent_refs, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("construct_extent_item returned %08x\n", Status);
        free_extent_refs(&extent_refs);
        return Status;
    }
    
    free_extent_refs(&extent_refs);
    
    return STATUS_SUCCESS;
}
