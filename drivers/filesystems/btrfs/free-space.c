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

// Number of increments in the size of each cache inode, in sectors. Should
// this be a constant number of sectors, a constant 256 KB, or what?
#define CACHE_INCREMENTS    64

static NTSTATUS remove_free_space_inode(device_extension* Vcb, KEY* key, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    traverse_ptr tp;
    INODE_ITEM* ii;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, key, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(key, &tp.item->key)) {
        ERR("could not find (%llx,%x,%llx) in root_root\n", key->obj_id, key->obj_type, key->offset);
        return STATUS_NOT_FOUND;
    }
    
    if (tp.item->size < offsetof(INODE_ITEM, st_blocks)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, offsetof(INODE_ITEM, st_blocks));
        return STATUS_INTERNAL_ERROR;
    }
    
    ii = (INODE_ITEM*)tp.item->data;
    
    Status = excise_extents_inode(Vcb, Vcb->root_root, key->obj_id, NULL, 0, ii->st_size, NULL, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08x\n", Status);
        return Status;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    return STATUS_SUCCESS;
}

NTSTATUS clear_free_space_cache(device_extension* Vcb) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b;
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
            break;
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            delete_tree_item(Vcb, &tp, &rollback);
            
            if (tp.item->size >= sizeof(FREE_SPACE_ITEM)) {
                FREE_SPACE_ITEM* fsi = (FREE_SPACE_ITEM*)tp.item->data;
                
                if (fsi->key.obj_type != TYPE_INODE_ITEM)
                    WARN("key (%llx,%x,%llx) does not point to an INODE_ITEM\n", fsi->key.obj_id, fsi->key.obj_type, fsi->key.offset);
                else {
                    Status = remove_free_space_inode(Vcb, &fsi->key, &rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("remove_free_space_inode for (%llx,%x,%llx) returned %08x\n", fsi->key.obj_id, fsi->key.obj_type, fsi->key.offset, Status);
                        goto end;
                    }
                }
            } else
                WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b)
            tp = next_tp;
    } while (b);
    
    Status = STATUS_SUCCESS;
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);
    
    return Status;
}

