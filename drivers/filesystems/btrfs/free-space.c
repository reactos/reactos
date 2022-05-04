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

// Number of increments in the size of each cache inode, in sectors. Should
// this be a constant number of sectors, a constant 256 KB, or what?
#define CACHE_INCREMENTS    64

static NTSTATUS remove_free_space_inode(device_extension* Vcb, uint64_t inode, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb* fcb;

    Status = open_fcb(Vcb, Vcb->root_root, inode, BTRFS_TYPE_FILE, NULL, false, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08lx\n", Status);
        return Status;
    }

    mark_fcb_dirty(fcb);

    if (fcb->inode_item.st_size > 0) {
        Status = excise_extents(fcb->Vcb, fcb, 0, sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            return Status;
        }
    }

    fcb->deleted = true;

    Status = flush_fcb(fcb, false, batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("flush_fcb returned %08lx\n", Status);
        free_fcb(fcb);
        return Status;
    }

    free_fcb(fcb);

    return STATUS_SUCCESS;
}

NTSTATUS clear_free_space_cache(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    bool b;
    LIST_ENTRY rollback;

    InitializeListHead(&rollback);

    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = 0;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    do {
        if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
            break;

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            if (tp.item->size >= sizeof(FREE_SPACE_ITEM)) {
                FREE_SPACE_ITEM* fsi = (FREE_SPACE_ITEM*)tp.item->data;

                if (fsi->key.obj_type != TYPE_INODE_ITEM)
                    WARN("key (%I64x,%x,%I64x) does not point to an INODE_ITEM\n", fsi->key.obj_id, fsi->key.obj_type, fsi->key.offset);
                else {
                    LIST_ENTRY* le;

                    Status = remove_free_space_inode(Vcb, fsi->key.obj_id, batchlist, Irp, &rollback);

                    if (!NT_SUCCESS(Status))
                        ERR("remove_free_space_inode for (%I64x,%x,%I64x) returned %08lx\n", fsi->key.obj_id, fsi->key.obj_type, fsi->key.offset, Status);

                    le = Vcb->chunks.Flink;
                    while (le != &Vcb->chunks) {
                        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

                        if (c->offset == tp.item->key.offset && c->cache) {
                            reap_fcb(c->cache);
                            c->cache = NULL;
                        }

                        le = le->Flink;
                    }
                }
            } else
                WARN("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        }

        b = find_next_item(Vcb, &tp, &next_tp, false, Irp);
        if (b)
            tp = next_tp;
    } while (b);

    Status = STATUS_SUCCESS;

    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    if (Vcb->space_root) {
        searchkey.obj_id = 0;
        searchkey.obj_type = 0;
        searchkey.offset = 0;

        Status = find_item(Vcb, Vcb->space_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08lx\n", Status);
            return Status;
        }

        do {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            b = find_next_item(Vcb, &tp, &next_tp, false, Irp);
            if (b)
                tp = next_tp;
        } while (b);
    }

    // regenerate free space tree
    if (Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE) {
        LIST_ENTRY* le;

        ExAcquireResourceSharedLite(&Vcb->chunk_lock, true);

        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

            if (!c->cache_loaded) {
                acquire_chunk_lock(c, Vcb);

                Status = load_cache_chunk(Vcb, c, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("load_cache_chunk(%I64x) returned %08lx\n", c->offset, Status);
                    release_chunk_lock(c, Vcb);
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    return Status;
                }

                c->changed = true;
                c->space_changed = true;

                release_chunk_lock(c, Vcb);
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->chunk_lock);
    }

    return Status;
}

NTSTATUS add_space_entry(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t offset, uint64_t size) {
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

static void load_free_space_bitmap(device_extension* Vcb, chunk* c, uint64_t offset, void* data, uint64_t* total_space) {
    RTL_BITMAP bmph;
    uint32_t i, len, *dwords = data;
    ULONG runlength, index;

    // flip bits
    for (i = 0; i < Vcb->superblock.sector_size / sizeof(uint32_t); i++) {
        dwords[i] = ~dwords[i];
    }

    len = Vcb->superblock.sector_size * 8;

    RtlInitializeBitMap(&bmph, data, len);

    index = 0;
    runlength = RtlFindFirstRunClear(&bmph, &index);

    while (runlength != 0) {
        uint64_t addr, length;

        if (index >= len)
            break;

        if (index + runlength >= len) {
            runlength = len - index;

            if (runlength == 0)
                break;
        }

        addr = offset + (index << Vcb->sector_shift);
        length = runlength << Vcb->sector_shift;

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
    uint64_t stripe;
    LIST_ENTRY list_entry;
} superblock_stripe;

static NTSTATUS add_superblock_stripe(LIST_ENTRY* stripes, uint64_t off, uint64_t len) {
    uint64_t i;

    for (i = 0; i < len; i++) {
        LIST_ENTRY* le;
        superblock_stripe* ss;
        bool ignore = false;

        le = stripes->Flink;
        while (le != stripes) {
            ss = CONTAINING_RECORD(le, superblock_stripe, list_entry);

            if (ss->stripe == off + i) {
                ignore = true;
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

static NTSTATUS get_superblock_size(chunk* c, uint64_t* size) {
    NTSTATUS Status;
    CHUNK_ITEM* ci = c->chunk_item;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    uint64_t off_start, off_end, space = 0;
    uint16_t i = 0, j;
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
                        ERR("add_superblock_stripe returned %08lx\n", Status);
                        goto end;
                    }
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID5) {
            for (j = 0; j < ci->num_stripes; j++) {
                uint64_t stripe_size = ci->size / (ci->num_stripes - 1);

                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % (ci->stripe_length * (ci->num_stripes - 1));
                    off_start *= ci->num_stripes - 1;

                    off_end = off_start + (ci->stripe_length * (ci->num_stripes - 1));

                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08lx\n", Status);
                        goto end;
                    }
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID6) {
            for (j = 0; j < ci->num_stripes; j++) {
                uint64_t stripe_size = ci->size / (ci->num_stripes - 2);

                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % (ci->stripe_length * (ci->num_stripes - 2));
                    off_start *= ci->num_stripes - 2;

                    off_end = off_start + (ci->stripe_length * (ci->num_stripes - 2));

                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08lx\n", Status);
                        goto end;
                    }
                }
            }
        } else { // SINGLE, DUPLICATE, RAID1, RAID1C3, RAID1C4
            for (j = 0; j < ci->num_stripes; j++) {
                if (cis[j].offset + ci->size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    off_start = ((superblock_addrs[i] - cis[j].offset) / c->chunk_item->stripe_length) * c->chunk_item->stripe_length;
                    off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), c->chunk_item->stripe_length);

                    Status = add_superblock_stripe(&stripes, off_start / ci->stripe_length, (off_end - off_start) / ci->stripe_length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_superblock_stripe returned %08lx\n", Status);
                        goto end;
                    }
                }
            }
        }

        i++;
    }

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

NTSTATUS load_stored_free_space_cache(device_extension* Vcb, chunk* c, bool load_only, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    FREE_SPACE_ITEM* fsi;
    uint64_t inode, *generation;
    uint8_t* data;
    NTSTATUS Status;
    uint32_t *checksums, crc32, num_sectors, num_valid_sectors, size;
    FREE_SPACE_ENTRY* fse;
    uint64_t num_entries, num_bitmaps, extent_length, bmpnum, off, total_space = 0, superblock_size;
    LIST_ENTRY *le, rollback;

    // FIXME - does this break if Vcb->superblock.sector_size is not 4096?

    TRACE("(%p, %I64x)\n", Vcb, c->offset);

    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        TRACE("(%I64x,%x,%I64x) not found\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_NOT_FOUND;
    }

    if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
        WARN("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
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

    Status = open_fcb(Vcb, Vcb->root_root, inode, BTRFS_TYPE_FILE, NULL, false, NULL, &c->cache, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08lx\n", Status);
        return STATUS_NOT_FOUND;
    }

    if (load_only)
        return STATUS_SUCCESS;

    if (c->cache->inode_item.st_size == 0) {
        WARN("cache had zero length\n");
        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_NOT_FOUND;
    }

    c->cache->inode_item.flags |= BTRFS_INODE_NODATACOW;

    if (num_entries == 0 && num_bitmaps == 0)
        return STATUS_SUCCESS;

    size = (uint32_t)sector_align(c->cache->inode_item.st_size, Vcb->superblock.sector_size);

    data = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);

    if (!data) {
        ERR("out of memory\n");
        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (c->chunk_item->size < 0x6400000) { // 100 MB
        WARN("deleting free space cache for chunk smaller than 100MB\n");
        goto clearcache;
    }

    Status = read_file(c->cache, data, 0, c->cache->inode_item.st_size, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("read_file returned %08lx\n", Status);
        ExFreePool(data);

        c->cache->deleted = true;
        mark_fcb_dirty(c->cache);

        free_fcb(c->cache);
        c->cache = NULL;
        return STATUS_NOT_FOUND;
    }

    if (size > c->cache->inode_item.st_size)
        RtlZeroMemory(&data[c->cache->inode_item.st_size], (ULONG)(size - c->cache->inode_item.st_size));

    num_sectors = size >> Vcb->sector_shift;

    generation = (uint64_t*)(data + (num_sectors * sizeof(uint32_t)));

    if (*generation != fsi->generation) {
        WARN("free space cache generation for %I64x was %I64x, expected %I64x\n", c->offset, *generation, fsi->generation);
        goto clearcache;
    }

    extent_length = (num_sectors * sizeof(uint32_t)) + sizeof(uint64_t) + (num_entries * sizeof(FREE_SPACE_ENTRY));

    num_valid_sectors = (ULONG)((sector_align(extent_length, Vcb->superblock.sector_size) >> Vcb->sector_shift) + num_bitmaps);

    if (num_valid_sectors > num_sectors) {
        ERR("free space cache for %I64x was %u sectors, expected at least %u\n", c->offset, num_sectors, num_valid_sectors);
        goto clearcache;
    }

    checksums = (uint32_t*)data;

    for (uint32_t i = 0; i < num_valid_sectors; i++) {
        if (i << Vcb->sector_shift > sizeof(uint32_t) * num_sectors)
            crc32 = ~calc_crc32c(0xffffffff, &data[i << Vcb->sector_shift], Vcb->superblock.sector_size);
        else if ((i + 1) << Vcb->sector_shift < sizeof(uint32_t) * num_sectors)
            crc32 = 0; // FIXME - test this
        else
            crc32 = ~calc_crc32c(0xffffffff, &data[sizeof(uint32_t) * num_sectors], ((i + 1) << Vcb->sector_shift) - (sizeof(uint32_t) * num_sectors));

        if (crc32 != checksums[i]) {
            WARN("checksum %u was %08x, expected %08x\n", i, crc32, checksums[i]);
            goto clearcache;
        }
    }

    off = (sizeof(uint32_t) * num_sectors) + sizeof(uint64_t);

    bmpnum = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        if ((off + sizeof(FREE_SPACE_ENTRY)) >> Vcb->sector_shift != off >> Vcb->sector_shift)
            off = sector_align(off, Vcb->superblock.sector_size);

        fse = (FREE_SPACE_ENTRY*)&data[off];

        if (fse->type == FREE_SPACE_EXTENT) {
            Status = add_space_entry(&c->space, &c->space_size, fse->offset, fse->size);
            if (!NT_SUCCESS(Status)) {
                ERR("add_space_entry returned %08lx\n", Status);
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
        bmpnum = sector_align(off, Vcb->superblock.sector_size) >> Vcb->sector_shift;
        off = (sizeof(uint32_t) * num_sectors) + sizeof(uint64_t);

        for (uint32_t i = 0; i < num_entries; i++) {
            if ((off + sizeof(FREE_SPACE_ENTRY)) >> Vcb->sector_shift != off >> Vcb->sector_shift)
                off = sector_align(off, Vcb->superblock.sector_size);

            fse = (FREE_SPACE_ENTRY*)&data[off];

            if (fse->type == FREE_SPACE_BITMAP) {
                // FIXME - make sure we don't overflow the buffer here
                load_free_space_bitmap(Vcb, c, fse->offset, &data[bmpnum << Vcb->sector_shift], &total_space);
                bmpnum++;
            }

            off += sizeof(FREE_SPACE_ENTRY);
        }
    }

    // do sanity check

    Status = get_superblock_size(c, &superblock_size);
    if (!NT_SUCCESS(Status)) {
        ERR("get_superblock_size returned %08lx\n", Status);
        ExFreePool(data);
        return Status;
    }

    if (c->chunk_item->size - c->used != total_space + superblock_size) {
        WARN("invalidating cache for chunk %I64x: space was %I64x, expected %I64x\n", c->offset, total_space + superblock_size, c->chunk_item->size - c->used);
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

    Status = delete_tree_item(Vcb, &tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08lx\n", Status);
        return Status;
    }

    Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08lx\n", Status);
        do_rollback(Vcb, &rollback);
        return Status;
    }

    clear_rollback(&rollback);

    c->cache->deleted = true;
    mark_fcb_dirty(c->cache);

    c->old_cache = c->cache;
    c->cache = NULL;

    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);
        LIST_ENTRY* le2 = le->Flink;

        RemoveEntryList(&s->list_entry);
        RemoveEntryList(&s->list_entry_size);
        ExFreePool(s);

        le = le2;
    }

    return STATUS_NOT_FOUND;
}

static NTSTATUS load_stored_free_space_tree(device_extension* Vcb, chunk* c, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    ULONG* bmparr = NULL;
    ULONG bmplen = 0;
    LIST_ENTRY* le;

    TRACE("(%p, %I64x)\n", Vcb, c->offset);

    if (!Vcb->space_root)
        return STATUS_NOT_FOUND;

    searchkey.obj_id = c->offset;
    searchkey.obj_type = TYPE_FREE_SPACE_INFO;
    searchkey.offset = c->chunk_item->size;

    Status = find_item(Vcb, Vcb->space_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        TRACE("(%I64x,%x,%I64x) not found\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return STATUS_NOT_FOUND;
    }

    if (tp.item->size < sizeof(FREE_SPACE_INFO)) {
        WARN("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_INFO));
        return STATUS_NOT_FOUND;
    }

    while (find_next_item(Vcb, &tp, &next_tp, false, Irp)) {
        tp = next_tp;

        if (tp.item->key.obj_id >= c->offset + c->chunk_item->size)
            break;

        if (tp.item->key.obj_type == TYPE_FREE_SPACE_EXTENT) {
            Status = add_space_entry(&c->space, &c->space_size, tp.item->key.obj_id, tp.item->key.offset);
            if (!NT_SUCCESS(Status)) {
                ERR("add_space_entry returned %08lx\n", Status);
                if (bmparr) ExFreePool(bmparr);
                return Status;
            }
        } else if (tp.item->key.obj_type == TYPE_FREE_SPACE_BITMAP) {
            ULONG explen, index, runlength;
            RTL_BITMAP bmp;
            uint64_t lastoff;
            ULONG bmpl;

            explen = (ULONG)(tp.item->key.offset >> Vcb->sector_shift) / 8;

            if (tp.item->size < explen) {
                WARN("(%I64x,%x,%I64x) was %u bytes, expected %lu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, explen);
                return STATUS_NOT_FOUND;
            } else if (tp.item->size == 0) {
                WARN("(%I64x,%x,%I64x) has size of 0\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                return STATUS_NOT_FOUND;
            }

            if (bmplen < tp.item->size) {
                if (bmparr)
                    ExFreePool(bmparr);

                bmplen = (ULONG)sector_align(tp.item->size, sizeof(ULONG));
                bmparr = ExAllocatePoolWithTag(PagedPool, bmplen, ALLOC_TAG);
                if (!bmparr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            // We copy the bitmap because it supposedly has to be ULONG-aligned
            RtlCopyMemory(bmparr, tp.item->data, tp.item->size);

            bmpl = (ULONG)tp.item->key.offset >> Vcb->sector_shift;

            RtlInitializeBitMap(&bmp, bmparr, bmpl);

            lastoff = tp.item->key.obj_id;

            runlength = RtlFindFirstRunClear(&bmp, &index);

            while (runlength != 0) {
                uint64_t runstart, runend;

                if (index >= bmpl)
                    break;

                if (index + runlength >= bmpl) {
                    runlength = bmpl - index;

                    if (runlength == 0)
                        break;
                }

                runstart = tp.item->key.obj_id + (index << Vcb->sector_shift);
                runend = runstart + (runlength << Vcb->sector_shift);

                if (runstart > lastoff) {
                    Status = add_space_entry(&c->space, &c->space_size, lastoff, runstart - lastoff);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_space_entry returned %08lx\n", Status);
                        if (bmparr) ExFreePool(bmparr);
                        return Status;
                    }
                }

                lastoff = runend;

                runlength = RtlFindNextForwardRunClear(&bmp, index + runlength, &index);
            }

            if (lastoff < tp.item->key.obj_id + tp.item->key.offset) {
                Status = add_space_entry(&c->space, &c->space_size, lastoff, tp.item->key.obj_id + tp.item->key.offset - lastoff);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_space_entry returned %08lx\n", Status);
                    if (bmparr) ExFreePool(bmparr);
                    return Status;
                }
            }
        }
    }

    if (bmparr)
        ExFreePool(bmparr);

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

    return STATUS_SUCCESS;
}

static NTSTATUS load_free_space_cache(device_extension* Vcb, chunk* c, PIRP Irp) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    uint64_t lastaddr;
    bool b;
    space* s;
    NTSTATUS Status;

    if (Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE && Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE_VALID) {
        Status = load_stored_free_space_tree(Vcb, c, Irp);

        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
            ERR("load_stored_free_space_tree returned %08lx\n", Status);
            return Status;
        }
    } else if (Vcb->superblock.generation - 1 == Vcb->superblock.cache_generation) {
        Status = load_stored_free_space_cache(Vcb, c, false, Irp);

        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
            ERR("load_stored_free_space_cache returned %08lx\n", Status);
            return Status;
        }
    } else
        Status = STATUS_NOT_FOUND;

    if (Status == STATUS_NOT_FOUND) {
        TRACE("generating free space cache for chunk %I64x\n", c->offset);

        searchkey.obj_id = c->offset;
        searchkey.obj_type = TYPE_EXTENT_ITEM;
        searchkey.offset = 0;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
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

                    TRACE("(%I64x,%I64x)\n", s->address, s->size);
                }

                if (tp.item->key.obj_type == TYPE_METADATA_ITEM)
                    lastaddr = tp.item->key.obj_id + Vcb->superblock.node_size;
                else
                    lastaddr = tp.item->key.obj_id + tp.item->key.offset;
            }

            b = find_next_item(Vcb, &tp, &next_tp, false, Irp);
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

            TRACE("(%I64x,%I64x)\n", s->address, s->size);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS load_cache_chunk(device_extension* Vcb, chunk* c, PIRP Irp) {
    NTSTATUS Status;

    if (c->cache_loaded)
        return STATUS_SUCCESS;

    Status = load_free_space_cache(Vcb, c, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("load_free_space_cache returned %08lx\n", Status);
        return Status;
    }

    protect_superblocks(c);

    c->cache_loaded = true;

    return STATUS_SUCCESS;
}

static NTSTATUS insert_cache_extent(fcb* fcb, uint64_t start, uint64_t length, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le = fcb->Vcb->chunks.Flink;
    chunk* c;
    uint64_t flags;

    flags = fcb->Vcb->data_flags;

    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, fcb->Vcb);

            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, false, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, length, false, 0))
                    return STATUS_SUCCESS;
            }

            release_chunk_lock(c, fcb->Vcb);
        }

        le = le->Flink;
    }

    Status = alloc_chunk(fcb->Vcb, flags, &c, false);

    if (!NT_SUCCESS(Status)) {
        ERR("alloc_chunk returned %08lx\n", Status);
        return Status;
    }

    acquire_chunk_lock(c, fcb->Vcb);

    if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
        if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, false, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, length, false, 0))
            return STATUS_SUCCESS;
    }

    release_chunk_lock(c, fcb->Vcb);

    return STATUS_DISK_FULL;
}

