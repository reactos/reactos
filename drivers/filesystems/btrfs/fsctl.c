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
#include "crc32c.h"
#include <ntddstor.h>
#include <ntdddisk.h>
#ifndef __REACTOS__
#include <sys/stat.h>
#endif

#ifndef FSCTL_CSV_CONTROL
#define FSCTL_CSV_CONTROL CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 181, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_QUERY_VOLUME_CONTAINER_STATE
#define FSCTL_QUERY_VOLUME_CONTAINER_STATE CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 228, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define DOTDOT ".."

#define SEF_AVOID_PRIVILEGE_CHECK 0x08 // on MSDN but not in any header files(?)

#define SEF_SACL_AUTO_INHERIT 0x02

extern LIST_ENTRY VcbList;
extern ERESOURCE global_loading_lock;
extern PDRIVER_OBJECT drvobj;
extern tFsRtlCheckLockForOplockRequest fFsRtlCheckLockForOplockRequest;
extern tFsRtlAreThereCurrentOrInProgressFileLocks fFsRtlAreThereCurrentOrInProgressFileLocks;

static void mark_subvol_dirty(device_extension* Vcb, root* r);

static NTSTATUS get_file_ids(PFILE_OBJECT FileObject, void* data, ULONG length) {
    btrfs_get_file_ids* bgfi;
    fcb* fcb;

    if (length < sizeof(btrfs_get_file_ids))
        return STATUS_BUFFER_OVERFLOW;

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    bgfi = data;

    bgfi->subvol = fcb->subvol->id;
    bgfi->inode = fcb->inode;
    bgfi->top = fcb->Vcb->root_fileref->fcb == fcb ? true : false;

    return STATUS_SUCCESS;
}

static void get_uuid(BTRFS_UUID* uuid) {
    LARGE_INTEGER seed;
    uint8_t i;

    seed = KeQueryPerformanceCounter(NULL);

    for (i = 0; i < 16; i+=2) {
        ULONG rand = RtlRandomEx(&seed.LowPart);

        uuid->uuid[i] = (rand & 0xff00) >> 8;
        uuid->uuid[i+1] = rand & 0xff;
    }
}

static NTSTATUS snapshot_tree_copy(device_extension* Vcb, uint64_t addr, root* subvol, uint64_t* newaddr, PIRP Irp, LIST_ENTRY* rollback) {
    uint8_t* buf;
    NTSTATUS Status;
    write_data_context wtc;
    LIST_ENTRY* le;
    tree t;
    tree_header* th;
    chunk* c;

    buf = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wtc.parity1 = wtc.parity2 = wtc.scratch = NULL;
    wtc.mdl = wtc.parity1_mdl = wtc.parity2_mdl = NULL;

    Status = read_data(Vcb, addr, Vcb->superblock.node_size, NULL, true, buf, NULL, NULL, Irp, 0, false, NormalPagePriority);
    if (!NT_SUCCESS(Status)) {
        ERR("read_data returned %08lx\n", Status);
        goto end;
    }

    th = (tree_header*)buf;

    RtlZeroMemory(&t, sizeof(tree));
    t.root = subvol;
    t.header.level = th->level;
    t.header.tree_id = t.root->id;

    Status = get_tree_new_address(Vcb, &t, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("get_tree_new_address returned %08lx\n", Status);
        goto end;
    }

    if (!t.has_new_address) {
        ERR("tree new address not set\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    c = get_chunk_from_address(Vcb, t.new_address);

    if (c)
        c->used += Vcb->superblock.node_size;
    else {
        ERR("could not find chunk for address %I64x\n", t.new_address);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    th->address = t.new_address;
    th->tree_id = subvol->id;
    th->generation = Vcb->superblock.generation;
    th->fs_uuid = Vcb->superblock.metadata_uuid;

    if (th->level == 0) {
        uint32_t i;
        leaf_node* ln = (leaf_node*)&th[1];

        for (i = 0; i < th->num_items; i++) {
            if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) && ln[i].offset + ln[i].size <= Vcb->superblock.node_size - sizeof(tree_header)) {
                EXTENT_DATA* ed = (EXTENT_DATA*)(((uint8_t*)&th[1]) + ln[i].offset);

                if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];

                    if (ed2->size != 0) { // not sparse
                        Status = increase_extent_refcount_data(Vcb, ed2->address, ed2->size, subvol->id, ln[i].key.obj_id, ln[i].key.offset - ed2->offset, 1, Irp);

                        if (!NT_SUCCESS(Status)) {
                            ERR("increase_extent_refcount_data returned %08lx\n", Status);
                            goto end;
                        }
                    }
                }
            }
        }
    } else {
        uint32_t i;
        internal_node* in = (internal_node*)&th[1];

        for (i = 0; i < th->num_items; i++) {
            TREE_BLOCK_REF tbr;

            tbr.offset = subvol->id;

            Status = increase_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr, NULL, th->level - 1, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("increase_extent_refcount returned %08lx\n", Status);
                goto end;
            }
        }
    }

    calc_tree_checksum(Vcb, th);

    KeInitializeEvent(&wtc.Event, NotificationEvent, false);
    InitializeListHead(&wtc.stripes);
    wtc.stripes_left = 0;

    Status = write_data(Vcb, t.new_address, buf, Vcb->superblock.node_size, &wtc, NULL, NULL, false, 0, NormalPagePriority);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08lx\n", Status);
        goto end;
    }

    if (wtc.stripes.Flink != &wtc.stripes) {
        bool need_wait = false;

        // launch writes and wait
        le = wtc.stripes.Flink;
        while (le != &wtc.stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (stripe->status != WriteDataStatus_Ignore) {
                need_wait = true;
                IoCallDriver(stripe->device->devobj, stripe->Irp);
            }

            le = le->Flink;
        }

        if (need_wait)
            KeWaitForSingleObject(&wtc.Event, Executive, KernelMode, false, NULL);

        le = wtc.stripes.Flink;
        while (le != &wtc.stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                log_device_error(Vcb, stripe->device, BTRFS_DEV_STAT_WRITE_ERRORS);
                break;
            }

            le = le->Flink;
        }

        free_write_data_stripes(&wtc);
        buf = NULL;
    }

    if (NT_SUCCESS(Status))
        *newaddr = t.new_address;

end:

    if (buf)
        ExFreePool(buf);

    return Status;
}

void flush_subvol_fcbs(root* subvol) {
    LIST_ENTRY* le = subvol->fcbs.Flink;

    if (IsListEmpty(&subvol->fcbs))
        return;

    while (le != &subvol->fcbs) {
        struct _fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
        IO_STATUS_BLOCK iosb;

        if (fcb->type != BTRFS_TYPE_DIRECTORY && !fcb->deleted)
            CcFlushCache(&fcb->nonpaged->segment_object, NULL, 0, &iosb);

        le = le->Flink;
    }
}

static NTSTATUS do_create_snapshot(device_extension* Vcb, PFILE_OBJECT parent, fcb* subvol_fcb, PANSI_STRING utf8, PUNICODE_STRING name, bool readonly, PIRP Irp) {
    LIST_ENTRY rollback;
    uint64_t id;
    NTSTATUS Status;
    root *r, *subvol = subvol_fcb->subvol;
    KEY searchkey;
    traverse_ptr tp;
    uint64_t address, *root_num;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    fcb* fcb = parent->FsContext;
    ccb* ccb = parent->FsContext2;
    LIST_ENTRY* le;
    file_ref *fileref, *fr;
    dir_child* dc = NULL;

    if (!ccb) {
        ERR("error - ccb was NULL\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (!(ccb->access & FILE_ADD_SUBDIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    fileref = ccb->fileref;

    if (fileref->fcb == Vcb->dummy_fcb)
        return STATUS_ACCESS_DENIED;

    // flush open files on this subvol

    flush_subvol_fcbs(subvol);

    // flush metadata

    if (Vcb->need_write)
        Status = do_write(Vcb, Irp);
    else
        Status = STATUS_SUCCESS;

    free_trees(Vcb);

    if (!NT_SUCCESS(Status)) {
        ERR("do_write returned %08lx\n", Status);
        return Status;
    }

    InitializeListHead(&rollback);

    // create new root

    id = InterlockedIncrement64(&Vcb->root_root->lastinode);
    Status = create_root(Vcb, id, &r, true, Vcb->superblock.generation, Irp);

    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08lx\n", Status);
        goto end;
    }

    r->lastinode = subvol->lastinode;

    if (!Vcb->uuid_root) {
        root* uuid_root;

        TRACE("uuid root doesn't exist, creating it\n");

        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, false, 0, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("create_root returned %08lx\n", Status);
            goto end;
        }

        Vcb->uuid_root = uuid_root;
    }

    root_num = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t), ALLOC_TAG);
    if (!root_num) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    tp.tree = NULL;

    do {
        get_uuid(&r->root_item.uuid);

        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid, sizeof(uint64_t));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(uint64_t)], sizeof(uint64_t));

        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, false, Irp);
    } while (NT_SUCCESS(Status) && !keycmp(searchkey, tp.item->key));

    *root_num = r->id;

    Status = insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(uint64_t), NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(root_num);
        goto end;
    }

    searchkey.obj_id = r->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        goto end;
    }

    Status = snapshot_tree_copy(Vcb, subvol->root_item.block_number, r, &address, Irp, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("snapshot_tree_copy returned %08lx\n", Status);
        goto end;
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    r->root_item.inode.generation = 1;
    r->root_item.inode.st_size = 3;
    r->root_item.inode.st_blocks = subvol->root_item.inode.st_blocks;
    r->root_item.inode.st_nlink = 1;
    r->root_item.inode.st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    r->root_item.inode.flags = 0x80000000; // FIXME - find out what these mean
    r->root_item.inode.flags_ro = 0xffffffff; // FIXME - find out what these mean
    r->root_item.generation = Vcb->superblock.generation;
    r->root_item.objid = subvol->root_item.objid;
    r->root_item.block_number = address;
    r->root_item.bytes_used = subvol->root_item.bytes_used;
    r->root_item.last_snapshot_generation = Vcb->superblock.generation;
    r->root_item.root_level = subvol->root_item.root_level;
    r->root_item.generation2 = Vcb->superblock.generation;
    r->root_item.parent_uuid = subvol->root_item.uuid;
    r->root_item.ctransid = subvol->root_item.ctransid;
    r->root_item.otransid = Vcb->superblock.generation;
    r->root_item.ctime = subvol->root_item.ctime;
    r->root_item.otime = now;

    if (readonly)
        r->root_item.flags |= BTRFS_SUBVOL_READONLY;

    r->treeholder.address = address;

    // FIXME - do we need to copy over the send and receive fields too?

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("error - could not find ROOT_ITEM for subvol %I64x\n", r->id);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    RtlCopyMemory(tp.item->data, &r->root_item, sizeof(ROOT_ITEM));

    // update ROOT_ITEM of original subvol

    subvol->root_item.last_snapshot_generation = Vcb->superblock.generation;

    mark_subvol_dirty(Vcb, subvol);

    // create fileref for entry in other subvolume

    fr = create_fileref(Vcb);
    if (!fr) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    Status = open_fcb(Vcb, r, r->root_item.objid, BTRFS_TYPE_DIRECTORY, utf8, false, fcb, &fr->fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08lx\n", Status);
        free_fileref(fr);
        goto end;
    }

    fr->parent = fileref;

    Status = add_dir_child(fileref->fcb, r->id, true, utf8, name, BTRFS_TYPE_DIRECTORY, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08lx\n", Status);

    fr->dc = dc;
    dc->fileref = fr;

    ExAcquireResourceExclusiveLite(&fileref->fcb->nonpaged->dir_children_lock, true);
    InsertTailList(&fileref->children, &fr->list_entry);
    ExReleaseResourceLite(&fileref->fcb->nonpaged->dir_children_lock);

    increase_fileref_refcount(fileref);

    fr->created = true;
    mark_fileref_dirty(fr);

    if (fr->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr->fcb->fileref = fr;

    fr->fcb->subvol->parent = fileref->fcb->subvol->id;

    free_fileref(fr);

    // change fcb's INODE_ITEM

    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    fcb->inode_item.st_size += utf8->Length * 2;

    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    if (!ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    fcb->subvol->root_item.ctime = now;
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;

    send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED, NULL);
    send_notification_fileref(fr->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

    le = subvol->fcbs.Flink;
    while (le != &subvol->fcbs) {
        struct _fcb* fcb2 = CONTAINING_RECORD(le, struct _fcb, list_entry);
        LIST_ENTRY* le2 = fcb2->extents.Flink;

        while (le2 != &fcb2->extents) {
            extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

            if (!ext->ignore)
                ext->unique = false;

            le2 = le2->Flink;
        }

        le = le->Flink;
    }

    Status = do_write(Vcb, Irp);

    free_trees(Vcb);

    if (!NT_SUCCESS(Status))
        ERR("do_write returned %08lx\n", Status);

end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    return Status;
}

static NTSTATUS create_snapshot(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG length, PIRP Irp) {
    PFILE_OBJECT subvol_obj;
    NTSTATUS Status;
    btrfs_create_snapshot* bcs = data;
    fcb* subvol_fcb;
    HANDLE subvolh;
    bool readonly, posix;
    ANSI_STRING utf8;
    UNICODE_STRING nameus;
    ULONG len;
    fcb* fcb;
    ccb* ccb;
    file_ref *fileref, *fr2;

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        btrfs_create_snapshot32* bcs32 = data;

        if (length < offsetof(btrfs_create_snapshot32, name))
            return STATUS_INVALID_PARAMETER;

        if (length < offsetof(btrfs_create_snapshot32, name) + bcs32->namelen)
            return STATUS_INVALID_PARAMETER;

        subvolh = Handle32ToHandle(bcs32->subvol);

        nameus.Buffer = bcs32->name;
        nameus.Length = nameus.MaximumLength = bcs32->namelen;

        readonly = bcs32->readonly;
        posix = bcs32->posix;
    } else {
#endif
        if (length < offsetof(btrfs_create_snapshot, name))
            return STATUS_INVALID_PARAMETER;

        if (length < offsetof(btrfs_create_snapshot, name) + bcs->namelen)
            return STATUS_INVALID_PARAMETER;

        subvolh = bcs->subvol;

        nameus.Buffer = bcs->name;
        nameus.Length = nameus.MaximumLength = bcs->namelen;

        readonly = bcs->readonly;
        posix = bcs->posix;
#if defined(_WIN64)
    }
#endif

    if (!subvolh)
        return STATUS_INVALID_PARAMETER;

    if (!FileObject || !FileObject->FsContext)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!fcb || !ccb || fcb->type != BTRFS_TYPE_DIRECTORY)
        return STATUS_INVALID_PARAMETER;

    fileref = ccb->fileref;

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!(ccb->access & FILE_ADD_SUBDIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (is_subvol_readonly(fcb->subvol, Irp))
        return STATUS_ACCESS_DENIED;

    Status = check_file_name_valid(&nameus, posix, false);
    if (!NT_SUCCESS(Status))
        return Status;

    utf8.Buffer = NULL;

    Status = utf16_to_utf8(NULL, 0, &len, nameus.Buffer, nameus.Length);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 failed with error %08lx\n", Status);
        return Status;
    }

    if (len == 0) {
        ERR("utf16_to_utf8 returned a length of 0\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (len > 0xffff) {
        ERR("len was too long\n");
        return STATUS_INVALID_PARAMETER;
    }

    utf8.MaximumLength = utf8.Length = (USHORT)len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);

    if (!utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = utf16_to_utf8(utf8.Buffer, len, &len, nameus.Buffer, nameus.Length);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 failed with error %08lx\n", Status);
        goto end2;
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    // no need for fcb_lock as we have tree_lock exclusively
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, false, NULL, NULL, PagedPool, ccb->case_sensitive || posix, Irp);

    if (NT_SUCCESS(Status)) {
        if (!fr2->deleted) {
            WARN("file already exists\n");
            free_fileref(fr2);
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end3;
        } else
            free_fileref(fr2);
    } else if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("open_fileref returned %08lx\n", Status);
        goto end3;
    }

    Status = ObReferenceObjectByHandle(subvolh, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&subvol_obj, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08lx\n", Status);
        goto end3;
    }

    if (subvol_obj->DeviceObject != FileObject->DeviceObject) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    subvol_fcb = subvol_obj->FsContext;
    if (!subvol_fcb) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (subvol_fcb->inode != subvol_fcb->subvol->root_item.objid) {
        WARN("handle inode was %I64x, expected %I64x\n", subvol_fcb->inode, subvol_fcb->subvol->root_item.objid);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ccb = subvol_obj->FsContext2;

    if (!ccb) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!(ccb->access & FILE_TRAVERSE)) {
        WARN("insufficient privileges\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    if (fcb == Vcb->dummy_fcb) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    // clear unique flag on extents of open files in subvol
    if (!IsListEmpty(&subvol_fcb->subvol->fcbs)) {
        LIST_ENTRY* le = subvol_fcb->subvol->fcbs.Flink;

        while (le != &subvol_fcb->subvol->fcbs) {
            struct _fcb* openfcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
            LIST_ENTRY* le2;

            le2 = openfcb->extents.Flink;

            while (le2 != &openfcb->extents) {
                extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                ext->unique = false;

                le2 = le2->Flink;
            }

            le = le->Flink;
        }
    }

    Status = do_create_snapshot(Vcb, FileObject, subvol_fcb, &utf8, &nameus, readonly, Irp);

    if (NT_SUCCESS(Status)) {
        file_ref* fr;

        Status = open_fileref(Vcb, &fr, &nameus, fileref, false, NULL, NULL, PagedPool, false, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08lx\n", Status);
            Status = STATUS_SUCCESS;
        } else {
            send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED, NULL);
            free_fileref(fr);
        }
    }

end:
    ObDereferenceObject(subvol_obj);

end3:
    ExReleaseResourceLite(&Vcb->tree_lock);

end2:
    ExFreePool(utf8.Buffer);

    return Status;
}

static NTSTATUS create_subvol(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, PIRP Irp) {
    btrfs_create_subvol* bcs;
    fcb *fcb, *rootfcb = NULL;
    ccb* ccb;
    file_ref* fileref;
    NTSTATUS Status;
    uint64_t id;
    root* r = NULL;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ULONG len;
    uint16_t irsize;
    UNICODE_STRING nameus;
    ANSI_STRING utf8;
    INODE_REF* ir;
    KEY searchkey;
    traverse_ptr tp;
    SECURITY_SUBJECT_CONTEXT subjcont;
    PSID owner;
    BOOLEAN defaulted;
    uint64_t* root_num;
    file_ref *fr = NULL, *fr2;
    dir_child* dc = NULL;

    fcb = FileObject->FsContext;
    if (!fcb) {
        ERR("error - fcb was NULL\n");
        return STATUS_INTERNAL_ERROR;
    }

    ccb = FileObject->FsContext2;
    if (!ccb) {
        ERR("error - ccb was NULL\n");
        return STATUS_INTERNAL_ERROR;
    }

    fileref = ccb->fileref;

    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        ERR("parent FCB was not a directory\n");
        return STATUS_NOT_A_DIRECTORY;
    }

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fileref->deleted || fcb->deleted) {
        ERR("parent has been deleted\n");
        return STATUS_FILE_DELETED;
    }

    if (!(ccb->access & FILE_ADD_SUBDIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (is_subvol_readonly(fcb->subvol, Irp))
        return STATUS_ACCESS_DENIED;

    if (fcb == Vcb->dummy_fcb)
        return STATUS_ACCESS_DENIED;

    if (!data || datalen < sizeof(btrfs_create_subvol))
        return STATUS_INVALID_PARAMETER;

    bcs = (btrfs_create_subvol*)data;

    if (offsetof(btrfs_create_subvol, name[0]) + bcs->namelen > datalen)
        return STATUS_INVALID_PARAMETER;

    nameus.Length = nameus.MaximumLength = bcs->namelen;
    nameus.Buffer = bcs->name;

    Status = check_file_name_valid(&nameus, bcs->posix, false);
    if (!NT_SUCCESS(Status))
        return Status;

    utf8.Buffer = NULL;

    Status = utf16_to_utf8(NULL, 0, &len, nameus.Buffer, nameus.Length);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 failed with error %08lx\n", Status);
        return Status;
    }

    if (len == 0) {
        ERR("utf16_to_utf8 returned a length of 0\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (len > 0xffff) {
        ERR("len was too long\n");
        return STATUS_INVALID_PARAMETER;
    }

    utf8.MaximumLength = utf8.Length = (USHORT)len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);

    if (!utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = utf16_to_utf8(utf8.Buffer, len, &len, nameus.Buffer, nameus.Length);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 failed with error %08lx\n", Status);
        goto end2;
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    // no need for fcb_lock as we have tree_lock exclusively
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, false, NULL, NULL, PagedPool, ccb->case_sensitive || bcs->posix, Irp);

    if (NT_SUCCESS(Status)) {
        if (!fr2->deleted) {
            WARN("file already exists\n");
            free_fileref(fr2);
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        } else
            free_fileref(fr2);
    } else if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("open_fileref returned %08lx\n", Status);
        goto end;
    }

    id = InterlockedIncrement64(&Vcb->root_root->lastinode);
    Status = create_root(Vcb, id, &r, false, 0, Irp);

    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08lx\n", Status);
        goto end;
    }

    TRACE("created root %I64x\n", id);

    if (!Vcb->uuid_root) {
        root* uuid_root;

        TRACE("uuid root doesn't exist, creating it\n");

        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, false, 0, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("create_root returned %08lx\n", Status);
            goto end;
        }

        Vcb->uuid_root = uuid_root;
    }

    root_num = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t), ALLOC_TAG);
    if (!root_num) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    tp.tree = NULL;

    do {
        get_uuid(&r->root_item.uuid);

        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid, sizeof(uint64_t));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(uint64_t)], sizeof(uint64_t));

        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, false, Irp);
    } while (NT_SUCCESS(Status) && !keycmp(searchkey, tp.item->key));

    *root_num = r->id;

    Status = insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(uint64_t), NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(root_num);
        goto end;
    }

    r->root_item.inode.generation = 1;
    r->root_item.inode.st_size = 3;
    r->root_item.inode.st_blocks = Vcb->superblock.node_size;
    r->root_item.inode.st_nlink = 1;
    r->root_item.inode.st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    r->root_item.inode.flags = 0x80000000; // FIXME - find out what these mean
    r->root_item.inode.flags_ro = 0xffffffff; // FIXME - find out what these mean

    if (bcs->readonly)
        r->root_item.flags |= BTRFS_SUBVOL_READONLY;

    r->root_item.objid = SUBVOL_ROOT_INODE;
    r->root_item.bytes_used = Vcb->superblock.node_size;
    r->root_item.ctransid = Vcb->superblock.generation;
    r->root_item.otransid = Vcb->superblock.generation;
    r->root_item.ctime = now;
    r->root_item.otime = now;

    // add .. inode to new subvol

    rootfcb = create_fcb(Vcb, PagedPool);
    if (!rootfcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    rootfcb->Vcb = Vcb;

    rootfcb->subvol = r;
    rootfcb->type = BTRFS_TYPE_DIRECTORY;
    rootfcb->inode = SUBVOL_ROOT_INODE;
    rootfcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&rootfcb->inode, sizeof(uint64_t)); // FIXME - we can hardcode this

    rootfcb->inode_item.generation = Vcb->superblock.generation;
    rootfcb->inode_item.transid = Vcb->superblock.generation;
    rootfcb->inode_item.st_nlink = 1;
    rootfcb->inode_item.st_mode = __S_IFDIR | inherit_mode(fileref->fcb, true);
    rootfcb->inode_item.st_atime = rootfcb->inode_item.st_ctime = rootfcb->inode_item.st_mtime = rootfcb->inode_item.otime = now;
    rootfcb->inode_item.st_gid = GID_NOBODY;

    rootfcb->atts = get_file_attributes(Vcb, rootfcb->subvol, rootfcb->inode, rootfcb->type, false, true, Irp);

    if (r->root_item.flags & BTRFS_SUBVOL_READONLY)
        rootfcb->atts |= FILE_ATTRIBUTE_READONLY;

    SeCaptureSubjectContext(&subjcont);

    Status = SeAssignSecurity(fcb->sd, NULL, (void**)&rootfcb->sd, true, &subjcont, IoGetFileObjectGenericMapping(), PagedPool);

    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurity returned %08lx\n", Status);
        goto end;
    }

    if (!rootfcb->sd) {
        ERR("SeAssignSecurity returned NULL security descriptor\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    Status = RtlGetOwnerSecurityDescriptor(rootfcb->sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetOwnerSecurityDescriptor returned %08lx\n", Status);
        rootfcb->inode_item.st_uid = UID_NOBODY;
        rootfcb->sd_dirty = true;
    } else {
        rootfcb->inode_item.st_uid = sid_to_uid(owner);
        rootfcb->sd_dirty = rootfcb->inode_item.st_uid == UID_NOBODY;
    }

    find_gid(rootfcb, fileref->fcb, &subjcont);

    rootfcb->inode_item_changed = true;

    acquire_fcb_lock_exclusive(Vcb);
    add_fcb_to_subvol(rootfcb);
    InsertTailList(&Vcb->all_fcbs, &rootfcb->list_entry_all);
    r->fcbs_version++;
    release_fcb_lock(Vcb);

    rootfcb->Header.IsFastIoPossible = fast_io_possible(rootfcb);
    rootfcb->Header.AllocationSize.QuadPart = 0;
    rootfcb->Header.FileSize.QuadPart = 0;
    rootfcb->Header.ValidDataLength.QuadPart = 0;

    rootfcb->created = true;

    if (fileref->fcb->inode_item.flags & BTRFS_INODE_COMPRESS)
        rootfcb->inode_item.flags |= BTRFS_INODE_COMPRESS;

    rootfcb->prop_compression = fileref->fcb->prop_compression;
    rootfcb->prop_compression_changed = rootfcb->prop_compression != PropCompression_None;

    r->lastinode = rootfcb->inode;

    // add INODE_REF

    irsize = (uint16_t)(offsetof(INODE_REF, name[0]) + sizeof(DOTDOT) - 1);
    ir = ExAllocatePoolWithTag(PagedPool, irsize, ALLOC_TAG);
    if (!ir) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    ir->index = 0;
    ir->n = sizeof(DOTDOT) - 1;
    RtlCopyMemory(ir->name, DOTDOT, ir->n);

    Status = insert_tree_item(Vcb, r, r->root_item.objid, TYPE_INODE_REF, r->root_item.objid, ir, irsize, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(ir);
        goto end;
    }

    // create fileref for entry in other subvolume

    fr = create_fileref(Vcb);
    if (!fr) {
        ERR("out of memory\n");

        reap_fcb(rootfcb);

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fr->fcb = rootfcb;

    mark_fcb_dirty(rootfcb);

    fr->parent = fileref;

    Status = add_dir_child(fileref->fcb, r->id, true, &utf8, &nameus, BTRFS_TYPE_DIRECTORY, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08lx\n", Status);

    fr->dc = dc;
    dc->fileref = fr;

    fr->fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fr->fcb->hash_ptrs) {
        ERR("out of memory\n");
        free_fileref(fr);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(fr->fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);

    fr->fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fr->fcb->hash_ptrs_uc) {
        ERR("out of memory\n");
        free_fileref(fr);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(fr->fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);

    ExAcquireResourceExclusiveLite(&fileref->fcb->nonpaged->dir_children_lock, true);
    InsertTailList(&fileref->children, &fr->list_entry);
    ExReleaseResourceLite(&fileref->fcb->nonpaged->dir_children_lock);

    increase_fileref_refcount(fileref);

    if (fr->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr->fcb->fileref = fr;

    fr->created = true;
    mark_fileref_dirty(fr);

    // change fcb->subvol's ROOT_ITEM

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    // change fcb's INODE_ITEM

    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size += utf8.Length * 2;
    fcb->inode_item.sequence++;

    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    if (!ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    fr->fcb->subvol->parent = fcb->subvol->id;

    Status = STATUS_SUCCESS;

end:
    if (!NT_SUCCESS(Status)) {
        if (fr) {
            fr->deleted = true;
            mark_fileref_dirty(fr);
        } else if (rootfcb) {
            rootfcb->deleted = true;
            mark_fcb_dirty(rootfcb);
        }

        if (r) {
            RemoveEntryList(&r->list_entry);
            InsertTailList(&Vcb->drop_roots, &r->list_entry);
        }
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (NT_SUCCESS(Status)) {
        send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED, NULL);
        send_notification_fileref(fr->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
    }

end2:
    if (fr)
        free_fileref(fr);

    return Status;
}

static NTSTATUS get_inode_info(PFILE_OBJECT FileObject, void* data, ULONG length) {
    btrfs_inode_info* bii = data;
    fcb* fcb;
    ccb* ccb;
    bool old_style;

    if (length < offsetof(btrfs_inode_info, disk_size_zstd))
        return STATUS_BUFFER_OVERFLOW;

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    ccb = FileObject->FsContext2;

    if (!ccb)
        return STATUS_INVALID_PARAMETER;

    if (!(ccb->access & FILE_READ_ATTRIBUTES)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (fcb->ads)
        fcb = ccb->fileref->parent->fcb;

    old_style = length < offsetof(btrfs_inode_info, sparse_size) + sizeof(((btrfs_inode_info*)NULL)->sparse_size);

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    bii->subvol = fcb->subvol->id;
    bii->inode = fcb->inode;
    bii->top = fcb->Vcb->root_fileref->fcb == fcb ? true : false;
    bii->type = fcb->type;
    bii->st_uid = fcb->inode_item.st_uid;
    bii->st_gid = fcb->inode_item.st_gid;
    bii->st_mode = fcb->inode_item.st_mode;

    if (fcb->inode_item.st_rdev == 0)
        bii->st_rdev = 0;
    else
        bii->st_rdev = makedev((fcb->inode_item.st_rdev & 0xFFFFFFFFFFF) >> 20, fcb->inode_item.st_rdev & 0xFFFFF);

    bii->flags = fcb->inode_item.flags;

    bii->inline_length = 0;
    bii->disk_size_uncompressed = 0;
    bii->disk_size_zlib = 0;
    bii->disk_size_lzo = 0;

    if (!old_style) {
        bii->disk_size_zstd = 0;
        bii->sparse_size = 0;
    }

    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        uint64_t last_end = 0;
        LIST_ENTRY* le;
        bool extents_inline = false;

        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (!ext->ignore) {
                if (!old_style && ext->offset > last_end)
                    bii->sparse_size += ext->offset - last_end;

                if (ext->extent_data.type == EXTENT_TYPE_INLINE) {
                    bii->inline_length += ext->datalen - (uint16_t)offsetof(EXTENT_DATA, data[0]);
                    last_end = ext->offset + ext->extent_data.decoded_size;
                    extents_inline = true;
                } else {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                    // FIXME - compressed extents with a hole in them are counted more than once
                    if (ed2->size != 0) {
                        switch (ext->extent_data.compression) {
                            case BTRFS_COMPRESSION_NONE:
                                bii->disk_size_uncompressed += ed2->num_bytes;
                                break;

                            case BTRFS_COMPRESSION_ZLIB:
                                bii->disk_size_zlib += ed2->size;
                                break;

                            case BTRFS_COMPRESSION_LZO:
                                bii->disk_size_lzo += ed2->size;
                                break;

                            case BTRFS_COMPRESSION_ZSTD:
                                if (!old_style)
                                    bii->disk_size_zstd += ed2->size;
                                break;
                        }
                    }

                    last_end = ext->offset + ed2->num_bytes;
                }
            }

            le = le->Flink;
        }

        if (!extents_inline && !old_style && sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) > last_end)
            bii->sparse_size += sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) - last_end;

        if (length >= offsetof(btrfs_inode_info, num_extents) + sizeof(((btrfs_inode_info*)NULL)->num_extents)) {
            EXTENT_DATA2* last_ed2 = NULL;

            le = fcb->extents.Flink;

            bii->num_extents = 0;

            while (le != &fcb->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);

                if (!ext->ignore && ext->extent_data.type != EXTENT_TYPE_INLINE) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                    if (ed2->size != 0) {
                        if (!last_ed2 || ed2->offset != last_ed2->offset + last_ed2->num_bytes)
                            bii->num_extents++;

                        last_ed2 = ed2;
                    } else
                        last_ed2 = NULL;
                }

                le = le->Flink;
            }
        }
    }

    switch (fcb->prop_compression) {
        case PropCompression_Zlib:
            bii->compression_type = BTRFS_COMPRESSION_ZLIB;
            break;

        case PropCompression_LZO:
            bii->compression_type = BTRFS_COMPRESSION_LZO;
            break;

        case PropCompression_ZSTD:
            bii->compression_type = BTRFS_COMPRESSION_ZSTD;
            break;

        default:
            bii->compression_type = BTRFS_COMPRESSION_ANY;
            break;
    }

    ExReleaseResourceLite(fcb->Header.Resource);

    return STATUS_SUCCESS;
}

