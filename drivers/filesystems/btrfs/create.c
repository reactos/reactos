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

#ifndef __REACTOS__
#include <sys/stat.h>
#endif /* __REACTOS__ */
#include "btrfs_drv.h"
#include <ntddstor.h>

extern PDEVICE_OBJECT master_devobj;

static WCHAR datastring[] = L"::$DATA";

fcb* create_fcb(device_extension* Vcb, POOL_TYPE pool_type) {
    fcb* fcb;

    if (pool_type == NonPagedPool) {
        fcb = ExAllocatePoolWithTag(pool_type, sizeof(struct _fcb), ALLOC_TAG);
        if (!fcb) {
            ERR("out of memory\n");
            return NULL;
        }
    } else {
        fcb = ExAllocateFromPagedLookasideList(&Vcb->fcb_lookaside);
        if (!fcb) {
            ERR("out of memory\n");
            return NULL;
        }
    }

#ifdef DEBUG_FCB_REFCOUNTS
    WARN("allocating fcb %p\n", fcb);
#endif
    RtlZeroMemory(fcb, sizeof(struct _fcb));
    fcb->pool_type = pool_type;

    fcb->Header.NodeTypeCode = BTRFS_NODE_TYPE_FCB;
    fcb->Header.NodeByteSize = sizeof(struct _fcb);

    fcb->nonpaged = ExAllocateFromNPagedLookasideList(&Vcb->fcb_np_lookaside);
    if (!fcb->nonpaged) {
        ERR("out of memory\n");

        if (pool_type == NonPagedPool)
            ExFreePool(fcb);
        else
            ExFreeToPagedLookasideList(&Vcb->fcb_lookaside, fcb);

        return NULL;
    }
    RtlZeroMemory(fcb->nonpaged, sizeof(struct _fcb_nonpaged));

    ExInitializeResourceLite(&fcb->nonpaged->paging_resource);
    fcb->Header.PagingIoResource = &fcb->nonpaged->paging_resource;

    ExInitializeFastMutex(&fcb->nonpaged->HeaderMutex);
    FsRtlSetupAdvancedHeader(&fcb->Header, &fcb->nonpaged->HeaderMutex);

    fcb->refcount = 1;
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fcb %p: refcount now %i\n", fcb, fcb->refcount);
#endif

    ExInitializeResourceLite(&fcb->nonpaged->resource);
    fcb->Header.Resource = &fcb->nonpaged->resource;

    ExInitializeResourceLite(&fcb->nonpaged->dir_children_lock);

    FsRtlInitializeFileLock(&fcb->lock, NULL, NULL);

    InitializeListHead(&fcb->extents);
    InitializeListHead(&fcb->hardlinks);
    InitializeListHead(&fcb->xattrs);

    InitializeListHead(&fcb->dir_children_index);
    InitializeListHead(&fcb->dir_children_hash);
    InitializeListHead(&fcb->dir_children_hash_uc);

    return fcb;
}

file_ref* create_fileref(device_extension* Vcb) {
    file_ref* fr;

    fr = ExAllocateFromPagedLookasideList(&Vcb->fileref_lookaside);
    if (!fr) {
        ERR("out of memory\n");
        return NULL;
    }

    RtlZeroMemory(fr, sizeof(file_ref));

    fr->nonpaged = ExAllocateFromNPagedLookasideList(&Vcb->fileref_np_lookaside);
    if (!fr->nonpaged) {
        ERR("out of memory\n");
        ExFreeToPagedLookasideList(&Vcb->fileref_lookaside, fr);
        return NULL;
    }

    fr->refcount = 1;

#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fileref %p: refcount now 1\n", fr);
#endif

    InitializeListHead(&fr->children);

    ExInitializeResourceLite(&fr->nonpaged->fileref_lock);
    ExInitializeResourceLite(&fr->nonpaged->children_lock);

    return fr;
}

NTSTATUS find_file_in_dir(PUNICODE_STRING filename, fcb* fcb, root** subvol, UINT64* inode, dir_child** pdc, BOOL case_sensitive) {
    NTSTATUS Status;
    UNICODE_STRING fnus;
    UINT32 hash;
    LIST_ENTRY* le;
    UINT8 c;
    BOOL locked = FALSE;

    if (!case_sensitive) {
        Status = RtlUpcaseUnicodeString(&fnus, filename, TRUE);

        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            return Status;
        }
    } else
        fnus = *filename;

    hash = calc_crc32c(0xffffffff, (UINT8*)fnus.Buffer, fnus.Length);

    c = hash >> 24;

    if (!ExIsResourceAcquiredSharedLite(&fcb->nonpaged->dir_children_lock)) {
        ExAcquireResourceSharedLite(&fcb->nonpaged->dir_children_lock, TRUE);
        locked = TRUE;
    }

    if (case_sensitive) {
        if (!fcb->hash_ptrs[c]) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }

        le = fcb->hash_ptrs[c];
        while (le != &fcb->dir_children_hash) {
            dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_hash);

            if (dc->hash == hash) {
                if (dc->name.Length == fnus.Length && RtlCompareMemory(dc->name.Buffer, fnus.Buffer, fnus.Length) == fnus.Length) {
                    if (dc->key.obj_type == TYPE_ROOT_ITEM) {
                        LIST_ENTRY* le2;

                        *subvol = NULL;

                        le2 = fcb->Vcb->roots.Flink;
                        while (le2 != &fcb->Vcb->roots) {
                            root* r2 = CONTAINING_RECORD(le2, root, list_entry);

                            if (r2->id == dc->key.obj_id) {
                                *subvol = r2;
                                break;
                            }

                            le2 = le2->Flink;
                        }

                        *inode = SUBVOL_ROOT_INODE;
                    } else {
                        *subvol = fcb->subvol;
                        *inode = dc->key.obj_id;
                    }

                    *pdc = dc;

                    Status = STATUS_SUCCESS;
                    goto end;
                }
            } else if (dc->hash > hash) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto end;
            }

            le = le->Flink;
        }
    } else {
        if (!fcb->hash_ptrs_uc[c]) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }

        le = fcb->hash_ptrs_uc[c];
        while (le != &fcb->dir_children_hash_uc) {
            dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_hash_uc);

            if (dc->hash_uc == hash) {
                if (dc->name_uc.Length == fnus.Length && RtlCompareMemory(dc->name_uc.Buffer, fnus.Buffer, fnus.Length) == fnus.Length) {
                    if (dc->key.obj_type == TYPE_ROOT_ITEM) {
                        LIST_ENTRY* le2;

                        *subvol = NULL;

                        le2 = fcb->Vcb->roots.Flink;
                        while (le2 != &fcb->Vcb->roots) {
                            root* r2 = CONTAINING_RECORD(le2, root, list_entry);

                            if (r2->id == dc->key.obj_id) {
                                *subvol = r2;
                                break;
                            }

                            le2 = le2->Flink;
                        }

                        *inode = SUBVOL_ROOT_INODE;
                    } else {
                        *subvol = fcb->subvol;
                        *inode = dc->key.obj_id;
                    }

                    *pdc = dc;

                    Status = STATUS_SUCCESS;
                    goto end;
                }
            } else if (dc->hash_uc > hash) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto end;
            }

            le = le->Flink;
        }
    }

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

end:
    if (locked)
        ExReleaseResourceLite(&fcb->nonpaged->dir_children_lock);

    if (!case_sensitive)
        ExFreePool(fnus.Buffer);

    return Status;
}

static NTSTATUS split_path(device_extension* Vcb, PUNICODE_STRING path, LIST_ENTRY* parts, BOOL* stream) {
    ULONG len, i;
    BOOL has_stream;
    WCHAR* buf;
    name_bit* nb;

    len = path->Length / sizeof(WCHAR);
    if (len > 0 && (path->Buffer[len - 1] == '/' || path->Buffer[len - 1] == '\\'))
        len--;

    has_stream = FALSE;
    for (i = 0; i < len; i++) {
        if (path->Buffer[i] == '/' || path->Buffer[i] == '\\') {
            has_stream = FALSE;
        } else if (path->Buffer[i] == ':') {
            has_stream = TRUE;
        }
    }

    buf = path->Buffer;

    for (i = 0; i < len; i++) {
        if (path->Buffer[i] == '/' || path->Buffer[i] == '\\') {
            nb = ExAllocateFromPagedLookasideList(&Vcb->name_bit_lookaside);
            if (!nb) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            nb->us.Buffer = buf;
            nb->us.Length = nb->us.MaximumLength = (USHORT)(&path->Buffer[i] - buf) * sizeof(WCHAR);
            InsertTailList(parts, &nb->list_entry);

            buf = &path->Buffer[i+1];
        }
    }

    nb = ExAllocateFromPagedLookasideList(&Vcb->name_bit_lookaside);
    if (!nb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nb->us.Buffer = buf;
    nb->us.Length = nb->us.MaximumLength = (USHORT)(&path->Buffer[i] - buf) * sizeof(WCHAR);
    InsertTailList(parts, &nb->list_entry);

    if (has_stream) {
        static WCHAR datasuf[] = {':','$','D','A','T','A',0};
        UNICODE_STRING dsus;

        dsus.Buffer = datasuf;
        dsus.Length = dsus.MaximumLength = (UINT16)wcslen(datasuf) * sizeof(WCHAR);

        for (i = 0; i < nb->us.Length / sizeof(WCHAR); i++) {
            if (nb->us.Buffer[i] == ':') {
                name_bit* nb2;

                nb2 = ExAllocateFromPagedLookasideList(&Vcb->name_bit_lookaside);
                if (!nb2) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                nb2->us.Buffer = &nb->us.Buffer[i+1];
                nb2->us.Length = nb2->us.MaximumLength = (UINT16)(nb->us.Length - (i * sizeof(WCHAR)) - sizeof(WCHAR));
                InsertTailList(parts, &nb2->list_entry);

                nb->us.Length = (UINT16)i * sizeof(WCHAR);
                nb->us.MaximumLength = nb->us.Length;

                nb = nb2;

                break;
            }
        }

        // FIXME - should comparison be case-insensitive?
        // remove :$DATA suffix
        if (nb->us.Length >= dsus.Length && RtlCompareMemory(&nb->us.Buffer[(nb->us.Length - dsus.Length)/sizeof(WCHAR)], dsus.Buffer, dsus.Length) == dsus.Length)
            nb->us.Length -= dsus.Length;

        if (nb->us.Length == 0) {
            RemoveTailList(parts);
            ExFreeToPagedLookasideList(&Vcb->name_bit_lookaside, nb);

            has_stream = FALSE;
        }
    }

    // if path is just stream name, remove first empty item
    if (has_stream && path->Length >= sizeof(WCHAR) && path->Buffer[0] == ':') {
        name_bit *nb1 = CONTAINING_RECORD(RemoveHeadList(parts), name_bit, list_entry);

        ExFreeToPagedLookasideList(&Vcb->name_bit_lookaside, nb1);
    }

    *stream = has_stream;

    return STATUS_SUCCESS;
}

NTSTATUS load_csum(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, UINT32* csum, UINT64 start, UINT64 length, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    UINT64 i, j;
    BOOL b;

    searchkey.obj_id = EXTENT_CSUM_ID;
    searchkey.obj_type = TYPE_EXTENT_CSUM;
    searchkey.offset = start;

    Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    i = 0;
    do {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            ULONG readlen;

            if (start < tp.item->key.offset)
                j = 0;
            else
                j = ((start - tp.item->key.offset) / Vcb->superblock.sector_size) + i;

            if (j * sizeof(UINT32) > tp.item->size || tp.item->key.offset > start + (i * Vcb->superblock.sector_size)) {
                ERR("checksum not found for %llx\n", start + (i * Vcb->superblock.sector_size));
                return STATUS_INTERNAL_ERROR;
            }

            readlen = (ULONG)min((tp.item->size / sizeof(UINT32)) - j, length - i);
            RtlCopyMemory(&csum[i], tp.item->data + (j * sizeof(UINT32)), readlen * sizeof(UINT32));
            i += readlen;

            if (i == length)
                break;
        }

        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);

        if (b)
            tp = next_tp;
    } while (b);

    if (i < length) {
        ERR("could not read checksums: offset %llx, length %llx sectors\n", start, length);
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS load_dir_children(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, fcb* fcb, BOOL ignore_size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    ULONG num_children = 0;

    fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fcb->hash_ptrs) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);

    fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fcb->hash_ptrs_uc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);

    if (!ignore_size && fcb->inode_item.st_size == 0)
        return STATUS_SUCCESS;

    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = 2;

    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey) == -1) {
        if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
            tp = next_tp;
            TRACE("moving on to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }

    while (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
        dir_child* dc;
        ULONG utf16len;

        if (tp.item->size < sizeof(DIR_ITEM)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            goto cont;
        }

        if (di->n == 0) {
            WARN("(%llx,%x,%llx): DIR_ITEM name length is zero\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            goto cont;
        }

        Status = RtlUTF8ToUnicodeN(NULL, 0, &utf16len, di->name, di->n);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
            goto cont;
        }

        dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
        if (!dc) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        dc->key = di->key;
        dc->index = tp.item->key.offset;
        dc->type = di->type;
        dc->fileref = NULL;

        dc->utf8.MaximumLength = dc->utf8.Length = di->n;
        dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, di->n, ALLOC_TAG);
        if (!dc->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(dc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(dc->utf8.Buffer, di->name, di->n);

        dc->name.MaximumLength = dc->name.Length = (UINT16)utf16len;
        dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, dc->name.MaximumLength, ALLOC_TAG);
        if (!dc->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = RtlUTF8ToUnicodeN(dc->name.Buffer, utf16len, &utf16len, di->name, di->n);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc->name.Buffer);
            ExFreePool(dc);
            goto cont;
        }

        Status = RtlUpcaseUnicodeString(&dc->name_uc, &dc->name, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc->name.Buffer);
            ExFreePool(dc);
            goto cont;
        }

        dc->hash = calc_crc32c(0xffffffff, (UINT8*)dc->name.Buffer, dc->name.Length);
        dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)dc->name_uc.Buffer, dc->name_uc.Length);

        InsertTailList(&fcb->dir_children_index, &dc->list_entry_index);

        insert_dir_child_into_hash_lists(fcb, dc);

        num_children++;

cont:
        if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp))
            tp = next_tp;
        else
            break;
    }

    // If a directory has a lot of files, force it to stick around until the next flush
    // so we aren't constantly re-reading.
    if (num_children >= 100)
        mark_fcb_dirty(fcb);

    return STATUS_SUCCESS;
}

