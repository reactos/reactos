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

typedef struct {
    uint8_t type;

    union {
        EXTENT_DATA_REF edr;
        SHARED_DATA_REF sdr;
        TREE_BLOCK_REF tbr;
        SHARED_BLOCK_REF sbr;
    };

    uint64_t hash;
    LIST_ENTRY list_entry;
} extent_ref;

uint64_t get_extent_data_ref_hash2(uint64_t root, uint64_t objid, uint64_t offset) {
    uint32_t high_crc = 0xffffffff, low_crc = 0xffffffff;

    high_crc = calc_crc32c(high_crc, (uint8_t*)&root, sizeof(uint64_t));
    low_crc = calc_crc32c(low_crc, (uint8_t*)&objid, sizeof(uint64_t));
    low_crc = calc_crc32c(low_crc, (uint8_t*)&offset, sizeof(uint64_t));

    return ((uint64_t)high_crc << 31) ^ (uint64_t)low_crc;
}

static __inline uint64_t get_extent_data_ref_hash(EXTENT_DATA_REF* edr) {
    return get_extent_data_ref_hash2(edr->root, edr->objid, edr->offset);
}

static uint64_t get_extent_hash(uint8_t type, void* data) {
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

static NTSTATUS add_shared_data_extent_ref(LIST_ENTRY* extent_refs, uint64_t parent, uint32_t count) {
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

static NTSTATUS add_shared_block_extent_ref(LIST_ENTRY* extent_refs, uint64_t parent) {
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

static NTSTATUS add_tree_block_extent_ref(LIST_ENTRY* extent_refs, uint64_t root) {
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

static void sort_extent_refs(LIST_ENTRY* extent_refs) {
    LIST_ENTRY newlist;

    if (IsListEmpty(extent_refs))
        return;

    // insertion sort

    InitializeListHead(&newlist);

    while (!IsListEmpty(extent_refs)) {
        extent_ref* er = CONTAINING_RECORD(RemoveHeadList(extent_refs), extent_ref, list_entry);
        LIST_ENTRY* le;
        bool inserted = false;

        le = newlist.Flink;
        while (le != &newlist) {
            extent_ref* er2 = CONTAINING_RECORD(le, extent_ref, list_entry);

            if (er->type < er2->type || (er->type == er2->type && er->hash > er2->hash)) {
                InsertHeadList(le->Blink, &er->list_entry);
                inserted = true;
                break;
            }

            le = le->Flink;
        }

        if (!inserted)
            InsertTailList(&newlist, &er->list_entry);
    }

    newlist.Flink->Blink = extent_refs;
    newlist.Blink->Flink = extent_refs;
    extent_refs->Flink = newlist.Flink;
    extent_refs->Blink = newlist.Blink;
}

static NTSTATUS construct_extent_item(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t flags, LIST_ENTRY* extent_refs,
                                      KEY* firstitem, uint8_t level, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY *le, *next_le;
    uint64_t refcount;
    uint16_t inline_len;
    bool all_inline = true;
    extent_ref* first_noninline = NULL;
    EXTENT_ITEM* ei;
    uint8_t* siptr;

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
        uint64_t rc;

        next_le = le->Flink;

        rc = get_extent_data_refcount(er->type, &er->edr);

        if (rc == 0) {
            RemoveEntryList(&er->list_entry);

            ExFreePool(er);
        } else {
            uint16_t extlen = get_extent_data_len(er->type);

            refcount += rc;

            er->hash = get_extent_hash(er->type, &er->edr);

            if (all_inline) {
                if ((uint16_t)(inline_len + 1 + extlen) > Vcb->superblock.node_size >> 2) {
                    all_inline = false;
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

        siptr = (uint8_t*)&ei2[1];
    } else
        siptr = (uint8_t*)&ei[1];

    sort_extent_refs(extent_refs);

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

    Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, inline_len, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(ei);
        return Status;
    }

    if (!all_inline) {
        le = &first_noninline->list_entry;

        while (le != extent_refs) {
            extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
            uint16_t len;
            uint8_t* data;

            if (er->type == TYPE_EXTENT_DATA_REF) {
                len = sizeof(EXTENT_DATA_REF);

                data = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);

                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(data, &er->edr, len);
            } else if (er->type == TYPE_SHARED_DATA_REF) {
                len = sizeof(uint32_t);

                data = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);

                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                *((uint32_t*)data) = er->sdr.count;
            } else {
                len = 0;
                data = NULL;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, address, er->type, er->hash, data, len, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                if (data) ExFreePool(data);
                return Status;
            }

            le = le->Flink;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS convert_old_extent(device_extension* Vcb, uint64_t address, bool tree, KEY* firstitem, uint8_t level, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    LIST_ENTRY extent_refs;
    uint64_t size;

    InitializeListHead(&extent_refs);

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("old-style extent %I64x not found\n", address);
        return STATUS_INTERNAL_ERROR;
    }

    size = tp.item->key.offset;

    Status = delete_tree_item(Vcb, &tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08lx\n", Status);
        return Status;
    }

    while (find_next_item(Vcb, &tp, &next_tp, false, Irp)) {
        tp = next_tp;

        if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_EXTENT_REF_V0 && tp.item->size >= sizeof(EXTENT_REF_V0)) {
            EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)tp.item->data;

            if (tree) {
                if (tp.item->key.offset == tp.item->key.obj_id) { // top of the tree
                    Status = add_tree_block_extent_ref(&extent_refs, erv0->root);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_tree_block_extent_ref returned %08lx\n", Status);
                        goto end;
                    }
                } else {
                    Status = add_shared_block_extent_ref(&extent_refs, tp.item->key.offset);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_shared_block_extent_ref returned %08lx\n", Status);
                        goto end;
                    }
                }
            } else {
                Status = add_shared_data_extent_ref(&extent_refs, tp.item->key.offset, erv0->count);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_shared_data_extent_ref returned %08lx\n", Status);
                    goto end;
                }
            }

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                goto end;
            }
        }

        if (tp.item->key.obj_id > address || tp.item->key.obj_type > TYPE_EXTENT_REF_V0)
            break;
    }

    Status = construct_extent_item(Vcb, address, size, tree ? (EXTENT_ITEM_TREE_BLOCK | EXTENT_ITEM_SHARED_BACKREFS) : EXTENT_ITEM_DATA,
                                   &extent_refs, firstitem, level, Irp);
    if (!NT_SUCCESS(Status))
        ERR("construct_extent_item returned %08lx\n", Status);

