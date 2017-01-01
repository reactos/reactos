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

// #define DEBUG_SPACE_LISTS

static NTSTATUS remove_free_space_inode(device_extension* Vcb, UINT64 inode, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb* fcb;
    
    Status = open_fcb(Vcb, Vcb->root_root, inode, BTRFS_TYPE_FILE, NULL, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        return Status;
    }
    
    fcb->dirty = TRUE;
    
    if (fcb->inode_item.st_size > 0) {
        Status = excise_extents(fcb->Vcb, fcb, 0, sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08x\n", Status);
            return Status;
        }
    }
    
    fcb->deleted = TRUE;
    
    flush_fcb(fcb, FALSE, batchlist, Irp, rollback);
    
    free_fcb(fcb);

    return STATUS_SUCCESS;
}

NTSTATUS clear_free_space_cache(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b;
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
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
                    LIST_ENTRY* le;
                    
                    Status = remove_free_space_inode(Vcb, fsi->key.obj_id, batchlist, Irp, &rollback);
                    
                    if (!NT_SUCCESS(Status))
                        ERR("remove_free_space_inode for (%llx,%x,%llx) returned %08x\n", fsi->key.obj_id, fsi->key.obj_type, fsi->key.offset, Status);
                    
                    le = Vcb->chunks.Flink;
                    while (le != &Vcb->chunks) {
                        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
                        
                        if (c->offset == tp.item->key.offset && c->cache) {
                            free_fcb(c->cache);
                            c->cache = NULL;
                        }
                        
                        le = le->Flink;
                    }
                }
            } else
                WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
        if (b)
            tp = next_tp;
    } while (b);
    
    Status = STATUS_SUCCESS;
    
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);
    
    return Status;
}