NTSTATUS open_fcb(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                  root* subvol, UINT64 inode, UINT8 type, PANSI_STRING utf8, fcb* parent, fcb** pfcb, POOL_TYPE pooltype, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    fcb *fcb, *deleted_fcb = NULL;
    BOOL atts_set = FALSE, sd_set = FALSE, no_data;
    LIST_ENTRY* lastle = NULL;
    EXTENT_DATA* ed = NULL;

    if (!IsListEmpty(&subvol->fcbs)) {
        LIST_ENTRY* le = subvol->fcbs.Flink;

        while (le != &subvol->fcbs) {
            fcb = CONTAINING_RECORD(le, struct _fcb, list_entry);

            if (fcb->inode == inode) {
                if (!fcb->ads) {
                    if (fcb->deleted)
                        deleted_fcb = fcb;
                    else {
#ifdef DEBUG_FCB_REFCOUNTS
                        LONG rc = InterlockedIncrement(&fcb->refcount);

                        WARN("fcb %p: refcount now %i (subvol %llx, inode %llx)\n", fcb, rc, fcb->subvol->id, fcb->inode);
#else
                        InterlockedIncrement(&fcb->refcount);
#endif

                        *pfcb = fcb;
                        return STATUS_SUCCESS;
                    }
                }
            } else if (fcb->inode > inode) {
                if (deleted_fcb) {
                    InterlockedIncrement(&deleted_fcb->refcount);
                    *pfcb = deleted_fcb;
                    return STATUS_SUCCESS;
                }

                lastle = le->Blink;
                break;
            }

            le = le->Flink;
        }
    }

    if (deleted_fcb) {
        InterlockedIncrement(&deleted_fcb->refcount);
        *pfcb = deleted_fcb;
        return STATUS_SUCCESS;
    }

    fcb = create_fcb(Vcb, pooltype);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fcb->Vcb = Vcb;

    fcb->subvol = subvol;
    fcb->inode = inode;
    fcb->type = type;

    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        free_fcb(Vcb, fcb);
        return Status;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        WARN("couldn't find INODE_ITEM for inode %llx in subvol %llx\n", inode, subvol->id);
        free_fcb(Vcb, fcb);
        return STATUS_INVALID_PARAMETER;
    }

    if (tp.item->size > 0)
        RtlCopyMemory(&fcb->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));

    if (fcb->type == 0) { // guess the type from the inode mode, if the caller doesn't know already
        if ((fcb->inode_item.st_mode & __S_IFDIR) == __S_IFDIR)
            fcb->type = BTRFS_TYPE_DIRECTORY;
        else if ((fcb->inode_item.st_mode & __S_IFCHR) == __S_IFCHR)
            fcb->type = BTRFS_TYPE_CHARDEV;
        else if ((fcb->inode_item.st_mode & __S_IFBLK) == __S_IFBLK)
            fcb->type = BTRFS_TYPE_BLOCKDEV;
        else if ((fcb->inode_item.st_mode & __S_IFIFO) == __S_IFIFO)
            fcb->type = BTRFS_TYPE_FIFO;
        else if ((fcb->inode_item.st_mode & __S_IFLNK) == __S_IFLNK)
            fcb->type = BTRFS_TYPE_SYMLINK;
        else if ((fcb->inode_item.st_mode & __S_IFSOCK) == __S_IFSOCK)
            fcb->type = BTRFS_TYPE_SOCKET;
        else
            fcb->type = BTRFS_TYPE_FILE;
    }

    no_data = fcb->inode_item.st_size == 0 || (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK);

    while (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
        tp = next_tp;

        if (tp.item->key.obj_id > inode)
            break;

        if ((no_data && tp.item->key.obj_type > TYPE_XATTR_ITEM) || tp.item->key.obj_type > TYPE_EXTENT_DATA)
            break;

        if (fcb->inode_item.st_nlink > 1 && tp.item->key.obj_type == TYPE_INODE_REF) {
            ULONG len;
            INODE_REF* ir;

            len = tp.item->size;
            ir = (INODE_REF*)tp.item->data;

            while (len >= sizeof(INODE_REF) - 1) {
                hardlink* hl;
                ULONG stringlen;

                hl = ExAllocatePoolWithTag(pooltype, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(Vcb, fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                hl->parent = tp.item->key.offset;
                hl->index = ir->index;

                hl->utf8.Length = hl->utf8.MaximumLength = ir->n;

                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(pooltype, hl->utf8.MaximumLength, ALLOC_TAG);
                    RtlCopyMemory(hl->utf8.Buffer, ir->name, ir->n);
                }

                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, ir->name, ir->n);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                    ExFreePool(hl);
                    free_fcb(Vcb, fcb);
                    return Status;
                }

                hl->name.Length = hl->name.MaximumLength = (UINT16)stringlen;

                if (stringlen == 0)
                    hl->name.Buffer = NULL;
                else {
                    hl->name.Buffer = ExAllocatePoolWithTag(pooltype, hl->name.MaximumLength, ALLOC_TAG);

                    if (!hl->name.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(hl);
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    Status = RtlUTF8ToUnicodeN(hl->name.Buffer, stringlen, &stringlen, ir->name, ir->n);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        ExFreePool(hl->name.Buffer);
                        ExFreePool(hl);
                        free_fcb(Vcb, fcb);
                        return Status;
                    }
                }

                InsertTailList(&fcb->hardlinks, &hl->list_entry);

                len -= sizeof(INODE_REF) - 1 + ir->n;
                ir = (INODE_REF*)&ir->name[ir->n];
            }
        } else if (fcb->inode_item.st_nlink > 1 && tp.item->key.obj_type == TYPE_INODE_EXTREF) {
            ULONG len;
            INODE_EXTREF* ier;

            len = tp.item->size;
            ier = (INODE_EXTREF*)tp.item->data;

            while (len >= sizeof(INODE_EXTREF) - 1) {
                hardlink* hl;
                ULONG stringlen;

                hl = ExAllocatePoolWithTag(pooltype, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(Vcb, fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                hl->parent = ier->dir;
                hl->index = ier->index;

                hl->utf8.Length = hl->utf8.MaximumLength = ier->n;

                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(pooltype, hl->utf8.MaximumLength, ALLOC_TAG);
                    RtlCopyMemory(hl->utf8.Buffer, ier->name, ier->n);
                }

                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, ier->name, ier->n);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                    ExFreePool(hl);
                    free_fcb(Vcb, fcb);
                    return Status;
                }

                hl->name.Length = hl->name.MaximumLength = (UINT16)stringlen;

                if (stringlen == 0)
                    hl->name.Buffer = NULL;
                else {
                    hl->name.Buffer = ExAllocatePoolWithTag(pooltype, hl->name.MaximumLength, ALLOC_TAG);

                    if (!hl->name.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(hl);
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    Status = RtlUTF8ToUnicodeN(hl->name.Buffer, stringlen, &stringlen, ier->name, ier->n);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        ExFreePool(hl->name.Buffer);
                        ExFreePool(hl);
                        free_fcb(Vcb, fcb);
                        return Status;
                    }
                }

                InsertTailList(&fcb->hardlinks, &hl->list_entry);

                len -= sizeof(INODE_EXTREF) - 1 + ier->n;
                ier = (INODE_EXTREF*)&ier->name[ier->n];
            }
        } else if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
            ULONG len;
            DIR_ITEM* di;

            static char xapref[] = "user.";

            if (tp.item->size < offsetof(DIR_ITEM, name[0])) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, offsetof(DIR_ITEM, name[0]));
                continue;
            }

            len = tp.item->size;
            di = (DIR_ITEM*)tp.item->data;

            do {
                if (len < offsetof(DIR_ITEM, name[0]) + di->m + di->n)
                    break;

                if (tp.item->key.offset == EA_REPARSE_HASH && di->n == strlen(EA_REPARSE) && RtlCompareMemory(EA_REPARSE, di->name, di->n) == di->n) {
                    if (di->m > 0) {
                        fcb->reparse_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, di->m, ALLOC_TAG);
                        if (!fcb->reparse_xattr.Buffer) {
                            ERR("out of memory\n");
                            free_fcb(Vcb, fcb);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory(fcb->reparse_xattr.Buffer, &di->name[di->n], di->m);
                    } else
                        fcb->reparse_xattr.Buffer = NULL;

                    fcb->reparse_xattr.Length = fcb->reparse_xattr.MaximumLength = di->m;
                } else if (tp.item->key.offset == EA_EA_HASH && di->n == strlen(EA_EA) && RtlCompareMemory(EA_EA, di->name, di->n) == di->n) {
                    if (di->m > 0) {
                        ULONG offset;

                        Status = IoCheckEaBufferValidity((FILE_FULL_EA_INFORMATION*)&di->name[di->n], di->m, &offset);

                        if (!NT_SUCCESS(Status))
                            WARN("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
                        else {
                            FILE_FULL_EA_INFORMATION* eainfo;

                            fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, di->m, ALLOC_TAG);
                            if (!fcb->ea_xattr.Buffer) {
                                ERR("out of memory\n");
                                free_fcb(Vcb, fcb);
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }

                            RtlCopyMemory(fcb->ea_xattr.Buffer, &di->name[di->n], di->m);

                            fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = di->m;

                            fcb->ealen = 4;

                            // calculate ealen
                            eainfo = (FILE_FULL_EA_INFORMATION*)&di->name[di->n];
                            do {
                                fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;

                                if (eainfo->NextEntryOffset == 0)
                                    break;

                                eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
                            } while (TRUE);
                        }
                    }
                } else if (tp.item->key.offset == EA_DOSATTRIB_HASH && di->n == strlen(EA_DOSATTRIB) && RtlCompareMemory(EA_DOSATTRIB, di->name, di->n) == di->n) {
                    if (di->m > 0) {
                        if (get_file_attributes_from_xattr(&di->name[di->n], di->m, &fcb->atts)) {
                            atts_set = TRUE;

                            if (fcb->type == BTRFS_TYPE_DIRECTORY)
                                fcb->atts |= FILE_ATTRIBUTE_DIRECTORY;
                            else if (fcb->type == BTRFS_TYPE_SYMLINK)
                                fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;

                            if (fcb->type != BTRFS_TYPE_DIRECTORY)
                                fcb->atts &= ~FILE_ATTRIBUTE_DIRECTORY;

                            if (inode == SUBVOL_ROOT_INODE) {
                                if (subvol->root_item.flags & BTRFS_SUBVOL_READONLY)
                                    fcb->atts |= FILE_ATTRIBUTE_READONLY;
                                else
                                    fcb->atts &= ~FILE_ATTRIBUTE_READONLY;
                            }
                        }
                    }
                } else if (tp.item->key.offset == EA_NTACL_HASH && di->n == strlen(EA_NTACL) && RtlCompareMemory(EA_NTACL, di->name, di->n) == di->n) {
                    if (di->m > 0) {
                        fcb->sd = ExAllocatePoolWithTag(PagedPool, di->m, ALLOC_TAG);
                        if (!fcb->sd) {
                            ERR("out of memory\n");
                            free_fcb(Vcb, fcb);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory(fcb->sd, &di->name[di->n], di->m);

                        // We have to test against our copy rather than the source, as RtlValidRelativeSecurityDescriptor
                        // will fail if the ACLs aren't 32-bit aligned.
                        if (!RtlValidRelativeSecurityDescriptor(fcb->sd, di->m, 0))
                            ExFreePool(fcb->sd);
                        else
                            sd_set = TRUE;
                    }
                } else if (tp.item->key.offset == EA_PROP_COMPRESSION_HASH && di->n == strlen(EA_PROP_COMPRESSION) && RtlCompareMemory(EA_PROP_COMPRESSION, di->name, di->n) == di->n) {
                    if (di->m > 0) {
                        const char lzo[] = "lzo";
                        const char zlib[] = "zlib";

                        if (di->m == strlen(lzo) && RtlCompareMemory(&di->name[di->n], lzo, di->m) == di->m)
                            fcb->prop_compression = PropCompression_LZO;
                        else if (di->m == strlen(zlib) && RtlCompareMemory(&di->name[di->n], zlib, di->m) == di->m)
                            fcb->prop_compression = PropCompression_Zlib;
                        else
                            fcb->prop_compression = PropCompression_None;
                    }
                } else if (di->n > strlen(xapref) && RtlCompareMemory(xapref, di->name, strlen(xapref)) == strlen(xapref)) {
                    dir_child* dc;
                    ULONG utf16len;

                    Status = RtlUTF8ToUnicodeN(NULL, 0, &utf16len, &di->name[strlen(xapref)], di->n - (ULONG)strlen(xapref));
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                        free_fcb(Vcb, fcb);
                        return Status;
                    }

                    dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
                    if (!dc) {
                        ERR("out of memory\n");
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlZeroMemory(dc, sizeof(dir_child));

                    dc->utf8.MaximumLength = dc->utf8.Length = di->n - (UINT16)strlen(xapref);
                    dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, dc->utf8.MaximumLength, ALLOC_TAG);
                    if (!dc->utf8.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(dc);
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(dc->utf8.Buffer, &di->name[strlen(xapref)], dc->utf8.Length);

                    dc->name.MaximumLength = dc->name.Length = (UINT16)utf16len;
                    dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, dc->name.MaximumLength, ALLOC_TAG);
                    if (!dc->name.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(dc->utf8.Buffer);
                        ExFreePool(dc);
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    Status = RtlUTF8ToUnicodeN(dc->name.Buffer, utf16len, &utf16len, dc->utf8.Buffer, dc->utf8.Length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        ExFreePool(dc->utf8.Buffer);
                        ExFreePool(dc->name.Buffer);
                        ExFreePool(dc);
                        free_fcb(Vcb, fcb);
                        return Status;
                    }

                    Status = RtlUpcaseUnicodeString(&dc->name_uc, &dc->name, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                        ExFreePool(dc->utf8.Buffer);
                        ExFreePool(dc->name.Buffer);
                        ExFreePool(dc);
                        free_fcb(Vcb, fcb);
                        return Status;
                    }

                    dc->size = di->m;

                    InsertTailList(&fcb->dir_children_index, &dc->list_entry_index);
                } else {
                    xattr* xa;

                    xa = ExAllocatePoolWithTag(PagedPool, offsetof(xattr, data[0]) + di->m + di->n, ALLOC_TAG);
                    if (!xa) {
                        ERR("out of memory\n");
                        free_fcb(Vcb, fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    xa->namelen = di->n;
                    xa->valuelen = di->m;
                    xa->dirty = FALSE;
                    RtlCopyMemory(xa->data, di->name, di->m + di->n);

                    InsertTailList(&fcb->xattrs, &xa->list_entry);
                }

                len -= (ULONG)offsetof(DIR_ITEM, name[0]) + di->m + di->n;

                if (len < offsetof(DIR_ITEM, name[0]))
                    break;

                di = (DIR_ITEM*)&di->name[di->m + di->n];
            } while (TRUE);
        } else if (tp.item->key.obj_type == TYPE_EXTENT_DATA) {
            extent* ext;
            BOOL unique = FALSE;

            ed = (EXTENT_DATA*)tp.item->data;

            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                    tp.item->size, sizeof(EXTENT_DATA));

                free_fcb(Vcb, fcb);
                return STATUS_INTERNAL_ERROR;
            }

            if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];

                if (tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                        tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));

                    free_fcb(Vcb, fcb);
                    return STATUS_INTERNAL_ERROR;
                }

                if (ed2->address == 0 || ed2->size == 0) // sparse
                    continue;

                if (ed2->size != 0 && is_tree_unique(Vcb, tp.tree, Irp))
                    unique = is_extent_unique(Vcb, ed2->address, ed2->size, Irp);
            }

            ext = ExAllocatePoolWithTag(pooltype, offsetof(extent, extent_data) + tp.item->size, ALLOC_TAG);
            if (!ext) {
                ERR("out of memory\n");
                free_fcb(Vcb, fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ext->offset = tp.item->key.offset;
            RtlCopyMemory(&ext->extent_data, tp.item->data, tp.item->size);
            ext->datalen = tp.item->size;
            ext->unique = unique;
            ext->ignore = FALSE;
            ext->inserted = FALSE;
            ext->csum = NULL;

            InsertTailList(&fcb->extents, &ext->list_entry);
        }
    }

    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        Status = load_dir_children(Vcb, fcb, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("load_dir_children returned %08x\n", Status);
            free_fcb(Vcb, fcb);
            return Status;
        }
    }

    if (no_data) {
        fcb->Header.AllocationSize.QuadPart = 0;
        fcb->Header.FileSize.QuadPart = 0;
        fcb->Header.ValidDataLength.QuadPart = 0;
    } else {
        if (ed && ed->type == EXTENT_TYPE_INLINE)
            fcb->Header.AllocationSize.QuadPart = fcb->inode_item.st_size;
        else
            fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);

        fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
        fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
    }

    if (!atts_set)
        fcb->atts = get_file_attributes(Vcb, fcb->subvol, fcb->inode, fcb->type, utf8 && utf8->Buffer[0] == '.', TRUE, Irp);

    if (!sd_set)
        fcb_get_sd(fcb, parent, FALSE, Irp);

    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT && fcb->reparse_xattr.Length == 0) {
        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;

        if (!Vcb->readonly && !is_subvol_readonly(subvol, Irp)) {
            fcb->atts_changed = TRUE;
            mark_fcb_dirty(fcb);
        }
    }

    if (lastle)
        InsertHeadList(lastle, &fcb->list_entry);
    else
        InsertTailList(&subvol->fcbs, &fcb->list_entry);

    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    *pfcb = fcb;
    return STATUS_SUCCESS;
}