static NTSTATUS set_inode_info(PFILE_OBJECT FileObject, void* data, ULONG length, PIRP Irp) {
    btrfs_set_inode_info* bsii = data;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;

    if (length < sizeof(btrfs_set_inode_info))
        return STATUS_INVALID_PARAMETER;

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    ccb = FileObject->FsContext2;

    if (!ccb)
        return STATUS_INVALID_PARAMETER;

    if (bsii->flags_changed && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if ((bsii->mode_changed || bsii->uid_changed || bsii->gid_changed) && !(ccb->access & WRITE_DAC)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (bsii->compression_type_changed && bsii->compression_type > BTRFS_COMPRESSION_ZSTD)
        return STATUS_INVALID_PARAMETER;

    if (fcb->ads)
        fcb = ccb->fileref->parent->fcb;

    if (is_subvol_readonly(fcb->subvol, Irp)) {
        WARN("trying to change inode on readonly subvolume\n");
        return STATUS_ACCESS_DENIED;
    }

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (bsii->flags_changed) {
        if (fcb->type != BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0 &&
            (bsii->flags & BTRFS_INODE_NODATACOW) != (fcb->inode_item.flags & BTRFS_INODE_NODATACOW)) {
            WARN("trying to change nocow flag on non-empty file\n");
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        fcb->inode_item.flags = bsii->flags;

        if (fcb->inode_item.flags & BTRFS_INODE_NODATACOW)
            fcb->inode_item.flags |= BTRFS_INODE_NODATASUM;
        else
            fcb->inode_item.flags &= ~(uint64_t)BTRFS_INODE_NODATASUM;
    }

    if (bsii->mode_changed) {
        uint32_t allowed = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH |
                         S_ISGID | S_ISVTX;

        if (ccb->access & WRITE_OWNER)
            allowed |= S_ISUID;

        fcb->inode_item.st_mode &= ~allowed;
        fcb->inode_item.st_mode |= bsii->st_mode & allowed;
    }

    if (bsii->uid_changed && fcb->inode_item.st_uid != bsii->st_uid) {
        fcb->inode_item.st_uid = bsii->st_uid;

        fcb->sd_dirty = true;
        fcb->sd_deleted = false;
    }

    if (bsii->gid_changed)
        fcb->inode_item.st_gid = bsii->st_gid;

    if (bsii->compression_type_changed) {
        switch (bsii->compression_type) {
            case BTRFS_COMPRESSION_ANY:
                fcb->prop_compression = PropCompression_None;
            break;

            case BTRFS_COMPRESSION_ZLIB:
                fcb->prop_compression = PropCompression_Zlib;
            break;

            case BTRFS_COMPRESSION_LZO:
                fcb->prop_compression = PropCompression_LZO;
            break;

            case BTRFS_COMPRESSION_ZSTD:
                fcb->prop_compression = PropCompression_ZSTD;
            break;
        }

        fcb->prop_compression_changed = true;
    }

    if (bsii->flags_changed || bsii->mode_changed || bsii->uid_changed || bsii->gid_changed || bsii->compression_type_changed) {
        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

static NTSTATUS get_devices(device_extension* Vcb, void* data, ULONG length) {
    btrfs_device* dev = NULL;
    NTSTATUS Status;
    LIST_ENTRY* le;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);
        ULONG structlen;

        if (length < sizeof(btrfs_device) - sizeof(WCHAR)) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto end;
        }

        if (!dev)
            dev = data;
        else {
            dev->next_entry = sizeof(btrfs_device) - sizeof(WCHAR) + dev->namelen;
            dev = (btrfs_device*)((uint8_t*)dev + dev->next_entry);
        }

        structlen = length - offsetof(btrfs_device, namelen);

        if (dev2->devobj) {
            Status = dev_ioctl(dev2->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &dev->namelen, structlen, true, NULL);
            if (!NT_SUCCESS(Status))
                goto end;

            dev->missing = false;
        } else {
            dev->namelen = 0;
            dev->missing = true;
        }

        dev->next_entry = 0;
        dev->dev_id = dev2->devitem.dev_id;
        dev->readonly = (Vcb->readonly || dev2->readonly) ? true : false;
        dev->device_number = dev2->disk_num;
        dev->partition_number = dev2->part_num;
        dev->size = dev2->devitem.num_bytes;

        if (dev2->devobj) {
            GET_LENGTH_INFORMATION gli;

            Status = dev_ioctl(dev2->devobj, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli), true, NULL);
            if (!NT_SUCCESS(Status))
                goto end;

            dev->max_size = gli.Length.QuadPart;
        } else
            dev->max_size = dev->size;

        RtlCopyMemory(dev->stats, dev2->stats, sizeof(uint64_t) * 5);

        length -= sizeof(btrfs_device) - sizeof(WCHAR) + dev->namelen;

        le = le->Flink;
    }

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS get_usage(device_extension* Vcb, void* data, ULONG length, PIRP Irp) {
    btrfs_usage* usage = (btrfs_usage*)data;
    btrfs_usage* lastbue = NULL;
    NTSTATUS Status;
    LIST_ENTRY* le;

    if (length < sizeof(btrfs_usage))
        return STATUS_BUFFER_OVERFLOW;

    if (!Vcb->chunk_usage_found) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

        if (!Vcb->chunk_usage_found)
            Status = find_chunk_usage(Vcb, Irp);
        else
            Status = STATUS_SUCCESS;

        ExReleaseResourceLite(&Vcb->tree_lock);

        if (!NT_SUCCESS(Status)) {
            ERR("find_chunk_usage returned %08lx\n", Status);
            return Status;
        }
    }

    length -= offsetof(btrfs_usage, devices);

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, true);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        bool addnew = false;

        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!lastbue) // first entry
            addnew = true;
        else {
            btrfs_usage* bue = usage;

            addnew = true;

            while (true) {
                if (bue->type == c->chunk_item->type) {
                    addnew = false;
                    break;
                }

                if (bue->next_entry == 0)
                    break;
                else
                    bue = (btrfs_usage*)((uint8_t*)bue + bue->next_entry);
            }
        }

        if (addnew) {
            btrfs_usage* bue;
            LIST_ENTRY* le2;
            uint64_t factor;

            if (!lastbue) {
                bue = usage;
            } else {
                if (length < offsetof(btrfs_usage, devices)) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto end;
                }

                length -= offsetof(btrfs_usage, devices);

                lastbue->next_entry = offsetof(btrfs_usage, devices) + (ULONG)(lastbue->num_devices * sizeof(btrfs_usage_device));

                bue = (btrfs_usage*)((uint8_t*)lastbue + lastbue->next_entry);
            }

            bue->next_entry = 0;
            bue->type = c->chunk_item->type;
            bue->size = 0;
            bue->used = 0;
            bue->num_devices = 0;

            if (c->chunk_item->type & BLOCK_FLAG_RAID0)
                factor = c->chunk_item->num_stripes;
            else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
                factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
            else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
                factor = c->chunk_item->num_stripes - 1;
            else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
                factor = c->chunk_item->num_stripes - 2;
            else
                factor = 1;

            le2 = le;
            while (le2 != &Vcb->chunks) {
                chunk* c2 = CONTAINING_RECORD(le2, chunk, list_entry);

                if (c2->chunk_item->type == c->chunk_item->type) {
                    uint16_t i;
                    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c2->chunk_item[1];
                    uint64_t stripesize;

                    bue->size += c2->chunk_item->size;
                    bue->used += c2->used;

                    stripesize = c2->chunk_item->size / factor;

                    for (i = 0; i < c2->chunk_item->num_stripes; i++) {
                        uint64_t j;
                        bool found = false;

                        for (j = 0; j < bue->num_devices; j++) {
                            if (bue->devices[j].dev_id == cis[i].dev_id) {
                                bue->devices[j].alloc += stripesize;
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            if (length < sizeof(btrfs_usage_device)) {
                                Status = STATUS_BUFFER_OVERFLOW;
                                goto end;
                            }

                            length -= sizeof(btrfs_usage_device);

                            bue->devices[bue->num_devices].dev_id = cis[i].dev_id;
                            bue->devices[bue->num_devices].alloc = stripesize;
                            bue->num_devices++;
                        }
                    }
                }

                le2 = le2->Flink;
            }

            lastbue = bue;
        }

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&Vcb->chunk_lock);

    return Status;
}