NTSTATUS add_space_entry(LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 offset, UINT64 size) {
    space* s;
    
    s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

    if (!s) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    s->address = offset;
    s->size = size;
    
    if (IsListEmpty(list))
        InsertTailList(list, &s->list_entry);
    else {
        space* s2 = CONTAINING_RECORD(list->Blink, space, list_entry);
        
        if (s2->address < offset)
            InsertTailList(list, &s->list_entry);
        else {
            LIST_ENTRY* le;
            
            le = list->Flink;
            while (le != list) {
                s2 = CONTAINING_RECORD(le, space, list_entry);
                
                if (s2->address > offset) {
                    InsertTailList(le, &s->list_entry);
                    goto size;
                }
                
                le = le->Flink;
            }
        }
    }
    
size:
    if (!list_size)
        return STATUS_SUCCESS;
    
    if (IsListEmpty(list_size))
        InsertTailList(list_size, &s->list_entry_size);
    else {
        space* s2 = CONTAINING_RECORD(list_size->Blink, space, list_entry_size);
        
        if (s2->size >= size)
            InsertTailList(list_size, &s->list_entry_size);
        else {
            LIST_ENTRY* le;
            
            le = list_size->Flink;
            while (le != list_size) {
                s2 = CONTAINING_RECORD(le, space, list_entry_size);
                
                if (s2->size <= size) {
                    InsertHeadList(le->Blink, &s->list_entry_size);
                    return STATUS_SUCCESS;
                }
                
                le = le->Flink;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static void load_free_space_bitmap(device_extension* Vcb, chunk* c, UINT64 offset, void* data, UINT64* total_space) {
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
        
        add_space_entry(&c->space, &c->space_size, addr, length);
        index += runlength;
        *total_space += length;
       
        runlength = RtlFindNextForwardRunClear(&bmph, index, &index);
    }
}

static void order_space_entry(space* s, LIST_ENTRY* list_size) {
    LIST_ENTRY* le;
    
    if (IsListEmpty(list_size)) {
        InsertHeadList(list_size, &s->list_entry_size);
        return;
    }
    
    le = list_size->Flink;
    
    while (le != list_size) {
        space* s2 = CONTAINING_RECORD(le, space, list_entry_size);
        
        if (s2->size <= s->size) {
            InsertHeadList(le->Blink, &s->list_entry_size);
            return;
        }
        
        le = le->Flink;
    }
    
    InsertTailList(list_size, &s->list_entry_size);
}

typedef struct {
    UINT64 stripe;
    LIST_ENTRY list_entry;
} superblock_stripe;

static NTSTATUS add_superblock_stripe(LIST_ENTRY* stripes, UINT64 off, UINT64 len) {
    UINT64 i;
    
    for (i = 0; i < len; i++) {
        LIST_ENTRY* le;
        superblock_stripe* ss;
        BOOL ignore = FALSE;
        
        le = stripes->Flink;
        while (le != stripes) {
            ss = CONTAINING_RECORD(le, superblock_stripe, list_entry);
            
            if (ss->stripe == off + i) {
                ignore = TRUE;
                break;
            }
            
            le = le->Flink;
        }
        
        if (ignore)
            continue;
        
        ss = ExAllocatePoolWithTag(PagedPool, sizeof(superblock_stripe), ALLOC_TAG);
        if (!ss) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        ss->stripe = off + i;
        InsertTailList(stripes, &ss->list_entry);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS get_superblock_size(chunk* c, UINT64* size) {
    NTSTATUS Status;
    CHUNK_ITEM* ci = c->chunk_item;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    UINT64 off_start, off_end, space;
    UINT16 i = 0, j;
    LIST_ENTRY stripes;
    
    InitializeListHead(&stripes);
    
    while (superblock_addrs[i] != 0) {
        if (ci->type & BLOCK_FLAG_RAID0 || ci->type & BLOCK_FLAG_RAID10) {
            for (j = 0; j < ci->num_stripes; j++) {
                ULONG sub_stripes = max(ci->sub_stripes, 1);
                
                if (cis[j].offset + (ci->size * ci->num_stripes / sub_stripes) > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % ci->stripe_length;
                    off_start *= ci->num_stripes / sub_stripes;
                    off_start += (j / sub_stripes) * ci->stripe_length;

                    off_end = off_start + ci->stripe_length;
                    
                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, 1);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08x\n", Status);
                        goto end;
                    }
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID5) {
            for (j = 0; j < ci->num_stripes; j++) {
                UINT64 stripe_size = ci->size / (ci->num_stripes - 1);
                
                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % (ci->stripe_length * (ci->num_stripes - 1));
                    off_start *= ci->num_stripes - 1;

                    off_end = off_start + (ci->stripe_length * (ci->num_stripes - 1));

                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08x\n", Status);
                        goto end;
                    }
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID6) {
            for (j = 0; j < ci->num_stripes; j++) {
                UINT64 stripe_size = ci->size / (ci->num_stripes - 2);
                
                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % (ci->stripe_length * (ci->num_stripes - 2));
                    off_start *= ci->num_stripes - 2;

                    off_end = off_start + (ci->stripe_length * (ci->num_stripes - 2));

                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08x\n", Status);
                        goto end;
                    }
                }
            }
        } else { // SINGLE, DUPLICATE, RAID1
            for (j = 0; j < ci->num_stripes; j++) {
                if (cis[j].offset + ci->size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = ((superblock_addrs[i] - cis[j].offset) / c->chunk_item->stripe_length) * c->chunk_item->stripe_length;
                    off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), c->chunk_item->stripe_length);
                    
                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08x\n", Status);
                        goto end;
                    }
                }
            }
        }
        
        i++;
    }
    
    space = 0;
    
    Status = STATUS_SUCCESS;
    
end:
    while (!IsListEmpty(&stripes)) {
        LIST_ENTRY* le = RemoveHeadList(&stripes);
        superblock_stripe* ss = CONTAINING_RECORD(le, superblock_stripe, list_entry);
        
        space++;
        
        ExFreePool(ss);
    }
    
    if (NT_SUCCESS(Status))
        *size = space * ci->stripe_length;
    
    return Status;
}