static NTSTATUS open_fcb_stream(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                                dir_child* dc, fcb* parent, fcb** pfcb, PIRP Irp) {
    fcb* fcb;
    UINT8* xattrdata;
    UINT16 xattrlen, overhead;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    static char xapref[] = "user.";
    ANSI_STRING xattr;
    UINT32 crc32;

    xattr.Length = (UINT16)strlen(xapref) + dc->utf8.Length;
    xattr.MaximumLength = xattr.Length + 1;
    xattr.Buffer = ExAllocatePoolWithTag(PagedPool, xattr.MaximumLength, ALLOC_TAG);
    if (!xattr.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(xattr.Buffer, xapref, strlen(xapref));
    RtlCopyMemory(&xattr.Buffer[strlen(xapref)], dc->utf8.Buffer, dc->utf8.Length);
    xattr.Buffer[xattr.Length] = 0;

    fcb = create_fcb(Vcb, PagedPool);
    if (!fcb) {
        ERR("out of memory\n");
        ExFreePool(xattr.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fcb->Vcb = Vcb;

    crc32 = calc_crc32c(0xfffffffe, (UINT8*)xattr.Buffer, xattr.Length);

    if (!get_xattr(Vcb, parent->subvol, parent->inode, xattr.Buffer, crc32, &xattrdata, &xattrlen, Irp)) {
        ERR("get_xattr failed\n");
        free_fcb(Vcb, fcb);
        ExFreePool(xattr.Buffer);
        return STATUS_INTERNAL_ERROR;
    }

    fcb->subvol = parent->subvol;
    fcb->inode = parent->inode;
    fcb->type = parent->type;
    fcb->ads = TRUE;
    fcb->adshash = crc32;
    fcb->adsxattr = xattr;

    // find XATTR_ITEM overhead and hence calculate maximum length

    searchkey.obj_id = parent->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;

    Status = find_item(Vcb, parent->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        free_fcb(Vcb, fcb);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        ERR("error - could not find key for xattr\n");
        free_fcb(Vcb, fcb);
        return STATUS_INTERNAL_ERROR;
    }

    if (tp.item->size < xattrlen) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, xattrlen);
        free_fcb(Vcb, fcb);
        return STATUS_INTERNAL_ERROR;
    }

    overhead = tp.item->size - xattrlen;

    fcb->adsmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - overhead;

    fcb->adsdata.Buffer = (char*)xattrdata;
    fcb->adsdata.Length = fcb->adsdata.MaximumLength = xattrlen;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = xattrlen;
    fcb->Header.FileSize.QuadPart = xattrlen;
    fcb->Header.ValidDataLength.QuadPart = xattrlen;

    TRACE("stream found: size = %x, hash = %08x\n", xattrlen, fcb->adshash);

    InsertHeadList(&parent->list_entry, &fcb->list_entry);

    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);

    *pfcb = fcb;

    return STATUS_SUCCESS;
}

NTSTATUS open_fileref_child(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb,
                            _In_ file_ref* sf, _In_ PUNICODE_STRING name, _In_ BOOL case_sensitive, _In_ BOOL lastpart, _In_ BOOL streampart,
                            _In_ POOL_TYPE pooltype, _Out_ file_ref** psf2, _In_opt_ PIRP Irp) {
    NTSTATUS Status;
    file_ref* sf2;

    if (sf->fcb == Vcb->dummy_fcb)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    if (streampart) {
        BOOL locked = FALSE;
        LIST_ENTRY* le;
        UNICODE_STRING name_uc;
        dir_child* dc = NULL;
        fcb* fcb;

        if (!case_sensitive) {
            Status = RtlUpcaseUnicodeString(&name_uc, name, TRUE);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                return Status;
            }
        }

        if (!ExIsResourceAcquiredSharedLite(&sf->fcb->nonpaged->dir_children_lock)) {
            ExAcquireResourceSharedLite(&sf->fcb->nonpaged->dir_children_lock, TRUE);
            locked = TRUE;
        }

        le = sf->fcb->dir_children_index.Flink;
        while (le != &sf->fcb->dir_children_index) {
            dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_index);

            if (dc2->index == 0) {
                if ((case_sensitive && dc2->name.Length == name->Length && RtlCompareMemory(dc2->name.Buffer, name->Buffer, dc2->name.Length) == dc2->name.Length) ||
                    (!case_sensitive && dc2->name_uc.Length == name_uc.Length && RtlCompareMemory(dc2->name_uc.Buffer, name_uc.Buffer, dc2->name_uc.Length) == dc2->name_uc.Length)
                ) {
                    dc = dc2;
                    break;
                }
            } else
                break;

            le = le->Flink;
        }

        if (!case_sensitive)
            ExFreePool(name_uc.Buffer);

        if (locked)
            ExReleaseResourceLite(&sf->fcb->nonpaged->dir_children_lock);

        if (!dc)
            return STATUS_OBJECT_NAME_NOT_FOUND;

        if (dc->fileref) {
            increase_fileref_refcount(dc->fileref);
            *psf2 = dc->fileref;
            return STATUS_SUCCESS;
        }

        Status = open_fcb_stream(Vcb, dc, sf->fcb, &fcb, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("open_fcb_stream returned %08x\n", Status);
            return Status;
        }

        sf2 = create_fileref(Vcb);
        if (!sf2) {
            ERR("out of memory\n");
            free_fcb(Vcb, fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        sf2->fcb = fcb;

        sf2->parent = (struct _file_ref*)sf;

        sf2->dc = dc;
        dc->fileref = sf2;

        ExAcquireResourceExclusiveLite(&sf->nonpaged->children_lock, TRUE);
        InsertTailList(&sf->children, &sf2->list_entry);
        ExReleaseResourceLite(&sf->nonpaged->children_lock);

        increase_fileref_refcount(sf);
    } else {
        root* subvol;
        UINT64 inode;
        dir_child* dc;

        Status = find_file_in_dir(name, sf->fcb, &subvol, &inode, &dc, case_sensitive);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            TRACE("could not find %.*S\n", name->Length / sizeof(WCHAR), name->Buffer);

            return lastpart ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_OBJECT_PATH_NOT_FOUND;
        } else if (!NT_SUCCESS(Status)) {
            ERR("find_file_in_dir returned %08x\n", Status);
            return Status;
        } else {
            fcb* fcb;
#ifdef DEBUG_STATS
            LARGE_INTEGER time1, time2;
#endif

            if (dc->fileref) {
                if (!lastpart && dc->type != BTRFS_TYPE_DIRECTORY) {
                    TRACE("passed path including file as subdirectory\n");
                    return STATUS_OBJECT_PATH_NOT_FOUND;
                }

                InterlockedIncrement(&dc->fileref->refcount);
                *psf2 = dc->fileref;
                return STATUS_SUCCESS;
            }

            if (!subvol || (subvol != Vcb->root_fileref->fcb->subvol && inode == SUBVOL_ROOT_INODE && subvol->parent != sf->fcb->subvol->id)) {
                fcb = Vcb->dummy_fcb;
                InterlockedIncrement(&fcb->refcount);
            } else {
#ifdef DEBUG_STATS
                time1 = KeQueryPerformanceCounter(NULL);
#endif
                Status = open_fcb(Vcb, subvol, inode, dc->type, &dc->utf8, sf->fcb, &fcb, pooltype, Irp);
#ifdef DEBUG_STATS
                time2 = KeQueryPerformanceCounter(NULL);
                Vcb->stats.open_fcb_calls++;
                Vcb->stats.open_fcb_time += time2.QuadPart - time1.QuadPart;
#endif

                if (!NT_SUCCESS(Status)) {
                    ERR("open_fcb returned %08x\n", Status);
                    return Status;
                }
            }

            if (dc->type != BTRFS_TYPE_DIRECTORY && !lastpart && !(fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)) {
                TRACE("passed path including file as subdirectory\n");
                free_fcb(Vcb, fcb);
                return STATUS_OBJECT_PATH_NOT_FOUND;
            }

            sf2 = create_fileref(Vcb);
            if (!sf2) {
                ERR("out of memory\n");
                free_fcb(Vcb, fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            sf2->fcb = fcb;

            if (dc->type == BTRFS_TYPE_DIRECTORY)
                fcb->fileref = sf2;

            sf2->dc = dc;
            dc->fileref = sf2;

            sf2->parent = (struct _file_ref*)sf;

            ExAcquireResourceExclusiveLite(&sf->nonpaged->children_lock, TRUE);
            InsertTailList(&sf->children, &sf2->list_entry);
            ExReleaseResourceLite(&sf->nonpaged->children_lock);

            increase_fileref_refcount(sf);
        }
    }

    *psf2 = sf2;

    return STATUS_SUCCESS;
}

NTSTATUS open_fileref(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb, _Out_ file_ref** pfr,
                      _In_ PUNICODE_STRING fnus, _In_opt_ file_ref* related, _In_ BOOL parent, _Out_opt_ USHORT* parsed, _Out_opt_ ULONG* fn_offset, _In_ POOL_TYPE pooltype,
                      _In_ BOOL case_sensitive, _In_opt_ PIRP Irp) {
    UNICODE_STRING fnus2;
    file_ref *dir, *sf, *sf2;
    LIST_ENTRY parts;
    BOOL has_stream;
    NTSTATUS Status;
    LIST_ENTRY* le;

    TRACE("(%p, %p, %p, %u, %p)\n", Vcb, pfr, related, parent, parsed);

#ifdef DEBUG
    if (!ExIsResourceAcquiredExclusiveLite(&Vcb->fcb_lock) && !ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
        ERR("fcb_lock not acquired exclusively\n");
        int3;
    }
#endif

    if (Vcb->removing || Vcb->locked)
        return STATUS_ACCESS_DENIED;

    fnus2 = *fnus;

    if (fnus2.Length < sizeof(WCHAR) && !related) {
        ERR("error - fnus was too short\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (related && fnus->Length == 0) {
        increase_fileref_refcount(related);

        *pfr = related;
        return STATUS_SUCCESS;
    }

    if (related) {
        dir = related;
    } else {
        if (fnus2.Buffer[0] != '\\') {
            ERR("error - filename %.*S did not begin with \\\n", fnus2.Length / sizeof(WCHAR), fnus2.Buffer);
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }

        // if path starts with two backslashes, ignore one of them
        if (fnus2.Length >= 2 * sizeof(WCHAR) && fnus2.Buffer[1] == '\\') {
            fnus2.Buffer++;
            fnus2.Length -= sizeof(WCHAR);
            fnus2.MaximumLength -= sizeof(WCHAR);
        }

        if (fnus2.Length == sizeof(WCHAR)) {
            if (Vcb->root_fileref->open_count == 0 && !(Vcb->Vpb->Flags & VPB_MOUNTED)) // don't allow root to be opened on unmounted FS
                return STATUS_DEVICE_NOT_READY;

            increase_fileref_refcount(Vcb->root_fileref);
            *pfr = Vcb->root_fileref;

            if (fn_offset)
                *fn_offset = 0;

            return STATUS_SUCCESS;
        }

        dir = Vcb->root_fileref;

        fnus2.Buffer++;
        fnus2.Length -= sizeof(WCHAR);
        fnus2.MaximumLength -= sizeof(WCHAR);
    }

    if (dir->fcb->type != BTRFS_TYPE_DIRECTORY && (fnus->Length < sizeof(WCHAR) || fnus->Buffer[0] != ':')) {
        WARN("passed related fileref which isn't a directory (%S) (fnus = %.*S)\n",
             file_desc_fileref(related), fnus->Length / sizeof(WCHAR), fnus->Buffer);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    InitializeListHead(&parts);

    if (fnus->Length != 0 &&
        (fnus->Length != wcslen(datastring) * sizeof(WCHAR) || RtlCompareMemory(fnus->Buffer, datastring, wcslen(datastring) * sizeof(WCHAR)) != wcslen(datastring) * sizeof(WCHAR))) {
        Status = split_path(Vcb, &fnus2, &parts, &has_stream);
        if (!NT_SUCCESS(Status)) {
            ERR("split_path returned %08x\n", Status);
            return Status;
        }
    }

    sf = dir;
    increase_fileref_refcount(dir);

    if (parent && !IsListEmpty(&parts)) {
        name_bit* nb;

        nb = CONTAINING_RECORD(RemoveTailList(&parts), name_bit, list_entry);
        ExFreePool(nb);

        if (has_stream && !IsListEmpty(&parts)) {
            nb = CONTAINING_RECORD(RemoveTailList(&parts), name_bit, list_entry);
            ExFreePool(nb);

            has_stream = FALSE;
        }
    }

    if (IsListEmpty(&parts)) {
        Status = STATUS_SUCCESS;
        *pfr = dir;

        if (fn_offset)
            *fn_offset = 0;

        goto end2;
    }

    le = parts.Flink;
    do {
        name_bit* nb = CONTAINING_RECORD(le, name_bit, list_entry);
        BOOL lastpart = le->Flink == &parts || (has_stream && le->Flink->Flink == &parts);
        BOOL streampart = has_stream && le->Flink == &parts;
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
#endif

#ifdef DEBUG_STATS
        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = open_fileref_child(Vcb, sf, &nb->us, case_sensitive, lastpart, streampart, pooltype, &sf2, Irp);
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);
        Vcb->stats.open_fileref_child_calls++;
        Vcb->stats.open_fileref_child_time += time2.QuadPart - time1.QuadPart;
#endif
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_OBJECT_PATH_NOT_FOUND || Status == STATUS_OBJECT_NAME_NOT_FOUND)
                TRACE("open_fileref_child returned %08x\n", Status);
            else
                ERR("open_fileref_child returned %08x\n", Status);

            goto end;
        }

        if (le->Flink == &parts) { // last entry
            if (fn_offset) {
                if (has_stream)
                    nb = CONTAINING_RECORD(le->Blink, name_bit, list_entry);

                *fn_offset = (ULONG)(nb->us.Buffer - fnus->Buffer);
            }

            break;
        }

        if (sf2->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) {
            Status = STATUS_REPARSE;

            if (parsed) {
                name_bit* nb2 = CONTAINING_RECORD(le->Flink, name_bit, list_entry);

                *parsed = (USHORT)(nb2->us.Buffer - fnus->Buffer - 1) * sizeof(WCHAR);
            }

            break;
        }

        free_fileref(Vcb, sf);
        sf = sf2;

        le = le->Flink;
    } while (le != &parts);

    if (Status != STATUS_REPARSE)
        Status = STATUS_SUCCESS;
    *pfr = sf2;

end:
    free_fileref(Vcb, sf);

    while (!IsListEmpty(&parts)) {
        name_bit* nb = CONTAINING_RECORD(RemoveHeadList(&parts), name_bit, list_entry);
        ExFreeToPagedLookasideList(&Vcb->name_bit_lookaside, nb);
    }

end2:
    TRACE("returning %08x\n", Status);

    return Status;
}

