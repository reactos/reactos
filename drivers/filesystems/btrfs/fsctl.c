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

static NTSTATUS snapshot_tree_copy(device_extension* Vcb, UINT64 addr, root* subvol, UINT64 dupflags, UINT64* newaddr, LIST_ENTRY* rollback) {
    UINT8* buf;
    NTSTATUS Status;
    write_tree_context* wtc;
    LIST_ENTRY* le;
    tree t;
    tree_header* th;
    chunk* c;
    
    buf = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_tree_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        ExFreePool(buf);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = read_tree(Vcb, addr, buf);
    if (!NT_SUCCESS(Status)) {
        ERR("read_tree returned %08x\n", Status);
        goto end;
    }
    
    th = (tree_header*)buf;
    
    RtlZeroMemory(&t, sizeof(tree));
    t.flags = dupflags;
    t.root = subvol;
    t.header.level = th->level;
    t.header.tree_id = t.root->id;
    
    Status = get_tree_new_address(Vcb, &t, rollback);
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
                        Status = increase_extent_refcount_data(Vcb, ed2->address, ed2->size, subvol, ln[i].key.obj_id, ln[i].key.offset - ed2->offset, 1, rollback);
                        
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
            Status = snapshot_tree_copy(Vcb, in[i].address, subvol, dupflags, &newaddr, rollback);
            
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
    
    Status = write_tree(Vcb, t.new_address, buf, wtc);
    if (!NT_SUCCESS(Status)) {
        ERR("write_tree returned %08x\n", Status);
        goto end;
    }
    
    if (wtc->stripes.Flink != &wtc->stripes) {
        // launch writes and wait
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
            
            IoCallDriver(stripe->device->devobj, stripe->Irp);
            
            le = le->Flink;
        }
        
        KeWaitForSingleObject(&wtc->Event, Executive, KernelMode, FALSE, NULL);
        
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
            
            if (!NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                break;
            }
            
            le = le->Flink;
        }
        
        free_write_tree_stripes(wtc);
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