static NTSTATUS load_stored_free_space_cache(device_extension* Vcb, chunk* c, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    FREE_SPACE_ITEM* fsi;
    UINT64 inode, num_sectors, num_valid_sectors, i, *generation;
    UINT8* data;
    NTSTATUS Status;
    UINT32 *checksums, crc32;
    FREE_SPACE_ENTRY* fse;
    UINT64 size, num_entries, num_bitmaps, extent_length, bmpnum, off, total_space = 0, superblock_size;
    LIST_ENTRY *le, rollback;
    
    // FIXME - does this break if Vcb->superblock.sector_size is not 4096?
    
    TRACE("(%p, %llx)\n", Vcb, c->offset);
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(tp.item->key, searchkey)) {
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
    num_entries = fsi->num_entries;
    num_bitmaps = fsi->num_bitmaps;
    
    Status = open_fcb(Vcb, Vcb->root_root, inode, BTRFS_TYPE_FILE, NULL, NULL, &c->cache, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        return STATUS_NOT_FOUND;
    }
    
    if (c->cache->inode_item.st_size == 0) {
        WARN("cache had zero length\n");
        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_NOT_FOUND;
    }
    
    c->cache->inode_item.flags |= BTRFS_INODE_NODATACOW;
    
    if (num_entries == 0 && num_bitmaps == 0)
        return STATUS_SUCCESS;
    
    size = sector_align(c->cache->inode_item.st_size, Vcb->superblock.sector_size);
    
    data = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
    
    if (!data) {
        ERR("out of memory\n");
        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_file(c->cache, data, 0, c->cache->inode_item.st_size, NULL, NULL, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("read_file returned %08x\n", Status);
        ExFreePool(data);
        
        c->cache->deleted = TRUE;
        mark_fcb_dirty(c->cache);
        
        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_NOT_FOUND;
    }
    
    if (size > c->cache->inode_item.st_size)
        RtlZeroMemory(&data[c->cache->inode_item.st_size], size - c->cache->inode_item.st_size);
    
    num_sectors = size / Vcb->superblock.sector_size;
    
    generation = (UINT64*)(data + (num_sectors * sizeof(UINT32)));
    
    if (*generation != fsi->generation) {
        WARN("free space cache generation for %llx was %llx, expected %llx\n", c->offset, *generation, fsi->generation);
        goto clearcache;
    }
    
    extent_length = (num_sectors * sizeof(UINT32)) + sizeof(UINT64) + (num_entries * sizeof(FREE_SPACE_ENTRY));
    
    num_valid_sectors = (sector_align(extent_length, Vcb->superblock.sector_size) / Vcb->superblock.sector_size) + num_bitmaps;
    
    if (num_valid_sectors > num_sectors) {
        ERR("free space cache for %llx was %llx sectors, expected at least %llx\n", c->offset, num_sectors, num_valid_sectors);
        goto clearcache;
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
            goto clearcache;
        }
    }
    
    off = (sizeof(UINT32) * num_sectors) + sizeof(UINT64);

    bmpnum = 0;
    for (i = 0; i < num_entries; i++) {
        if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
            off = sector_align(off, Vcb->superblock.sector_size);
        
        fse = (FREE_SPACE_ENTRY*)&data[off];
        
        if (fse->type == FREE_SPACE_EXTENT) {
            Status = add_space_entry(&c->space, &c->space_size, fse->offset, fse->size);
            if (!NT_SUCCESS(Status)) {
                ERR("add_space_entry returned %08x\n", Status);
                ExFreePool(data);
                return Status;
            }
            
            total_space += fse->size;
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
                load_free_space_bitmap(Vcb, c, fse->offset, &data[bmpnum * Vcb->superblock.sector_size], &total_space);
                bmpnum++;
            }
            
            off += sizeof(FREE_SPACE_ENTRY);
        }
    }
    
    // do sanity check

    Status = get_superblock_size(c, &superblock_size);
    if (!NT_SUCCESS(Status)) {
        ERR("get_superblock_size returned %08x\n", Status);
        ExFreePool(data);
        return Status;
    }
    
    if (c->chunk_item->size - c->used != total_space + superblock_size) {
        WARN("invalidating cache for chunk %llx: space was %llx, expected %llx\n", c->offset, total_space + superblock_size, c->chunk_item->size - c->used);
        goto clearcache;
    }
    
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);
        LIST_ENTRY* le2 = le->Flink;
        
        if (le2 != &c->space) {
            space* s2 = CONTAINING_RECORD(le2, space, list_entry);
            
            if (s2->address == s->address + s->size) {
                s->size += s2->size;
                
                RemoveEntryList(&s2->list_entry);
                RemoveEntryList(&s2->list_entry_size);
                ExFreePool(s2);
                
                RemoveEntryList(&s->list_entry_size);
                order_space_entry(s, &c->space_size);
                
                le2 = le;
            }
        }
        
        le = le2;
    }
    
    ExFreePool(data);
    
    return STATUS_SUCCESS;
    
clearcache:
    ExFreePool(data);
    
    InitializeListHead(&rollback);
    
    delete_tree_item(Vcb, &tp, &rollback);
    
    Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08x\n", Status);
        do_rollback(Vcb, &rollback);
        return Status;
    }
    
    clear_rollback(Vcb, &rollback);
    
    c->cache->deleted = TRUE;
    mark_fcb_dirty(c->cache);
    
    free_fcb(c->cache);
    c->cache = NULL;
    return STATUS_NOT_FOUND;
}

NTSTATUS load_free_space_cache(device_extension* Vcb, chunk* c, PIRP Irp) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    UINT64 lastaddr;
    BOOL b;
    space* s;
    NTSTATUS Status;