static NTSTATUS add_space_entry(chunk* c, UINT64 offset, UINT64 size) {
    space* s;
    
    s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

    if (!s) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    s->offset = offset;
    s->size = size;
    s->type = SPACE_TYPE_FREE;
    
    if (IsListEmpty(&c->space))
        InsertTailList(&c->space, &s->list_entry);
    else {
        space* s2 = CONTAINING_RECORD(c->space.Blink, space, list_entry);
        
        if (s2->offset < offset)
            InsertTailList(&c->space, &s->list_entry);
        else {
            LIST_ENTRY* le;
            
            le = c->space.Flink;
            while (le != &c->space) {
                s2 = CONTAINING_RECORD(le, space, list_entry);
                
                if (s2->offset > offset) {
                    InsertTailList(le, &s->list_entry);
                    return STATUS_SUCCESS;
                }
                
                le = le->Flink;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static void load_free_space_bitmap(device_extension* Vcb, chunk* c, UINT64 offset, void* data) {
    RTL_BITMAP bmph;
    UINT32 i, *dwords = data;
    ULONG runlength, index;
    
    // flip bits
    for (i = 0; i < Vcb->superblock.sector_size / sizeof(UINT32); i++) {
        dwords[i] = ~dwords[i];
    }

    RtlInitializeBitMap(&bmph, data, Vcb->superblock.sector_size * 8);

    index = 0;
    runlength = RtlFindFirstRunClear(&bmph, &index);
            
    while (runlength != 0) {
        UINT64 addr, length;
        
        addr = offset + (index * Vcb->superblock.sector_size);
        length = Vcb->superblock.sector_size * runlength;
        
        add_space_entry(c, addr, length);
        index += runlength;
       
        runlength = RtlFindNextForwardRunClear(&bmph, index, &index);
    }
}

static NTSTATUS load_stored_free_space_cache(device_extension* Vcb, chunk* c) {
    KEY searchkey;
    traverse_ptr tp, tp2;
    FREE_SPACE_ITEM* fsi;
    UINT64 inode, num_sectors, num_valid_sectors, i, *generation;
    INODE_ITEM* ii;
    UINT8* data;
    NTSTATUS Status;
    UINT32 *checksums, crc32;
    FREE_SPACE_ENTRY* fse;
    UINT64 size, num_entries, num_bitmaps, extent_length, bmpnum, off;
    LIST_ENTRY* le;
    
    // FIXME - does this break if Vcb->superblock.sector_size is not 4096?
    
    TRACE("(%p, %llx)\n", Vcb, c->offset);
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        TRACE("(%llx,%x,%llx) not found\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_NOT_FOUND;
    }
    
    if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
        WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        return STATUS_NOT_FOUND;
    }
    
    fsi = (FREE_SPACE_ITEM*)tp.item->data;
    
    if (fsi->key.obj_type != TYPE_INODE_ITEM) {
        WARN("cache pointed to something other than an INODE_ITEM\n");
        return STATUS_NOT_FOUND;
    }
    
    inode = fsi->key.obj_id;
    
    searchkey = fsi->key;

    num_entries = fsi->num_entries;
    num_bitmaps = fsi->num_bitmaps;
    
    Status = find_item(Vcb, Vcb->root_root, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp2.item->key, &searchkey)) {
        WARN("(%llx,%x,%llx) not found\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_NOT_FOUND;
    }
    
    if (tp2.item->size < sizeof(INODE_ITEM)) {
        WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp2.item->size, sizeof(INODE_ITEM));
        return STATUS_NOT_FOUND;
    }
    
    ii = (INODE_ITEM*)tp2.item->data;
    
    if (ii->st_size == 0) {
        ERR("inode %llx had a length of 0\n", inode);
        return STATUS_NOT_FOUND;
    }
    
    c->cache_size = ii->st_size;
    c->cache_inode = fsi->key.obj_id;
    
    size = sector_align(ii->st_size, Vcb->superblock.sector_size);
    
    data = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
    
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_file(Vcb, Vcb->root_root, inode, data, 0, ii->st_size, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("read_file returned %08x\n", Status);
        ExFreePool(data);
        return Status;
    }
    
    if (size > ii->st_size)
        RtlZeroMemory(&data[ii->st_size], size - ii->st_size);
    
    num_sectors = size / Vcb->superblock.sector_size;
    
    generation = (UINT64*)(data + (num_sectors * sizeof(UINT32)));
    
    if (*generation != fsi->generation) {
        WARN("free space cache generation for %llx was %llx, expected %llx\n", c->offset, generation, fsi->generation);
        ExFreePool(data);
        return STATUS_NOT_FOUND;
    }
    
    extent_length = (num_sectors * sizeof(UINT32)) + sizeof(UINT64) + (num_entries * sizeof(FREE_SPACE_ENTRY));
    
    num_valid_sectors = (sector_align(extent_length, Vcb->superblock.sector_size) / Vcb->superblock.sector_size) + num_bitmaps;
    
    if (num_valid_sectors > num_sectors) {
        ERR("free space cache for %llx was %llx sectors, expected at least %llx\n", c->offset, num_sectors, num_valid_sectors);
        ExFreePool(data);
        return STATUS_NOT_FOUND;
    }
    
    checksums = (UINT32*)data;
    
    for (i = 0; i < num_valid_sectors; i++) {
        if (i * Vcb->superblock.sector_size > sizeof(UINT32) * num_sectors)
            crc32 = ~calc_crc32c(0xffffffff, &data[i * Vcb->superblock.sector_size], Vcb->superblock.sector_size);
        else if ((i + 1) * Vcb->superblock.sector_size < sizeof(UINT32) * num_sectors)
            crc32 = 0; // FIXME - test this
        else
            crc32 = ~calc_crc32c(0xffffffff, &data[sizeof(UINT32) * num_sectors], ((i + 1) * Vcb->superblock.sector_size) - (sizeof(UINT32) * num_sectors));
        
        if (crc32 != checksums[i]) {
            WARN("checksum %llu was %08x, expected %08x\n", i, crc32, checksums[i]);
            ExFreePool(data);
            return STATUS_NOT_FOUND;
        }
    }
    
    off = (sizeof(UINT32) * num_sectors) + sizeof(UINT64);

    bmpnum = 0;
    for (i = 0; i < num_entries; i++) {
        if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
            off = sector_align(off, Vcb->superblock.sector_size);
        
        fse = (FREE_SPACE_ENTRY*)&data[off];
        
        if (fse->type == FREE_SPACE_EXTENT) {
            Status = add_space_entry(c, fse->offset, fse->size);
            if (!NT_SUCCESS(Status)) {
                ERR("add_space_entry returned %08x\n", Status);
                ExFreePool(data);
                return Status;
            }
        } else if (fse->type != FREE_SPACE_BITMAP) {
            ERR("unknown free-space type %x\n", fse->type);
        }
                
        off += sizeof(FREE_SPACE_ENTRY);
    }
    
    if (num_bitmaps > 0) {
        bmpnum = sector_align(off, Vcb->superblock.sector_size) / Vcb->superblock.sector_size;
        off = (sizeof(UINT32) * num_sectors) + sizeof(UINT64);
        
        for (i = 0; i < num_entries; i++) {
            if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
                off = sector_align(off, Vcb->superblock.sector_size);
            
            fse = (FREE_SPACE_ENTRY*)&data[off];
            
            if (fse->type == FREE_SPACE_BITMAP) {
                // FIXME - make sure we don't overflow the buffer here
                load_free_space_bitmap(Vcb, c, fse->offset, &data[bmpnum * Vcb->superblock.sector_size]);
                bmpnum++;
            }
            
            off += sizeof(FREE_SPACE_ENTRY);
        }
    }
    
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);
        LIST_ENTRY* le2 = le->Flink;
        
        if (le2 != &c->space) {
            space* s2 = CONTAINING_RECORD(le2, space, list_entry);
            
            if (s2->offset == s->offset + s->size) {
                s->size += s2->size;
                
                RemoveEntryList(&s2->list_entry);
                ExFreePool(s2);
                
                le2 = le;
            }
        }
        
        le = le2;
    }
    
    ExFreePool(data);
    
    return STATUS_SUCCESS;
}