end:
    free_extent_refs(&extent_refs);

    return Status;
}

NTSTATUS increase_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, uint8_t type, void* data, KEY* firstitem, uint8_t level, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    ULONG len, max_extent_item_size;
    uint16_t datalen = get_extent_data_len(type);
    EXTENT_ITEM* ei;
    uint8_t* ptr;
    uint64_t inline_rc, offset;
    uint8_t* data2;
    EXTENT_ITEM* newei;
    bool skinny;
    bool is_tree = type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF;

    if (datalen == 0) {
        ERR("unrecognized extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    // If entry doesn't exist yet, create new inline extent item

    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        uint16_t eisize;

        eisize = sizeof(EXTENT_ITEM);
        if (is_tree && !(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) eisize += sizeof(EXTENT_ITEM2);
        eisize += sizeof(uint8_t);
        eisize += datalen;

        ei = ExAllocatePoolWithTag(PagedPool, eisize, ALLOC_TAG);
        if (!ei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ei->refcount = get_extent_data_refcount(type, data);
        ei->generation = Vcb->superblock.generation;
        ei->flags = is_tree ? EXTENT_ITEM_TREE_BLOCK : EXTENT_ITEM_DATA;
        ptr = (uint8_t*)&ei[1];

        if (is_tree && !(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
            EXTENT_ITEM2* ei2 = (EXTENT_ITEM2*)ptr;
            ei2->firstitem = *firstitem;
            ei2->level = level;
            ptr = (uint8_t*)&ei2[1];
        }

        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);

        if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && is_tree)
            Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, ei, eisize, NULL, Irp);
        else
            Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, eisize, NULL, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            return Status;
        }

        return STATUS_SUCCESS;
    } else if (tp.item->key.obj_id == address && tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset != size) {
        ERR("extent %I64x exists, but with size %I64x rather than %I64x expected\n", tp.item->key.obj_id, tp.item->key.offset, size);
        return STATUS_INTERNAL_ERROR;
    }

    skinny = tp.item->key.obj_type == TYPE_METADATA_ITEM;

    if (tp.item->size == sizeof(EXTENT_ITEM_V0) && !skinny) {
        Status = convert_old_extent(Vcb, address, is_tree, firstitem, level, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_extent returned %08lx\n", Status);
            return Status;
        }

        return increase_extent_refcount(Vcb, address, size, type, data, firstitem, level, Irp);
    }

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (uint8_t*)&ei[1];

    if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return STATUS_INTERNAL_ERROR;
        }

        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }

    inline_rc = 0;

    // Loop through existing inline extent entries

    while (len > 0) {
        uint8_t secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        uint64_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

        len--;

        if (sectlen > len) {
            ERR("(%I64x,%x,%I64x): %lx bytes left, expecting at least %lx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }

        // If inline extent already present, increase refcount and return

        if (secttype == type) {
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(uint8_t));
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;

                if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
                    uint32_t rc = get_extent_data_refcount(type, data);
                    EXTENT_DATA_REF* sectedr2;

                    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(newei, tp.item->data, tp.item->size);

                    newei->refcount += rc;

                    sectedr2 = (EXTENT_DATA_REF*)((uint8_t*)newei + ((uint8_t*)sectedr - tp.item->data));
                    sectedr2->count += rc;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_TREE_BLOCK_REF) {
                TREE_BLOCK_REF* secttbr = (TREE_BLOCK_REF*)(ptr + sizeof(uint8_t));
                TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;

                if (secttbr->offset == tbr->offset) {
                    TRACE("trying to increase refcount of non-shared tree extent\n");
                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_BLOCK_REF) {
                SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(uint8_t));
                SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;

                if (sectsbr->offset == sbr->offset)
                    return STATUS_SUCCESS;
            } else if (type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(uint8_t));
                SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;

                if (sectsdr->offset == sdr->offset) {
                    uint32_t rc = get_extent_data_refcount(type, data);
                    SHARED_DATA_REF* sectsdr2;

                    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(newei, tp.item->data, tp.item->size);

                    newei->refcount += rc;

                    sectsdr2 = (SHARED_DATA_REF*)((uint8_t*)newei + ((uint8_t*)sectsdr - tp.item->data));
                    sectsdr2->count += rc;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
        }

        len -= sectlen;
        ptr += sizeof(uint8_t) + sectlen;
        inline_rc += sectcount;
    }

    offset = get_extent_hash(type, data);

    max_extent_item_size = (Vcb->superblock.node_size >> 4) - sizeof(leaf_node);

    // If we can, add entry as inline extent item

    if (inline_rc == ei->refcount && tp.item->size + sizeof(uint8_t) + datalen < max_extent_item_size) {
        len = tp.item->size - sizeof(EXTENT_ITEM);
        ptr = (uint8_t*)&ei[1];

        if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
            len -= sizeof(EXTENT_ITEM2);
            ptr += sizeof(EXTENT_ITEM2);
        }

        // Confusingly, it appears that references are sorted forward by type (i.e. EXTENT_DATA_REFs before
        // SHARED_DATA_REFs), but then backwards by hash...

        while (len > 0) {
            uint8_t secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);

            if (secttype > type)
                break;

            if (secttype == type) {
                uint64_t sectoff = get_extent_hash(secttype, ptr + 1);

                if (sectoff < offset)
                    break;
            }

            len -= sectlen + sizeof(uint8_t);
            ptr += sizeof(uint8_t) + sectlen;
        }

        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size + sizeof(uint8_t) + datalen, ALLOC_TAG);
        RtlCopyMemory(newei, tp.item->data, ptr - tp.item->data);

        newei->refcount += get_extent_data_refcount(type, data);

        if (len > 0)
            RtlCopyMemory((uint8_t*)newei + (ptr - tp.item->data) + sizeof(uint8_t) + datalen, ptr, len);

        ptr = (ptr - tp.item->data) + (uint8_t*)newei;

        *ptr = type;
        RtlCopyMemory(ptr + 1, data, datalen);

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size + sizeof(uint8_t) + datalen, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            return Status;
        }

        return STATUS_SUCCESS;
    }

    // Look for existing non-inline entry, and increase refcount if found

    if (inline_rc != ei->refcount) {
        traverse_ptr tp2;

        searchkey.obj_id = address;
        searchkey.obj_type = type;
        searchkey.offset = offset;

        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (!keycmp(tp2.item->key, searchkey)) {
            if (type == TYPE_SHARED_DATA_REF && tp2.item->size < sizeof(uint32_t)) {
                ERR("(%I64x,%x,%I64x) was %x bytes, expecting %Ix\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp2.item->size, sizeof(uint32_t));
                return STATUS_INTERNAL_ERROR;
            } else if (type != TYPE_SHARED_DATA_REF && tp2.item->size < datalen) {
                ERR("(%I64x,%x,%I64x) was %x bytes, expecting %x\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp2.item->size, datalen);
                return STATUS_INTERNAL_ERROR;
            }

            data2 = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);
            if (!data2) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

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
                uint32_t* sdr = (uint32_t*)data2;

                *sdr += get_extent_data_refcount(type, data);
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }

            Status = delete_tree_item(Vcb, &tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, data2, tp2.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                return Status;
            }

            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount += get_extent_data_refcount(type, data);

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                return Status;
            }

            return STATUS_SUCCESS;
        }
    }

    // Otherwise, add new non-inline entry

    if (type == TYPE_SHARED_DATA_REF) {
        SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;

        data2 = ExAllocatePoolWithTag(PagedPool, sizeof(uint32_t), ALLOC_TAG);
        if (!data2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        datalen = sizeof(uint32_t);

        *((uint32_t*)data2) = sdr->count;
    } else if (type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF) {
        data2 = NULL;
        datalen = 0;
    } else {
        data2 = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
        if (!data2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(data2, data, datalen);
    }

    Status = insert_tree_item(Vcb, Vcb->extent_root, address, type, offset, data2, datalen, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        return Status;
    }

    newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
    if (!newei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(newei, tp.item->data, tp.item->size);

    newei->refcount += get_extent_data_refcount(type, data);

    Status = delete_tree_item(Vcb, &tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08lx\n", Status);
        return Status;
    }

    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS increase_extent_refcount_data(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint64_t inode, uint64_t offset, uint32_t refcount, PIRP Irp) {
    EXTENT_DATA_REF edr;

    edr.root = root;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;

    return increase_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp);
}

NTSTATUS decrease_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, uint8_t type, void* data, KEY* firstitem,
                                  uint8_t level, uint64_t parent, bool superseded, PIRP Irp) {
    KEY searchkey;
    NTSTATUS Status;
    traverse_ptr tp, tp2;
    EXTENT_ITEM* ei;
    ULONG len;
    uint64_t inline_rc;
    uint8_t* ptr;
    uint32_t rc = data ? get_extent_data_refcount(type, data) : 1;
    ULONG datalen = get_extent_data_len(type);
    bool is_tree = (type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF), skinny = false;

    if (is_tree && Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            skinny = true;
    }

    if (!skinny) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_EXTENT_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("could not find EXTENT_ITEM for address %I64x\n", address);
            return STATUS_INTERNAL_ERROR;
        }

        if (tp.item->key.offset != size) {
            ERR("extent %I64x had length %I64x, not %I64x as expected\n", address, tp.item->key.offset, size);
            return STATUS_INTERNAL_ERROR;
        }

        if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
            Status = convert_old_extent(Vcb, address, is_tree, firstitem, level, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("convert_old_extent returned %08lx\n", Status);
                return Status;
            }

            return decrease_extent_refcount(Vcb, address, size, type, data, firstitem, level, parent, superseded, Irp);
        }
    }

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (uint8_t*)&ei[1];

    if (ei->flags & EXTENT_ITEM_TREE_BLOCK && !skinny) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return STATUS_INTERNAL_ERROR;
        }

        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }

    if (ei->refcount < rc) {
        ERR("error - extent has refcount %I64x, trying to reduce by %x\n", ei->refcount, rc);
        return STATUS_INTERNAL_ERROR;
    }

    inline_rc = 0;

    // Loop through inline extent entries

    while (len > 0) {
        uint8_t secttype = *ptr;
        uint16_t sectlen = get_extent_data_len(secttype);
        uint64_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

        len--;

        if (sectlen > len) {
            ERR("(%I64x,%x,%I64x): %lx bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return STATUS_INTERNAL_ERROR;
        }

        if (sectlen == 0) {
            ERR("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return STATUS_INTERNAL_ERROR;
        }

        if (secttype == type) {
            if (type == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(uint8_t));
                EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;

                if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
                    uint16_t neweilen;
                    EXTENT_ITEM* newei;

                    if (ei->refcount == edr->count) {
                        Status = delete_tree_item(Vcb, &tp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_tree_item returned %08lx\n", Status);
                            return Status;
                        }

                        if (!superseded)
                            add_checksum_entry(Vcb, address, (ULONG)(size >> Vcb->sector_shift), NULL, Irp);

                        return STATUS_SUCCESS;
                    }

                    if (sectedr->count < edr->count) {
                        ERR("error - extent section has refcount %x, trying to reduce by %x\n", sectedr->count, edr->count);
                        return STATUS_INTERNAL_ERROR;
                    }

                    if (sectedr->count > edr->count)    // reduce section refcount
                        neweilen = tp.item->size;
                    else                                // remove section entirely
                        neweilen = tp.item->size - sizeof(uint8_t) - sectlen;

                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    if (sectedr->count > edr->count) {
                        EXTENT_DATA_REF* newedr = (EXTENT_DATA_REF*)((uint8_t*)newei + ((uint8_t*)sectedr - tp.item->data));

                        RtlCopyMemory(newei, ei, neweilen);

                        newedr->count -= rc;
                    } else {
                        RtlCopyMemory(newei, ei, ptr - tp.item->data);

                        if (len > sectlen)
                            RtlCopyMemory((uint8_t*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(uint8_t), len - sectlen);
                    }

                    newei->refcount -= rc;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_DATA_REF) {
                SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(uint8_t));
                SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;

                if (sectsdr->offset == sdr->offset) {
                    EXTENT_ITEM* newei;
                    uint16_t neweilen;

                    if (ei->refcount == sectsdr->count) {
                        Status = delete_tree_item(Vcb, &tp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_tree_item returned %08lx\n", Status);
                            return Status;
                        }

                        if (!superseded)
                            add_checksum_entry(Vcb, address, (ULONG)(size >> Vcb->sector_shift), NULL, Irp);

                        return STATUS_SUCCESS;
                    }

                    if (sectsdr->count < sdr->count) {
                        ERR("error - SHARED_DATA_REF has refcount %x, trying to reduce by %x\n", sectsdr->count, sdr->count);
                        return STATUS_INTERNAL_ERROR;
                    }

                    if (sectsdr->count > sdr->count)    // reduce section refcount
                        neweilen = tp.item->size;
                    else                                // remove section entirely
                        neweilen = tp.item->size - sizeof(uint8_t) - sectlen;

                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    if (sectsdr->count > sdr->count) {
                        SHARED_DATA_REF* newsdr = (SHARED_DATA_REF*)((uint8_t*)newei + ((uint8_t*)sectsdr - tp.item->data));

                        RtlCopyMemory(newei, ei, neweilen);

                        newsdr->count -= rc;
                    } else {
                        RtlCopyMemory(newei, ei, ptr - tp.item->data);

                        if (len > sectlen)
                            RtlCopyMemory((uint8_t*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(uint8_t), len - sectlen);
                    }

                    newei->refcount -= rc;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_TREE_BLOCK_REF) {
                TREE_BLOCK_REF* secttbr = (TREE_BLOCK_REF*)(ptr + sizeof(uint8_t));
                TREE_BLOCK_REF* tbr = (TREE_BLOCK_REF*)data;

                if (secttbr->offset == tbr->offset) {
                    EXTENT_ITEM* newei;
                    uint16_t neweilen;

                    if (ei->refcount == 1) {
                        Status = delete_tree_item(Vcb, &tp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_tree_item returned %08lx\n", Status);
                            return Status;
                        }

                        return STATUS_SUCCESS;
                    }

                    neweilen = tp.item->size - sizeof(uint8_t) - sectlen;

                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(newei, ei, ptr - tp.item->data);

                    if (len > sectlen)
                        RtlCopyMemory((uint8_t*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(uint8_t), len - sectlen);

                    newei->refcount--;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else if (type == TYPE_SHARED_BLOCK_REF) {
                SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(uint8_t));
                SHARED_BLOCK_REF* sbr = (SHARED_BLOCK_REF*)data;

                if (sectsbr->offset == sbr->offset) {
                    EXTENT_ITEM* newei;
                    uint16_t neweilen;

                    if (ei->refcount == 1) {
                        Status = delete_tree_item(Vcb, &tp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_tree_item returned %08lx\n", Status);
                            return Status;
                        }

                        return STATUS_SUCCESS;
                    }

                    neweilen = tp.item->size - sizeof(uint8_t) - sectlen;

                    newei = ExAllocatePoolWithTag(PagedPool, neweilen, ALLOC_TAG);
                    if (!newei) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(newei, ei, ptr - tp.item->data);

                    if (len > sectlen)
                        RtlCopyMemory((uint8_t*)newei + (ptr - tp.item->data), ptr + sectlen + sizeof(uint8_t), len - sectlen);

                    newei->refcount--;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, neweilen, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_tree_item returned %08lx\n", Status);
                        return Status;
                    }

                    return STATUS_SUCCESS;
                }
            } else {
                ERR("unhandled extent type %x\n", type);
                return STATUS_INTERNAL_ERROR;
            }
        }

        len -= sectlen;
        ptr += sizeof(uint8_t) + sectlen;
        inline_rc += sectcount;
    }

    if (inline_rc == ei->refcount) {
        ERR("entry not found in inline extent item for address %I64x\n", address);
        return STATUS_INTERNAL_ERROR;
    }

    if (type == TYPE_SHARED_DATA_REF)
        datalen = sizeof(uint32_t);
    else if (type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF)
        datalen = 0;

    searchkey.obj_id = address;
    searchkey.obj_type = type;
    searchkey.offset = (type == TYPE_SHARED_DATA_REF || type == TYPE_EXTENT_REF_V0) ? parent : get_extent_hash(type, data);

    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    if (keycmp(tp2.item->key, searchkey)) {
        ERR("(%I64x,%x,%I64x) not found\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset);
        return STATUS_INTERNAL_ERROR;
    }

    if (tp2.item->size < datalen) {
        ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %lu\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, tp2.item->size, datalen);
        return STATUS_INTERNAL_ERROR;
    }

    if (type == TYPE_EXTENT_DATA_REF) {
        EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)tp2.item->data;
        EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;

        if (sectedr->root == edr->root && sectedr->objid == edr->objid && sectedr->offset == edr->offset) {
            EXTENT_ITEM* newei;

            if (ei->refcount == edr->count) {
                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08lx\n", Status);
                    return Status;
                }

                Status = delete_tree_item(Vcb, &tp2);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08lx\n", Status);
                    return Status;
                }

                if (!superseded)
                    add_checksum_entry(Vcb, address, (ULONG)(size >> Vcb->sector_shift), NULL, Irp);

                return STATUS_SUCCESS;
            }

            if (sectedr->count < edr->count) {
                ERR("error - extent section has refcount %x, trying to reduce by %x\n", sectedr->count, edr->count);
                return STATUS_INTERNAL_ERROR;
            }

            Status = delete_tree_item(Vcb, &tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            if (sectedr->count > edr->count) {
                EXTENT_DATA_REF* newedr = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);

                if (!newedr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(newedr, sectedr, tp2.item->size);

                newedr->count -= edr->count;

                Status = insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, newedr, tp2.item->size, NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08lx\n", Status);
                    return Status;
                }
            }

            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                return Status;
            }

            return STATUS_SUCCESS;
        } else {
            ERR("error - hash collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_SHARED_DATA_REF) {
        SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;

        if (tp2.item->key.offset == sdr->offset) {
            uint32_t* sectsdrcount = (uint32_t*)tp2.item->data;
            EXTENT_ITEM* newei;

            if (ei->refcount == sdr->count) {
                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08lx\n", Status);
                    return Status;
                }

                Status = delete_tree_item(Vcb, &tp2);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08lx\n", Status);
                    return Status;
                }

                if (!superseded)
                    add_checksum_entry(Vcb, address, (ULONG)(size >> Vcb->sector_shift), NULL, Irp);

                return STATUS_SUCCESS;
            }

            if (*sectsdrcount < sdr->count) {
                ERR("error - extent section has refcount %x, trying to reduce by %x\n", *sectsdrcount, sdr->count);
                return STATUS_INTERNAL_ERROR;
            }

            Status = delete_tree_item(Vcb, &tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            if (*sectsdrcount > sdr->count) {
                uint32_t* newsdr = ExAllocatePoolWithTag(PagedPool, tp2.item->size, ALLOC_TAG);

                if (!newsdr) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                *newsdr = *sectsdrcount - sdr->count;

                Status = insert_tree_item(Vcb, Vcb->extent_root, tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset, newsdr, tp2.item->size, NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08lx\n", Status);
                    return Status;
                }
            }

            newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!newei) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(newei, tp.item->data, tp.item->size);

            newei->refcount -= rc;

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08lx\n", Status);
                return Status;
            }

            return STATUS_SUCCESS;
        } else {
            ERR("error - collision?\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else if (type == TYPE_TREE_BLOCK_REF || type == TYPE_SHARED_BLOCK_REF) {
        EXTENT_ITEM* newei;

        if (ei->refcount == 1) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = delete_tree_item(Vcb, &tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            return STATUS_SUCCESS;
        }

        Status = delete_tree_item(Vcb, &tp2);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            return Status;
        }

        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
        if (!newei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newei, tp.item->data, tp.item->size);

        newei->refcount -= rc;

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            return Status;
        }

        return STATUS_SUCCESS;
    } else if (type == TYPE_EXTENT_REF_V0) {
        EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)tp2.item->data;
        EXTENT_ITEM* newei;

        if (ei->refcount == erv0->count) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            Status = delete_tree_item(Vcb, &tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08lx\n", Status);
                return Status;
            }

            if (!superseded)
                add_checksum_entry(Vcb, address, (ULONG)(size >> Vcb->sector_shift), NULL, Irp);

            return STATUS_SUCCESS;
        }

        Status = delete_tree_item(Vcb, &tp2);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            return Status;
        }

        newei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
        if (!newei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newei, tp.item->data, tp.item->size);

        newei->refcount -= rc;

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newei, tp.item->size, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            return Status;
        }

        return STATUS_SUCCESS;
    } else {
        ERR("unhandled extent type %x\n", type);
        return STATUS_INTERNAL_ERROR;
    }
}