//     LIST_ENTRY* le;
    
    if (Vcb->superblock.generation - 1 == Vcb->superblock.cache_generation) {
        Status = load_stored_free_space_cache(Vcb, c, Irp);
        
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
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
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
                    
                    s->address = lastaddr;
                    s->size = tp.item->key.obj_id - lastaddr;
                    InsertTailList(&c->space, &s->list_entry);
                    
                    order_space_entry(s, &c->space_size);
                    
                    TRACE("(%llx,%llx)\n", s->address, s->size);
                }
                
                if (tp.item->key.obj_type == TYPE_METADATA_ITEM)
                    lastaddr = tp.item->key.obj_id + Vcb->superblock.node_size;
                else
                    lastaddr = tp.item->key.obj_id + tp.item->key.offset;
            }
            
            b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
            if (b)
                tp = next_tp;
        } while (b);
        
        if (lastaddr < c->offset + c->chunk_item->size) {
            s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
            
            if (!s) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            s->address = lastaddr;
            s->size = c->offset + c->chunk_item->size - lastaddr;
            InsertTailList(&c->space, &s->list_entry);
            
            order_space_entry(s, &c->space_size);
            
            TRACE("(%llx,%llx)\n", s->address, s->size);
        }
    }
    
//     le = c->space_size.Flink;
//     while (le != &c->space_size) {
//         space* s = CONTAINING_RECORD(le, space, list_entry_size);
//         
//         ERR("(%llx, %llx)\n", s->address, s->size);
//         
//         le = le->Flink;
//     }
//     ERR("---\n");

    return STATUS_SUCCESS;
}

static NTSTATUS insert_cache_extent(fcb* fcb, UINT64 start, UINT64 length, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = fcb->Vcb->chunks.Flink;
    chunk* c;
    UINT64 flags;
    
    flags = fcb->Vcb->data_flags;
    
    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, TRUE);
    
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (!c->readonly && !c->reloc) {
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, FALSE, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, length)) {
                    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
                    return STATUS_SUCCESS;
                }
            }
            
            ExReleaseResourceLite(&c->lock);
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, TRUE);
    
    if ((c = alloc_chunk(fcb->Vcb, flags))) {
        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, FALSE, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, length))
                return STATUS_SUCCESS;
        }
        
        ExReleaseResourceLite(&c->lock);
    } else   
        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
    
    WARN("couldn't find any data chunks with %llx bytes free\n", length);

    return STATUS_DISK_FULL;
}