NTSTATUS add_dir_child(fcb* fcb, UINT64 inode, BOOL subvol, PANSI_STRING utf8, PUNICODE_STRING name, UINT8 type, dir_child** pdc) {
    NTSTATUS Status;
    dir_child* dc;

    dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
    if (!dc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8->Length, ALLOC_TAG);
    if (!dc->utf8.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, name->Length, ALLOC_TAG);
    if (!dc->name.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dc->key.obj_id = inode;
    dc->key.obj_type = subvol ? TYPE_ROOT_ITEM : TYPE_INODE_ITEM;
    dc->key.offset = subvol ? 0xffffffffffffffff : 0;
    dc->type = type;
    dc->fileref = NULL;

    dc->utf8.Length = dc->utf8.MaximumLength = utf8->Length;
    RtlCopyMemory(dc->utf8.Buffer, utf8->Buffer, utf8->Length);

    dc->name.Length = dc->name.MaximumLength = name->Length;
    RtlCopyMemory(dc->name.Buffer, name->Buffer, name->Length);

    Status = RtlUpcaseUnicodeString(&dc->name_uc, name, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc->name.Buffer);
        ExFreePool(dc);
        return Status;
    }

    dc->hash = calc_crc32c(0xffffffff, (UINT8*)dc->name.Buffer, dc->name.Length);
    dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)dc->name_uc.Buffer, dc->name_uc.Length);

    ExAcquireResourceExclusiveLite(&fcb->nonpaged->dir_children_lock, TRUE);

    if (IsListEmpty(&fcb->dir_children_index))
        dc->index = 2;
    else {
        dir_child* dc2 = CONTAINING_RECORD(fcb->dir_children_index.Blink, dir_child, list_entry_index);

        dc->index = max(2, dc2->index + 1);
    }

    InsertTailList(&fcb->dir_children_index, &dc->list_entry_index);

    insert_dir_child_into_hash_lists(fcb, dc);

    ExReleaseResourceLite(&fcb->nonpaged->dir_children_lock);

    *pdc = dc;

    return STATUS_SUCCESS;
}

UINT32 inherit_mode(fcb* parfcb, BOOL is_dir) {
    UINT32 mode;

    if (!parfcb)
        return 0755;

    mode = parfcb->inode_item.st_mode & ~S_IFDIR;
    mode &= ~S_ISVTX; // clear sticky bit
    mode &= ~S_ISUID; // clear setuid bit

    if (!is_dir)
        mode &= ~S_ISGID; // if not directory, clear setgid bit

    return mode;
}

static NTSTATUS file_create2(_In_ PIRP Irp, _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb, _In_ PUNICODE_STRING fpus,
                             _In_ file_ref* parfileref, _In_ ULONG options, _In_reads_bytes_opt_(ealen) FILE_FULL_EA_INFORMATION* ea, _In_ ULONG ealen,
                             _Out_ file_ref** pfr, _In_ LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb* fcb;
    ULONG utf8len;
    char* utf8 = NULL;
    UINT64 inode;
    UINT8 type;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POOL_TYPE pool_type = IrpSp->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
    USHORT defda;
    file_ref* fileref;
    dir_child* dc;
    ANSI_STRING utf8as;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG rc;
#endif

    if (parfileref->fcb == Vcb->dummy_fcb)
        return STATUS_ACCESS_DENIED;

    if (options & FILE_DIRECTORY_FILE && IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_TEMPORARY)
        return STATUS_INVALID_PARAMETER;

    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, fpus->Buffer, fpus->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N returned %08x\n", Status);
        return Status;
    }

    utf8 = ExAllocatePoolWithTag(pool_type, utf8len + 1, ALLOC_TAG);
    if (!utf8) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlUnicodeToUTF8N(utf8, utf8len, &utf8len, fpus->Buffer, fpus->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }

    utf8[utf8len] = 0;

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    TRACE("create file %.*S\n", fpus->Length / sizeof(WCHAR), fpus->Buffer);
    ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
    TRACE("parfileref->fcb->inode_item.st_size (inode %llx) was %llx\n", parfileref->fcb->inode, parfileref->fcb->inode_item.st_size);
    parfileref->fcb->inode_item.st_size += utf8len * 2;
    TRACE("parfileref->fcb->inode_item.st_size (inode %llx) now %llx\n", parfileref->fcb->inode, parfileref->fcb->inode_item.st_size);
    parfileref->fcb->inode_item.transid = Vcb->superblock.generation;
    parfileref->fcb->inode_item.sequence++;
    parfileref->fcb->inode_item.st_ctime = now;
    parfileref->fcb->inode_item.st_mtime = now;
    ExReleaseResourceLite(parfileref->fcb->Header.Resource);

    parfileref->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(parfileref->fcb);

    inode = InterlockedIncrement64(&parfileref->fcb->subvol->lastinode);

    type = options & FILE_DIRECTORY_FILE ? BTRFS_TYPE_DIRECTORY : BTRFS_TYPE_FILE;

    // FIXME - link FILE_ATTRIBUTE_READONLY to st_mode

    TRACE("requested attributes = %x\n", IrpSp->Parameters.Create.FileAttributes);

    defda = 0;

    if (utf8[0] == '.')
        defda |= FILE_ATTRIBUTE_HIDDEN;

    if (options & FILE_DIRECTORY_FILE) {
        defda |= FILE_ATTRIBUTE_DIRECTORY;
        IrpSp->Parameters.Create.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    } else
        IrpSp->Parameters.Create.FileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

    if (!(IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        IrpSp->Parameters.Create.FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
        defda |= FILE_ATTRIBUTE_ARCHIVE;
    }

    TRACE("defda = %x\n", defda);

    if (IrpSp->Parameters.Create.FileAttributes == FILE_ATTRIBUTE_NORMAL)
        IrpSp->Parameters.Create.FileAttributes = defda;

    fcb = create_fcb(Vcb, pool_type);
    if (!fcb) {
        ERR("out of memory\n");
        ExFreePool(utf8);

        ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
        parfileref->fcb->inode_item.st_size -= utf8len * 2;
        ExReleaseResourceLite(parfileref->fcb->Header.Resource);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fcb->Vcb = Vcb;

    if (IrpSp->Flags & SL_OPEN_PAGING_FILE) {
        fcb->Header.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
        Vcb->disallow_dismount = TRUE;
    }

    fcb->inode_item.generation = Vcb->superblock.generation;
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size = 0;
    fcb->inode_item.st_blocks = 0;
    fcb->inode_item.block_group = 0;
    fcb->inode_item.st_nlink = 1;
    fcb->inode_item.st_gid = GID_NOBODY; // FIXME?
    fcb->inode_item.st_mode = inherit_mode(parfileref->fcb, type == BTRFS_TYPE_DIRECTORY); // use parent's permissions by default
    fcb->inode_item.st_rdev = 0;
    fcb->inode_item.flags = 0;
    fcb->inode_item.sequence = 1;
    fcb->inode_item.st_atime = now;
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    fcb->inode_item.otime = now;

    if (type == BTRFS_TYPE_DIRECTORY)
        fcb->inode_item.st_mode |= S_IFDIR;
    else {
        fcb->inode_item.st_mode |= S_IFREG;
        fcb->inode_item.st_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH); // remove executable bit if not directory
    }

    if (IrpSp->Flags & SL_OPEN_PAGING_FILE) {
        fcb->inode_item.flags = BTRFS_INODE_NODATACOW | BTRFS_INODE_NODATASUM | BTRFS_INODE_NOCOMPRESS;
    } else {
        // inherit nodatacow flag from parent directory
        if (parfileref->fcb->inode_item.flags & BTRFS_INODE_NODATACOW) {
            fcb->inode_item.flags |= BTRFS_INODE_NODATACOW;

            if (type != BTRFS_TYPE_DIRECTORY)
                fcb->inode_item.flags |= BTRFS_INODE_NODATASUM;
        }

        if (parfileref->fcb->inode_item.flags & BTRFS_INODE_COMPRESS)
            fcb->inode_item.flags |= BTRFS_INODE_COMPRESS;
    }

    fcb->prop_compression = parfileref->fcb->prop_compression;
    fcb->prop_compression_changed = fcb->prop_compression != PropCompression_None;

    fcb->inode_item_changed = TRUE;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;

    fcb->atts = IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL;
    fcb->atts_changed = fcb->atts != defda;

#ifdef DEBUG_FCB_REFCOUNTS
    rc = InterlockedIncrement(&parfileref->fcb->refcount);
    WARN("fcb %p: refcount now %i (%S)\n", parfileref->fcb, rc, file_desc_fileref(parfileref));
#else
    InterlockedIncrement(&parfileref->fcb->refcount);
#endif
    fcb->subvol = parfileref->fcb->subvol;
    fcb->inode = inode;
    fcb->type = type;
    fcb->created = TRUE;
    fcb->deleted = TRUE;

    mark_fcb_dirty(fcb);

    Status = fcb_get_new_sd(fcb, parfileref, IrpSp->Parameters.Create.SecurityContext->AccessState);

    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_new_sd returned %08x\n", Status);
        free_fcb(Vcb, fcb);

        ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
        parfileref->fcb->inode_item.st_size -= utf8len * 2;
        ExReleaseResourceLite(parfileref->fcb->Header.Resource);

        return Status;
    }

    fcb->sd_dirty = TRUE;

    if (ea && ealen > 0) {
        FILE_FULL_EA_INFORMATION* eainfo;

        fcb->ealen = 4;

        // capitalize EA names
        eainfo = ea;
        do {
            STRING s;

            s.Length = s.MaximumLength = eainfo->EaNameLength;
            s.Buffer = eainfo->EaName;

            RtlUpperString(&s, &s);

            fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;

            if (eainfo->NextEntryOffset == 0)
                break;

            eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
        } while (TRUE);

        fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(pool_type, ealen, ALLOC_TAG);
        if (!fcb->ea_xattr.Buffer) {
            ERR("out of memory\n");
            free_fcb(Vcb, fcb);

            ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
            parfileref->fcb->inode_item.st_size -= utf8len * 2;
            ExReleaseResourceLite(parfileref->fcb->Header.Resource);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = (UINT16)ealen;
        RtlCopyMemory(fcb->ea_xattr.Buffer, ea, fcb->ea_xattr.Length);

        fcb->ea_changed = TRUE;
    }

    fileref = create_fileref(Vcb);
    if (!fileref) {
        ERR("out of memory\n");
        free_fcb(Vcb, fcb);

        ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
        parfileref->fcb->inode_item.st_size -= utf8len * 2;
        ExReleaseResourceLite(parfileref->fcb->Header.Resource);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fileref->fcb = fcb;

    if (Irp->Overlay.AllocationSize.QuadPart > 0 && !write_fcb_compressed(fcb)) {
        Status = extend_file(fcb, fileref, Irp->Overlay.AllocationSize.QuadPart, TRUE, NULL, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("extend_file returned %08x\n", Status);
            free_fileref(Vcb, fileref);

            ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
            parfileref->fcb->inode_item.st_size -= utf8len * 2;
            ExReleaseResourceLite(parfileref->fcb->Header.Resource);

            return Status;
        }
    }

    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs) {
            ERR("out of memory\n");
            free_fileref(Vcb, fileref);

            ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
            parfileref->fcb->inode_item.st_size -= utf8len * 2;
            ExReleaseResourceLite(parfileref->fcb->Header.Resource);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);

        fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs_uc) {
            ERR("out of memory\n");
            free_fileref(Vcb, fileref);

            ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
            parfileref->fcb->inode_item.st_size -= utf8len * 2;
            ExReleaseResourceLite(parfileref->fcb->Header.Resource);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);
    }

    fcb->deleted = FALSE;

    fileref->created = TRUE;
    mark_fileref_dirty(fileref);

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    fileref->parent = parfileref;

    utf8as.Buffer = utf8;
    utf8as.Length = utf8as.MaximumLength = (UINT16)utf8len;

    Status = add_dir_child(fileref->parent->fcb, fcb->inode, FALSE, &utf8as, fpus, fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);

    ExFreePool(utf8);

    fileref->dc = dc;
    dc->fileref = fileref;

    ExAcquireResourceExclusiveLite(&parfileref->nonpaged->children_lock, TRUE);
    InsertTailList(&parfileref->children, &fileref->list_entry);
    ExReleaseResourceLite(&parfileref->nonpaged->children_lock);

    increase_fileref_refcount(parfileref);

    InsertTailList(&fcb->subvol->fcbs, &fcb->list_entry);
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);

    *pfr = fileref;

    if (type == BTRFS_TYPE_DIRECTORY)
        fileref->fcb->fileref = fileref;

    TRACE("created new file %S in subvol %llx, inode %llx\n", file_desc_fileref(fileref), fcb->subvol->id, fcb->inode);

    return STATUS_SUCCESS;
}