static NTSTATUS allocate_cache_chunk(device_extension* Vcb, chunk* c, bool* changed, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    uint64_t num_entries, new_cache_size, i;
    uint32_t num_sectors;
    bool realloc_extents = false;

    // FIXME - also do bitmaps
    // FIXME - make sure this works when sector_size is not 4096

    *changed = false;

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

    new_cache_size = sizeof(uint64_t) + (num_entries * sizeof(FREE_SPACE_ENTRY));

    num_sectors = (uint32_t)sector_align(new_cache_size, Vcb->superblock.sector_size) >> Vcb->sector_shift;
    num_sectors = (uint32_t)sector_align(num_sectors, CACHE_INCREMENTS);

    // adjust for padding
    // FIXME - there must be a more efficient way of doing this
    new_cache_size = sizeof(uint64_t) + (sizeof(uint32_t) * num_sectors);
    for (i = 0; i < num_entries; i++) {
        if ((new_cache_size >> Vcb->sector_shift) != ((new_cache_size + sizeof(FREE_SPACE_ENTRY)) >> Vcb->sector_shift))
            new_cache_size = sector_align(new_cache_size, Vcb->superblock.sector_size);

        new_cache_size += sizeof(FREE_SPACE_ENTRY);
    }

    new_cache_size = sector_align(new_cache_size, CACHE_INCREMENTS << Vcb->sector_shift);

    TRACE("chunk %I64x: cache_size = %I64x, new_cache_size = %I64x\n", c->offset, c->cache ? c->cache->inode_item.st_size : 0, new_cache_size);

    if (c->cache) {
        if (new_cache_size > c->cache->inode_item.st_size)
            realloc_extents = true;
        else {
            le = c->cache->extents.Flink;

            while (le != &c->cache->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);

                if (!ext->ignore && (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ext->extent_data.data[0];

                    if (ed2->size != 0) {
                        chunk* c2 = get_chunk_from_address(Vcb, ed2->address);

                        if (c2 && (c2->readonly || c2->reloc)) {
                            realloc_extents = true;
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

        c->cache = create_fcb(Vcb, PagedPool);
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
        c->cache->hash = calc_crc32c(0xffffffff, (uint8_t*)&c->cache->inode, sizeof(uint64_t));

        c->cache->type = BTRFS_TYPE_FILE;
        c->cache->created = true;

        // create new free space entry

        fsi = ExAllocatePoolWithTag(PagedPool, sizeof(FREE_SPACE_ITEM), ALLOC_TAG);
        if (!fsi) {
            ERR("out of memory\n");
            reap_fcb(c->cache);
            c->cache = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            ExFreePool(fsi);
            reap_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }

        if (!keycmp(searchkey, tp.item->key)) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                ExFreePool(fsi);
                reap_fcb(c->cache);
                c->cache = NULL;
                return Status;
            }
        }

        fsi->key.obj_id = c->cache->inode;
        fsi->key.obj_type = TYPE_INODE_ITEM;
        fsi->key.offset = 0;

        Status = insert_tree_item(Vcb, Vcb->root_root, FREE_SPACE_CACHE_ID, 0, c->offset, fsi, sizeof(FREE_SPACE_ITEM), NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            ExFreePool(fsi);
            reap_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }

        // allocate space

        Status = insert_cache_extent(c->cache, 0, new_cache_size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_cache_extent returned %08lx\n", Status);
            reap_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }

        c->cache->extents_changed = true;
        InsertTailList(&Vcb->all_fcbs, &c->cache->list_entry_all);

        add_fcb_to_subvol(c->cache);

        Status = flush_fcb(c->cache, true, batchlist, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_fcb returned %08lx\n", Status);
            free_fcb(c->cache);
            c->cache = NULL;
            return Status;
        }

        *changed = true;
    } else if (realloc_extents) {
        KEY searchkey;
        traverse_ptr tp;

        TRACE("reallocating extents\n");

        // add free_space entry to tree cache

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (keycmp(searchkey, tp.item->key)) {
            ERR("could not find (%I64x,%x,%I64x) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            return STATUS_INTERNAL_ERROR;
        }

        if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }

        tp.tree->write = true;

        // remove existing extents

        if (c->cache->inode_item.st_size > 0) {
            le = c->cache->extents.Flink;

            while (le != &c->cache->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);

                if (!ext->ignore && (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ext->extent_data.data[0];

                    if (ed2->size != 0) {
                        chunk* c2 = get_chunk_from_address(Vcb, ed2->address);

                        if (c2) {
                            c2->changed = true;
                            c2->space_changed = true;
                        }
                    }
                }

                le = le->Flink;
            }

            Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("excise_extents returned %08lx\n", Status);
                return Status;
            }
        }

        // add new extent

        Status = insert_cache_extent(c->cache, 0, new_cache_size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_cache_extent returned %08lx\n", Status);
            return Status;
        }

        // modify INODE_ITEM

        c->cache->inode_item.st_size = new_cache_size;
        c->cache->inode_item.st_blocks = new_cache_size;

        Status = flush_fcb(c->cache, true, batchlist, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_fcb returned %08lx\n", Status);
            return Status;
        }

        *changed = true;
    } else {
        KEY searchkey;
        traverse_ptr tp;

        // add INODE_ITEM and free_space entry to tree cache, for writing later

        searchkey.obj_id = c->cache->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (keycmp(searchkey, tp.item->key)) {
            INODE_ITEM* ii;

            ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
            if (!ii) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(ii, &c->cache->inode_item, sizeof(INODE_ITEM));

            Status = insert_tree_item(Vcb, Vcb->root_root, c->cache->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                ExFreePool(ii);
                return Status;
            }

            *changed = true;
        } else {
            if (tp.item->size < sizeof(INODE_ITEM)) {
                ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
                return STATUS_INTERNAL_ERROR;
            }

            tp.tree->write = true;
        }

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (keycmp(searchkey, tp.item->key)) {
            ERR("could not find (%I64x,%x,%I64x) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            return STATUS_INTERNAL_ERROR;
        }

        if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }

        tp.tree->write = true;
    }

    // FIXME - reduce inode allocation if cache is shrinking. Make sure to avoid infinite write loops

    return STATUS_SUCCESS;
}

NTSTATUS allocate_cache(device_extension* Vcb, bool* changed, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, batchlist;
    NTSTATUS Status;

    *changed = false;

    InitializeListHead(&batchlist);

    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, true);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->space_changed && c->chunk_item->size >= 0x6400000) { // 100MB
            bool b;

            acquire_chunk_lock(c, Vcb);
            Status = allocate_cache_chunk(Vcb, c, &b, &batchlist, Irp, rollback);
            release_chunk_lock(c, Vcb);

            if (b)
                *changed = true;

            if (!NT_SUCCESS(Status)) {
                ERR("allocate_cache_chunk(%I64x) returned %08lx\n", c->offset, Status);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                clear_batch_list(Vcb, &batchlist);
                return Status;
            }
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static void add_rollback_space(LIST_ENTRY* rollback, bool add, LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t address, uint64_t length, chunk* c) {
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

    add_rollback(rollback, add ? ROLLBACK_ADD_SPACE : ROLLBACK_SUBTRACT_SPACE, rs);
}

void space_list_add2(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t address, uint64_t length, chunk* c, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    space *s, *s2;

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
            add_rollback_space(rollback, true, list, list_size, address, length, c);

        return;
    }

    le = list->Flink;
    do {
        s2 = CONTAINING_RECORD(le, space, list_entry);

        // old entry envelops new one completely
        if (s2->address <= address && s2->address + s2->size >= address + length)
            return;

        // new entry envelops old one completely
        if (address <= s2->address && address + length >= s2->address + s2->size) {
            if (address < s2->address) {
                if (rollback)
                    add_rollback_space(rollback, true, list, list_size, address, s2->address - address, c);

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
                    add_rollback_space(rollback, true, list, list_size, s2->address + s2->size, address + length - s2->address - s2->size, c);

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
                add_rollback_space(rollback, true, list, list_size, address, s2->address - address, c);

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
                add_rollback_space(rollback, true, list, list_size, address, s2->address + s2->size - address, c);

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
                add_rollback_space(rollback, true, list, list_size, address, length, c);

            s->address = address;
            s->size = length;
            InsertHeadList(s2->list_entry.Blink, &s->list_entry);

            if (list_size)
                order_space_entry(s, list_size);

            return;
        }

        le = le->Flink;
    } while (le != list);

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
        add_rollback_space(rollback, true, list, list_size, address, length, c);
}

void space_list_merge(LIST_ENTRY* spacelist, LIST_ENTRY* spacelist_size, LIST_ENTRY* deleting) {
    LIST_ENTRY* le = deleting->Flink;

    while (le != deleting) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        space_list_add2(spacelist, spacelist_size, s->address, s->size, NULL, NULL);

        le = le->Flink;
    }
}

static NTSTATUS copy_space_list(LIST_ENTRY* old_list, LIST_ENTRY* new_list) {
    LIST_ENTRY* le;

    le = old_list->Flink;
    while (le != old_list) {
        space* s = CONTAINING_RECORD(le, space, list_entry);
        space* s2;

        s2 = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
        if (!s2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        s2->address = s->address;
        s2->size = s->size;

        InsertTailList(new_list, &s2->list_entry);

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS update_chunk_cache(device_extension* Vcb, chunk* c, BTRFS_TIME* now, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    FREE_SPACE_ITEM* fsi;
    void* data;
    uint64_t num_entries, *cachegen, off;
    uint32_t *checksums, num_sectors;
    LIST_ENTRY space_list, deleting;

    data = ExAllocatePoolWithTag(NonPagedPool, (ULONG)c->cache->inode_item.st_size, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(data, (ULONG)c->cache->inode_item.st_size);

    InitializeListHead(&space_list);
    InitializeListHead(&deleting);

    Status = copy_space_list(&c->space, &space_list);
    if (!NT_SUCCESS(Status)) {
        ERR("copy_space_list returned %08lx\n", Status);
        goto end;
    }

    Status = copy_space_list(&c->deleting, &deleting);
    if (!NT_SUCCESS(Status)) {
        ERR("copy_space_list returned %08lx\n", Status);

        while (!IsListEmpty(&space_list)) {
            ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
        }

        goto end;
    }

    space_list_merge(&space_list, NULL, &deleting);

    num_entries = 0;
    num_sectors = (uint32_t)(c->cache->inode_item.st_size >> Vcb->sector_shift);
    off = (sizeof(uint32_t) * num_sectors) + sizeof(uint64_t);

    while (!IsListEmpty(&space_list)) {
        FREE_SPACE_ENTRY* fse;

        space* s = CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry);

        if ((off + sizeof(FREE_SPACE_ENTRY)) >> Vcb->sector_shift != off >> Vcb->sector_shift)
            off = sector_align(off, Vcb->superblock.sector_size);

        fse = (FREE_SPACE_ENTRY*)((uint8_t*)data + off);

        fse->offset = s->address;
        fse->size = s->size;
        fse->type = FREE_SPACE_EXTENT;
        num_entries++;

        off += sizeof(FREE_SPACE_ENTRY);
    }

    // update INODE_ITEM

    c->cache->inode_item.generation = Vcb->superblock.generation;
    c->cache->inode_item.transid = Vcb->superblock.generation;
    c->cache->inode_item.sequence++;
    c->cache->inode_item.st_ctime = *now;

    Status = flush_fcb(c->cache, true, batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("flush_fcb returned %08lx\n", Status);
        goto end;
    }

    // update free_space item

    searchkey.obj_id = FREE_SPACE_CACHE_ID;
    searchkey.obj_type = 0;
    searchkey.offset = c->offset;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        goto end;
    }

    if (keycmp(searchkey, tp.item->key)) {
        ERR("could not find (%I64x,%x,%I64x) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    if (tp.item->size < sizeof(FREE_SPACE_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(FREE_SPACE_ITEM));
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    fsi = (FREE_SPACE_ITEM*)tp.item->data;

    fsi->generation = Vcb->superblock.generation;
    fsi->num_entries = num_entries;
    fsi->num_bitmaps = 0;

    // set cache generation

    cachegen = (uint64_t*)((uint8_t*)data + (sizeof(uint32_t) * num_sectors));
    *cachegen = Vcb->superblock.generation;

    // calculate cache checksums

    checksums = (uint32_t*)data;

    // FIXME - if we know sector is fully zeroed, use cached checksum

    for (uint32_t i = 0; i < num_sectors; i++) {
        if (i << Vcb->sector_shift > sizeof(uint32_t) * num_sectors)
            checksums[i] = ~calc_crc32c(0xffffffff, (uint8_t*)data + (i << Vcb->sector_shift), Vcb->superblock.sector_size);
        else if ((i + 1) << Vcb->sector_shift < sizeof(uint32_t) * num_sectors)
            checksums[i] = 0; // FIXME - test this
        else
            checksums[i] = ~calc_crc32c(0xffffffff, (uint8_t*)data + (sizeof(uint32_t) * num_sectors), ((i + 1) << Vcb->sector_shift) - (sizeof(uint32_t) * num_sectors));
    }

    // write cache

    Status = do_write_file(c->cache, 0, c->cache->inode_item.st_size, data, NULL, false, 0, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("do_write_file returned %08lx\n", Status);

        // Writing the cache isn't critical, so we don't return an error if writing fails. This means
        // we can still flush on a degraded mount if metadata is RAID1 but data is RAID0.
    }

    Status = STATUS_SUCCESS;

end:
    ExFreePool(data);

    return Status;
}

static NTSTATUS update_chunk_cache_tree(device_extension* Vcb, chunk* c, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY space_list;
    FREE_SPACE_INFO* fsi;
    KEY searchkey;
    traverse_ptr tp;
    uint32_t fsi_count = 0;

    InitializeListHead(&space_list);

    Status = copy_space_list(&c->space, &space_list);
    if (!NT_SUCCESS(Status)) {
        ERR("copy_space_list returned %08lx\n", Status);
        return Status;
    }

    space_list_merge(&space_list, NULL, &c->deleting);

    searchkey.obj_id = c->offset;
    searchkey.obj_type = TYPE_FREE_SPACE_EXTENT;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->space_root, &tp, &searchkey, false, Irp);
    if (Status == STATUS_NOT_FOUND)
        goto after_tree_walk;
    else if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);

        while (!IsListEmpty(&space_list)) {
            ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
        }

        return Status;
    }

    while (!IsListEmpty(&space_list) && tp.item->key.obj_id < c->offset + c->chunk_item->size) {
        traverse_ptr next_tp;

        if (tp.item->key.obj_type == TYPE_FREE_SPACE_EXTENT) {
            space* s = CONTAINING_RECORD(space_list.Flink, space, list_entry);

            if (s->address < tp.item->key.obj_id || (s->address == tp.item->key.obj_id && s->size < tp.item->key.offset)) {
                // add entry

                Status = insert_tree_item(Vcb, Vcb->space_root, s->address, TYPE_FREE_SPACE_EXTENT, s->size, NULL, 0,
                                          NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08lx\n", Status);

                    while (!IsListEmpty(&space_list)) {
                        ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
                    }

                    return Status;
                }

                fsi_count++;

                ExFreePool(s);
                RemoveHeadList(&space_list);
                continue;
            } else if (s->address == tp.item->key.obj_id && s->size == tp.item->key.offset) {
                // unchanged entry

                fsi_count++;

                ExFreePool(s);
                RemoveHeadList(&space_list);
            } else {
                // remove entry

                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08lx\n", Status);

                    while (!IsListEmpty(&space_list)) {
                        ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
                    }

                    return Status;
                }
            }
        } else if (tp.item->key.obj_type == TYPE_FREE_SPACE_BITMAP) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);

                while (!IsListEmpty(&space_list)) {
                    ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
                }

                return Status;
            }
        }

        if (!find_next_item(Vcb, &tp, &next_tp, false, Irp))
            goto after_tree_walk;

        tp = next_tp;
    }

    // after loop, delete remaining tree items for this chunk

    while (tp.item->key.obj_id < c->offset + c->chunk_item->size) {
        traverse_ptr next_tp;

        if (tp.item->key.obj_type == TYPE_FREE_SPACE_EXTENT || tp.item->key.obj_type == TYPE_FREE_SPACE_BITMAP) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }
        }

        if (!find_next_item(Vcb, &tp, &next_tp, false, NULL))
            break;

        tp = next_tp;
    }