static NTSTATUS allocate_cache_chunk(device_extension* Vcb, chunk* c, BOOL* changed, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    UINT64 num_entries, new_cache_size, i;
    UINT32 num_sectors;
    BOOL realloc_extents = FALSE;
    
    // FIXME - also do bitmaps
    // FIXME - make sure this works when sector_size is not 4096
    
    *changed = FALSE;
    
    num_entries = 0;
    
    // num_entries is the number of entries in c->space and c->deleting - it might
    // be slightly higher then what we end up writing, but doing it this way is much
    // quicker and simpler.
    if (!IsListEmpty(&c->space)) {
        le = c->space.Flink;
        while (le != &c->space) {
            num_entries++;

            le = le->Flink;
        }
    }
    
    if (!IsListEmpty(&c->deleting)) {
        le = c->deleting.Flink;
        while (le != &c->deleting) {
            num_entries++;

            le = le->Flink;
        }
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
    
    TRACE("chunk %llx: cache_size = %llx, new_cache_size = %llx\n", c->offset, c->cache ? c->cache->inode_item.st_size : 0, new_cache_size);
    
    if (c->cache) {
        if (new_cache_size > c->cache->inode_item.st_size)
            realloc_extents = TRUE;
        else {
            le = c->cache->extents.Flink;
            
            while (le != &c->cache->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);
                
                if (!ext->ignore && (ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ext->data->data[0];
                    
                    if (ed2->size != 0) {
                        chunk* c2 = get_chunk_from_address(Vcb, ed2->address);
                        
                        if (c2 && (c2->readonly || c2->reloc)) {
                            realloc_extents = TRUE;
                            break;
                        }
                    }
                }
                
                le = le->Flink;
            }
        }
    }
    
    if (!c->cache) {
        FREE_SPACE_ITEM* fsi;
        KEY searchkey;
        traverse_ptr tp;
        
        // create new inode
        
        c->cache = create_fcb(PagedPool);
        if (!c->cache) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
            
        c->cache->Vcb = Vcb;
        
        c->cache->inode_item.st_size = new_cache_size;
        c->cache->inode_item.st_blocks = new_cache_size;
        c->cache->inode_item.st_nlink = 1;
        c->cache->inode_item.st_mode = S_IRUSR | S_IWUSR | __S_IFREG;
        c->cache->inode_item.flags = BTRFS_INODE_NODATASUM | BTRFS_INODE_NODATACOW | BTRFS_INODE_NOCOMPRESS | BTRFS_INODE_PREALLOC;
        
        c->cache->Header.IsFastIoPossible = fast_io_possible(c->cache);
        c->cache->Header.AllocationSize.QuadPart = 0;
        c->cache->Header.FileSize.QuadPart = 0;
        c->cache->Header.ValidDataLength.QuadPart = 0;
        
        c->cache->subvol = Vcb->root_root;
        
        c->cache->inode = InterlockedIncrement64(&Vcb->root_root->lastinode);
        
        c->cache->type = BTRFS_TYPE_FILE;
        c->cache->created = TRUE;
        
        // create new free space entry
        
        fsi = ExAllocatePoolWithTag(PagedPool, sizeof(FREE_SPACE_ITEM), ALLOC_TAG);
        if (!fsi) {
            ERR("out of memory\n");
            free_fcb(c->cache);
            c->cache = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            ExFreePool(fsi);
            free_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }
        
        if (!keycmp(searchkey, tp.item->key))
            delete_tree_item(Vcb, &tp, rollback);
        
        fsi->key.obj_id = c->cache->inode;
        fsi->key.obj_type = TYPE_INODE_ITEM;
        fsi->key.offset = 0;
        
        if (!insert_tree_item(Vcb, Vcb->root_root, FREE_SPACE_CACHE_ID, 0, c->offset, fsi, sizeof(FREE_SPACE_ITEM), NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            free_fcb(c->cache);
            c->cache = NULL;
            return STATUS_INTERNAL_ERROR;
        }
        
        // allocate space
        
        Status = insert_cache_extent(c->cache, 0, new_cache_size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_cache_extent returned %08x\n", Status);
            free_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }
        
        c->cache->extents_changed = TRUE;
        InsertTailList(&Vcb->all_fcbs, &c->cache->list_entry_all);
        
        flush_fcb(c->cache, TRUE, batchlist, Irp, rollback);
        
        *changed = TRUE;
    } else if (realloc_extents) {
        KEY searchkey;
        traverse_ptr tp;
        
        TRACE("reallocating extents\n");
        
        // add free_space entry to tree cache
        
        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(searchkey, tp.item->key)) {
            ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }
        
        tp.tree->write = TRUE;

        // remove existing extents
        
        if (c->cache->inode_item.st_size > 0) {
            le = c->cache->extents.Flink;
            
            while (le != &c->cache->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);
                
                if (!ext->ignore && (ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ext->data->data[0];
                    
                    if (ed2->size != 0) {
                        chunk* c2 = get_chunk_from_address(Vcb, ed2->address);
                        
                        if (!c2->list_entry_changed.Flink)
                            InsertTailList(&Vcb->chunks_changed, &c2->list_entry_changed);
                    }
                }
                
                le = le->Flink;
            }
            
            Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("excise_extents returned %08x\n", Status);
                return Status;
            }
        }
        
        // add new extent
        
        Status = insert_cache_extent(c->cache, 0, new_cache_size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_cache_extent returned %08x\n", Status);
            return Status;
        }
        
        // modify INODE_ITEM
        
        c->cache->inode_item.st_size = new_cache_size;
        c->cache->inode_item.st_blocks = new_cache_size;
        
        flush_fcb(c->cache, TRUE, batchlist, Irp, rollback);
    
        *changed = TRUE;
    } else {
        KEY searchkey;
        traverse_ptr tp;
        
        // add INODE_ITEM and free_space entry to tree cache, for writing later
        
        searchkey.obj_id = c->cache->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(searchkey, tp.item->key)) {
            INODE_ITEM* ii;
            
            ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
            RtlCopyMemory(ii, &c->cache->inode_item, sizeof(INODE_ITEM));
            
            if (!insert_tree_item(Vcb, Vcb->root_root, c->cache->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            *changed = TRUE;
        } else {        
            if (tp.item->size < sizeof(INODE_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
                return STATUS_INTERNAL_ERROR;
            }
            
            tp.tree->write = TRUE;
        }

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(searchkey, tp.item->key)) {
            ERR("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            int3;
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }
        
        tp.tree->write = TRUE;
    }
    
    // FIXME - reduce inode allocation if cache is shrinking. Make sure to avoid infinite write loops
    
    return STATUS_SUCCESS;
}

NTSTATUS allocate_cache(device_extension* Vcb, BOOL* changed, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks_changed.Flink, batchlist;
    NTSTATUS Status;

    *changed = FALSE;
    
    InitializeListHead(&batchlist);
    
    while (le != &Vcb->chunks_changed) {
        BOOL b;
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_changed);

        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        Status = allocate_cache_chunk(Vcb, c, &b, &batchlist, Irp, rollback);
        ExReleaseResourceLite(&c->lock);
        
        if (b)
            *changed = TRUE;
        
        if (!NT_SUCCESS(Status)) {
            ERR("allocate_cache_chunk(%llx) returned %08x\n", c->offset, Status);
            clear_batch_list(Vcb, &batchlist);
            return Status;
        }
        
        le = le->Flink;
    }
    
    commit_batch_list(Vcb, &batchlist, Irp, rollback);
    
    return STATUS_SUCCESS;
}

static void add_rollback_space(device_extension* Vcb, LIST_ENTRY* rollback, BOOL add, LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 address, UINT64 length, chunk* c) {
    rollback_space* rs;
    
    rs = ExAllocatePoolWithTag(PagedPool, sizeof(rollback_space), ALLOC_TAG);
    if (!rs) {
        ERR("out of memory\n");
        return;
    }
    
    rs->list = list;
    rs->list_size = list_size;
    rs->address = address;
    rs->length = length;
    rs->chunk = c;
    
    add_rollback(Vcb, rollback, add ? ROLLBACK_ADD_SPACE : ROLLBACK_SUBTRACT_SPACE, rs);
}

void _space_list_add2(device_extension* Vcb, LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 address, UINT64 length, chunk* c, LIST_ENTRY* rollback, const char* func) {
    LIST_ENTRY* le;
    space *s, *s2;
    
#ifdef DEBUG_SPACE_LISTS
    _debug_message(func, "called space_list_add (%p, %llx, %llx, %p)\n", list, address, length, rollback);
#endif
    
    if (IsListEmpty(list)) {
        s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

        if (!s) {
            ERR("out of memory\n");
            return;
        }
        
        s->address = address;
        s->size = length;
        InsertTailList(list, &s->list_entry);
        
        if (list_size)
            InsertTailList(list_size, &s->list_entry_size);
        
        if (rollback)
            add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, length, c);
        
        return;
    }
    
    le = list->Flink;
    while (le != list) {
        s2 = CONTAINING_RECORD(le, space, list_entry);
        
        // old entry envelops new one completely
        if (s2->address <= address && s2->address + s2->size >= address + length)
            return;
        
        // new entry envelops old one completely
        if (address <= s2->address && address + length >= s2->address + s2->size) {
            if (address < s2->address) {
                if (rollback)
                    add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, s2->address - address, c);
                
                s2->size += s2->address - address;
                s2->address = address;
                
                while (s2->list_entry.Blink != list) {
                    space* s3 = CONTAINING_RECORD(s2->list_entry.Blink, space, list_entry);
                    
                    if (s3->address + s3->size == s2->address) {
                        s2->address = s3->address;
                        s2->size += s3->size;
                        
                        RemoveEntryList(&s3->list_entry);
                        
                        if (list_size)
                            RemoveEntryList(&s3->list_entry_size);
                        
                        ExFreePool(s3);
                    } else
                        break;
                }
            }
            
            if (length > s2->size) {
                if (rollback)
                    add_rollback_space(Vcb, rollback, TRUE, list, list_size, s2->address + s2->size, address + length - s2->address - s2->size, c);
                
                s2->size = length;
                
                while (s2->list_entry.Flink != list) {
                    space* s3 = CONTAINING_RECORD(s2->list_entry.Flink, space, list_entry);
                    
                    if (s3->address <= s2->address + s2->size) {
                        s2->size = max(s2->size, s3->address + s3->size - s2->address);
                        
                        RemoveEntryList(&s3->list_entry);
                        
                        if (list_size)
                            RemoveEntryList(&s3->list_entry_size);
                        
                        ExFreePool(s3);
                    } else
                        break;
                }
            }
            
            if (list_size) {
                RemoveEntryList(&s2->list_entry_size);
                order_space_entry(s2, list_size);
            }
            
            return;
        }
        
        // new entry overlaps start of old one
        if (address < s2->address && address + length >= s2->address) {
            if (rollback)
                add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, s2->address - address, c);
            
            s2->size += s2->address - address;
            s2->address = address;
            
            while (s2->list_entry.Blink != list) {
                space* s3 = CONTAINING_RECORD(s2->list_entry.Blink, space, list_entry);
                
                if (s3->address + s3->size == s2->address) {
                    s2->address = s3->address;
                    s2->size += s3->size;
                    
                    RemoveEntryList(&s3->list_entry);
                    
                    if (list_size)
                        RemoveEntryList(&s3->list_entry_size);
                    
                    ExFreePool(s3);
                } else
                    break;
            }
            
            if (list_size) {
                RemoveEntryList(&s2->list_entry_size);
                order_space_entry(s2, list_size);
            }
            
            return;
        }
        
        // new entry overlaps end of old one
        if (address <= s2->address + s2->size && address + length > s2->address + s2->size) {
            if (rollback)
                add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, s2->address + s2->size - address, c);
            
            s2->size = address + length - s2->address;
            
            while (s2->list_entry.Flink != list) {
                space* s3 = CONTAINING_RECORD(s2->list_entry.Flink, space, list_entry);
                
                if (s3->address <= s2->address + s2->size) {
                    s2->size = max(s2->size, s3->address + s3->size - s2->address);
                    
                    RemoveEntryList(&s3->list_entry);
                    
                    if (list_size)
                        RemoveEntryList(&s3->list_entry_size);
                    
                    ExFreePool(s3);
                } else
                    break;
            }
            
            if (list_size) {
                RemoveEntryList(&s2->list_entry_size);
                order_space_entry(s2, list_size);
            }
            
            return;
        }
        
        // add completely separate entry
        if (s2->address > address + length) {
            s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

            if (!s) {
                ERR("out of memory\n");
                return;
            }
            
            if (rollback)
                add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, length, c);
            
            s->address = address;
            s->size = length;
            InsertHeadList(s2->list_entry.Blink, &s->list_entry);
            
            if (list_size)
                order_space_entry(s, list_size);
            
            return;
        }
        
        le = le->Flink;
    }
    
    // check if contiguous with last entry
    if (s2->address + s2->size == address) {
        s2->size += length;
        
        if (list_size) {
            RemoveEntryList(&s2->list_entry_size);
            order_space_entry(s2, list_size);
        }
        
        return;
    }
    
    // otherwise, insert at end
    s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

    if (!s) {
        ERR("out of memory\n");
        return;
    }
    
    s->address = address;
    s->size = length;
    InsertTailList(list, &s->list_entry);
    
    if (list_size)
        order_space_entry(s, list_size);
    
    if (rollback)
        add_rollback_space(Vcb, rollback, TRUE, list, list_size, address, length, c);
}