static void flush_subvol_fcbs(root* subvol) {
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

static NTSTATUS do_create_snapshot(device_extension* Vcb, PFILE_OBJECT parent, fcb* subvol_fcb, UINT32 crc32, PANSI_STRING utf8) {
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
    ULONG disize, rrsize;
    DIR_ITEM *di, *di2;
    ROOT_REF *rr, *rr2;
    INODE_ITEM* ii;
    
    InitializeListHead(&rollback);
    
    acquire_tree_lock(Vcb, TRUE);
    
    // flush open files on this subvol
    
    flush_subvol_fcbs(subvol);

    // flush metadata
    
    if (Vcb->write_trees > 0)
        do_write(Vcb, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(&rollback);
    
    InitializeListHead(&rollback);
    
    // create new root
    
    if (Vcb->root_root->lastinode == 0)
        get_last_inode(Vcb, Vcb->root_root);
    
    id = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;
    Status = create_root(Vcb, id, &r, TRUE, Vcb->superblock.generation, &rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08x\n", Status);
        goto end;
    }
    
    if (!Vcb->uuid_root) {
        root* uuid_root;
        
        TRACE("uuid root doesn't exist, creating it\n");
        
        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, FALSE, 0, &rollback);
        
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
        
        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE);
    } while (NT_SUCCESS(Status) && !keycmp(&searchkey, &tp.item->key));
    
    *root_num = r->id;
    
    if (!insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(UINT64), NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(root_num);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    searchkey.obj_id = r->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    Status = snapshot_tree_copy(Vcb, subvol->root_item.block_number, r, tp.tree->flags, &address, &rollback);
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
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!subvol->treeholder.tree->write) {
        subvol->treeholder.tree->write = TRUE;
        Vcb->write_trees++;
    }
    
    // add DIR_ITEM
    
    dirpos = find_next_dir_index(Vcb, fcb->subvol, fcb->inode);
    if (dirpos == 0) {
        ERR("find_next_dir_index failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    disize = sizeof(DIR_ITEM) - 1 + utf8->Length;
    di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(di);
        goto end;
    }
    
    di->key.obj_id = id;
    di->key.obj_type = TYPE_ROOT_ITEM;
    di->key.offset = 0xffffffffffffffff;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8->Length;
    di->type = BTRFS_TYPE_DIRECTORY;
    RtlCopyMemory(di->name, utf8->Buffer, utf8->Length);
    
    RtlCopyMemory(di2, di, disize);
    
    Status = add_dir_item(Vcb, fcb->subvol, fcb->inode, crc32, di, disize, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_dir_item returned %08x\n", Status);
        goto end;
    }
    
    // add DIR_INDEX
    
    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_DIR_INDEX, dirpos, di2, disize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add ROOT_REF
    
    rrsize = sizeof(ROOT_REF) - 1 + utf8->Length;
    rr = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
    if (!rr) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    rr->dir = fcb->inode;
    rr->index = dirpos;
    rr->n = utf8->Length;
    RtlCopyMemory(rr->name, utf8->Buffer, utf8->Length);
    
    if (!insert_tree_item(Vcb, Vcb->root_root, fcb->subvol->id, TYPE_ROOT_REF, r->id, rr, rrsize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add ROOT_BACKREF
    
    rr2 = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
    if (!rr2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(rr2, rr, rrsize);
    
    if (!insert_tree_item(Vcb, Vcb->root_root, r->id, TYPE_ROOT_BACKREF, fcb->subvol->id, rr2, rrsize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // change fcb's INODE_ITEM
    
    // unlike when we create a file normally, the seq of the parent doesn't appear to change
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size += utf8->Length * 2;
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("error - could not find INODE_ITEM for directory %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
    delete_tree_item(Vcb, &tp, &rollback);
    
    insert_tree_item(Vcb, fcb->subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ii, sizeof(INODE_ITEM), NULL, &rollback);
    
    fcb->subvol->root_item.ctime = now;
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    
    if (Vcb->write_trees > 0)
        do_write(Vcb, &rollback);
    
    free_trees(Vcb);
    
    Status = STATUS_SUCCESS;
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    release_tree_lock(Vcb, TRUE);
    
    return Status;
}

static NTSTATUS create_snapshot(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG length) {
    PFILE_OBJECT subvol_obj;
    NTSTATUS Status;
    btrfs_create_snapshot* bcs = data;
    fcb* subvol_fcb;
    ANSI_STRING utf8;
    UNICODE_STRING nameus;
    ULONG len;
    UINT32 crc32;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    
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
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8.Buffer, utf8.Length);
    
    if (find_file_in_dir_with_crc32(Vcb, &nameus, crc32, fcb->subvol, fcb->inode, NULL, NULL, NULL, NULL)) {
        WARN("file already exists\n");
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end2;
    }
    
    Status = ObReferenceObjectByHandle(bcs->subvol, 0, *IoFileObjectType, UserMode, (void**)&subvol_obj, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("ObReferenceObjectByHandle returned %08x\n", Status);
        goto end2;
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
    
    Status = do_create_snapshot(Vcb, FileObject, subvol_fcb, crc32, &utf8);
    
    if (NT_SUCCESS(Status)) {
        UNICODE_STRING ffn;
        
        ffn.Length = fileref->full_filename.Length + bcs->namelen;
        if (fcb != fcb->Vcb->root_fileref->fcb)
            ffn.Length += sizeof(WCHAR);
        
        ffn.MaximumLength = ffn.Length;
        ffn.Buffer = ExAllocatePoolWithTag(PagedPool, ffn.Length, ALLOC_TAG);
        
        if (ffn.Buffer) {
            ULONG i;
            
            RtlCopyMemory(ffn.Buffer, fileref->full_filename.Buffer, fileref->full_filename.Length);
            i = fileref->full_filename.Length;
            
            if (fcb != fcb->Vcb->root_fileref->fcb) {
                ffn.Buffer[i / sizeof(WCHAR)] = '\\';
                i += sizeof(WCHAR);
            }
            
            RtlCopyMemory(&ffn.Buffer[i / sizeof(WCHAR)], bcs->name, bcs->namelen);
            
            TRACE("full filename = %.*S\n", ffn.Length / sizeof(WCHAR), ffn.Buffer);
            
            FsRtlNotifyFullReportChange(Vcb->NotifySync, &Vcb->DirNotifyList, (PSTRING)&ffn, i, NULL, NULL,
                                        FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED, NULL);
            
            ExFreePool(ffn.Buffer);
        } else
            ERR("out of memory\n");
    }
    
end:
    ObDereferenceObject(subvol_obj);
    
end2:
    ExFreePool(utf8.Buffer);
    
    return Status;
}

static NTSTATUS create_subvol(device_extension* Vcb, PFILE_OBJECT FileObject, WCHAR* name, ULONG length) {
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    NTSTATUS Status;
    LIST_ENTRY rollback;
    UINT64 id;
    root* r;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ULONG len, disize, rrsize, irsize;
    UNICODE_STRING nameus;
    ANSI_STRING utf8;
    UINT64 dirpos;
    DIR_ITEM *di, *di2;
    UINT32 crc32;
    ROOT_REF *rr, *rr2;
    INODE_ITEM* ii;
    INODE_REF* ir;
    KEY searchkey;
    traverse_ptr tp;
    SECURITY_DESCRIPTOR* sd = NULL;
    SECURITY_SUBJECT_CONTEXT subjcont;
    PSID owner;
    BOOLEAN defaulted;
    UINT64* root_num;
    
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
    
    acquire_tree_lock(Vcb, TRUE);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    InitializeListHead(&rollback);
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8.Buffer, utf8.Length);
    
    if (find_file_in_dir_with_crc32(fcb->Vcb, &nameus, crc32, fcb->subvol, fcb->inode, NULL, NULL, NULL, NULL)) {
        WARN("file already exists\n");
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }
    
    if (Vcb->root_root->lastinode == 0)
        get_last_inode(Vcb, Vcb->root_root);
    
    // FIXME - make sure rollback removes new roots from internal structures
    
    id = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;
    Status = create_root(Vcb, id, &r, FALSE, 0, &rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("create_root returned %08x\n", Status);
        goto end;
    }
    
    TRACE("created root %llx\n", id);
    
    if (!Vcb->uuid_root) {
        root* uuid_root;
        
        TRACE("uuid root doesn't exist, creating it\n");
        
        Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, FALSE, 0, &rollback);
        
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
        
        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE);
    } while (NT_SUCCESS(Status) && !keycmp(&searchkey, &tp.item->key));
    
    *root_num = r->id;
    
    if (!insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(UINT64), NULL, &rollback)) {
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
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(ii, sizeof(INODE_ITEM));
    ii->generation = Vcb->superblock.generation;
    ii->transid = Vcb->superblock.generation;
    ii->st_nlink = 1;
    ii->st_mode = __S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 40755
    ii->st_atime = ii->st_ctime = ii->st_mtime = ii->otime = now;
    ii->st_gid = GID_NOBODY; // FIXME?
       
    SeCaptureSubjectContext(&subjcont);
    
    Status = SeAssignSecurity(fcb->sd, NULL, (void**)&sd, TRUE, &subjcont, IoGetFileObjectGenericMapping(), PagedPool);
    
    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurity returned %08x\n", Status);
        goto end;
    }
    
    if (!sd) {
        ERR("SeAssignSecurity returned NULL security descriptor\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    Status = RtlGetOwnerSecurityDescriptor(sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetOwnerSecurityDescriptor returned %08x\n", Status);
        ii->st_uid = UID_NOBODY;
    } else {
        ii->st_uid = sid_to_uid(&owner);
    }

    if (!insert_tree_item(Vcb, r, r->root_item.objid, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add security.NTACL xattr
    
    Status = set_xattr(Vcb, r, r->root_item.objid, EA_NTACL, EA_NTACL_HASH, (UINT8*)sd, RtlLengthSecurityDescriptor(fcb->sd), &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("set_xattr returned %08x\n", Status);
        goto end;
    }
    
    ExFreePool(sd);
    
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

    if (!insert_tree_item(Vcb, r, r->root_item.objid, TYPE_INODE_REF, r->root_item.objid, ir, irsize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add DIR_ITEM
    
    dirpos = find_next_dir_index(Vcb, fcb->subvol, fcb->inode);
    if (dirpos == 0) {
        ERR("find_next_dir_index failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    disize = sizeof(DIR_ITEM) - 1 + utf8.Length;
    di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(di);
        goto end;
    }
    
    di->key.obj_id = id;
    di->key.obj_type = TYPE_ROOT_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8.Length;
    di->type = BTRFS_TYPE_DIRECTORY;
    RtlCopyMemory(di->name, utf8.Buffer, utf8.Length);
    
    RtlCopyMemory(di2, di, disize);
    
    Status = add_dir_item(Vcb, fcb->subvol, fcb->inode, crc32, di, disize, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_dir_item returned %08x\n", Status);
        goto end;
    }
    
    // add DIR_INDEX
    
    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_DIR_INDEX, dirpos, di2, disize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add ROOT_REF
    
    rrsize = sizeof(ROOT_REF) - 1 + utf8.Length;
    rr = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
    if (!rr) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    rr->dir = fcb->inode;
    rr->index = dirpos;
    rr->n = utf8.Length;
    RtlCopyMemory(rr->name, utf8.Buffer, utf8.Length);
    
    if (!insert_tree_item(Vcb, Vcb->root_root, fcb->subvol->id, TYPE_ROOT_REF, r->id, rr, rrsize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // add ROOT_BACKREF
    
    rr2 = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
    if (!rr2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(rr2, rr, rrsize);
    
    if (!insert_tree_item(Vcb, Vcb->root_root, r->id, TYPE_ROOT_BACKREF, fcb->subvol->id, rr2, rrsize, NULL, &rollback)) {
        ERR("insert_tree_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // change fcb->subvol's ROOT_ITEM
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;
    
    // change fcb's INODE_ITEM
    
    // unlike when we create a file normally, the times and seq of the parent don't appear to change
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size += utf8.Length * 2;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("error - could not find INODE_ITEM for directory %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
    delete_tree_item(Vcb, &tp, &rollback);
    
    insert_tree_item(Vcb, fcb->subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ii, sizeof(INODE_ITEM), NULL, &rollback);
    
    Vcb->root_root->lastinode = id;

    Status = STATUS_SUCCESS;    
    
end:
    if (NT_SUCCESS(Status))
        Status = consider_write(Vcb);
    
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(&rollback);
    
    release_tree_lock(Vcb, TRUE);
    
    if (NT_SUCCESS(Status)) {
        UNICODE_STRING ffn;
        
        ffn.Length = fileref->full_filename.Length + length;
        if (fcb != fcb->Vcb->root_fileref->fcb)
            ffn.Length += sizeof(WCHAR);
        
        ffn.MaximumLength = ffn.Length;
        ffn.Buffer = ExAllocatePoolWithTag(PagedPool, ffn.Length, ALLOC_TAG);
        
        if (ffn.Buffer) {
            ULONG i;
            
            RtlCopyMemory(ffn.Buffer, fileref->full_filename.Buffer, fileref->full_filename.Length);
            i = fileref->full_filename.Length;
            
            if (fcb != fcb->Vcb->root_fileref->fcb) {
                ffn.Buffer[i / sizeof(WCHAR)] = '\\';
                i += sizeof(WCHAR);
            }
            
            RtlCopyMemory(&ffn.Buffer[i / sizeof(WCHAR)], name, length);
            
            TRACE("full filename = %.*S\n", ffn.Length / sizeof(WCHAR), ffn.Buffer);
            
            FsRtlNotifyFullReportChange(Vcb->NotifySync, &Vcb->DirNotifyList, (PSTRING)&ffn, i, NULL, NULL,
                                        FILE_NOTIFY_CHANGE_DIR_NAME, FILE_ACTION_ADDED, NULL);
            
            ExFreePool(ffn.Buffer);
        } else
            ERR("out of memory\n");
    }
    
end2:
    if (utf8.Buffer)
        ExFreePool(utf8.Buffer);
    
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
            WARN("STUB: FSCTL_LOCK_VOLUME\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_UNLOCK_VOLUME:
            WARN("STUB: FSCTL_UNLOCK_VOLUME\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
            WARN("STUB: FSCTL_INVALIDATE_VOLUMES\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
                                       IrpSp->Parameters.DeviceIoControl.OutputBufferLength, &Irp->IoStatus.Information);
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
            WARN("STUB: FSCTL_IS_VOLUME_DIRTY\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
            WARN("STUB: FSCTL_GET_OBJECT_ID\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
                                       IrpSp->Parameters.DeviceIoControl.OutputBufferLength, &Irp->IoStatus.Information);
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
            WARN("STUB: FSCTL_CREATE_OR_GET_OBJECT_ID\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_SPARSE:
            WARN("STUB: FSCTL_SET_SPARSE\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_SET_ZERO_DATA:
            WARN("STUB: FSCTL_SET_ZERO_DATA\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case FSCTL_QUERY_ALLOCATED_RANGES:
            WARN("STUB: FSCTL_QUERY_ALLOCATED_RANGES\n");
            Status = STATUS_NOT_IMPLEMENTED;
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
            Status = get_file_ids(IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.DeviceIoControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_CREATE_SUBVOL:
            Status = create_subvol(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.DeviceIoControl.OutputBufferLength);
            break;
            
        case FSCTL_BTRFS_CREATE_SNAPSHOT:
            Status = create_snapshot(DeviceObject->DeviceExtension, IrpSp->FileObject, map_user_buffer(Irp), IrpSp->Parameters.DeviceIoControl.OutputBufferLength);
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