NTSTATUS decrease_extent_refcount_data(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint64_t inode,
                                       uint64_t offset, uint32_t refcount, bool superseded, PIRP Irp) {
    EXTENT_DATA_REF edr;

    edr.root = root;
    edr.objid = inode;
    edr.offset = offset;
    edr.count = refcount;

    return decrease_extent_refcount(Vcb, address, size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, 0, superseded, Irp);
}

NTSTATUS decrease_extent_refcount_tree(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root,
                                       uint8_t level, PIRP Irp) {
    TREE_BLOCK_REF tbr;

    tbr.offset = root;

    return decrease_extent_refcount(Vcb, address, size, TYPE_TREE_BLOCK_REF, &tbr, NULL/*FIXME*/, level, 0, false, Irp);
}

static uint32_t find_extent_data_refcount(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint64_t objid, uint64_t offset, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        TRACE("could not find address %I64x in extent tree\n", address);
        return 0;
    }

    if (tp.item->key.offset != size) {
        ERR("extent %I64x had size %I64x, not %I64x as expected\n", address, tp.item->key.offset, size);
        return 0;
    }

    if (tp.item->size >= sizeof(EXTENT_ITEM)) {
        EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;
        uint32_t len = tp.item->size - sizeof(EXTENT_ITEM);
        uint8_t* ptr = (uint8_t*)&ei[1];

        while (len > 0) {
            uint8_t secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);
            uint32_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

            len--;

            if (sectlen > len) {
                ERR("(%I64x,%x,%I64x): %x bytes left, expecting at least %lx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
                return 0;
            }

            if (sectlen == 0) {
                ERR("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
                return 0;
            }

            if (secttype == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(uint8_t));

                if (sectedr->root == root && sectedr->objid == objid && sectedr->offset == offset)
                    return sectcount;
            }

            len -= sectlen;
            ptr += sizeof(uint8_t) + sectlen;
        }
    }

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_DATA_REF;
    searchkey.offset = get_extent_data_ref_hash2(root, objid, offset);

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        if (tp.item->size < sizeof(EXTENT_DATA_REF))
            ERR("(%I64x,%x,%I64x) has size %u, not %Iu as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA_REF));
        else {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)tp.item->data;

            return edr->count;
        }
    }

    return 0;
}