NTSTATUS load_free_space_cache(device_extension* Vcb, chunk* c) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    UINT64 lastaddr;
    BOOL b;
    space *s, *s2;
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    if (Vcb->superblock.generation - 1 == Vcb->superblock.cache_generation) {
        Status = load_stored_free_space_cache(Vcb, c);
        
        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
            ERR("load_stored_free_space_cache returned %08x\n", Status);
            return Status;
        }
    } else
        Status = STATUS_NOT_FOUND;
     
    if (Status == STATUS_NOT_FOUND) {
        TRACE("generating free space cache for chunk %llx\n", c->offset);
        
        searchkey.obj_id = c->offset;
        searchkey.obj_type = TYPE_EXTENT_ITEM;
        searchkey.offset = 0;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        lastaddr = c->offset;
        
        do {
            if (tp.item->key.obj_id >= c->offset + c->chunk_item->size)
                break;
            
            if (tp.item->key.obj_id >= c->offset && (tp.item->key.obj_type == TYPE_EXTENT_ITEM || tp.item->key.obj_type == TYPE_METADATA_ITEM)) {
                if (tp.item->key.obj_id > lastaddr) {
                    s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
                    
                    if (!s) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    s->offset = lastaddr;
                    s->size = tp.item->key.obj_id - lastaddr;
                    s->type = SPACE_TYPE_FREE;
                    InsertTailList(&c->space, &s->list_entry);
                    
                    TRACE("(%llx,%llx)\n", s->offset, s->size);
                }
                
                if (tp.item->key.obj_type == TYPE_METADATA_ITEM)
                    lastaddr = tp.item->key.obj_id + Vcb->superblock.node_size;
                else
                    lastaddr = tp.item->key.obj_id + tp.item->key.offset;
            }
            
            b = find_next_item(Vcb, &tp, &next_tp, FALSE);
            if (b)
                tp = next_tp;
        } while (b);
        
        if (lastaddr < c->offset + c->chunk_item->size) {
            s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
            
            if (!s) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            s->offset = lastaddr;
            s->size = c->offset + c->chunk_item->size - lastaddr;
            s->type = SPACE_TYPE_FREE;
            InsertTailList(&c->space, &s->list_entry);
            
            TRACE("(%llx,%llx)\n", s->offset, s->size);
        }
    }
    
    // add allocated space
    
    lastaddr = c->offset;
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->offset > lastaddr) {
            s2 = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
            
            if (!s2) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            s2->offset = lastaddr;
            s2->size = s->offset - lastaddr;
            s2->type = SPACE_TYPE_USED;
            
            InsertTailList(&s->list_entry, &s2->list_entry);
        }
        
        lastaddr = s->offset + s->size;
        
        le = le->Flink;
    }
    
    if (lastaddr < c->offset + c->chunk_item->size) {
        s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
        
        if (!s) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        s->offset = lastaddr;
        s->size = c->offset + c->chunk_item->size - lastaddr;
        s->type = SPACE_TYPE_USED;
        InsertTailList(&c->space, &s->list_entry);
    }
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        TRACE("%llx,%llx,%u\n", s->offset, s->size, s->type);
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS insert_cache_extent(device_extension* Vcb, UINT64 inode, UINT64 start, UINT64 length, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    chunk* c;
    UINT64 flags;
    
    // FIXME - how do we know which RAID level to put this to?
    flags = BLOCK_FLAG_DATA; // SINGLE
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk_inode(Vcb, Vcb->root_root, inode, NULL, c, start, length, FALSE, NULL, NULL, rollback))
                return STATUS_SUCCESS;
        }
        
        le = le->Flink;
    }
    
    if ((c = alloc_chunk(Vcb, flags, rollback))) {
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk_inode(Vcb, Vcb->root_root, inode, NULL, c, start, length, FALSE, NULL, NULL, rollback))
                return STATUS_SUCCESS;
        }
    }
    
    WARN("couldn't find any data chunks with %llx bytes free\n", length);

    return STATUS_DISK_FULL;
}