static NTSTATUS is_volume_mounted(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    ULONG cc;
    IO_STATUS_BLOCK iosb;
    bool verify = false;
    LIST_ENTRY* le;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj && dev->removable) {
            Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), false, &iosb);

            if (iosb.Information != sizeof(ULONG))
                cc = 0;

            if (Status == STATUS_VERIFY_REQUIRED || (NT_SUCCESS(Status) && cc != dev->change_count)) {
                dev->devobj->Flags |= DO_VERIFY_VOLUME;
                verify = true;
            }

            if (NT_SUCCESS(Status) && iosb.Information == sizeof(ULONG))
                dev->change_count = cc;

            if (!NT_SUCCESS(Status) || verify) {
                IoSetHardErrorOrVerifyDevice(Irp, dev->devobj);
                ExReleaseResourceLite(&Vcb->tree_lock);

                return verify ? STATUS_VERIFY_REQUIRED : Status;
            }
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    return STATUS_SUCCESS;
}

static NTSTATUS fs_get_statistics(void* buffer, DWORD buflen, ULONG_PTR* retlen) {
    FILESYSTEM_STATISTICS* fss;

    WARN("STUB: FSCTL_FILESYSTEM_GET_STATISTICS\n");

    // This is hideously wrong, but at least it stops SMB from breaking

    if (buflen < sizeof(FILESYSTEM_STATISTICS))
        return STATUS_BUFFER_TOO_SMALL;

    fss = buffer;
    RtlZeroMemory(fss, sizeof(FILESYSTEM_STATISTICS));

    fss->Version = 1;
    fss->FileSystemType = FILESYSTEM_STATISTICS_TYPE_NTFS;
    fss->SizeOfCompleteStructure = sizeof(FILESYSTEM_STATISTICS);

    *retlen = sizeof(FILESYSTEM_STATISTICS);

    return STATUS_SUCCESS;
}

static NTSTATUS set_sparse(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG length, PIRP Irp) {
    FILE_SET_SPARSE_BUFFER* fssb = data;
    NTSTATUS Status;
    bool set;
    fcb* fcb;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;

    if (data && length < sizeof(FILE_SET_SPARSE_BUFFER))
        return STATUS_INVALID_PARAMETER;

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    fcb = FileObject->FsContext;

    if (!fcb) {
        ERR("FCB was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!ccb) {
        ERR("CCB was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (!fileref) {
        ERR("no fileref\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fcb->ads) {
        fileref = fileref->parent;
        fcb = fileref->fcb;
    }

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("FileObject did not point to a file\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (fssb)
        set = fssb->SetSparse;
    else
        set = true;

    if (set) {
        fcb->atts |= FILE_ATTRIBUTE_SPARSE_FILE;
        fcb->atts_changed = true;
    } else {
        ULONG defda;

        fcb->atts &= ~FILE_ATTRIBUTE_SPARSE_FILE;
        fcb->atts_changed = true;

        defda = get_file_attributes(Vcb, fcb->subvol, fcb->inode, fcb->type,
                                    fileref && fileref->dc && fileref->dc->name.Length >= sizeof(WCHAR) && fileref->dc->name.Buffer[0] == '.', true, Irp);

        fcb->atts_deleted = defda == fcb->atts;
    }

    mark_fcb_dirty(fcb);
    queue_notification_fcb(fileref, FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS zero_data(device_extension* Vcb, fcb* fcb, uint64_t start, uint64_t length, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    bool make_inline, compress;
    uint64_t start_data, end_data;
    ULONG buf_head;
    uint8_t* data;

    make_inline = fcb->inode_item.st_size <= Vcb->options.max_inline || fcb_is_inline(fcb);

    if (!make_inline)
        compress = write_fcb_compressed(fcb);

    if (make_inline) {
        start_data = 0;
        end_data = fcb->inode_item.st_size;
        buf_head = (ULONG)offsetof(EXTENT_DATA, data[0]);
    } else if (compress) {
        start_data = start & ~(uint64_t)(COMPRESSED_EXTENT_SIZE - 1);
        end_data = min(sector_align(start + length, COMPRESSED_EXTENT_SIZE),
                       sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size));
        buf_head = 0;
    } else {
        start_data = start & ~(uint64_t)(Vcb->superblock.sector_size - 1);
        end_data = sector_align(start + length, Vcb->superblock.sector_size);
        buf_head = 0;
    }

    data = ExAllocatePoolWithTag(PagedPool, (ULONG)(buf_head + end_data - start_data), ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(data + buf_head, (ULONG)(end_data - start_data));

    if (start > start_data || start + length < end_data) {
        Status = read_file(fcb, data + buf_head, start_data, end_data - start_data, NULL, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08lx\n", Status);
            ExFreePool(data);
            return Status;
        }
    }

    RtlZeroMemory(data + buf_head + start - start_data, (ULONG)length);

    if (make_inline) {
        uint16_t edsize;
        EXTENT_DATA* ed = (EXTENT_DATA*)data;

        Status = excise_extents(Vcb, fcb, 0, sector_align(end_data, Vcb->superblock.sector_size), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            ExFreePool(data);
            return Status;
        }

        edsize = (uint16_t)(offsetof(EXTENT_DATA, data[0]) + end_data);

        ed->generation = Vcb->superblock.generation;
        ed->decoded_size = end_data;
        ed->compression = BTRFS_COMPRESSION_NONE;
        ed->encryption = BTRFS_ENCRYPTION_NONE;
        ed->encoding = BTRFS_ENCODING_NONE;
        ed->type = EXTENT_TYPE_INLINE;

        Status = add_extent_to_fcb(fcb, 0, ed, edsize, false, NULL, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_extent_to_fcb returned %08lx\n", Status);
            ExFreePool(data);
            return Status;
        }

        ExFreePool(data);

        fcb->inode_item.st_blocks += end_data;
    } else if (compress) {
        Status = write_compressed(fcb, start_data, end_data, data, Irp, rollback);

        ExFreePool(data);

        if (!NT_SUCCESS(Status)) {
            ERR("write_compressed returned %08lx\n", Status);
            return Status;
        }
    } else {
        Status = do_write_file(fcb, start_data, end_data, data, Irp, false, 0, rollback);

        ExFreePool(data);

        if (!NT_SUCCESS(Status)) {
            ERR("do_write_file returned %08lx\n", Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS set_zero_data(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG length, PIRP Irp) {
    FILE_ZERO_DATA_INFORMATION* fzdi = data;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    LIST_ENTRY rollback, *le;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    uint64_t start, end;
    extent* ext;
    IO_STATUS_BLOCK iosb;

    if (!data || length < sizeof(FILE_ZERO_DATA_INFORMATION))
        return STATUS_INVALID_PARAMETER;

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fzdi->BeyondFinalZero.QuadPart <= fzdi->FileOffset.QuadPart) {
        WARN("BeyondFinalZero was less than or equal to FileOffset (%I64x <= %I64x)\n", fzdi->BeyondFinalZero.QuadPart, fzdi->FileOffset.QuadPart);
        return STATUS_INVALID_PARAMETER;
    }

    fcb = FileObject->FsContext;

    if (!fcb) {
        ERR("FCB was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    ccb = FileObject->FsContext2;

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_DATA)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    fileref = ccb->fileref;

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    InitializeListHead(&rollback);

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &iosb);

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("FileObject did not point to a file\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (fcb->ads) {
        ERR("FileObject is stream\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if ((uint64_t)fzdi->FileOffset.QuadPart >= fcb->inode_item.st_size) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    ext = NULL;
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext2 = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext2->ignore) {
            ext = ext2;
            break;
        }

        le = le->Flink;
    }

    if (!ext) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    if (ext->extent_data.type == EXTENT_TYPE_INLINE) {
        Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("zero_data returned %08lx\n", Status);
            goto end;
        }
    } else {
        start = sector_align(fzdi->FileOffset.QuadPart, Vcb->superblock.sector_size);

        if ((uint64_t)fzdi->BeyondFinalZero.QuadPart > fcb->inode_item.st_size)
            end = sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size);
        else
            end = (fzdi->BeyondFinalZero.QuadPart >> Vcb->sector_shift) << Vcb->sector_shift;

        if (end <= start) {
            Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, Irp, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("zero_data returned %08lx\n", Status);
                goto end;
            }
        } else {
            if (start > (uint64_t)fzdi->FileOffset.QuadPart) {
                Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, start - fzdi->FileOffset.QuadPart, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("zero_data returned %08lx\n", Status);
                    goto end;
                }
            }

            if (end < (uint64_t)fzdi->BeyondFinalZero.QuadPart) {
                Status = zero_data(Vcb, fcb, end, fzdi->BeyondFinalZero.QuadPart - end, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("zero_data returned %08lx\n", Status);
                    goto end;
                }
            }

            if (end > start) {
                Status = excise_extents(Vcb, fcb, start, end, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("excise_extents returned %08lx\n", Status);
                    goto end;
                }
            }
        }
    }

    CcPurgeCacheSection(FileObject->SectionObjectPointer, &fzdi->FileOffset, (ULONG)(fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart), false);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;

    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    if (!ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;

    fcb->extents_changed = true;
    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    queue_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    Status = STATUS_SUCCESS;

end:
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(&rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS query_ranges(PFILE_OBJECT FileObject, FILE_ALLOCATED_RANGE_BUFFER* inbuf, ULONG inbuflen, void* outbuf, ULONG outbuflen, ULONG_PTR* retlen) {
    NTSTATUS Status;
    fcb* fcb;
    LIST_ENTRY* le;
    FILE_ALLOCATED_RANGE_BUFFER* ranges = outbuf;
    ULONG i = 0;
    uint64_t last_start, last_end;

    TRACE("FSCTL_QUERY_ALLOCATED_RANGES\n");

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!inbuf || inbuflen < sizeof(FILE_ALLOCATED_RANGE_BUFFER) || !outbuf)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb) {
        ERR("FCB was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    // If file is not marked as sparse, claim the whole thing as an allocated range

    if (!(fcb->atts & FILE_ATTRIBUTE_SPARSE_FILE)) {
        if (fcb->inode_item.st_size == 0)
            Status = STATUS_SUCCESS;
        else if (outbuflen < sizeof(FILE_ALLOCATED_RANGE_BUFFER))
            Status = STATUS_BUFFER_TOO_SMALL;
        else {
            ranges[i].FileOffset.QuadPart = 0;
            ranges[i].Length.QuadPart = fcb->inode_item.st_size;
            i++;
            Status = STATUS_SUCCESS;
        }

        goto end;

    }

    le = fcb->extents.Flink;

    last_start = 0;
    last_end = 0;

    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext->ignore) {
            EXTENT_DATA2* ed2 = (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) ? (EXTENT_DATA2*)ext->extent_data.data : NULL;
            uint64_t len = ed2 ? ed2->num_bytes : ext->extent_data.decoded_size;

            if (ext->offset > last_end) { // first extent after a hole
                if (last_end > last_start) {
                    if ((i + 1) * sizeof(FILE_ALLOCATED_RANGE_BUFFER) <= outbuflen) {
                        ranges[i].FileOffset.QuadPart = last_start;
                        ranges[i].Length.QuadPart = min(fcb->inode_item.st_size, last_end) - last_start;
                        i++;
                    } else {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        goto end;
                    }
                }

                last_start = ext->offset;
            }

            last_end = ext->offset + len;
        }

        le = le->Flink;
    }

    if (last_end > last_start) {
        if ((i + 1) * sizeof(FILE_ALLOCATED_RANGE_BUFFER) <= outbuflen) {
            ranges[i].FileOffset.QuadPart = last_start;
            ranges[i].Length.QuadPart = min(fcb->inode_item.st_size, last_end) - last_start;
            i++;
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }
    }

    Status = STATUS_SUCCESS;

end:
    *retlen = i * sizeof(FILE_ALLOCATED_RANGE_BUFFER);

    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

static NTSTATUS get_object_id(PFILE_OBJECT FileObject, FILE_OBJECTID_BUFFER* buf, ULONG buflen, ULONG_PTR* retlen) {
    fcb* fcb;

    TRACE("(%p, %p, %lx, %p)\n", FileObject, buf, buflen, retlen);

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!buf || buflen < sizeof(FILE_OBJECTID_BUFFER))
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb) {
        ERR("FCB was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    RtlCopyMemory(&buf->ObjectId[0], &fcb->inode, sizeof(uint64_t));
    RtlCopyMemory(&buf->ObjectId[sizeof(uint64_t)], &fcb->subvol->id, sizeof(uint64_t));

    ExReleaseResourceLite(fcb->Header.Resource);

    RtlZeroMemory(&buf->ExtendedInfo, sizeof(buf->ExtendedInfo));

    *retlen = sizeof(FILE_OBJECTID_BUFFER);

    return STATUS_SUCCESS;
}

static void flush_fcb_caches(device_extension* Vcb) {
    LIST_ENTRY* le;

    le = Vcb->all_fcbs.Flink;
    while (le != &Vcb->all_fcbs) {
        struct _fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_all);
        IO_STATUS_BLOCK iosb;

        if (fcb->type != BTRFS_TYPE_DIRECTORY && !fcb->deleted)
            CcFlushCache(&fcb->nonpaged->segment_object, NULL, 0, &iosb);

        le = le->Flink;
    }
}

static NTSTATUS lock_volume(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    KIRQL irql;
    bool lock_paused_balance = false;

    TRACE("FSCTL_LOCK_VOLUME\n");

    if (Vcb->scrub.thread) {
        WARN("cannot lock while scrub running\n");
        return STATUS_DEVICE_NOT_READY;
    }

    if (Vcb->balance.thread) {
        WARN("cannot lock while balance running\n");
        return STATUS_DEVICE_NOT_READY;
    }

    TRACE("locking volume\n");

    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK);

    if (Vcb->locked)
        return STATUS_SUCCESS;

    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, true);

    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        ExReleaseResourceLite(&Vcb->fileref_lock);
        goto end;
    }

    ExReleaseResourceLite(&Vcb->fileref_lock);

    if (Vcb->balance.thread && KeReadStateEvent(&Vcb->balance.event)) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);
        KeClearEvent(&Vcb->balance.event);
        ExReleaseResourceLite(&Vcb->tree_lock);

        lock_paused_balance = true;
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    flush_fcb_caches(Vcb);

    if (Vcb->need_write && !Vcb->readonly)
        Status = do_write(Vcb, Irp);
    else
        Status = STATUS_SUCCESS;

    free_trees(Vcb);

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (!NT_SUCCESS(Status)) {
        ERR("do_write returned %08lx\n", Status);
        goto end;
    }

    IoAcquireVpbSpinLock(&irql);

    if (!(Vcb->Vpb->Flags & VPB_LOCKED)) {
        Vcb->Vpb->Flags |= VPB_LOCKED;
        Vcb->locked = true;
        Vcb->locked_fileobj = IrpSp->FileObject;
        Vcb->lock_paused_balance = lock_paused_balance;
    } else {
        Status = STATUS_ACCESS_DENIED;
        IoReleaseVpbSpinLock(irql);

        if (lock_paused_balance)
            KeSetEvent(&Vcb->balance.event, 0, false);

        goto end;
    }

    IoReleaseVpbSpinLock(irql);

    Status = STATUS_SUCCESS;

end:
    if (!NT_SUCCESS(Status))
        FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK_FAILED);

    return Status;
}

void do_unlock_volume(device_extension* Vcb) {
    KIRQL irql;

    IoAcquireVpbSpinLock(&irql);

    Vcb->locked = false;
    Vcb->Vpb->Flags &= ~VPB_LOCKED;
    Vcb->locked_fileobj = NULL;

    IoReleaseVpbSpinLock(irql);

    if (Vcb->lock_paused_balance)
        KeSetEvent(&Vcb->balance.event, 0, false);
}

static NTSTATUS unlock_volume(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    TRACE("FSCTL_UNLOCK_VOLUME\n");

    if (!Vcb->locked || IrpSp->FileObject != Vcb->locked_fileobj)
        return STATUS_NOT_LOCKED;

    TRACE("unlocking volume\n");

    do_unlock_volume(Vcb);

    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_UNLOCK);

    return STATUS_SUCCESS;
}

static NTSTATUS invalidate_volumes(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    LUID TcbPrivilege = {SE_TCB_PRIVILEGE, 0};
    NTSTATUS Status;
    HANDLE h;
    PFILE_OBJECT fileobj;
    PDEVICE_OBJECT devobj;
    LIST_ENTRY* le;

    TRACE("FSCTL_INVALIDATE_VOLUMES\n");

    if (!SeSinglePrivilegeCheck(TcbPrivilege, Irp->RequestorMode))
        return STATUS_PRIVILEGE_NOT_HELD;

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(uint32_t))
            return STATUS_INVALID_PARAMETER;

        h = (HANDLE)LongToHandle((*(uint32_t*)Irp->AssociatedIrp.SystemBuffer));
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(HANDLE))
            return STATUS_INVALID_PARAMETER;

        h = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
#if defined(_WIN64)
    }
#endif

    Status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&fileobj, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08lx\n", Status);
        return Status;
    }

    devobj = fileobj->DeviceObject;

    ExAcquireResourceSharedLite(&global_loading_lock, true);

    le = VcbList.Flink;

    while (le != &VcbList) {
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);

        if (Vcb->Vpb && Vcb->Vpb->RealDevice == devobj) {
            if (Vcb->Vpb == devobj->Vpb) {
                KIRQL irql;
                PVPB newvpb;
                bool free_newvpb = false;

                newvpb = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), ALLOC_TAG);
                if (!newvpb) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                RtlZeroMemory(newvpb, sizeof(VPB));

                ObReferenceObject(devobj);

                ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

                Vcb->removing = true;

                ExReleaseResourceLite(&Vcb->tree_lock);

                CcWaitForCurrentLazyWriterActivity();

                ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

                flush_fcb_caches(Vcb);

                if (Vcb->need_write && !Vcb->readonly)
                    Status = do_write(Vcb, Irp);
                else
                    Status = STATUS_SUCCESS;

                free_trees(Vcb);

                if (!NT_SUCCESS(Status)) {
                    ERR("do_write returned %08lx\n", Status);
                    ExReleaseResourceLite(&Vcb->tree_lock);
                    ExFreePool(newvpb);
                    ObDereferenceObject(devobj);
                    goto end;
                }

                flush_fcb_caches(Vcb);

                ExReleaseResourceLite(&Vcb->tree_lock);

                IoAcquireVpbSpinLock(&irql);

                if (devobj->Vpb->Flags & VPB_MOUNTED) {
                    newvpb->Type = IO_TYPE_VPB;
                    newvpb->Size = sizeof(VPB);
                    newvpb->RealDevice = devobj;
                    newvpb->Flags = devobj->Vpb->Flags & VPB_REMOVE_PENDING;

                    devobj->Vpb = newvpb;
                } else
                    free_newvpb = true;

                IoReleaseVpbSpinLock(irql);

                if (free_newvpb)
                    ExFreePool(newvpb);

                if (Vcb->open_files == 0)
                    uninit(Vcb);

                ObDereferenceObject(devobj);
            }

            break;
        }

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&global_loading_lock);

    ObDereferenceObject(fileobj);

    return Status;
}