after_tree_walk:
    // after loop, insert remaining space_list entries

    while (!IsListEmpty(&space_list)) {
        space* s = CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry);

        Status = insert_tree_item(Vcb, Vcb->space_root, s->address, TYPE_FREE_SPACE_EXTENT, s->size, NULL, 0,
                                  NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);

            while (!IsListEmpty(&space_list)) {
                ExFreePool(CONTAINING_RECORD(RemoveHeadList(&space_list), space, list_entry));
            }

            return Status;
        }

        fsi_count++;

        ExFreePool(s);
    }

    // change TYPE_FREE_SPACE_INFO in place if present, and insert otherwise

    searchkey.obj_id = c->offset;
    searchkey.obj_type = TYPE_FREE_SPACE_INFO;
    searchkey.offset = c->chunk_item->size;

    Status = find_item(Vcb, Vcb->space_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND) {
        ERR("find_item returned %08lx\n", Status);
        return Status;
    }

    if (NT_SUCCESS(Status) && !keycmp(tp.item->key, searchkey)) {
        if (tp.item->size == sizeof(FREE_SPACE_INFO)) {
            tree* t;

            // change in place if possible

            fsi = (FREE_SPACE_INFO*)tp.item->data;

            fsi->count = fsi_count;
            fsi->flags = 0;

            tp.tree->write = true;

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
        } else
            delete_tree_item(Vcb, &tp);
    }

    // insert FREE_SPACE_INFO

    fsi = ExAllocatePoolWithTag(PagedPool, sizeof(FREE_SPACE_INFO), ALLOC_TAG);
    if (!fsi) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fsi->count = fsi_count;
    fsi->flags = 0;

    Status = insert_tree_item(Vcb, Vcb->space_root, c->offset, TYPE_FREE_SPACE_INFO, c->chunk_item->size, fsi, sizeof(fsi),
                              NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS update_chunk_caches(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, batchlist;
    NTSTATUS Status;
    chunk* c;
    LARGE_INTEGER time;
    BTRFS_TIME now;

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    InitializeListHead(&batchlist);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->space_changed && c->chunk_item->size >= 0x6400000) { // 100MB
            acquire_chunk_lock(c, Vcb);
            Status = update_chunk_cache(Vcb, c, &now, &batchlist, Irp, rollback);
            release_chunk_lock(c, Vcb);

            if (!NT_SUCCESS(Status)) {
                ERR("update_chunk_cache(%I64x) returned %08lx\n", c->offset, Status);
                clear_batch_list(Vcb, &batchlist);
                return Status;
            }
        }

        le = le->Flink;
    }

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08lx\n", Status);
        return Status;
    }

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->changed && (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)) {
            ExAcquireResourceExclusiveLite(&c->partial_stripes_lock, true);

            while (!IsListEmpty(&c->partial_stripes)) {
                partial_stripe* ps = CONTAINING_RECORD(RemoveHeadList(&c->partial_stripes), partial_stripe, list_entry);

                Status = flush_partial_stripe(Vcb, c, ps);

                if (ps->bmparr)
                    ExFreePool(ps->bmparr);

                ExFreePool(ps);

                if (!NT_SUCCESS(Status)) {
                    ERR("flush_partial_stripe returned %08lx\n", Status);
                    ExReleaseResourceLite(&c->partial_stripes_lock);
                    return Status;
                }
            }

            ExReleaseResourceLite(&c->partial_stripes_lock);
        }

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