static void space_list_merge(device_extension* Vcb, LIST_ENTRY* spacelist, LIST_ENTRY* spacelist_size, LIST_ENTRY* deleting) {
    LIST_ENTRY* le;
    
    if (!IsListEmpty(deleting)) {
        le = deleting->Flink;
        while (le != deleting) {
            space* s = CONTAINING_RECORD(le, space, list_entry);
            
            space_list_add2(Vcb, spacelist, spacelist_size, s->address, s->size, NULL);
            
            le = le->Flink;
        }
    }
}

static NTSTATUS update_chunk_cache(device_extension* Vcb, chunk* c, BTRFS_TIME* now, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    FREE_SPACE_ITEM* fsi;
    void* data;
    FREE_SPACE_ENTRY* fse;
    UINT64 num_entries, num_sectors, *cachegen, i, off;
    UINT32* checksums;
    LIST_ENTRY* le;
    
    space_list_merge(Vcb, &c->space, &c->space_size, &c->deleting);
    
    data = ExAllocatePoolWithTag(NonPagedPool, c->cache->inode_item.st_size, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(data, c->cache->inode_item.st_size);
    
    num_entries = 0;
    num_sectors = c->cache->inode_item.st_size / Vcb->superblock.sector_size;
    off = (sizeof(UINT32) * num_sectors) + sizeof(UINT64);
    
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if ((off + sizeof(FREE_SPACE_ENTRY)) / Vcb->superblock.sector_size != off / Vcb->superblock.sector_size)
            off = sector_align(off, Vcb->superblock.sector_size);
        
        fse = (FREE_SPACE_ENTRY*)((UINT8*)data + off);
        
        fse->offset = s->address;
        fse->size = s->size;
        fse->type = FREE_SPACE_EXTENT;
        num_entries++;
        
        off += sizeof(FREE_SPACE_ENTRY);
        
        le = le->Flink;
    }

    // update INODE_ITEM
    
    c->cache->inode_item.generation = Vcb->superblock.generation;
    c->cache->inode_item.transid = Vcb->superblock.generation;
    c->cache->inode_item.sequence++;
    c->cache->inode_item.st_ctime = *now;
    
    flush_fcb(c->cache, TRUE, batchlist, Irp, rollback);
    
    // update free_space item
    
    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(searchkey, tp.item->key)) {
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
    
    Status = do_write_file(c->cache, 0, c->cache->inode_item.st_size, data, NULL, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("do_write_file returned %08x\n", Status);
        return Status;
    }

    ExFreePool(data);
    
    return STATUS_SUCCESS;
}