static NTSTATUS is_volume_dirty(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG* volstate;

    if (Irp->AssociatedIrp.SystemBuffer) {
        volstate = Irp->AssociatedIrp.SystemBuffer;
    } else if (Irp->MdlAddress != NULL) {
        volstate = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, LowPagePriority);

        if (!volstate)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else
        return STATUS_INVALID_USER_BUFFER;

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG))
        return STATUS_INVALID_PARAMETER;

    *volstate = 0;

    if (IrpSp->FileObject->FsContext != Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    Irp->IoStatus.Information = sizeof(ULONG);

    return STATUS_SUCCESS;
}

static NTSTATUS get_compression(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    USHORT* compression;

    TRACE("FSCTL_GET_COMPRESSION\n");

    if (Irp->AssociatedIrp.SystemBuffer) {
        compression = Irp->AssociatedIrp.SystemBuffer;
    } else if (Irp->MdlAddress != NULL) {
        compression = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, LowPagePriority);

        if (!compression)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else
        return STATUS_INVALID_USER_BUFFER;

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(USHORT))
        return STATUS_INVALID_PARAMETER;

    *compression = COMPRESSION_FORMAT_NONE;

    Irp->IoStatus.Information = sizeof(USHORT);

    return STATUS_SUCCESS;
}

static NTSTATUS set_compression(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    USHORT* compression;

    TRACE("FSCTL_SET_COMPRESSION\n");

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(USHORT))
        return STATUS_INVALID_PARAMETER;

    compression = Irp->AssociatedIrp.SystemBuffer;

    if (*compression != COMPRESSION_FORMAT_NONE)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

static void update_volumes(device_extension* Vcb) {
    LIST_ENTRY* le;
    volume_device_extension* vde = Vcb->vde;
    pdo_device_extension* pdode = vde->pdode;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        vc->generation = Vcb->superblock.generation - 1;

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    ExReleaseResourceLite(&Vcb->tree_lock);
}

NTSTATUS dismount_volume(device_extension* Vcb, bool shutdown, PIRP Irp) {
    NTSTATUS Status;
    bool open_files;

    TRACE("FSCTL_DISMOUNT_VOLUME\n");

    if (!(Vcb->Vpb->Flags & VPB_MOUNTED))
        return STATUS_SUCCESS;

    if (!shutdown) {
        if (Vcb->disallow_dismount || Vcb->page_file_count != 0) {
            WARN("attempting to dismount boot volume or one containing a pagefile\n");
            return STATUS_ACCESS_DENIED;
        }

        Status = FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_DISMOUNT);
        if (!NT_SUCCESS(Status)) {
            WARN("FsRtlNotifyVolumeEvent returned %08lx\n", Status);
        }
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    if (!Vcb->locked) {
        flush_fcb_caches(Vcb);

        if (Vcb->need_write && !Vcb->readonly) {
            Status = do_write(Vcb, Irp);

            if (!NT_SUCCESS(Status))
                ERR("do_write returned %08lx\n", Status);
        }
    }

    free_trees(Vcb);

    Vcb->removing = true;

    open_files = Vcb->open_files > 0;

    if (Vcb->vde) {
        update_volumes(Vcb);
        Vcb->vde->mounted_device = NULL;
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (!open_files)
        uninit(Vcb);

    return STATUS_SUCCESS;
}

static NTSTATUS is_device_part_of_mounted_btrfs_raid(PDEVICE_OBJECT devobj, PFILE_OBJECT fileobj) {
    NTSTATUS Status;
    ULONG to_read;
    superblock* sb;
    BTRFS_UUID fsuuid, devuuid;
    LIST_ENTRY* le;

    to_read = devobj->SectorSize == 0 ? sizeof(superblock) : (ULONG)sector_align(sizeof(superblock), devobj->SectorSize);

    sb = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = sync_read_phys(devobj, fileobj, superblock_addrs[0], to_read, (uint8_t*)sb, true);
    if (!NT_SUCCESS(Status)) {
        ERR("sync_read_phys returned %08lx\n", Status);
        ExFreePool(sb);
        return Status;
    }

    if (sb->magic != BTRFS_MAGIC) {
        TRACE("device is not Btrfs\n");
        ExFreePool(sb);
        return STATUS_SUCCESS;
    }

    if (!check_superblock_checksum(sb)) {
        TRACE("device has Btrfs magic, but invalid superblock checksum\n");
        ExFreePool(sb);
        return STATUS_SUCCESS;
    }

    fsuuid = sb->uuid;
    devuuid = sb->dev_item.device_uuid;

    ExFreePool(sb);

    ExAcquireResourceSharedLite(&global_loading_lock, true);

    le = VcbList.Flink;

    while (le != &VcbList) {
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);

        if (RtlCompareMemory(&Vcb->superblock.uuid, &fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            LIST_ENTRY* le2;

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

            if (Vcb->superblock.num_devices > 1) {
                le2 = Vcb->devices.Flink;
                while (le2 != &Vcb->devices) {
                    device* dev = CONTAINING_RECORD(le2, device, list_entry);

                    if (RtlCompareMemory(&dev->devitem.device_uuid, &devuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                        ExReleaseResourceLite(&Vcb->tree_lock);
                        ExReleaseResourceLite(&global_loading_lock);
                        return STATUS_DEVICE_NOT_READY;
                    }

                    le2 = le2->Flink;
                }
            }

            ExReleaseResourceLite(&Vcb->tree_lock);
            ExReleaseResourceLite(&global_loading_lock);
            return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&global_loading_lock);

    return STATUS_SUCCESS;
}

void trim_whole_device(device* dev) {
    DEVICE_MANAGE_DATA_SET_ATTRIBUTES dmdsa;
    NTSTATUS Status;

    // FIXME - avoid "bootloader area"??

    dmdsa.Size = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES);
    dmdsa.Action = DeviceDsmAction_Trim;
    dmdsa.Flags = DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE | DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED;
    dmdsa.ParameterBlockOffset = 0;
    dmdsa.ParameterBlockLength = 0;
    dmdsa.DataSetRangesOffset = 0;
    dmdsa.DataSetRangesLength = 0;

    Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES, &dmdsa, sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), NULL, 0, true, NULL);
    if (!NT_SUCCESS(Status))
        WARN("IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES returned %08lx\n", Status);
}

static NTSTATUS add_device(device_extension* Vcb, PIRP Irp, KPROCESSOR_MODE processor_mode) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PFILE_OBJECT fileobj, mountmgrfo;
    PDEVICE_OBJECT DeviceObject;
    HANDLE h;
    LIST_ENTRY* le;
    device* dev;
    DEV_ITEM* di;
    uint64_t dev_id, size;
    uint8_t* mb;
    uint64_t* stats;
    UNICODE_STRING mmdevpath, pnp_name, pnp_name2;
    volume_child* vc;
    PDEVICE_OBJECT mountmgr;
    KEY searchkey;
    traverse_ptr tp;
    STORAGE_DEVICE_NUMBER sdn;
    volume_device_extension* vde;
    pdo_device_extension* pdode;
    const GUID* pnp_guid;
    GET_LENGTH_INFORMATION gli;

    pnp_name.Buffer = NULL;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!Vcb->vde) {
        WARN("not allowing second device to be added to non-PNP device\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (Vcb->readonly) // FIXME - handle adding R/W device to seeding device
        return STATUS_MEDIA_WRITE_PROTECTED;

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(uint32_t))
            return STATUS_INVALID_PARAMETER;

        h = (HANDLE)LongToHandle((*(uint32_t*)Irp->AssociatedIrp.SystemBuffer));
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(HANDLE))
            return STATUS_INVALID_PARAMETER;

        h = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
#if defined(_WIN64)
    }
#endif

    Status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&fileobj, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08lx\n", Status);
        return Status;
    }

    DeviceObject = fileobj->DeviceObject;

    Status = get_device_pnp_name(DeviceObject, &pnp_name, &pnp_guid);
    if (!NT_SUCCESS(Status)) {
        ERR("get_device_pnp_name returned %08lx\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }

    // If this is a disk, we have been handed the PDO, so need to go up to find something we can use
    if (RtlCompareMemory(pnp_guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID) && DeviceObject->AttachedDevice)
        DeviceObject = DeviceObject->AttachedDevice;

    Status = dev_ioctl(DeviceObject, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, true, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_DISK_IS_WRITABLE returned %08lx\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }

    Status = is_device_part_of_mounted_btrfs_raid(DeviceObject, fileobj);
    if (!NT_SUCCESS(Status)) {
        ERR("is_device_part_of_mounted_btrfs_raid returned %08lx\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }

    // if disk, check it has no partitions
    if (RtlCompareMemory(pnp_guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID)) {
        ULONG dlisize;
        DRIVE_LAYOUT_INFORMATION_EX* dli = NULL;

        dlisize = 0;

        do {
            dlisize += 1024;

            if (dli)
                ExFreePool(dli);

            dli = ExAllocatePoolWithTag(PagedPool, dlisize, ALLOC_TAG);
            if (!dli) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end2;
            }

            Status = dev_ioctl(DeviceObject, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, dli, dlisize, true, NULL);
        } while (Status == STATUS_BUFFER_TOO_SMALL);

        if (NT_SUCCESS(Status) && dli->PartitionCount > 0) {
            ExFreePool(dli);
            ERR("not adding disk which has partitions\n");
            Status = STATUS_DEVICE_NOT_READY;
            goto end2;
        }

        ExFreePool(dli);
    }

    Status = dev_ioctl(DeviceObject, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                       &sdn, sizeof(STORAGE_DEVICE_NUMBER), true, NULL);
    if (NT_SUCCESS(Status)) {
        if (sdn.DeviceType != FILE_DEVICE_DISK) { // FIXME - accept floppies and CDs?
            WARN("device was not disk\n");
            ObDereferenceObject(fileobj);
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        sdn.DeviceNumber = 0xffffffff;
        sdn.PartitionNumber = 0xffffffff;
    }

    Status = dev_ioctl(DeviceObject, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                        &gli, sizeof(gli), true, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("error reading length information: %08lx\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }

    size = gli.Length.QuadPart;

    if (size < 0x100000) {
        ERR("device was not large enough to hold FS (%I64x bytes, need at least 1 MB)\n", size);
        ObDereferenceObject(fileobj);
        return STATUS_INTERNAL_ERROR;
    }

    volume_removal(&pnp_name);

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    if (Vcb->need_write)
        Status = do_write(Vcb, Irp);
    else
        Status = STATUS_SUCCESS;

    free_trees(Vcb);

    if (!NT_SUCCESS(Status)) {
        ERR("do_write returned %08lx\n", Status);
        goto end;
    }

    dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
    if (!dev) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(dev, sizeof(device));

    dev->devobj = DeviceObject;
    dev->fileobj = fileobj;
    dev->seeding = false;
    init_device(Vcb, dev, true);

    InitializeListHead(&dev->space);

    if (size > 0x100000) { // add disk hole - the first MB is marked as used
        Status = add_space_entry(&dev->space, NULL, 0x100000, size - 0x100000);
        if (!NT_SUCCESS(Status)) {
            ERR("add_space_entry returned %08lx\n", Status);
            goto end;
        }
    }

    dev_id = 0;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);

        if (dev2->devitem.dev_id > dev_id)
            dev_id = dev2->devitem.dev_id;

        le = le->Flink;
    }

    dev_id++;

    dev->devitem.dev_id = dev_id;
    dev->devitem.num_bytes = size;
    dev->devitem.bytes_used = 0;
    dev->devitem.optimal_io_align = Vcb->superblock.sector_size;
    dev->devitem.optimal_io_width = Vcb->superblock.sector_size;
    dev->devitem.minimal_io_size = Vcb->superblock.sector_size;
    dev->devitem.type = 0;
    dev->devitem.generation = 0;
    dev->devitem.start_offset = 0;
    dev->devitem.dev_group = 0;
    dev->devitem.seek_speed = 0;
    dev->devitem.bandwidth = 0;
    get_uuid(&dev->devitem.device_uuid);
    dev->devitem.fs_uuid = Vcb->superblock.uuid;

    di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        goto end;
    }

    RtlCopyMemory(di, &dev->devitem, sizeof(DEV_ITEM));

    Status = insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, di->dev_id, di, sizeof(DEV_ITEM), NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(di);
        goto end;
    }

    // add stats entry to dev tree
    stats = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t) * 5, ALLOC_TAG);
    if (!stats) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(stats, sizeof(uint64_t) * 5);

    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = di->dev_id;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        ExFreePool(stats);
        goto end;
    }

    if (!keycmp(tp.item->key, searchkey)) {
        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08lx\n", Status);
            ExFreePool(stats);
            goto end;
        }
    }

    Status = insert_tree_item(Vcb, Vcb->dev_root, 0, TYPE_DEV_STATS, di->dev_id, stats, sizeof(uint64_t) * 5, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(stats);
        goto end;
    }

    if (dev->trim && !dev->readonly && !Vcb->options.no_trim)
        trim_whole_device(dev);

    // We clear the first megabyte of the device, so Windows doesn't identify it as another FS
    mb = ExAllocatePoolWithTag(PagedPool, 0x100000, ALLOC_TAG);
    if (!mb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(mb, 0x100000);

    Status = write_data_phys(DeviceObject, fileobj, 0, mb, 0x100000);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data_phys returned %08lx\n", Status);
        ExFreePool(mb);
        goto end;
    }

    ExFreePool(mb);

    vde = Vcb->vde;
    pdode = vde->pdode;

    vc = ExAllocatePoolWithTag(NonPagedPool, sizeof(volume_child), ALLOC_TAG);
    if (!vc) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    vc->uuid = dev->devitem.device_uuid;
    vc->devid = dev_id;
    vc->generation = Vcb->superblock.generation;
    vc->devobj = DeviceObject;
    vc->fileobj = fileobj;
    vc->notification_entry = NULL;
    vc->boot_volume = false;

    Status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange, 0, fileobj,
                                            drvobj, pnp_removal, vde->pdode, &vc->notification_entry);
    if (!NT_SUCCESS(Status))
        WARN("IoRegisterPlugPlayNotification returned %08lx\n", Status);

    pnp_name2 = pnp_name;

    if (pnp_name.Length > 4 * sizeof(WCHAR) && pnp_name.Buffer[0] == '\\' && (pnp_name.Buffer[1] == '\\' || pnp_name.Buffer[1] == '?') &&
        pnp_name.Buffer[2] == '?' && pnp_name.Buffer[3] == '\\') {
        pnp_name2.Buffer = &pnp_name2.Buffer[3];
        pnp_name2.Length -= 3 * sizeof(WCHAR);
        pnp_name2.MaximumLength -= 3 * sizeof(WCHAR);
    }

    vc->pnp_name.Length = vc->pnp_name.MaximumLength = pnp_name2.Length;

    if (pnp_name2.Length == 0)
        vc->pnp_name.Buffer = NULL;
    else {
        vc->pnp_name.Buffer = ExAllocatePoolWithTag(PagedPool, pnp_name2.Length, ALLOC_TAG);
        if (!vc->pnp_name.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(vc->pnp_name.Buffer, pnp_name2.Buffer, pnp_name2.Length);
    }

    vc->size = size;
    vc->seeding = false;
    vc->disk_num = sdn.DeviceNumber;
    vc->part_num = sdn.PartitionNumber;
    vc->had_drive_letter = false;

    ExAcquireResourceExclusiveLite(&pdode->child_lock, true);
    InsertTailList(&pdode->children, &vc->list_entry);
    pdode->num_children++;
    pdode->children_loaded++;
    ExReleaseResourceLite(&pdode->child_lock);

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
    if (!NT_SUCCESS(Status))
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
    else {
        Status = remove_drive_letter(mountmgr, &pnp_name);
        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
            WARN("remove_drive_letter returned %08lx\n", Status);

        vc->had_drive_letter = NT_SUCCESS(Status);

        ObDereferenceObject(mountmgrfo);
    }

    Vcb->superblock.num_devices++;
    Vcb->superblock.total_bytes += size;
    Vcb->devices_loaded++;
    InsertTailList(&Vcb->devices, &dev->list_entry);

    // FIXME - send notification that volume size has increased

    ObReferenceObject(DeviceObject); // for Vcb

    Status = do_write(Vcb, Irp);
    if (!NT_SUCCESS(Status))
        ERR("do_write returned %08lx\n", Status);

    ObReferenceObject(fileobj);

end:
    free_trees(Vcb);

    ExReleaseResourceLite(&Vcb->tree_lock);

end2:
    ObDereferenceObject(fileobj);

    if (pnp_name.Buffer)
        ExFreePool(pnp_name.Buffer);

    if (NT_SUCCESS(Status))
        FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_CHANGE_SIZE);

    return Status;
}

static NTSTATUS allow_extended_dasd_io(device_extension* Vcb, PFILE_OBJECT FileObject) {
    fcb* fcb;
    ccb* ccb;

    TRACE("FSCTL_ALLOW_EXTENDED_DASD_IO\n");

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    if (fcb != Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    if (!ccb)
        return STATUS_INVALID_PARAMETER;

    ccb->allow_extended_dasd_io = true;

    return STATUS_SUCCESS;
}

static NTSTATUS query_uuid(device_extension* Vcb, void* data, ULONG length) {
    if (length < sizeof(BTRFS_UUID))
        return STATUS_BUFFER_OVERFLOW;

    RtlCopyMemory(data, &Vcb->superblock.uuid, sizeof(BTRFS_UUID));

    return STATUS_SUCCESS;
}

static NTSTATUS reset_stats(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode) {
    uint64_t devid;
    NTSTATUS Status;
    LIST_ENTRY* le;

    if (length < sizeof(uint64_t))
        return STATUS_INVALID_PARAMETER;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    devid = *((uint64_t*)data);

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devitem.dev_id == devid) {
            RtlZeroMemory(dev->stats, sizeof(uint64_t) * 5);
            dev->stats_changed = true;
            Vcb->stats_changed = true;
            Vcb->need_write = true;
            Status = STATUS_SUCCESS;
            goto end;
        }

        le = le->Flink;
    }

    WARN("device %I64x not found\n", devid);
    Status = STATUS_INVALID_PARAMETER;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS get_integrity_information(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen) {
    FSCTL_GET_INTEGRITY_INFORMATION_BUFFER* fgiib = (FSCTL_GET_INTEGRITY_INFORMATION_BUFFER*)data;

    TRACE("FSCTL_GET_INTEGRITY_INFORMATION\n");

    // STUB

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    if (!data || datalen < sizeof(FSCTL_GET_INTEGRITY_INFORMATION_BUFFER))
        return STATUS_INVALID_PARAMETER;

    fgiib->ChecksumAlgorithm = 0;
    fgiib->Reserved = 0;
    fgiib->Flags = 0;
    fgiib->ChecksumChunkSizeInBytes = Vcb->superblock.sector_size;
    fgiib->ClusterSizeInBytes = Vcb->superblock.sector_size;

    return STATUS_SUCCESS;
}

static NTSTATUS set_integrity_information(PFILE_OBJECT FileObject, void* data, ULONG datalen) {
    TRACE("FSCTL_SET_INTEGRITY_INFORMATION\n");

    // STUB

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    if (!data || datalen < sizeof(FSCTL_SET_INTEGRITY_INFORMATION_BUFFER))
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

bool fcb_is_inline(fcb* fcb) {
    LIST_ENTRY* le;

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext->ignore)
            return ext->extent_data.type == EXTENT_TYPE_INLINE;

        le = le->Flink;
    }

    return false;
}