uint64_t get_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;

        return ei->refcount;
    }

    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find (%I64x,%x,%I64x) in extent tree\n", address, TYPE_EXTENT_ITEM, size);
        return 0;
    } else if (tp.item->key.offset != size) {
        ERR("extent %I64x had size %I64x, not %I64x as expected\n", address, tp.item->key.offset, size);
        return 0;
    }

    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        EXTENT_ITEM_V0* eiv0 = (EXTENT_ITEM_V0*)tp.item->data;

        return eiv0->refcount;
    } else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %x bytes, expected at least %Ix\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                      tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    return ei->refcount;
}

bool is_extent_unique(device_extension* Vcb, uint64_t address, uint64_t size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    uint64_t rc, rcrun, root = 0, inode = 0, offset = 0;
    uint32_t len;
    EXTENT_ITEM* ei;
    uint8_t* ptr;
    bool b;

    rc = get_extent_refcount(Vcb, address, size, Irp);

    if (rc == 1)
        return true;

    if (rc == 0)
        return false;

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        WARN("error - find_item returned %08lx\n", Status);
        return false;
    }

    if (keycmp(tp.item->key, searchkey)) {
        WARN("could not find (%I64x,%x,%I64x)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return false;
    }

    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return false;

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        WARN("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return false;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (uint8_t*)&ei[1];

    if (ei->flags & EXTENT_ITEM_TREE_BLOCK) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            WARN("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return false;
        }

        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }

    rcrun = 0;

    // Loop through inline extent entries

    while (len > 0) {
        uint8_t secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        uint64_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

        len--;

        if (sectlen > len) {
            WARN("(%I64x,%x,%I64x): %x bytes left, expecting at least %lx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return false;
        }

        if (sectlen == 0) {
            WARN("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return false;
        }

        if (secttype == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(uint8_t));

            if (root == 0 && inode == 0) {
                root = sectedr->root;
                inode = sectedr->objid;
                offset = sectedr->offset;
            } else if (root != sectedr->root || inode != sectedr->objid || offset != sectedr->offset)
                return false;
        } else
            return false;

        len -= sectlen;
        ptr += sizeof(uint8_t) + sectlen;
        rcrun += sectcount;
    }

    if (rcrun == rc)
        return true;

    // Loop through non-inlines if some refs still unaccounted for

    do {
        b = find_next_item(Vcb, &tp, &next_tp, false, Irp);

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_EXTENT_DATA_REF) {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)tp.item->data;

            if (tp.item->size < sizeof(EXTENT_DATA_REF)) {
                WARN("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                     tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
                return false;
            }

            if (root == 0 && inode == 0) {
                root = edr->root;
                inode = edr->objid;
                offset = edr->offset;
            } else if (root != edr->root || inode != edr->objid || offset != edr->offset)
                return false;

            rcrun += edr->count;
        }

        if (rcrun == rc)
            return true;

        if (b) {
            tp = next_tp;

            if (tp.item->key.obj_id > searchkey.obj_id)
                break;
        }
    } while (b);

    // If we reach this point, there's still some refs unaccounted for somewhere.
    // Return false in case we mess things up elsewhere.

    return false;
}