static NTSTATUS create_stream(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                              file_ref** pfileref, file_ref** pparfileref, PUNICODE_STRING fpus, PUNICODE_STRING stream, PIRP Irp,
                              ULONG options, POOL_TYPE pool_type, BOOL case_sensitive, LIST_ENTRY* rollback) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    file_ref *fileref, *newpar, *parfileref;
    fcb* fcb;
    static char xapref[] = "user.";
    static WCHAR DOSATTRIB[] = L"DOSATTRIB";
    static WCHAR EA[] = L"EA";
    static WCHAR reparse[] = L"reparse";
    UINT16 xapreflen = (UINT16)strlen(xapref);
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ULONG utf8len, overhead;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    dir_child* dc;
    ACCESS_MASK granted_access;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG rc;
#endif

    TRACE("fpus = %.*S\n", fpus->Length / sizeof(WCHAR), fpus->Buffer);
    TRACE("stream = %.*S\n", stream->Length / sizeof(WCHAR), stream->Buffer);

    parfileref = *pparfileref;

    if (parfileref->fcb == Vcb->dummy_fcb)
        return STATUS_ACCESS_DENIED;

    Status = open_fileref(Vcb, &newpar, fpus, parfileref, FALSE, NULL, NULL, PagedPool, case_sensitive, Irp);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        UNICODE_STRING fpus2;

        if (!is_file_name_valid(fpus, FALSE))
            return STATUS_OBJECT_NAME_INVALID;

        fpus2.Length = fpus2.MaximumLength = fpus->Length;
        fpus2.Buffer = ExAllocatePoolWithTag(pool_type, fpus2.MaximumLength, ALLOC_TAG);

        if (!fpus2.Buffer) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(fpus2.Buffer, fpus->Buffer, fpus2.Length);

        SeLockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

        if (!SeAccessCheck(parfileref->fcb->sd, &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                           TRUE, options & FILE_DIRECTORY_FILE ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0, NULL,
                           IoGetFileObjectGenericMapping(), IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode,
                           &granted_access, &Status)) {
            SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
            return Status;
        }

        SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

        Status = file_create2(Irp, Vcb, &fpus2, parfileref, options, NULL, 0, &newpar, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("file_create2 returned %08x\n", Status);
            ExFreePool(fpus2.Buffer);
            return Status;
        }

        send_notification_fileref(newpar, options & FILE_DIRECTORY_FILE ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);
        send_notification_fcb(newpar->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
    } else if (!NT_SUCCESS(Status)) {
        ERR("open_fileref returned %08x\n", Status);
        return Status;
    }

    parfileref = newpar;
    *pparfileref = parfileref;

    if (parfileref->fcb->type != BTRFS_TYPE_FILE && parfileref->fcb->type != BTRFS_TYPE_SYMLINK && parfileref->fcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("parent not file, directory, or symlink\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (options & FILE_DIRECTORY_FILE) {
        WARN("tried to create directory as stream\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (parfileref->fcb->atts & FILE_ATTRIBUTE_READONLY)
        return STATUS_ACCESS_DENIED;

    SeLockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

    if (!SeAccessCheck(parfileref->fcb->sd, &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                       TRUE, FILE_WRITE_DATA, 0, NULL, IoGetFileObjectGenericMapping(), IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode,
                       &granted_access, &Status)) {
        SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
        return Status;
    }

    SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

    if ((stream->Length == wcslen(DOSATTRIB) * sizeof(WCHAR) && RtlCompareMemory(stream->Buffer, DOSATTRIB, stream->Length) == stream->Length) ||
        (stream->Length == wcslen(EA) * sizeof(WCHAR) && RtlCompareMemory(stream->Buffer, EA, stream->Length) == stream->Length) ||
        (stream->Length == wcslen(reparse) * sizeof(WCHAR) && RtlCompareMemory(stream->Buffer, reparse, stream->Length) == stream->Length)) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    fcb = create_fcb(Vcb, pool_type);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fcb->Vcb = Vcb;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;

#ifdef DEBUG_FCB_REFCOUNTS
    rc = InterlockedIncrement(&parfileref->fcb->refcount);
    WARN("fcb %p: refcount now %i (%S)\n", parfileref->fcb, rc, file_desc_fileref(parfileref));
#else
    InterlockedIncrement(&parfileref->fcb->refcount);
#endif
    fcb->subvol = parfileref->fcb->subvol;
    fcb->inode = parfileref->fcb->inode;
    fcb->type = parfileref->fcb->type;

    fcb->ads = TRUE;

    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 1 returned %08x\n", Status);
        free_fcb(Vcb, fcb);
        return Status;
    }

    fcb->adsxattr.Length = (UINT16)utf8len + xapreflen;
    fcb->adsxattr.MaximumLength = fcb->adsxattr.Length + 1;
    fcb->adsxattr.Buffer = ExAllocatePoolWithTag(pool_type, fcb->adsxattr.MaximumLength, ALLOC_TAG);
    if (!fcb->adsxattr.Buffer) {
        ERR("out of memory\n");
        free_fcb(Vcb, fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(fcb->adsxattr.Buffer, xapref, xapreflen);

    Status = RtlUnicodeToUTF8N(&fcb->adsxattr.Buffer[xapreflen], utf8len, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 2 returned %08x\n", Status);
        free_fcb(Vcb, fcb);
        return Status;
    }

    fcb->adsxattr.Buffer[fcb->adsxattr.Length] = 0;

    TRACE("adsxattr = %s\n", fcb->adsxattr.Buffer);

    fcb->adshash = calc_crc32c(0xfffffffe, (UINT8*)fcb->adsxattr.Buffer, fcb->adsxattr.Length);
    TRACE("adshash = %08x\n", fcb->adshash);

    searchkey.obj_id = parfileref->fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = fcb->adshash;

    Status = find_item(Vcb, parfileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        free_fcb(Vcb, fcb);
        return Status;
    }

    if (!keycmp(tp.item->key, searchkey))
        overhead = tp.item->size;
    else
        overhead = 0;

    fcb->adsmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - (sizeof(DIR_ITEM) - 1);

    if (utf8len + xapreflen + overhead > fcb->adsmaxlen) {
        WARN("not enough room for new DIR_ITEM (%u + %u > %u)", utf8len + xapreflen, overhead, fcb->adsmaxlen);
        free_fcb(Vcb, fcb);
        return STATUS_DISK_FULL;
    } else
        fcb->adsmaxlen -= overhead + utf8len + xapreflen;

    fileref = create_fileref(Vcb);
    if (!fileref) {
        ERR("out of memory\n");
        free_fcb(Vcb, fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fileref->fcb = fcb;

    dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
    if (!dc) {
        ERR("out of memory\n");
        free_fileref(Vcb, fileref);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(dc, sizeof(dir_child));

    dc->utf8.MaximumLength = dc->utf8.Length = fcb->adsxattr.Length - xapreflen;
    dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, dc->utf8.MaximumLength, ALLOC_TAG);
    if (!dc->utf8.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc);
        free_fileref(Vcb, fileref);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(dc->utf8.Buffer, &fcb->adsxattr.Buffer[xapreflen], fcb->adsxattr.Length - xapreflen);

    dc->name.MaximumLength = dc->name.Length = stream->Length;
    dc->name.Buffer = ExAllocatePoolWithTag(pool_type, dc->name.MaximumLength, ALLOC_TAG);
    if (!dc->name.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc);
        free_fileref(Vcb, fileref);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(dc->name.Buffer, stream->Buffer, stream->Length);

    Status = RtlUpcaseUnicodeString(&dc->name_uc, &dc->name, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc->name.Buffer);
        ExFreePool(dc);
        free_fileref(Vcb, fileref);
        return Status;
    }

    dc->fileref = fileref;
    fileref->dc = dc;

    InsertHeadList(&parfileref->fcb->dir_children_index, &dc->list_entry_index);

    mark_fcb_dirty(fcb);
    mark_fileref_dirty(fileref);

    InsertHeadList(&parfileref->fcb->list_entry, &fcb->list_entry); // insert in list after parent fcb
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    parfileref->fcb->inode_item.transid = Vcb->superblock.generation;
    parfileref->fcb->inode_item.sequence++;
    parfileref->fcb->inode_item.st_ctime = now;
    parfileref->fcb->inode_item_changed = TRUE;

    mark_fcb_dirty(parfileref->fcb);

    parfileref->fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    parfileref->fcb->subvol->root_item.ctime = now;

    fileref->parent = (struct _file_ref*)parfileref;

    ExAcquireResourceExclusiveLite(&parfileref->nonpaged->children_lock, TRUE);
    InsertTailList(&parfileref->children, &fileref->list_entry);
    ExReleaseResourceLite(&parfileref->nonpaged->children_lock);

    increase_fileref_refcount(parfileref);

    *pfileref = fileref;

    send_notification_fileref(parfileref, FILE_NOTIFY_CHANGE_STREAM_NAME, FILE_ACTION_ADDED_STREAM, &fileref->dc->name);

    return STATUS_SUCCESS;
}

// LXSS programs can be distinguished by the fact they have a NULL PEB.
#ifdef _AMD64_
#ifdef __REACTOS__
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess (
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
);
#endif
static __inline BOOL called_from_lxss() {
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;
    ULONG retlen;

    Status = ZwQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &retlen);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwQueryInformationProcess returned %08x\n", Status);
        return FALSE;
    }

    return !pbi.PebBaseAddress;
}
#else
#define called_from_lxss() FALSE
#endif

static NTSTATUS file_create(PIRP Irp, _Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                            PFILE_OBJECT FileObject, file_ref* related, BOOL loaded_related, PUNICODE_STRING fnus, ULONG disposition, ULONG options, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    file_ref *fileref, *parfileref = NULL;
    ULONG i, j;
    ccb* ccb;
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    UNICODE_STRING dsus, fpus, stream;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POOL_TYPE pool_type = IrpSp->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG oc;
#endif

    TRACE("(%p, %p, %p, %.*S, %x, %x)\n", Irp, Vcb, FileObject, fnus->Length / sizeof(WCHAR), fnus->Buffer, disposition, options);

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (options & FILE_DELETE_ON_CLOSE && IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_READONLY)
        return STATUS_CANNOT_DELETE;

    dsus.Buffer = datasuf;
    dsus.Length = dsus.MaximumLength = (USHORT)wcslen(datasuf) * sizeof(WCHAR);
    fpus.Buffer = NULL;

    if (!loaded_related) {
        Status = open_fileref(Vcb, &parfileref, fnus, related, TRUE, NULL, NULL, pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, Irp);

        if (!NT_SUCCESS(Status))
            goto end;
    } else
        parfileref = related;

    if (parfileref->fcb->type != BTRFS_TYPE_DIRECTORY && (fnus->Length < sizeof(WCHAR) || fnus->Buffer[0] != ':')) {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto end;
    }

    if (is_subvol_readonly(parfileref->fcb->subvol, Irp)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    i = (fnus->Length / sizeof(WCHAR))-1;
    while ((fnus->Buffer[i] == '\\' || fnus->Buffer[i] == '/') && i > 0) { i--; }

    j = i;

    while (i > 0 && fnus->Buffer[i-1] != '\\' && fnus->Buffer[i-1] != '/') { i--; }

    fpus.MaximumLength = (USHORT)((j - i + 2) * sizeof(WCHAR));
    fpus.Buffer = ExAllocatePoolWithTag(pool_type, fpus.MaximumLength, ALLOC_TAG);
    if (!fpus.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fpus.Length = (USHORT)((j - i + 1) * sizeof(WCHAR));

    RtlCopyMemory(fpus.Buffer, &fnus->Buffer[i], (j - i + 1) * sizeof(WCHAR));
    fpus.Buffer[j - i + 1] = 0;

    if (fpus.Length > dsus.Length) { // check for :$DATA suffix
        UNICODE_STRING lb;

        lb.Buffer = &fpus.Buffer[(fpus.Length - dsus.Length)/sizeof(WCHAR)];
        lb.Length = lb.MaximumLength = dsus.Length;

        TRACE("lb = %.*S\n", lb.Length/sizeof(WCHAR), lb.Buffer);

        if (FsRtlAreNamesEqual(&dsus, &lb, TRUE, NULL)) {
            TRACE("ignoring :$DATA suffix\n");

            fpus.Length -= lb.Length;

            if (fpus.Length > sizeof(WCHAR) && fpus.Buffer[(fpus.Length-1)/sizeof(WCHAR)] == ':')
                fpus.Length -= sizeof(WCHAR);

            TRACE("fpus = %.*S\n", fpus.Length / sizeof(WCHAR), fpus.Buffer);
        }
    }

    stream.Length = 0;

    for (i = 0; i < fpus.Length / sizeof(WCHAR); i++) {
        if (fpus.Buffer[i] == ':') {
            stream.Length = (USHORT)(fpus.Length - (i * sizeof(WCHAR)) - sizeof(WCHAR));
            stream.Buffer = &fpus.Buffer[i+1];
            fpus.Buffer[i] = 0;
            fpus.Length = (USHORT)(i * sizeof(WCHAR));
            break;
        }
    }

    if (stream.Length > 0) {
        Status = create_stream(Vcb, &fileref, &parfileref, &fpus, &stream, Irp, options, pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("create_stream returned %08x\n", Status);
            goto end;
        }

        IoSetShareAccess(IrpSp->Parameters.Create.SecurityContext->DesiredAccess, IrpSp->Parameters.Create.ShareAccess,
                         FileObject, &fileref->fcb->share_access);
    } else {
        ACCESS_MASK granted_access;

        if (!is_file_name_valid(&fpus, FALSE)) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto end;
        }

        SeLockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

        if (!SeAccessCheck(parfileref->fcb->sd, &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                           TRUE, options & FILE_DIRECTORY_FILE ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0, NULL,
                           IoGetFileObjectGenericMapping(), IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode,
                           &granted_access, &Status)) {
            SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
            goto end;
        }

        SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

        if (Irp->AssociatedIrp.SystemBuffer && IrpSp->Parameters.Create.EaLength > 0) {
            ULONG offset;

            Status = IoCheckEaBufferValidity(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Create.EaLength, &offset);
            if (!NT_SUCCESS(Status)) {
                ERR("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
                goto end;
            }
        }

        Status = file_create2(Irp, Vcb, &fpus, parfileref, options, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Create.EaLength,
                              &fileref, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("file_create2 returned %08x\n", Status);
            goto end;
        }

        IoSetShareAccess(IrpSp->Parameters.Create.SecurityContext->DesiredAccess, IrpSp->Parameters.Create.ShareAccess, FileObject, &fileref->fcb->share_access);

        send_notification_fileref(fileref, options & FILE_DIRECTORY_FILE ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);
        send_notification_fcb(fileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
    }

    FileObject->FsContext = fileref->fcb;

    ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
    if (!ccb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        free_fileref(Vcb, fileref);
        goto end;
    }

    RtlZeroMemory(ccb, sizeof(*ccb));

    ccb->fileref = fileref;

    ccb->NodeType = BTRFS_NODE_TYPE_CCB;
    ccb->NodeSize = sizeof(*ccb);
    ccb->disposition = disposition;
    ccb->options = options;
    ccb->query_dir_offset = 0;
    RtlInitUnicodeString(&ccb->query_string, NULL);
    ccb->has_wildcard = FALSE;
    ccb->specific_file = FALSE;
    ccb->access = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ccb->case_sensitive = IrpSp->Flags & SL_CASE_SENSITIVE;
    ccb->reserving = FALSE;
    ccb->lxss = called_from_lxss();

#ifdef DEBUG_FCB_REFCOUNTS
    oc = InterlockedIncrement(&fileref->open_count);
    ERR("fileref %p: open_count now %i\n", fileref, oc);
#else
    InterlockedIncrement(&fileref->open_count);
#endif
    InterlockedIncrement(&Vcb->open_files);

    FileObject->FsContext2 = ccb;

    FileObject->SectionObjectPointer = &fileref->fcb->nonpaged->segment_object;

    goto end2;

end:
    if (fpus.Buffer)
        ExFreePool(fpus.Buffer);

end2:
    if (parfileref && !loaded_related)
        free_fileref(Vcb, parfileref);

    return Status;
}

static __inline void debug_create_options(ULONG RequestedOptions) {
    if (RequestedOptions != 0) {
        ULONG options = RequestedOptions;

        TRACE("requested options:\n");

        if (options & FILE_DIRECTORY_FILE) {
            TRACE("    FILE_DIRECTORY_FILE\n");
            options &= ~FILE_DIRECTORY_FILE;
        }

        if (options & FILE_WRITE_THROUGH) {
            TRACE("    FILE_WRITE_THROUGH\n");
            options &= ~FILE_WRITE_THROUGH;
        }

        if (options & FILE_SEQUENTIAL_ONLY) {
            TRACE("    FILE_SEQUENTIAL_ONLY\n");
            options &= ~FILE_SEQUENTIAL_ONLY;
        }

        if (options & FILE_NO_INTERMEDIATE_BUFFERING) {
            TRACE("    FILE_NO_INTERMEDIATE_BUFFERING\n");
            options &= ~FILE_NO_INTERMEDIATE_BUFFERING;
        }

        if (options & FILE_SYNCHRONOUS_IO_ALERT) {
            TRACE("    FILE_SYNCHRONOUS_IO_ALERT\n");
            options &= ~FILE_SYNCHRONOUS_IO_ALERT;
        }

        if (options & FILE_SYNCHRONOUS_IO_NONALERT) {
            TRACE("    FILE_SYNCHRONOUS_IO_NONALERT\n");
            options &= ~FILE_SYNCHRONOUS_IO_NONALERT;
        }

        if (options & FILE_NON_DIRECTORY_FILE) {
            TRACE("    FILE_NON_DIRECTORY_FILE\n");
            options &= ~FILE_NON_DIRECTORY_FILE;
        }

        if (options & FILE_CREATE_TREE_CONNECTION) {
            TRACE("    FILE_CREATE_TREE_CONNECTION\n");
            options &= ~FILE_CREATE_TREE_CONNECTION;
        }

        if (options & FILE_COMPLETE_IF_OPLOCKED) {
            TRACE("    FILE_COMPLETE_IF_OPLOCKED\n");
            options &= ~FILE_COMPLETE_IF_OPLOCKED;
        }

        if (options & FILE_NO_EA_KNOWLEDGE) {
            TRACE("    FILE_NO_EA_KNOWLEDGE\n");
            options &= ~FILE_NO_EA_KNOWLEDGE;
        }

        if (options & FILE_OPEN_REMOTE_INSTANCE) {
            TRACE("    FILE_OPEN_REMOTE_INSTANCE\n");
            options &= ~FILE_OPEN_REMOTE_INSTANCE;
        }

        if (options & FILE_RANDOM_ACCESS) {
            TRACE("    FILE_RANDOM_ACCESS\n");
            options &= ~FILE_RANDOM_ACCESS;
        }

        if (options & FILE_DELETE_ON_CLOSE) {
            TRACE("    FILE_DELETE_ON_CLOSE\n");
            options &= ~FILE_DELETE_ON_CLOSE;
        }

        if (options & FILE_OPEN_BY_FILE_ID) {
            TRACE("    FILE_OPEN_BY_FILE_ID\n");
            options &= ~FILE_OPEN_BY_FILE_ID;
        }

        if (options & FILE_OPEN_FOR_BACKUP_INTENT) {
            TRACE("    FILE_OPEN_FOR_BACKUP_INTENT\n");
            options &= ~FILE_OPEN_FOR_BACKUP_INTENT;
        }

        if (options & FILE_NO_COMPRESSION) {
            TRACE("    FILE_NO_COMPRESSION\n");
            options &= ~FILE_NO_COMPRESSION;
        }

#if NTDDI_VERSION >= NTDDI_WIN7
        if (options & FILE_OPEN_REQUIRING_OPLOCK) {
            TRACE("    FILE_OPEN_REQUIRING_OPLOCK\n");
            options &= ~FILE_OPEN_REQUIRING_OPLOCK;
        }

        if (options & FILE_DISALLOW_EXCLUSIVE) {
            TRACE("    FILE_DISALLOW_EXCLUSIVE\n");
            options &= ~FILE_DISALLOW_EXCLUSIVE;
        }
#endif

        if (options & FILE_RESERVE_OPFILTER) {
            TRACE("    FILE_RESERVE_OPFILTER\n");
            options &= ~FILE_RESERVE_OPFILTER;
        }

        if (options & FILE_OPEN_REPARSE_POINT) {
            TRACE("    FILE_OPEN_REPARSE_POINT\n");
            options &= ~FILE_OPEN_REPARSE_POINT;
        }

        if (options & FILE_OPEN_NO_RECALL) {
            TRACE("    FILE_OPEN_NO_RECALL\n");
            options &= ~FILE_OPEN_NO_RECALL;
        }

        if (options & FILE_OPEN_FOR_FREE_SPACE_QUERY) {
            TRACE("    FILE_OPEN_FOR_FREE_SPACE_QUERY\n");
            options &= ~FILE_OPEN_FOR_FREE_SPACE_QUERY;
        }

        if (options)
            TRACE("    unknown options: %x\n", options);
    } else {
        TRACE("requested options: (none)\n");
    }
}

static NTSTATUS get_reparse_block(fcb* fcb, UINT8** data) {
    NTSTATUS Status;

    if (fcb->type == BTRFS_TYPE_FILE || fcb->type == BTRFS_TYPE_SYMLINK) {
        ULONG size, bytes_read, i;

        if (fcb->type == BTRFS_TYPE_FILE && fcb->inode_item.st_size < sizeof(ULONG)) {
            WARN("file was too short to be a reparse point\n");
            return STATUS_INVALID_PARAMETER;
        }

        // 0x10007 = 0xffff (maximum length of data buffer) + 8 bytes header
        size = (ULONG)min(0x10007, fcb->inode_item.st_size);

        if (size == 0)
            return STATUS_INVALID_PARAMETER;

        *data = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
        if (!*data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = read_file(fcb, *data, 0, size, &bytes_read, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file_fcb returned %08x\n", Status);
            ExFreePool(*data);
            return Status;
        }

        if (fcb->type == BTRFS_TYPE_SYMLINK) {
            ULONG stringlen, reqlen;
            UINT16 subnamelen, printnamelen;
            REPARSE_DATA_BUFFER* rdb;

            Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, (char*)*data, bytes_read);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                ExFreePool(*data);
                return Status;
            }

            subnamelen = printnamelen = (USHORT)stringlen;

            reqlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + subnamelen + printnamelen;

            rdb = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);

            if (!rdb) {
                ERR("out of memory\n");
                ExFreePool(*data);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
            rdb->ReparseDataLength = (USHORT)(reqlen - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer));
            rdb->Reserved = 0;

            rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
            rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameOffset = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameLength = printnamelen;
            rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;

            Status = RtlUTF8ToUnicodeN(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                                    stringlen, &stringlen, (char*)*data, size);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                ExFreePool(rdb);
                ExFreePool(*data);
                return Status;
            }

            for (i = 0; i < stringlen / sizeof(WCHAR); i++) {
                if (rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] == '/')
                    rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] = '\\';
            }

            RtlCopyMemory(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)],
                        &rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength);

            ExFreePool(*data);

            *data = (UINT8*)rdb;
        } else {
            Status = FsRtlValidateReparsePointBuffer(bytes_read, (REPARSE_DATA_BUFFER*)*data);
            if (!NT_SUCCESS(Status)) {
                ERR("FsRtlValidateReparsePointBuffer returned %08x\n", Status);
                ExFreePool(*data);
                return Status;
            }
        }
    } else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length == 0)
            return STATUS_INTERNAL_ERROR;

        if (fcb->reparse_xattr.Length < sizeof(ULONG)) {
            WARN("xattr was too short to be a reparse point\n");
            return STATUS_INTERNAL_ERROR;
        }

        Status = FsRtlValidateReparsePointBuffer(fcb->reparse_xattr.Length, (REPARSE_DATA_BUFFER*)fcb->reparse_xattr.Buffer);
        if (!NT_SUCCESS(Status)) {
            ERR("FsRtlValidateReparsePointBuffer returned %08x\n", Status);
            return Status;
        }

        *data = ExAllocatePoolWithTag(PagedPool, fcb->reparse_xattr.Length, ALLOC_TAG);
        if (!*data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(*data, fcb->reparse_xattr.Buffer, fcb->reparse_xattr.Length);
    }

    return STATUS_SUCCESS;
}

