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
#ifndef __REACTOS__
#include <winioctl.h>
#endif

#ifndef FSCTL_CSV_CONTROL
#define FSCTL_CSV_CONTROL CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 181, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define DOTDOT ".."

#define SEF_AVOID_PRIVILEGE_CHECK 0x08 // on MSDN but not in any header files(?)

extern LIST_ENTRY VcbList;
extern ERESOURCE global_loading_lock;
extern LIST_ENTRY volumes;
extern ERESOURCE volumes_lock;

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
    bgfi->top = fcb->Vcb->root_fileref->fcb == fcb ? TRUE : FALSE;
    
    return STATUS_SUCCESS;
}

static void get_uuid(BTRFS_UUID* uuid) {
    LARGE_INTEGER seed;
    UINT8 i;
    
    seed = KeQueryPerformanceCounter(NULL);

    for (i = 0; i < 16; i+=2) {
        ULONG rand = RtlRandomEx(&seed.LowPart);
        
        uuid->uuid[i] = (rand & 0xff00) >> 8;
        uuid->uuid[i+1] = rand & 0xff;
    }
}

static NTSTATUS snapshot_tree_copy(device_extension* Vcb, UINT64 addr, root* subvol, UINT64* newaddr, PIRP Irp, LIST_ENTRY* rollback) {
    UINT8* buf;
    NTSTATUS Status;
    write_data_context* wtc;
    LIST_ENTRY* le;
    tree t;
    tree_header* th;
    chunk* c;
    
    buf = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        ExFreePool(buf);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_data(Vcb, addr, Vcb->superblock.node_size, NULL, TRUE, buf, NULL, NULL, Irp, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("read_data returned %08x\n", Status);
        goto end;
    }
    
    th = (tree_header*)buf;
    
    RtlZeroMemory(&t, sizeof(tree));
    t.root = subvol;
    t.header.level = th->level;
    t.header.tree_id = t.root->id;
    
    Status = get_tree_new_address(Vcb, &t, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("get_tree_new_address returned %08x\n", Status);
        goto end;
    }
    
    if (!t.has_new_address) {
        ERR("tree new address not set\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    c = get_chunk_from_address(Vcb, t.new_address);
            
    if (c) {
        increase_chunk_usage(c, Vcb->superblock.node_size);
    } else {
        ERR("could not find chunk for address %llx\n", t.new_address);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    th->address = t.new_address;
    th->tree_id = subvol->id;
    th->generation = Vcb->superblock.generation;
    th->fs_uuid = Vcb->superblock.uuid;
    
    if (th->level == 0) {
        UINT32 i;
        leaf_node* ln = (leaf_node*)&th[1];
        
        for (i = 0; i < th->num_items; i++) {
            if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) && ln[i].offset + ln[i].size <= Vcb->superblock.node_size - sizeof(tree_header)) {
                EXTENT_DATA* ed = (EXTENT_DATA*)(((UINT8*)&th[1]) + ln[i].offset);
                
                if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];
                    
                    if (ed2->size != 0) { // not sparse
                        Status = increase_extent_refcount_data(Vcb, ed2->address, ed2->size, subvol->id, ln[i].key.obj_id, ln[i].key.offset - ed2->offset, 1, Irp, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("increase_extent_refcount_data returned %08x\n", Status);
                            goto end;
                        }
                    }
                }
            }
        }
    } else {
        UINT32 i;
        internal_node* in = (internal_node*)&th[1];
        
        for (i = 0; i < th->num_items; i++) {
            TREE_BLOCK_REF tbr;
            
            tbr.offset = subvol->id;
            
            Status = increase_extent_refcount(Vcb, in[i].address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr, NULL, th->level - 1, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("increase_extent_refcount returned %08x\n", Status);
                goto end;
            }
        }
    }
    
    *((UINT32*)buf) = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
    
    KeInitializeEvent(&wtc->Event, NotificationEvent, FALSE);
    InitializeListHead(&wtc->stripes);
    wtc->tree = TRUE;
    wtc->stripes_left = 0;
    
    Status = write_data(Vcb, t.new_address, buf, FALSE, Vcb->superblock.node_size, wtc, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08x\n", Status);
        goto end;
    }
    
    if (wtc->stripes.Flink != &wtc->stripes) {
        // launch writes and wait
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore)
                IoCallDriver(stripe->device->devobj, stripe->Irp);
            
            le = le->Flink;
        }
        
        KeWaitForSingleObject(&wtc->Event, Executive, KernelMode, FALSE, NULL);
        
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                break;
            }
            
            le = le->Flink;
        }
        
        free_write_data_stripes(wtc);
        buf = NULL;
    }
    
    if (NT_SUCCESS(Status))
        *newaddr = t.new_address;
    
end:
    ExFreePool(wtc);
    
    if (buf)
        ExFreePool(buf);
    
    return Status;
}