uint64_t get_extent_flags(device_extension* Vcb, uint64_t address, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;

        return ei->flags;
    }

    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find %I64x in extent tree\n", address);
        return 0;
    }

    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return 0;
    else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %x bytes, expected at least %Ix\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                      tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    return ei->flags;
}

void update_extent_flags(device_extension* Vcb, uint64_t address, uint64_t flags, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    EXTENT_ITEM* ei;

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return;
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA && tp.item->key.obj_id == address &&
        tp.item->key.obj_type == TYPE_METADATA_ITEM && tp.item->size >= sizeof(EXTENT_ITEM)) {
        ei = (EXTENT_ITEM*)tp.item->data;
        ei->flags = flags;
        return;
    }

    if (tp.item->key.obj_id != address || tp.item->key.obj_type != TYPE_EXTENT_ITEM) {
        ERR("couldn't find %I64x in extent tree\n", address);
        return;
    }

    if (tp.item->size == sizeof(EXTENT_ITEM_V0))
        return;
    else if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %x bytes, expected at least %Ix\n", tp.item->key.obj_id, tp.item->key.obj_type,
                                                                      tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return;
    }

    ei = (EXTENT_ITEM*)tp.item->data;
    ei->flags = flags;
}

static changed_extent* get_changed_extent_item(chunk* c, uint64_t address, uint64_t size, bool no_csum) {
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
    ce->superseded = false;
    InitializeListHead(&ce->refs);
    InitializeListHead(&ce->old_refs);

    InsertTailList(&c->changed_extents, &ce->list_entry);

    return ce;
}