static void fcb_load_csums(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, fcb* fcb, PIRP Irp) {
    LIST_ENTRY* le;
    NTSTATUS Status;

    if (fcb->csum_loaded)
        return;

    if (IsListEmpty(&fcb->extents) || fcb->inode_item.flags & BTRFS_INODE_NODATASUM)
        goto end;

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (ext->extent_data.type == EXTENT_TYPE_REGULAR) {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ext->extent_data.data[0];
            UINT64 len;

            len = (ext->extent_data.compression == BTRFS_COMPRESSION_NONE ? ed2->num_bytes : ed2->size) / Vcb->superblock.sector_size;

            ext->csum = ExAllocatePoolWithTag(NonPagedPool, (ULONG)(len * sizeof(UINT32)), ALLOC_TAG);
            if (!ext->csum) {
                ERR("out of memory\n");
                goto end;
            }

            Status = load_csum(Vcb, ext->csum, ed2->address + (ext->extent_data.compression == BTRFS_COMPRESSION_NONE ? ed2->offset : 0), len, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("load_csum returned %08x\n", Status);
                goto end;
            }
        }

        le = le->Flink;
    }

end:
    fcb->csum_loaded = TRUE;
}

static NTSTATUS open_file(PDEVICE_OBJECT DeviceObject, _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    PFILE_OBJECT FileObject = NULL;
    ULONG RequestedDisposition;
    ULONG options;
    NTSTATUS Status;
    ccb* ccb;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    USHORT parsed;
    ULONG fn_offset = 0;
    file_ref *related, *fileref = NULL;
    POOL_TYPE pool_type = IrpSp->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
    ACCESS_MASK granted_access;
    BOOL loaded_related = FALSE;
    UNICODE_STRING fn;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG oc;
#endif
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
    UINT8 open_type = 0;

    time1 = KeQueryPerformanceCounter(NULL);
#endif

    Irp->IoStatus.Information = 0;

    RequestedDisposition = ((IrpSp->Parameters.Create.Options >> 24) & 0xff);
    options = IrpSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;

    if (options & FILE_DIRECTORY_FILE && RequestedDisposition == FILE_SUPERSEDE) {
        WARN("error - supersede requested with FILE_DIRECTORY_FILE\n");
        return STATUS_INVALID_PARAMETER;
    }

    FileObject = IrpSp->FileObject;

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (FileObject->RelatedFileObject && FileObject->RelatedFileObject->FsContext2) {
        struct _ccb* relatedccb = FileObject->RelatedFileObject->FsContext2;

        related = relatedccb->fileref;
    } else
        related = NULL;

    debug_create_options(options);

    switch (RequestedDisposition) {
        case FILE_SUPERSEDE:
            TRACE("requested disposition: FILE_SUPERSEDE\n");
            break;

        case FILE_CREATE:
            TRACE("requested disposition: FILE_CREATE\n");
            break;

        case FILE_OPEN:
            TRACE("requested disposition: FILE_OPEN\n");
            break;

        case FILE_OPEN_IF:
            TRACE("requested disposition: FILE_OPEN_IF\n");
            break;

        case FILE_OVERWRITE:
            TRACE("requested disposition: FILE_OVERWRITE\n");
            break;

        case FILE_OVERWRITE_IF:
            TRACE("requested disposition: FILE_OVERWRITE_IF\n");
            break;

        default:
            ERR("unknown disposition: %x\n", RequestedDisposition);
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
    }

    fn = FileObject->FileName;

    TRACE("(%.*S)\n", fn.Length / sizeof(WCHAR), fn.Buffer);
    TRACE("FileObject = %p\n", FileObject);

    if (Vcb->readonly && (RequestedDisposition == FILE_SUPERSEDE || RequestedDisposition == FILE_CREATE || RequestedDisposition == FILE_OVERWRITE)) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto exit;
    }

    acquire_fcb_lock_exclusive(Vcb);

    if (options & FILE_OPEN_BY_FILE_ID) {
        if (fn.Length == sizeof(UINT64) && related && RequestedDisposition == FILE_OPEN) {
            UINT64 inode;

            RtlCopyMemory(&inode, fn.Buffer, sizeof(UINT64));

            if (related->fcb == Vcb->root_fileref->fcb && inode == 0)
                inode = Vcb->root_fileref->fcb->inode;

            if (inode == 0) { // we use 0 to mean the parent of a subvolume
                fileref = related->parent;
                increase_fileref_refcount(fileref);
                Status = STATUS_SUCCESS;
            } else {
                Status = open_fileref_by_inode(Vcb, related->fcb->subvol, inode, &fileref, Irp);
            }
        } else {
            WARN("FILE_OPEN_BY_FILE_ID only supported for inodes\n");
            Status = STATUS_NOT_IMPLEMENTED;
            release_fcb_lock(Vcb);
            goto exit;
        }
    } else {
        if (related && fn.Length != 0 && fn.Buffer[0] == '\\') {
            Status = STATUS_INVALID_PARAMETER;
            release_fcb_lock(Vcb);
            goto exit;
        }

        if (!related && RequestedDisposition != FILE_OPEN && !(IrpSp->Flags & SL_OPEN_TARGET_DIRECTORY)) {
            ULONG fnoff;

            Status = open_fileref(Vcb, &related, &fn, NULL, TRUE, &parsed, &fnoff,
                                  pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, Irp);

            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_OBJECT_PATH_NOT_FOUND;
            else if (Status == STATUS_REPARSE)
                fileref = related;
            else if (NT_SUCCESS(Status)) {
                fnoff *= sizeof(WCHAR);
                fnoff += (related->dc ? related->dc->name.Length : 0) + sizeof(WCHAR);

                if (related->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) {
                    Status = STATUS_REPARSE;
                    fileref = related;
                    parsed = (USHORT)fnoff - sizeof(WCHAR);
                } else {
                    fn.Buffer = &fn.Buffer[fnoff / sizeof(WCHAR)];
                    fn.Length -= (USHORT)fnoff;

                    Status = open_fileref(Vcb, &fileref, &fn, related, IrpSp->Flags & SL_OPEN_TARGET_DIRECTORY, &parsed, &fn_offset,
                                          pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, Irp);

                    loaded_related = TRUE;
                }

            }
        } else {
            Status = open_fileref(Vcb, &fileref, &fn, related, IrpSp->Flags & SL_OPEN_TARGET_DIRECTORY, &parsed, &fn_offset,
                                  pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, Irp);
        }
    }

    if (Status == STATUS_REPARSE) {
        REPARSE_DATA_BUFFER* data;

        ExAcquireResourceSharedLite(fileref->fcb->Header.Resource, TRUE);
        Status = get_reparse_block(fileref->fcb, (UINT8**)&data);
        ExReleaseResourceLite(fileref->fcb->Header.Resource);

        if (!NT_SUCCESS(Status)) {
            ERR("get_reparse_block returned %08x\n", Status);

            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_REPARSE;
            RtlCopyMemory(&Irp->IoStatus.Information, data, sizeof(ULONG));

            data->Reserved = FileObject->FileName.Length - parsed;

            Irp->Tail.Overlay.AuxiliaryBuffer = (void*)data;

            free_fileref(Vcb, fileref);
            release_fcb_lock(Vcb);

            goto exit;
        }
    }

    if (NT_SUCCESS(Status) && fileref->deleted)
        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (NT_SUCCESS(Status)) {
        if (RequestedDisposition == FILE_CREATE) {
            TRACE("file %S already exists, returning STATUS_OBJECT_NAME_COLLISION\n", file_desc_fileref(fileref));
            Status = STATUS_OBJECT_NAME_COLLISION;

            free_fileref(Vcb, fileref);
            release_fcb_lock(Vcb);

            goto exit;
        }
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        if (RequestedDisposition == FILE_OPEN || RequestedDisposition == FILE_OVERWRITE) {
            TRACE("file doesn't exist, returning STATUS_OBJECT_NAME_NOT_FOUND\n");
            release_fcb_lock(Vcb);
            goto exit;
        }
    } else if (Status == STATUS_OBJECT_PATH_NOT_FOUND) {
        TRACE("open_fileref returned %08x\n", Status);
        release_fcb_lock(Vcb);
        goto exit;
    } else {
        ERR("open_fileref returned %08x\n", Status);
        release_fcb_lock(Vcb);
        goto exit;
    }

    if (NT_SUCCESS(Status)) { // file already exists
        file_ref* sf;
        BOOL readonly;

        release_fcb_lock(Vcb);

        if (RequestedDisposition == FILE_SUPERSEDE || RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF) {
            LARGE_INTEGER zero;

#ifdef DEBUG_STATS
            open_type = 1;
#endif
            if (fileref->fcb->type == BTRFS_TYPE_DIRECTORY || is_subvol_readonly(fileref->fcb->subvol, Irp)) {
                Status = STATUS_ACCESS_DENIED;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }

            if (Vcb->readonly) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }

            zero.QuadPart = 0;
            if (!MmCanFileBeTruncated(&fileref->fcb->nonpaged->segment_object, &zero)) {
                Status = STATUS_USER_MAPPED_FILE;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
        }

        if (IrpSp->Parameters.Create.SecurityContext->DesiredAccess != 0) {
            SeLockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

            if (!SeAccessCheck((fileref->fcb->ads || fileref->fcb == Vcb->dummy_fcb) ? fileref->parent->fcb->sd : fileref->fcb->sd,
                               &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                               TRUE, IrpSp->Parameters.Create.SecurityContext->DesiredAccess, 0, NULL,
                               IoGetFileObjectGenericMapping(), IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode,
                               &granted_access, &Status)) {
                SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
                TRACE("SeAccessCheck failed, returning %08x\n", Status);

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }

            SeUnlockSubjectContext(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
        } else
            granted_access = 0;

        TRACE("deleted = %s\n", fileref->deleted ? "TRUE" : "FALSE");

        sf = fileref;
        while (sf) {
            if (sf->delete_on_close) {
                TRACE("could not open as deletion pending\n");
                Status = STATUS_DELETE_PENDING;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
            sf = sf->parent;
        }

        readonly = (!fileref->fcb->ads && fileref->fcb->atts & FILE_ATTRIBUTE_READONLY) || (fileref->fcb->ads && fileref->parent->fcb->atts & FILE_ATTRIBUTE_READONLY) ||
                   is_subvol_readonly(fileref->fcb->subvol, Irp) || fileref->fcb == Vcb->dummy_fcb || Vcb->readonly;

        if (options & FILE_DELETE_ON_CLOSE && (fileref == Vcb->root_fileref || readonly)) {
            Status = STATUS_CANNOT_DELETE;

            acquire_fcb_lock_exclusive(Vcb);
            free_fileref(Vcb, fileref);
            release_fcb_lock(Vcb);

            goto exit;
        }

        if (readonly) {
            ACCESS_MASK allowed;

            allowed = READ_CONTROL | SYNCHRONIZE | ACCESS_SYSTEM_SECURITY | FILE_READ_DATA |
                      FILE_READ_EA | FILE_READ_ATTRIBUTES | FILE_EXECUTE | FILE_LIST_DIRECTORY |
                      FILE_TRAVERSE;

            if (!Vcb->readonly && (fileref->fcb == Vcb->dummy_fcb || fileref->fcb->inode == SUBVOL_ROOT_INODE))
                allowed |= DELETE;

            if (fileref->fcb != Vcb->dummy_fcb && !is_subvol_readonly(fileref->fcb->subvol, Irp) && !Vcb->readonly) {
                allowed |= DELETE | WRITE_OWNER | WRITE_DAC | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;

                if (!fileref->fcb->ads && fileref->fcb->type == BTRFS_TYPE_DIRECTORY)
                    allowed |= FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | FILE_DELETE_CHILD;
            } else if (fileref->fcb->inode == SUBVOL_ROOT_INODE && is_subvol_readonly(fileref->fcb->subvol, Irp) && !Vcb->readonly) {
                // We allow a subvolume root to be opened read-write even if its readonly flag is set, so it can be cleared

                allowed |= FILE_WRITE_ATTRIBUTES;
            }

            if (IrpSp->Parameters.Create.SecurityContext->DesiredAccess & MAXIMUM_ALLOWED) {
                granted_access &= allowed;
                IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess &= allowed;
            } else if (granted_access & ~allowed) {
                Status = Vcb->readonly ? STATUS_MEDIA_WRITE_PROTECTED : STATUS_ACCESS_DENIED;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
        }

        if ((fileref->fcb->type == BTRFS_TYPE_SYMLINK || fileref->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) && !(options & FILE_OPEN_REPARSE_POINT))  {
            REPARSE_DATA_BUFFER* data;

            /* How reparse points work from the point of view of the filesystem appears to
             * undocumented. When returning STATUS_REPARSE, MSDN encourages us to return
             * IO_REPARSE in Irp->IoStatus.Information, but that means we have to do our own
             * translation. If we instead return the reparse tag in Information, and store
             * a pointer to the reparse data buffer in Irp->Tail.Overlay.AuxiliaryBuffer,
             * IopSymlinkProcessReparse will do the translation for us. */

            Status = get_reparse_block(fileref->fcb, (UINT8**)&data);
            if (!NT_SUCCESS(Status)) {
                ERR("get_reparse_block returned %08x\n", Status);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_REPARSE;
                Irp->IoStatus.Information = data->ReparseTag;

                if (fn.Buffer[(fn.Length / sizeof(WCHAR)) - 1] == '\\')
                    data->Reserved = sizeof(WCHAR);

                Irp->Tail.Overlay.AuxiliaryBuffer = (void*)data;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
        }

        if (fileref->fcb->type == BTRFS_TYPE_DIRECTORY && !fileref->fcb->ads) {
            if (options & FILE_NON_DIRECTORY_FILE && !(fileref->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)) {
                Status = STATUS_FILE_IS_A_DIRECTORY;

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
        } else if (options & FILE_DIRECTORY_FILE) {
            TRACE("returning STATUS_NOT_A_DIRECTORY (type = %u, %S)\n", fileref->fcb->type, file_desc_fileref(fileref));
            Status = STATUS_NOT_A_DIRECTORY;

            acquire_fcb_lock_exclusive(Vcb);
            free_fileref(Vcb, fileref);
            release_fcb_lock(Vcb);

            goto exit;
        }

        if (fileref->open_count > 0) {
            Status = IoCheckShareAccess(granted_access, IrpSp->Parameters.Create.ShareAccess, FileObject, &fileref->fcb->share_access, FALSE);

            if (!NT_SUCCESS(Status)) {
                if (Status == STATUS_SHARING_VIOLATION)
                    TRACE("IoCheckShareAccess failed, returning %08x\n", Status);
                else
                    WARN("IoCheckShareAccess failed, returning %08x\n", Status);

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }

            IoUpdateShareAccess(FileObject, &fileref->fcb->share_access);
        } else
            IoSetShareAccess(granted_access, IrpSp->Parameters.Create.ShareAccess, FileObject, &fileref->fcb->share_access);

        if (granted_access & FILE_WRITE_DATA || options & FILE_DELETE_ON_CLOSE) {
            if (!MmFlushImageSection(&fileref->fcb->nonpaged->segment_object, MmFlushForWrite)) {
                Status = (options & FILE_DELETE_ON_CLOSE) ? STATUS_CANNOT_DELETE : STATUS_SHARING_VIOLATION;

                IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }
        }

        if (RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF || RequestedDisposition == FILE_SUPERSEDE) {
            ULONG defda, oldatts, filter;
            LARGE_INTEGER time;
            BTRFS_TIME now;

            if ((RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF) && readonly) {
                WARN("cannot overwrite readonly file\n");
                Status = STATUS_ACCESS_DENIED;

                IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                goto exit;
            }

            if (!fileref->fcb->ads && (IrpSp->Parameters.Create.FileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != ((fileref->fcb->atts & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)))) {
                IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                acquire_fcb_lock_exclusive(Vcb);
                free_fileref(Vcb, fileref);
                release_fcb_lock(Vcb);

                Status = STATUS_ACCESS_DENIED;
                goto exit;
            }

            if (fileref->fcb->ads) {
                Status = stream_set_end_of_file_information(Vcb, 0, fileref->fcb, fileref, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("stream_set_end_of_file_information returned %08x\n", Status);

                    IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                    acquire_fcb_lock_exclusive(Vcb);
                    free_fileref(Vcb, fileref);
                    release_fcb_lock(Vcb);

                    goto exit;
                }
            } else {
                Status = truncate_file(fileref->fcb, 0, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("truncate_file returned %08x\n", Status);

                    IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                    acquire_fcb_lock_exclusive(Vcb);
                    free_fileref(Vcb, fileref);
                    release_fcb_lock(Vcb);

                    goto exit;
                }
            }

            if (Irp->Overlay.AllocationSize.QuadPart > 0) {
                Status = extend_file(fileref->fcb, fileref, Irp->Overlay.AllocationSize.QuadPart, TRUE, NULL, rollback);

                if (!NT_SUCCESS(Status)) {
                    ERR("extend_file returned %08x\n", Status);

                    IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                    acquire_fcb_lock_exclusive(Vcb);
                    free_fileref(Vcb, fileref);
                    release_fcb_lock(Vcb);

                    goto exit;
                }
            }

            if (!fileref->fcb->ads) {
                LIST_ENTRY* le;

                if (Irp->AssociatedIrp.SystemBuffer && IrpSp->Parameters.Create.EaLength > 0) {
                    ULONG offset;
                    FILE_FULL_EA_INFORMATION* eainfo;

                    Status = IoCheckEaBufferValidity(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Create.EaLength, &offset);
                    if (!NT_SUCCESS(Status)) {
                        ERR("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);

                        IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                        acquire_fcb_lock_exclusive(Vcb);
                        free_fileref(Vcb, fileref);
                        release_fcb_lock(Vcb);

                        goto exit;
                    }

                    fileref->fcb->ealen = 4;

                    // capitalize EA name
                    eainfo = Irp->AssociatedIrp.SystemBuffer;
                    do {
                        STRING s;

                        s.Length = s.MaximumLength = eainfo->EaNameLength;
                        s.Buffer = eainfo->EaName;

                        RtlUpperString(&s, &s);

                        fileref->fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;

                        if (eainfo->NextEntryOffset == 0)
                            break;

                        eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
                    } while (TRUE);

                    if (fileref->fcb->ea_xattr.Buffer)
                        ExFreePool(fileref->fcb->ea_xattr.Buffer);

                    fileref->fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(pool_type, IrpSp->Parameters.Create.EaLength, ALLOC_TAG);
                    if (!fileref->fcb->ea_xattr.Buffer) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;

                        IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                        acquire_fcb_lock_exclusive(Vcb);
                        free_fileref(Vcb, fileref);
                        release_fcb_lock(Vcb);

                        goto exit;
                    }

                    fileref->fcb->ea_xattr.Length = fileref->fcb->ea_xattr.MaximumLength = (USHORT)IrpSp->Parameters.Create.EaLength;
                    RtlCopyMemory(fileref->fcb->ea_xattr.Buffer, Irp->AssociatedIrp.SystemBuffer, fileref->fcb->ea_xattr.Length);
                } else {
                    if (fileref->fcb->ea_xattr.Length > 0) {
                        ExFreePool(fileref->fcb->ea_xattr.Buffer);
                        fileref->fcb->ea_xattr.Buffer = NULL;
                        fileref->fcb->ea_xattr.Length = fileref->fcb->ea_xattr.MaximumLength = 0;

                        fileref->fcb->ea_changed = TRUE;
                        fileref->fcb->ealen = 0;
                    }
                }

                // remove streams and send notifications
                le = fileref->fcb->dir_children_index.Flink;
                while (le != &fileref->fcb->dir_children_index) {
                    dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_index);
                    LIST_ENTRY* le2 = le->Flink;

                    if (dc->index == 0) {
                        if (!dc->fileref) {
                            file_ref* fr2;

                            Status = open_fileref_child(Vcb, fileref, &dc->name, TRUE, TRUE, TRUE, PagedPool, &fr2, NULL);
                            if (!NT_SUCCESS(Status))
                                WARN("open_fileref_child returned %08x\n", Status);
                        }

                        if (dc->fileref) {
                            send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_STREAM_NAME, FILE_ACTION_REMOVED_STREAM, &dc->name);

                            Status = delete_fileref(dc->fileref, NULL, NULL, rollback);
                            if (!NT_SUCCESS(Status)) {
                                ERR("delete_fileref returned %08x\n", Status);

                                acquire_fcb_lock_exclusive(Vcb);
                                free_fileref(Vcb, fileref);
                                release_fcb_lock(Vcb);

                                goto exit;
                            }
                        }
                    } else
                        break;

                    le = le2;
                }
            }

            KeQuerySystemTime(&time);
            win_time_to_unix(time, &now);

            filter = FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;

            if (fileref->fcb->ads) {
                fileref->parent->fcb->inode_item.st_mtime = now;
                fileref->parent->fcb->inode_item_changed = TRUE;
                mark_fcb_dirty(fileref->parent->fcb);

                send_notification_fcb(fileref->parent, filter, FILE_ACTION_MODIFIED, &fileref->dc->name);
            } else {
                mark_fcb_dirty(fileref->fcb);

                oldatts = fileref->fcb->atts;

                defda = get_file_attributes(Vcb, fileref->fcb->subvol, fileref->fcb->inode, fileref->fcb->type,
                                            fileref->dc && fileref->dc->name.Length >= sizeof(WCHAR) && fileref->dc->name.Buffer[0] == '.', TRUE, Irp);

                if (RequestedDisposition == FILE_SUPERSEDE)
                    fileref->fcb->atts = IrpSp->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
                else
                    fileref->fcb->atts |= IrpSp->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;

                if (fileref->fcb->atts != oldatts) {
                    fileref->fcb->atts_changed = TRUE;
                    fileref->fcb->atts_deleted = IrpSp->Parameters.Create.FileAttributes == defda;
                    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
                }

                fileref->fcb->inode_item.transid = Vcb->superblock.generation;
                fileref->fcb->inode_item.sequence++;
                fileref->fcb->inode_item.st_ctime = now;
                fileref->fcb->inode_item.st_mtime = now;
                fileref->fcb->inode_item_changed = TRUE;

                send_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED, NULL);
            }
        } else {
            if (options & FILE_NO_EA_KNOWLEDGE && fileref->fcb->ea_xattr.Length > 0) {
                FILE_FULL_EA_INFORMATION* ffei = (FILE_FULL_EA_INFORMATION*)fileref->fcb->ea_xattr.Buffer;

                do {
                    if (ffei->Flags & FILE_NEED_EA) {
                        WARN("returning STATUS_ACCESS_DENIED as no EA knowledge\n");
                        Status = STATUS_ACCESS_DENIED;

                        IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

                        acquire_fcb_lock_exclusive(Vcb);
                        free_fileref(Vcb, fileref);
                        release_fcb_lock(Vcb);

                        goto exit;
                    }

                    if (ffei->NextEntryOffset == 0)
                        break;

                    ffei = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ffei) + ffei->NextEntryOffset);
                } while (TRUE);
            }
        }

        FileObject->FsContext = fileref->fcb;

        ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
        if (!ccb) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;

            IoRemoveShareAccess(FileObject, &fileref->fcb->share_access);

            acquire_fcb_lock_exclusive(Vcb);
            free_fileref(Vcb, fileref);
            release_fcb_lock(Vcb);

            goto exit;
        }

        RtlZeroMemory(ccb, sizeof(*ccb));

        ccb->NodeType = BTRFS_NODE_TYPE_CCB;
        ccb->NodeSize = sizeof(*ccb);
        ccb->disposition = RequestedDisposition;
        ccb->options = options;
        ccb->query_dir_offset = 0;
        RtlInitUnicodeString(&ccb->query_string, NULL);
        ccb->has_wildcard = FALSE;
        ccb->specific_file = FALSE;
        ccb->access = granted_access;
        ccb->case_sensitive = IrpSp->Flags & SL_CASE_SENSITIVE;
        ccb->reserving = FALSE;
        ccb->lxss = called_from_lxss();

        ccb->fileref = fileref;

        FileObject->FsContext2 = ccb;
        FileObject->SectionObjectPointer = &fileref->fcb->nonpaged->segment_object;

        if (NT_SUCCESS(Status)) {
            switch (RequestedDisposition) {
                case FILE_SUPERSEDE:
                    Irp->IoStatus.Information = FILE_SUPERSEDED;
                    break;

                case FILE_OPEN:
                case FILE_OPEN_IF:
                    Irp->IoStatus.Information = FILE_OPENED;
                    break;

                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                    Irp->IoStatus.Information = FILE_OVERWRITTEN;
                    break;
            }
        }

        // Make sure paging files don't have any extents marked as being prealloc,
        // as this would mean we'd have to lock exclusively when writing.
        if (IrpSp->Flags & SL_OPEN_PAGING_FILE) {
            LIST_ENTRY* le;
            BOOL changed = FALSE;

            ExAcquireResourceExclusiveLite(fileref->fcb->Header.Resource, TRUE);

            le = fileref->fcb->extents.Flink;

            while (le != &fileref->fcb->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);

                if (ext->extent_data.type == EXTENT_TYPE_PREALLOC) {
                    ext->extent_data.type = EXTENT_TYPE_REGULAR;
                    changed = TRUE;
                }

                le = le->Flink;
            }

            ExReleaseResourceLite(fileref->fcb->Header.Resource);

            if (changed) {
                fileref->fcb->extents_changed = TRUE;
                mark_fcb_dirty(fileref->fcb);
            }

            fileref->fcb->Header.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
            Vcb->disallow_dismount = TRUE;
        }