static void flush_subvol_fcbs(root* subvol, LIST_ENTRY* rollback) {
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

static NTSTATUS do_create_snapshot(device_extension* Vcb, PFILE_OBJECT parent, fcb* subvol_fcb, PANSI_STRING utf8, PUNICODE_STRING name, PIRP Irp) {
    LIST_ENTRY rollback;
    UINT64 id;
    NTSTATUS Status;
    root *r, *subvol = subvol_fcb->subvol;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 address, dirpos, *root_num;
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
    
    InitializeListHead(&rollback);
    
    // flush open files on this subvol
    
    flush_subvol_fcbs(subvol, &rollback);

    // flush metadata
    
    if (Vcb->need_write)
        do_write(Vcb, Irp, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    InitializeListHead(&rollback);
    
    // create new root
    
    id = InterlockedIncrement64(&Vcb->root_root->lastinode);
    Status = create_root(Vcb, id, &r, TRUE, Vcb->superblock.generation, Irp, &rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08x\n", Status);
        goto end;
    }
    
    r->lastinode = subvol->lastinode;
    
    if (!Vcb->uuid_root) {
        root* uuid_root;
        
        TRACE("uuid root doesn't exist, creating it\n");
        
        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, FALSE, 0, Irp, &rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("create_root returned %08x\n", Status);
            goto end;
        }
        
        Vcb->uuid_root = uuid_root;
    }
    
    root_num = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64), ALLOC_TAG);
    if (!root_num) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    tp.tree = NULL;
    
    do {
        get_uuid(&r->root_item.uuid);
        
        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid, sizeof(UINT64));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(UINT64)], sizeof(UINT64));
        
        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
    } while (NT_SUCCESS(Status) && !keycmp(searchkey, tp.item->key));
    
    *root_num = r->id;
    
    if (!insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(UINT64), NULL, Irp, &rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(root_num);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    searchkey.obj_id = r->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    Status = snapshot_tree_copy(Vcb, subvol->root_item.block_number, r, &address, Irp, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("snapshot_tree_copy returned %08x\n", Status);
        goto end;
    }
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    r->root_item.inode.generation = 1;
    r->root_item.inode.st_size = 3;
    r->root_item.inode.st_blocks = subvol->root_item.inode.st_blocks;
    r->root_item.inode.st_nlink = 1;
    r->root_item.inode.st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    r->root_item.inode.flags = 0xffffffff80000000; // FIXME - find out what these mean
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
    
    r->treeholder.address = address;
    
    // FIXME - do we need to copy over the send and receive fields too?
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("error - could not find ROOT_ITEM for subvol %llx\n", r->id);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    RtlCopyMemory(tp.item->data, &r->root_item, sizeof(ROOT_ITEM));
    
    // update ROOT_ITEM of original subvol
    
    subvol->root_item.last_snapshot_generation = Vcb->superblock.generation;
    
    // We also rewrite the top of the old subvolume tree, for some reason
    searchkey.obj_id = 0;
    searchkey.obj_type = 0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    subvol->treeholder.tree->write = TRUE;
    
    // create fileref for entry in other subvolume
    
    fr = create_fileref();
    if (!fr) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fr->utf8.Length = fr->utf8.MaximumLength = utf8->Length;
    fr->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, fr->utf8.MaximumLength, ALLOC_TAG);
    if (!fr->utf8.Buffer) {
        ERR("out of memory\n");
        free_fileref(fr);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(fr->utf8.Buffer, utf8->Buffer, utf8->Length);
    
    Status = open_fcb(Vcb, r, r->root_item.objid, BTRFS_TYPE_DIRECTORY, utf8, fcb, &fr->fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        free_fileref(fr);
        goto end;
    }
    
    Status = fcb_get_last_dir_index(fcb, &dirpos, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_last_dir_index returned %08x\n", Status);
        free_fileref(fr);
        goto end;
    }
    
    fr->index = dirpos;
    
    fr->filepart.MaximumLength = fr->filepart.Length = name->Length;
    
    fr->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fr->filepart.MaximumLength, ALLOC_TAG);
    if (!fr->filepart.Buffer) {
        ERR("out of memory\n");
        free_fileref(fr);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(fr->filepart.Buffer, name->Buffer, name->Length);
    
    Status = RtlUpcaseUnicodeString(&fr->filepart_uc, &fr->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        free_fileref(fr);
        goto end;
    }
    
    fr->parent = fileref;
    
    Status = add_dir_child(fileref->fcb, r->id, TRUE, dirpos, utf8, &fr->filepart, &fr->filepart_uc, BTRFS_TYPE_DIRECTORY, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);
    
    fr->dc = dc;
    dc->fileref = fr;
    
    insert_fileref_child(fileref, fr, TRUE);
    increase_fileref_refcount(fileref);
    
    fr->created = TRUE;
    mark_fileref_dirty(fr);
    
    if (fr->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr->fcb->fileref = fr;
    
    free_fileref(fr);

    // change fcb's INODE_ITEM
    
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    fcb->inode_item.st_size += utf8->Length * 2;
    
    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;
    
    if (!ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    fcb->subvol->root_item.ctime = now;
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    
    send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED);
    send_notification_fileref(fr->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    
    le = subvol->fcbs.Flink;
    while (le != &subvol->fcbs) {
        struct _fcb* fcb2 = CONTAINING_RECORD(le, struct _fcb, list_entry);
        LIST_ENTRY* le2 = fcb2->extents.Flink;
        
        while (le2 != &fcb2->extents) {
            extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
            
            if (!ext->ignore)
                ext->unique = FALSE;
            
            le2 = le2->Flink;
        }
        
        le = le->Flink;
    }
    
    do_write(Vcb, Irp, &rollback);
    
    free_trees(Vcb);
    
    Status = STATUS_SUCCESS;
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);

    return Status;
}

static NTSTATUS create_snapshot(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG length, PIRP Irp) {
    PFILE_OBJECT subvol_obj;
    NTSTATUS Status;
    btrfs_create_snapshot* bcs = data;
    fcb* subvol_fcb;
    ANSI_STRING utf8;
    UNICODE_STRING nameus;
    ULONG len;
    fcb* fcb;
    ccb* ccb;
    file_ref *fileref, *fr2;
    
    if (length < offsetof(btrfs_create_snapshot, name))
        return STATUS_INVALID_PARAMETER;
    
    if (length < offsetof(btrfs_create_snapshot, name) + bcs->namelen)
        return STATUS_INVALID_PARAMETER;
    
    if (!bcs->subvol)
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
    
    if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY)
        return STATUS_ACCESS_DENIED;
    
    nameus.Buffer = bcs->name;
    nameus.Length = nameus.MaximumLength = bcs->namelen;
    
    if (!is_file_name_valid(&nameus))
        return STATUS_OBJECT_NAME_INVALID;
    
    utf8.Buffer = NULL;
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &len, bcs->name, bcs->namelen);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N failed with error %08x\n", Status);
        return Status;
    }
    
    if (len == 0) {
        ERR("RtlUnicodeToUTF8N returned a length of 0\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    utf8.MaximumLength = utf8.Length = len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);
    
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = RtlUnicodeToUTF8N(utf8.Buffer, len, &len, bcs->name, bcs->namelen);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N failed with error %08x\n", Status);
        goto end2;
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    // no need for fcb_lock as we have tree_lock exclusively
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, FALSE, NULL, NULL, PagedPool, FALSE, Irp);
    
    if (NT_SUCCESS(Status)) {
        if (!fr2->deleted) {
            WARN("file already exists\n");
            free_fileref(fr2);
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end3;
        } else
            free_fileref(fr2);
    } else if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("open_fileref returned %08x\n", Status);
        goto end3;
    }
    
    Status = ObReferenceObjectByHandle(bcs->subvol, 0, *IoFileObjectType, UserMode, (void**)&subvol_obj, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08x\n", Status);
        goto end3;
    }
    
    subvol_fcb = subvol_obj->FsContext;
    if (!subvol_fcb) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (subvol_fcb->inode != subvol_fcb->subvol->root_item.objid) {
        WARN("handle inode was %llx, expected %llx\n", subvol_fcb->inode, subvol_fcb->subvol->root_item.objid);
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
    
    // clear unique flag on extents of open files in subvol
    if (!IsListEmpty(&subvol_fcb->subvol->fcbs)) {
        LIST_ENTRY* le = subvol_fcb->subvol->fcbs.Flink;
        
        while (le != &subvol_fcb->subvol->fcbs) {
            struct _fcb* openfcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
            LIST_ENTRY* le2;
            
            le2 = openfcb->extents.Flink;
            
            while (le2 != &openfcb->extents) {
                extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
                
                ext->unique = FALSE;
                
                le2 = le2->Flink;
            }
            
            le = le->Flink;
        }
    }
    
    Status = do_create_snapshot(Vcb, FileObject, subvol_fcb, &utf8, &nameus, Irp);
    
    if (NT_SUCCESS(Status)) {
        file_ref* fr;

        Status = open_fileref(Vcb, &fr, &nameus, fileref, FALSE, NULL, NULL, PagedPool, FALSE, Irp);
        
        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08x\n", Status);
            Status = STATUS_SUCCESS;
        } else {
            send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED);
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

static NTSTATUS create_subvol(device_extension* Vcb, PFILE_OBJECT FileObject, WCHAR* name, ULONG length, PIRP Irp) {
    fcb *fcb, *rootfcb;
    ccb* ccb;
    file_ref* fileref;
    NTSTATUS Status;
    LIST_ENTRY rollback;
    UINT64 id;
    root* r;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ULONG len, irsize;
    UNICODE_STRING nameus;
    ANSI_STRING utf8;
    UINT64 dirpos;
    INODE_REF* ir;
    KEY searchkey;
    traverse_ptr tp;
    SECURITY_SUBJECT_CONTEXT subjcont;
    PSID owner;
    BOOLEAN defaulted;
    UINT64* root_num;
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
    
    if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY)
        return STATUS_ACCESS_DENIED;
    
    nameus.Length = nameus.MaximumLength = length;
    nameus.Buffer = name;
    
    if (!is_file_name_valid(&nameus))
        return STATUS_OBJECT_NAME_INVALID;
    
    utf8.Buffer = NULL;
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &len, name, length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N failed with error %08x\n", Status);
        return Status;
    }
    
    if (len == 0) {
        ERR("RtlUnicodeToUTF8N returned a length of 0\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    utf8.MaximumLength = utf8.Length = len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);
    
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = RtlUnicodeToUTF8N(utf8.Buffer, len, &len, name, length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N failed with error %08x\n", Status);
        goto end2;
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    InitializeListHead(&rollback);
    
    // no need for fcb_lock as we have tree_lock exclusively
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, FALSE, NULL, NULL, PagedPool, FALSE, Irp);
    
    if (NT_SUCCESS(Status)) {
        if (!fr2->deleted) {
            WARN("file already exists\n");
            free_fileref(fr2);
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        } else
            free_fileref(fr2);
    } else if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("open_fileref returned %08x\n", Status);
        goto end;
    }
    
    // FIXME - make sure rollback removes new roots from internal structures
    
    id = InterlockedIncrement64(&Vcb->root_root->lastinode);
    Status = create_root(Vcb, id, &r, FALSE, 0, Irp, &rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08x\n", Status);
        goto end;
    }
    
    TRACE("created root %llx\n", id);
    
    if (!Vcb->uuid_root) {
        root* uuid_root;
        
        TRACE("uuid root doesn't exist, creating it\n");
        
        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, FALSE, 0, Irp, &rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("create_root returned %08x\n", Status);
            goto end;
        }
        
        Vcb->uuid_root = uuid_root;
    }
    
    root_num = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64), ALLOC_TAG);
    if (!root_num) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    tp.tree = NULL;
    
    do {
        get_uuid(&r->root_item.uuid);
        
        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid, sizeof(UINT64));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(UINT64)], sizeof(UINT64));
        
        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
    } while (NT_SUCCESS(Status) && !keycmp(searchkey, tp.item->key));
    
    *root_num = r->id;
    
    if (!insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(UINT64), NULL, Irp, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    r->root_item.inode.generation = 1;
    r->root_item.inode.st_size = 3;
    r->root_item.inode.st_blocks = Vcb->superblock.node_size;
    r->root_item.inode.st_nlink = 1;
    r->root_item.inode.st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    r->root_item.inode.flags = 0xffffffff80000000; // FIXME - find out what these mean
    
    r->root_item.objid = SUBVOL_ROOT_INODE;
    r->root_item.bytes_used = Vcb->superblock.node_size;
    r->root_item.ctransid = Vcb->superblock.generation;
    r->root_item.otransid = Vcb->superblock.generation;
    r->root_item.ctime = now;
    r->root_item.otime = now;
    
    // add .. inode to new subvol
    
    rootfcb = create_fcb(PagedPool);
    if (!rootfcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    rootfcb->Vcb = Vcb;
    
    rootfcb->subvol = r;
    rootfcb->inode = SUBVOL_ROOT_INODE;
    rootfcb->type = BTRFS_TYPE_DIRECTORY;
    
    rootfcb->inode_item.generation = Vcb->superblock.generation;
    rootfcb->inode_item.transid = Vcb->superblock.generation;
    rootfcb->inode_item.st_nlink = 1;
    rootfcb->inode_item.st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    rootfcb->inode_item.st_atime = rootfcb->inode_item.st_ctime = rootfcb->inode_item.st_mtime = rootfcb->inode_item.otime = now;
    rootfcb->inode_item.st_gid = GID_NOBODY; // FIXME?
    
    rootfcb->atts = get_file_attributes(Vcb, &rootfcb->inode_item, rootfcb->subvol, rootfcb->inode, rootfcb->type, FALSE, TRUE, Irp);
    
    SeCaptureSubjectContext(&subjcont);
    
    Status = SeAssignSecurity(fcb->sd, NULL, (void**)&rootfcb->sd, TRUE, &subjcont, IoGetFileObjectGenericMapping(), PagedPool);
    
    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurity returned %08x\n", Status);
        goto end;
    }
    
    if (!rootfcb->sd) {
        ERR("SeAssignSecurity returned NULL security descriptor\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    Status = RtlGetOwnerSecurityDescriptor(rootfcb->sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetOwnerSecurityDescriptor returned %08x\n", Status);
        rootfcb->inode_item.st_uid = UID_NOBODY;
    } else {
        rootfcb->inode_item.st_uid = sid_to_uid(owner);
    }
    
    rootfcb->sd_dirty = TRUE;
    rootfcb->inode_item_changed = TRUE;

    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    InsertTailList(&r->fcbs, &rootfcb->list_entry);
    InsertTailList(&Vcb->all_fcbs, &rootfcb->list_entry_all);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    rootfcb->Header.IsFastIoPossible = fast_io_possible(rootfcb);
    rootfcb->Header.AllocationSize.QuadPart = 0;
    rootfcb->Header.FileSize.QuadPart = 0;
    rootfcb->Header.ValidDataLength.QuadPart = 0;
    
    rootfcb->created = TRUE;
    
    r->lastinode = rootfcb->inode;
    
    // add INODE_REF
    
    irsize = sizeof(INODE_REF) - 1 + strlen(DOTDOT);
    ir = ExAllocatePoolWithTag(PagedPool, irsize, ALLOC_TAG);
    if (!ir) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    ir->index = 0;
    ir->n = strlen(DOTDOT);
    RtlCopyMemory(ir->name, DOTDOT, ir->n);

    if (!insert_tree_item(Vcb, r, r->root_item.objid, TYPE_INODE_REF, r->root_item.objid, ir, irsize, NULL, Irp, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // create fileref for entry in other subvolume
    
    fr = create_fileref();
    if (!fr) {
        ERR("out of memory\n");
        
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fcb(rootfcb);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fr->fcb = rootfcb;
    
    mark_fcb_dirty(rootfcb);
    
    Status = fcb_get_last_dir_index(fcb, &dirpos, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_last_dir_index returned %08x\n", Status);
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        goto end;
    }
    
    fr->index = dirpos;
    fr->utf8 = utf8;
    
    fr->filepart.MaximumLength = fr->filepart.Length = nameus.Length;
    fr->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fr->filepart.MaximumLength, ALLOC_TAG);
    if (!fr->filepart.Buffer) {
        ERR("out of memory\n");
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(fr->filepart.Buffer, nameus.Buffer, nameus.Length);
    
    Status = RtlUpcaseUnicodeString(&fr->filepart_uc, &fr->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        goto end;
    }
    
    fr->parent = fileref;
    
    Status = add_dir_child(fileref->fcb, r->id, TRUE, dirpos, &utf8, &fr->filepart, &fr->filepart_uc, BTRFS_TYPE_DIRECTORY, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);
    
    fr->dc = dc;
    dc->fileref = fr;
    
    fr->fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fr->fcb->hash_ptrs) {
        ERR("out of memory\n");
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(fr->fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);
    
    fr->fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!fcb->hash_ptrs_uc) {
        ERR("out of memory\n");
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(fr->fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);
    
    insert_fileref_child(fileref, fr, TRUE);
    increase_fileref_refcount(fileref);
    
    if (fr->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr->fcb->fileref = fr;
    
    fr->created = TRUE;
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
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    Status = STATUS_SUCCESS;    
    
end:
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(Vcb, &rollback);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    if (NT_SUCCESS(Status)) {
        send_notification_fileref(fr, FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED);
        send_notification_fileref(fr->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    }
    
end2:
    if (fr) {
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        free_fileref(fr);
        ExReleaseResourceLite(&Vcb->fcb_lock);
    }
        
    return Status;
}

static NTSTATUS get_inode_info(PFILE_OBJECT FileObject, void* data, ULONG length) {
    btrfs_inode_info* bii = data;
    fcb* fcb;
    ccb* ccb;
    
    if (length < sizeof(btrfs_inode_info))
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
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    bii->subvol = fcb->subvol->id;
    bii->inode = fcb->inode;
    bii->top = fcb->Vcb->root_fileref->fcb == fcb ? TRUE : FALSE;
    bii->type = fcb->type;
    bii->st_uid = fcb->inode_item.st_uid;
    bii->st_gid = fcb->inode_item.st_gid;
    bii->st_mode = fcb->inode_item.st_mode;
    bii->st_rdev = fcb->inode_item.st_rdev;
    bii->flags = fcb->inode_item.flags;
    
    bii->inline_length = 0;
    bii->disk_size[0] = 0;
    bii->disk_size[1] = 0;
    bii->disk_size[2] = 0;
    
    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        LIST_ENTRY* le;
        
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            
            if (!ext->ignore) {
                if (ext->data->type == EXTENT_TYPE_INLINE) {
                    bii->inline_length += ext->data->decoded_size;
                } else {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                    
                    // FIXME - compressed extents with a hole in them are counted more than once
                    if (ed2->size != 0) {
                        if (ext->data->compression == BTRFS_COMPRESSION_NONE) {
                            bii->disk_size[0] += ed2->num_bytes;
                        } else if (ext->data->compression == BTRFS_COMPRESSION_ZLIB) {
                            bii->disk_size[1] += ed2->size;
                        } else if (ext->data->compression == BTRFS_COMPRESSION_LZO) {
                            bii->disk_size[2] += ed2->size;
                        }
                    }
                }
            }
            
            le = le->Flink;
        }
    }
    
    ExReleaseResourceLite(fcb->Header.Resource);
    
    return STATUS_SUCCESS;
}

static NTSTATUS set_inode_info(PFILE_OBJECT FileObject, void* data, ULONG length) {
    btrfs_set_inode_info* bsii = data;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    
    if (length < sizeof(btrfs_set_inode_info))
        return STATUS_BUFFER_OVERFLOW;
    
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
    
    if (bsii->mode_changed && !(ccb->access & WRITE_DAC)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }
    
    if ((bsii->uid_changed || bsii->gid_changed) && !(ccb->access & WRITE_OWNER)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
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
            fcb->inode_item.flags &= ~(UINT64)BTRFS_INODE_NODATASUM;
    }
    
    if (bsii->mode_changed) {
        UINT32 allowed = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
        
        fcb->inode_item.st_mode &= ~allowed;
        fcb->inode_item.st_mode |= bsii->st_mode & allowed;
    }
    
    if (bsii->uid_changed) {
        PSID sid; 
        SECURITY_INFORMATION secinfo;
        SECURITY_DESCRIPTOR sd;
        void* oldsd;
        
        fcb->inode_item.st_uid = bsii->st_uid;
        
        uid_to_sid(bsii->st_uid, &sid);
        
        Status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlCreateSecurityDescriptor returned %08x\n", Status);
            goto end;
        }
        
        Status = RtlSetOwnerSecurityDescriptor(&sd, sid, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlSetOwnerSecurityDescriptor returned %08x\n", Status);
            goto end;
        }
        
        oldsd = fcb->sd;
        
        secinfo = OWNER_SECURITY_INFORMATION;
        Status = SeSetSecurityDescriptorInfoEx(NULL, &secinfo, &sd, (void**)&fcb->sd, SEF_AVOID_PRIVILEGE_CHECK, PagedPool, IoGetFileObjectGenericMapping());
        
        if (!NT_SUCCESS(Status)) {
            ERR("SeSetSecurityDescriptorInfo returned %08x\n", Status);
            goto end;
        }
        
        ExFreePool(oldsd);
        
        fcb->sd_dirty = TRUE;
        
        send_notification_fcb(ccb->fileref, FILE_NOTIFY_CHANGE_SECURITY, FILE_ACTION_MODIFIED);
    }
    
    if (bsii->gid_changed)
        fcb->inode_item.st_gid = bsii->st_gid;
    
    if (bsii->flags_changed || bsii->mode_changed || bsii->uid_changed || bsii->gid_changed) {
        fcb->inode_item_changed = TRUE;
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
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
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
            dev = (btrfs_device*)((UINT8*)dev + dev->next_entry);
        }
        
        structlen = length - offsetof(btrfs_device, namelen);
        
        Status = dev_ioctl(dev2->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &dev->namelen, structlen, TRUE, NULL);
        if (!NT_SUCCESS(Status))
            goto end;
        
        dev->next_entry = 0;
        dev->dev_id = dev2->devitem.dev_id;
        dev->size = dev2->length;
        dev->readonly = (Vcb->readonly || dev2->readonly) ? TRUE : FALSE;
        dev->device_number = dev2->disk_num;
        dev->partition_number = dev2->part_num;
        
        length -= sizeof(btrfs_device) - sizeof(WCHAR) + dev->namelen;
        
        le = le->Flink;
    }

end:
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static NTSTATUS get_usage(device_extension* Vcb, void* data, ULONG length) {
    btrfs_usage* usage = (btrfs_usage*)data;
    btrfs_usage* lastbue = NULL;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    if (length < sizeof(btrfs_usage))
        return STATUS_BUFFER_OVERFLOW;
    
    length -= offsetof(btrfs_usage, devices);
    
    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        BOOL addnew = FALSE;
        
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (!lastbue) // first entry
            addnew = TRUE;
        else {
            btrfs_usage* bue = usage;
            
            addnew = TRUE;
            
            while (TRUE) {
                if (bue->type == c->chunk_item->type) {
                    addnew = FALSE;
                    break;
                }
                
                if (bue->next_entry == 0)
                    break;
                else
                    bue = (btrfs_usage*)((UINT8*)bue + bue->next_entry);
            }
        }
        
        if (addnew) {
            btrfs_usage* bue;
            LIST_ENTRY* le2;
            UINT64 factor;
            
            if (!lastbue) {
                bue = usage;
            } else {
                if (length < offsetof(btrfs_usage, devices)) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto end;
                }
                
                length -= offsetof(btrfs_usage, devices);
                
                lastbue->next_entry = offsetof(btrfs_usage, devices) + (lastbue->num_devices * sizeof(btrfs_usage_device));
                
                bue = (btrfs_usage*)((UINT8*)lastbue + lastbue->next_entry);
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
                    UINT16 i;
                    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c2->chunk_item[1];
                    UINT64 stripesize;
                    
                    bue->size += c2->chunk_item->size;
                    bue->used += c2->used;
                    
                    stripesize = c2->chunk_item->size / factor;
                    
                    for (i = 0; i < c2->chunk_item->num_stripes; i++) {
                        UINT64 j;
                        BOOL found = FALSE;
                        
                        for (j = 0; j < bue->num_devices; j++) {
                            if (bue->devices[j].dev_id == cis[i].dev_id) {
                                bue->devices[j].alloc += stripesize;
                                found = TRUE;
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
    BOOL verify = FALSE;
    LIST_ENTRY* le;
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        
        if (dev->devobj && dev->removable) {
            Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), FALSE, &iosb);
            
            if (iosb.Information != sizeof(ULONG))
                cc = 0;
            
            if (Status == STATUS_VERIFY_REQUIRED || (NT_SUCCESS(Status) && cc != dev->change_count)) {
                dev->devobj->Flags |= DO_VERIFY_VOLUME;
                verify = TRUE;
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

static NTSTATUS fs_get_statistics(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject, void* buffer, DWORD buflen, ULONG_PTR* retlen) {
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
    BOOL set;
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
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("FileObject did not point to a file\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fssb)
        set = fssb->SetSparse;
    else
        set = TRUE;
    
    if (set) {
        fcb->atts |= FILE_ATTRIBUTE_SPARSE_FILE;
        fcb->atts_changed = TRUE;
    } else {
        ULONG defda;
        
        fcb->atts &= ~FILE_ATTRIBUTE_SPARSE_FILE;
        fcb->atts_changed = TRUE;
        
        defda = get_file_attributes(Vcb, &fcb->inode_item, fcb->subvol, fcb->inode, fcb->type,
                                    fileref && fileref->filepart.Length > 0 && fileref->filepart.Buffer[0] == '.', TRUE, Irp);
        
        fcb->atts_deleted = defda == fcb->atts;
    }
    
    mark_fcb_dirty(fcb);
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED);
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static NTSTATUS zero_data(device_extension* Vcb, fcb* fcb, UINT64 start, UINT64 length, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    BOOL compress = write_fcb_compressed(fcb);
    UINT64 start_data, end_data;
    UINT8* data;
        
    if (compress) {
        start_data = start & ~(UINT64)(COMPRESSED_EXTENT_SIZE - 1);
        end_data = min(sector_align(start + length, COMPRESSED_EXTENT_SIZE),
                       sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size));
    } else {
        start_data = start & ~(UINT64)(fcb->Vcb->superblock.sector_size - 1);
        end_data = sector_align(start + length, fcb->Vcb->superblock.sector_size);
    }

    data = ExAllocatePoolWithTag(PagedPool, end_data - start_data, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(data, end_data - start_data);
    
    if (start > start_data || start + length < end_data) {
        Status = read_file(fcb, data, start_data, end_data - start_data, NULL, Irp, TRUE);
        
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08x\n", Status);
            ExFreePool(data);
            return Status;
        }
    }
    
    RtlZeroMemory(data + start - start_data, length);
    
    if (compress) {
        Status = write_compressed(fcb, start_data, end_data, data, Irp, rollback);
        
        ExFreePool(data);
        
        if (!NT_SUCCESS(Status)) {
            ERR("write_compressed returned %08x\n", Status);
            return Status;
        }
    } else {
        Status = do_write_file(fcb, start_data, end_data, data, Irp, rollback);
        
        ExFreePool(data);
        
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_file returned %08x\n", Status);
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
    UINT64 start, end;
    extent* ext;
    IO_STATUS_BLOCK iosb;
    
    if (!data || length < sizeof(FILE_ZERO_DATA_INFORMATION))
        return STATUS_INVALID_PARAMETER;
    
    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    if (fzdi->BeyondFinalZero.QuadPart <= fzdi->FileOffset.QuadPart) {
        WARN("BeyondFinalZero was less than or equal to FileOffset (%llx <= %llx)\n", fzdi->BeyondFinalZero.QuadPart, fzdi->FileOffset.QuadPart);
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
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    CcFlushCache(&fcb->nonpaged->segment_object, NULL, 0, &iosb);
    
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
    
    if (fzdi->FileOffset.QuadPart >= fcb->inode_item.st_size) {
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
    
    if (ext->datalen >= sizeof(EXTENT_DATA) && ext->data->type == EXTENT_TYPE_INLINE) {
        Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("zero_data returned %08x\n", Status);
            goto end;
        }
    } else {
        start = sector_align(fzdi->FileOffset.QuadPart, Vcb->superblock.sector_size);
        
        if (fzdi->BeyondFinalZero.QuadPart > fcb->inode_item.st_size)
            end = sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size);
        else
            end = (fzdi->BeyondFinalZero.QuadPart / Vcb->superblock.sector_size) * Vcb->superblock.sector_size;
        
        if (end <= start) {
            Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, Irp, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("zero_data returned %08x\n", Status);
                goto end;
            }
        } else {
            if (start > fzdi->FileOffset.QuadPart) {
                Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, start - fzdi->FileOffset.QuadPart, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("zero_data returned %08x\n", Status);
                    goto end;
                }
            }
            
            if (end < fzdi->BeyondFinalZero.QuadPart) {
                Status = zero_data(Vcb, fcb, end, fzdi->BeyondFinalZero.QuadPart - end, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("zero_data returned %08x\n", Status);
                    goto end;
                }
            }
            
            if (end > start) {
                Status = excise_extents(Vcb, fcb, start, end, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("excise_extents returned %08x\n", Status);
                    goto end;
                }
            }
        }
    }
    
    CcPurgeCacheSection(&fcb->nonpaged->segment_object, &fzdi->FileOffset, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, FALSE);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    
    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;
    
    if (!ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;
    
    fcb->extents_changed = TRUE;
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    Status = STATUS_SUCCESS;
    
end:
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(Vcb, &rollback);
    
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static NTSTATUS query_ranges(device_extension* Vcb, PFILE_OBJECT FileObject, FILE_ALLOCATED_RANGE_BUFFER* inbuf, ULONG inbuflen, void* outbuf, ULONG outbuflen, ULONG_PTR* retlen) {
    NTSTATUS Status;
    fcb* fcb;
    LIST_ENTRY* le;
    FILE_ALLOCATED_RANGE_BUFFER* ranges = outbuf;
    ULONG i = 0;
    UINT64 last_start, last_end;
    
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
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
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
            EXTENT_DATA2* ed2 = (ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) ? (EXTENT_DATA2*)ext->data->data : NULL;
            UINT64 len = ed2 ? ed2->num_bytes : ext->data->decoded_size;
            
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

static NTSTATUS get_object_id(device_extension* Vcb, PFILE_OBJECT FileObject, FILE_OBJECTID_BUFFER* buf, ULONG buflen, ULONG_PTR* retlen) {
    fcb* fcb;
    
    TRACE("(%p, %p, %p, %x, %p)\n", Vcb, FileObject, buf, buflen, retlen);
    
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
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    RtlCopyMemory(&buf->ObjectId[0], &fcb->inode, sizeof(UINT64));
    RtlCopyMemory(&buf->ObjectId[sizeof(UINT64)], &fcb->subvol->id, sizeof(UINT64));
    
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
    LIST_ENTRY rollback;
    KIRQL irql;
    BOOL lock_paused_balance = FALSE;
    
    TRACE("FSCTL_LOCK_VOLUME\n");
    
    TRACE("locking volume\n");
    
    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK);
    
    if (Vcb->locked)
        return STATUS_SUCCESS;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    
    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        ExReleaseResourceLite(&Vcb->fcb_lock);
        goto end;
    }
    
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    InitializeListHead(&rollback);
    
    if (Vcb->balance.thread && KeReadStateEvent(&Vcb->balance.event)) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
        KeClearEvent(&Vcb->balance.event);
        ExReleaseResourceLite(&Vcb->tree_lock);
        
        lock_paused_balance = TRUE;
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
    
    flush_fcb_caches(Vcb);
    
    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, Irp, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    IoAcquireVpbSpinLock(&irql);

    if (!(Vcb->Vpb->Flags & VPB_LOCKED)) { 
        Vcb->Vpb->Flags |= VPB_LOCKED;
        Vcb->locked = TRUE;
        Vcb->locked_fileobj = IrpSp->FileObject;
        Vcb->lock_paused_balance = lock_paused_balance;
    } else {
        Status = STATUS_ACCESS_DENIED;
        IoReleaseVpbSpinLock(irql);
        
        if (lock_paused_balance)
            KeSetEvent(&Vcb->balance.event, 0, FALSE);
        
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

    Vcb->locked = FALSE;
    Vcb->Vpb->Flags &= ~VPB_LOCKED;
    Vcb->locked_fileobj = NULL;

    IoReleaseVpbSpinLock(irql);
    
    if (Vcb->lock_paused_balance)
        KeSetEvent(&Vcb->balance.event, 0, FALSE);
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
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(UINT32))
            return STATUS_INVALID_PARAMETER;

        h = (HANDLE)LongToHandle((*(PUINT32)Irp->AssociatedIrp.SystemBuffer));
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(HANDLE))
            return STATUS_INVALID_PARAMETER;

        h = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
#if defined(_WIN64)
    }
#endif

    Status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, KernelMode, (void**)&fileobj, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08x\n", Status);
        return Status;
    }
    
    devobj = fileobj->DeviceObject;
    ObDereferenceObject(fileobj);

    ExAcquireResourceSharedLite(&global_loading_lock, TRUE);
    
    le = VcbList.Flink;
    
    while (le != &VcbList) {
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);
        
        if (Vcb->Vpb && Vcb->Vpb->RealDevice == devobj) {
            if (Vcb->Vpb == devobj->Vpb) {
                KIRQL irql;
                PVPB newvpb;
                BOOL free_newvpb = FALSE;
                LIST_ENTRY rollback;
                
                newvpb = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), ALLOC_TAG);
                if (!newvpb) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                RtlZeroMemory(newvpb, sizeof(VPB));
                
                IoAcquireVpbSpinLock(&irql);
                devobj->Vpb->Flags &= ~VPB_MOUNTED;
                IoReleaseVpbSpinLock(irql);
                
                InitializeListHead(&rollback);
                
                ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
                
                Vcb->removing = TRUE;
                
                ExReleaseResourceLite(&Vcb->tree_lock);
                
                CcWaitForCurrentLazyWriterActivity();
                
                ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
                
                flush_fcb_caches(Vcb);
                
                if (Vcb->need_write && !Vcb->readonly)
                    do_write(Vcb, Irp, &rollback);
                
                free_trees(Vcb);
                
                clear_rollback(Vcb, &rollback);
                
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
                    free_newvpb = TRUE;

                IoReleaseVpbSpinLock(irql);
                
                if (free_newvpb)
                    ExFreePool(newvpb);
                
                uninit(Vcb, FALSE);
            }
            
            break;
        }
        
        le = le->Flink;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&global_loading_lock);
    
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

static NTSTATUS get_compression(device_extension* Vcb, PIRP Irp) {
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

static void update_volumes(device_extension* Vcb) {
    LIST_ENTRY* le;
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    
    le = volumes.Flink;
        
    while (le != &volumes) {
        volume* v = CONTAINING_RECORD(le, volume, list_entry);
        
        if (RtlCompareMemory(&Vcb->superblock.uuid, &v->fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            LIST_ENTRY* le;
            
            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev = CONTAINING_RECORD(le, device, list_entry);
                
                if (RtlCompareMemory(&dev->devitem.device_uuid, &v->devuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                    v->gen1 = v->gen2 = Vcb->superblock.generation - 1;
                    break;
                }
                
                le = le->Flink;
            }
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&volumes_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);
}

static NTSTATUS dismount_volume(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    KIRQL irql;
    LIST_ENTRY rollback;
    
    TRACE("FSCTL_DISMOUNT_VOLUME\n");
    
    if (!(Vcb->Vpb->Flags & VPB_MOUNTED))
        return STATUS_SUCCESS;
    
    if (Vcb->disallow_dismount) {
        WARN("attempting to dismount boot volume or one containing a pagefile\n");
        return STATUS_ACCESS_DENIED;
    }
    
    InitializeListHead(&rollback);
    
    Status = FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_DISMOUNT);
    if (!NT_SUCCESS(Status)) {
        WARN("FsRtlNotifyVolumeEvent returned %08x\n", Status);
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
    
    flush_fcb_caches(Vcb);
    
    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, Irp, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    Vcb->removing = TRUE;
    update_volumes(Vcb);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    IoAcquireVpbSpinLock(&irql);
    Vcb->Vpb->Flags &= ~VPB_MOUNTED;
    Vcb->Vpb->Flags |= VPB_DIRECT_WRITES_ALLOWED;
    IoReleaseVpbSpinLock(irql);
    
    return STATUS_SUCCESS;
}

static NTSTATUS is_device_part_of_mounted_btrfs_raid(PDEVICE_OBJECT devobj) {
    NTSTATUS Status;
    ULONG to_read;
    superblock* sb;
    UINT32 crc32;
    BTRFS_UUID fsuuid, devuuid;
    LIST_ENTRY* le;
    
    to_read = devobj->SectorSize == 0 ? sizeof(superblock) : sector_align(sizeof(superblock), devobj->SectorSize);
    
    sb = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = sync_read_phys(devobj, superblock_addrs[0], to_read, (UINT8*)sb, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("sync_read_phys returned %08x\n", Status);
        ExFreePool(sb);
        return Status;
    }
    
    if (sb->magic != BTRFS_MAGIC) {
        TRACE("device is not Btrfs\n");
        ExFreePool(sb);
        return STATUS_SUCCESS;
    }
    
    crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));

    if (crc32 != *((UINT32*)sb->checksum)) {
        TRACE("device has Btrfs magic, but invalid superblock checksum\n");
        ExFreePool(sb);
        return STATUS_SUCCESS;
    }
    
    fsuuid = sb->uuid;
    devuuid = sb->dev_item.device_uuid;
    
    ExFreePool(sb);
    
    ExAcquireResourceSharedLite(&global_loading_lock, TRUE);
    
    le = VcbList.Flink;
    
    while (le != &VcbList) {
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);
        
        if (RtlCompareMemory(&Vcb->superblock.uuid, &fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            LIST_ENTRY* le2;
            
            ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
            
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

static NTSTATUS add_device(device_extension* Vcb, PIRP Irp, void* data, ULONG length, KPROCESSOR_MODE processor_mode) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PFILE_OBJECT fileobj, mountmgrfo;
    HANDLE h;
    LIST_ENTRY rollback, *le;
    GET_LENGTH_INFORMATION gli;
    device* dev;
    DEV_ITEM* di;
    UINT64 dev_id;
    UINT8* mb;
    UINT64* stats;
    MOUNTDEV_NAME mdn1, *mdn2;
    UNICODE_STRING volname, mmdevpath;
    volume* v;
    PDEVICE_OBJECT mountmgr;
    KEY searchkey;
    traverse_ptr tp;
    STORAGE_DEVICE_NUMBER sdn;
    
    volname.Buffer = NULL;
    
    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;
    
    if (Vcb->readonly) // FIXME - handle adding R/W device to seeding device
        return STATUS_MEDIA_WRITE_PROTECTED;
    
#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(UINT32))
            return STATUS_INVALID_PARAMETER;

        h = (HANDLE)LongToHandle((*(PUINT32)Irp->AssociatedIrp.SystemBuffer));
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
        ERR("ObReferenceObjectByHandle returned %08x\n", Status);
        return Status;
    }
    
    Status = is_device_part_of_mounted_btrfs_raid(fileobj->DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("is_device_part_of_mounted_btrfs_raid returned %08x\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }
    
    Status = dev_ioctl(fileobj->DeviceObject, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_DISK_IS_WRITABLE returned %08x\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }
    
    Status = dev_ioctl(fileobj->DeviceObject, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                       &gli, sizeof(gli), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("error reading length information: %08x\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }
    
    if (gli.Length.QuadPart < 0x100000) {
        ERR("device was not large enough to hold FS (%llx bytes, need at least 1 MB)\n", gli.Length.QuadPart);
        ObDereferenceObject(fileobj);
        return STATUS_INTERNAL_ERROR;
    }
    
    Status = dev_ioctl(fileobj->DeviceObject, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0,
                       &mdn1, sizeof(MOUNTDEV_NAME), TRUE, NULL);
    if (Status == STATUS_BUFFER_OVERFLOW) {
        mdn2 = ExAllocatePoolWithTag(PagedPool, offsetof(MOUNTDEV_NAME, Name[0]) + mdn1.NameLength, ALLOC_TAG);
        if (!mdn2) {
            ERR("out of memory\n");
            ObDereferenceObject(fileobj);
            return STATUS_INTERNAL_ERROR;
        }
        
        Status = dev_ioctl(fileobj->DeviceObject, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0,
                           mdn2, offsetof(MOUNTDEV_NAME, Name[0]) + mdn1.NameLength, TRUE, NULL);
        
        if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
            ObDereferenceObject(fileobj);
            return Status;
        }
    } else if (NT_SUCCESS(Status)) {
        mdn2 = ExAllocatePoolWithTag(PagedPool, sizeof(MOUNTDEV_NAME), ALLOC_TAG);
        if (!mdn2) {
            ERR("out of memory\n");
            ObDereferenceObject(fileobj);
            return STATUS_INTERNAL_ERROR;
        }
        
        RtlCopyMemory(mdn2, &mdn1, sizeof(MOUNTDEV_NAME));
    } else {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
        ObDereferenceObject(fileobj);
        return Status;
    }
    
    if (mdn2->NameLength == 0) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned zero-length name\n");
        ObDereferenceObject(fileobj);
        ExFreePool(mdn2);
        return STATUS_INTERNAL_ERROR;
    }
    
    volname.Length = volname.MaximumLength = mdn2->NameLength;
    volname.Buffer = ExAllocatePoolWithTag(PagedPool, volname.MaximumLength, ALLOC_TAG);
    if (!volname.Buffer) {
        ERR("out of memory\n");
        ObDereferenceObject(fileobj);
        ExFreePool(mdn2);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(volname.Buffer, mdn2->Name, volname.Length);
    ExFreePool(mdn2);
    
    InitializeListHead(&rollback);
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
        
    if (Vcb->need_write) {
        Status = do_write(Vcb, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write returned %08x\n", Status);
            do_rollback(Vcb, &rollback);
            goto end;
        }
    }
    
    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
    if (!dev) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(dev, sizeof(device));

    dev->devobj = fileobj->DeviceObject;
    dev->seeding = FALSE;
    dev->length = gli.Length.QuadPart;
    init_device(Vcb, dev, FALSE, TRUE);
    
    InitializeListHead(&dev->space);
    
    if (gli.Length.QuadPart > 0x100000) { // add disk hole - the first MB is marked as used
        Status = add_space_entry(&dev->space, NULL, 0x100000, gli.Length.QuadPart - 0x100000);
        if (!NT_SUCCESS(Status)) {
            ERR("add_space_entry returned %08x\n", Status);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
    }
    
    dev_id = 0;
    
    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        
        if (dev->devitem.dev_id > dev_id)
            dev_id = dev->devitem.dev_id;
        
        le = le->Flink;
    }
    
    dev_id++;
    
    dev->devitem.dev_id = dev_id;
    dev->devitem.num_bytes = gli.Length.QuadPart;
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
    
    if (!insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, di->dev_id, di, sizeof(DEV_ITEM), NULL, Irp, &rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(di);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add stats entry to dev tree
    stats = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * 5, ALLOC_TAG);
    if (!stats) {
        ERR("out of memory\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    RtlZeroMemory(stats, sizeof(UINT64) * 5);
    
    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = di->dev_id;
    
    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!keycmp(tp.item->key, searchkey))
        delete_tree_item(Vcb, &tp, &rollback);
    
    if (!insert_tree_item(Vcb, Vcb->dev_root, 0, TYPE_DEV_STATS, di->dev_id, stats, sizeof(UINT64) * 5, NULL, Irp, &rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(stats);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // We clear the first megabyte of the device, so Windows doesn't identify it as another FS
    mb = ExAllocatePoolWithTag(PagedPool, 0x100000, ALLOC_TAG);
    if (!mb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(mb, 0x100000);
    
    Status = write_data_phys(fileobj->DeviceObject, 0, mb, 0x100000);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data_phys returned %08x\n", Status);
        goto end;
    }
    
    ExFreePool(mb);
    
    v = ExAllocatePoolWithTag(PagedPool, sizeof(volume), ALLOC_TAG);
    if (!v) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    v->fsuuid = Vcb->superblock.uuid;
    v->devuuid = dev->devitem.device_uuid;
    v->devnum = dev_id;
    v->devpath = volname;
    v->length = gli.Length.QuadPart;
    v->gen1 = v->gen2 = Vcb->superblock.generation;
    v->seeding = FALSE;
    v->processed = TRUE;
    
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    InsertTailList(&volumes, &v->list_entry);
    ExReleaseResourceLite(&volumes_lock);
    
    volname.Buffer = NULL;
    
    Status = dev_ioctl(fileobj->DeviceObject, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                       &sdn, sizeof(STORAGE_DEVICE_NUMBER), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        WARN("IOCTL_STORAGE_GET_DEVICE_NUMBER returned %08x\n", Status);
        v->disk_num = 0;
        v->part_num = 0;
    } else {
        v->disk_num = sdn.DeviceNumber;
        v->part_num = sdn.PartitionNumber;
    }
    
    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
    if (!NT_SUCCESS(Status))
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
    else {
        remove_drive_letter(mountmgr, v);
        
        ObDereferenceObject(mountmgrfo);
    }
    
    Vcb->superblock.num_devices++;
    Vcb->superblock.total_bytes += gli.Length.QuadPart;
    Vcb->devices_loaded++;
    InsertTailList(&Vcb->devices, &dev->list_entry);
    
    ObReferenceObject(fileobj->DeviceObject);
    
    do_write(Vcb, Irp, &rollback);

    free_trees(Vcb);
    
    clear_rollback(Vcb, &rollback);
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&Vcb->tree_lock);
    ObDereferenceObject(fileobj);
    
    if (volname.Buffer)
        ExFreePool(volname.Buffer);
    
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
    
    ccb->allow_extended_dasd_io = TRUE;
    
    return STATUS_SUCCESS;
}

static NTSTATUS query_uuid(device_extension* Vcb, void* data, ULONG length) {
    if (length < sizeof(BTRFS_UUID))
        return STATUS_BUFFER_OVERFLOW;
    
    RtlCopyMemory(data, &Vcb->superblock.uuid, sizeof(BTRFS_UUID));
    
    return STATUS_SUCCESS;
}

NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP Irp, UINT32 type, BOOL user) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    
    switch (type) {
        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
            WARN("STUB: FSCTL_REQUEST_OPLOCK_LEVEL_1\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
            WARN("STUB: FSCTL_REQUEST_OPLOCK_LEVEL_2\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_REQUEST_BATCH_OPLOCK:
            WARN("STUB: FSCTL_REQUEST_BATCH_OPLOCK\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
            WARN("STUB: FSCTL_OPLOCK_BREAK_ACKNOWLEDGE\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
            WARN("STUB: FSCTL_OPBATCH_ACK_CLOSE_PENDING\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_OPLOCK_BREAK_NOTIFY:
            WARN("STUB: FSCTL_OPLOCK_BREAK_NOTIFY\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_LOCK_VOLUME:
            Status = lock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_UNLOCK_VOLUME:
            Status = unlock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_DISMOUNT_VOLUME:
            Status = dismount_volume(DeviceObject->DeviceExtension, Irp);
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
            Status = get_compression(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_SET_COMPRESSION:
            WARN("STUB: FSCTL_SET_COMPRESSION\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_BOOTLOADER_ACCESSED:
            WARN("STUB: FSCTL_SET_BOOTLOADER_ACCESSED\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_OPLOCK_BREAK_ACK_NO_2:
            WARN("STUB: FSCTL_OPLOCK_BREAK_ACK_NO_2\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_INVALIDATE_VOLUMES:
            Status = invalidate_volumes(Irp);
            break;

        case FSCTL_QUERY_FAT_BPB:
            WARN("STUB: FSCTL_QUERY_FAT_BPB\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_REQUEST_FILTER_OPLOCK:
            WARN("STUB: FSCTL_REQUEST_FILTER_OPLOCK\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_FILESYSTEM_GET_STATISTICS:
            Status = fs_get_statistics(DeviceObject, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                       IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
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
            WARN("STUB: FSCTL_GET_RETRIEVAL_POINTERS\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
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
            Status = get_object_id(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->UserBuffer,
                                   IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_DELETE_OBJECT_ID:
            WARN("STUB: FSCTL_DELETE_OBJECT_ID\n");
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case FSCTL_SET_REPARSE_POINT:
            Status = set_reparse_point(DeviceObject, Irp);
            break;

        case FSCTL_GET_REPARSE_POINT:
            Status = get_reparse_point(DeviceObject, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                       IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_DELETE_REPARSE_POINT:
            Status = delete_reparse_point(DeviceObject, Irp);
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
            Status = get_object_id(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->UserBuffer,
                                   IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
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
            Status = query_ranges(DeviceObject->DeviceExtension, IrpSp->FileObject, IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
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

#if WIN32_WINNT >= 0x0600
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
        case FSCTL_BTRFS_GET_FILE_IDS:
            Status = get_file_ids(IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_CREATE_SUBVOL:
            Status = create_subvol(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength, Irp);
            break;
            
        case FSCTL_BTRFS_CREATE_SNAPSHOT:
            Status = create_snapshot(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength, Irp);
            break;
            
        case FSCTL_BTRFS_GET_INODE_INFO:
            Status = get_inode_info(IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_SET_INODE_INFO:
            Status = set_inode_info(IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_GET_DEVICES:
            Status = get_devices(DeviceObject->DeviceExtension, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_GET_USAGE:
            Status = get_usage(DeviceObject->DeviceExtension, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_START_BALANCE:
            Status = start_balance(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;
            
        case FSCTL_BTRFS_QUERY_BALANCE:
            Status = query_balance(DeviceObject->DeviceExtension, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
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
            Status = add_device(DeviceObject->DeviceExtension, Irp, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;
            
        case FSCTL_BTRFS_REMOVE_DEVICE:
            Status = remove_device(DeviceObject->DeviceExtension, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
        break;
        
        case FSCTL_BTRFS_GET_UUID:
            Status = query_uuid(DeviceObject->DeviceExtension, map_user_buffer(Irp), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
        break;

        default:
            TRACE("unknown control code %x (DeviceType = %x, Access = %x, Function = %x, Method = %x)\n",
                          IrpSp->Parameters.FileSystemControl.FsControlCode, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xff0000) >> 16,
                          (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xc000) >> 14, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3ffc) >> 2,
                          IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    return Status;
}