static NTSTATUS duplicate_extents(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, PIRP Irp) {
    DUPLICATE_EXTENTS_DATA* ded = (DUPLICATE_EXTENTS_DATA*)data;
    fcb *fcb = FileObject ? FileObject->FsContext : NULL, *sourcefcb;
    ccb *ccb = FileObject ? FileObject->FsContext2 : NULL, *sourceccb;
    NTSTATUS Status;
    PFILE_OBJECT sourcefo;
    uint64_t sourcelen, nbytes = 0;
    LIST_ENTRY rollback, *le, newexts;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    bool make_inline;

    if (!ded || datalen < sizeof(DUPLICATE_EXTENTS_DATA))
        return STATUS_BUFFER_TOO_SMALL;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (ded->ByteCount.QuadPart == 0)
        return STATUS_SUCCESS;

    if (!fcb || !ccb || fcb == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    if (is_subvol_readonly(fcb->subvol, Irp))
        return STATUS_ACCESS_DENIED;

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_DATA)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (!fcb->ads && fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK)
        return STATUS_INVALID_PARAMETER;

    Status = ObReferenceObjectByHandle(ded->FileHandle, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&sourcefo, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08lx\n", Status);
        return Status;
    }

    if (sourcefo->DeviceObject != FileObject->DeviceObject) {
        WARN("source and destination are on different volumes\n");
        ObDereferenceObject(sourcefo);
        return STATUS_INVALID_PARAMETER;
    }

    sourcefcb = sourcefo->FsContext;
    sourceccb = sourcefo->FsContext2;

    if (!sourcefcb || !sourceccb || sourcefcb == Vcb->volume_fcb) {
        ObDereferenceObject(sourcefo);
        return STATUS_INVALID_PARAMETER;
    }

    if (!sourcefcb->ads && !fcb->ads) {
        if ((ded->SourceFileOffset.QuadPart & (Vcb->superblock.sector_size - 1)) || (ded->TargetFileOffset.QuadPart & (Vcb->superblock.sector_size - 1))) {
            ObDereferenceObject(sourcefo);
            return STATUS_INVALID_PARAMETER;
        }

        if (ded->ByteCount.QuadPart & (Vcb->superblock.sector_size - 1)) {
            ObDereferenceObject(sourcefo);
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (Irp->RequestorMode == UserMode && (!(sourceccb->access & FILE_READ_DATA) || !(sourceccb->access & FILE_READ_ATTRIBUTES))) {
        WARN("insufficient privileges\n");
        ObDereferenceObject(sourcefo);
        return STATUS_ACCESS_DENIED;
    }

    if (!sourcefcb->ads && sourcefcb->type != BTRFS_TYPE_FILE && sourcefcb->type != BTRFS_TYPE_SYMLINK) {
        ObDereferenceObject(sourcefo);
        return STATUS_INVALID_PARAMETER;
    }

    sourcelen = sourcefcb->ads ? sourcefcb->adsdata.Length : sourcefcb->inode_item.st_size;

    if (sector_align(sourcelen, Vcb->superblock.sector_size) < (uint64_t)ded->SourceFileOffset.QuadPart + (uint64_t)ded->ByteCount.QuadPart) {
        ObDereferenceObject(sourcefo);
        return STATUS_NOT_SUPPORTED;
    }

    if (fcb == sourcefcb &&
        ((ded->SourceFileOffset.QuadPart >= ded->TargetFileOffset.QuadPart && ded->SourceFileOffset.QuadPart < ded->TargetFileOffset.QuadPart + ded->ByteCount.QuadPart) ||
        (ded->TargetFileOffset.QuadPart >= ded->SourceFileOffset.QuadPart && ded->TargetFileOffset.QuadPart < ded->SourceFileOffset.QuadPart + ded->ByteCount.QuadPart))) {
        WARN("source and destination are the same, and the ranges overlap\n");
        ObDereferenceObject(sourcefo);
        return STATUS_INVALID_PARAMETER;
    }

    // fail if nocsum flag set on one file but not the other
    if (!fcb->ads && !sourcefcb->ads && (fcb->inode_item.flags & BTRFS_INODE_NODATASUM) != (sourcefcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
        ObDereferenceObject(sourcefo);
        return STATUS_INVALID_PARAMETER;
    }

    InitializeListHead(&rollback);
    InitializeListHead(&newexts);

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fcb != sourcefcb)
        ExAcquireResourceSharedLite(sourcefcb->Header.Resource, true);

    if (!FsRtlFastCheckLockForWrite(&fcb->lock, &ded->TargetFileOffset, &ded->ByteCount, 0, FileObject, PsGetCurrentProcess())) {
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto end;
    }

    if (!FsRtlFastCheckLockForRead(&sourcefcb->lock, &ded->SourceFileOffset, &ded->ByteCount, 0, FileObject, PsGetCurrentProcess())) {
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto end;
    }

    make_inline = fcb->ads ? false : (fcb->inode_item.st_size <= Vcb->options.max_inline || fcb_is_inline(fcb));

    if (fcb->ads || sourcefcb->ads || make_inline || fcb_is_inline(sourcefcb)) {
        uint8_t* data2;
        ULONG bytes_read, dataoff, datalen2;

        if (make_inline) {
            dataoff = (ULONG)ded->TargetFileOffset.QuadPart;
            datalen2 = (ULONG)fcb->inode_item.st_size;
        } else if (fcb->ads) {
            dataoff = 0;
            datalen2 = (ULONG)ded->ByteCount.QuadPart;
        } else {
            dataoff = ded->TargetFileOffset.QuadPart & (Vcb->superblock.sector_size - 1);
            datalen2 = (ULONG)sector_align(ded->ByteCount.QuadPart + dataoff, Vcb->superblock.sector_size);
        }

        data2 = ExAllocatePoolWithTag(PagedPool, datalen2, ALLOC_TAG);
        if (!data2) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        if (dataoff > 0) {
            if (make_inline)
                Status = read_file(fcb, data2, 0, datalen2, NULL, Irp);
            else
                Status = read_file(fcb, data2, ded->TargetFileOffset.QuadPart - dataoff, dataoff, NULL, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }
        }

        if (sourcefcb->ads) {
            Status = read_stream(sourcefcb, data2 + dataoff, ded->SourceFileOffset.QuadPart, (ULONG)ded->ByteCount.QuadPart, &bytes_read);
            if (!NT_SUCCESS(Status)) {
                ERR("read_stream returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }
        } else {
            Status = read_file(sourcefcb, data2 + dataoff, ded->SourceFileOffset.QuadPart, ded->ByteCount.QuadPart, &bytes_read, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }
        }

        if (dataoff + bytes_read < datalen2)
            RtlZeroMemory(data2 + dataoff + bytes_read, datalen2 - bytes_read);

        if (fcb->ads)
            RtlCopyMemory(&fcb->adsdata.Buffer[ded->TargetFileOffset.QuadPart], data2, (USHORT)min(ded->ByteCount.QuadPart, fcb->adsdata.Length - ded->TargetFileOffset.QuadPart));
        else if (make_inline) {
            uint16_t edsize;
            EXTENT_DATA* ed;

            Status = excise_extents(Vcb, fcb, 0, sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size), Irp, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("excise_extents returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }

            edsize = (uint16_t)(offsetof(EXTENT_DATA, data[0]) + datalen2);

            ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
            if (!ed) {
                ERR("out of memory\n");
                ExFreePool(data2);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            ed->generation = Vcb->superblock.generation;
            ed->decoded_size = fcb->inode_item.st_size;
            ed->compression = BTRFS_COMPRESSION_NONE;
            ed->encryption = BTRFS_ENCRYPTION_NONE;
            ed->encoding = BTRFS_ENCODING_NONE;
            ed->type = EXTENT_TYPE_INLINE;

            RtlCopyMemory(ed->data, data2, datalen2);

            Status = add_extent_to_fcb(fcb, 0, ed, edsize, false, NULL, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_extent_to_fcb returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }

            fcb->inode_item.st_blocks += datalen2;
        } else {
            uint64_t start = ded->TargetFileOffset.QuadPart - (ded->TargetFileOffset.QuadPart & (Vcb->superblock.sector_size - 1));

            Status = do_write_file(fcb, start, start + datalen2, data2, Irp, false, 0, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("do_write_file returned %08lx\n", Status);
                ExFreePool(data2);
                goto end;
            }
        }

        ExFreePool(data2);
    } else {
        LIST_ENTRY* lastextle;

        le = sourcefcb->extents.Flink;
        while (le != &sourcefcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (!ext->ignore) {
                if (ext->offset >= (uint64_t)ded->SourceFileOffset.QuadPart + (uint64_t)ded->ByteCount.QuadPart)
                    break;

                if (ext->extent_data.type != EXTENT_TYPE_INLINE) {
                    ULONG extlen = offsetof(extent, extent_data) + sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                    extent* ext2;
                    EXTENT_DATA2 *ed2s, *ed2d;
                    chunk* c;

                    ed2s = (EXTENT_DATA2*)ext->extent_data.data;

                    if (ext->offset + ed2s->num_bytes <= (uint64_t)ded->SourceFileOffset.QuadPart) {
                        le = le->Flink;
                        continue;
                    }

                    ext2 = ExAllocatePoolWithTag(PagedPool, extlen, ALLOC_TAG);
                    if (!ext2) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }

                    if (ext->offset < (uint64_t)ded->SourceFileOffset.QuadPart)
                        ext2->offset = ded->TargetFileOffset.QuadPart;
                    else
                        ext2->offset = ext->offset - ded->SourceFileOffset.QuadPart + ded->TargetFileOffset.QuadPart;

                    ext2->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                    ext2->unique = false;
                    ext2->ignore = false;
                    ext2->inserted = true;

                    ext2->extent_data.generation = Vcb->superblock.generation;
                    ext2->extent_data.decoded_size = ext->extent_data.decoded_size;
                    ext2->extent_data.compression = ext->extent_data.compression;
                    ext2->extent_data.encryption = ext->extent_data.encryption;
                    ext2->extent_data.encoding = ext->extent_data.encoding;
                    ext2->extent_data.type = ext->extent_data.type;

                    ed2d = (EXTENT_DATA2*)ext2->extent_data.data;

                    ed2d->address = ed2s->address;
                    ed2d->size = ed2s->size;

                    if (ext->offset < (uint64_t)ded->SourceFileOffset.QuadPart) {
                        ed2d->offset = ed2s->offset + ded->SourceFileOffset.QuadPart - ext->offset;
                        ed2d->num_bytes = min((uint64_t)ded->ByteCount.QuadPart, ed2s->num_bytes + ext->offset - ded->SourceFileOffset.QuadPart);
                    } else {
                        ed2d->offset = ed2s->offset;
                        ed2d->num_bytes = min(ded->SourceFileOffset.QuadPart + ded->ByteCount.QuadPart - ext->offset, ed2s->num_bytes);
                    }

                    if (ext->csum) {
                        if (ext->extent_data.compression == BTRFS_COMPRESSION_NONE) {
                            ext2->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2d->num_bytes * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                            if (!ext2->csum) {
                                ERR("out of memory\n");
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                ExFreePool(ext2);
                                goto end;
                            }

                            RtlCopyMemory(ext2->csum, (uint8_t*)ext->csum + (((ed2d->offset - ed2s->offset) * Vcb->csum_size) >> Vcb->sector_shift),
                                          (ULONG)((ed2d->num_bytes * Vcb->csum_size) >> Vcb->sector_shift));
                        } else {
                            ext2->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2d->size * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                            if (!ext2->csum) {
                                ERR("out of memory\n");
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                ExFreePool(ext2);
                                goto end;
                            }

                            RtlCopyMemory(ext2->csum, ext->csum, (ULONG)((ed2s->size * Vcb->csum_size) >> Vcb->sector_shift));
                        }
                    } else
                        ext2->csum = NULL;

                    InsertTailList(&newexts, &ext2->list_entry);

                    c = get_chunk_from_address(Vcb, ed2s->address);
                    if (!c) {
                        ERR("get_chunk_from_address(%I64x) failed\n", ed2s->address);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }

                    Status = update_changed_extent_ref(Vcb, c, ed2s->address, ed2s->size, fcb->subvol->id, fcb->inode, ext2->offset - ed2d->offset,
                                                    1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("update_changed_extent_ref returned %08lx\n", Status);
                        goto end;
                    }

                    nbytes += ed2d->num_bytes;
                }
            }

            le = le->Flink;
        }

        Status = excise_extents(Vcb, fcb, ded->TargetFileOffset.QuadPart, ded->TargetFileOffset.QuadPart + ded->ByteCount.QuadPart, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);

            while (!IsListEmpty(&newexts)) {
                extent* ext = CONTAINING_RECORD(RemoveHeadList(&newexts), extent, list_entry);
                ExFreePool(ext);
            }

            goto end;
        }

        // clear unique flags in source fcb
        le = sourcefcb->extents.Flink;
        while (le != &sourcefcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (!ext->ignore && ext->unique && (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC)) {
                EXTENT_DATA2* ed2s = (EXTENT_DATA2*)ext->extent_data.data;
                LIST_ENTRY* le2;

                le2 = newexts.Flink;
                while (le2 != &newexts) {
                    extent* ext2 = CONTAINING_RECORD(le2, extent, list_entry);

                    if (ext2->extent_data.type == EXTENT_TYPE_REGULAR || ext2->extent_data.type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2d = (EXTENT_DATA2*)ext2->extent_data.data;

                        if (ed2d->address == ed2s->address && ed2d->size == ed2s->size) {
                            ext->unique = false;
                            break;
                        }
                    }

                    le2 = le2->Flink;
                }
            }

            le = le->Flink;
        }

        lastextle = &fcb->extents;
        while (!IsListEmpty(&newexts)) {
            extent* ext = CONTAINING_RECORD(RemoveHeadList(&newexts), extent, list_entry);

            add_extent(fcb, lastextle, ext);
            lastextle = &ext->list_entry;
        }
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    if (fcb->ads) {
        ccb->fileref->parent->fcb->inode_item.sequence++;

        if (!ccb->user_set_change_time)
            ccb->fileref->parent->fcb->inode_item.st_ctime = now;

        ccb->fileref->parent->fcb->inode_item_changed = true;
        mark_fcb_dirty(ccb->fileref->parent->fcb);
    } else {
        fcb->inode_item.st_blocks += nbytes;
        fcb->inode_item.sequence++;

        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;

        if (!ccb->user_set_write_time) {
            fcb->inode_item.st_mtime = now;
            queue_notification_fcb(ccb->fileref, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
        }

        fcb->inode_item_changed = true;
        fcb->extents_changed = true;
    }

    mark_fcb_dirty(fcb);

    if (FileObject->SectionObjectPointer->DataSectionObject)
        CcPurgeCacheSection(FileObject->SectionObjectPointer, &ded->TargetFileOffset, (ULONG)ded->ByteCount.QuadPart, false);

    Status = STATUS_SUCCESS;

end:
    ObDereferenceObject(sourcefo);

    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    if (fcb != sourcefcb)
        ExReleaseResourceLite(sourcefcb->Header.Resource);

    ExReleaseResourceLite(fcb->Header.Resource);

    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS check_inode_used(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb,
                                 _In_ root* subvol, _In_ uint64_t inode, _In_ uint32_t hash, _In_opt_ PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    uint8_t c = hash >> 24;

    if (subvol->fcbs_ptrs[c]) {
        LIST_ENTRY* le = subvol->fcbs_ptrs[c];

        while (le != &subvol->fcbs) {
            struct _fcb* fcb2 = CONTAINING_RECORD(le, struct _fcb, list_entry);

            if (fcb2->inode == inode)
                return STATUS_SUCCESS;
            else if (fcb2->hash > hash)
                break;

            le = le->Flink;
        }
    }

    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, subvol, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
        return STATUS_SUCCESS;

    return STATUS_NOT_FOUND;
}

static NTSTATUS mknod(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, PIRP Irp) {
    NTSTATUS Status;
    btrfs_mknod* bmn;
    fcb *parfcb, *fcb;
    ccb* parccb;
    file_ref *parfileref, *fileref;
    UNICODE_STRING name;
    root* subvol;
    uint64_t inode;
    dir_child* dc;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ANSI_STRING utf8;
    ULONG len, i;
    SECURITY_SUBJECT_CONTEXT subjcont;
    PSID owner;
    BOOLEAN defaulted;

    TRACE("(%p, %p, %p, %lu)\n", Vcb, FileObject, data, datalen);

    if (!FileObject || !FileObject->FsContext || !FileObject->FsContext2 || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    parfcb = FileObject->FsContext;

    if (parfcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("trying to create file in something other than a directory\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (is_subvol_readonly(parfcb->subvol, Irp))
        return STATUS_ACCESS_DENIED;

    parccb = FileObject->FsContext2;
    parfileref = parccb->fileref;

    if (!parfileref)
        return STATUS_INVALID_PARAMETER;

    if (datalen < sizeof(btrfs_mknod))
        return STATUS_INVALID_PARAMETER;

    bmn = (btrfs_mknod*)data;

    if (datalen < offsetof(btrfs_mknod, name[0]) + bmn->namelen || bmn->namelen < sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER;

    if (bmn->type == BTRFS_TYPE_UNKNOWN || bmn->type > BTRFS_TYPE_SYMLINK)
        return STATUS_INVALID_PARAMETER;

    if ((bmn->type == BTRFS_TYPE_DIRECTORY && !(parccb->access & FILE_ADD_SUBDIRECTORY)) ||
        (bmn->type != BTRFS_TYPE_DIRECTORY && !(parccb->access & FILE_ADD_FILE))) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    if (bmn->inode != 0) {
        if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode))
            return STATUS_PRIVILEGE_NOT_HELD;
    }

    for (i = 0; i < bmn->namelen / sizeof(WCHAR); i++) {
        if (bmn->name[i] == 0 || bmn->name[i] == '/')
            return STATUS_OBJECT_NAME_INVALID;
    }

    // don't allow files called . or ..
    if (bmn->name[0] == '.' && (bmn->namelen == sizeof(WCHAR) || (bmn->namelen == 2 * sizeof(WCHAR) && bmn->name[1] == '.')))
        return STATUS_OBJECT_NAME_INVALID;

    Status = utf16_to_utf8(NULL, 0, &len, bmn->name, bmn->namelen);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 returned %08lx\n", Status);
        return Status;
    }

    if (len == 0) {
        ERR("utf16_to_utf8 returned a length of 0\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (len > 0xffff) {
        ERR("len was too long (%lx)\n", len);
        return STATUS_INVALID_PARAMETER;
    }

    utf8.MaximumLength = utf8.Length = (USHORT)len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);

    if (!utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = utf16_to_utf8(utf8.Buffer, len, &len, bmn->name, bmn->namelen);
    if (!NT_SUCCESS(Status)) {
        ERR("utf16_to_utf8 failed with error %08lx\n", Status);
        ExFreePool(utf8.Buffer);
        return Status;
    }

    name.Length = name.MaximumLength = bmn->namelen;
    name.Buffer = bmn->name;

    Status = find_file_in_dir(&name, parfcb, &subvol, &inode, &dc, true);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("find_file_in_dir returned %08lx\n", Status);
        goto end;
    }

    if (NT_SUCCESS(Status)) {
        WARN("filename already exists\n");
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb = create_fcb(Vcb, PagedPool);
    if (!fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fcb->Vcb = Vcb;

    fcb->inode_item.generation = Vcb->superblock.generation;
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size = 0;
    fcb->inode_item.st_blocks = 0;
    fcb->inode_item.block_group = 0;
    fcb->inode_item.st_nlink = 1;
    fcb->inode_item.st_uid = UID_NOBODY;
    fcb->inode_item.st_gid = GID_NOBODY;
    fcb->inode_item.st_mode = inherit_mode(parfcb, bmn->type == BTRFS_TYPE_DIRECTORY);

    if (bmn->type == BTRFS_TYPE_BLOCKDEV || bmn->type == BTRFS_TYPE_CHARDEV)
        fcb->inode_item.st_rdev = (minor(bmn->st_rdev) & 0xFFFFF) | ((major(bmn->st_rdev) & 0xFFFFFFFFFFF) << 20);
    else
        fcb->inode_item.st_rdev = 0;

    fcb->inode_item.flags = 0;
    fcb->inode_item.sequence = 1;
    fcb->inode_item.st_atime = now;
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    fcb->inode_item.otime = now;

    if (bmn->type == BTRFS_TYPE_DIRECTORY)
        fcb->inode_item.st_mode |= __S_IFDIR;
    else if (bmn->type == BTRFS_TYPE_CHARDEV)
        fcb->inode_item.st_mode |= __S_IFCHR;
    else if (bmn->type == BTRFS_TYPE_BLOCKDEV)
        fcb->inode_item.st_mode |= __S_IFBLK;
    else if (bmn->type == BTRFS_TYPE_FIFO)
        fcb->inode_item.st_mode |= __S_IFIFO;
    else if (bmn->type == BTRFS_TYPE_SOCKET)
        fcb->inode_item.st_mode |= __S_IFSOCK;
    else if (bmn->type == BTRFS_TYPE_SYMLINK)
        fcb->inode_item.st_mode |= __S_IFLNK;
    else
        fcb->inode_item.st_mode |= __S_IFREG;

    if (bmn->type != BTRFS_TYPE_DIRECTORY)
        fcb->inode_item.st_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH); // remove executable bit if not directory

    // inherit nodatacow flag from parent directory
    if (parfcb->inode_item.flags & BTRFS_INODE_NODATACOW) {
        fcb->inode_item.flags |= BTRFS_INODE_NODATACOW;

        if (bmn->type != BTRFS_TYPE_DIRECTORY)
            fcb->inode_item.flags |= BTRFS_INODE_NODATASUM;
    }

    if (parfcb->inode_item.flags & BTRFS_INODE_COMPRESS)
        fcb->inode_item.flags |= BTRFS_INODE_COMPRESS;

    fcb->prop_compression = parfcb->prop_compression;
    fcb->prop_compression_changed = fcb->prop_compression != PropCompression_None;

    fcb->inode_item_changed = true;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;

    fcb->atts = 0;

    if (bmn->name[0] == '.')
        fcb->atts |= FILE_ATTRIBUTE_HIDDEN;

    if (bmn->type == BTRFS_TYPE_DIRECTORY)
        fcb->atts |= FILE_ATTRIBUTE_DIRECTORY;

    fcb->atts_changed = false;

    InterlockedIncrement(&parfcb->refcount);
    fcb->subvol = parfcb->subvol;

    SeCaptureSubjectContext(&subjcont);

    Status = SeAssignSecurityEx(parfileref ? parfileref->fcb->sd : NULL, NULL, (void**)&fcb->sd, NULL, fcb->type == BTRFS_TYPE_DIRECTORY,
                                SEF_SACL_AUTO_INHERIT, &subjcont, IoGetFileObjectGenericMapping(), PagedPool);

    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurityEx returned %08lx\n", Status);
        reap_fcb(fcb);
        goto end;
    }

    Status = RtlGetOwnerSecurityDescriptor(fcb->sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        WARN("RtlGetOwnerSecurityDescriptor returned %08lx\n", Status);
        fcb->sd_dirty = true;
    } else {
        fcb->inode_item.st_uid = sid_to_uid(owner);
        fcb->sd_dirty = fcb->inode_item.st_uid == UID_NOBODY;
    }

    find_gid(fcb, parfcb, &subjcont);

    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, true);
    acquire_fcb_lock_exclusive(Vcb);

    if (bmn->inode == 0) {
        fcb->inode = InterlockedIncrement64(&parfcb->subvol->lastinode);
        fcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&fcb->inode, sizeof(uint64_t));
    } else {
        if (bmn->inode > (uint64_t)parfcb->subvol->lastinode) {
            fcb->inode = parfcb->subvol->lastinode = bmn->inode;
            fcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&fcb->inode, sizeof(uint64_t));
        } else {
            uint32_t hash = calc_crc32c(0xffffffff, (uint8_t*)&bmn->inode, sizeof(uint64_t));

            Status = check_inode_used(Vcb, subvol, bmn->inode, hash, Irp);
            if (NT_SUCCESS(Status)) { // STATUS_SUCCESS means inode found
                release_fcb_lock(Vcb);
                ExReleaseResourceLite(&Vcb->fileref_lock);

                WARN("inode collision\n");
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            } else if (Status != STATUS_NOT_FOUND) {
                ERR("check_inode_used returned %08lx\n", Status);

                release_fcb_lock(Vcb);
                ExReleaseResourceLite(&Vcb->fileref_lock);
                goto end;
            }

            fcb->inode = bmn->inode;
            fcb->hash = hash;
        }
    }

    fcb->inode = inode;
    fcb->type = bmn->type;

    fileref = create_fileref(Vcb);
    if (!fileref) {
        release_fcb_lock(Vcb);
        ExReleaseResourceLite(&Vcb->fileref_lock);

        ERR("out of memory\n");
        reap_fcb(fcb);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fileref->fcb = fcb;

    fcb->created = true;
    fileref->created = true;

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    fileref->parent = parfileref;

    mark_fcb_dirty(fcb);
    mark_fileref_dirty(fileref);

    Status = add_dir_child(fileref->parent->fcb, fcb->inode, false, &utf8, &name, fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08lx\n", Status);

    fileref->dc = dc;
    dc->fileref = fileref;

    ExAcquireResourceExclusiveLite(&parfileref->fcb->nonpaged->dir_children_lock, true);
    InsertTailList(&parfileref->children, &fileref->list_entry);
    ExReleaseResourceLite(&parfileref->fcb->nonpaged->dir_children_lock);

    increase_fileref_refcount(parfileref);

    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs) {
            release_fcb_lock(Vcb);
            ExReleaseResourceLite(&Vcb->fileref_lock);

            ERR("out of memory\n");
            free_fileref(fileref);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlZeroMemory(fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);

        fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs_uc) {
            release_fcb_lock(Vcb);
            ExReleaseResourceLite(&Vcb->fileref_lock);

            ERR("out of memory\n");
            free_fileref(fileref);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlZeroMemory(fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);
    }

    add_fcb_to_subvol(fcb);
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);

    if (bmn->type == BTRFS_TYPE_DIRECTORY)
        fileref->fcb->fileref = fileref;

    ExAcquireResourceExclusiveLite(parfcb->Header.Resource, true);
    parfcb->inode_item.st_size += utf8.Length * 2;
    parfcb->inode_item.transid = Vcb->superblock.generation;
    parfcb->inode_item.sequence++;

    if (!parccb->user_set_change_time)
        parfcb->inode_item.st_ctime = now;

    if (!parccb->user_set_write_time)
        parfcb->inode_item.st_mtime = now;

    parfcb->subvol->fcbs_version++;

    ExReleaseResourceLite(parfcb->Header.Resource);
    release_fcb_lock(Vcb);
    ExReleaseResourceLite(&Vcb->fileref_lock);

    parfcb->inode_item_changed = true;
    mark_fcb_dirty(parfcb);

    send_notification_fileref(fileref, bmn->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);

    if (!parccb->user_set_write_time)
        queue_notification_fcb(parfileref, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:

    ExFreePool(utf8.Buffer);

    return Status;
}

static void mark_subvol_dirty(device_extension* Vcb, root* r) {
    if (!r->dirty) {
        r->dirty = true;

        ExAcquireResourceExclusiveLite(&Vcb->dirty_subvols_lock, true);
        InsertTailList(&Vcb->dirty_subvols, &r->list_entry_dirty);
        ExReleaseResourceLite(&Vcb->dirty_subvols_lock);
    }

    Vcb->need_write = true;
}

static NTSTATUS recvd_subvol(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, KPROCESSOR_MODE processor_mode) {
    btrfs_received_subvol* brs = (btrfs_received_subvol*)data;
    fcb* fcb;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;

    TRACE("(%p, %p, %p, %lu)\n", Vcb, FileObject, data, datalen);

    if (!data || datalen < sizeof(btrfs_received_subvol))
        return STATUS_INVALID_PARAMETER;

    if (!FileObject || !FileObject->FsContext || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb->subvol)
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    if (fcb->subvol->root_item.rtransid != 0) {
        WARN("subvol already has received information set\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    RtlCopyMemory(&fcb->subvol->root_item.received_uuid, &brs->uuid, sizeof(BTRFS_UUID));
    fcb->subvol->root_item.stransid = brs->generation;
    fcb->subvol->root_item.rtransid = Vcb->superblock.generation;
    fcb->subvol->root_item.rtime = now;

    fcb->subvol->received = true;
    mark_subvol_dirty(Vcb, fcb->subvol);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS fsctl_get_xattrs(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, KPROCESSOR_MODE processor_mode) {
    LIST_ENTRY* le;
    btrfs_set_xattr* bsxa;
    ULONG reqlen = (ULONG)offsetof(btrfs_set_xattr, data[0]);
    fcb* fcb;
    ccb* ccb;

    if (!data || datalen < reqlen)
        return STATUS_INVALID_PARAMETER;

    if (!FileObject || !FileObject->FsContext || !FileObject->FsContext2 || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES)) && processor_mode == UserMode) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    le = fcb->xattrs.Flink;
    while (le != &fcb->xattrs) {
        xattr* xa = CONTAINING_RECORD(le, xattr, list_entry);

        if (xa->valuelen > 0)
            reqlen += (ULONG)offsetof(btrfs_set_xattr, data[0]) + xa->namelen + xa->valuelen;

        le = le->Flink;
    }

    if (datalen < reqlen) {
        ExReleaseResourceLite(fcb->Header.Resource);
        return STATUS_BUFFER_OVERFLOW;
    }

    bsxa = (btrfs_set_xattr*)data;

    if (reqlen > 0) {
        le = fcb->xattrs.Flink;
        while (le != &fcb->xattrs) {
            xattr* xa = CONTAINING_RECORD(le, xattr, list_entry);

            if (xa->valuelen > 0) {
                bsxa->namelen = xa->namelen;
                bsxa->valuelen = xa->valuelen;
                memcpy(bsxa->data, xa->data, xa->namelen + xa->valuelen);

                bsxa = (btrfs_set_xattr*)&bsxa->data[xa->namelen + xa->valuelen];
            }

            le = le->Flink;
        }
    }

    bsxa->namelen = 0;
    bsxa->valuelen = 0;

    ExReleaseResourceLite(fcb->Header.Resource);

    return STATUS_SUCCESS;
}

static NTSTATUS fsctl_set_xattr(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, PIRP Irp) {
    NTSTATUS Status;
    btrfs_set_xattr* bsxa;
    xattr* xa;
    fcb* fcb;
    ccb* ccb;
    LIST_ENTRY* le;

    static const char stream_pref[] = "user.";

    TRACE("(%p, %p, %p, %lu)\n", Vcb, FileObject, data, datalen);

    if (!data || datalen < sizeof(btrfs_set_xattr))
        return STATUS_INVALID_PARAMETER;

    bsxa = (btrfs_set_xattr*)data;

    if (datalen < offsetof(btrfs_set_xattr, data[0]) + bsxa->namelen + bsxa->valuelen)
        return STATUS_INVALID_PARAMETER;

    if (bsxa->namelen + bsxa->valuelen + sizeof(tree_header) + sizeof(leaf_node) + offsetof(DIR_ITEM, name[0]) > Vcb->superblock.node_size)
        return STATUS_INVALID_PARAMETER;

    if (!FileObject || !FileObject->FsContext || !FileObject->FsContext2 || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (is_subvol_readonly(fcb->subvol, Irp))
        return STATUS_ACCESS_DENIED;

    if (!(ccb->access & FILE_WRITE_ATTRIBUTES) && Irp->RequestorMode == UserMode) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (bsxa->namelen == sizeof(EA_NTACL) - 1 && RtlCompareMemory(bsxa->data, EA_NTACL, sizeof(EA_NTACL) - 1) == sizeof(EA_NTACL) - 1) {
        if ((!(ccb->access & WRITE_DAC) || !(ccb->access & WRITE_OWNER)) && Irp->RequestorMode == UserMode) {
            WARN("insufficient privileges\n");
            Status = STATUS_ACCESS_DENIED;
            goto end;
        }

        if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode)) {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto end;
        }

        if (fcb->sd)
            ExFreePool(fcb->sd);

        if (bsxa->valuelen > 0 && RtlValidRelativeSecurityDescriptor(bsxa->data + bsxa->namelen, bsxa->valuelen, 0)) {
            fcb->sd = ExAllocatePoolWithTag(PagedPool, bsxa->valuelen, ALLOC_TAG);
            if (!fcb->sd) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlCopyMemory(fcb->sd, bsxa->data + bsxa->namelen, bsxa->valuelen);
        } else if (fcb->sd)
            fcb->sd = NULL;

        fcb->sd_dirty = true;

        if (!fcb->sd) {
            fcb_get_sd(fcb, ccb->fileref->parent->fcb, false, Irp);
            fcb->sd_deleted = true;
        }

        mark_fcb_dirty(fcb);

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen == sizeof(EA_DOSATTRIB) - 1 && RtlCompareMemory(bsxa->data, EA_DOSATTRIB, sizeof(EA_DOSATTRIB) - 1) == sizeof(EA_DOSATTRIB) - 1) {
        ULONG atts;

        if (bsxa->valuelen > 0 && get_file_attributes_from_xattr(bsxa->data + bsxa->namelen, bsxa->valuelen, &atts)) {
            fcb->atts = atts;

            if (fcb->type == BTRFS_TYPE_DIRECTORY)
                fcb->atts |= FILE_ATTRIBUTE_DIRECTORY;
            else if (fcb->type == BTRFS_TYPE_SYMLINK)
                fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;

            if (fcb->inode == SUBVOL_ROOT_INODE) {
                if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY)
                    fcb->atts |= FILE_ATTRIBUTE_READONLY;
                else
                    fcb->atts &= ~FILE_ATTRIBUTE_READONLY;
            }

            fcb->atts_deleted = false;
        } else {
            bool hidden = ccb->fileref && ccb->fileref->dc && ccb->fileref->dc->utf8.Buffer && ccb->fileref->dc->utf8.Buffer[0] == '.';

            fcb->atts = get_file_attributes(Vcb, fcb->subvol, fcb->inode, fcb->type, hidden, true, Irp);
            fcb->atts_deleted = true;
        }

        fcb->atts_changed = true;
        mark_fcb_dirty(fcb);

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen == sizeof(EA_REPARSE) - 1 && RtlCompareMemory(bsxa->data, EA_REPARSE, sizeof(EA_REPARSE) - 1) == sizeof(EA_REPARSE) - 1) {
        if (fcb->reparse_xattr.Buffer) {
            ExFreePool(fcb->reparse_xattr.Buffer);
            fcb->reparse_xattr.Buffer = NULL;
            fcb->reparse_xattr.Length = fcb->reparse_xattr.MaximumLength = 0;
        }

        if (bsxa->valuelen > 0) {
            fcb->reparse_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, bsxa->valuelen, ALLOC_TAG);
            if (!fcb->reparse_xattr.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlCopyMemory(fcb->reparse_xattr.Buffer, bsxa->data + bsxa->namelen, bsxa->valuelen);
            fcb->reparse_xattr.Length = fcb->reparse_xattr.MaximumLength = bsxa->valuelen;
        }

        fcb->reparse_xattr_changed = true;
        mark_fcb_dirty(fcb);

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen == sizeof(EA_EA) - 1 && RtlCompareMemory(bsxa->data, EA_EA, sizeof(EA_EA) - 1) == sizeof(EA_EA) - 1) {
        if (!(ccb->access & FILE_WRITE_EA) && Irp->RequestorMode == UserMode) {
            WARN("insufficient privileges\n");
            Status = STATUS_ACCESS_DENIED;
            goto end;
        }

        if (fcb->ea_xattr.Buffer) {
            ExFreePool(fcb->ea_xattr.Buffer);
            fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = 0;
            fcb->ea_xattr.Buffer = NULL;
        }

        fcb->ealen = 0;

        if (bsxa->valuelen > 0) {
            ULONG offset;

            Status = IoCheckEaBufferValidity((FILE_FULL_EA_INFORMATION*)(bsxa->data + bsxa->namelen), bsxa->valuelen, &offset);

            if (!NT_SUCCESS(Status))
                WARN("IoCheckEaBufferValidity returned %08lx (error at offset %lu)\n", Status, offset);
            else {
                FILE_FULL_EA_INFORMATION* eainfo;

                fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, bsxa->valuelen, ALLOC_TAG);
                if (!fcb->ea_xattr.Buffer) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                RtlCopyMemory(fcb->ea_xattr.Buffer, bsxa->data + bsxa->namelen, bsxa->valuelen);

                fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = bsxa->valuelen;

                fcb->ealen = 4;

                // calculate ealen
                eainfo = (FILE_FULL_EA_INFORMATION*)(bsxa->data + bsxa->namelen);
                do {
                    fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;

                    if (eainfo->NextEntryOffset == 0)
                        break;

                    eainfo = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)eainfo) + eainfo->NextEntryOffset);
                } while (true);
            }
        }

        fcb->ea_changed = true;
        mark_fcb_dirty(fcb);

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen == sizeof(EA_CASE_SENSITIVE) - 1 && RtlCompareMemory(bsxa->data, EA_CASE_SENSITIVE, sizeof(EA_CASE_SENSITIVE) - 1) == sizeof(EA_CASE_SENSITIVE) - 1) {
        if (bsxa->valuelen > 0 && bsxa->data[bsxa->namelen] == '1') {
            fcb->case_sensitive = true;
            mark_fcb_dirty(fcb);
        }

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen == sizeof(EA_PROP_COMPRESSION) - 1 && RtlCompareMemory(bsxa->data, EA_PROP_COMPRESSION, sizeof(EA_PROP_COMPRESSION) - 1) == sizeof(EA_PROP_COMPRESSION) - 1) {
        static const char lzo[] = "lzo";
        static const char zlib[] = "zlib";
        static const char zstd[] = "zstd";

        if (bsxa->valuelen == sizeof(zstd) - 1 && RtlCompareMemory(bsxa->data + bsxa->namelen, zstd, bsxa->valuelen) == bsxa->valuelen)
            fcb->prop_compression = PropCompression_ZSTD;
        else if (bsxa->valuelen == sizeof(lzo) - 1 && RtlCompareMemory(bsxa->data + bsxa->namelen, lzo, bsxa->valuelen) == bsxa->valuelen)
            fcb->prop_compression = PropCompression_LZO;
        else if (bsxa->valuelen == sizeof(zlib) - 1 && RtlCompareMemory(bsxa->data + bsxa->namelen, zlib, bsxa->valuelen) == bsxa->valuelen)
            fcb->prop_compression = PropCompression_Zlib;
        else
            fcb->prop_compression = PropCompression_None;

        if (fcb->prop_compression != PropCompression_None) {
            fcb->inode_item.flags |= BTRFS_INODE_COMPRESS;
            fcb->inode_item_changed = true;
        }

        fcb->prop_compression_changed = true;
        mark_fcb_dirty(fcb);

        Status = STATUS_SUCCESS;
        goto end;
    } else if (bsxa->namelen >= (sizeof(stream_pref) - 1) && RtlCompareMemory(bsxa->data, stream_pref, sizeof(stream_pref) - 1) == sizeof(stream_pref) - 1) {
        // don't allow xattrs beginning with user., as these appear as streams instead
        Status = STATUS_OBJECT_NAME_INVALID;
        goto end;
    }

    xa = ExAllocatePoolWithTag(PagedPool, offsetof(xattr, data[0]) + bsxa->namelen + bsxa->valuelen, ALLOC_TAG);
    if (!xa) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    le = fcb->xattrs.Flink;
    while (le != &fcb->xattrs) {
        xattr* xa2 = CONTAINING_RECORD(le, xattr, list_entry);

        if (xa2->namelen == bsxa->namelen && RtlCompareMemory(xa2->data, bsxa->data, xa2->namelen) == xa2->namelen) {
            RemoveEntryList(&xa2->list_entry);
            ExFreePool(xa2);
            break;
        }

        le = le->Flink;
    }

    xa->namelen = bsxa->namelen;
    xa->valuelen = bsxa->valuelen;
    xa->dirty = true;
    RtlCopyMemory(xa->data, bsxa->data, bsxa->namelen + bsxa->valuelen);

    InsertTailList(&fcb->xattrs, &xa->list_entry);

    fcb->xattrs_changed = true;
    mark_fcb_dirty(fcb);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS reserve_subvol(device_extension* Vcb, PFILE_OBJECT FileObject, PIRP Irp) {
    fcb* fcb;
    ccb* ccb;

    TRACE("(%p, %p)\n", Vcb, FileObject);

    // "Reserving" a readonly subvol allows the calling process to write into it until the handle is closed.

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!FileObject || !FileObject->FsContext || !FileObject->FsContext2 || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!(fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY))
        return STATUS_INVALID_PARAMETER;

    if (fcb->subvol->reserved)
        return STATUS_INVALID_PARAMETER;

    fcb->subvol->reserved = PsGetCurrentProcess();
    ccb->reserving = true;

    return STATUS_SUCCESS;
}

static NTSTATUS get_subvol_path(device_extension* Vcb, uint64_t id, WCHAR* out, ULONG outlen, PIRP Irp) {
    LIST_ENTRY* le;
    root* r = NULL;
    NTSTATUS Status;
    file_ref* fr;
    UNICODE_STRING us;

    le = Vcb->roots.Flink;
    while (le != &Vcb->roots) {
        root* r2 = CONTAINING_RECORD(le, root, list_entry);

        if (r2->id == id) {
            r = r2;
            break;
        }

        le = le->Flink;
    }

    if (!r) {
        ERR("couldn't find subvol %I64x\n", id);
        return STATUS_INTERNAL_ERROR;
    }

    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, true);

    Status = open_fileref_by_inode(Vcb, r, r->root_item.objid, &fr, Irp);
    if (!NT_SUCCESS(Status)) {
        ExReleaseResourceLite(&Vcb->fileref_lock);
        ERR("open_fileref_by_inode returned %08lx\n", Status);
        return Status;
    }

    us.Buffer = out;
    us.Length = 0;
    us.MaximumLength = (USHORT)min(0xffff, outlen) - sizeof(WCHAR);

    Status = fileref_get_filename(fr, &us, NULL, NULL);

    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
        out[us.Length / sizeof(WCHAR)] = 0;
    else
        ERR("fileref_get_filename returned %08lx\n", Status);

    free_fileref(fr);

    ExReleaseResourceLite(&Vcb->fileref_lock);

    return Status;
}