#ifdef DEBUG_FCB_REFCOUNTS
        oc = InterlockedIncrement(&fileref->open_count);
        ERR("fileref %p: open_count now %i\n", fileref, oc);
#else
        InterlockedIncrement(&fileref->open_count);
#endif
        InterlockedIncrement(&Vcb->open_files);
    } else {
#ifdef DEBUG_STATS
        open_type = 2;
#endif
        Status = file_create(Irp, Vcb, FileObject, related, loaded_related, &fn, RequestedDisposition, options, rollback);
        release_fcb_lock(Vcb);

        Irp->IoStatus.Information = NT_SUCCESS(Status) ? FILE_CREATED : 0;
        granted_access = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    }

    if (NT_SUCCESS(Status) && !(options & FILE_NO_INTERMEDIATE_BUFFERING))
        FileObject->Flags |= FO_CACHE_SUPPORTED;

exit:
    if (loaded_related) {
        acquire_fcb_lock_exclusive(Vcb);
        free_fileref(Vcb, related);
        release_fcb_lock(Vcb);
    }

    if (Status == STATUS_SUCCESS) {
        fcb* fcb2;

        IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess |= granted_access;
        IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess &= ~(granted_access | MAXIMUM_ALLOWED);

        if (!FileObject->Vpb)
            FileObject->Vpb = DeviceObject->Vpb;

        fcb2 = FileObject->FsContext;

        if (fcb2->ads) {
            struct _ccb* ccb2 = FileObject->FsContext2;

            fcb2 = ccb2->fileref->parent->fcb;
        }

        ExAcquireResourceExclusiveLite(fcb2->Header.Resource, TRUE);
        fcb_load_csums(Vcb, fcb2, Irp);
        ExReleaseResourceLite(fcb2->Header.Resource);
    } else if (Status != STATUS_REPARSE && Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
        TRACE("returning %08x\n", Status);

#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);

    if (open_type == 0) {
        Vcb->stats.open_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_opens++;
    } else if (open_type == 1) {
        Vcb->stats.overwrite_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_overwrites++;
    } else if (open_type == 2) {
        Vcb->stats.create_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_creates++;
    }
#endif

    return Status;
}