NTSTATUS update_chunk_caches(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks_changed.Flink, batchlist;
    NTSTATUS Status;
    chunk* c;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    InitializeListHead(&batchlist);
    
    while (le != &Vcb->chunks_changed) {
        c = CONTAINING_RECORD(le, chunk, list_entry_changed);
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        Status = update_chunk_cache(Vcb, c, &now, &batchlist, Irp, rollback);
        ExReleaseResourceLite(&c->lock);

        if (!NT_SUCCESS(Status)) {
            ERR("update_chunk_cache(%llx) returned %08x\n", c->offset, Status);
            clear_batch_list(Vcb, &batchlist);
            return Status;
        }
        
        le = le->Flink;
    }
    
    commit_batch_list(Vcb, &batchlist, Irp, rollback);
    
    return STATUS_SUCCESS;
}

void _space_list_add(device_extension* Vcb, chunk* c, BOOL deleting, UINT64 address, UINT64 length, LIST_ENTRY* rollback, const char* func) {
    LIST_ENTRY* list;
    
    TRACE("(%p, %p, %u, %llx, %llx, %p)\n", Vcb, c, deleting, address, length, rollback);
    
    list = deleting ? &c->deleting : &c->space;
    
    if (!c->list_entry_changed.Flink)
        InsertTailList(&Vcb->chunks_changed, &c->list_entry_changed);
    
    _space_list_add2(Vcb, list, deleting ? NULL : &c->space_size, address, length, c, rollback, func);
}

