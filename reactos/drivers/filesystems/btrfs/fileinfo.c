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

static NTSTATUS STDCALL move_subvol(device_extension* Vcb, fcb* fcb, root* destsubvol, UINT64 destinode, PANSI_STRING utf8, UINT32 crc32, UINT32 oldcrc32, BTRFS_TIME* now, BOOL ReplaceIfExists, LIST_ENTRY* rollback);

static NTSTATUS STDCALL set_basic_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, LIST_ENTRY* rollback) {
    FILE_BASIC_INFORMATION* fbi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ULONG defda;
    BOOL inode_item_changed = FALSE;
    NTSTATUS Status;
    
    if (fcb->ads)
        fcb = fcb->par;
    
    TRACE("file = %.*S, attributes = %x\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fbi->FileAttributes);
    
    if (fbi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY && fcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("attempted to set FILE_ATTRIBUTE_DIRECTORY on non-directory\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    // FIXME - what if FCB is volume or root?
    // FIXME - what about subvol roots?
    
    // FIXME - link FILE_ATTRIBUTE_READONLY to st_mode
    // FIXME - handle times == -1
    
    // FileAttributes == 0 means don't set - undocumented, but seen in fastfat
    if (fbi->FileAttributes != 0) {
        LARGE_INTEGER time;
        BTRFS_TIME now;
        
        defda = get_file_attributes(Vcb, &fcb->inode_item, fcb->subvol, fcb->inode, fcb->type, fcb->filepart.Length > 0 && fcb->filepart.Buffer[0] == '.', TRUE);
        
        if (fcb->type == BTRFS_TYPE_DIRECTORY)
            fbi->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        else if (fcb->type == BTRFS_TYPE_SYMLINK)
            fbi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
                
        // create new xattr
        if (defda != fbi->FileAttributes) {
            char val[64];
            
            TRACE("inserting new DOSATTRIB xattr\n");
            sprintf(val, "0x%lx", fbi->FileAttributes);
        
            Status = set_xattr(Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (UINT8*)val, strlen(val), rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                return Status;
            }
        } else
            delete_xattr(Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, rollback);
        
        fcb->atts = fbi->FileAttributes;
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        fcb->inode_item.st_ctime = now;
        fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
        
        inode_item_changed = TRUE;
    }
    
//     FIXME - CreationTime
//     FIXME - LastAccessTime
//     FIXME - LastWriteTime
//     FIXME - ChangeTime

    if (inode_item_changed) {
        KEY searchkey;
        traverse_ptr tp;
        INODE_ITEM* ii;
        
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            delete_tree_item(Vcb, &tp, rollback);
        else
            WARN("couldn't find old INODE_ITEM\n");
        
        free_traverse_ptr(&tp);
        
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
            
        RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
        
        if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(ii);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL set_disposition_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_DISPOSITION_INFORMATION* fdi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ULONG atts;
    
    TRACE("changing delete_on_close to %s for %.*S (fcb %p)\n", fdi->DeleteFile ? "TRUE" : "FALSE", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fcb);
    
    atts = fcb->ads ? fcb->par->atts : fcb->atts;
    TRACE("atts = %x\n", atts);
    
    if (atts & FILE_ATTRIBUTE_READONLY)
        return STATUS_CANNOT_DELETE;
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0)
        return STATUS_DIRECTORY_NOT_EMPTY;
    
    if (!MmFlushImageSection(&fcb->nonpaged->segment_object, MmFlushForDelete)) {
        WARN("trying to delete file which is being mapped as an image\n");
        return STATUS_CANNOT_DELETE;
    }
    
    if (fcb->inode == SUBVOL_ROOT_INODE) {
        FIXME("FIXME - subvol deletion not yet supported\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    fcb->delete_on_close = fdi->DeleteFile;
    // FIXME - should this fail if file opened with FILE_DELETE_ON_CLOSE?
    FileObject->DeletePending = fdi->DeleteFile;
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_inode_extref(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 parinode, UINT64 index, PANSI_STRING utf8, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    INODE_EXTREF* ier;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_EXTREF;
    searchkey.offset = calc_crc32c((UINT32)parinode, (UINT8*)utf8->Buffer, utf8->Length);

    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        ULONG iersize = tp.item->size + sizeof(INODE_EXTREF) - 1 + utf8->Length;
        UINT8* ier2;
        UINT32 maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
        
        if (iersize > maxlen) {
            ERR("item would be too long (%u > %u)\n", iersize, maxlen);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
        
        ier2 = ExAllocatePoolWithTag(PagedPool, iersize, ALLOC_TAG);
        if (!ier2) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(ier2, tp.item->data, tp.item->size);
        
        ier = (INODE_EXTREF*)&ier2[tp.item->size];
        ier->dir = parinode;
        ier->index = index;
        ier->n = utf8->Length;
        RtlCopyMemory(ier->name, utf8->Buffer, utf8->Length);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ier2, iersize, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        ier = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_EXTREF) - 1 + utf8->Length, ALLOC_TAG);
        if (!ier) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ier->dir = parinode;
        ier->index = index;
        ier->n = utf8->Length;
        RtlCopyMemory(ier->name, utf8->Buffer, utf8->Length);
    
        if (!insert_tree_item(Vcb, subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ier, sizeof(INODE_EXTREF) - 1 + utf8->Length, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

NTSTATUS add_inode_ref(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 parinode, UINT64 index, PANSI_STRING utf8, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    INODE_REF* ir;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_REF;
    searchkey.offset = parinode;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        ULONG irsize = tp.item->size + sizeof(INODE_REF) - 1 + utf8->Length;
        UINT8* ir2;
        UINT32 maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
        
        if (irsize > maxlen) {
            if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF) {
                TRACE("INODE_REF too long, creating INODE_EXTREF\n");
                free_traverse_ptr(&tp);
                return add_inode_extref(Vcb, subvol, inode, parinode, index, utf8, rollback);
            } else {
                ERR("item would be too long (%u > %u)\n", irsize, maxlen);
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        ir2 = ExAllocatePoolWithTag(PagedPool, irsize, ALLOC_TAG);
        if (!ir2) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(ir2, tp.item->data, tp.item->size);
        
        ir = (INODE_REF*)&ir2[tp.item->size];
        ir->index = index;
        ir->n = utf8->Length;
        RtlCopyMemory(ir->name, utf8->Buffer, utf8->Length);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ir2, irsize, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + utf8->Length, ALLOC_TAG);
        if (!ir) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ir->index = index;
        ir->n = utf8->Length;
        RtlCopyMemory(ir->name, utf8->Buffer, utf8->Length);
    
        if (!insert_tree_item(Vcb, subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ir, sizeof(INODE_REF) - 1 + ir->n, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

static NTSTATUS get_fcb_from_dir_item(device_extension* Vcb, fcb** pfcb, fcb* parent, root* subvol, DIR_ITEM* di) {
    LIST_ENTRY* le;
    fcb* sf2;
    struct _fcb* c;
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    le = parent->children.Flink;
    
    while (le != &parent->children) {
        c = CONTAINING_RECORD(le, struct _fcb, list_entry);
        
        if (c->refcount > 0 && c->inode == di->key.obj_id && c->subvol == subvol) {
            c->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
            WARN("fcb %p: refcount now %i (%.*S)\n", c, c->refcount, c->full_filename.Length / sizeof(WCHAR), c->full_filename.Buffer);
#endif
            *pfcb = c;
            return STATUS_SUCCESS;
        }
        
        le = le->Flink;
    }
    
    sf2 = create_fcb();
    if (!sf2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    sf2->Vcb = Vcb;

    sf2->utf8.Length = sf2->utf8.MaximumLength = di->n;
    sf2->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, di->n, ALLOC_TAG);
    if (!sf2->utf8.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(sf2->utf8.Buffer, di->name, di->n);

    sf2->par = parent;
    
    parent->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fcb %p: refcount now %i (%.*S)\n", parent, parent->refcount, parent->full_filename.Length / sizeof(WCHAR), parent->full_filename.Buffer);
#endif
    
    if (di->key.obj_type == TYPE_ROOT_ITEM) {
        root* fcbroot = Vcb->roots;
        while (fcbroot && fcbroot->id != di->key.obj_id)
            fcbroot = fcbroot->next;
        
        sf2->subvol = fcbroot;
        sf2->inode = SUBVOL_ROOT_INODE;
    } else {
        sf2->subvol = subvol;
        sf2->inode = di->key.obj_id;
    }
    
    sf2->type = di->type;
    
    if (Vcb->fcbs)
        Vcb->fcbs->prev = sf2;
    
    sf2->next = Vcb->fcbs;
    Vcb->fcbs = sf2;
    
    sf2->name_offset = parent->full_filename.Length / sizeof(WCHAR);
   
    if (parent != Vcb->root_fcb)
        sf2->name_offset++;
    
    InsertTailList(&parent->children, &sf2->list_entry);
    
    searchkey.obj_id = sf2->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, sf2->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        free_fcb(sf2);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("couldn't find INODE_ITEM for inode %llx in subvol %llx\n", sf2->inode, sf2->subvol->id);
        free_traverse_ptr(&tp);
        free_fcb(sf2);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size > 0)
        RtlCopyMemory(&sf2->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
    
    free_traverse_ptr(&tp);
    
    // This is just a quick function for the sake of move_across_subvols. As such, we don't bother
    // with sf2->atts, sf2->sd, or sf2->full_filename.
    
    *pfcb = sf2;

    return STATUS_SUCCESS;
}

// static LONG get_tree_count(device_extension* Vcb, LIST_ENTRY* tc) {
//     LONG rc = 0;
//     LIST_ENTRY* le = Vcb->trees.Flink;
//     
//     while (le != &Vcb->trees) {
//         tree* t = CONTAINING_RECORD(le, tree, list_entry);
//         
//         rc += t->refcount;
//         
//         le = le->Flink;
//     }
//     
//     le = tc->Flink;
//     while (le != tc) {
//         tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
//         tree* t;
//         
//         rc--;
//         
//         t = tc2->tree->parent;
//         while (t) {
//             rc--;
//             t = t->parent;
//         }
//         
//         le = le->Flink;
//     }
//     
//     return rc;
// }

static NTSTATUS STDCALL move_inode_across_subvols(device_extension* Vcb, fcb* fcb, root* destsubvol, UINT64 destinode, UINT64 inode, UINT64 oldparinode, PANSI_STRING utf8, UINT32 crc32, BTRFS_TIME* now, LIST_ENTRY* rollback) {
    UINT64 oldindex, index;
    UINT32 oldcrc32;
    INODE_ITEM* ii;
    BOOL has_hardlink = FALSE;
    DIR_ITEM* di;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b;
    
    // move INODE_ITEM
    
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    fcb->inode_item.st_ctime = *now;    
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        delete_tree_item(Vcb, &tp, rollback);
        
        if (fcb->inode_item.st_nlink > 1) {
            fcb->inode_item.st_nlink--;
            
            ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
            if (!ii) {
                ERR("out of memory\n");
                free_traverse_ptr(&tp);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
            
            if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback)) {
                ERR("error - failed to insert item\n");
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
            
            has_hardlink = TRUE;
        }
    } else {
        WARN("couldn't find old INODE_ITEM\n");
    }
    
    free_traverse_ptr(&tp);
    
    fcb->inode_item.st_nlink = 1;
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
    
    if (!insert_tree_item(Vcb, destsubvol, inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback)) {
        ERR("error - failed to insert item\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    oldcrc32 = calc_crc32c(0xfffffffe, (UINT8*)fcb->utf8.Buffer, (ULONG)fcb->utf8.Length);
    
    // delete old DIR_ITEM
    
    Status = delete_dir_item(Vcb, fcb->subvol, oldparinode, oldcrc32, &fcb->utf8, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_dir_item returned %08x\n", Status);
        return Status;
    }
    
    // create new DIR_ITEM
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + utf8->Length, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
                        
    di->key.obj_id = inode;
    di->key.obj_type = TYPE_INODE_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8->Length;
    di->type = fcb->type;
    RtlCopyMemory(di->name, utf8->Buffer, utf8->Length);
    
    Status = add_dir_item(Vcb, destsubvol, destinode, crc32, di, sizeof(DIR_ITEM) - 1 + utf8->Length, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_dir_item returned %08x\n", Status);
        return Status;
    }
    
    Status = delete_inode_ref(Vcb, fcb->subvol, fcb->inode, oldparinode, &fcb->utf8, &oldindex, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_inode_ref returned %08x\n", Status);
        return Status;
    }
    
    // delete DIR_INDEX
    
    if (oldindex == 0) {
        WARN("couldn't find old INODE_REF\n");
    } else {    
        searchkey.obj_id = oldparinode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = oldindex;
        
        Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&searchkey, &tp.item->key))
            delete_tree_item(Vcb, &tp, rollback);
        else
            WARN("couldn't find old DIR_INDEX\n");
        
        free_traverse_ptr(&tp);
    }
    
    // get new index
    
    searchkey.obj_id = destinode;
    searchkey.obj_type = TYPE_DIR_INDEX + 1;
    searchkey.offset = 0;
        
    Status = find_item(Vcb, destsubvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
        
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (find_prev_item(Vcb, &tp, &next_tp, FALSE)) {
            free_traverse_ptr(&tp);
            tp = next_tp;
                
            TRACE("moving back to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }

    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_DIR_INDEX) {
        index = tp.item->key.offset + 1;
    } else
        index = 2;
        
    free_traverse_ptr(&tp);
    
    // create INODE_REF
    
    Status = add_inode_ref(Vcb, destsubvol, inode, destinode, index, utf8, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_inode_ref returned %08x\n", Status);
        return Status;
    }
    
    // create DIR_INDEX
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + utf8->Length, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
                        
    di->key.obj_id = inode;
    di->key.obj_type = TYPE_INODE_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8->Length;
    di->type = fcb->type;
    RtlCopyMemory(di->name, utf8->Buffer, utf8->Length);
    
    if (!insert_tree_item(Vcb, destsubvol, destinode, TYPE_DIR_INDEX, index, di, sizeof(DIR_ITEM) - 1 + utf8->Length, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    // move XATTR_ITEMs
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_XATTR_ITEM && tp.item->size > 0) {
            di = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            
            if (!di) {
                ERR("out of memory\n");
                free_traverse_ptr(&tp);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(di, tp.item->data, tp.item->size);
            
            if (!insert_tree_item(Vcb, destsubvol, inode, TYPE_XATTR_ITEM, tp.item->key.offset, di, tp.item->size, NULL, rollback)) {
                ERR("error - failed to insert item\n");
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (!has_hardlink)
                delete_tree_item(Vcb, &tp, rollback);
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);

    // do extents
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_EXTENT_DATA) {
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            } else {
                EXTENT_DATA* ed = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!ed) {
                    ERR("out of memory\n");
                    free_traverse_ptr(&tp);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(ed, tp.item->data, tp.item->size);
                
                // FIXME - update ed's generation
                        
                if (!insert_tree_item(Vcb, destsubvol, inode, TYPE_EXTENT_DATA, tp.item->key.offset, ed, tp.item->size, NULL, rollback)) {
                    ERR("error - failed to insert item\n");
                    free_traverse_ptr(&tp);
                    return STATUS_INTERNAL_ERROR;
                }
            
                if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                    
                    if (ed2->address != 0) {
                        Status = add_extent_ref(Vcb, ed2->address, ed2->size, destsubvol, inode, tp.item->key.offset, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("add_extent_ref returned %08x\n", Status);
                            free_traverse_ptr(&tp);
                            return Status;
                        }
                        
                        if (!has_hardlink) {
                            Status = remove_extent_ref(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, tp.item->key.offset, NULL, rollback);
                        
                            if (!NT_SUCCESS(Status)) {
                                ERR("remove_extent_ref returned %08x\n", Status);
                                free_traverse_ptr(&tp);
                                return Status;
                            }
                        }
                    }
                }
                
                if (!has_hardlink)
                    delete_tree_item(Vcb, &tp, rollback);
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_EXTENT_DATA)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

typedef struct {
    fcb* fcb;
    UINT8 level;
    UINT32 crc32;
    UINT64 newinode;
    UINT64 newparinode;
    BOOL subvol;
    ANSI_STRING utf8;
    LIST_ENTRY list_entry;
} dir_list;

static NTSTATUS add_to_dir_list(fcb* fcb, UINT8 level, LIST_ENTRY* dl, UINT64 newparinode, BOOL* empty) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    NTSTATUS Status;
    
    *empty = TRUE;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = 2;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_DIR_INDEX) {
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            } else {
                DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
                struct _fcb* child;
                dir_list* dl2;
                
                if (tp.item->size < sizeof(DIR_ITEM) - 1 + di->n + di->m) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                } else {
                    if (di->key.obj_type == TYPE_INODE_ITEM || di->key.obj_type == TYPE_ROOT_ITEM) {
                        if (di->key.obj_type == TYPE_ROOT_ITEM)
                            TRACE("moving subvol %llx\n", di->key.obj_id);
                        else
                            TRACE("moving inode %llx\n", di->key.obj_id);
                        
                        *empty = FALSE;
                        
                        Status = get_fcb_from_dir_item(fcb->Vcb, &child, fcb, fcb->subvol, di);
                        if (!NT_SUCCESS(Status)) {
                            ERR("get_fcb_from_dir_item returned %08x\n", Status);
                            free_traverse_ptr(&tp);
                            return Status;
                        }
                        
                        dl2 = ExAllocatePoolWithTag(PagedPool, sizeof(dir_list), ALLOC_TAG);
                        if (!dl2) {
                            ERR("out of memory\n");
                            free_traverse_ptr(&tp);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        dl2->fcb = child;
                        dl2->level = level;
                        dl2->newparinode = newparinode;
                        dl2->subvol = di->key.obj_type == TYPE_ROOT_ITEM;
                        
                        dl2->utf8.Length = dl2->utf8.MaximumLength = di->n;
                        dl2->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, dl2->utf8.MaximumLength, ALLOC_TAG);
                        if (!dl2->utf8.Buffer) {
                            ERR("out of memory\n");
                            free_traverse_ptr(&tp);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        RtlCopyMemory(dl2->utf8.Buffer, di->name, dl2->utf8.Length);
                        dl2->crc32 = calc_crc32c(0xfffffffe, (UINT8*)dl2->utf8.Buffer, (ULONG)dl2->utf8.Length);
                        
                        InsertTailList(dl, &dl2->list_entry);
                    }
                }
            }
        }
        
        b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL move_across_subvols(device_extension* Vcb, fcb* fcb, root* destsubvol, UINT64 destinode, PANSI_STRING utf8, UINT32 crc32, BTRFS_TIME* now, LIST_ENTRY* rollback) {
    UINT64 inode, oldparinode;
    NTSTATUS Status;
    LIST_ENTRY dl;
    
    if (destsubvol->lastinode == 0)
        get_last_inode(Vcb, destsubvol);
    
    inode = destsubvol->lastinode + 1;
    destsubvol->lastinode++;
    
    oldparinode = fcb->subvol == fcb->par->subvol ? fcb->par->inode : SUBVOL_ROOT_INODE;
    
    Status = move_inode_across_subvols(Vcb, fcb, destsubvol, destinode, inode, oldparinode, utf8, crc32, now, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("move_inode_across_subvols returned %08x\n", Status);
        return Status;
    }
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0) {
        BOOL b, empty;
        UINT8 level, max_level;
        LIST_ENTRY* le;
        
        InitializeListHead(&dl);
        
        add_to_dir_list(fcb, 0, &dl, inode, &b);
        
        level = 0;
        do {
            empty = TRUE;
            
            le = dl.Flink;
            while (le != &dl) {
                dir_list* dl2 = CONTAINING_RECORD(le, dir_list, list_entry);
                
                if (dl2->level == level && !dl2->subvol) {
                    inode++;
                    destsubvol->lastinode++;
                    
                    dl2->newinode = inode;
                    
                    if (dl2->fcb->type == BTRFS_TYPE_DIRECTORY) {
                        add_to_dir_list(dl2->fcb, level+1, &dl, dl2->newinode, &b);
                        if (!b) empty = FALSE;
                    }
                }
                
                le = le->Flink;
            }
            
            if (!empty) level++;
        } while (!empty);
        
        max_level = level;
        
        for (level = 0; level <= max_level; level++) {
            TRACE("level %u\n", level);
            
            le = dl.Flink;
            while (le != &dl) {
                dir_list* dl2 = CONTAINING_RECORD(le, dir_list, list_entry);
                
                if (dl2->level == level) {
                    if (dl2->subvol) {
                        TRACE("subvol %llx\n", dl2->fcb->subvol->id);
                        
                        Status = move_subvol(Vcb, dl2->fcb, destsubvol, dl2->newparinode, &dl2->utf8, dl2->crc32, dl2->crc32, now, FALSE, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("move_subvol returned %08x\n", Status);
                            return Status;
                        }
                    } else {
                        TRACE("inode %llx\n", dl2->fcb->inode);

                        Status = move_inode_across_subvols(Vcb, dl2->fcb, destsubvol, dl2->newparinode, dl2->newinode, dl2->fcb->par->inode, &dl2->utf8, dl2->crc32, now, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("move_inode_across_subvols returned %08x\n", Status);
                            return Status;
                        }
                    }
                }
                
                le = le->Flink;
            }
        }
        
        while (!IsListEmpty(&dl)) {
            dir_list* dl2;
            
            le = RemoveHeadList(&dl);
            dl2 = CONTAINING_RECORD(le, dir_list, list_entry);
            
            ExFreePool(dl2->utf8.Buffer);
            free_fcb(dl2->fcb);
            
            ExFreePool(dl2);
        }
    }
    
    fcb->inode = inode;
    fcb->subvol = destsubvol;
      
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = *now;
    
    return STATUS_SUCCESS;
}

static NTSTATUS delete_root_ref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, UINT64 parinode, PANSI_STRING utf8, UINT64* index, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (tp.item->size < sizeof(ROOT_REF)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(ROOT_REF));
        } else {
            ROOT_REF* rr;
            ULONG len;
            
            rr = (ROOT_REF*)tp.item->data;
            len = tp.item->size;
            
            do {
                ULONG itemlen;
                
                if (len < sizeof(ROOT_REF) || len < sizeof(ROOT_REF) - 1 + rr->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                itemlen = sizeof(ROOT_REF) - sizeof(char) + rr->n;
                
                if (rr->dir == parinode && rr->n == utf8->Length && RtlCompareMemory(rr->name, utf8->Buffer, rr->n) == rr->n) {
                    ULONG newlen = tp.item->size - itemlen;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newrr = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *rroff;
                        
                        if (!newrr) {
                            ERR("out of memory\n");
                            free_traverse_ptr(&tp);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)rr > tp.item->data) {
                            RtlCopyMemory(newrr, tp.item->data, (UINT8*)rr - tp.item->data);
                            rroff = newrr + ((UINT8*)rr - tp.item->data);
                        } else {
                            rroff = newrr;
                        }
                        
                        if ((UINT8*)&rr->name[rr->n] - tp.item->data < tp.item->size)
                            RtlCopyMemory(rroff, &rr->name[rr->n], tp.item->size - ((UINT8*)&rr->name[rr->n] - tp.item->data));
                        
                        insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newrr, newlen, NULL, rollback);
                    }
                    
                    if (index)
                        *index = rr->index;
                    
                    break;
                }
                
                if (len > itemlen) {
                    len -= itemlen;
                    rr = (ROOT_REF*)&rr->name[rr->n];
                } else
                    break;
            } while (len > 0);
        }
    } else {
        WARN("could not find ROOT_REF entry for subvol %llx in %llx\n", searchkey.offset, searchkey.obj_id);
    }
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_root_ref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, ROOT_REF* rr, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        ULONG rrsize = tp.item->size + sizeof(ROOT_REF) - 1 + rr->n;
        UINT8* rr2;
        
        rr2 = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
        if (!rr2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(rr2, tp.item->data, tp.item->size);
        
        RtlCopyMemory(rr2 + tp.item->size, rr, sizeof(ROOT_REF) - 1 + rr->n);
        ExFreePool(rr);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr2, rrsize, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(rr2);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr, sizeof(ROOT_REF) - 1 + rr->n, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(rr);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL update_root_backref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    UINT8* data;
    ULONG datalen;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey) && tp.item->size > 0) {
        datalen = tp.item->size;
        
        data = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
        if (!data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(data, tp.item->data, datalen);
    } else {
        datalen = 0;
    }
    
    free_traverse_ptr(&tp);
    
    searchkey.obj_id = subvolid;
    searchkey.obj_type = TYPE_ROOT_BACKREF;
    searchkey.offset = parsubvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey))
        delete_tree_item(Vcb, &tp, rollback);
    
    free_traverse_ptr(&tp);
    
    if (datalen > 0) {
        if (!insert_tree_item(Vcb, Vcb->root_root, subvolid, TYPE_ROOT_BACKREF, parsubvolid, data, datalen, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(data);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL move_subvol(device_extension* Vcb, fcb* fcb, root* destsubvol, UINT64 destinode, PANSI_STRING utf8, UINT32 crc32, UINT32 oldcrc32, BTRFS_TIME* now, BOOL ReplaceIfExists, LIST_ENTRY* rollback) {
    DIR_ITEM* di;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 oldindex, index;
    ROOT_REF* rr;
    
    // delete old DIR_ITEM
    
    Status = delete_dir_item(Vcb, fcb->par->subvol, fcb->par->inode, oldcrc32, &fcb->utf8, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_dir_item returned %08x\n", Status);
        return Status;
    }
    
    // create new DIR_ITEM
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + utf8->Length, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
        
    di->key.obj_id = fcb->subvol->id;
    di->key.obj_type = TYPE_ROOT_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8->Length;
    di->type = fcb->type;
    RtlCopyMemory(di->name, utf8->Buffer, utf8->Length);
    
    Status = add_dir_item(Vcb, destsubvol, destinode, crc32, di, sizeof(DIR_ITEM) - 1 + utf8->Length, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_dir_item returned %08x\n", Status);
        return Status;
    }
    
    // delete old ROOT_REF
    
    oldindex = 0;
    
    Status = delete_root_ref(Vcb, fcb->subvol->id, fcb->par->subvol->id, fcb->par->inode, &fcb->utf8, &oldindex, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_root_ref returned %08x\n", Status);
        return Status;
    }
    
    TRACE("root index = %llx\n", oldindex);
    
    // delete old DIR_INDEX
    
    if (oldindex != 0) {
        searchkey.obj_id = fcb->par->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = oldindex;
        
        Status = find_item(Vcb, fcb->par->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&searchkey, &tp.item->key)) {
            TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            
            delete_tree_item(Vcb, &tp, rollback);
        } else {
            WARN("could not find old DIR_INDEX entry\n");
        }
        
        free_traverse_ptr(&tp);
    }
    
    // create new DIR_INDEX
    
    if (fcb->par->subvol == destsubvol && fcb->par->inode == destinode) {
        index = oldindex;
    } else {
        index = find_next_dir_index(Vcb, destsubvol, destinode);
    }
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + utf8->Length, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
        
    di->key.obj_id = fcb->subvol->id;
    di->key.obj_type = TYPE_ROOT_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = utf8->Length;
    di->type = fcb->type;
    RtlCopyMemory(di->name, utf8->Buffer, utf8->Length);
    
    if (!insert_tree_item(Vcb, destsubvol, destinode, TYPE_DIR_INDEX, index, di, sizeof(DIR_ITEM) - 1 + utf8->Length, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    // create new ROOT_REF
    
    rr = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_REF) - 1 + utf8->Length, ALLOC_TAG);
    if (!rr) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    rr->dir = destinode;
    rr->index = index;
    rr->n = utf8->Length;
    RtlCopyMemory(rr->name, utf8->Buffer, utf8->Length);
    
    Status = add_root_ref(Vcb, fcb->subvol->id, destsubvol->id, rr, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_root_ref returned %08x\n", Status);
        return Status;
    }
    
    Status = update_root_backref(Vcb, fcb->subvol->id, fcb->par->subvol->id, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_root_backref 1 returned %08x\n", Status);
        return Status;
    }
    
    if (fcb->par->subvol != destsubvol) {
        Status = update_root_backref(Vcb, fcb->subvol->id, destsubvol->id, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_root_backref 1 returned %08x\n", Status);
            return Status;
        }
        
        fcb->par->subvol->root_item.ctransid = Vcb->superblock.generation;
        fcb->par->subvol->root_item.ctime = *now;
    }
    
    destsubvol->root_item.ctransid = Vcb->superblock.generation;
    destsubvol->root_item.ctime = *now;
    
    return STATUS_SUCCESS;
}

static BOOL has_open_children(fcb* fcb) {
    LIST_ENTRY* le = fcb->children.Flink;
    struct _fcb* c;
    
    while (le != &fcb->children) {
        c = CONTAINING_RECORD(le, struct _fcb, list_entry);
        
        if (c->refcount > 0) {
            if (c->open_count > 0)
                return TRUE;
            
            if (has_open_children(c))
                return TRUE;
        }
        
        le = le->Flink;
    }
    
    return FALSE;
}

static NTSTATUS STDCALL set_rename_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, PFILE_OBJECT tfo, BOOL ReplaceIfExists, LIST_ENTRY* rollback) {
    FILE_RENAME_INFORMATION* fri = Irp->AssociatedIrp.SystemBuffer;
    fcb *fcb = FileObject->FsContext, *tfofcb, *oldparfcb, *oldfcb;
    root* parsubvol;
    UINT64 parinode, dirpos;
    WCHAR* fn;
    UNICODE_STRING fnus;
    ULONG fnlen, utf8len, disize;
    NTSTATUS Status;
    ANSI_STRING utf8;
    UINT32 crc32, oldcrc32;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    DIR_ITEM* di;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    BOOL across_directories;
    INODE_ITEM* ii;
    
    // FIXME - MSDN says we should be able to rename streams here, but I can't get it to work.
    
    TRACE("    tfo = %p\n", tfo);
    TRACE("    ReplaceIfExists = %u\n", ReplaceIfExists);
    TRACE("    RootDirectory = %p\n", fri->RootDirectory);
    TRACE("    FileName = %.*S\n", fri->FileNameLength / sizeof(WCHAR), fri->FileName);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    utf8.Buffer = NULL;
    
    if (!fcb->par) {
        ERR("error - tried to rename file with no parent\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    fn = fri->FileName;
    fnlen = fri->FileNameLength / sizeof(WCHAR);
    
    if (!tfo) {
        parsubvol = fcb->par->subvol;
        parinode = fcb->par->inode;
        tfofcb = NULL;
        
        across_directories = FALSE;
    } else {
        LONG i;
        
        tfofcb = tfo->FsContext;
        parsubvol = tfofcb->subvol;
        parinode = tfofcb->inode;
        
        for (i = fnlen - 1; i >= 0; i--) {
            if (fri->FileName[i] == '\\' || fri->FileName[i] == '/') {
                fn = &fri->FileName[i+1];
                fnlen = (fri->FileNameLength / sizeof(WCHAR)) - i - 1;
                break;
            }
        }
        
        across_directories = parsubvol != fcb->par->subvol || parinode != fcb->par->inode;
    }
    
    fnus.Buffer = fn;
    fnus.Length = fnus.MaximumLength = fnlen * sizeof(WCHAR);
    
    TRACE("fnus = %.*S\n", fnus.Length / sizeof(WCHAR), fnus.Buffer);
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto end;
    
    utf8.MaximumLength = utf8.Length = utf8len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    Status = RtlUnicodeToUTF8N(utf8.Buffer, utf8len, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto end;
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8.Buffer, (ULONG)utf8.Length);
    
    // FIXME - set to crc32 if utf8 and oldutf8 are identical
    oldcrc32 = calc_crc32c(0xfffffffe, (UINT8*)fcb->utf8.Buffer, (ULONG)fcb->utf8.Length);
    
//     TRACE("utf8 fn = %s (%08x), old utf8 fn = %s (%08x)\n", utf8, crc32, oldutf8, oldcrc32);

    oldfcb = NULL;

    Status = get_fcb(Vcb, &oldfcb, &fnus, tfo ? tfo->FsContext : NULL, FALSE);

    if (NT_SUCCESS(Status)) {
        WARN("destination file %.*S already exists\n", oldfcb->full_filename.Length / sizeof(WCHAR), oldfcb->full_filename.Buffer);
        
        if (fcb != oldfcb && !(oldfcb->open_count == 0 && oldfcb->deleted)) {
            if (!ReplaceIfExists) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            } else if (oldfcb->open_count >= 1 && !oldfcb->deleted) {
                WARN("trying to overwrite open file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
            
            if (oldfcb->type == BTRFS_TYPE_DIRECTORY) {
                WARN("trying to overwrite directory\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
        }
    }
    
    if (has_open_children(fcb)) {
        WARN("trying to rename file with open children\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    if (oldfcb) {
        Status = delete_fcb(oldfcb, NULL, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_fcb returned %08x\n", Status);
            goto end;
        }
    }
    
    if (fcb->inode == SUBVOL_ROOT_INODE) {
        UNICODE_STRING filename;
        
        filename.Buffer = fn;
        filename.MaximumLength = filename.Length = fnlen * sizeof(WCHAR);
        
        Status = move_subvol(Vcb, fcb, tfofcb->subvol, tfofcb->inode, &utf8, crc32, oldcrc32, &now, ReplaceIfExists, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("move_subvol returned %08x\n", Status);
            goto end;
        }
    } else if (parsubvol != fcb->subvol) {
        UNICODE_STRING filename;
        
        filename.Buffer = fn;
        filename.MaximumLength = filename.Length = fnlen * sizeof(WCHAR);
        
        Status = move_across_subvols(Vcb, fcb, tfofcb->subvol, tfofcb->inode, &utf8, crc32, &now, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("move_across_subvols returned %08x\n", Status);
            goto end;
        }
    } else {
        UINT64 oldindex;
        INODE_ITEM* ii;
        
        // delete old DIR_ITEM entry
        
        Status = delete_dir_item(Vcb, fcb->subvol, fcb->par->inode, oldcrc32, &fcb->utf8, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_dir_item returned %08x\n", Status);
            return Status;
        }
        
        // FIXME - make sure fcb's filepart matches the case on disk
        
        // create new DIR_ITEM entry
        
        di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + utf8.Length, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        di->key.obj_id = fcb->inode;
        di->key.obj_type = TYPE_INODE_ITEM;
        di->key.offset = 0;
        di->transid = Vcb->superblock.generation;
        di->m = 0;
        di->n = utf8.Length;
        di->type = fcb->type;
        RtlCopyMemory(di->name, utf8.Buffer, utf8.Length);
        
        Status = add_dir_item(Vcb, parsubvol, parinode, crc32, di, sizeof(DIR_ITEM) - 1 + utf8.Length, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_dir_item returned %08x\n", Status);
            return Status;
        }
        
        oldindex = 0;
        
        Status = delete_inode_ref(Vcb, fcb->subvol, fcb->inode, fcb->par->inode, &fcb->utf8, &oldindex, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_inode_ref returned %08x\n", Status);
            return Status;
        }

        // delete old DIR_INDEX entry
        
        if (oldindex != 0) {
            searchkey.obj_id = fcb->par->inode;
            searchkey.obj_type = TYPE_DIR_INDEX;
            searchkey.offset = oldindex;
            
            Status = find_item(Vcb, fcb->par->subvol, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }
            
            if (!keycmp(&tp.item->key, &searchkey))
                delete_tree_item(Vcb, &tp, rollback);
            else {
                WARN("couldn't find DIR_INDEX\n");
            }
                
            free_traverse_ptr(&tp);
        } else {
            WARN("couldn't get index from INODE_REF\n");
        }
        
        // create new DIR_INDEX entry
        
        if (parsubvol != fcb->par->subvol || parinode != fcb->par->inode) {
            searchkey.obj_id = parinode;
            searchkey.obj_type = TYPE_DIR_INDEX + 1;
            searchkey.offset = 0;
            
            Status = find_item(Vcb, parsubvol, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }
            
            dirpos = 2;
            
            do {
                TRACE("%llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                
                if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_DIR_INDEX) {
                    dirpos = tp.item->key.offset + 1;
                    break;
                }
                
                if (find_prev_item(Vcb, &tp, &next_tp, FALSE)) {
                    free_traverse_ptr(&tp);
                    tp = next_tp;
                } else
                    break;
            } while (tp.item->key.obj_id >= parinode && tp.item->key.obj_type >= TYPE_DIR_INDEX);

            free_traverse_ptr(&tp);
        } else
            dirpos = oldindex;
        
        disize = (ULONG)(sizeof(DIR_ITEM) - 1 + utf8.Length);
        di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        di->key.obj_id = fcb->inode;
        di->key.obj_type = TYPE_INODE_ITEM;
        di->key.offset = 0;
        di->transid = Vcb->superblock.generation;
        di->m = 0;
        di->n = (UINT16)utf8.Length;
        di->type = fcb->type;
        RtlCopyMemory(di->name, utf8.Buffer, utf8.Length);
        
        if (!insert_tree_item(Vcb, parsubvol, parinode, TYPE_DIR_INDEX, dirpos, di, disize, NULL, rollback))
            ERR("error - failed to insert item\n");
        
        // create new INODE_REF entry
        
        Status = add_inode_ref(Vcb, parsubvol, fcb->inode, parinode, dirpos, &utf8, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_inode_ref returned %08x\n", Status);
            return Status;
        }
        
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        fcb->inode_item.st_ctime = now;
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, parsubvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            delete_tree_item(Vcb, &tp, rollback);
        
        free_traverse_ptr(&tp);
        
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
        
        if (!insert_tree_item(Vcb, parsubvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback)) {
            WARN("insert_tree_item failed\n");
        }
    }
    
    // update directory INODE_ITEMs
    
    fcb->par->inode_item.transid = Vcb->superblock.generation;
    fcb->par->inode_item.sequence++;
    fcb->par->inode_item.st_ctime = now;
    fcb->par->inode_item.st_mtime = now;
    
    TRACE("fcb->par->inode_item.st_size was %llx\n", fcb->par->inode_item.st_size);
    if (!tfofcb || (fcb->par->inode == tfofcb->inode && fcb->par->subvol == tfofcb->subvol)) {
        fcb->par->inode_item.st_size += 2 * (utf8.Length - fcb->utf8.Length);
    } else {
        fcb->par->inode_item.st_size -= 2 * fcb->utf8.Length;
        TRACE("tfofcb->inode_item.st_size was %llx\n", tfofcb->inode_item.st_size);
        tfofcb->inode_item.st_size += 2 * utf8.Length;
        TRACE("tfofcb->inode_item.st_size now %llx\n", tfofcb->inode_item.st_size);
        tfofcb->inode_item.transid = Vcb->superblock.generation;
        tfofcb->inode_item.sequence++;
        tfofcb->inode_item.st_ctime = now;
        tfofcb->inode_item.st_mtime = now;
    }
    TRACE("fcb->par->inode_item.st_size now %llx\n", fcb->par->inode_item.st_size);
    
    if (oldfcb && oldfcb->par != fcb->par) {
        TRACE("oldfcb->par->inode_item.st_size was %llx\n", oldfcb->par->inode_item.st_size);
        oldfcb->par->inode_item.st_size -= 2 * oldfcb->utf8.Length;
        TRACE("oldfcb->par->inode_item.st_size now %llx\n", oldfcb->par->inode_item.st_size);
    }
    
    searchkey.obj_id = fcb->par->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, fcb->par->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
        delete_tree_item(Vcb, &tp, rollback);
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ii, &fcb->par->inode_item, sizeof(INODE_ITEM));
    
    if (!insert_tree_item(Vcb, fcb->par->subvol, fcb->par->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback))
        WARN("insert_tree_item failed\n");
    
    free_traverse_ptr(&tp);
    
    if (tfofcb && (fcb->par->inode != tfofcb->inode || fcb->par->subvol != tfofcb->subvol)) {
        searchkey.obj_id = tfofcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, tfofcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            delete_tree_item(Vcb, &tp, rollback);
        
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ii, &tfofcb->inode_item, sizeof(INODE_ITEM));
        
        if (!insert_tree_item(Vcb, tfofcb->subvol, tfofcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback))
            WARN("insert_tree_item failed\n");
        
        free_traverse_ptr(&tp);
    }
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;
    
    // FIXME - handle overwrite by rename here
    FsRtlNotifyFullReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&fcb->full_filename, fcb->name_offset * sizeof(WCHAR), NULL, NULL,
                                fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                across_directories ? FILE_ACTION_REMOVED : FILE_ACTION_RENAMED_OLD_NAME, NULL);

    // FIXME - change full_filename and name_offset of open children
    
    if (fnlen != fcb->filepart.Length / sizeof(WCHAR) || RtlCompareMemory(fn, fcb->filepart.Buffer, fcb->filepart.Length) != fcb->filepart.Length) {
        RtlFreeUnicodeString(&fcb->filepart);
        fcb->filepart.Length = fcb->filepart.MaximumLength = (USHORT)(fnlen * sizeof(WCHAR));
        fcb->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->filepart.Length, ALLOC_TAG);
        
        if (!fcb->filepart.Buffer) {
            ERR("out of memory\n");
            
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(fcb->filepart.Buffer, fn, fcb->filepart.Length);
    }
    
    if (tfo && tfofcb != fcb->par) {
        oldparfcb = fcb->par;
        fcb->par = tfofcb;
        
        fcb->par->refcount++;
        
        RemoveEntryList(&fcb->list_entry);
        InsertTailList(&fcb->par->children, &fcb->list_entry);
        
#ifdef DEBUG_FCB_REFCOUNTS
        WARN("fcb %p: refcount now %i (%.*S)\n", fcb->par, fcb->par->refcount, fcb->par->full_filename.Length / sizeof(WCHAR), fcb->par->full_filename.Buffer);
#endif
        free_fcb(oldparfcb);
    }
    
    ExFreePool(fcb->utf8.Buffer);
    fcb->utf8 = utf8;
    utf8.Buffer = NULL;
    
    // change fcb->full_filename
    
    fcb->full_filename.MaximumLength = fcb->par->full_filename.Length + fcb->filepart.Length;
    if (fcb->par->par) fcb->full_filename.MaximumLength += sizeof(WCHAR);
    ExFreePool(fcb->full_filename.Buffer);
    
    fcb->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->full_filename.MaximumLength, ALLOC_TAG);
    if (!fcb->full_filename.Buffer) {
        ERR("out of memory\n");
        
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(fcb->full_filename.Buffer, fcb->par->full_filename.Buffer, fcb->par->full_filename.Length);
    fcb->full_filename.Length = fcb->par->full_filename.Length;
    
    if (fcb->par->par) {
        fcb->full_filename.Buffer[fcb->full_filename.Length / sizeof(WCHAR)] = '\\';
        fcb->full_filename.Length += sizeof(WCHAR);
    }
    fcb->name_offset = fcb->full_filename.Length / sizeof(WCHAR);
    
    RtlAppendUnicodeStringToString(&fcb->full_filename, &fcb->filepart);
    
    FsRtlNotifyFullReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&fcb->full_filename, fcb->name_offset * sizeof(WCHAR), NULL, NULL,
                                fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                across_directories ? FILE_ACTION_ADDED : FILE_ACTION_RENAMED_NEW_NAME, NULL);
    
    Status = STATUS_SUCCESS;
    
end:
    if (utf8.Buffer)
        ExFreePool(utf8.Buffer);
    
    if (oldfcb)
        free_fcb(oldfcb);
    
    return Status;
}

static NTSTATUS STDCALL stream_set_end_of_file_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, BOOL advance_only, LIST_ENTRY* rollback) {
    FILE_END_OF_FILE_INFORMATION* feofi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    KEY searchkey;
    traverse_ptr tp;
    INODE_ITEM* ii;
    CC_FILE_SIZES ccfs;
    UINT8* data = NULL;
    UINT16 datalen;
    NTSTATUS Status;
    
    TRACE("setting new end to %llx bytes (currently %x)\n", feofi->EndOfFile.QuadPart, fcb->adssize);
    
    if (feofi->EndOfFile.QuadPart < fcb->adssize) {
        if (advance_only)
            return STATUS_SUCCESS;
        
        TRACE("truncating stream to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        if (feofi->EndOfFile.QuadPart > 0) {
            if (!get_xattr(Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, &data, &datalen)) {
                ERR("get_xattr failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        Status = set_xattr(Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, data, feofi->EndOfFile.QuadPart, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            return Status;
        }
        
        fcb->adssize = feofi->EndOfFile.QuadPart;
        
        if (data)
            ExFreePool(data);
    } else if (feofi->EndOfFile.QuadPart > fcb->adssize) {
        UINT16 maxlen;
        UINT8* data2;
        
        TRACE("extending stream to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        // find maximum length of xattr
        maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_XATTR_ITEM;
        searchkey.offset = fcb->adshash;

        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&tp.item->key, &searchkey)) {
            ERR("error - could not find key for xattr\n");
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (tp.item->size < datalen) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, datalen);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
        
        maxlen -= tp.item->size - datalen; // subtract XATTR_ITEM overhead
        
        free_traverse_ptr(&tp);
        
        if (feofi->EndOfFile.QuadPart > maxlen) {
            ERR("error - xattr too long (%llu > %u)\n", feofi->EndOfFile.QuadPart, maxlen);
            return STATUS_DISK_FULL;
        }
        
        if (!get_xattr(Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, &data, &datalen)) {
            ERR("get_xattr failed\n");
            return STATUS_INTERNAL_ERROR;
        }

        data2 = ExAllocatePoolWithTag(PagedPool, feofi->EndOfFile.QuadPart, ALLOC_TAG);
        if (!data2) {
            ERR("out of memory\n");
            ExFreePool(data);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(data2, data, datalen);
        ExFreePool(data);
        
        RtlZeroMemory(&data2[datalen], feofi->EndOfFile.QuadPart - datalen);
        
        Status = set_xattr(Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, data2, feofi->EndOfFile.QuadPart, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            return Status;
        }
        
        fcb->adssize = feofi->EndOfFile.QuadPart;
        
        ExFreePool(data2);
    }

    ccfs.AllocationSize = fcb->Header.AllocationSize;
    ccfs.FileSize = fcb->Header.FileSize;
    ccfs.ValidDataLength = fcb->Header.ValidDataLength;

    CcSetFileSizes(FileObject, &ccfs);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fcb->par->inode_item.transid = Vcb->superblock.generation;
    fcb->par->inode_item.sequence++;
    fcb->par->inode_item.st_ctime = now;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey))
        delete_tree_item(Vcb, &tp, rollback);
    else
        WARN("couldn't find existing INODE_ITEM\n");

    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ii, &fcb->par->inode_item, sizeof(INODE_ITEM));
    insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
    
    free_traverse_ptr(&tp);
    
    fcb->par->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->par->subvol->root_item.ctime = now;

    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL set_end_of_file_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, BOOL advance_only, LIST_ENTRY* rollback) {
    FILE_END_OF_FILE_INFORMATION* feofi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    NTSTATUS Status;
    LARGE_INTEGER time;
    KEY searchkey;
    traverse_ptr tp;
    INODE_ITEM* ii;
    CC_FILE_SIZES ccfs;
    
    if (fcb->deleted)
        return STATUS_FILE_CLOSED;
    
    if (fcb->ads)
        return stream_set_end_of_file_information(Vcb, Irp, FileObject, advance_only, rollback);
    
    TRACE("filename %.*S\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
    TRACE("paging IO: %s\n", Irp->Flags & IRP_PAGING_IO ? "TRUE" : "FALSE");
    TRACE("FileObject: AllocationSize = %llx, FileSize = %llx, ValidDataLength = %llx\n",
        fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
    
//     int3;
    TRACE("setting new end to %llx bytes (currently %llx)\n", feofi->EndOfFile.QuadPart, fcb->inode_item.st_size);
    
//     if (feofi->EndOfFile.QuadPart==0x36c000)
//         int3;
    
    if (feofi->EndOfFile.QuadPart < fcb->inode_item.st_size) {
        if (advance_only)
            return STATUS_SUCCESS;
        
        TRACE("truncating file to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        Status = truncate_file(fcb, feofi->EndOfFile.QuadPart, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - truncate_file failed\n");
            return Status;
        }
    } else if (feofi->EndOfFile.QuadPart > fcb->inode_item.st_size) {
        if (Irp->Flags & IRP_PAGING_IO) {
            TRACE("paging IO tried to extend file size\n");
            return STATUS_SUCCESS;
        }
        
        TRACE("extending file to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        // FIXME - pass flag to say that new extents should be prealloc rather than sparse
        Status = extend_file(fcb, feofi->EndOfFile.QuadPart, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - extend_file failed\n");
            return Status;
        }
    }
    
    ccfs.AllocationSize = fcb->Header.AllocationSize;
    ccfs.FileSize = fcb->Header.FileSize;
    ccfs.ValidDataLength = fcb->Header.ValidDataLength;

    CcSetFileSizes(FileObject, &ccfs);
    TRACE("setting FileSize for %.*S to %llx\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, ccfs.FileSize);
    
    KeQuerySystemTime(&time);
    
    win_time_to_unix(time, &fcb->inode_item.st_mtime);
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey))
        delete_tree_item(Vcb, &tp, rollback);
    else
        WARN("couldn't find existing INODE_ITEM\n");

    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
    insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
    
    free_traverse_ptr(&tp);

    return STATUS_SUCCESS;
}

// static NTSTATUS STDCALL set_allocation_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
//     FILE_ALLOCATION_INFORMATION* fai = (FILE_ALLOCATION_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;
//     fcb* fcb = FileObject->FsContext;
//     
//     FIXME("FIXME\n");
//     ERR("fcb = %p (%.*S)\n", fcb, fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
//     ERR("AllocationSize = %llx\n", fai->AllocationSize.QuadPart);
//     
//     return STATUS_NOT_IMPLEMENTED;
// }

static NTSTATUS STDCALL set_position_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_POSITION_INFORMATION* fpi = (FILE_POSITION_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;
#ifdef DEBUG_LONG_MESSAGES
    fcb* fcb = FileObject->FsContext;
    
    TRACE("setting the position on %.*S to %llx\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fpi->CurrentByteOffset.QuadPart);
#endif
    
    // FIXME - make sure aligned for FO_NO_INTERMEDIATE_BUFFERING
    
    FileObject->CurrentByteOffset = fpi->CurrentByteOffset;
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    fcb* fcb = IrpSp->FileObject->FsContext;
    BOOL top_level;
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }
    
    if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Irp->IoStatus.Information = 0;
    
    Status = STATUS_NOT_IMPLEMENTED;

    TRACE("set information\n");
    
    acquire_tree_lock(Vcb, TRUE);

    switch (IrpSp->Parameters.SetFile.FileInformationClass) {
        case FileAllocationInformation:
            TRACE("FileAllocationInformation\n");
            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, FALSE, &rollback);
            break;

        case FileBasicInformation:
            TRACE("FileBasicInformation\n");
            Status = set_basic_information(Vcb, Irp, IrpSp->FileObject, &rollback);
            break;

        case FileDispositionInformation:
            TRACE("FileDispositionInformation\n");
            Status = set_disposition_information(Vcb, Irp, IrpSp->FileObject);
            break;

        case FileEndOfFileInformation:
            TRACE("FileEndOfFileInformation\n");
            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.AdvanceOnly, &rollback);
            break;

        case FileLinkInformation:
            FIXME("STUB: FileLinkInformation\n");
            break;

        case FilePositionInformation:
            TRACE("FilePositionInformation\n");
            Status = set_position_information(Vcb, Irp, IrpSp->FileObject);
            break;

        case FileRenameInformation:
            TRACE("FileRenameInformation\n");
            // FIXME - make this work with streams
            Status = set_rename_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject, IrpSp->Parameters.SetFile.ReplaceIfExists, &rollback);
            break;

        case FileValidDataLengthInformation:
            FIXME("STUB: FileValidDataLengthInformation\n");
            break;
            
#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileNormalizedNameInformation:
            FIXME("STUB: FileNormalizedNameInformation\n");
            break;
#endif

            
#if (NTDDI_VERSION >= NTDDI_WIN7)
        case FileStandardLinkInformation:
            FIXME("STUB: FileStandardLinkInformation\n");
            break;
            
        case FileRemoteProtocolInformation:
            FIXME("STUB: FileRemoteProtocolInformation\n");
            break;
#endif
            
        default:
            WARN("unknown FileInformationClass %u\n", IrpSp->Parameters.SetFile.FileInformationClass);
    }
    
    if (NT_SUCCESS(Status))
        Status = consider_write(Vcb);
    
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);
    
    release_tree_lock(Vcb, TRUE);
    
end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL fill_in_file_basic_information(FILE_BASIC_INFORMATION* fbi, INODE_ITEM* ii, LONG* length, fcb* fcb) {
    RtlZeroMemory(fbi, sizeof(FILE_BASIC_INFORMATION));
    
    *length -= sizeof(FILE_BASIC_INFORMATION);
    
    fbi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
    fbi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
    fbi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
    fbi->ChangeTime.QuadPart = 0;
    fbi->FileAttributes = fcb->ads ? fcb->par->atts : fcb->atts;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_network_open_information(FILE_NETWORK_OPEN_INFORMATION* fnoi, fcb* fcb, LONG* length) {
    INODE_ITEM* ii;
    
    if (*length < sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
        WARN("overflow\n");
        return STATUS_BUFFER_OVERFLOW;
    }
    
    RtlZeroMemory(fnoi, sizeof(FILE_NETWORK_OPEN_INFORMATION));
    
    *length -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    
    if (fcb->ads)
        ii = &fcb->par->inode_item;
    else
        ii = &fcb->inode_item;
    
    
    fnoi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
    fnoi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
    fnoi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
    fnoi->ChangeTime.QuadPart = 0;
    
    if (fcb->ads) {
        fnoi->AllocationSize.QuadPart = fnoi->EndOfFile.QuadPart = fcb->adssize;
        fnoi->FileAttributes = fcb->par->atts;
    } else {
        fnoi->AllocationSize.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        fnoi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fnoi->FileAttributes = fcb->atts;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_standard_information(FILE_STANDARD_INFORMATION* fsi, fcb* fcb, LONG* length) {
    RtlZeroMemory(fsi, sizeof(FILE_STANDARD_INFORMATION));
    
    *length -= sizeof(FILE_STANDARD_INFORMATION);
    
    if (fcb->ads) {
        fsi->AllocationSize.QuadPart = fsi->EndOfFile.QuadPart = fcb->adssize;
        fsi->NumberOfLinks = fcb->par->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fcb->par->inode_item.st_mode);
    } else {
        fsi->AllocationSize.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        fsi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fcb->inode_item.st_mode);
    }
    
    TRACE("length = %llu\n", fsi->EndOfFile.QuadPart);
    
    fsi->DeletePending = fcb->delete_on_close;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_internal_information(FILE_INTERNAL_INFORMATION* fii, UINT64 inode, LONG* length) {
    *length -= sizeof(FILE_INTERNAL_INFORMATION);
    
    fii->IndexNumber.QuadPart = inode;
    
    return STATUS_SUCCESS;
}  
    
static NTSTATUS STDCALL fill_in_file_ea_information(FILE_EA_INFORMATION* eai, LONG* length) {
    *length -= sizeof(FILE_EA_INFORMATION);
    
    eai->EaSize = 0;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_access_information(FILE_ACCESS_INFORMATION* fai, LONG* length) {
    *length -= sizeof(FILE_ACCESS_INFORMATION);
    
    fai->AccessFlags = GENERIC_READ;
    
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS STDCALL fill_in_file_position_information(FILE_POSITION_INFORMATION* fpi, PFILE_OBJECT FileObject, LONG* length) {
    RtlZeroMemory(fpi, sizeof(FILE_POSITION_INFORMATION));
    
    *length -= sizeof(FILE_POSITION_INFORMATION);
    
    fpi->CurrentByteOffset = FileObject->CurrentByteOffset;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_mode_information(FILE_MODE_INFORMATION* fmi, ccb* ccb, LONG* length) {
    RtlZeroMemory(fmi, sizeof(FILE_MODE_INFORMATION));
    
    *length -= sizeof(FILE_MODE_INFORMATION);
    
    if (ccb->options & FILE_WRITE_THROUGH)
        fmi->Mode |= FILE_WRITE_THROUGH;
    
    if (ccb->options & FILE_SEQUENTIAL_ONLY)
        fmi->Mode |= FILE_SEQUENTIAL_ONLY;
    
    if (ccb->options & FILE_NO_INTERMEDIATE_BUFFERING)
        fmi->Mode |= FILE_NO_INTERMEDIATE_BUFFERING;
    
    if (ccb->options & FILE_SYNCHRONOUS_IO_ALERT)
        fmi->Mode |= FILE_SYNCHRONOUS_IO_ALERT;
    
    if (ccb->options & FILE_SYNCHRONOUS_IO_NONALERT)
        fmi->Mode |= FILE_SYNCHRONOUS_IO_NONALERT;
    
    if (ccb->options & FILE_DELETE_ON_CLOSE)
        fmi->Mode |= FILE_DELETE_ON_CLOSE;
        
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_alignment_information(FILE_ALIGNMENT_INFORMATION* fai, device_extension* Vcb, LONG* length) {
    RtlZeroMemory(fai, sizeof(FILE_ALIGNMENT_INFORMATION));
    
    *length -= sizeof(FILE_ALIGNMENT_INFORMATION);
    
    fai->AlignmentRequirement = Vcb->devices[0].devobj->AlignmentRequirement;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_name_information(FILE_NAME_INFORMATION* fni, fcb* fcb, LONG* length) {
#ifdef _DEBUG
    ULONG retlen = 0;
#endif
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    ULONG datasuflen = wcslen(datasuf) * sizeof(WCHAR);
    
    RtlZeroMemory(fni, sizeof(FILE_NAME_INFORMATION));
    
    *length -= (LONG)offsetof(FILE_NAME_INFORMATION, FileName[0]);
    
    TRACE("maximum length is %u\n", *length);
    fni->FileNameLength = 0;
    
    fni->FileName[0] = 0;
    
    if (*length >= (LONG)fcb->full_filename.Length) {
        RtlCopyMemory(fni->FileName, fcb->full_filename.Buffer, fcb->full_filename.Length);
#ifdef _DEBUG
        retlen = fcb->full_filename.Length;
#endif
        *length -= fcb->full_filename.Length;
    } else {
        if (*length > 0) {
            RtlCopyMemory(fni->FileName, fcb->full_filename.Buffer, *length);
#ifdef _DEBUG
            retlen = *length;
#endif
        }
        *length = -1;
    }
    
    fni->FileNameLength = fcb->full_filename.Length;
    
    if (fcb->ads) {
        if (*length >= (LONG)datasuflen) {
            RtlCopyMemory(&fni->FileName[fcb->full_filename.Length / sizeof(WCHAR)], datasuf, datasuflen);
#ifdef _DEBUG
            retlen += datasuflen;
#endif
            *length -= datasuflen;
        } else {
            if (*length > 0) {
                RtlCopyMemory(&fni->FileName[fcb->full_filename.Length / sizeof(WCHAR)], datasuf, *length);
#ifdef _DEBUG
                retlen += *length;
#endif
            }
            *length = -1;
        }
    }
    
    TRACE("%.*S\n", retlen / sizeof(WCHAR), fni->FileName);

    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_attribute_information(FILE_ATTRIBUTE_TAG_INFORMATION* ati, fcb* fcb, LONG* length) {
    *length -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    
    ati->FileAttributes = fcb->ads ? fcb->par->atts : fcb->atts;
    ati->ReparseTag = 0;
    
    return STATUS_SUCCESS;
}

typedef struct {
    UNICODE_STRING name;
    UINT64 size;
} stream_info;

static NTSTATUS STDCALL fill_in_file_stream_information(FILE_STREAM_INFORMATION* fsi, fcb* fcb, LONG* length) {
    ULONG reqsize;
    UINT64 i, num_streams;
    stream_info* streams;
    FILE_STREAM_INFORMATION* entry;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    static char xapref[] = "user.";
    UNICODE_STRING suf;
    
    suf.Buffer = datasuf;
    suf.Length = suf.MaximumLength = wcslen(datasuf) * sizeof(WCHAR);
    
    num_streams = 1;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_XATTR_ITEM) {
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            } else {
                ULONG len = tp.item->size;
                DIR_ITEM* xa = (DIR_ITEM*)tp.item->data;
                
                do {
                    if (len < sizeof(DIR_ITEM) || len < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                        ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        break;
                    }
                    
                    if (xa->n > strlen(xapref) && RtlCompareMemory(xa->name, xapref, strlen(xapref)) == strlen(xapref)) {
                        if (tp.item->key.offset != EA_DOSATTRIB_HASH || xa->n != strlen(EA_DOSATTRIB) || RtlCompareMemory(xa->name, EA_DOSATTRIB, xa->n) != xa->n) {
                            num_streams++;
                        }
                    }
                    
                    len -= sizeof(DIR_ITEM) - sizeof(char) + xa->n + xa->m;
                    xa = (DIR_ITEM*)&xa->name[xa->n + xa->m]; // FIXME - test xattr hash collisions work
                } while (len > 0);
            }
        }
        
        b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);

    free_traverse_ptr(&tp);
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    streams = ExAllocatePoolWithTag(PagedPool, sizeof(stream_info) * num_streams, ALLOC_TAG);
    if (!streams) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    reqsize = 0;
    
    streams[0].name.Length = streams[0].name.MaximumLength = 0;
    streams[0].name.Buffer = NULL;
    streams[0].size = fcb->inode_item.st_size;
    reqsize += sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + streams[0].name.Length;
    
    i = 1;
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_XATTR_ITEM) {
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            } else {
                ULONG len = tp.item->size;
                DIR_ITEM* xa = (DIR_ITEM*)tp.item->data;
                ULONG stringlen;
                
                do {
                    if (len < sizeof(DIR_ITEM) || len < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                        ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        break;
                    }
                    
                    if (xa->n > strlen(xapref) && RtlCompareMemory(xa->name, xapref, strlen(xapref)) == strlen(xapref) &&
                        (tp.item->key.offset != EA_DOSATTRIB_HASH || xa->n != strlen(EA_DOSATTRIB) || RtlCompareMemory(xa->name, EA_DOSATTRIB, xa->n) != xa->n)) {
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, &xa->name[strlen(xapref)], xa->n - strlen(xapref));
                        if (!NT_SUCCESS(Status)) {
                            UINT64 j;
                            
                            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                            
                            for (j = i; j < num_streams; j++)
                                streams[j].name.Buffer = NULL;
                            
                            goto end;
                        }
                        
                        streams[i].name.Buffer = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!streams[i].name.Buffer) {
                            UINT64 j;
                            
                            ERR("out of memory\n");
                            
                            for (j = i+1; j < num_streams; j++)
                                streams[j].name.Buffer = NULL;
                            
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                            
                        Status = RtlUTF8ToUnicodeN(streams[i].name.Buffer, stringlen, &stringlen, &xa->name[strlen(xapref)], xa->n - strlen(xapref));
                        
                        if (!NT_SUCCESS(Status)) {
                            UINT64 j;
                            
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                            
                            ExFreePool(streams[i].name.Buffer);
                            for (j = i; j < num_streams; j++)
                                streams[j].name.Buffer = NULL;
                            
                            goto end;
                        }
                        
                        streams[i].name.Length = streams[i].name.MaximumLength = stringlen;
                        
                        streams[i].size = xa->m;
                        reqsize = sector_align(reqsize, sizeof(LONGLONG));
                        reqsize += sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + streams[i].name.Length;
                        
                        TRACE("streams[%llu].name = %.*S (length = %u)\n", i, streams[i].name.Length / sizeof(WCHAR), streams[i].name.Buffer, streams[i].name.Length / sizeof(WCHAR));

                        i++;
                    }
                    
                    len -= sizeof(DIR_ITEM) - sizeof(char) + xa->n + xa->m;
                    xa = (DIR_ITEM*)&xa->name[xa->n + xa->m]; // FIXME - test xattr hash collisions work
                } while (len > 0);
            }
        }
        
        b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);    
    
    free_traverse_ptr(&tp);
    
    TRACE("length = %i, reqsize = %u\n", *length, reqsize);
    
    if (reqsize > *length) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto end;
    }
    
    entry = fsi;
    for (i = 0; i < num_streams; i++) {
        entry->StreamNameLength = streams[i].name.Length + suf.Length + sizeof(WCHAR);
        entry->StreamSize.QuadPart = streams[i].size;
        
        if (i == 0)
            entry->StreamAllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        else
            entry->StreamAllocationSize.QuadPart = streams[i].size;
        
        entry->StreamName[0] = ':';
        
        if (streams[i].name.Length > 0)
            RtlCopyMemory(&entry->StreamName[1], streams[i].name.Buffer, streams[i].name.Length);
        
        RtlCopyMemory(&entry->StreamName[1 + (streams[i].name.Length / sizeof(WCHAR))], suf.Buffer, suf.Length);
        
        if (i == num_streams - 1)
            entry->NextEntryOffset = 0;
        else {
            entry->NextEntryOffset = sector_align(sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + streams[i].name.Length, sizeof(LONGLONG));

            entry = (FILE_STREAM_INFORMATION*)((UINT8*)entry + entry->NextEntryOffset);
        }
    }
    
    *length -= reqsize;
    
    Status = STATUS_SUCCESS;
    
end:
    for (i = 0; i < num_streams; i++) {
        if (streams[i].name.Buffer)
            ExFreePool(streams[i].name.Buffer);
    }
    
    ExFreePool(streams);
    
    return Status;
}

static NTSTATUS STDCALL fill_in_file_standard_link_information(FILE_STANDARD_LINK_INFORMATION* fsli, fcb* fcb, LONG* length) {
    TRACE("FileStandardLinkInformation\n");
    
    // FIXME - NumberOfAccessibleLinks should subtract open links which have been marked as delete_on_close
    
    fsli->NumberOfAccessibleLinks = fcb->inode_item.st_nlink;
    fsli->TotalNumberOfLinks = fcb->inode_item.st_nlink;
    fsli->DeletePending = fcb->delete_on_close;
    fsli->Directory = fcb->type == BTRFS_TYPE_DIRECTORY ? TRUE : FALSE;
    
    *length -= sizeof(FILE_STANDARD_LINK_INFORMATION);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL query_info(device_extension* Vcb, PFILE_OBJECT FileObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    LONG length = IrpSp->Parameters.QueryFile.Length;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p)\n", Vcb, FileObject, Irp);
    TRACE("fcb = %p\n", fcb);
    
    if (fcb == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;
    
    switch (IrpSp->Parameters.QueryFile.FileInformationClass) {
        case FileAllInformation:
        {
            FILE_ALL_INFORMATION* fai = Irp->AssociatedIrp.SystemBuffer;
            INODE_ITEM* ii;
            
            TRACE("FileAllInformation\n");
            
            if (fcb->ads)
                ii = &fcb->par->inode_item;
            else
                ii = &fcb->inode_item;
            
            if (length > 0)
                fill_in_file_basic_information(&fai->BasicInformation, ii, &length, fcb);
            
            if (length > 0)
                fill_in_file_standard_information(&fai->StandardInformation, fcb, &length);
            
            if (length > 0)
                fill_in_file_internal_information(&fai->InternalInformation, fcb->inode, &length);
            
            if (length > 0)
                fill_in_file_ea_information(&fai->EaInformation, &length);
            
            if (length > 0)
                fill_in_file_access_information(&fai->AccessInformation, &length);
            
            if (length > 0)
                fill_in_file_position_information(&fai->PositionInformation, FileObject, &length);
                
            if (length > 0)
                fill_in_file_mode_information(&fai->ModeInformation, ccb, &length);
            
            if (length > 0)
                fill_in_file_alignment_information(&fai->AlignmentInformation, Vcb, &length);
            
            if (length > 0)
                fill_in_file_name_information(&fai->NameInformation, fcb, &length);
            
            Status = STATUS_SUCCESS;

            break;
        }

        case FileAttributeTagInformation:
        {
            FILE_ATTRIBUTE_TAG_INFORMATION* ati = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileAttributeTagInformation\n");
            
            Status = fill_in_file_attribute_information(ati, fcb, &length);
            
            break;
        }

        case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION* fbi = Irp->AssociatedIrp.SystemBuffer;
            INODE_ITEM* ii;
            
            TRACE("FileBasicInformation\n");
            
            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_BASIC_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }
            
            if (fcb->ads)
                ii = &fcb->par->inode_item;
            else
                ii = &fcb->inode_item;
            
            Status = fill_in_file_basic_information(fbi, ii, &length, fcb);
            break;
        }

        case FileCompressionInformation:
            FIXME("STUB: FileCompressionInformation\n");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;

        case FileEaInformation:
        {
            FILE_EA_INFORMATION* eai = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileEaInformation\n");
            
            Status = fill_in_file_ea_information(eai, &length);
            
            break;
        }

        case FileInternalInformation:
        {
            FILE_INTERNAL_INFORMATION* fii = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileInternalInformation\n");
            
            Status = fill_in_file_internal_information(fii, fcb->inode, &length);
            
            break;
        }

        case FileNameInformation:
        {
            FILE_NAME_INFORMATION* fni = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNameInformation\n");
            
            Status = fill_in_file_name_information(fni, fcb, &length);
            
            break;
        }

        case FileNetworkOpenInformation:
        {
            FILE_NETWORK_OPEN_INFORMATION* fnoi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNetworkOpenInformation\n");
            
            Status = fill_in_file_network_open_information(fnoi, fcb, &length);

            break;
        }

        case FilePositionInformation:
        {
            FILE_POSITION_INFORMATION* fpi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FilePositionInformation\n");
            
            Status = fill_in_file_position_information(fpi, FileObject, &length);
            
            break;
        }

        case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION* fsi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileStandardInformation\n");
            
            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_STANDARD_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }
            
            Status = fill_in_file_standard_information(fsi, fcb, &length);
            
            break;
        }

        case FileStreamInformation:
        {
            FILE_STREAM_INFORMATION* fsi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileStreamInformation\n");
            
            Status = fill_in_file_stream_information(fsi, fcb, &length);

            break;
        }

#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileHardLinkInformation:
            FIXME("STUB: FileHardLinkInformation\n");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
            
        case FileNormalizedNameInformation:
        {
            FILE_NAME_INFORMATION* fni = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNormalizedNameInformation\n");
            
            Status = fill_in_file_name_information(fni, fcb, &length);
            
            break;
        }
#endif

            
#if (NTDDI_VERSION >= NTDDI_WIN7)
        case FileStandardLinkInformation:
        {
            FILE_STANDARD_LINK_INFORMATION* fsli = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileStandardLinkInformation\n");
            
            Status = fill_in_file_standard_link_information(fsli, fcb, &length);
            
            break;
        }
        
        case FileRemoteProtocolInformation:
            FIXME("STUB: FileRemoteProtocolInformation\n");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
#endif
        
        default:
            WARN("unknown FileInformationClass %u\n", IrpSp->Parameters.QueryFile.FileInformationClass);
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
    }
    
    if (length < 0) {
        length = 0;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    
    Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - length;

exit:    
    TRACE("query_info returning %08x\n", Status);
    
    return Status;
}

NTSTATUS STDCALL drv_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    fcb* fcb;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level;
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    Irp->IoStatus.Information = 0;
    
    TRACE("query information\n");
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    acquire_tree_lock(Vcb, FALSE);
    
    fcb = IrpSp->FileObject->FsContext;
    TRACE("fcb = %p\n", fcb);
    TRACE("fcb->subvol = %p\n", fcb->subvol);
    
    Status = query_info(fcb->Vcb, IrpSp->FileObject, Irp);
    
    TRACE("returning %08x\n", Status);
    
    Irp->IoStatus.Status = Status;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    release_tree_lock(Vcb, FALSE);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    return Status;
}