static NTSTATUS find_subvol(device_extension* Vcb, void* in, ULONG inlen, void* out, ULONG outlen, PIRP Irp) {
    btrfs_find_subvol* bfs;
    NTSTATUS Status;
    traverse_ptr tp;
    KEY searchkey;

    if (!in || inlen < sizeof(btrfs_find_subvol))
        return STATUS_INVALID_PARAMETER;

    if (!out || outlen < sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    bfs = (btrfs_find_subvol*)in;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    if (!Vcb->uuid_root) {
        ERR("couldn't find uuid root\n");
        Status = STATUS_NOT_FOUND;
        goto end;
    }

    RtlCopyMemory(&searchkey.obj_id, &bfs->uuid, sizeof(uint64_t));
    searchkey.obj_type = TYPE_SUBVOL_UUID;
    RtlCopyMemory(&searchkey.offset, &bfs->uuid.uuid[sizeof(uint64_t)], sizeof(uint64_t));

    Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, false, Irp);

    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        goto end;
    }

    if (!keycmp(searchkey, tp.item->key) && tp.item->size >= sizeof(uint64_t)) {
        uint64_t* id = (uint64_t*)tp.item->data;

        if (bfs->ctransid != 0) {
            KEY searchkey2;
            traverse_ptr tp2;

            searchkey2.obj_id = *id;
            searchkey2.obj_type = TYPE_ROOT_ITEM;
            searchkey2.offset = 0xffffffffffffffff;

            Status = find_item(Vcb, Vcb->root_root, &tp2, &searchkey2, false, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("find_item returned %08lx\n", Status);
                goto end;
            }

            if (tp2.item->key.obj_id == searchkey2.obj_id && tp2.item->key.obj_type == searchkey2.obj_type &&
                tp2.item->size >= offsetof(ROOT_ITEM, otransid)) {
                ROOT_ITEM* ri = (ROOT_ITEM*)tp2.item->data;

                if (ri->ctransid == bfs->ctransid) {
                    TRACE("found subvol %I64x\n", *id);
                    Status = get_subvol_path(Vcb, *id, out, outlen, Irp);
                    goto end;
                }
            }
        } else {
            TRACE("found subvol %I64x\n", *id);
            Status = get_subvol_path(Vcb, *id, out, outlen, Irp);
            goto end;
        }
    }

    searchkey.obj_type = TYPE_SUBVOL_REC_UUID;

    Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, false, Irp);

    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08lx\n", Status);
        goto end;
    }

    if (!keycmp(searchkey, tp.item->key) && tp.item->size >= sizeof(uint64_t)) {
        uint64_t* ids = (uint64_t*)tp.item->data;
        ULONG i;

        for (i = 0; i < tp.item->size / sizeof(uint64_t); i++) {
            if (bfs->ctransid != 0) {
                KEY searchkey2;
                traverse_ptr tp2;

                searchkey2.obj_id = ids[i];
                searchkey2.obj_type = TYPE_ROOT_ITEM;
                searchkey2.offset = 0xffffffffffffffff;

                Status = find_item(Vcb, Vcb->root_root, &tp2, &searchkey2, false, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_item returned %08lx\n", Status);
                    goto end;
                }

                if (tp2.item->key.obj_id == searchkey2.obj_id && tp2.item->key.obj_type == searchkey2.obj_type &&
                    tp2.item->size >= offsetof(ROOT_ITEM, otransid)) {
                    ROOT_ITEM* ri = (ROOT_ITEM*)tp2.item->data;

                    if (ri->ctransid == bfs->ctransid) {
                        TRACE("found subvol %I64x\n", ids[i]);
                        Status = get_subvol_path(Vcb, ids[i], out, outlen, Irp);
                        goto end;
                    }
                }
            } else {
                TRACE("found subvol %I64x\n", ids[i]);
                Status = get_subvol_path(Vcb, ids[i], out, outlen, Irp);
                goto end;
            }
        }
    }

    Status = STATUS_NOT_FOUND;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS resize_device(device_extension* Vcb, void* data, ULONG len, PIRP Irp) {
    btrfs_resize* br = (btrfs_resize*)data;
    NTSTATUS Status;
    LIST_ENTRY* le;
    device* dev = NULL;

    TRACE("(%p, %p, %lu)\n", Vcb, data, len);

    if (!data || len < sizeof(btrfs_resize) || (br->size & (Vcb->superblock.sector_size - 1)) != 0)
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);

        if (dev2->devitem.dev_id == br->device) {
            dev = dev2;
            break;
        }

        le = le->Flink;
    }

    if (!dev) {
        ERR("could not find device %I64x\n", br->device);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!dev->devobj) {
        ERR("trying to resize missing device\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (dev->readonly) {
        ERR("trying to resize readonly device\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (br->size > 0 && dev->devitem.num_bytes == br->size) {
        TRACE("size unchanged, returning STATUS_SUCCESS\n");
        Status = STATUS_SUCCESS;
        goto end;
    }

    if (br->size > 0 && dev->devitem.num_bytes > br->size) { // shrink device
        bool need_balance = true;
        uint64_t old_size, delta;

        le = dev->space.Flink;
        while (le != &dev->space) {
            space* s = CONTAINING_RECORD(le, space, list_entry);

            if (s->address <= br->size && s->address + s->size >= dev->devitem.num_bytes) {
                need_balance = false;
                break;
            }

            le = le->Flink;
        }

        delta = dev->devitem.num_bytes - br->size;

        if (need_balance) {
            OBJECT_ATTRIBUTES oa;
            int i;

            if (Vcb->balance.thread) {
                WARN("balance already running\n");
                Status = STATUS_DEVICE_NOT_READY;
                goto end;
            }

            RtlZeroMemory(Vcb->balance.opts, sizeof(btrfs_balance_opts) * 3);

            for (i = 0; i < 3; i++) {
                Vcb->balance.opts[i].flags = BTRFS_BALANCE_OPTS_ENABLED | BTRFS_BALANCE_OPTS_DEVID | BTRFS_BALANCE_OPTS_DRANGE;
                Vcb->balance.opts[i].devid = dev->devitem.dev_id;
                Vcb->balance.opts[i].drange_start = br->size;
                Vcb->balance.opts[i].drange_end = dev->devitem.num_bytes;
            }

            Vcb->balance.paused = false;
            Vcb->balance.shrinking = true;
            Vcb->balance.status = STATUS_SUCCESS;
            KeInitializeEvent(&Vcb->balance.event, NotificationEvent, !Vcb->balance.paused);

            space_list_subtract2(&dev->space, NULL, br->size, delta, NULL, NULL);

            InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

            Status = PsCreateSystemThread(&Vcb->balance.thread, 0, &oa, NULL, NULL, balance_thread, Vcb);
            if (!NT_SUCCESS(Status)) {
                ERR("PsCreateSystemThread returned %08lx\n", Status);
                goto end;
            }

            Status = STATUS_MORE_PROCESSING_REQUIRED;

            goto end;
        }

        old_size = dev->devitem.num_bytes;
        dev->devitem.num_bytes = br->size;

        Status = update_dev_item(Vcb, dev, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("update_dev_item returned %08lx\n", Status);
            dev->devitem.num_bytes = old_size;
            goto end;
        }

        space_list_subtract2(&dev->space, NULL, br->size, delta, NULL, NULL);

        Vcb->superblock.total_bytes -= delta;
    } else { // extend device
        GET_LENGTH_INFORMATION gli;
        uint64_t old_size, delta;

        Status = dev_ioctl(dev->devobj, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                           &gli, sizeof(gli), true, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_DISK_GET_LENGTH_INFO returned %08lx\n", Status);
            goto end;
        }

        if (br->size == 0) {
            br->size = gli.Length.QuadPart;

            if (dev->devitem.num_bytes == br->size) {
                TRACE("size unchanged, returning STATUS_SUCCESS\n");
                Status = STATUS_SUCCESS;
                goto end;
            }

            if (br->size == 0) {
                ERR("IOCTL_DISK_GET_LENGTH_INFO returned 0 length\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
        } else if ((uint64_t)gli.Length.QuadPart < br->size) {
            ERR("device was %I64x bytes, trying to extend to %I64x\n", gli.Length.QuadPart, br->size);
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        delta = br->size - dev->devitem.num_bytes;

        old_size = dev->devitem.num_bytes;
        dev->devitem.num_bytes = br->size;

        Status = update_dev_item(Vcb, dev, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("update_dev_item returned %08lx\n", Status);
            dev->devitem.num_bytes = old_size;
            goto end;
        }

        space_list_add2(&dev->space, NULL, dev->devitem.num_bytes, delta, NULL, NULL);

        Vcb->superblock.total_bytes += delta;
    }

    Status = STATUS_SUCCESS;
    Vcb->need_write = true;

end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    if (NT_SUCCESS(Status))
        FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_CHANGE_SIZE);

    return Status;
}

static NTSTATUS fsctl_oplock(device_extension* Vcb, PIRP* Pirp) {
    NTSTATUS Status;
    PIRP Irp = *Pirp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    uint32_t fsctl = IrpSp->Parameters.FileSystemControl.FsControlCode;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
    ccb* ccb = FileObject ? FileObject->FsContext2 : NULL;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    PREQUEST_OPLOCK_INPUT_BUFFER buf = NULL;
    bool oplock_request = false, oplock_ack = false;
#else
    bool oplock_request = false;
#endif
    ULONG oplock_count = 0;
#ifdef __REACTOS__
    bool shared_request;
#endif

    if (!fcb) {
        ERR("fcb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_DIRECTORY)
        return STATUS_INVALID_PARAMETER;

#if (NTDDI_VERSION >= NTDDI_WIN7)
    if (fsctl == FSCTL_REQUEST_OPLOCK) {
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(REQUEST_OPLOCK_INPUT_BUFFER))
            return STATUS_BUFFER_TOO_SMALL;

        if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(REQUEST_OPLOCK_OUTPUT_BUFFER))
            return STATUS_BUFFER_TOO_SMALL;

        buf = Irp->AssociatedIrp.SystemBuffer;

        // flags are mutually exclusive
        if (buf->Flags & REQUEST_OPLOCK_INPUT_FLAG_REQUEST && buf->Flags & REQUEST_OPLOCK_INPUT_FLAG_ACK)
            return STATUS_INVALID_PARAMETER;

        oplock_request = buf->Flags & REQUEST_OPLOCK_INPUT_FLAG_REQUEST;
        oplock_ack = buf->Flags & REQUEST_OPLOCK_INPUT_FLAG_ACK;

        if (!oplock_request && !oplock_ack)
            return STATUS_INVALID_PARAMETER;
    }
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
    bool shared_request = (fsctl == FSCTL_REQUEST_OPLOCK_LEVEL_2) || (fsctl == FSCTL_REQUEST_OPLOCK && !(buf->RequestedOplockLevel & OPLOCK_LEVEL_CACHE_WRITE));
#else
    shared_request = (fsctl == FSCTL_REQUEST_OPLOCK_LEVEL_2);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
    if (fcb->type == BTRFS_TYPE_DIRECTORY && (fsctl != FSCTL_REQUEST_OPLOCK || !shared_request)) {
#else
    if (fcb->type == BTRFS_TYPE_DIRECTORY && !shared_request) {
#endif
        WARN("oplock requests on directories can only be for read or read-handle oplocks\n");
        return STATUS_INVALID_PARAMETER;
    }

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fsctl == FSCTL_REQUEST_OPLOCK_LEVEL_1 || fsctl == FSCTL_REQUEST_BATCH_OPLOCK || fsctl == FSCTL_REQUEST_FILTER_OPLOCK ||
        fsctl == FSCTL_REQUEST_OPLOCK_LEVEL_2 || oplock_request) {
        if (shared_request) {
            if (fcb->type == BTRFS_TYPE_FILE) {
                if (fFsRtlCheckLockForOplockRequest)
                    oplock_count = !fFsRtlCheckLockForOplockRequest(&fcb->lock, &fcb->Header.AllocationSize);
                else if (fFsRtlAreThereCurrentOrInProgressFileLocks)
                    oplock_count = fFsRtlAreThereCurrentOrInProgressFileLocks(&fcb->lock);
                else
                    oplock_count = FsRtlAreThereCurrentFileLocks(&fcb->lock);
            }
        } else
            oplock_count = fileref->open_count;
    }

#if (NTDDI_VERSION >= NTDDI_WIN7)
    if ((fsctl == FSCTL_REQUEST_FILTER_OPLOCK || fsctl == FSCTL_REQUEST_BATCH_OPLOCK ||
        (fsctl == FSCTL_REQUEST_OPLOCK && buf->RequestedOplockLevel & OPLOCK_LEVEL_CACHE_HANDLE)) &&
#else
    if ((fsctl == FSCTL_REQUEST_FILTER_OPLOCK || fsctl == FSCTL_REQUEST_BATCH_OPLOCK) &&
#endif
        fileref->delete_on_close) {
        ExReleaseResourceLite(fcb->Header.Resource);
        ExReleaseResourceLite(&Vcb->tree_lock);
        return STATUS_DELETE_PENDING;
    }

    Status = FsRtlOplockFsctrl(fcb_oplock(fcb), Irp, oplock_count);

    *Pirp = NULL;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS get_retrieval_pointers(device_extension* Vcb, PFILE_OBJECT FileObject, STARTING_VCN_INPUT_BUFFER* in,
                                       ULONG inlen, RETRIEVAL_POINTERS_BUFFER* out, ULONG outlen, ULONG_PTR* retlen) {
    NTSTATUS Status;
    fcb* fcb;

    TRACE("get_retrieval_pointers(%p, %p, %p, %lx, %p, %lx, %p)\n", Vcb, FileObject, in, inlen,
                                                                    out, outlen, retlen);

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    if (inlen < sizeof(STARTING_VCN_INPUT_BUFFER) || in->StartingVcn.QuadPart < 0)
        return STATUS_INVALID_PARAMETER;

    if (!out)
        return STATUS_INVALID_PARAMETER;

    if (outlen < offsetof(RETRIEVAL_POINTERS_BUFFER, Extents[0]))
        return STATUS_BUFFER_TOO_SMALL;

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    _SEH2_TRY {
        LIST_ENTRY* le = fcb->extents.Flink;
        extent* first_ext = NULL;
        unsigned int num_extents = 0, first_extent_num = 0, i;
        uint64_t num_sectors, last_off = 0;

        num_sectors = (fcb->inode_item.st_size + Vcb->superblock.sector_size - 1) >> Vcb->sector_shift;

        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (ext->ignore || ext->extent_data.type == EXTENT_TYPE_INLINE) {
                le = le->Flink;
                continue;
            }

            if (ext->offset > last_off)
                num_extents++;

            if ((ext->offset >> Vcb->sector_shift) <= (uint64_t)in->StartingVcn.QuadPart &&
                (ext->offset + ext->extent_data.decoded_size) >> Vcb->sector_shift > (uint64_t)in->StartingVcn.QuadPart) {
                first_ext = ext;
                first_extent_num = num_extents;
            }

            num_extents++;

            last_off = ext->offset + ext->extent_data.decoded_size;

            le = le->Flink;
        }

        if (num_sectors > last_off >> Vcb->sector_shift)
            num_extents++;

        if (!first_ext) {
            Status = STATUS_END_OF_FILE;
            _SEH2_LEAVE;
        }

        out->ExtentCount = num_extents - first_extent_num;
        out->StartingVcn.QuadPart = first_ext->offset >> Vcb->sector_shift;
        outlen -= offsetof(RETRIEVAL_POINTERS_BUFFER, Extents[0]);
        *retlen = offsetof(RETRIEVAL_POINTERS_BUFFER, Extents[0]);

        le = &first_ext->list_entry;
        i = 0;
        last_off = 0;

        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (ext->ignore || ext->extent_data.type == EXTENT_TYPE_INLINE) {
                le = le->Flink;
                continue;
            }

            if (ext->offset > last_off) {
                if (outlen < sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER)) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    _SEH2_LEAVE;
                }

                out->Extents[i].NextVcn.QuadPart = ext->offset >> Vcb->sector_shift;
                out->Extents[i].Lcn.QuadPart = -1;

                outlen -= sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
                *retlen += sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
                i++;
            }

            if (outlen < sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            out->Extents[i].NextVcn.QuadPart = (ext->offset + ext->extent_data.decoded_size) >> Vcb->sector_shift;

            if (ext->extent_data.compression == BTRFS_COMPRESSION_NONE) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                out->Extents[i].Lcn.QuadPart = (ed2->address + ed2->offset) >> Vcb->sector_shift;
            } else
                out->Extents[i].Lcn.QuadPart = -1;

            outlen -= sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
            *retlen += sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
            i++;

            le = le->Flink;
        }

        if (num_sectors << Vcb->sector_shift > last_off) {
            if (outlen < sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            out->Extents[i].NextVcn.QuadPart = num_sectors;
            out->Extents[i].Lcn.QuadPart = -1;

            outlen -= sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
            *retlen += sizeof(LARGE_INTEGER) + sizeof(LARGE_INTEGER);
        }

        Status = STATUS_SUCCESS;
    } _SEH2_FINALLY {
        ExReleaseResourceLite(fcb->Header.Resource);
    } _SEH2_END;

    return Status;
}

static NTSTATUS add_csum_sparse_extents(device_extension* Vcb, uint64_t sparse_extents, uint8_t** ptr, bool found, void* hash_ptr) {
    if (!found) {
        uint8_t* sector = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.sector_size, ALLOC_TAG);

        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        memset(sector, 0, Vcb->superblock.sector_size);

        get_sector_csum(Vcb, sector, hash_ptr);

        ExFreePool(sector);
    }

    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C: {
            uint32_t* csum = (uint32_t*)*ptr;
            uint32_t sparse_hash = *(uint32_t*)hash_ptr;

            for (uint64_t i = 0; i < sparse_extents; i++) {
                csum[i] = sparse_hash;
            }

            break;
        }

        case CSUM_TYPE_XXHASH: {
            uint64_t* csum = (uint64_t*)*ptr;
            uint64_t sparse_hash = *(uint64_t*)hash_ptr;

            for (uint64_t i = 0; i < sparse_extents; i++) {
                csum[i] = sparse_hash;
            }

            break;
        }

        case CSUM_TYPE_SHA256:
        case CSUM_TYPE_BLAKE2: {
            uint8_t* csum = (uint8_t*)*ptr;

            for (uint64_t i = 0; i < sparse_extents; i++) {
                memcpy(csum, hash_ptr, 32);
                csum += 32;
            }

            break;
        }

        default:
            ERR("unrecognized hash type %x\n", Vcb->superblock.csum_type);
            return STATUS_INTERNAL_ERROR;
    }

    *ptr += sparse_extents * Vcb->csum_size;

    return STATUS_SUCCESS;
}