NTSTATUS update_chunk_caches_tree(device_extension* Vcb, PIRP Irp) {
    LIST_ENTRY *le;
    NTSTATUS Status;
    chunk* c;

    Vcb->superblock.compat_ro_flags |= BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE_VALID;

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, true);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->space_changed) {
            acquire_chunk_lock(c, Vcb);
            Status = update_chunk_cache_tree(Vcb, c, Irp);
            release_chunk_lock(c, Vcb);

            if (!NT_SUCCESS(Status)) {
                ERR("update_chunk_cache_tree(%I64x) returned %08lx\n", c->offset, Status);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                return Status;
            }
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    return STATUS_SUCCESS;
}

void space_list_add(chunk* c, uint64_t address, uint64_t length, LIST_ENTRY* rollback) {
    TRACE("(%p, %I64x, %I64x, %p)\n", c, address, length, rollback);

    c->changed = true;
    c->space_changed = true;

    space_list_add2(&c->deleting, NULL, address, length, c, rollback);
}

void space_list_subtract2(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t address, uint64_t length, chunk* c, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *le2;
    space *s, *s2;

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
                add_rollback_space(rollback, false, list, list_size, s2->address, s2->size, c);

            RemoveEntryList(&s2->list_entry);

            if (list_size)
                RemoveEntryList(&s2->list_entry_size);

            ExFreePool(s2);
        } else if (address + length > s2->address && address + length < s2->address + s2->size) {
            if (address > s2->address) { // cut out hole
                if (rollback)
                    add_rollback_space(rollback, false, list, list_size, address, length, c);

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
                    add_rollback_space(rollback, false, list, list_size, s2->address, address + length - s2->address, c);

                s2->size -= address + length - s2->address;
                s2->address = address + length;

                if (list_size) {
                    RemoveEntryList(&s2->list_entry_size);
                    order_space_entry(s2, list_size);
                }
            }
        } else if (address > s2->address && address < s2->address + s2->size) { // remove end of entry
            if (rollback)
                add_rollback_space(rollback, false, list, list_size, address, s2->address + s2->size - address, c);

            s2->size = address - s2->address;

            if (list_size) {
                RemoveEntryList(&s2->list_entry_size);
                order_space_entry(s2, list_size);
            }
        }

        le = le2;
    }
}

void space_list_subtract(chunk* c, uint64_t address, uint64_t length, LIST_ENTRY* rollback) {
    c->changed = true;
    c->space_changed = true;

    space_list_subtract2(&c->space, &c->space_size, address, length, c, rollback);

    space_list_subtract2(&c->deleting, NULL, address, length, c, rollback);
}