static NTSTATUS allocate_cache_chunk(device_extension* Vcb, chunk* c, BOOL* changed, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    UINT64 num_entries, new_cache_size, i;
    UINT64 lastused = c->offset;
    UINT32 num_sectors;
    
    // FIXME - also do bitmaps
    // FIXME - make sure this works when sector_size is not 4096
    
    *changed = FALSE;
    
    num_entries = 0;
    
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if (s->type == SPACE_TYPE_USED || s->type == SPACE_TYPE_WRITING) {
            if (s->offset > lastused) {
//                 TRACE("free: (%llx,%llx)\n", lastused, s->offset - lastused);
                num_entries++;
            }
            
            lastused = s->offset + s->size;
        }
        
        le = le->Flink;
    }
    
    if (c->offset + c->chunk_item->size > lastused) {
//         TRACE("free: (%llx,%llx)\n", lastused, c->offset + c->chunk_item->size - lastused);
        num_entries++;
    }
    
    new_cache_size = sizeof(UINT64) + (num_entries * sizeof(FREE_SPACE_ENTRY));
    
    num_sectors = sector_align(new_cache_size, Vcb->superblock.sector_size) / Vcb->superblock.sector_size;
    num_sectors = sector_align(num_sectors, CACHE_INCREMENTS);
    
    // adjust for padding
    // FIXME - there must be a more efficient way of doing this
    new_cache_size = sizeof(UINT64) + (sizeof(UINT32) * num_sectors);
    for (i = 0; i < num_entries; i++) {
        if ((new_cache_size / Vcb->superblock.sector_size) != ((new_cache_size + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size))
            new_cache_size = sector_align(new_cache_size, Vcb->superblock.sector_size);
        
        new_cache_size += sizeof(FREE_SPACE_ENTRY);
    }
    
    new_cache_size = sector_align(new_cache_size, CACHE_INCREMENTS * Vcb->superblock.sector_size);
    
    TRACE("chunk %llx: cache_size = %llx, new_cache_size = %llx\n", c->offset, c->cache_size, new_cache_size);
    
    if (new_cache_size > c->cache_size) {
        if (c->cache_size == 0) {
            INODE_ITEM* ii;
            FREE_SPACE_ITEM* fsi;
            UINT64 inode;
            
            // create new inode
            
            ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
            if (!ii) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlZeroMemory(ii, sizeof(INODE_ITEM));
            ii->st_size = new_cache_size;
            ii->st_blocks = new_cache_size;
            ii->st_nlink = 1;
            ii->st_mode = S_IRUSR | S_IWUSR | __S_IFREG;
            ii->flags = BTRFS_INODE_NODATASUM | BTRFS_INODE_NODATACOW | BTRFS_INODE_NOCOMPRESS | BTRFS_INODE_PREALLOC;
            
            if (Vcb->root_root->lastinode == 0)
                get_last_inode(Vcb, Vcb->root_root);
            
            inode = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;

            if (!insert_tree_item(Vcb, Vcb->root_root, inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            // create new free space entry
            
            fsi = ExAllocatePoolWithTag(PagedPool, sizeof(FREE_SPACE_ITEM), ALLOC_TAG);
            if (!fsi) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            fsi->key.obj_id = inode;
            fsi->key.obj_type = TYPE_INODE_ITEM;
            fsi->key.offset = 0;
            
            if (!insert_tree_item(Vcb, Vcb->root_root, FREE_SPACE_CACHE_ID, 0, c->offset, fsi, sizeof(FREE_SPACE_ITEM), NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            // allocate space
            
            Status = insert_cache_extent(Vcb, inode, 0, new_cache_size, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_cache_extent returned %08x\n", Status);
                return Status;
            }
            
            Vcb->root_root->lastinode = inode;
            c->cache_inode = inode;
        } else {
            KEY searchkey;
            traverse_ptr tp;
            INODE_ITEM* ii;
            
            ERR("extending existing inode\n");
            
            // FIXME - try to extend existing extent first of all
            // Or ditch all existing extents and replace with one new one?
            
            // add INODE_ITEM to tree cache
            
            searchkey.obj_id = c->cache_inode;
            searchkey.obj_type = TYPE_INODE_ITEM;
            searchkey.offset = 0;
            
            Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (keycmp(&searchkey, &tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (tp.item->size < sizeof(INODE_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
                return STATUS_INTERNAL_ERROR;
            }
            
            ii = (INODE_ITEM*)tp.item->data;
            
            if (!tp.tree->write) {
                tp.tree->write = TRUE;
                Vcb->write_trees++;
            }

            // add free_space entry to tree cache
            
            searchkey.obj_id = FREE_SPACE_CACHE_ID;
            searchkey.obj_type = 0;
            searchkey.offset = c->offset;
            
            Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (keycmp(&searchkey, &tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
                return STATUS_INTERNAL_ERROR;
            }
            
            if (!tp.tree->write) {
                tp.tree->write = TRUE;
                Vcb->write_trees++;
            }

            // add new extent
            
            Status = insert_cache_extent(Vcb, c->cache_inode, c->cache_size, new_cache_size - c->cache_size, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_cache_extent returned %08x\n", Status);
                return Status;
            }
            
            // modify INODE_ITEM
            
            ii->st_size = new_cache_size;
            ii->st_blocks = new_cache_size;
        }
        
        c->cache_size = new_cache_size;
        *changed = TRUE;
    } else {
        KEY searchkey;
        traverse_ptr tp;
        
        // add INODE_ITEM and free_space entry to tree cache, for writing later
        
        searchkey.obj_id = c->cache_inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&searchkey, &tp.item->key)) {
            ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size < sizeof(INODE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }
        
        if (!tp.tree->write) {
            tp.tree->write = TRUE;
            Vcb->write_trees++;
        }

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&searchkey, &tp.item->key)) {
            ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }
        
        if (!tp.tree->write) {
            tp.tree->write = TRUE;
            Vcb->write_trees++;
        }
    }
    
    // FIXME - reduce inode allocation if cache is shrinking. Make sure to avoid infinite write loops
    
    return STATUS_SUCCESS;
}

NTSTATUS allocate_cache(device_extension* Vcb, BOOL* changed, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    NTSTATUS Status;
    chunk* c;

    *changed = FALSE;
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->space_changed) {
            BOOL b;
            
            Status = allocate_cache_chunk(Vcb, c, &b, rollback);
            
            if (b)
                *changed = TRUE;
            
            if (!NT_SUCCESS(Status)) {
                ERR("allocate_cache_chunk(%llx) returned %08x\n", c->offset, Status);
                return Status;
            }
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_chunk_cache(device_extension* Vcb, chunk* c, BTRFS_TIME* now, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    FREE_SPACE_ITEM* fsi;
    INODE_ITEM* ii;
    void* data;
    FREE_SPACE_ENTRY* fse;
    UINT64 num_entries, num_sectors, lastused, *cachegen, i, off;
    UINT32* checksums;
    LIST_ENTRY* le;
    BOOL b;
    
    data = ExAllocatePoolWithTag(NonPagedPool, c->cache_size, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(data, c->cache_size);
    
    num_entries = 0;
    num_sectors = c->cache_size / Vcb->superblock.sector_size;
    off = (sizeof(UINT32) * num_sectors) + sizeof(UINT64);
    
    lastused = c->offset;
    
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if (s->type == SPACE_TYPE_USED || s->type == SPACE_TYPE_WRITING) {
            if (s->offset > lastused) {
                if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
                    off = sector_align(off, Vcb->superblock.sector_size);
                
                fse = (FREE_SPACE_ENTRY*)((UINT8*)data + off);
                
                fse->offset = lastused;
                fse->size = s->offset - lastused;
                fse->type = FREE_SPACE_EXTENT;
                num_entries++;
                
                off += sizeof(FREE_SPACE_ENTRY);
            }
            
            lastused = s->offset + s->size;
        }
        
        le = le->Flink;
    }
    
    if (c->offset + c->chunk_item->size > lastused) {
        if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
            off = sector_align(off, Vcb->superblock.sector_size);
        
        fse = (FREE_SPACE_ENTRY*)((UINT8*)data + off);
        
        fse->offset = lastused;
        fse->size = c->offset + c->chunk_item->size - lastused;
        fse->type = FREE_SPACE_EXTENT;
        num_entries++;
    }

    // update INODE_ITEM
    
    searchkey.obj_id = c->cache_inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size < sizeof(INODE_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    ii = (INODE_ITEM*)tp.item->data;
    
    ii->generation = Vcb->superblock.generation;
    ii->transid = Vcb->superblock.generation;
    ii->sequence++;
    ii->st_ctime = *now;
    
    // update free_space item
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        return STATUS_INTERNAL_ERROR;
    }
    
    fsi = (FREE_SPACE_ITEM*)tp.item->data;
    
    fsi->generation = Vcb->superblock.generation;
    fsi->num_entries = num_entries;
    fsi->num_bitmaps = 0;
    
    // set cache generation
    
    cachegen = (UINT64*)((UINT8*)data + (sizeof(UINT32) * num_sectors));
    *cachegen = Vcb->superblock.generation;
    
    // calculate cache checksums
    
    checksums = (UINT32*)data;
    
    // FIXME - if we know sector is fully zeroed, use cached checksum
    
    for (i = 0; i < num_sectors; i++) {
        if (i * Vcb->superblock.sector_size > sizeof(UINT32) * num_sectors)
            checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
        else if ((i + 1) * Vcb->superblock.sector_size < sizeof(UINT32) * num_sectors)
            checksums[i] = 0; // FIXME - test this
        else
            checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + (sizeof(UINT32) * num_sectors), ((i + 1) * Vcb->superblock.sector_size) - (sizeof(UINT32) * num_sectors));
    }
    
    // write cache
    
    searchkey.obj_id = c->cache_inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        traverse_ptr next_tp;
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->key.offset < c->cache_size) {
            EXTENT_DATA* ed;
            EXTENT_DATA2* eds;
            
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
                return STATUS_INTERNAL_ERROR;
            }
            
            ed = (EXTENT_DATA*)tp.item->data;
            
            if (ed->type != EXTENT_TYPE_REGULAR) {
                ERR("cache EXTENT_DATA type not regular\n");
                return STATUS_INTERNAL_ERROR;
            }

            if (tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
                return STATUS_INTERNAL_ERROR;
            }
            
            eds = (EXTENT_DATA2*)&ed->data[0];

            if (ed->compression != BTRFS_COMPRESSION_NONE) {
                ERR("not writing compressed cache\n");
                return STATUS_INTERNAL_ERROR;
            }

            if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
                WARN("encryption not supported\n");
                return STATUS_INTERNAL_ERROR;
            }

            if (ed->encoding != BTRFS_ENCODING_NONE) {
                WARN("other encodings not supported\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            if (eds->address == 0) {
                ERR("not writing cache to sparse extent\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            Status = write_data(Vcb, eds->address + eds->offset, (UINT8*)data + tp.item->key.offset, min(c->cache_size - tp.item->key.offset, eds->num_bytes));
            if (!NT_SUCCESS(Status)) {
                ERR("write_data returned %08x\n", Status);
                return Status;
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
                break;
        }
    } while (b);
    
    ExFreePool(data);
    
    return STATUS_SUCCESS;
}

NTSTATUS update_chunk_caches(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    NTSTATUS Status;
    chunk* c;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->space_changed) {
            Status = update_chunk_cache(Vcb, c, &now, rollback);

            if (!NT_SUCCESS(Status)) {
                ERR("update_chunk_cache(%llx) returned %08x\n", c->offset, Status);
                return Status;
            }
            
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}