static NTSTATUS get_csum_info(device_extension* Vcb, PFILE_OBJECT FileObject, btrfs_csum_info* buf, ULONG buflen,
                              ULONG_PTR* retlen, KPROCESSOR_MODE processor_mode) {
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;

    TRACE("get_csum_info(%p, %p, %p, %lx, %p, %x)\n", Vcb, FileObject, buf, buflen, retlen, processor_mode);

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!fcb || !ccb)
        return STATUS_INVALID_PARAMETER;

    if (!buf)
        return STATUS_INVALID_PARAMETER;

    if (buflen < offsetof(btrfs_csum_info, data[0]))
        return STATUS_BUFFER_TOO_SMALL;


    if (processor_mode == UserMode && !(ccb->access & (FILE_READ_DATA | FILE_WRITE_DATA))) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    _SEH2_TRY {
        LIST_ENTRY* le;
        uint8_t* ptr;
        uint64_t last_off;
        uint8_t sparse_hash[MAX_HASH_SIZE];
        bool sparse_hash_found = false;

        if (fcb->ads) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        if (fcb->type == BTRFS_TYPE_DIRECTORY) {
            Status = STATUS_FILE_IS_A_DIRECTORY;
            _SEH2_LEAVE;
        }

        if (fcb->inode_item.flags & BTRFS_INODE_NODATASUM) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        buf->csum_type = Vcb->superblock.csum_type;
        buf->csum_length = Vcb->csum_size;

        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (ext->ignore) {
                le = le->Flink;
                continue;
            }

            if (ext->extent_data.type == EXTENT_TYPE_INLINE) {
                buf->num_sectors = 0;
                *retlen = offsetof(btrfs_csum_info, data[0]);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }

            le = le->Flink;
        }

        buf->num_sectors = (fcb->inode_item.st_size + Vcb->superblock.sector_size - 1) >> Vcb->sector_shift;

        if (buflen < offsetof(btrfs_csum_info, data[0]) + (buf->csum_length * buf->num_sectors)) {
            Status = STATUS_BUFFER_OVERFLOW;
            *retlen = offsetof(btrfs_csum_info, data[0]);
            _SEH2_LEAVE;
        }

        ptr = buf->data;
        last_off = 0;

        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            EXTENT_DATA2* ed2;

            if (ext->ignore || ext->extent_data.type == EXTENT_TYPE_INLINE) {
                le = le->Flink;
                continue;
            }

            if (ext->offset > last_off) {
                uint64_t sparse_extents = (ext->offset - last_off) >> Vcb->sector_shift;

                add_csum_sparse_extents(Vcb, sparse_extents, &ptr, sparse_hash_found, sparse_hash);
                sparse_hash_found = true;
            }

            ed2 = (EXTENT_DATA2*)ext->extent_data.data;

            if (ext->extent_data.compression != BTRFS_COMPRESSION_NONE)
                memset(ptr, 0, (ed2->num_bytes >> Vcb->sector_shift) * Vcb->csum_size); // dummy value for compressed extents
            else {
                if (ext->csum)
                    memcpy(ptr, ext->csum, (ed2->num_bytes >> Vcb->sector_shift) * Vcb->csum_size);
                else
                    memset(ptr, 0, (ed2->num_bytes >> Vcb->sector_shift) * Vcb->csum_size);

                ptr += (ed2->num_bytes >> Vcb->sector_shift) * Vcb->csum_size;
            }

            last_off = ext->offset + ed2->num_bytes;

            le = le->Flink;
        }

        if (buf->num_sectors > last_off >> Vcb->sector_shift) {
            uint64_t sparse_extents = buf->num_sectors - (last_off >> Vcb->sector_shift);

            add_csum_sparse_extents(Vcb, sparse_extents, &ptr, sparse_hash_found, sparse_hash);
        }

        *retlen = offsetof(btrfs_csum_info, data[0]) + (buf->csum_length * buf->num_sectors);
        Status = STATUS_SUCCESS;
    } _SEH2_FINALLY {
        ExReleaseResourceLite(fcb->Header.Resource);
    } _SEH2_END;

    return Status;
}

NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP* Pirp, uint32_t type) {
    PIRP Irp = *Pirp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    if (IrpSp->FileObject && IrpSp->FileObject->FsContext) {
        device_extension* Vcb = DeviceObject->DeviceExtension;

        if (Vcb->type == VCB_TYPE_FS)
            FsRtlCheckOplock(fcb_oplock(IrpSp->FileObject->FsContext), Irp, NULL, NULL, NULL);
    }

    switch (type) {
        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        case FSCTL_REQUEST_BATCH_OPLOCK:
        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
        case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
        case FSCTL_OPLOCK_BREAK_NOTIFY:
        case FSCTL_OPLOCK_BREAK_ACK_NO_2:
        case FSCTL_REQUEST_FILTER_OPLOCK:
#if (NTDDI_VERSION >= NTDDI_WIN7)
        case FSCTL_REQUEST_OPLOCK:
#endif
            Status = fsctl_oplock(DeviceObject->DeviceExtension, Pirp);
            break;

        case FSCTL_LOCK_VOLUME:
            Status = lock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_UNLOCK_VOLUME:
            Status = unlock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_DISMOUNT_VOLUME:
            Status = dismount_volume(DeviceObject->DeviceExtension, false, Irp);
            break;

        case FSCTL_IS_VOLUME_MOUNTED:
            Status = is_volume_mounted(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_IS_PATHNAME_VALID:
            WARN("STUB: FSCTL_IS_PATHNAME_VALID\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_MARK_VOLUME_DIRTY:
            WARN("STUB: FSCTL_MARK_VOLUME_DIRTY\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_QUERY_RETRIEVAL_POINTERS:
            WARN("STUB: FSCTL_QUERY_RETRIEVAL_POINTERS\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_COMPRESSION:
            Status = get_compression(Irp);
            break;

        case FSCTL_SET_COMPRESSION:
            Status = set_compression(Irp);
            break;

        case FSCTL_SET_BOOTLOADER_ACCESSED:
            WARN("STUB: FSCTL_SET_BOOTLOADER_ACCESSED\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_INVALIDATE_VOLUMES:
            Status = invalidate_volumes(Irp);
            break;

        case FSCTL_QUERY_FAT_BPB:
            WARN("STUB: FSCTL_QUERY_FAT_BPB\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_FILESYSTEM_GET_STATISTICS:
            Status = fs_get_statistics(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_GET_NTFS_VOLUME_DATA:
            WARN("STUB: FSCTL_GET_NTFS_VOLUME_DATA\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_NTFS_FILE_RECORD:
            WARN("STUB: FSCTL_GET_NTFS_FILE_RECORD\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_VOLUME_BITMAP:
            WARN("STUB: FSCTL_GET_VOLUME_BITMAP\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_RETRIEVAL_POINTERS:
            Status = get_retrieval_pointers(DeviceObject->DeviceExtension, IrpSp->FileObject,
                                            IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                            IrpSp->Parameters.FileSystemControl.InputBufferLength,
                                            Irp->UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                                            &Irp->IoStatus.Information);
            break;

        case FSCTL_MOVE_FILE:
            WARN("STUB: FSCTL_MOVE_FILE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_IS_VOLUME_DIRTY:
            Status = is_volume_dirty(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_ALLOW_EXTENDED_DASD_IO:
            Status = allow_extended_dasd_io(DeviceObject->DeviceExtension, IrpSp->FileObject);
            break;

        case FSCTL_FIND_FILES_BY_SID:
            WARN("STUB: FSCTL_FIND_FILES_BY_SID\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_OBJECT_ID:
            WARN("STUB: FSCTL_SET_OBJECT_ID\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_OBJECT_ID:
            Status = get_object_id(IrpSp->FileObject, Irp->UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                                   &Irp->IoStatus.Information);
            break;

        case FSCTL_DELETE_OBJECT_ID:
            WARN("STUB: FSCTL_DELETE_OBJECT_ID\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_REPARSE_POINT:
            Status = set_reparse_point(Irp);
            break;

        case FSCTL_GET_REPARSE_POINT:
            Status = get_reparse_point(IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                       IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_DELETE_REPARSE_POINT:
            Status = delete_reparse_point(Irp);
            break;

        case FSCTL_ENUM_USN_DATA:
            WARN("STUB: FSCTL_ENUM_USN_DATA\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SECURITY_ID_CHECK:
            WARN("STUB: FSCTL_SECURITY_ID_CHECK\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_READ_USN_JOURNAL:
            WARN("STUB: FSCTL_READ_USN_JOURNAL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_OBJECT_ID_EXTENDED:
            WARN("STUB: FSCTL_SET_OBJECT_ID_EXTENDED\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_CREATE_OR_GET_OBJECT_ID:
            Status = get_object_id(IrpSp->FileObject, Irp->UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                                   &Irp->IoStatus.Information);
            break;

        case FSCTL_SET_SPARSE:
            Status = set_sparse(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_SET_ZERO_DATA:
            Status = set_zero_data(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                   IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_QUERY_ALLOCATED_RANGES:
            Status = query_ranges(IrpSp->FileObject, IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                  IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->UserBuffer,
                                  IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_ENABLE_UPGRADE:
            WARN("STUB: FSCTL_ENABLE_UPGRADE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_ENCRYPTION:
            WARN("STUB: FSCTL_SET_ENCRYPTION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_ENCRYPTION_FSCTL_IO:
            WARN("STUB: FSCTL_ENCRYPTION_FSCTL_IO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_WRITE_RAW_ENCRYPTED:
            WARN("STUB: FSCTL_WRITE_RAW_ENCRYPTED\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_READ_RAW_ENCRYPTED:
            WARN("STUB: FSCTL_READ_RAW_ENCRYPTED\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_CREATE_USN_JOURNAL:
            WARN("STUB: FSCTL_CREATE_USN_JOURNAL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_READ_FILE_USN_DATA:
            WARN("STUB: FSCTL_READ_FILE_USN_DATA\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_WRITE_USN_CLOSE_RECORD:
            WARN("STUB: FSCTL_WRITE_USN_CLOSE_RECORD\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_EXTEND_VOLUME:
            WARN("STUB: FSCTL_EXTEND_VOLUME\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_QUERY_USN_JOURNAL:
            WARN("STUB: FSCTL_QUERY_USN_JOURNAL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_DELETE_USN_JOURNAL:
            WARN("STUB: FSCTL_DELETE_USN_JOURNAL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_MARK_HANDLE:
            WARN("STUB: FSCTL_MARK_HANDLE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SIS_COPYFILE:
            WARN("STUB: FSCTL_SIS_COPYFILE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SIS_LINK_FILES:
            WARN("STUB: FSCTL_SIS_LINK_FILES\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_RECALL_FILE:
            WARN("STUB: FSCTL_RECALL_FILE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_READ_FROM_PLEX:
            WARN("STUB: FSCTL_READ_FROM_PLEX\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_FILE_PREFETCH:
            WARN("STUB: FSCTL_FILE_PREFETCH\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

#if _WIN32_WINNT >= 0x0600
        case FSCTL_MAKE_MEDIA_COMPATIBLE:
            WARN("STUB: FSCTL_MAKE_MEDIA_COMPATIBLE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_DEFECT_MANAGEMENT:
            WARN("STUB: FSCTL_SET_DEFECT_MANAGEMENT\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_QUERY_SPARING_INFO:
            WARN("STUB: FSCTL_QUERY_SPARING_INFO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_QUERY_ON_DISK_VOLUME_INFO:
            WARN("STUB: FSCTL_QUERY_ON_DISK_VOLUME_INFO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_VOLUME_COMPRESSION_STATE:
            WARN("STUB: FSCTL_SET_VOLUME_COMPRESSION_STATE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_MODIFY_RM:
            WARN("STUB: FSCTL_TXFS_MODIFY_RM\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_QUERY_RM_INFORMATION:
            WARN("STUB: FSCTL_TXFS_QUERY_RM_INFORMATION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_ROLLFORWARD_REDO:
            WARN("STUB: FSCTL_TXFS_ROLLFORWARD_REDO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_ROLLFORWARD_UNDO:
            WARN("STUB: FSCTL_TXFS_ROLLFORWARD_UNDO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_START_RM:
            WARN("STUB: FSCTL_TXFS_START_RM\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_SHUTDOWN_RM:
            WARN("STUB: FSCTL_TXFS_SHUTDOWN_RM\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_READ_BACKUP_INFORMATION:
            WARN("STUB: FSCTL_TXFS_READ_BACKUP_INFORMATION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_WRITE_BACKUP_INFORMATION:
            WARN("STUB: FSCTL_TXFS_WRITE_BACKUP_INFORMATION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_CREATE_SECONDARY_RM:
            WARN("STUB: FSCTL_TXFS_CREATE_SECONDARY_RM\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_GET_METADATA_INFO:
            WARN("STUB: FSCTL_TXFS_GET_METADATA_INFO\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_GET_TRANSACTED_VERSION:
            WARN("STUB: FSCTL_TXFS_GET_TRANSACTED_VERSION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_SAVEPOINT_INFORMATION:
            WARN("STUB: FSCTL_TXFS_SAVEPOINT_INFORMATION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_CREATE_MINIVERSION:
            WARN("STUB: FSCTL_TXFS_CREATE_MINIVERSION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_TRANSACTION_ACTIVE:
            WARN("STUB: FSCTL_TXFS_TRANSACTION_ACTIVE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_ZERO_ON_DEALLOCATION:
            WARN("STUB: FSCTL_SET_ZERO_ON_DEALLOCATION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_REPAIR:
            WARN("STUB: FSCTL_SET_REPAIR\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_REPAIR:
            WARN("STUB: FSCTL_GET_REPAIR\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_WAIT_FOR_REPAIR:
            WARN("STUB: FSCTL_WAIT_FOR_REPAIR\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_INITIATE_REPAIR:
            WARN("STUB: FSCTL_INITIATE_REPAIR\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_CSC_INTERNAL:
            WARN("STUB: FSCTL_CSC_INTERNAL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SHRINK_VOLUME:
            WARN("STUB: FSCTL_SHRINK_VOLUME\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_SHORT_NAME_BEHAVIOR:
            WARN("STUB: FSCTL_SET_SHORT_NAME_BEHAVIOR\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_DFSR_SET_GHOST_HANDLE_STATE:
            WARN("STUB: FSCTL_DFSR_SET_GHOST_HANDLE_STATE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES:
            WARN("STUB: FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_LIST_TRANSACTIONS:
            WARN("STUB: FSCTL_TXFS_LIST_TRANSACTIONS\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_QUERY_PAGEFILE_ENCRYPTION:
            WARN("STUB: FSCTL_QUERY_PAGEFILE_ENCRYPTION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_RESET_VOLUME_ALLOCATION_HINTS:
            WARN("STUB: FSCTL_RESET_VOLUME_ALLOCATION_HINTS\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_TXFS_READ_BACKUP_INFORMATION2:
            WARN("STUB: FSCTL_TXFS_READ_BACKUP_INFORMATION2\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_CSV_CONTROL:
            WARN("STUB: FSCTL_CSV_CONTROL\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
#endif
        // TRACE rather than WARN because Windows 10 spams this undocumented fsctl
        case FSCTL_QUERY_VOLUME_CONTAINER_STATE:
            TRACE("STUB: FSCTL_QUERY_VOLUME_CONTAINER_STATE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_GET_INTEGRITY_INFORMATION:
            Status = get_integrity_information(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp, NormalPagePriority),
                                               IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_SET_INTEGRITY_INFORMATION:
            Status = set_integrity_information(IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength);
            break;

        case FSCTL_DUPLICATE_EXTENTS_TO_FILE:
            Status = duplicate_extents(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                       IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_GET_FILE_IDS:
            Status = get_file_ids(IrpSp->FileObject, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_CREATE_SUBVOL:
            Status = create_subvol(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_CREATE_SNAPSHOT:
            Status = create_snapshot(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_GET_INODE_INFO:
            Status = get_inode_info(IrpSp->FileObject, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_SET_INODE_INFO:
            Status = set_inode_info(IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_GET_DEVICES:
            Status = get_devices(DeviceObject->DeviceExtension, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_GET_USAGE:
            Status = get_usage(DeviceObject->DeviceExtension, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_START_BALANCE:
            Status = start_balance(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_QUERY_BALANCE:
            Status = query_balance(DeviceObject->DeviceExtension, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_PAUSE_BALANCE:
            Status = pause_balance(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_RESUME_BALANCE:
            Status = resume_balance(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_STOP_BALANCE:
            Status = stop_balance(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_ADD_DEVICE:
            Status = add_device(DeviceObject->DeviceExtension, Irp, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_REMOVE_DEVICE:
            Status = remove_device(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_GET_UUID:
            Status = query_uuid(DeviceObject->DeviceExtension, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_START_SCRUB:
            Status = start_scrub(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_QUERY_SCRUB:
            Status = query_scrub(DeviceObject->DeviceExtension, Irp->RequestorMode, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case FSCTL_BTRFS_PAUSE_SCRUB:
            Status = pause_scrub(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_RESUME_SCRUB:
            Status = resume_scrub(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_STOP_SCRUB:
            Status = stop_scrub(DeviceObject->DeviceExtension, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_RESET_STATS:
            Status = reset_stats(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_MKNOD:
            Status = mknod(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_RECEIVED_SUBVOL:
            Status = recvd_subvol(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                  IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_GET_XATTRS:
            Status = fsctl_get_xattrs(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_SET_XATTR:
            Status = fsctl_set_xattr(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                     IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_RESERVE_SUBVOL:
            Status = reserve_subvol(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp);
            break;

        case FSCTL_BTRFS_FIND_SUBVOL:
            Status = find_subvol(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength,
                                 Irp->UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_SEND_SUBVOL:
            Status = send_subvol(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength,
                                 IrpSp->FileObject, Irp);
            break;

        case FSCTL_BTRFS_READ_SEND_BUFFER:
            Status = read_send_buffer(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                                      &Irp->IoStatus.Information, Irp->RequestorMode);
            break;

        case FSCTL_BTRFS_RESIZE:
            Status = resize_device(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer,
                                   IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp);
            break;

        case FSCTL_BTRFS_GET_CSUM_INFO:
            Status = get_csum_info(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                   IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information,
                                   Irp->RequestorMode);
            break;

        default:
            WARN("unknown control code %lx (DeviceType = %lx, Access = %lx, Function = %lx, Method = %lx)\n",
                          IrpSp->Parameters.FileSystemControl.FsControlCode, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xff0000) >> 16,
                          (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xc000) >> 14, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3ffc) >> 2,
                          IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return Status;
}