NTSTATUS update_changed_extent_ref(device_extension* Vcb, chunk* c, uint64_t address, uint64_t size, uint64_t root, uint64_t objid, uint64_t offset, int32_t count,
                                   bool no_csum, bool superseded, PIRP Irp) {
    LIST_ENTRY* le;
    changed_extent* ce;
    changed_extent_ref* cer;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    uint32_t old_count;

    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, true);

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

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            goto end;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("could not find address %I64x in extent tree\n", address);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }

        if (tp.item->key.offset != size) {
            ERR("extent %I64x had size %I64x, not %I64x as expected\n", address, tp.item->key.offset, size);
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
            ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
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
                ce->superseded = true;

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
        ce->superseded = true;

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&c->changed_extents_lock);

    return Status;
}

void add_changed_extent_ref(chunk* c, uint64_t address, uint64_t size, uint64_t root, uint64_t objid, uint64_t offset, uint32_t count, bool no_csum) {
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

uint64_t find_extent_shared_tree_refcount(device_extension* Vcb, uint64_t address, uint64_t parent, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    uint64_t inline_rc;
    EXTENT_ITEM* ei;
    uint32_t len;
    uint8_t* ptr;

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        TRACE("could not find address %I64x in extent tree\n", address);
        return 0;
    }

    if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->key.offset != Vcb->superblock.node_size) {
        ERR("extent %I64x had size %I64x, not %x as expected\n", address, tp.item->key.offset, Vcb->superblock.node_size);
        return 0;
    }

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x): size was %u, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }

    ei = (EXTENT_ITEM*)tp.item->data;
    inline_rc = 0;

    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (uint8_t*)&ei[1];

    if (searchkey.obj_type == TYPE_EXTENT_ITEM && ei->flags & EXTENT_ITEM_TREE_BLOCK) {
        if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2)) {
            ERR("(%I64x,%x,%I64x): size was %u, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                                                                          tp.item->size, sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2));
            return 0;
        }

        len -= sizeof(EXTENT_ITEM2);
        ptr += sizeof(EXTENT_ITEM2);
    }

    while (len > 0) {
        uint8_t secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        uint64_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

        len--;

        if (sectlen > len) {
            ERR("(%I64x,%x,%I64x): %x bytes left, expecting at least %lx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return 0;
        }

        if (sectlen == 0) {
            ERR("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return 0;
        }

        if (secttype == TYPE_SHARED_BLOCK_REF) {
            SHARED_BLOCK_REF* sectsbr = (SHARED_BLOCK_REF*)(ptr + sizeof(uint8_t));

            if (sectsbr->offset == parent)
                return 1;
        }

        len -= sectlen;
        ptr += sizeof(uint8_t) + sectlen;
        inline_rc += sectcount;
    }

    // FIXME - what if old?

    if (inline_rc == ei->refcount)
        return 0;

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_SHARED_BLOCK_REF;
    searchkey.offset = parent;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (!keycmp(searchkey, tp.item->key))
        return 1;

    return 0;
}

uint32_t find_extent_shared_data_refcount(device_extension* Vcb, uint64_t address, uint64_t parent, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    uint64_t inline_rc;
    EXTENT_ITEM* ei;
    uint32_t len;
    uint8_t* ptr;

    searchkey.obj_id = address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_EXTENT_ITEM && tp.item->key.obj_type != TYPE_METADATA_ITEM)) {
        TRACE("could not find address %I64x in extent tree\n", address);
        return 0;
    }

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%I64x,%x,%I64x): size was %u, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }

    ei = (EXTENT_ITEM*)tp.item->data;
    inline_rc = 0;

    len = tp.item->size - sizeof(EXTENT_ITEM);
    ptr = (uint8_t*)&ei[1];

    while (len > 0) {
        uint8_t secttype = *ptr;
        ULONG sectlen = get_extent_data_len(secttype);
        uint64_t sectcount = get_extent_data_refcount(secttype, ptr + sizeof(uint8_t));

        len--;

        if (sectlen > len) {
            ERR("(%I64x,%x,%I64x): %x bytes left, expecting at least %lx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
            return 0;
        }

        if (sectlen == 0) {
            ERR("(%I64x,%x,%I64x): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
            return 0;
        }

        if (secttype == TYPE_SHARED_DATA_REF) {
            SHARED_DATA_REF* sectsdr = (SHARED_DATA_REF*)(ptr + sizeof(uint8_t));

            if (sectsdr->offset == parent)
                return sectsdr->count;
        }

        len -= sectlen;
        ptr += sizeof(uint8_t) + sectlen;
        inline_rc += sectcount;
    }

    // FIXME - what if old?

    if (inline_rc == ei->refcount)
        return 0;

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_SHARED_DATA_REF;
    searchkey.offset = parent;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return 0;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        if (tp.item->size < sizeof(uint32_t))
            ERR("(%I64x,%x,%I64x) has size %u, not %Iu as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(uint32_t));
        else {
            uint32_t* count = (uint32_t*)tp.item->data;
            return *count;
        }
    }

    return 0;
}