void _space_list_subtract2(device_extension* Vcb, LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 address, UINT64 length, chunk* c, LIST_ENTRY* rollback, const char* func) {
    LIST_ENTRY *le, *le2;
    space *s, *s2;
    
#ifdef DEBUG_SPACE_LISTS
    _debug_message(func, "called space_list_subtract (%p, %llx, %llx, %p)\n", list, address, length, rollback);
#endif
    
    if (IsListEmpty(list))
        return;
    
    le = list->Flink;
    while (le != list) {
        s2 = CONTAINING_RECORD(le, space, list_entry);
        le2 = le->Flink;
        
        if (s2->address >= address + length)
            return;
        
        if (s2->address >= address && s2->address + s2->size <= address + length) { // remove entry entirely
            if (rollback)
                add_rollback_space(Vcb, rollback, FALSE, list, list_size, s2->address, s2->size, c);
            
            RemoveEntryList(&s2->list_entry);
            
            if (list_size)
                RemoveEntryList(&s2->list_entry_size);
            
            ExFreePool(s2);
        } else if (address + length > s2->address && address + length < s2->address + s2->size) {
            if (address > s2->address) { // cut out hole
                if (rollback)
                    add_rollback_space(Vcb, rollback, FALSE, list, list_size, address, length, c);
                
                s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);

                if (!s) {
                    ERR("out of memory\n");
                    return;
                }
                
                s->address = s2->address;
                s->size = address - s2->address;
                InsertHeadList(s2->list_entry.Blink, &s->list_entry);
                
                s2->size = s2->address + s2->size - address - length;
                s2->address = address + length;
                
                if (list_size) {
                    RemoveEntryList(&s2->list_entry_size);
                    order_space_entry(s2, list_size);
                    order_space_entry(s, list_size);
                }
                
                return;
            } else { // remove start of entry
                if (rollback)
                    add_rollback_space(Vcb, rollback, FALSE, list, list_size, s2->address, address + length - s2->address, c);
                
                s2->size -= address + length - s2->address;
                s2->address = address + length;
                
                if (list_size) {
                    RemoveEntryList(&s2->list_entry_size);
                    order_space_entry(s2, list_size);
                }
            }
        } else if (address > s2->address && address < s2->address + s2->size) { // remove end of entry
            if (rollback)
                add_rollback_space(Vcb, rollback, FALSE, list, list_size, address, s2->address + s2->size - address, c);
            
            s2->size = address - s2->address;
            
            if (list_size) {
                RemoveEntryList(&s2->list_entry_size);
                order_space_entry(s2, list_size);
            }
        }
        
        le = le2;
    }
}

void _space_list_subtract(device_extension* Vcb, chunk* c, BOOL deleting, UINT64 address, UINT64 length, LIST_ENTRY* rollback, const char* func) {
    LIST_ENTRY* list;
    
    list = deleting ? &c->deleting : &c->space;
    
    if (!c->list_entry_changed.Flink)
        InsertTailList(&Vcb->chunks_changed, &c->list_entry_changed);
    
    _space_list_subtract2(Vcb, list, deleting ? NULL : &c->space_size, address, length, c, rollback, func);
}