static NTSTATUS verify_vcb(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    BOOL need_verify = FALSE;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj && dev->removable) {
            ULONG cc;
            IO_STATUS_BLOCK iosb;

            Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), TRUE, &iosb);

            if (IoIsErrorUserInduced(Status)) {
                ERR("IOCTL_STORAGE_CHECK_VERIFY returned %08x (user-induced)\n", Status);
                need_verify = TRUE;
            } else if (!NT_SUCCESS(Status)) {
                ERR("IOCTL_STORAGE_CHECK_VERIFY returned %08x\n", Status);
                goto end;
            } else if (iosb.Information < sizeof(ULONG)) {
                ERR("iosb.Information was too short\n");
                Status = STATUS_INTERNAL_ERROR;
            } else if (cc != dev->change_count) {
                dev->devobj->Flags |= DO_VERIFY_VOLUME;
                need_verify = TRUE;
            }
        }

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    if (need_verify) {
        PDEVICE_OBJECT devobj;

        devobj = IoGetDeviceToVerify(Irp->Tail.Overlay.Thread);
        IoSetDeviceToVerify(Irp->Tail.Overlay.Thread, NULL);

        if (!devobj) {
            devobj = IoGetDeviceToVerify(PsGetCurrentThread());
            IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
        }

        devobj = Vcb->Vpb ? Vcb->Vpb->RealDevice : NULL;

        if (devobj)
            Status = IoVerifyVolume(devobj, FALSE);
        else
            Status = STATUS_VERIFY_REQUIRED;
    }

    return Status;
}

static BOOL has_manage_volume_privilege(ACCESS_STATE* access_state, KPROCESSOR_MODE processor_mode) {
    PRIVILEGE_SET privset;

    privset.PrivilegeCount = 1;
    privset.Control = PRIVILEGE_SET_ALL_NECESSARY;
    privset.Privilege[0].Luid = RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE);
    privset.Privilege[0].Attributes = 0;

    return SePrivilegeCheck(&privset, &access_state->SubjectSecurityContext, processor_mode) ? TRUE : FALSE;
}

_Dispatch_type_(IRP_MJ_CREATE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level, locked = FALSE;

    FsRtlEnterFileSystem();

    TRACE("create (flags = %x)\n", Irp->Flags);

    top_level = is_top_level(Irp);

    /* return success if just called for FS device object */
    if (DeviceObject == master_devobj)  {
        TRACE("create called for FS device object\n");

        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;

        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_create(DeviceObject, Irp);
        goto exit;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (!(Vcb->Vpb->Flags & VPB_MOUNTED)) {
        Status = STATUS_DEVICE_NOT_READY;
        goto exit;
    }

    if (Vcb->removing) {
        Status = STATUS_ACCESS_DENIED;
        goto exit;
    }

    Status = verify_vcb(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("verify_vcb returned %08x\n", Status);
        goto exit;
    }

    ExAcquireResourceSharedLite(&Vcb->load_lock, TRUE);
    locked = TRUE;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->Flags != 0) {
        UINT32 flags = IrpSp->Flags;

        TRACE("flags:\n");

        if (flags & SL_CASE_SENSITIVE) {
            TRACE("SL_CASE_SENSITIVE\n");
            flags &= ~SL_CASE_SENSITIVE;
        }

        if (flags & SL_FORCE_ACCESS_CHECK) {
            TRACE("SL_FORCE_ACCESS_CHECK\n");
            flags &= ~SL_FORCE_ACCESS_CHECK;
        }

        if (flags & SL_OPEN_PAGING_FILE) {
            TRACE("SL_OPEN_PAGING_FILE\n");
            flags &= ~SL_OPEN_PAGING_FILE;
        }

        if (flags & SL_OPEN_TARGET_DIRECTORY) {
            TRACE("SL_OPEN_TARGET_DIRECTORY\n");
            flags &= ~SL_OPEN_TARGET_DIRECTORY;
        }

        if (flags & SL_STOP_ON_SYMLINK) {
            TRACE("SL_STOP_ON_SYMLINK\n");
            flags &= ~SL_STOP_ON_SYMLINK;
        }

        if (flags)
            WARN("unknown flags: %x\n", flags);
    } else {
        TRACE("flags: (none)\n");
    }

    if (!IrpSp->FileObject) {
        ERR("FileObject was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (IrpSp->FileObject->RelatedFileObject) {
        fcb* relatedfcb = IrpSp->FileObject->RelatedFileObject->FsContext;

        if (relatedfcb && relatedfcb->Vcb != Vcb) {
            WARN("RelatedFileObject was for different device\n");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
    }

    // opening volume
    if (IrpSp->FileObject->FileName.Length == 0 && !IrpSp->FileObject->RelatedFileObject) {
        ULONG RequestedDisposition = ((IrpSp->Parameters.Create.Options >> 24) & 0xff);
        ULONG RequestedOptions = IrpSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
#ifdef DEBUG_FCB_REFCOUNTS
        LONG rc;
#endif
        ccb* ccb;

        TRACE("open operation for volume\n");

        if (RequestedDisposition != FILE_OPEN && RequestedDisposition != FILE_OPEN_IF) {
            Status = STATUS_ACCESS_DENIED;
            goto exit;
        }

        if (RequestedOptions & FILE_DIRECTORY_FILE) {
            Status = STATUS_NOT_A_DIRECTORY;
            goto exit;
        }

        ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
        if (!ccb) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(ccb, sizeof(*ccb));

        ccb->NodeType = BTRFS_NODE_TYPE_CCB;
        ccb->NodeSize = sizeof(*ccb);
        ccb->disposition = RequestedDisposition;
        ccb->options = RequestedOptions;
        ccb->access = IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;
        ccb->manage_volume_privilege = has_manage_volume_privilege(IrpSp->Parameters.Create.SecurityContext->AccessState,
                                                                   IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode);
        ccb->reserving = FALSE;
        ccb->lxss = called_from_lxss();

#ifdef DEBUG_FCB_REFCOUNTS
        rc = InterlockedIncrement(&Vcb->volume_fcb->refcount);
        WARN("fcb %p: refcount now %i (volume)\n", Vcb->volume_fcb, rc);
#else
        InterlockedIncrement(&Vcb->volume_fcb->refcount);
#endif
        IrpSp->FileObject->FsContext = Vcb->volume_fcb;
        IrpSp->FileObject->FsContext2 = ccb;

        IrpSp->FileObject->SectionObjectPointer = &Vcb->volume_fcb->nonpaged->segment_object;

        if (!IrpSp->FileObject->Vpb)
            IrpSp->FileObject->Vpb = DeviceObject->Vpb;

        InterlockedIncrement(&Vcb->open_files);

        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
    } else {
        LIST_ENTRY rollback;
        BOOL skip_lock;

        InitializeListHead(&rollback);

        TRACE("file name: %.*S\n", IrpSp->FileObject->FileName.Length / sizeof(WCHAR), IrpSp->FileObject->FileName.Buffer);

        if (IrpSp->FileObject->RelatedFileObject)
            TRACE("related file = %S\n", file_desc(IrpSp->FileObject->RelatedFileObject));

        // Don't lock again if we're being called from within CcCopyRead etc.
        skip_lock = ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock);

        if (!skip_lock)
            ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

        Status = open_file(DeviceObject, Vcb, Irp, &rollback);

        if (!NT_SUCCESS(Status))
            do_rollback(Vcb, &rollback);
        else
            clear_rollback(&rollback);

        if (!skip_lock)
            ExReleaseResourceLite(&Vcb->tree_lock);
    }

exit:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT );

    TRACE("create returning %08x\n", Status);

    if (locked)
        ExReleaseResourceLite(&Vcb->load_lock);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
