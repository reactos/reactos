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

static NTSTATUS snapshot_tree_copy(device_extension* Vcb, UINT64 addr, root* subvol, UINT64 dupflags, UINT64* newaddr, PIRP Irp, LIST_ENTRY* rollback) {
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
    
    Status = read_data(Vcb, addr, Vcb->superblock.node_size, NULL, TRUE, buf, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("read_data returned %08x\n", Status);
        goto end;
    }
    
    th = (tree_header*)buf;
    
    RtlZeroMemory(&t, sizeof(tree));
    t.flags = dupflags;
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
    
    if (th->level == 0) {
        UINT32 i;
        leaf_node* ln = (leaf_node*)&th[1];
        
        for (i = 0; i < th->num_items; i++) {
            if (ln[i].key.obj_type == TYPE_EXTENT_DATA && ln[i].size >= sizeof(EXTENT_DATA) && ln[i].offset + ln[i].size <= Vcb->superblock.node_size - sizeof(tree_header)) {
                EXTENT_DATA* ed = (EXTENT_DATA*)(((UINT8*)&th[1]) + ln[i].offset);
                
                // FIXME - what are we supposed to do with prealloc here? Replace it with sparse extents, or do new preallocation?
                if (ed->type == EXTENT_TYPE_REGULAR && ln[i].size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
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
        UINT64 newaddr;
        internal_node* in = (internal_node*)&th[1];
        
        for (i = 0; i < th->num_items; i++) {
            Status = snapshot_tree_copy(Vcb, in[i].address, subvol, dupflags, &newaddr, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("snapshot_tree_copy returned %08x\n", Status);
                goto end;
            }
            
            in[i].generation = Vcb->superblock.generation;
            in[i].address = newaddr;
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
    
    clear_rollback(&rollback);
    
    InitializeListHead(&rollback);
    
    // create new root
    
    if (Vcb->root_root->lastinode == 0)
        get_last_inode(Vcb, Vcb->root_root, Irp);
    
    id = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;
    Status = create_root(Vcb, id, &r, TRUE, Vcb->superblock.generation, Irp, &rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08x\n", Status);
        goto end;
    }
    
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
    } while (NT_SUCCESS(Status) && !keycmp(&searchkey, &tp.item->key));
    
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
    
    Status = snapshot_tree_copy(Vcb, subvol->root_item.block_number, r, tp.tree->flags, &address, Irp, &rollback);
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
    Vcb->root_root->lastinode = r->id;
    
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
    
    Status = open_fcb(Vcb, r, r->root_item.objid, BTRFS_TYPE_DIRECTORY, utf8, fcb, &fr->fcb, Irp);
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
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    
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
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, FALSE, NULL, NULL, Irp);
    
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
    
    Status = do_create_snapshot(Vcb, FileObject, subvol_fcb, &utf8, &nameus, Irp);
    
    if (NT_SUCCESS(Status)) {
        file_ref* fr;

        Status = open_fileref(Vcb, &fr, &nameus, fileref, FALSE, NULL, NULL, Irp);
        
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
    Status = open_fileref(fcb->Vcb, &fr2, &nameus, fileref, FALSE, NULL, NULL, Irp);
    
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
    
    if (Vcb->root_root->lastinode == 0)
        get_last_inode(Vcb, Vcb->root_root, Irp);
    
    // FIXME - make sure rollback removes new roots from internal structures
    
    id = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;
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
    } while (NT_SUCCESS(Status) && !keycmp(&searchkey, &tp.item->key));
    
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

    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    InsertTailList(&r->fcbs, &rootfcb->list_entry);
    InsertTailList(&Vcb->all_fcbs, &rootfcb->list_entry_all);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    rootfcb->Header.IsFastIoPossible = fast_io_possible(rootfcb);
    rootfcb->Header.AllocationSize.QuadPart = 0;
    rootfcb->Header.FileSize.QuadPart = 0;
    rootfcb->Header.ValidDataLength.QuadPart = 0;
    
    rootfcb->created = TRUE;
    
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
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    
    mark_fcb_dirty(fcb);
    
    Vcb->root_root->lastinode = id;

    Status = STATUS_SUCCESS;    
    
end:
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(&rollback);
    
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
    
    if (bsii->flags_changed || bsii->mode_changed || bsii->uid_changed || bsii->gid_changed)
        mark_fcb_dirty(fcb);
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    
    return Status;
}

static NTSTATUS is_volume_mounted(device_extension* Vcb, PIRP Irp) {
    UINT64 i, num_devices;
    NTSTATUS Status;
    ULONG cc;
    IO_STATUS_BLOCK iosb;
    BOOL verify = FALSE;
    
    num_devices = Vcb->superblock.num_devices;
    for (i = 0; i < num_devices; i++) {
        if (Vcb->devices[i].devobj && Vcb->devices[i].removable) {
            Status = dev_ioctl(Vcb->devices[i].devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), FALSE, &iosb);
            
            if (iosb.Information != sizeof(ULONG))
                cc = 0;
            
            if (Status == STATUS_VERIFY_REQUIRED || (NT_SUCCESS(Status) && cc != Vcb->devices[i].change_count)) {
                Vcb->devices[i].devobj->Flags |= DO_VERIFY_VOLUME;
                verify = TRUE;
            }
            
            if (NT_SUCCESS(Status) && iosb.Information == sizeof(ULONG))
                Vcb->devices[i].change_count = cc;
            
            if (!NT_SUCCESS(Status) || verify) {
                IoSetHardErrorOrVerifyDevice(Irp, Vcb->devices[i].devobj);
                
                return verify ? STATUS_VERIFY_REQUIRED : Status;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS fs_get_statistics(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject, void* buffer, DWORD buflen, DWORD* retlen) {
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
    
    if (!(ccb->access & FILE_WRITE_ATTRIBUTES)) {
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

static NTSTATUS zero_data(device_extension* Vcb, fcb* fcb, UINT64 start, UINT64 length, LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback) {
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
        Status = read_file(fcb, data, start_data, end_data - start_data, NULL, Irp);
        
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08x\n", Status);
            ExFreePool(data);
            return Status;
        }
    }
    
    RtlZeroMemory(data + start - start_data, length);
    
    if (compress) {
        Status = write_compressed(fcb, start_data, end_data, data, changed_sector_list, Irp, rollback);
        
        ExFreePool(data);
        
        if (!NT_SUCCESS(Status)) {
            ERR("write_compressed returned %08x\n", Status);
            return Status;
        }
    } else {
        Status = do_write_file(fcb, start_data, end_data, data, changed_sector_list, Irp, rollback);
        
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
    LIST_ENTRY rollback, changed_sector_list, *le;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    UINT64 start, end;
    extent* ext;
    BOOL nocsum;
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
    
    if (!(ccb->access & FILE_WRITE_DATA)) {
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
    
    nocsum = fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
    
    if (!nocsum)
        InitializeListHead(&changed_sector_list);
    
    if (ext->datalen >= sizeof(EXTENT_DATA) && ext->data->type == EXTENT_TYPE_INLINE) {
        Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, nocsum ? NULL : &changed_sector_list, Irp, &rollback);
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
            Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, fzdi->BeyondFinalZero.QuadPart - fzdi->FileOffset.QuadPart, nocsum ? NULL : &changed_sector_list, Irp, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("zero_data returned %08x\n", Status);
                goto end;
            }
        } else {
            if (start > fzdi->FileOffset.QuadPart) {
                Status = zero_data(Vcb, fcb, fzdi->FileOffset.QuadPart, start - fzdi->FileOffset.QuadPart, nocsum ? NULL : &changed_sector_list, Irp, &rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("zero_data returned %08x\n", Status);
                    goto end;
                }
            }
            
            if (end < fzdi->BeyondFinalZero.QuadPart) {
                Status = zero_data(Vcb, fcb, end, fzdi->BeyondFinalZero.QuadPart - end, nocsum ? NULL : &changed_sector_list, Irp, &rollback);
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
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    
    fcb->extents_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    Status = STATUS_SUCCESS;
    
    if (!nocsum) {
        ExAcquireResourceExclusiveLite(&Vcb->checksum_lock, TRUE);
        commit_checksum_changes(Vcb, &changed_sector_list);
        ExReleaseResourceLite(&Vcb->checksum_lock);
    }
    
end:
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(&rollback);
    
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static NTSTATUS query_ranges(device_extension* Vcb, PFILE_OBJECT FileObject, FILE_ALLOCATED_RANGE_BUFFER* inbuf, ULONG inbuflen, void* outbuf, ULONG outbuflen, DWORD* retlen) {
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

static NTSTATUS get_object_id(device_extension* Vcb, PFILE_OBJECT FileObject, FILE_OBJECTID_BUFFER* buf, ULONG buflen, DWORD* retlen) {
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
    
    TRACE("FSCTL_LOCK_VOLUME\n");
    
    TRACE("locking volume\n");
    
    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK);
    
    if (Vcb->locked)
        return STATUS_SUCCESS;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    
    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->fcb->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        ExReleaseResourceLite(&Vcb->fcb_lock);
        goto end;
    }
    
    Vcb->locked = TRUE;
    
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    InitializeListHead(&rollback);
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
    
    flush_fcb_caches(Vcb);
    
    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, Irp, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(&rollback);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    IoAcquireVpbSpinLock(&irql);

    if (!(Vcb->Vpb->Flags & VPB_LOCKED)) { 
        Vcb->Vpb->Flags |= VPB_LOCKED;
        Vcb->locked_fileobj = IrpSp->FileObject;
    } else {
        Status = STATUS_ACCESS_DENIED;
        IoReleaseVpbSpinLock(irql);
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
                
                clear_rollback(&rollback);
                
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

NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP Irp, UINT32 type, BOOL user) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    
    switch (type) {
        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
            WARN("STUB: FSCTL_REQUEST_OPLOCK_LEVEL_1\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
            WARN("STUB: FSCTL_REQUEST_OPLOCK_LEVEL_2\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_REQUEST_BATCH_OPLOCK:
            WARN("STUB: FSCTL_REQUEST_BATCH_OPLOCK\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
            WARN("STUB: FSCTL_OPLOCK_BREAK_ACKNOWLEDGE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
            WARN("STUB: FSCTL_OPBATCH_ACK_CLOSE_PENDING\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_OPLOCK_BREAK_NOTIFY:
            WARN("STUB: FSCTL_OPLOCK_BREAK_NOTIFY\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_LOCK_VOLUME:
            Status = lock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_UNLOCK_VOLUME:
            Status = unlock_volume(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_DISMOUNT_VOLUME:
            WARN("STUB: FSCTL_DISMOUNT_VOLUME\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_IS_VOLUME_MOUNTED:
            Status = is_volume_mounted(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_IS_PATHNAME_VALID:
            WARN("STUB: FSCTL_IS_PATHNAME_VALID\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_MARK_VOLUME_DIRTY:
            WARN("STUB: FSCTL_MARK_VOLUME_DIRTY\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_RETRIEVAL_POINTERS:
            WARN("STUB: FSCTL_QUERY_RETRIEVAL_POINTERS\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_COMPRESSION:
            WARN("STUB: FSCTL_GET_COMPRESSION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_COMPRESSION:
            WARN("STUB: FSCTL_SET_COMPRESSION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_BOOTLOADER_ACCESSED:
            WARN("STUB: FSCTL_SET_BOOTLOADER_ACCESSED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_OPLOCK_BREAK_ACK_NO_2:
            WARN("STUB: FSCTL_OPLOCK_BREAK_ACK_NO_2\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_INVALIDATE_VOLUMES:
            Status = invalidate_volumes(Irp);
            break;

        case FSCTL_QUERY_FAT_BPB:
            WARN("STUB: FSCTL_QUERY_FAT_BPB\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_REQUEST_FILTER_OPLOCK:
            WARN("STUB: FSCTL_REQUEST_FILTER_OPLOCK\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_FILESYSTEM_GET_STATISTICS:
            Status = fs_get_statistics(DeviceObject, IrpSp->FileObject, Irp->AssociatedIrp.SystemBuffer,
                                       IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_GET_NTFS_VOLUME_DATA:
            WARN("STUB: FSCTL_GET_NTFS_VOLUME_DATA\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_NTFS_FILE_RECORD:
            WARN("STUB: FSCTL_GET_NTFS_FILE_RECORD\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_VOLUME_BITMAP:
            WARN("STUB: FSCTL_GET_VOLUME_BITMAP\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_RETRIEVAL_POINTERS:
            WARN("STUB: FSCTL_GET_RETRIEVAL_POINTERS\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_MOVE_FILE:
            WARN("STUB: FSCTL_MOVE_FILE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_IS_VOLUME_DIRTY:
            Status = is_volume_dirty(DeviceObject->DeviceExtension, Irp);
            break;

        case FSCTL_ALLOW_EXTENDED_DASD_IO:
            WARN("STUB: FSCTL_ALLOW_EXTENDED_DASD_IO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_FIND_FILES_BY_SID:
            WARN("STUB: FSCTL_FIND_FILES_BY_SID\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_OBJECT_ID:
            WARN("STUB: FSCTL_SET_OBJECT_ID\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_OBJECT_ID:
            Status = get_object_id(DeviceObject->DeviceExtension, IrpSp->FileObject, Irp->UserBuffer,
                                   IrpSp->Parameters.FileSystemControl.OutputBufferLength, &Irp->IoStatus.Information);
            break;

        case FSCTL_DELETE_OBJECT_ID:
            WARN("STUB: FSCTL_DELETE_OBJECT_ID\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SECURITY_ID_CHECK:
            WARN("STUB: FSCTL_SECURITY_ID_CHECK\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_READ_USN_JOURNAL:
            WARN("STUB: FSCTL_READ_USN_JOURNAL\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_OBJECT_ID_EXTENDED:
            WARN("STUB: FSCTL_SET_OBJECT_ID_EXTENDED\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_ENCRYPTION:
            WARN("STUB: FSCTL_SET_ENCRYPTION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_ENCRYPTION_FSCTL_IO:
            WARN("STUB: FSCTL_ENCRYPTION_FSCTL_IO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_WRITE_RAW_ENCRYPTED:
            WARN("STUB: FSCTL_WRITE_RAW_ENCRYPTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_READ_RAW_ENCRYPTED:
            WARN("STUB: FSCTL_READ_RAW_ENCRYPTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_CREATE_USN_JOURNAL:
            WARN("STUB: FSCTL_CREATE_USN_JOURNAL\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_READ_FILE_USN_DATA:
            WARN("STUB: FSCTL_READ_FILE_USN_DATA\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_WRITE_USN_CLOSE_RECORD:
            WARN("STUB: FSCTL_WRITE_USN_CLOSE_RECORD\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_EXTEND_VOLUME:
            WARN("STUB: FSCTL_EXTEND_VOLUME\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_USN_JOURNAL:
            WARN("STUB: FSCTL_QUERY_USN_JOURNAL\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_DELETE_USN_JOURNAL:
            WARN("STUB: FSCTL_DELETE_USN_JOURNAL\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_MARK_HANDLE:
            WARN("STUB: FSCTL_MARK_HANDLE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SIS_COPYFILE:
            WARN("STUB: FSCTL_SIS_COPYFILE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SIS_LINK_FILES:
            WARN("STUB: FSCTL_SIS_LINK_FILES\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_RECALL_FILE:
            WARN("STUB: FSCTL_RECALL_FILE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_READ_FROM_PLEX:
            WARN("STUB: FSCTL_READ_FROM_PLEX\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_FILE_PREFETCH:
            WARN("STUB: FSCTL_FILE_PREFETCH\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

#if WIN32_WINNT >= 0x0600
        case FSCTL_MAKE_MEDIA_COMPATIBLE:
            WARN("STUB: FSCTL_MAKE_MEDIA_COMPATIBLE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_DEFECT_MANAGEMENT:
            WARN("STUB: FSCTL_SET_DEFECT_MANAGEMENT\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_SPARING_INFO:
            WARN("STUB: FSCTL_QUERY_SPARING_INFO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_ON_DISK_VOLUME_INFO:
            WARN("STUB: FSCTL_QUERY_ON_DISK_VOLUME_INFO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_VOLUME_COMPRESSION_STATE:
            WARN("STUB: FSCTL_SET_VOLUME_COMPRESSION_STATE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_MODIFY_RM:
            WARN("STUB: FSCTL_TXFS_MODIFY_RM\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_QUERY_RM_INFORMATION:
            WARN("STUB: FSCTL_TXFS_QUERY_RM_INFORMATION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_ROLLFORWARD_REDO:
            WARN("STUB: FSCTL_TXFS_ROLLFORWARD_REDO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_ROLLFORWARD_UNDO:
            WARN("STUB: FSCTL_TXFS_ROLLFORWARD_UNDO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_START_RM:
            WARN("STUB: FSCTL_TXFS_START_RM\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_SHUTDOWN_RM:
            WARN("STUB: FSCTL_TXFS_SHUTDOWN_RM\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_READ_BACKUP_INFORMATION:
            WARN("STUB: FSCTL_TXFS_READ_BACKUP_INFORMATION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_WRITE_BACKUP_INFORMATION:
            WARN("STUB: FSCTL_TXFS_WRITE_BACKUP_INFORMATION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_CREATE_SECONDARY_RM:
            WARN("STUB: FSCTL_TXFS_CREATE_SECONDARY_RM\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_GET_METADATA_INFO:
            WARN("STUB: FSCTL_TXFS_GET_METADATA_INFO\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_GET_TRANSACTED_VERSION:
            WARN("STUB: FSCTL_TXFS_GET_TRANSACTED_VERSION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_SAVEPOINT_INFORMATION:
            WARN("STUB: FSCTL_TXFS_SAVEPOINT_INFORMATION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_CREATE_MINIVERSION:
            WARN("STUB: FSCTL_TXFS_CREATE_MINIVERSION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_TRANSACTION_ACTIVE:
            WARN("STUB: FSCTL_TXFS_TRANSACTION_ACTIVE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_ZERO_ON_DEALLOCATION:
            WARN("STUB: FSCTL_SET_ZERO_ON_DEALLOCATION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_REPAIR:
            WARN("STUB: FSCTL_SET_REPAIR\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_GET_REPAIR:
            WARN("STUB: FSCTL_GET_REPAIR\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_WAIT_FOR_REPAIR:
            WARN("STUB: FSCTL_WAIT_FOR_REPAIR\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_INITIATE_REPAIR:
            WARN("STUB: FSCTL_INITIATE_REPAIR\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_CSC_INTERNAL:
            WARN("STUB: FSCTL_CSC_INTERNAL\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SHRINK_VOLUME:
            WARN("STUB: FSCTL_SHRINK_VOLUME\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_SHORT_NAME_BEHAVIOR:
            WARN("STUB: FSCTL_SET_SHORT_NAME_BEHAVIOR\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_DFSR_SET_GHOST_HANDLE_STATE:
            WARN("STUB: FSCTL_DFSR_SET_GHOST_HANDLE_STATE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES:
            WARN("STUB: FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_LIST_TRANSACTIONS:
            WARN("STUB: FSCTL_TXFS_LIST_TRANSACTIONS\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_PAGEFILE_ENCRYPTION:
            WARN("STUB: FSCTL_QUERY_PAGEFILE_ENCRYPTION\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_RESET_VOLUME_ALLOCATION_HINTS:
            WARN("STUB: FSCTL_RESET_VOLUME_ALLOCATION_HINTS\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_TXFS_READ_BACKUP_INFORMATION2:
            WARN("STUB: FSCTL_TXFS_READ_BACKUP_INFORMATION2\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
            
        case FSCTL_CSV_CONTROL:
            WARN("STUB: FSCTL_CSV_CONTROL\n");
            Status = STATUS_NOT_IMPLEMENTED;
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

        default:
            TRACE("unknown control code %x (DeviceType = %x, Access = %x, Function = %x, Method = %x)\n",
                          IrpSp->Parameters.FileSystemControl.FsControlCode, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xff0000) >> 16,
                          (IrpSp->Parameters.FileSystemControl.FsControlCode & 0xc000) >> 14, (IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3ffc) >> 2,
                          IrpSp->Parameters.FileSystemControl.FsControlCode & 0x3);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }
    
    return Status;
}
