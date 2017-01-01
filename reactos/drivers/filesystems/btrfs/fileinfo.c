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

#if (NTDDI_VERSION >= NTDDI_WIN10)
// not currently in mingw - introduced with Windows 10
#ifndef FileIdInformation
#define FileIdInformation (enum _FILE_INFORMATION_CLASS)59
#endif
#endif

static NTSTATUS get_inode_dir_path(device_extension* Vcb, root* subvol, UINT64 inode, PUNICODE_STRING us, PIRP Irp);

static NTSTATUS STDCALL set_basic_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_BASIC_INFORMATION* fbi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    ULONG defda, filter = 0;
    BOOL inode_item_changed = FALSE;
    NTSTATUS Status;
    
    if (fcb->ads) {
        if (fileref && fileref->parent)
            fcb = fileref->parent->fcb;
        else {
            ERR("stream did not have fileref\n");
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    TRACE("file = %S, attributes = %x\n", file_desc(FileObject), fbi->FileAttributes);
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fbi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY && fcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("attempted to set FILE_ATTRIBUTE_DIRECTORY on non-directory\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb->inode == SUBVOL_ROOT_INODE && fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY &&
        (fbi->FileAttributes == 0 || fbi->FileAttributes & FILE_ATTRIBUTE_READONLY)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    if (fbi->CreationTime.QuadPart == -1)
        ccb->user_set_creation_time = TRUE;
    else if (fbi->CreationTime.QuadPart != 0) {
        win_time_to_unix(fbi->CreationTime, &fcb->inode_item.otime);
        inode_item_changed = TRUE;
        filter |= FILE_NOTIFY_CHANGE_CREATION;
        
        ccb->user_set_creation_time = TRUE;
    }
    
    if (fbi->LastAccessTime.QuadPart == -1)
        ccb->user_set_access_time = TRUE;
    else if (fbi->LastAccessTime.QuadPart != 0) {
        win_time_to_unix(fbi->LastAccessTime, &fcb->inode_item.st_atime);
        inode_item_changed = TRUE;
        filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        
        ccb->user_set_access_time = TRUE;
    }
    
    if (fbi->LastWriteTime.QuadPart == -1)
        ccb->user_set_write_time = TRUE;
    else if (fbi->LastWriteTime.QuadPart != 0) {
        win_time_to_unix(fbi->LastWriteTime, &fcb->inode_item.st_mtime);
        inode_item_changed = TRUE;
        filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        
        ccb->user_set_write_time = TRUE;
    }
    
    if (fbi->ChangeTime.QuadPart == -1)
        ccb->user_set_change_time = TRUE;
    else if (fbi->ChangeTime.QuadPart != 0) {
        win_time_to_unix(fbi->ChangeTime, &fcb->inode_item.st_ctime);
        inode_item_changed = TRUE;
        // no filter for this
        
        ccb->user_set_change_time = TRUE;
    }
    
    // FileAttributes == 0 means don't set - undocumented, but seen in fastfat
    if (fbi->FileAttributes != 0) {
        LARGE_INTEGER time;
        BTRFS_TIME now;
        
        defda = get_file_attributes(Vcb, &fcb->inode_item, fcb->subvol, fcb->inode, fcb->type, fileref->filepart.Length > 0 && fileref->filepart.Buffer[0] == '.', TRUE, Irp);
        
        if (fcb->type == BTRFS_TYPE_DIRECTORY)
            fbi->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        else if (fcb->type == BTRFS_TYPE_SYMLINK)
            fbi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
                
        fcb->atts_changed = TRUE;
        
        if (fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)
            fbi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
        
        if (defda == fbi->FileAttributes)
            fcb->atts_deleted = TRUE;
        
        fcb->atts = fbi->FileAttributes;
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;
        
        fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
        
        if (fcb->inode == SUBVOL_ROOT_INODE) {
            if (fbi->FileAttributes & FILE_ATTRIBUTE_READONLY)
                fcb->subvol->root_item.flags |= BTRFS_SUBVOL_READONLY;
            else
                fcb->subvol->root_item.flags &= ~BTRFS_SUBVOL_READONLY;
        }
        
        inode_item_changed = TRUE;
        
        filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    }

    if (inode_item_changed) {
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        fcb->inode_item_changed = TRUE;
        
        mark_fcb_dirty(fcb);
    }
    
    if (filter != 0)
        send_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);
    
    return Status;
}

static NTSTATUS STDCALL set_disposition_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_DISPOSITION_INFORMATION* fdi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    ULONG atts;
    NTSTATUS Status;
    
    if (!fileref)
        return STATUS_INVALID_PARAMETER;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    TRACE("changing delete_on_close to %s for %S (fcb %p)\n", fdi->DeleteFile ? "TRUE" : "FALSE", file_desc(FileObject), fcb);
    
    if (fcb->ads) {
        if (fileref->parent)
            atts = fileref->parent->fcb->atts;
        else {
            ERR("no fileref for stream\n");
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
    } else
        atts = fcb->atts;
    
    TRACE("atts = %x\n", atts);
    
    if (atts & FILE_ATTRIBUTE_READONLY) {
        Status = STATUS_CANNOT_DELETE;
        goto end;
    }
    
    // FIXME - can we skip this bit for subvols?
    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0) {
        Status = STATUS_DIRECTORY_NOT_EMPTY;
        goto end;
    }
    
    if (!MmFlushImageSection(&fcb->nonpaged->segment_object, MmFlushForDelete)) {
        WARN("trying to delete file which is being mapped as an image\n");
        Status = STATUS_CANNOT_DELETE;
        goto end;
    }
    
    ccb->fileref->delete_on_close = fdi->DeleteFile;
    
    FileObject->DeletePending = fdi->DeleteFile;
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    
    ExReleaseResourceLite(&Vcb->fcb_lock);

    return Status;
}

BOOL has_open_children(file_ref* fileref) {
    LIST_ENTRY* le = fileref->children.Flink;
    
    if (IsListEmpty(&fileref->children))
        return FALSE;
        
    while (le != &fileref->children) {
        file_ref* c = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (c->open_count > 0)
            return TRUE;
        
        if (has_open_children(c))
            return TRUE;

        le = le->Flink;
    }
    
    return FALSE;
}

static NTSTATUS duplicate_fcb(fcb* oldfcb, fcb** pfcb) {
    device_extension* Vcb = oldfcb->Vcb;
    fcb* fcb;
    LIST_ENTRY* le;
    
    // FIXME - we can skip a lot of this if the inode is about to be deleted
    
    fcb = create_fcb(PagedPool); // FIXME - what if we duplicate the paging file?
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fcb->Vcb = Vcb;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize = oldfcb->Header.AllocationSize;
    fcb->Header.FileSize = oldfcb->Header.FileSize;
    fcb->Header.ValidDataLength = oldfcb->Header.ValidDataLength;
    
    fcb->type = oldfcb->type;
    
    if (oldfcb->ads) {
        fcb->ads = TRUE;
        fcb->adshash = oldfcb->adshash;
        fcb->adsmaxlen = oldfcb->adsmaxlen;
        
        if (oldfcb->adsxattr.Buffer && oldfcb->adsxattr.Length > 0) {
            fcb->adsxattr.Length = oldfcb->adsxattr.Length;
            fcb->adsxattr.MaximumLength = fcb->adsxattr.Length + 1;
            fcb->adsxattr.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->adsxattr.MaximumLength, ALLOC_TAG);
            
            if (!fcb->adsxattr.Buffer) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(fcb->adsxattr.Buffer, oldfcb->adsxattr.Buffer, fcb->adsxattr.Length);
            fcb->adsxattr.Buffer[fcb->adsxattr.Length] = 0;
        }
        
        if (oldfcb->adsdata.Buffer && oldfcb->adsdata.Length > 0) {
            fcb->adsdata.Length = fcb->adsdata.MaximumLength = oldfcb->adsdata.Length;
            fcb->adsdata.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->adsdata.MaximumLength, ALLOC_TAG);
            
            if (!fcb->adsdata.Buffer) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(fcb->adsdata.Buffer, oldfcb->adsdata.Buffer, fcb->adsdata.Length);
        }
        
        goto end;
    }
    
    RtlCopyMemory(&fcb->inode_item, &oldfcb->inode_item, sizeof(INODE_ITEM));
    fcb->inode_item_changed = TRUE;
    
    if (oldfcb->sd && RtlLengthSecurityDescriptor(oldfcb->sd) > 0) {
        fcb->sd = ExAllocatePoolWithTag(PagedPool, RtlLengthSecurityDescriptor(oldfcb->sd), ALLOC_TAG);
        if (!fcb->sd) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fcb->sd, oldfcb->sd, RtlLengthSecurityDescriptor(oldfcb->sd));
    }
    
    fcb->atts = oldfcb->atts;
    
    le = oldfcb->extents.Flink;
    while (le != &oldfcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        
        if (!ext->ignore) {
            extent* ext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
            
            if (!ext2) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            ext2->offset = ext->offset;
            ext2->datalen = ext->datalen;
            
            if (ext2->datalen > 0) {
                ext2->data = ExAllocatePoolWithTag(PagedPool, ext2->datalen, ALLOC_TAG);
                
                if (!ext2->data) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(ext2->data, ext->data, ext2->datalen);
            } else
                ext2->data = NULL;
            
            ext2->unique = FALSE;
            ext2->ignore = FALSE;
            ext2->inserted = TRUE;
            
            if (ext->csum) {
                ULONG len;
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                
                if (ext->data->compression == BTRFS_COMPRESSION_NONE)
                    len = ed2->num_bytes;
                else
                    len = ed2->size;
                
                len = len * sizeof(UINT32) / Vcb->superblock.sector_size;
                
                ext2->csum = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
                if (!ext2->csum) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(ext2->csum, ext->csum, len);
            } else
                ext2->csum = NULL;

            InsertTailList(&fcb->extents, &ext2->list_entry);
        }
        
        le = le->Flink;
    }
    
    le = oldfcb->hardlinks.Flink;
    while (le != &oldfcb->hardlinks) {
        hardlink *hl = CONTAINING_RECORD(le, hardlink, list_entry), *hl2;
        
        hl2 = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
        
        if (!hl2) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        hl2->parent = hl->parent;
        hl2->index = hl->index;
        
        hl2->name.Length = hl2->name.MaximumLength = hl->name.Length;
        hl2->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl2->name.MaximumLength, ALLOC_TAG);
        
        if (!hl2->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl2);
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(hl2->name.Buffer, hl->name.Buffer, hl->name.Length);
        
        hl2->utf8.Length = hl2->utf8.MaximumLength = hl->utf8.Length;
        hl2->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl2->utf8.MaximumLength, ALLOC_TAG);
        
        if (!hl2->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl2->name.Buffer);
            ExFreePool(hl2);
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(hl2->utf8.Buffer, hl->utf8.Buffer, hl->utf8.Length);
        
        InsertTailList(&fcb->hardlinks, &hl2->list_entry);
        
        le = le->Flink;
    }
    
    fcb->last_dir_index = oldfcb->last_dir_index;
    
    if (oldfcb->reparse_xattr.Buffer && oldfcb->reparse_xattr.Length > 0) {
        fcb->reparse_xattr.Length = fcb->reparse_xattr.MaximumLength = oldfcb->reparse_xattr.Length;
        
        fcb->reparse_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->reparse_xattr.MaximumLength, ALLOC_TAG);
        if (!fcb->reparse_xattr.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fcb->reparse_xattr.Buffer, oldfcb->reparse_xattr.Buffer, fcb->reparse_xattr.Length);
    }
    
    if (oldfcb->ea_xattr.Buffer && oldfcb->ea_xattr.Length > 0) {
        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = oldfcb->ea_xattr.Length;
        
        fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->ea_xattr.MaximumLength, ALLOC_TAG);
        if (!fcb->ea_xattr.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fcb->ea_xattr.Buffer, oldfcb->ea_xattr.Buffer, fcb->ea_xattr.Length);
    }

end:
    *pfcb = fcb;
    
    return STATUS_SUCCESS;
}

typedef struct _move_entry {
    file_ref* fileref;
    fcb* dummyfcb;
    file_ref* dummyfileref;
    struct _move_entry* parent;
    LIST_ENTRY list_entry;
} move_entry;

static NTSTATUS add_children_to_move_list(move_entry* me, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    BOOL b;
    LIST_ENTRY* le;
    move_entry* me2;
    
    static char xapref[] = "user.";
    ULONG xapreflen = strlen(xapref);
    
    ExAcquireResourceSharedLite(&me->fileref->nonpaged->children_lock, TRUE);
    
    le = me->fileref->children.Flink;
    while (le != &me->fileref->children) {
        file_ref* fr = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (!fr->deleted) {
            me2 = ExAllocatePoolWithTag(PagedPool, sizeof(move_entry), ALLOC_TAG);
            if (!me2) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            me2->fileref = fr;
            
            increase_fileref_refcount(fr);

            me2->dummyfcb = NULL;
            me2->dummyfileref = NULL;
            me2->parent = me;
            
            InsertHeadList(&me->list_entry, &me2->list_entry);
        }
        
        le = le->Flink;
    }
    
    searchkey.obj_id = me->fileref->fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(me->fileref->fcb->Vcb, me->fileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    do {
        traverse_ptr next_tp;
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            DIR_ITEM* xa = (DIR_ITEM*)tp.item->data;
            ULONG len;
            
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            len = tp.item->size;
            
            do {
                if (len < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
                
                if (xa->n > xapreflen && RtlCompareMemory(xa->name, xapref, xapreflen) == xapreflen &&
                    (tp.item->key.offset != EA_DOSATTRIB_HASH || xa->n != strlen(EA_DOSATTRIB) || RtlCompareMemory(xa->name, EA_DOSATTRIB, xa->n) != xa->n) &&
                    (tp.item->key.offset != EA_EA_HASH || xa->n != strlen(EA_EA) || RtlCompareMemory(xa->name, EA_EA, xa->n) != xa->n)
                ) {
                    BOOL found = FALSE;
                
                    le = me->fileref->children.Flink;
                    
                    while (le != &me->fileref->children) {
                        file_ref* fr = CONTAINING_RECORD(le, file_ref, list_entry);

                        if (fr->fcb->ads && fr->fcb->adshash == tp.item->key.offset && fr->fcb->adsxattr.Length == xa->n &&
                            RtlCompareMemory(fr->fcb->adsxattr.Buffer, xa->name, xa->n) == xa->n) {
                            found = TRUE;
                            break;
                        }
                         
                        le = le->Flink;
                    }
                    
                    if (!found) {
                        fcb* fcb;
                        file_ref* fr;
                        ANSI_STRING xattr;
                        ULONG stringlen;
                        
                        xattr.Length = xa->n;
                        xattr.MaximumLength = xattr.Length + 1;
                        xattr.Buffer = ExAllocatePoolWithTag(PagedPool, xattr.MaximumLength, ALLOC_TAG);
                        
                        if (!xattr.Buffer) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        RtlCopyMemory(xattr.Buffer, xa->name, xa->n);
                        xattr.Buffer[xa->n] = 0;
                        
                        Status = open_fcb_stream(me->fileref->fcb->Vcb, me->fileref->fcb->subvol, me->fileref->fcb->inode, &xattr,
                                                 tp.item->key.offset, me->fileref->fcb, &fcb, Irp);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("open_fcb_stream returned %08x\n", Status);
                            ExFreePool(xattr.Buffer);
                            goto end;
                        }
                        
                        fr = create_fileref();
                        if (!fr) {
                            ERR("out of memory\n");
                            free_fcb(fcb);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        fr->fcb = fcb;
                        
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, &xa->name[xapreflen], xa->n - xapreflen);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }
                        
                        fr->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!fr->filepart.Buffer) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            free_fileref(fr);
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(fr->filepart.Buffer, stringlen, &stringlen, &xa->name[xapreflen], xa->n - xapreflen);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }
                        
                        fr->filepart.Length = fr->filepart.MaximumLength = stringlen;
                        
                        Status = RtlUpcaseUnicodeString(&fr->filepart_uc, &fr->filepart, TRUE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }

                        fr->parent = (struct _file_ref*)me->fileref;
                        increase_fileref_refcount(fr->parent);
                        
                        insert_fileref_child(me->fileref, fr, FALSE);

                        me2 = ExAllocatePoolWithTag(PagedPool, sizeof(move_entry), ALLOC_TAG);
                        if (!me2) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            free_fileref(fr);
                            goto end;
                        }
                        
                        me2->fileref = fr;
                        me2->dummyfcb = NULL;
                        me2->dummyfileref = NULL;
                        me2->parent = me;
                        
                        InsertHeadList(&me->list_entry, &me2->list_entry);
                    }
                }
                
                len -= sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                
                if (len > 0)
                    xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
            } while (len > 0);
        }
        
        b = find_next_item(me->fileref->fcb->Vcb, &tp, &next_tp, FALSE, Irp);
        if (b) {
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > searchkey.obj_id || (next_tp.item->key.obj_id == searchkey.obj_id && next_tp.item->key.obj_type > searchkey.obj_type))
                break;
        }
    } while (b);
    
    if (me->fileref->fcb->type == BTRFS_TYPE_DIRECTORY && me->fileref->fcb->inode_item.st_size != 0) {
        searchkey.obj_id = me->fileref->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = 2;
        
        Status = find_item(me->fileref->fcb->Vcb, me->fileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        do {
            traverse_ptr next_tp;
            
            // FIXME - both lists are ordered; we can make this more efficient
            
            if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
                BOOL found = FALSE;
                
                le = me->fileref->children.Flink;
                
                while (le != &me->fileref->children) {
                    file_ref* fr = CONTAINING_RECORD(le, file_ref, list_entry);
                    
                    if (!fr->fcb->ads) {
                        if (fr->index == tp.item->key.offset) {
                            found = TRUE;
                            break;
                        } else if (fr->index > tp.item->key.offset)
                            break;
                    }
                    
                    le = le->Flink;
                }
                
                if (!found) {
                    DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
                    
                    if (tp.item->size < sizeof(DIR_ITEM)) {
                        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (tp.item->size < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
                        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM) - 1 + di->m + di->n);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (di->n == 0) {
                        ERR("(%llx,%x,%llx): filename length was 0\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (di->key.obj_type == TYPE_INODE_ITEM || di->key.obj_type == TYPE_ROOT_ITEM) {
                        ANSI_STRING utf8;
                        fcb* fcb;
                        file_ref* fr;
                        ULONG stringlen;
                        root* subvol;
                        UINT64 inode;
                        dir_child* dc = NULL;
                        
                        utf8.Length = utf8.MaximumLength = di->n;
                        utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
                        if (!utf8.Buffer) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        RtlCopyMemory(utf8.Buffer, di->name, di->n);
                        
                        if (di->key.obj_type == TYPE_ROOT_ITEM) {
                            LIST_ENTRY* le2;
                            
                            subvol = NULL;
                            
                            le2 = me->fileref->fcb->Vcb->roots.Flink;
                            while (le2 != &me->fileref->fcb->Vcb->roots) {
                                root* r2 = CONTAINING_RECORD(le2, root, list_entry);
                                
                                if (r2->id == di->key.obj_id) {
                                    subvol = r2;
                                    break;
                                }
                                
                                le2 = le2->Flink;
                            }
                            
                            if (!subvol) {
                                ERR("could not find subvol %llx\n", di->key.obj_id);
                                Status = STATUS_INTERNAL_ERROR;
                                goto end;
                            }

                            inode = SUBVOL_ROOT_INODE;
                        } else {
                            subvol = me->fileref->fcb->subvol;
                            inode = di->key.obj_id;
                        }
                        
                        Status = open_fcb(me->fileref->fcb->Vcb, subvol, inode, di->type, &utf8, me->fileref->fcb, &fcb, PagedPool, Irp);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("open_fcb returned %08x\n", Status);
                            ExFreePool(utf8.Buffer);
                            goto end;
                        }
                        
                        fr = create_fileref();
                        if (!fr) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(utf8.Buffer);
                            free_fcb(fcb);
                            goto end;
                        }
                        
                        fr->fcb = fcb;
                        fr->utf8 = utf8;
                        
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, utf8.Buffer, utf8.Length);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }
                        
                        fr->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!fr->filepart.Buffer) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            free_fileref(fr);
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(fr->filepart.Buffer, stringlen, &stringlen, utf8.Buffer, utf8.Length);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }
                        
                        fr->filepart.Length = fr->filepart.MaximumLength = stringlen;
                        
                        Status = RtlUpcaseUnicodeString(&fr->filepart_uc, &fr->filepart, TRUE);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                            free_fileref(fr);
                            goto end;
                        }
                        
                        fr->parent = me->fileref;

                        fr->index = tp.item->key.offset;
                        increase_fileref_refcount(me->fileref);
                        
                        Status = add_dir_child(me->fileref->fcb, di->key.obj_type == TYPE_ROOT_ITEM ? subvol->id : fr->fcb->inode,
                                               di->key.obj_type == TYPE_ROOT_ITEM ? TRUE : FALSE, fr->index, &utf8, &fr->filepart, &fr->filepart_uc, BTRFS_TYPE_DIRECTORY, &dc);
                        if (!NT_SUCCESS(Status))
                            WARN("add_dir_child returned %08x\n", Status);
                        
                        fr->dc = dc;
                        dc->fileref = fr;
                        
                        insert_fileref_child(fr->parent, fr, FALSE);
                        
                        if (fr->fcb->type == BTRFS_TYPE_DIRECTORY)
                            fr->fcb->fileref = fr;
                        
                        me2 = ExAllocatePoolWithTag(PagedPool, sizeof(move_entry), ALLOC_TAG);
                        if (!me2) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            free_fileref(fr);
                            goto end;
                        }
                        
                        me2->fileref = fr;
                        me2->dummyfcb = NULL;
                        me2->dummyfileref = NULL;
                        me2->parent = me;
                        
                        InsertHeadList(&me->list_entry, &me2->list_entry);
                    } else {
                        ERR("unrecognized key (%llx,%x,%llx)\n", di->key.obj_id, di->key.obj_type, di->key.offset);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                }
            }
            
            b = find_next_item(me->fileref->fcb->Vcb, &tp, &next_tp, FALSE, Irp);
            if (b) {
                tp = next_tp;
                
                if (next_tp.item->key.obj_id > searchkey.obj_id || (next_tp.item->key.obj_id == searchkey.obj_id && next_tp.item->key.obj_type > searchkey.obj_type))
                    break;
            }
        } while (b);
    }
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&me->fileref->nonpaged->children_lock);
    
    return Status;
}

void remove_dir_child_from_hash_lists(fcb* fcb, dir_child* dc) {
    UINT8 c;
    
    c = dc->hash >> 24;
    
    if (fcb->hash_ptrs[c] == &dc->list_entry_hash) {
        if (dc->list_entry_hash.Flink == &fcb->dir_children_hash)
            fcb->hash_ptrs[c] = NULL;
        else {
            dir_child* dc2 = CONTAINING_RECORD(dc->list_entry_hash.Flink, dir_child, list_entry_hash);
            
            if (dc2->hash >> 24 == c)
                fcb->hash_ptrs[c] = &dc2->list_entry_hash;
            else
                fcb->hash_ptrs[c] = NULL;
        }
    }
    
    RemoveEntryList(&dc->list_entry_hash);
    
    c = dc->hash_uc >> 24;
    
    if (fcb->hash_ptrs_uc[c] == &dc->list_entry_hash_uc) {
        if (dc->list_entry_hash_uc.Flink == &fcb->dir_children_hash_uc)
            fcb->hash_ptrs_uc[c] = NULL;
        else {
            dir_child* dc2 = CONTAINING_RECORD(dc->list_entry_hash_uc.Flink, dir_child, list_entry_hash_uc);
            
            if (dc2->hash_uc >> 24 == c)
                fcb->hash_ptrs_uc[c] = &dc2->list_entry_hash_uc;
            else
                fcb->hash_ptrs_uc[c] = NULL;
        }
    }
    
    RemoveEntryList(&dc->list_entry_hash_uc);
}

static NTSTATUS move_across_subvols(file_ref* fileref, file_ref* destdir, PANSI_STRING utf8, PUNICODE_STRING fnus, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY move_list, *le;
    move_entry* me;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    file_ref* origparent;
    
    InitializeListHead(&move_list);
    
    me = ExAllocatePoolWithTag(PagedPool, sizeof(move_entry), ALLOC_TAG);
    
    if (!me) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    origparent = fileref->parent;
    
    me->fileref = fileref;
    increase_fileref_refcount(me->fileref);
    me->dummyfcb = NULL;
    me->dummyfileref = NULL;
    me->parent = NULL;
    
    InsertTailList(&move_list, &me->list_entry);
    
    le = move_list.Flink;
    while (le != &move_list) {
        me = CONTAINING_RECORD(le, move_entry, list_entry);
        
        ExAcquireResourceSharedLite(me->fileref->fcb->Header.Resource, TRUE);
        
        if (!me->fileref->fcb->ads && me->fileref->fcb->subvol == origparent->fcb->subvol) {
            Status = add_children_to_move_list(me, Irp);
            
            if (!NT_SUCCESS(Status)) {
                ERR("add_children_to_move_list returned %08x\n", Status);
                goto end;
            }
        }
        
        ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
        
        le = le->Flink;
    }
    
    // loop through list and create new inodes
    
    le = move_list.Flink;
    while (le != &move_list) {
        me = CONTAINING_RECORD(le, move_entry, list_entry);
        
        if (me->fileref->fcb->inode != SUBVOL_ROOT_INODE) {
            if (!me->dummyfcb) {
                ULONG defda;
                BOOL inserted = FALSE;
                LIST_ENTRY* le;
                
                ExAcquireResourceExclusiveLite(me->fileref->fcb->Header.Resource, TRUE);
                
                Status = duplicate_fcb(me->fileref->fcb, &me->dummyfcb);
                if (!NT_SUCCESS(Status)) {
                    ERR("duplicate_fcb returned %08x\n", Status);
                    ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
                    goto end;
                }
                
                me->dummyfcb->subvol = me->fileref->fcb->subvol;
                me->dummyfcb->inode = me->fileref->fcb->inode;
                
                if (!me->dummyfcb->ads) {
                    me->dummyfcb->sd_dirty = me->fileref->fcb->sd_dirty;
                    me->dummyfcb->atts_changed = me->fileref->fcb->atts_changed;
                    me->dummyfcb->atts_deleted = me->fileref->fcb->atts_deleted;
                    me->dummyfcb->extents_changed = me->fileref->fcb->extents_changed;
                    me->dummyfcb->reparse_xattr_changed = me->fileref->fcb->reparse_xattr_changed;
                    me->dummyfcb->ea_changed = me->fileref->fcb->ea_changed;
                }
                
                me->dummyfcb->created = me->fileref->fcb->created;
                me->dummyfcb->deleted = me->fileref->fcb->deleted;
                mark_fcb_dirty(me->dummyfcb);
                
                if (!me->fileref->fcb->ads) {
                    LIST_ENTRY* le2;
                    
                    me->fileref->fcb->subvol = destdir->fcb->subvol;
                    me->fileref->fcb->inode = InterlockedIncrement64(&destdir->fcb->subvol->lastinode);
                    me->fileref->fcb->inode_item.st_nlink = 1;
                    
                    defda = get_file_attributes(me->fileref->fcb->Vcb, &me->fileref->fcb->inode_item, me->fileref->fcb->subvol, me->fileref->fcb->inode,
                                                me->fileref->fcb->type, me->fileref->filepart.Length > 0 && me->fileref->filepart.Buffer[0] == '.', TRUE, Irp);
                    
                    me->fileref->fcb->sd_dirty = !!me->fileref->fcb->sd;
                    me->fileref->fcb->atts_changed = defda != me->fileref->fcb->atts;
                    me->fileref->fcb->extents_changed = !IsListEmpty(&me->fileref->fcb->extents);
                    me->fileref->fcb->reparse_xattr_changed = !!me->fileref->fcb->reparse_xattr.Buffer;
                    me->fileref->fcb->ea_changed = !!me->fileref->fcb->ea_xattr.Buffer;
                    me->fileref->fcb->inode_item_changed = TRUE;
                    
                    le2 = me->fileref->fcb->extents.Flink;
                    while (le2 != &me->fileref->fcb->extents) {
                        extent* ext = CONTAINING_RECORD(le2, extent, list_entry);
                        
                        if (!ext->ignore && ext->datalen >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2) &&
                            (ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC)) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                                        
                            if (ed2->size != 0) {
                                chunk* c = get_chunk_from_address(me->fileref->fcb->Vcb, ed2->address);
                                
                                if (!c) {
                                    ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                                } else {
                                    Status = update_changed_extent_ref(me->fileref->fcb->Vcb, c, ed2->address, ed2->size, me->fileref->fcb->subvol->id, me->fileref->fcb->inode,
                                                                       ext->offset - ed2->offset, 1, me->fileref->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, FALSE, Irp);
                                    
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("update_changed_extent_ref returned %08x\n", Status);
                                        ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
                                        goto end;
                                    }
                                }
                            
                            }
                        }
                        
                        le2 = le2->Flink;
                    }
                } else {
                    me->fileref->fcb->subvol = me->parent->fileref->fcb->subvol;
                    me->fileref->fcb->inode = me->parent->fileref->fcb->inode;
                }
                
                me->fileref->fcb->created = TRUE;
                
                InsertHeadList(&me->fileref->fcb->list_entry, &me->dummyfcb->list_entry);
                RemoveEntryList(&me->fileref->fcb->list_entry);
                
                le = destdir->fcb->subvol->fcbs.Flink;
                while (le != &destdir->fcb->subvol->fcbs) {
                    fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
                    
                    if (fcb->inode > me->fileref->fcb->inode) {
                        InsertHeadList(le->Blink, &me->fileref->fcb->list_entry);
                        inserted = TRUE;
                        break;
                    }
                    
                    le = le->Flink;
                }
                
                if (!inserted)
                    InsertTailList(&destdir->fcb->subvol->fcbs, &me->fileref->fcb->list_entry);
                
                InsertTailList(&me->fileref->fcb->Vcb->all_fcbs, &me->dummyfcb->list_entry_all);
                
                while (!IsListEmpty(&me->fileref->fcb->hardlinks)) {
                    LIST_ENTRY* le = RemoveHeadList(&me->fileref->fcb->hardlinks);
                    hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);
                    
                    if (hl->name.Buffer)
                        ExFreePool(hl->name.Buffer);
                    
                    if (hl->utf8.Buffer)
                        ExFreePool(hl->utf8.Buffer);

                    ExFreePool(hl);
                }
                
                me->fileref->fcb->inode_item_changed = TRUE;
                mark_fcb_dirty(me->fileref->fcb);
                
                if ((!me->dummyfcb->ads && me->dummyfcb->inode_item.st_nlink > 1) || (me->dummyfcb->ads && me->parent->dummyfcb->inode_item.st_nlink > 1)) {
                    LIST_ENTRY* le2 = le->Flink;
                    
                    while (le2 != &move_list) {
                        move_entry* me2 = CONTAINING_RECORD(le2, move_entry, list_entry);
                        
                        if (me2->fileref->fcb == me->fileref->fcb && !me2->fileref->fcb->ads) {
                            me2->dummyfcb = me->dummyfcb;
                            InterlockedIncrement(&me->dummyfcb->refcount);
                        }
                        
                        le2 = le2->Flink;
                    }
                }
                
                ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
            } else {
                ExAcquireResourceExclusiveLite(me->fileref->fcb->Header.Resource, TRUE);
                me->fileref->fcb->inode_item.st_nlink++;
                me->fileref->fcb->inode_item_changed = TRUE;
                ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
            }
        }
        
        le = le->Flink;
    }
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fileref->fcb->subvol->root_item.ctransid = fileref->fcb->Vcb->superblock.generation;
    fileref->fcb->subvol->root_item.ctime = now;
    
    // loop through list and create new filerefs
    
    le = move_list.Flink;
    while (le != &move_list) {
        hardlink* hl;
        BOOL name_changed = FALSE;
        
        me = CONTAINING_RECORD(le, move_entry, list_entry);
        
        me->dummyfileref = create_fileref();
        if (!me->dummyfileref) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        if (me->fileref->fcb->inode == SUBVOL_ROOT_INODE)
            me->dummyfileref->fcb = me->fileref->fcb;
        else
            me->dummyfileref->fcb = me->dummyfcb;
        
        InterlockedIncrement(&me->dummyfileref->fcb->refcount);

        me->dummyfileref->filepart = me->fileref->filepart;
        
        if (le == move_list.Flink) // first item
            me->fileref->filepart.Length = me->fileref->filepart.MaximumLength = fnus->Length;
        else
            me->fileref->filepart.MaximumLength = me->fileref->filepart.Length;
        
        me->fileref->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, me->fileref->filepart.MaximumLength, ALLOC_TAG);
        
        if (!me->fileref->filepart.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(me->fileref->filepart.Buffer, le == move_list.Flink ? fnus->Buffer : me->dummyfileref->filepart.Buffer, me->fileref->filepart.Length);
        
        Status = RtlUpcaseUnicodeString(&me->fileref->filepart_uc, &me->fileref->filepart, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            goto end;
        }
        
        me->dummyfileref->utf8 = me->fileref->utf8;
        me->dummyfileref->oldutf8 = me->fileref->oldutf8;
        
        if (le == move_list.Flink) {
            if (me->fileref->utf8.Length != utf8->Length || RtlCompareMemory(me->fileref->utf8.Buffer, utf8->Buffer, utf8->Length) != utf8->Length)
                name_changed = TRUE;
            
            me->fileref->utf8.Length = me->fileref->utf8.MaximumLength = utf8->Length;
        } else
            me->fileref->utf8.MaximumLength = me->fileref->utf8.Length;
        
        if (me->fileref->utf8.MaximumLength > 0) {
            me->fileref->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, me->fileref->utf8.MaximumLength, ALLOC_TAG);
            
            if (!me->fileref->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            RtlCopyMemory(me->fileref->utf8.Buffer, le == move_list.Flink ? utf8->Buffer : me->dummyfileref->utf8.Buffer, me->fileref->utf8.Length);
        }
        
        me->dummyfileref->delete_on_close = me->fileref->delete_on_close;
        me->dummyfileref->deleted = me->fileref->deleted;
        
        me->dummyfileref->created = me->fileref->created;
        me->fileref->created = TRUE;
        
        me->dummyfileref->parent = me->parent ? me->parent->dummyfileref : origparent;
        increase_fileref_refcount(me->dummyfileref->parent);
        
        me->dummyfileref->index = me->fileref->index;

        insert_fileref_child(me->dummyfileref->parent, me->dummyfileref, TRUE);
       
        me->dummyfileref->debug_desc = me->fileref->debug_desc;
        
        if (me->dummyfileref->fcb->type == BTRFS_TYPE_DIRECTORY)
            me->dummyfileref->fcb->fileref = me->dummyfileref;
        
        if (!me->parent) {
            RemoveEntryList(&me->fileref->list_entry);
            
            free_fileref(me->fileref->parent);
            
            increase_fileref_refcount(destdir);
            
            Status = fcb_get_last_dir_index(destdir->fcb, &me->fileref->index, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("fcb_get_last_dir_index returned %08x\n", Status);
                goto end;
            }
            
            if (me->fileref->dc) {
                // remove from old parent
                ExAcquireResourceExclusiveLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock, TRUE);
                RemoveEntryList(&me->fileref->dc->list_entry_index);
                remove_dir_child_from_hash_lists(me->fileref->parent->fcb, me->fileref->dc);
                ExReleaseResourceLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock);
                
                if (name_changed) {
                    ExFreePool(me->fileref->dc->utf8.Buffer);
                    ExFreePool(me->fileref->dc->name.Buffer);
                    ExFreePool(me->fileref->dc->name_uc.Buffer);
                    
                    me->fileref->dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8->Length, ALLOC_TAG);
                    if (!me->fileref->dc->utf8.Buffer) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    me->fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, me->fileref->filepart.Length, ALLOC_TAG);
                    if (!me->fileref->dc->name.Buffer) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    me->fileref->dc->name_uc.Buffer = ExAllocatePoolWithTag(PagedPool, me->fileref->filepart_uc.Length, ALLOC_TAG);
                    if (!me->fileref->dc->name_uc.Buffer) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    me->fileref->dc->utf8.Length = me->fileref->dc->utf8.MaximumLength = utf8->Length;
                    RtlCopyMemory(me->fileref->dc->utf8.Buffer, utf8->Buffer, utf8->Length);
                    
                    me->fileref->dc->name.Length = me->fileref->dc->name.MaximumLength = me->fileref->filepart.Length;
                    RtlCopyMemory(me->fileref->dc->name.Buffer, me->fileref->filepart.Buffer, me->fileref->filepart.Length);
                    
                    me->fileref->dc->name_uc.Length = me->fileref->dc->name_uc.MaximumLength = me->fileref->filepart_uc.Length;
                    RtlCopyMemory(me->fileref->dc->name_uc.Buffer, me->fileref->filepart_uc.Buffer, me->fileref->filepart_uc.Length);
                    
                    me->fileref->dc->hash = calc_crc32c(0xffffffff, (UINT8*)me->fileref->dc->name.Buffer, me->fileref->dc->name.Length);
                    me->fileref->dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)me->fileref->dc->name_uc.Buffer, me->fileref->dc->name_uc.Length);
                }
                
                // add to new parent
                ExAcquireResourceExclusiveLite(&destdir->fcb->nonpaged->dir_children_lock, TRUE);
                InsertTailList(&destdir->fcb->dir_children_index, &me->fileref->dc->list_entry_index);
                insert_dir_child_into_hash_lists(destdir->fcb, me->fileref->dc);
                ExReleaseResourceLite(&destdir->fcb->nonpaged->dir_children_lock);
            }
            
            me->fileref->parent = destdir;
            
            insert_fileref_child(me->fileref->parent, me->fileref, TRUE);
            
            TRACE("me->fileref->parent->fcb->inode_item.st_size (inode %llx) was %llx\n", me->fileref->parent->fcb->inode, me->fileref->parent->fcb->inode_item.st_size);
            me->fileref->parent->fcb->inode_item.st_size += me->fileref->utf8.Length * 2;
            TRACE("me->fileref->parent->fcb->inode_item.st_size (inode %llx) now %llx\n", me->fileref->parent->fcb->inode, me->fileref->parent->fcb->inode_item.st_size);
            me->fileref->parent->fcb->inode_item.transid = me->fileref->fcb->Vcb->superblock.generation;
            me->fileref->parent->fcb->inode_item.sequence++;
            me->fileref->parent->fcb->inode_item.st_ctime = now;
            me->fileref->parent->fcb->inode_item.st_mtime = now;
            me->fileref->parent->fcb->inode_item_changed = TRUE;
            mark_fcb_dirty(me->fileref->parent->fcb);
        }

        if (me->fileref->fcb->inode == SUBVOL_ROOT_INODE)
            me->fileref->fcb->subvol->root_item.num_references++;

        if (!me->dummyfileref->fcb->ads) {
            Status = delete_fileref(me->dummyfileref, NULL, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_fileref returned %08x\n", Status);
                goto end;
            }
        }
        
        if (me->fileref->fcb->inode_item.st_nlink > 1) {
            hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
            if (!hl) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            hl->parent = me->fileref->parent->fcb->inode;
            hl->index = me->fileref->index;
            
            hl->utf8.Length = hl->utf8.MaximumLength = me->fileref->utf8.Length;
            hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
            if (!hl->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExFreePool(hl);
                goto end;
            }
            
            RtlCopyMemory(hl->utf8.Buffer, me->fileref->utf8.Buffer, me->fileref->utf8.Length);
            
            hl->name.Length = hl->name.MaximumLength = me->fileref->filepart.Length;
            hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
            if (!hl->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExFreePool(hl->utf8.Buffer);
                ExFreePool(hl);
                goto end;
            }
            
            RtlCopyMemory(hl->name.Buffer, me->fileref->filepart.Buffer, me->fileref->filepart.Length);
            
            InsertTailList(&me->fileref->fcb->hardlinks, &hl->list_entry);
        }
        
        mark_fileref_dirty(me->fileref);
        
        le = le->Flink;
    }
    
    // loop through, and only mark streams as deleted if their parent inodes are also deleted
    
    le = move_list.Flink;
    while (le != &move_list) {
        me = CONTAINING_RECORD(le, move_entry, list_entry);
        
        if (me->dummyfileref->fcb->ads && me->parent->dummyfileref->fcb->deleted) {
            Status = delete_fileref(me->dummyfileref, NULL, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_fileref returned %08x\n", Status);
                goto end;
            }
        }
        
        le = le->Flink;
    }
    
    destdir->fcb->subvol->root_item.ctransid = destdir->fcb->Vcb->superblock.generation;
    destdir->fcb->subvol->root_item.ctime = now;
    
    me = CONTAINING_RECORD(move_list.Flink, move_entry, list_entry);
    send_notification_fileref(me->dummyfileref, fileref->fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED);
    send_notification_fileref(fileref, fileref->fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
    send_notification_fileref(me->dummyfileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    send_notification_fileref(fileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);

    Status = STATUS_SUCCESS;
    
end:
    while (!IsListEmpty(&move_list)) {
        le = RemoveHeadList(&move_list);
        me = CONTAINING_RECORD(le, move_entry, list_entry);
        
        if (me->dummyfcb)
            free_fcb(me->dummyfcb);
        
        if (me->dummyfileref)
            free_fileref(me->dummyfileref);
        
        free_fileref(me->fileref);
        
        ExFreePool(me);
    }
    
    return Status;
}

void insert_dir_child_into_hash_lists(fcb* fcb, dir_child* dc) {
    BOOL inserted;
    LIST_ENTRY* le;
    UINT8 c, d;
    
    c = dc->hash >> 24;
    
    inserted = FALSE;
    
    d = c;
    do {
        le = fcb->hash_ptrs[d];
        
        if (d == 0)
            break;
        
        d--;
    } while (!le);
    
    if (!le)
        le = fcb->dir_children_hash.Flink;
    
    while (le != &fcb->dir_children_hash) {
        dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_hash);
        
        if (dc2->hash > dc->hash) {
            InsertHeadList(le->Blink, &dc->list_entry_hash);
            inserted = TRUE;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!inserted)
        InsertTailList(&fcb->dir_children_hash, &dc->list_entry_hash);
    
    if (!fcb->hash_ptrs[c])
        fcb->hash_ptrs[c] = &dc->list_entry_hash;
    else {
        dir_child* dc2 = CONTAINING_RECORD(fcb->hash_ptrs[c], dir_child, list_entry_hash);
        
        if (dc2->hash > dc->hash)
            fcb->hash_ptrs[c] = &dc->list_entry_hash;
    }
    
    c = dc->hash_uc >> 24;
    
    inserted = FALSE;
    
    d = c;
    do {
        le = fcb->hash_ptrs_uc[d];
        
        if (d == 0)
            break;
        
        d--;
    } while (!le);
    
    if (!le)
        le = fcb->dir_children_hash_uc.Flink;
    
    while (le != &fcb->dir_children_hash_uc) {
        dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_hash_uc);
        
        if (dc2->hash_uc > dc->hash_uc) {
            InsertHeadList(le->Blink, &dc->list_entry_hash_uc);
            inserted = TRUE;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!inserted)
        InsertTailList(&fcb->dir_children_hash_uc, &dc->list_entry_hash_uc);
    
    if (!fcb->hash_ptrs_uc[c])
        fcb->hash_ptrs_uc[c] = &dc->list_entry_hash_uc;
    else {
        dir_child* dc2 = CONTAINING_RECORD(fcb->hash_ptrs_uc[c], dir_child, list_entry_hash_uc);
        
        if (dc2->hash_uc > dc->hash_uc)
            fcb->hash_ptrs_uc[c] = &dc->list_entry_hash_uc;
    }
}

static NTSTATUS STDCALL set_rename_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, PFILE_OBJECT tfo) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FILE_RENAME_INFORMATION* fri = Irp->AssociatedIrp.SystemBuffer;
    fcb *fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref *fileref = ccb ? ccb->fileref : NULL, *oldfileref = NULL, *related = NULL, *fr2 = NULL;
    UINT64 index;
    WCHAR* fn;
    ULONG fnlen, utf8len;
    UNICODE_STRING fnus;
    ANSI_STRING utf8;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    LIST_ENTRY rollback, *le;
    hardlink* hl;
    
    InitializeListHead(&rollback);
    
    TRACE("tfo = %p\n", tfo);
    TRACE("ReplaceIfExists = %u\n", IrpSp->Parameters.SetFile.ReplaceIfExists);
    TRACE("RootDirectory = %p\n", fri->RootDirectory);
    TRACE("FileName = %.*S\n", fri->FileNameLength / sizeof(WCHAR), fri->FileName);
    
    fn = fri->FileName;
    fnlen = fri->FileNameLength / sizeof(WCHAR);
    
    if (!tfo) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref set and no directory given\n");
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        LONG i;
        
        while (fnlen > 0 && (fri->FileName[fnlen - 1] == '/' || fri->FileName[fnlen - 1] == '\\'))
            fnlen--;
        
        if (fnlen == 0)
            return STATUS_INVALID_PARAMETER;
        
        for (i = fnlen - 1; i >= 0; i--) {
            if (fri->FileName[i] == '\\' || fri->FileName[i] == '/') {
                fn = &fri->FileName[i+1];
                fnlen = (fri->FileNameLength / sizeof(WCHAR)) - i - 1;
                break;
            }
        }
    }
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->ads) {
        FIXME("FIXME - renaming streams\n"); // FIXME
        Status = STATUS_NOT_IMPLEMENTED;
        goto end;
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
    
    if (tfo && tfo->FsContext2) {
        struct _ccb* relatedccb = tfo->FsContext2;
        
        related = relatedccb->fileref;
        increase_fileref_refcount(related);
    } else if (fnus.Length >= sizeof(WCHAR) && fnus.Buffer[0] != '\\') {
        related = fileref->parent;
        increase_fileref_refcount(related);
    }

    Status = open_fileref(Vcb, &oldfileref, &fnus, related, FALSE, NULL, NULL, PagedPool, ccb->case_sensitive,  Irp);

    if (NT_SUCCESS(Status)) {
        TRACE("destination file %S already exists\n", file_desc_fileref(oldfileref));
        
        if (fileref != oldfileref && !oldfileref->deleted) {
            if (!IrpSp->Parameters.SetFile.ReplaceIfExists) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            } else if ((oldfileref->open_count >= 1 || has_open_children(oldfileref)) && !oldfileref->deleted) {
                WARN("trying to overwrite open file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
            
            if (oldfileref->fcb->type == BTRFS_TYPE_DIRECTORY) {
                WARN("trying to overwrite directory\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
        }
        
        if (fileref == oldfileref || oldfileref->deleted) {
            free_fileref(oldfileref);
            oldfileref = NULL;
        }
    }
    
    if (!related) {
        Status = open_fileref(Vcb, &related, &fnus, NULL, TRUE, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08x\n", Status);
            goto end;
        }
    }
    
    if (has_open_children(fileref)) {
        WARN("trying to rename file with open children\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    if (oldfileref) {
        ACCESS_MASK access;
        SECURITY_SUBJECT_CONTEXT subjcont;
        
        SeCaptureSubjectContext(&subjcont);

        if (!SeAccessCheck(oldfileref->fcb->sd, &subjcont, FALSE, DELETE, 0, NULL,
                           IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
            SeReleaseSubjectContext(&subjcont);
            WARN("SeAccessCheck failed, returning %08x\n", Status);
            goto end;
        }

        SeReleaseSubjectContext(&subjcont);
        
        Status = delete_fileref(oldfileref, NULL, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_fileref returned %08x\n", Status);
            goto end;
        }
    }
    
    if (fileref->parent->fcb->subvol != related->fcb->subvol && fileref->fcb->subvol == fileref->parent->fcb->subvol) {
        Status = move_across_subvols(fileref, related, &utf8, &fnus, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("move_across_subvols returned %08x\n", Status);
        }
        goto end;
    }
    
    if (related == fileref->parent) { // keeping file in same directory
        UNICODE_STRING fnus2, oldfn, newfn;
        USHORT name_offset;
        ULONG oldutf8len;
        
        fnus2.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);
        if (!fnus2.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        Status = fileref_get_filename(fileref, &oldfn, &name_offset);
        if (!NT_SUCCESS(Status)) {
            ERR("fileref_get_filename returned %08x\n", Status);
            goto end;
        }
        
        fnus2.Length = fnus2.MaximumLength = fnus.Length;
        RtlCopyMemory(fnus2.Buffer, fnus.Buffer, fnus.Length);
        
        oldutf8len = fileref->utf8.Length;
        
        if (!fileref->created && !fileref->oldutf8.Buffer)
            fileref->oldutf8 = fileref->utf8;
        else
            ExFreePool(fileref->utf8.Buffer);
        
        TRACE("renaming %.*S to %.*S\n", fileref->filepart.Length / sizeof(WCHAR), fileref->filepart.Buffer, fnus2.Length / sizeof(WCHAR), fnus.Buffer);
        
        fileref->utf8 = utf8;
        fileref->filepart = fnus2;
        
        Status = fileref_get_filename(fileref, &newfn, &name_offset);
        if (!NT_SUCCESS(Status)) {
            ERR("fileref_get_filename returned %08x\n", Status);
            ExFreePool(oldfn.Buffer);
            goto end;
        }
        
        if (fileref->filepart_uc.Buffer)
            ExFreePool(fileref->filepart_uc.Buffer);
        
        Status = RtlUpcaseUnicodeString(&fileref->filepart_uc, &fileref->filepart, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            ExFreePool(oldfn.Buffer);
            ExFreePool(newfn.Buffer);
            goto end;
        }
        
        mark_fileref_dirty(fileref);
        
        if (fileref->dc) {
            ExAcquireResourceExclusiveLite(&fileref->parent->fcb->nonpaged->dir_children_lock, TRUE);
            
            ExFreePool(fileref->dc->utf8.Buffer);
            ExFreePool(fileref->dc->name.Buffer);
            ExFreePool(fileref->dc->name_uc.Buffer);
            
            fileref->dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);
            if (!fileref->dc->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                goto end;
            }
            
            fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->filepart.Length, ALLOC_TAG);
            if (!fileref->dc->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                goto end;
            }
            
            fileref->dc->name_uc.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->filepart_uc.Length, ALLOC_TAG);
            if (!fileref->dc->name_uc.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                goto end;
            }
            
            fileref->dc->utf8.Length = fileref->dc->utf8.MaximumLength = utf8.Length;
            RtlCopyMemory(fileref->dc->utf8.Buffer, utf8.Buffer, utf8.Length);
            
            fileref->dc->name.Length = fileref->dc->name.MaximumLength = fileref->filepart.Length;
            RtlCopyMemory(fileref->dc->name.Buffer, fileref->filepart.Buffer, fileref->filepart.Length);
            
            fileref->dc->name_uc.Length = fileref->dc->name_uc.MaximumLength = fileref->filepart_uc.Length;
            RtlCopyMemory(fileref->dc->name_uc.Buffer, fileref->filepart_uc.Buffer, fileref->filepart_uc.Length);
            
            remove_dir_child_from_hash_lists(fileref->parent->fcb, fileref->dc);
            
            fileref->dc->hash = calc_crc32c(0xffffffff, (UINT8*)fileref->dc->name.Buffer, fileref->dc->name.Length);
            fileref->dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)fileref->dc->name_uc.Buffer, fileref->dc->name_uc.Length);
            
            insert_dir_child_into_hash_lists(fileref->parent->fcb, fileref->dc);
            
            ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
        }
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        
        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;
        
        fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(fcb);
        
        // update parent's INODE_ITEM
        
        related->fcb->inode_item.transid = Vcb->superblock.generation;
        TRACE("related->fcb->inode_item.st_size (inode %llx) was %llx\n", related->fcb->inode, related->fcb->inode_item.st_size);
        related->fcb->inode_item.st_size = related->fcb->inode_item.st_size + (2 * utf8.Length) - (2* oldutf8len);
        TRACE("related->fcb->inode_item.st_size (inode %llx) now %llx\n", related->fcb->inode, related->fcb->inode_item.st_size);
        related->fcb->inode_item.sequence++;
        related->fcb->inode_item.st_ctime = now;
        related->fcb->inode_item.st_mtime = now;
        
        related->fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(related->fcb);
        send_notification_fileref(related, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
        
        FsRtlNotifyFilterReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&oldfn, name_offset, NULL, NULL,
                                      fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_RENAMED_OLD_NAME, NULL, NULL);
        FsRtlNotifyFilterReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&newfn, name_offset, NULL, NULL,
                                      fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_RENAMED_NEW_NAME, NULL, NULL);
        
        ExFreePool(oldfn.Buffer);
        ExFreePool(newfn.Buffer);
        
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    // We move files by moving the existing fileref to the new directory, and
    // replacing it with a dummy fileref with the same original values, but marked as deleted.
    
    fr2 = create_fileref();
    
    fr2->fcb = fileref->fcb;
    fr2->fcb->refcount++;
    
    fr2->filepart = fileref->filepart;
    fr2->filepart_uc = fileref->filepart_uc;
    fr2->utf8 = fileref->utf8;
    fr2->oldutf8 = fileref->oldutf8;
    fr2->index = fileref->index;
    fr2->delete_on_close = fileref->delete_on_close;
    fr2->deleted = TRUE;
    fr2->created = fileref->created;
    fr2->parent = fileref->parent;
    fr2->dc = NULL;
    
    if (fr2->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr2->fcb->fileref = fr2;

    Status = fcb_get_last_dir_index(related->fcb, &index, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_last_dir_index returned %08x\n", Status);
        goto end;
    }
    
    fileref->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);
    if (!fileref->filepart.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fileref->filepart.Length = fileref->filepart.MaximumLength = fnus.Length;
    RtlCopyMemory(fileref->filepart.Buffer, fnus.Buffer, fnus.Length);
    
    Status = RtlUpcaseUnicodeString(&fileref->filepart_uc, &fileref->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        goto end;
    }
    
    fileref->utf8 = utf8;
    fileref->oldutf8.Buffer = NULL;
    fileref->index = index;
    fileref->deleted = FALSE;
    fileref->created = TRUE;
    fileref->parent = related;

    ExAcquireResourceExclusiveLite(&fileref->parent->nonpaged->children_lock, TRUE);
    InsertHeadList(&fileref->list_entry, &fr2->list_entry);
    RemoveEntryList(&fileref->list_entry);
    ExReleaseResourceLite(&fileref->parent->nonpaged->children_lock);
    
    insert_fileref_child(related, fileref, TRUE);
    
    mark_fileref_dirty(fr2);
    mark_fileref_dirty(fileref);
    
    if (fileref->dc) {
        // remove from old parent
        ExAcquireResourceExclusiveLite(&fr2->parent->fcb->nonpaged->dir_children_lock, TRUE);
        RemoveEntryList(&fileref->dc->list_entry_index);
        remove_dir_child_from_hash_lists(fr2->parent->fcb, fileref->dc);
        ExReleaseResourceLite(&fr2->parent->fcb->nonpaged->dir_children_lock);
        
        if (fileref->utf8.Length != fr2->utf8.Length || RtlCompareMemory(fileref->utf8.Buffer, fr2->utf8.Buffer, fr2->utf8.Length) != fr2->utf8.Length) {
            // handle changed name
            
            ExFreePool(fileref->dc->utf8.Buffer);
            ExFreePool(fileref->dc->name.Buffer);
            ExFreePool(fileref->dc->name_uc.Buffer);
            
            fileref->dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);
            if (!fileref->dc->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->filepart.Length, ALLOC_TAG);
            if (!fileref->dc->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            fileref->dc->name_uc.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->filepart_uc.Length, ALLOC_TAG);
            if (!fileref->dc->name_uc.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            fileref->dc->utf8.Length = fileref->dc->utf8.MaximumLength = utf8.Length;
            RtlCopyMemory(fileref->dc->utf8.Buffer, utf8.Buffer, utf8.Length);
            
            fileref->dc->name.Length = fileref->dc->name.MaximumLength = fileref->filepart.Length;
            RtlCopyMemory(fileref->dc->name.Buffer, fileref->filepart.Buffer, fileref->filepart.Length);
            
            fileref->dc->name_uc.Length = fileref->dc->name_uc.MaximumLength = fileref->filepart_uc.Length;
            RtlCopyMemory(fileref->dc->name_uc.Buffer, fileref->filepart_uc.Buffer, fileref->filepart_uc.Length);
            
            fileref->dc->hash = calc_crc32c(0xffffffff, (UINT8*)fileref->dc->name.Buffer, fileref->dc->name.Length);
            fileref->dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)fileref->dc->name_uc.Buffer, fileref->dc->name_uc.Length);
        }
        
        // add to new parent
        ExAcquireResourceExclusiveLite(&related->fcb->nonpaged->dir_children_lock, TRUE);
        InsertTailList(&related->fcb->dir_children_index, &fileref->dc->list_entry_index);
        insert_dir_child_into_hash_lists(related->fcb, fileref->dc);
        ExReleaseResourceLite(&related->fcb->nonpaged->dir_children_lock);
    }
    
    if (fcb->inode_item.st_nlink > 1) {
        // add new hardlink entry to fcb
        
        hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
        if (!hl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        hl->parent = related->fcb->inode;
        hl->index = index;
        
        hl->name.Length = hl->name.MaximumLength = fileref->filepart.Length;
        hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
        
        if (!hl->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(hl->name.Buffer, fileref->filepart.Buffer, fileref->filepart.Length);
        
        hl->utf8.Length = hl->utf8.MaximumLength = fileref->utf8.Length;
        hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
        
        if (!hl->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl->name.Buffer);
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(hl->utf8.Buffer, fileref->utf8.Buffer, fileref->utf8.Length);
        
        InsertTailList(&fcb->hardlinks, &hl->list_entry);
    }
        
    // delete old hardlink entry from fcb
    
    le = fcb->hardlinks.Flink;
    while (le != &fcb->hardlinks) {
        hl = CONTAINING_RECORD(le, hardlink, list_entry);
        
        if (hl->parent == fr2->parent->fcb->inode && hl->index == fr2->index) {
            RemoveEntryList(&hl->list_entry);
            
            if (hl->utf8.Buffer)
                ExFreePool(hl->utf8.Buffer);
            
            if (hl->name.Buffer)
                ExFreePool(hl->name.Buffer);
            
            ExFreePool(hl);
            break;
        }
        
        le = le->Flink;
    }

    // update inode's INODE_ITEM
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    
    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    // update new parent's INODE_ITEM
    
    related->fcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("related->fcb->inode_item.st_size (inode %llx) was %llx\n", related->fcb->inode, related->fcb->inode_item.st_size);
    related->fcb->inode_item.st_size += 2 * utf8len;
    TRACE("related->fcb->inode_item.st_size (inode %llx) now %llx\n", related->fcb->inode, related->fcb->inode_item.st_size);
    related->fcb->inode_item.sequence++;
    related->fcb->inode_item.st_ctime = now;
    related->fcb->inode_item.st_mtime = now;
    
    related->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(related->fcb);
    
    // update old parent's INODE_ITEM
    
    fr2->parent->fcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("fr2->parent->fcb->inode_item.st_size (inode %llx) was %llx\n", fr2->parent->fcb->inode, fr2->parent->fcb->inode_item.st_size);
    fr2->parent->fcb->inode_item.st_size -= 2 * fr2->utf8.Length;
    TRACE("fr2->parent->fcb->inode_item.st_size (inode %llx) now %llx\n", fr2->parent->fcb->inode, fr2->parent->fcb->inode_item.st_size);
    fr2->parent->fcb->inode_item.sequence++;
    fr2->parent->fcb->inode_item.st_ctime = now;
    fr2->parent->fcb->inode_item.st_mtime = now;
    
    free_fileref(fr2);
    
    fr2->parent->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fr2->parent->fcb);
    
    send_notification_fileref(fr2, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED);
    send_notification_fileref(fileref, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
    send_notification_fileref(related, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    send_notification_fileref(fr2->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);

    Status = STATUS_SUCCESS;

end:
    if (oldfileref)
        free_fileref(oldfileref);
    
    if (!NT_SUCCESS(Status) && related)
        free_fileref(related);
    
    if (!NT_SUCCESS(Status) && fr2)
        free_fileref(fr2);
    
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

NTSTATUS STDCALL stream_set_end_of_file_information(device_extension* Vcb, UINT64 end, fcb* fcb, file_ref* fileref, PFILE_OBJECT FileObject, BOOL advance_only, LIST_ENTRY* rollback) {
    LARGE_INTEGER time;
    BTRFS_TIME now;
    CC_FILE_SIZES ccfs;
    
    TRACE("setting new end to %llx bytes (currently %x)\n", end, fcb->adsdata.Length);
    
    if (!fileref || !fileref->parent) {
        ERR("no fileref for stream\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (end < fcb->adsdata.Length) {
        if (advance_only)
            return STATUS_SUCCESS;
        
        TRACE("truncating stream to %llx bytes\n", end);
        
        fcb->adsdata.Length = end;
    } else if (end > fcb->adsdata.Length) {
        TRACE("extending stream to %llx bytes\n", end);

        if (end > fcb->adsmaxlen) {
            ERR("error - xattr too long (%llu > %u)\n", end, fcb->adsmaxlen);
            return STATUS_DISK_FULL;
        }

        if (end > fcb->adsdata.MaximumLength) {
            char* data = ExAllocatePoolWithTag(PagedPool, end, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                ExFreePool(data);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            if (fcb->adsdata.Buffer) {
                RtlCopyMemory(data, fcb->adsdata.Buffer, fcb->adsdata.Length);
                ExFreePool(fcb->adsdata.Buffer);
            }
            
            fcb->adsdata.Buffer = data;
            fcb->adsdata.MaximumLength = end;
        }
        
        RtlZeroMemory(&fcb->adsdata.Buffer[fcb->adsdata.Length], end - fcb->adsdata.Length);
        
        fcb->adsdata.Length = end;
    }
    
    mark_fcb_dirty(fcb);
    
    fcb->Header.AllocationSize.QuadPart = end;
    fcb->Header.FileSize.QuadPart = end;
    fcb->Header.ValidDataLength.QuadPart = end;

    if (FileObject) {
        ccfs.AllocationSize = fcb->Header.AllocationSize;
        ccfs.FileSize = fcb->Header.FileSize;
        ccfs.ValidDataLength = fcb->Header.ValidDataLength;

        CcSetFileSizes(FileObject, &ccfs);
    }
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fileref->parent->fcb->inode_item.transid = Vcb->superblock.generation;
    fileref->parent->fcb->inode_item.sequence++;
    fileref->parent->fcb->inode_item.st_ctime = now;
    
    fileref->parent->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fileref->parent->fcb);

    fileref->parent->fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fileref->parent->fcb->subvol->root_item.ctime = now;

    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL set_end_of_file_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, BOOL advance_only) {
    FILE_END_OF_FILE_INFORMATION* feofi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    NTSTATUS Status;
    LARGE_INTEGER time;
    CC_FILE_SIZES ccfs;
    LIST_ENTRY rollback;
    BOOL set_size = FALSE;
    
    if (!fileref) {
        ERR("fileref is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    InitializeListHead(&rollback);
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fileref ? fileref->deleted : fcb->deleted) {
        Status = STATUS_FILE_CLOSED;
        goto end;
    }
    
    if (fcb->ads) {
        Status = stream_set_end_of_file_information(Vcb, feofi->EndOfFile.QuadPart, fcb, fileref, FileObject, advance_only, &rollback);
        goto end;
    }
    
    TRACE("file: %S\n", file_desc(FileObject));
    TRACE("paging IO: %s\n", Irp->Flags & IRP_PAGING_IO ? "TRUE" : "FALSE");
    TRACE("FileObject: AllocationSize = %llx, FileSize = %llx, ValidDataLength = %llx\n",
        fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
    
//     int3;
    TRACE("setting new end to %llx bytes (currently %llx)\n", feofi->EndOfFile.QuadPart, fcb->inode_item.st_size);
    
//     if (feofi->EndOfFile.QuadPart==0x36c000)
//         int3;
    
    if (feofi->EndOfFile.QuadPart < fcb->inode_item.st_size) {
        if (advance_only) {
            Status = STATUS_SUCCESS;
            goto end;
        }
        
        TRACE("truncating file to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        if (!MmCanFileBeTruncated(&fcb->nonpaged->segment_object, &feofi->EndOfFile)) {
            Status = STATUS_USER_MAPPED_FILE;
            goto end;
        }
        
        Status = truncate_file(fcb, feofi->EndOfFile.QuadPart, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - truncate_file failed\n");
            goto end;
        }
    } else if (feofi->EndOfFile.QuadPart > fcb->inode_item.st_size) {
        if (Irp->Flags & IRP_PAGING_IO) {
            TRACE("paging IO tried to extend file size\n");
            Status = STATUS_SUCCESS;
            goto end;
        }
        
        TRACE("extending file to %llx bytes\n", feofi->EndOfFile.QuadPart);
        
        Status = extend_file(fcb, fileref, feofi->EndOfFile.QuadPart, TRUE, NULL, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - extend_file failed\n");
            goto end;
        }
    }
    
    ccfs.AllocationSize = fcb->Header.AllocationSize;
    ccfs.FileSize = fcb->Header.FileSize;
    ccfs.ValidDataLength = fcb->Header.ValidDataLength;
    set_size = TRUE;
    
    if (!ccb->user_set_write_time) {
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &fcb->inode_item.st_mtime);
    }
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE, FILE_ACTION_MODIFIED);

    Status = STATUS_SUCCESS;

end:
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    
    if (set_size)
        CcSetFileSizes(FileObject, &ccfs);
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
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
    
    TRACE("setting the position on %S to %llx\n", file_desc(FileObject), fpi->CurrentByteOffset.QuadPart);
    
    // FIXME - make sure aligned for FO_NO_INTERMEDIATE_BUFFERING
    
    FileObject->CurrentByteOffset = fpi->CurrentByteOffset;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL set_link_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, PFILE_OBJECT tfo) {
    FILE_LINK_INFORMATION* fli = Irp->AssociatedIrp.SystemBuffer;
    fcb *fcb = FileObject->FsContext, *tfofcb, *parfcb;
    ccb* ccb = FileObject->FsContext2;
    file_ref *fileref = ccb ? ccb->fileref : NULL, *oldfileref = NULL, *related = NULL, *fr2 = NULL;
    UINT64 index;
    WCHAR* fn;
    ULONG fnlen, utf8len;
    UNICODE_STRING fnus;
    ANSI_STRING utf8;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    LIST_ENTRY rollback;
    hardlink* hl;
    ACCESS_MASK access;
    SECURITY_SUBJECT_CONTEXT subjcont;
    dir_child* dc = NULL;
    
    InitializeListHead(&rollback);
    
    // FIXME - check fli length
    // FIXME - don't ignore fli->RootDirectory
    
    TRACE("ReplaceIfExists = %x\n", fli->ReplaceIfExists);
    TRACE("RootDirectory = %p\n", fli->RootDirectory);
    TRACE("FileNameLength = %x\n", fli->FileNameLength);
    TRACE("FileName = %.*S\n", fli->FileNameLength / sizeof(WCHAR), fli->FileName);
    
    fn = fli->FileName;
    fnlen = fli->FileNameLength / sizeof(WCHAR);
    
    if (!tfo) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref set and no directory given\n");
            return STATUS_INVALID_PARAMETER;
        }
        
        parfcb = fileref->parent->fcb;
        tfofcb = NULL;
    } else {
        LONG i;
        
        tfofcb = tfo->FsContext;
        parfcb = tfofcb;
        
        while (fnlen > 0 && (fli->FileName[fnlen - 1] == '/' || fli->FileName[fnlen - 1] == '\\'))
            fnlen--;
        
        if (fnlen == 0)
            return STATUS_INVALID_PARAMETER;
        
        for (i = fnlen - 1; i >= 0; i--) {
            if (fli->FileName[i] == '\\' || fli->FileName[i] == '/') {
                fn = &fli->FileName[i+1];
                fnlen = (fli->FileNameLength / sizeof(WCHAR)) - i - 1;
                break;
            }
        }
    }
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        WARN("tried to create hard link on directory\n");
        Status = STATUS_FILE_IS_A_DIRECTORY;
        goto end;
    }
    
    if (fcb->ads) {
        WARN("tried to create hard link on stream\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
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
    
    if (tfo && tfo->FsContext2) {
        struct _ccb* relatedccb = tfo->FsContext2;
        
        related = relatedccb->fileref;
        increase_fileref_refcount(related);
    }

    Status = open_fileref(Vcb, &oldfileref, &fnus, related, FALSE, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

    if (NT_SUCCESS(Status)) {
        if (!oldfileref->deleted) {
            WARN("destination file %S already exists\n", file_desc_fileref(oldfileref));
        
            if (!fli->ReplaceIfExists) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            } else if (oldfileref->open_count >= 1 && !oldfileref->deleted) {
                WARN("trying to overwrite open file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (fileref == oldfileref) {
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
            
            if (oldfileref->fcb->type == BTRFS_TYPE_DIRECTORY) {
                WARN("trying to overwrite directory\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            }
        } else {
            free_fileref(oldfileref);
            oldfileref = NULL;
        }
    }
    
    if (!related) {
        Status = open_fileref(Vcb, &related, &fnus, NULL, TRUE, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08x\n", Status);
            goto end;
        }
    }
    
    SeCaptureSubjectContext(&subjcont);

    if (!SeAccessCheck(related->fcb->sd, &subjcont, FALSE, FILE_ADD_FILE, 0, NULL,
                       IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
        SeReleaseSubjectContext(&subjcont);
        WARN("SeAccessCheck failed, returning %08x\n", Status);
        goto end;
    }

    SeReleaseSubjectContext(&subjcont);
    
    if (fcb->subvol != parfcb->subvol) {
        WARN("can't create hard link over subvolume boundary\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (oldfileref) {
        SeCaptureSubjectContext(&subjcont);

        if (!SeAccessCheck(oldfileref->fcb->sd, &subjcont, FALSE, DELETE, 0, NULL,
                           IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
            SeReleaseSubjectContext(&subjcont);
            WARN("SeAccessCheck failed, returning %08x\n", Status);
            goto end;
        }

        SeReleaseSubjectContext(&subjcont);
        
        Status = delete_fileref(oldfileref, NULL, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_fileref returned %08x\n", Status);
            goto end;
        }
    }
    
    Status = fcb_get_last_dir_index(related->fcb, &index, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_last_dir_index returned %08x\n", Status);
        goto end;
    }
    
    fr2 = create_fileref();
    
    fr2->fcb = fcb;
    fcb->refcount++;
    
    fr2->utf8 = utf8;
    fr2->index = index;
    fr2->created = TRUE;
    fr2->parent = related;
    
    fr2->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);
    if (!fr2->filepart.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fr2->filepart.Length = fr2->filepart.MaximumLength = fnus.Length;
    RtlCopyMemory(fr2->filepart.Buffer, fnus.Buffer, fnus.Length);
    
    Status = RtlUpcaseUnicodeString(&fr2->filepart_uc, &fr2->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        goto end;
    }
      
    insert_fileref_child(related, fr2, TRUE);
    
    Status = add_dir_child(related->fcb, fcb->inode, FALSE, index, &utf8, &fr2->filepart, &fr2->filepart_uc, fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);
    
    fr2->dc = dc;
    dc->fileref = fr2;

    // add hardlink for existing fileref, if it's not there already
    if (IsListEmpty(&fcb->hardlinks)) {
        hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
        if (!hl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        hl->parent = fileref->parent->fcb->inode;
        hl->index = fileref->index;
        
        hl->name.Length = hl->name.MaximumLength = fileref->filepart.Length;
        hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
        
        if (!hl->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(hl->name.Buffer, fileref->filepart.Buffer, fileref->filepart.Length);
        
        hl->utf8.Length = hl->utf8.MaximumLength = fileref->utf8.Length;
        hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
        
        if (!hl->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl->name.Buffer);
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(hl->utf8.Buffer, fileref->utf8.Buffer, fileref->utf8.Length);
        
        InsertTailList(&fcb->hardlinks, &hl->list_entry);
    }
    
    hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
    if (!hl) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    hl->parent = related->fcb->inode;
    hl->index = index;
    
    hl->name.Length = hl->name.MaximumLength = fnus.Length;
    hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
    
    if (!hl->name.Buffer) {
        ERR("out of memory\n");
        ExFreePool(hl);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(hl->name.Buffer, fnus.Buffer, fnus.Length);
    
    hl->utf8.Length = hl->utf8.MaximumLength = utf8.Length;
    hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
    
    if (!hl->utf8.Buffer) {
        ERR("out of memory\n");
        ExFreePool(hl->name.Buffer);
        ExFreePool(hl);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(hl->utf8.Buffer, utf8.Buffer, utf8.Length);
    
    InsertTailList(&fcb->hardlinks, &hl->list_entry);
    
    mark_fileref_dirty(fr2);
    free_fileref(fr2);
    
    // update inode's INODE_ITEM
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    fcb->inode_item.st_nlink++;
    
    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    // update parent's INODE_ITEM
    
    parfcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("parfcb->inode_item.st_size (inode %llx) was %llx\n", parfcb->inode, parfcb->inode_item.st_size);
    parfcb->inode_item.st_size += 2 * utf8len;
    TRACE("parfcb->inode_item.st_size (inode %llx) now %llx\n", parfcb->inode, parfcb->inode_item.st_size);
    parfcb->inode_item.sequence++;
    parfcb->inode_item.st_ctime = now;
    
    parfcb->inode_item_changed = TRUE;
    mark_fcb_dirty(parfcb);
    
    send_notification_fileref(fr2, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);

    Status = STATUS_SUCCESS;
    
end:
    if (oldfileref)
        free_fileref(oldfileref);
    
    if (!NT_SUCCESS(Status) && related)
        free_fileref(related);
    
    if (!NT_SUCCESS(Status) && fr2)
        free_fileref(fr2);
    
    if (NT_SUCCESS(Status))
        clear_rollback(Vcb, &rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

NTSTATUS STDCALL drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    fcb* fcb = IrpSp->FileObject->FsContext;
    ccb* ccb = IrpSp->FileObject->FsContext2;
    BOOL top_level;

    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    if (!(Vcb->Vpb->Flags & VPB_MOUNTED)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    if (Vcb->readonly && IrpSp->Parameters.SetFile.FileInformationClass != FilePositionInformation) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }
    
    if (!fcb) {
        ERR("no fcb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (!ccb) {
        ERR("no ccb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY && IrpSp->Parameters.SetFile.FileInformationClass != FilePositionInformation &&
        (fcb->inode != SUBVOL_ROOT_INODE || IrpSp->Parameters.SetFile.FileInformationClass != FileBasicInformation)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Irp->IoStatus.Information = 0;
    
    Status = STATUS_NOT_IMPLEMENTED;

    TRACE("set information\n");

    switch (IrpSp->Parameters.SetFile.FileInformationClass) {
        case FileAllocationInformation:
        {
            TRACE("FileAllocationInformation\n");
            
            if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_DATA)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }
            
            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, FALSE);
            break;
        }

        case FileBasicInformation:
        {
            TRACE("FileBasicInformation\n");
            
            if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }
            
            Status = set_basic_information(Vcb, Irp, IrpSp->FileObject);
            
            break;
        }

        case FileDispositionInformation:
        {
            TRACE("FileDispositionInformation\n");
            
            if (Irp->RequestorMode == UserMode && !(ccb->access & DELETE)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }
            
            Status = set_disposition_information(Vcb, Irp, IrpSp->FileObject);
            
            break;
        }

        case FileEndOfFileInformation:
        {
            TRACE("FileEndOfFileInformation\n");
            
            if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }
            
            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.AdvanceOnly);
            
            break;
        }

        case FileLinkInformation:
            TRACE("FileLinkInformation\n");
            Status = set_link_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject);
            break;

        case FilePositionInformation:
        {
            TRACE("FilePositionInformation\n");
            
            Status = set_position_information(Vcb, Irp, IrpSp->FileObject);
            
            break;
        }

        case FileRenameInformation:
            TRACE("FileRenameInformation\n");
            // FIXME - make this work with streams
            Status = set_rename_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject);
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
            TRACE("FileRemoteProtocolInformation\n");
            break;
#endif
            
        default:
            WARN("unknown FileInformationClass %u\n", IrpSp->Parameters.SetFile.FileInformationClass);
    }
    
end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL fill_in_file_basic_information(FILE_BASIC_INFORMATION* fbi, INODE_ITEM* ii, LONG* length, fcb* fcb, file_ref* fileref) {
    RtlZeroMemory(fbi, sizeof(FILE_BASIC_INFORMATION));
    
    *length -= sizeof(FILE_BASIC_INFORMATION);
    
    fbi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
    fbi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
    fbi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
    fbi->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    
    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        } else
            fbi->FileAttributes = fileref->parent->fcb->atts;
    } else
        fbi->FileAttributes = fcb->atts;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_network_open_information(FILE_NETWORK_OPEN_INFORMATION* fnoi, fcb* fcb, file_ref* fileref, LONG* length) {
    INODE_ITEM* ii;
    
    if (*length < sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
        WARN("overflow\n");
        return STATUS_BUFFER_OVERFLOW;
    }
    
    RtlZeroMemory(fnoi, sizeof(FILE_NETWORK_OPEN_INFORMATION));
    
    *length -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    
    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        ii = &fileref->parent->fcb->inode_item;
    } else
        ii = &fcb->inode_item;
    
    fnoi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
    fnoi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
    fnoi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
    fnoi->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    
    if (fcb->ads) {
        fnoi->AllocationSize.QuadPart = fnoi->EndOfFile.QuadPart = fcb->adsdata.Length;
        fnoi->FileAttributes = fileref->parent->fcb->atts;
    } else {
        fnoi->AllocationSize.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        fnoi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fnoi->FileAttributes = fcb->atts;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_standard_information(FILE_STANDARD_INFORMATION* fsi, fcb* fcb, file_ref* fileref, LONG* length) {
    RtlZeroMemory(fsi, sizeof(FILE_STANDARD_INFORMATION));
    
    *length -= sizeof(FILE_STANDARD_INFORMATION);
    
    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        fsi->AllocationSize.QuadPart = fsi->EndOfFile.QuadPart = fcb->adsdata.Length;
        fsi->NumberOfLinks = fileref->parent->fcb->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fileref->parent->fcb->inode_item.st_mode);
    } else {
        fsi->AllocationSize.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        fsi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fcb->inode_item.st_mode);
    }
    
    TRACE("length = %llu\n", fsi->EndOfFile.QuadPart);
    
    fsi->DeletePending = fileref ? fileref->delete_on_close : FALSE;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_internal_information(FILE_INTERNAL_INFORMATION* fii, fcb* fcb, LONG* length) {
    *length -= sizeof(FILE_INTERNAL_INFORMATION);
    
    fii->IndexNumber.QuadPart = make_file_id(fcb->subvol, fcb->inode);
    
    return STATUS_SUCCESS;
}  
    
static NTSTATUS STDCALL fill_in_file_ea_information(FILE_EA_INFORMATION* eai, fcb* fcb, LONG* length) {
    *length -= sizeof(FILE_EA_INFORMATION);
    
    /* This value appears to be the size of the structure NTFS stores on disk, and not,
     * as might be expected, the size of FILE_FULL_EA_INFORMATION (which is what we store).
     * The formula is 4 bytes as a header, followed by 5 + NameLength + ValueLength for each
     * item. */
    
    eai->EaSize = fcb->ealen;
    
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
    
    fai->AlignmentRequirement = first_device(Vcb)->devobj->AlignmentRequirement;
    
    return STATUS_SUCCESS;
}

typedef struct {
    file_ref* fileref;
    LIST_ENTRY list_entry;
} fileref_list;

NTSTATUS fileref_get_filename(file_ref* fileref, PUNICODE_STRING fn, USHORT* name_offset) {
    LIST_ENTRY fr_list, *le;
    file_ref* fr;
    NTSTATUS Status;
    ULONG len, i;
    
    // FIXME - we need a lock on filerefs' filepart
    
    if (fileref == fileref->fcb->Vcb->root_fileref) {
        fn->Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(WCHAR), ALLOC_TAG);
        if (!fn->Buffer) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        fn->Length = fn->MaximumLength = sizeof(WCHAR);
        fn->Buffer[0] = '\\';

        if (name_offset)
            *name_offset = 0;

        return STATUS_SUCCESS;
    }
    
    InitializeListHead(&fr_list);
    
    len = 0;
    fr = fileref;
    
    do {
        fileref_list* frl;
        
        frl = ExAllocatePoolWithTag(PagedPool, sizeof(fileref_list), ALLOC_TAG);
        if (!frl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        frl->fileref = fr;
        InsertTailList(&fr_list, &frl->list_entry);
        
        len += fr->filepart.Length;
        
        if (fr != fileref->fcb->Vcb->root_fileref)
            len += sizeof(WCHAR);
        
        fr = fr->parent;
    } while (fr);
    
    fn->Buffer = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
    if (!fn->Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fn->Length = fn->MaximumLength = len;
    
    i = 0;
    
    le = fr_list.Blink;
    while (le != &fr_list) {
        fileref_list* frl = CONTAINING_RECORD(le, fileref_list, list_entry);
        
        if (frl->fileref != fileref->fcb->Vcb->root_fileref) {
            fn->Buffer[i] = frl->fileref->fcb->ads ? ':' : '\\';
            i++;
            
            if (name_offset && frl->fileref == fileref)
                *name_offset = i * sizeof(WCHAR);
            
            RtlCopyMemory(&fn->Buffer[i], frl->fileref->filepart.Buffer, frl->fileref->filepart.Length);
            i += frl->fileref->filepart.Length / sizeof(WCHAR);
        }
        
        le = le->Blink;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    while (!IsListEmpty(&fr_list)) {
        fileref_list* frl;
        
        le = RemoveHeadList(&fr_list);
        frl = CONTAINING_RECORD(le, fileref_list, list_entry);
        
        ExFreePool(frl);
    }
    
    return Status;
}

static NTSTATUS STDCALL fill_in_file_name_information(FILE_NAME_INFORMATION* fni, fcb* fcb, file_ref* fileref, LONG* length) {
#ifdef _DEBUG
    ULONG retlen = 0;
#endif
    UNICODE_STRING fn;
    NTSTATUS Status;
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    ULONG datasuflen = wcslen(datasuf) * sizeof(WCHAR);
    
    if (!fileref) {
        ERR("called without fileref\n");
        return STATUS_INVALID_PARAMETER;
    }
   
    RtlZeroMemory(fni, sizeof(FILE_NAME_INFORMATION));
    
    *length -= (LONG)offsetof(FILE_NAME_INFORMATION, FileName[0]);
    
    TRACE("maximum length is %u\n", *length);
    fni->FileNameLength = 0;
    
    fni->FileName[0] = 0;
    
    Status = fileref_get_filename(fileref, &fn, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("fileref_get_filename returned %08x\n", Status);
        return Status;
    }
    
    if (*length >= (LONG)fn.Length) {
        RtlCopyMemory(fni->FileName, fn.Buffer, fn.Length);
#ifdef _DEBUG
        retlen = fn.Length;
#endif
        *length -= fn.Length;
    } else {
        if (*length > 0) {
            RtlCopyMemory(fni->FileName, fn.Buffer, *length);
#ifdef _DEBUG
            retlen = *length;
#endif
        }
        *length = -1;
    }
    
    fni->FileNameLength = fn.Length;
    
    if (fcb->ads) {
        if (*length >= (LONG)datasuflen) {
            RtlCopyMemory(&fni->FileName[fn.Length / sizeof(WCHAR)], datasuf, datasuflen);
#ifdef _DEBUG
            retlen += datasuflen;
#endif
            *length -= datasuflen;
        } else {
            if (*length > 0) {
                RtlCopyMemory(&fni->FileName[fn.Length / sizeof(WCHAR)], datasuf, *length);
#ifdef _DEBUG
                retlen += *length;
#endif
            }
            *length = -1;
        }
    }
    
    ExFreePool(fn.Buffer);
    
    TRACE("%.*S\n", retlen / sizeof(WCHAR), fni->FileName);

    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL fill_in_file_attribute_information(FILE_ATTRIBUTE_TAG_INFORMATION* ati, fcb* fcb, file_ref* fileref, PIRP Irp, LONG* length) {
    *length -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    
    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        ati->FileAttributes = fileref->parent->fcb->atts;
    } else
        ati->FileAttributes = fcb->atts;
    
    if (!(ati->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        ati->ReparseTag = 0;
    else
        ati->ReparseTag = get_reparse_tag(fcb->Vcb, fcb->subvol, fcb->inode, fcb->type, fcb->atts, Irp);
    
    return STATUS_SUCCESS;
}

typedef struct {
    UNICODE_STRING name;
    UINT64 size;
    BOOL ignore;
    LIST_ENTRY list_entry;
} stream_info;

static NTSTATUS STDCALL fill_in_file_stream_information(FILE_STREAM_INFORMATION* fsi, file_ref* fileref, PIRP Irp, LONG* length) {
    ULONG reqsize;
    LIST_ENTRY streamlist, *le;
    FILE_STREAM_INFORMATION *entry, *lastentry;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    stream_info* si;
    
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    static char xapref[] = "user.";
    UNICODE_STRING suf;
    
    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    InitializeListHead(&streamlist);
    
    ExAcquireResourceSharedLite(&fileref->fcb->Vcb->tree_lock, TRUE);
    ExAcquireResourceSharedLite(fileref->fcb->Header.Resource, TRUE);
    
    suf.Buffer = datasuf;
    suf.Length = suf.MaximumLength = wcslen(datasuf) * sizeof(WCHAR);
    
    searchkey.obj_id = fileref->fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = 0;

    Status = find_item(fileref->fcb->Vcb, fileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (fileref->fcb->type != BTRFS_TYPE_DIRECTORY) {
        si = ExAllocatePoolWithTag(PagedPool, sizeof(stream_info), ALLOC_TAG);
        if (!si) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        si->name.Length = si->name.MaximumLength = 0;
        si->name.Buffer = NULL;
        si->size = fileref->fcb->inode_item.st_size;
        si->ignore = FALSE;
        
        InsertTailList(&streamlist, &si->list_entry);
    }
    
    do {
        if (tp.item->key.obj_id == fileref->fcb->inode && tp.item->key.obj_type == TYPE_XATTR_ITEM) {
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
                        (tp.item->key.offset != EA_DOSATTRIB_HASH || xa->n != strlen(EA_DOSATTRIB) || RtlCompareMemory(xa->name, EA_DOSATTRIB, xa->n) != xa->n) &&
                        (tp.item->key.offset != EA_EA_HASH || xa->n != strlen(EA_EA) || RtlCompareMemory(xa->name, EA_EA, xa->n) != xa->n)
                    ) {
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, &xa->name[strlen(xapref)], xa->n - strlen(xapref));
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                            goto end;
                        }
                        
                        si = ExAllocatePoolWithTag(PagedPool, sizeof(stream_info), ALLOC_TAG);
                        if (!si) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        si->name.Buffer = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!si->name.Buffer) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(si);
                            goto end;
                        }
                            
                        Status = RtlUTF8ToUnicodeN(si->name.Buffer, stringlen, &stringlen, &xa->name[strlen(xapref)], xa->n - strlen(xapref));
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                            ExFreePool(si->name.Buffer);
                            ExFreePool(si);
                            goto end;
                        }
                        
                        si->name.Length = si->name.MaximumLength = stringlen;
                        
                        si->size = xa->m;
                        
                        si->ignore = FALSE;
                        
                        TRACE("stream name = %.*S (length = %u)\n", si->name.Length / sizeof(WCHAR), si->name.Buffer, si->name.Length / sizeof(WCHAR));
                        
                        InsertTailList(&streamlist, &si->list_entry);
                    }
                    
                    len -= sizeof(DIR_ITEM) - sizeof(char) + xa->n + xa->m;
                    xa = (DIR_ITEM*)&xa->name[xa->n + xa->m]; // FIXME - test xattr hash collisions work
                } while (len > 0);
            }
        }
        
        b = find_next_item(fileref->fcb->Vcb, &tp, &next_tp, FALSE, Irp);
        if (b) {
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fileref->fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);
    
    ExAcquireResourceSharedLite(&fileref->nonpaged->children_lock, TRUE);
    
    le = fileref->children.Flink;
    while (le != &fileref->children) {
        file_ref* fr = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (fr->fcb && fr->fcb->ads) {
            LIST_ENTRY* le2 = streamlist.Flink;
            BOOL found = FALSE;
            
            while (le2 != &streamlist) {
                si = CONTAINING_RECORD(le2, stream_info, list_entry);
                
                if (si && si->name.Buffer && si->name.Length == fr->filepart.Length &&
                    RtlCompareMemory(si->name.Buffer, fr->filepart.Buffer, si->name.Length) == si->name.Length) {
                    
                    si->size = fr->fcb->adsdata.Length;
                    si->ignore = fr->fcb->deleted;
                    
                    found = TRUE;
                    break;
                }
                
                le2 = le2->Flink;
            }
            
            if (!found && !fr->fcb->deleted) {
                si = ExAllocatePoolWithTag(PagedPool, sizeof(stream_info), ALLOC_TAG);
                if (!si) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                si->name.Length = si->name.MaximumLength = fr->filepart.Length;
                
                si->name.Buffer = ExAllocatePoolWithTag(PagedPool, si->name.MaximumLength, ALLOC_TAG);
                if (!si->name.Buffer) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    ExFreePool(si);
                    goto end;
                }
                
                RtlCopyMemory(si->name.Buffer, fr->filepart.Buffer, fr->filepart.Length);
                
                si->size = fr->fcb->adsdata.Length;
                si->ignore = FALSE;
                
                InsertTailList(&streamlist, &si->list_entry);
            }
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&fileref->nonpaged->children_lock);
    
    reqsize = 0;
    
    le = streamlist.Flink;
    while (le != &streamlist) {
        si = CONTAINING_RECORD(le, stream_info, list_entry);
        
        if (!si->ignore) {
            reqsize = sector_align(reqsize, sizeof(LONGLONG));
            reqsize += sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + si->name.Length;
        }

        le = le->Flink;
    }        
    
    TRACE("length = %i, reqsize = %u\n", *length, reqsize);
    
    if (reqsize > *length) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto end;
    }
    
    entry = fsi;
    lastentry = NULL;
    
    le = streamlist.Flink;
    while (le != &streamlist) {
        si = CONTAINING_RECORD(le, stream_info, list_entry);
        
        if (!si->ignore) {
            ULONG off;
            
            entry->NextEntryOffset = 0;
            entry->StreamNameLength = si->name.Length + suf.Length + sizeof(WCHAR);
            entry->StreamSize.QuadPart = si->size;
            
            if (le == streamlist.Flink)
                entry->StreamAllocationSize.QuadPart = sector_align(fileref->fcb->inode_item.st_size, fileref->fcb->Vcb->superblock.sector_size);
            else
                entry->StreamAllocationSize.QuadPart = si->size;
            
            entry->StreamName[0] = ':';
            
            if (si->name.Length > 0)
                RtlCopyMemory(&entry->StreamName[1], si->name.Buffer, si->name.Length);
            
            RtlCopyMemory(&entry->StreamName[1 + (si->name.Length / sizeof(WCHAR))], suf.Buffer, suf.Length);
            
            if (lastentry)
                lastentry->NextEntryOffset = (UINT8*)entry - (UINT8*)lastentry;
            
            off = sector_align(sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + si->name.Length, sizeof(LONGLONG));

            lastentry = entry;
            entry = (FILE_STREAM_INFORMATION*)((UINT8*)entry + off);
        }
        
        le = le->Flink;
    }
    
    *length -= reqsize;
    
    Status = STATUS_SUCCESS;
    
end:
    while (!IsListEmpty(&streamlist)) {
        le = RemoveHeadList(&streamlist);
        si = CONTAINING_RECORD(le, stream_info, list_entry);
        
        if (si->name.Buffer)
            ExFreePool(si->name.Buffer);
        
        ExFreePool(si);
    }
    
    ExReleaseResourceLite(fileref->fcb->Header.Resource);
    ExReleaseResourceLite(&fileref->fcb->Vcb->tree_lock);
    
    return Status;
}

#ifndef __REACTOS__
static NTSTATUS STDCALL fill_in_file_standard_link_information(FILE_STANDARD_LINK_INFORMATION* fsli, fcb* fcb, file_ref* fileref, LONG* length) {
    TRACE("FileStandardLinkInformation\n");
    
    // FIXME - NumberOfAccessibleLinks should subtract open links which have been marked as delete_on_close
    
    fsli->NumberOfAccessibleLinks = fcb->inode_item.st_nlink;
    fsli->TotalNumberOfLinks = fcb->inode_item.st_nlink;
    fsli->DeletePending = fileref ? fileref->delete_on_close : FALSE;
    fsli->Directory = fcb->type == BTRFS_TYPE_DIRECTORY ? TRUE : FALSE;
    
    *length -= sizeof(FILE_STANDARD_LINK_INFORMATION);
    
    return STATUS_SUCCESS;
}
#endif /* __REACTOS__ */

typedef struct {
    UNICODE_STRING name;
    UINT64 inode;
    LIST_ENTRY list_entry;
} name_bit;

static NTSTATUS get_subvol_path(device_extension* Vcb, root* subvol, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    LIST_ENTRY* le;
    root* parsubvol;
    UNICODE_STRING dirpath;
    ROOT_REF* rr;
    ULONG namelen;
    
    // FIXME - add subvol->parent field
    
    if (subvol == Vcb->root_fileref->fcb->subvol) {
        subvol->path.Length = subvol->path.MaximumLength = sizeof(WCHAR);
        subvol->path.Buffer = ExAllocatePoolWithTag(PagedPool, subvol->path.Length, ALLOC_TAG);
        subvol->path.Buffer[0] = '\\';
        return STATUS_SUCCESS;
    }
    
    searchkey.obj_id = subvol->id;
    searchkey.obj_type = TYPE_ROOT_BACKREF;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) { // top subvol
        subvol->path.Length = subvol->path.MaximumLength = sizeof(WCHAR);
        subvol->path.Buffer = ExAllocatePoolWithTag(PagedPool, subvol->path.Length, ALLOC_TAG);
        subvol->path.Buffer[0] = '\\';
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size < sizeof(ROOT_REF)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(ROOT_REF));
        return STATUS_INTERNAL_ERROR;
    }
    
    rr = (ROOT_REF*)tp.item->data;
    
    if (tp.item->size < sizeof(ROOT_REF) - 1 + rr->n) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(ROOT_REF) - 1 + rr->n);
        return STATUS_INTERNAL_ERROR;
    }
    
    le = Vcb->roots.Flink;

    parsubvol = NULL;

    while (le != &Vcb->roots) {
        root* r2 = CONTAINING_RECORD(le, root, list_entry);
        
        if (r2->id == tp.item->key.offset) {
            parsubvol = r2;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!parsubvol) {
        ERR("unable to find subvol %llx\n", tp.item->key.offset);
        return STATUS_INTERNAL_ERROR;
    }
    
    // FIXME - recursion

    Status = get_inode_dir_path(Vcb, parsubvol, rr->dir, &dirpath, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("get_inode_dir_path returned %08x\n", Status);
        return Status;
    }
    
    Status = RtlUTF8ToUnicodeN(NULL, 0, &namelen, rr->name, rr->n);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
        goto end;
    }
    
    if (namelen == 0) {
        ERR("length was 0\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    subvol->path.Length = subvol->path.MaximumLength = dirpath.Length + namelen;
    subvol->path.Buffer = ExAllocatePoolWithTag(PagedPool, subvol->path.Length, ALLOC_TAG);
    
    if (!subvol->path.Buffer) {  
        ERR("out of memory\n");
        
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(subvol->path.Buffer, dirpath.Buffer, dirpath.Length);
        
    Status = RtlUTF8ToUnicodeN(&subvol->path.Buffer[dirpath.Length / sizeof(WCHAR)], namelen, &namelen, rr->name, rr->n);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
        goto end;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    if (dirpath.Buffer)
        ExFreePool(dirpath.Buffer);
    
    if (!NT_SUCCESS(Status) && subvol->path.Buffer) {
        ExFreePool(subvol->path.Buffer);
        subvol->path.Buffer = NULL;
    }
    
    return Status;
}

static NTSTATUS get_inode_dir_path(device_extension* Vcb, root* subvol, UINT64 inode, PUNICODE_STRING us, PIRP Irp) {
    KEY searchkey;
    NTSTATUS Status;
    UINT64 in;
    traverse_ptr tp;
    LIST_ENTRY name_trail, *le;
    UINT16 levels = 0;
    UINT32 namelen = 0;
    WCHAR* usbuf;
    
    InitializeListHead(&name_trail);
    
    in = inode;
    
    // FIXME - start with subvol prefix
    if (!subvol->path.Buffer) {
        Status = get_subvol_path(Vcb, subvol, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("get_subvol_path returned %08x\n", Status);
            return Status;
        }
    }
    
    while (in != subvol->root_item.objid) {
        searchkey.obj_id = in;
        searchkey.obj_type = TYPE_INODE_EXTREF;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id) {
            ERR("could not find INODE_REF for inode %llx in subvol %llx\n", searchkey.obj_id, subvol->id);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if (tp.item->key.obj_type == TYPE_INODE_REF) {
            INODE_REF* ir = (INODE_REF*)tp.item->data;
            name_bit* nb;
            ULONG len;
            
            if (tp.item->size < sizeof(INODE_REF)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_REF));
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            if (tp.item->size < sizeof(INODE_REF) - 1 + ir->n) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_REF) - 1 + ir->n);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            nb = ExAllocatePoolWithTag(PagedPool, sizeof(name_bit), ALLOC_TAG);
            if (!nb) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            nb->name.Buffer = NULL;
            
            InsertTailList(&name_trail, &nb->list_entry);
            levels++;
            
            Status = RtlUTF8ToUnicodeN(NULL, 0, &len, ir->name, ir->n);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                goto end;
            }
            
            if (len == 0) {
                ERR("length was 0\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            nb->name.Length = nb->name.MaximumLength = len;
            
            nb->name.Buffer = ExAllocatePoolWithTag(PagedPool, nb->name.Length, ALLOC_TAG);
            if (!nb->name.Buffer) {  
                ERR("out of memory\n");
                
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
                
            Status = RtlUTF8ToUnicodeN(nb->name.Buffer, len, &len, ir->name, ir->n);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                goto end;
            }
            
            in = tp.item->key.offset;
            namelen += nb->name.Length;
            
//         } else if (tp.item->key.obj_type == TYPE_INODE_EXTREF) {
//             // FIXME
        } else {
            ERR("could not find INODE_REF for inode %llx in subvol %llx\n", searchkey.obj_id, subvol->id);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
    }
    
    namelen += (levels + 1) * sizeof(WCHAR);
    
    us->Length = us->MaximumLength = namelen;
    us->Buffer = ExAllocatePoolWithTag(PagedPool, us->Length, ALLOC_TAG);
    
    if (!us->Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    us->Buffer[0] = '\\';
    usbuf = &us->Buffer[1];
    
    le = name_trail.Blink;
    while (le != &name_trail) {
        name_bit* nb = CONTAINING_RECORD(le, name_bit, list_entry);
        
        RtlCopyMemory(usbuf, nb->name.Buffer, nb->name.Length);
        usbuf += nb->name.Length / sizeof(WCHAR);
        
        usbuf[0] = '\\';
        usbuf++;
        
        le = le->Blink;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    while (!IsListEmpty(&name_trail)) {
        name_bit* nb = CONTAINING_RECORD(name_trail.Flink, name_bit, list_entry);
        
        if (nb->name.Buffer)
            ExFreePool(nb->name.Buffer);
        
        RemoveEntryList(&nb->list_entry);
        
        ExFreePool(nb);
    }

    return Status;
}

NTSTATUS open_fileref_by_inode(device_extension* Vcb, root* subvol, UINT64 inode, file_ref** pfr, PIRP Irp) {
    NTSTATUS Status;
    fcb* fcb;
    hardlink* hl;
    file_ref *parfr, *fr;
    dir_child* dc = NULL;
    
    Status = open_fcb(Vcb, subvol, inode, 0, NULL, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        return Status;
    }
    
    if (fcb->fileref) {
        *pfr = fcb->fileref;
        increase_fileref_refcount(fcb->fileref);
        return STATUS_SUCCESS;
    }
    
    // find hardlink if fcb doesn't have any loaded
    if (IsListEmpty(&fcb->hardlinks)) {
        KEY searchkey;
        traverse_ptr tp;
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_INODE_EXTREF;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            free_fcb(fcb);
            return Status;
        }
        
        if (tp.item->key.obj_id == fcb->inode) {
            if (tp.item->key.obj_type == TYPE_INODE_REF) {
                INODE_REF* ir;
                ULONG stringlen;

                ir = (INODE_REF*)tp.item->data;

                hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                hl->parent = tp.item->key.offset;
                hl->index = ir->index;
                
                hl->utf8.Length = hl->utf8.MaximumLength = ir->n;
                
                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
                    RtlCopyMemory(hl->utf8.Buffer, ir->name, ir->n);
                }
                
                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, ir->name, ir->n);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                    ExFreePool(hl);
                    free_fcb(fcb);
                    return Status;
                }
                
                hl->name.Length = hl->name.MaximumLength = stringlen;
                
                if (stringlen == 0)
                    hl->name.Buffer = NULL;
                else {
                    hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
                    
                    if (!hl->name.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(hl);
                        free_fcb(fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    Status = RtlUTF8ToUnicodeN(hl->name.Buffer, stringlen, &stringlen, ir->name, ir->n);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        ExFreePool(hl->name.Buffer);
                        ExFreePool(hl);
                        free_fcb(fcb);
                        return Status;
                    }
                }
                    
                InsertTailList(&fcb->hardlinks, &hl->list_entry);
            } else if (tp.item->key.obj_type == TYPE_INODE_EXTREF) {
                INODE_EXTREF* ier;
                hardlink* hl;
                ULONG stringlen;

                ier = (INODE_EXTREF*)tp.item->data;
                
                hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                hl->parent = ier->dir;
                hl->index = ier->index;
                
                hl->utf8.Length = hl->utf8.MaximumLength = ier->n;
                
                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
                    RtlCopyMemory(hl->utf8.Buffer, ier->name, ier->n);
                }
                
                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, ier->name, ier->n);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                    ExFreePool(hl);
                    free_fcb(fcb);
                    return Status;
                }
                
                hl->name.Length = hl->name.MaximumLength = stringlen;
                
                if (stringlen == 0)
                    hl->name.Buffer = NULL;
                else {
                    hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
                    
                    if (!hl->name.Buffer) {
                        ERR("out of memory\n");
                        ExFreePool(hl);
                        free_fcb(fcb);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    Status = RtlUTF8ToUnicodeN(hl->name.Buffer, stringlen, &stringlen, ier->name, ier->n);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        ExFreePool(hl->name.Buffer);
                        ExFreePool(hl);
                        free_fcb(fcb);
                        return Status;
                    }
                }
                
                InsertTailList(&fcb->hardlinks, &hl->list_entry);
            }
        }
    }
    
    if (IsListEmpty(&fcb->hardlinks)) {
        ERR("subvol %llx, inode %llx has no hardlinks\n", subvol->id, inode);
        free_fcb(fcb);
        return STATUS_INTERNAL_ERROR;
    }
    
    hl = CONTAINING_RECORD(fcb->hardlinks.Flink, hardlink, list_entry);
    
    // FIXME - does this work with subvols?
    
    if (hl->parent == inode) // root of subvol
        parfr = NULL;
    else {
        Status = open_fileref_by_inode(Vcb, subvol, hl->parent, &parfr, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref_by_inode returned %08x\n", Status);
            free_fcb(fcb);
            return Status;
        }
    }
    
    fr = create_fileref();
    if (!fr) {
        ERR("out of memory\n");
        free_fcb(fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fr->fcb = fcb;
    fcb->fileref = fr;
    
    fr->index = hl->index;
    
    fr->utf8.Length = fr->utf8.MaximumLength = hl->utf8.Length;
    if (fr->utf8.Length > 0) {
        fr->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, fr->utf8.Length, ALLOC_TAG);
        
        if (!fr->utf8.Buffer) {
            ERR("out of memory\n");
            free_fileref(fr);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fr->utf8.Buffer, hl->utf8.Buffer, hl->utf8.Length);
    }
    
    fr->filepart.MaximumLength = fr->filepart.Length = hl->name.Length;
    
    if (fr->filepart.Length > 0) {
        fr->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fr->filepart.MaximumLength, ALLOC_TAG);
        if (!fr->filepart.Buffer) {
            ERR("out of memory\n");
            free_fileref(fr);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fr->filepart.Buffer, hl->name.Buffer, hl->name.Length);
    }
    
    Status = RtlUpcaseUnicodeString(&fr->filepart_uc, &fr->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        free_fileref(fr);
        return Status;
    }
    
    fr->parent = parfr;
    
    Status = add_dir_child(parfr->fcb, fr->fcb->inode == SUBVOL_ROOT_INODE ? fr->fcb->subvol->id : fr->fcb->inode, fr->fcb->inode == SUBVOL_ROOT_INODE,
                           fr->index, &fr->utf8, &fr->filepart, &fr->filepart_uc, fr->fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);
    
    fr->dc = dc;
    dc->fileref = fr;
    
    insert_fileref_child(parfr, fr, TRUE);

    *pfr = fr;
    
    return STATUS_SUCCESS;
}

#ifndef __REACTOS__
static NTSTATUS STDCALL fill_in_hard_link_information(FILE_LINKS_INFORMATION* fli, file_ref* fileref, PIRP Irp, LONG* length) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    ULONG bytes_needed;
    FILE_LINK_ENTRY_INFORMATION* feli;
    BOOL overflow = FALSE;
    fcb* fcb = fileref->fcb;
    ULONG len;
    
    if (fcb->ads)
        return STATUS_INVALID_PARAMETER;
    
    if (*length < offsetof(FILE_LINKS_INFORMATION, Entry))
        return STATUS_INVALID_PARAMETER;
    
    RtlZeroMemory(fli, *length);
    
    bytes_needed = offsetof(FILE_LINKS_INFORMATION, Entry);
    len = bytes_needed;
    feli = NULL;
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    if (fcb->inode == SUBVOL_ROOT_INODE) {
        ULONG namelen;
        
        if (fcb == fcb->Vcb->root_fileref->fcb)
            namelen = sizeof(WCHAR);
        else
            namelen = fileref->filepart.Length;
                    
        bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) - sizeof(WCHAR) + namelen;
        
        if (bytes_needed > *length)
            overflow = TRUE;
        
        if (!overflow) {
            feli = &fli->Entry;
                        
            feli->NextEntryOffset = 0;
            feli->ParentFileId = 0; // we use an inode of 0 to mean the parent of a subvolume
            
            if (fcb == fcb->Vcb->root_fileref->fcb) {
                feli->FileNameLength = 1;
                feli->FileName[0] = '.';
            } else {
                feli->FileNameLength = fileref->filepart.Length / sizeof(WCHAR);
                RtlCopyMemory(feli->FileName, fileref->filepart.Buffer, fileref->filepart.Length);
            }
            
            fli->EntriesReturned++;
            
            len = bytes_needed;
        }
    } else {
        ExAcquireResourceExclusiveLite(&fcb->Vcb->fcb_lock, TRUE);
    
        if (IsListEmpty(&fcb->hardlinks)) {
            bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) + fileref->filepart.Length - sizeof(WCHAR);
            
            if (bytes_needed > *length)
                overflow = TRUE;
            
            if (!overflow) {
                feli = &fli->Entry;

                feli->NextEntryOffset = 0;
                feli->ParentFileId = fileref->parent->fcb->inode;
                feli->FileNameLength = fileref->filepart.Length / sizeof(WCHAR);
                RtlCopyMemory(feli->FileName, fileref->filepart.Buffer, fileref->filepart.Length);
                
                fli->EntriesReturned++;
                
                len = bytes_needed;
            }
        } else {
            le = fcb->hardlinks.Flink;
            while (le != &fcb->hardlinks) {
                hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);
                file_ref* parfr;
                
                TRACE("parent %llx, index %llx, name %.*S\n", hl->parent, hl->index, hl->name.Length / sizeof(WCHAR), hl->name.Buffer);
                
                Status = open_fileref_by_inode(fcb->Vcb, fcb->subvol, hl->parent, &parfr, Irp);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("open_fileref_by_inode returned %08x\n", Status);
                } else if (!parfr->deleted) {
                    LIST_ENTRY* le2;
                    BOOL found = FALSE, deleted = FALSE;
                    UNICODE_STRING* fn;
                    
                    le2 = parfr->children.Flink;
                    while (le2 != &parfr->children) {
                        file_ref* fr2 = CONTAINING_RECORD(le2, file_ref, list_entry);
                        
                        if (fr2->index == hl->index) {
                            found = TRUE;
                            deleted = fr2->deleted;
                            
                            if (!deleted)
                                fn = &fr2->filepart;
                            
                            break;
                        }
                        
                        le2 = le2->Flink;
                    }
                    
                    if (!found)
                        fn = &hl->name;
                    
                    if (!deleted) {
                        TRACE("fn = %.*S (found = %u)\n", fn->Length / sizeof(WCHAR), fn->Buffer, found);
                        
                        if (feli)
                            bytes_needed = sector_align(bytes_needed, 8);
                        
                        bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) + fn->Length - sizeof(WCHAR);
                        
                        if (bytes_needed > *length)
                            overflow = TRUE;
                        
                        if (!overflow) {
                            if (feli) {
                                feli->NextEntryOffset = sector_align(sizeof(FILE_LINK_ENTRY_INFORMATION) + ((feli->FileNameLength - 1) * sizeof(WCHAR)), 8);
                                feli = (FILE_LINK_ENTRY_INFORMATION*)((UINT8*)feli + feli->NextEntryOffset);
                            } else
                                feli = &fli->Entry;
                            
                            feli->NextEntryOffset = 0;
                            feli->ParentFileId = parfr->fcb->inode;
                            feli->FileNameLength = fn->Length / sizeof(WCHAR);
                            RtlCopyMemory(feli->FileName, fn->Buffer, fn->Length);
                            
                            fli->EntriesReturned++;
                            
                            len = bytes_needed;
                        }
                    }
                    
                    free_fileref(parfr);
                }
                
                le = le->Flink;
            }
        }
        
        ExReleaseResourceLite(&fcb->Vcb->fcb_lock);
    }
    
    fli->BytesNeeded = bytes_needed;
    
    *length -= len;

    Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
    
    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}
#endif /* __REACTOS__ */

#if (NTDDI_VERSION >= NTDDI_WIN10)
#ifdef __MINGW32__
typedef struct _FILE_ID_128 {
    UCHAR Identifier[16];
} FILE_ID_128, *PFILE_ID_128; 

typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, *PFILE_ID_INFORMATION;
#endif

static NTSTATUS fill_in_file_id_information(FILE_ID_INFORMATION* fii, fcb* fcb, LONG* length) {
    RtlCopyMemory(&fii->VolumeSerialNumber, &fcb->Vcb->superblock.uuid.uuid[8], sizeof(UINT64));
    RtlCopyMemory(&fii->FileId.Identifier[0], &fcb->inode, sizeof(UINT64));
    RtlCopyMemory(&fii->FileId.Identifier[sizeof(UINT64)], &fcb->subvol->id, sizeof(UINT64));
    
    *length -= sizeof(FILE_ID_INFORMATION);
    
    return STATUS_SUCCESS;
}
#endif

static NTSTATUS STDCALL query_info(device_extension* Vcb, PFILE_OBJECT FileObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    LONG length = IrpSp->Parameters.QueryFile.Length;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p)\n", Vcb, FileObject, Irp);
    TRACE("fcb = %p\n", fcb);
    
    if (fcb == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;
    
    if (!ccb) {
        ERR("ccb is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    switch (IrpSp->Parameters.QueryFile.FileInformationClass) {
        case FileAllInformation:
        {
            FILE_ALL_INFORMATION* fai = Irp->AssociatedIrp.SystemBuffer;
            INODE_ITEM* ii;
            
            TRACE("FileAllInformation\n");
            
            if (Irp->RequestorMode != KernelMode && !(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                goto exit;
            }
            
            if (fcb->ads) {
                if (!fileref || !fileref->parent) {
                    ERR("no fileref for stream\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto exit;
                }
                
                ii = &fileref->parent->fcb->inode_item;
            } else
                ii = &fcb->inode_item;
            
            if (length > 0)
                fill_in_file_basic_information(&fai->BasicInformation, ii, &length, fcb, fileref);
            
            if (length > 0)
                fill_in_file_standard_information(&fai->StandardInformation, fcb, fileref, &length);
            
            if (length > 0)
                fill_in_file_internal_information(&fai->InternalInformation, fcb, &length);
            
            if (length > 0)
                fill_in_file_ea_information(&fai->EaInformation, fcb, &length);
            
            if (length > 0)
                fill_in_file_access_information(&fai->AccessInformation, &length);
            
            if (length > 0)
                fill_in_file_position_information(&fai->PositionInformation, FileObject, &length);
                
            if (length > 0)
                fill_in_file_mode_information(&fai->ModeInformation, ccb, &length);
            
            if (length > 0)
                fill_in_file_alignment_information(&fai->AlignmentInformation, Vcb, &length);
            
            if (length > 0)
                fill_in_file_name_information(&fai->NameInformation, fcb, fileref, &length);
            
            Status = STATUS_SUCCESS;

            break;
        }

        case FileAttributeTagInformation:
        {
            FILE_ATTRIBUTE_TAG_INFORMATION* ati = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileAttributeTagInformation\n");
            
            if (Irp->RequestorMode != KernelMode && !(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                goto exit;
            }
            
            Status = fill_in_file_attribute_information(ati, fcb, fileref, Irp, &length);
            
            break;
        }

        case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION* fbi = Irp->AssociatedIrp.SystemBuffer;
            INODE_ITEM* ii;
            
            TRACE("FileBasicInformation\n");
            
            if (Irp->RequestorMode != KernelMode && !(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                goto exit;
            }
            
            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_BASIC_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }
            
            if (fcb->ads) {
                if (!fileref || !fileref->parent) {
                    ERR("no fileref for stream\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto exit;
                }
                
                ii = &fileref->parent->fcb->inode_item;
            } else
                ii = &fcb->inode_item;
            
            Status = fill_in_file_basic_information(fbi, ii, &length, fcb, fileref);
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
            
            Status = fill_in_file_ea_information(eai, fcb, &length);
            
            break;
        }

        case FileInternalInformation:
        {
            FILE_INTERNAL_INFORMATION* fii = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileInternalInformation\n");
            
            Status = fill_in_file_internal_information(fii, fcb, &length);
            
            break;
        }

        case FileNameInformation:
        {
            FILE_NAME_INFORMATION* fni = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNameInformation\n");
            
            Status = fill_in_file_name_information(fni, fcb, fileref, &length);
            
            break;
        }

        case FileNetworkOpenInformation:
        {
            FILE_NETWORK_OPEN_INFORMATION* fnoi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNetworkOpenInformation\n");
            
            if (Irp->RequestorMode != KernelMode && !(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                goto exit;
            }
            
            Status = fill_in_file_network_open_information(fnoi, fcb, fileref, &length);

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
            
            Status = fill_in_file_standard_information(fsi, fcb, ccb->fileref, &length);
            
            break;
        }

        case FileStreamInformation:
        {
            FILE_STREAM_INFORMATION* fsi = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileStreamInformation\n");
            
            Status = fill_in_file_stream_information(fsi, fileref, Irp, &length);

            break;
        }

#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileHardLinkInformation:
        {
            FILE_LINKS_INFORMATION* fli = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileHardLinkInformation\n");
            
            Status = fill_in_hard_link_information(fli, fileref, Irp, &length);
            
            break;
        }
            
        case FileNormalizedNameInformation:
        {
            FILE_NAME_INFORMATION* fni = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileNormalizedNameInformation\n");
            
            Status = fill_in_file_name_information(fni, fcb, fileref, &length);
            
            break;
        }
#endif
            
#if (NTDDI_VERSION >= NTDDI_WIN7)
        case FileStandardLinkInformation:
        {
            FILE_STANDARD_LINK_INFORMATION* fsli = Irp->AssociatedIrp.SystemBuffer;
            
            TRACE("FileStandardLinkInformation\n");
            
            Status = fill_in_file_standard_link_information(fsli, fcb, ccb->fileref, &length);
            
            break;
        }
        
        case FileRemoteProtocolInformation:
            TRACE("FileRemoteProtocolInformation\n");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        case FileIdInformation:
        {
            FILE_ID_INFORMATION* fii = Irp->AssociatedIrp.SystemBuffer;
            
            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_ID_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }
            
            TRACE("FileIdInformation\n");
            
            Status = fill_in_file_id_information(fii, fcb, &length);  
            
            break;
        }
#pragma GCC diagnostic pop            
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
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    Irp->IoStatus.Information = 0;
    
    TRACE("query information\n");
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
    fcb = IrpSp->FileObject->FsContext;
    TRACE("fcb = %p\n", fcb);
    TRACE("fcb->subvol = %p\n", fcb->subvol);
    
    Status = query_info(fcb->Vcb, IrpSp->FileObject, Irp);
    
    TRACE("returning %08x\n", Status);
    
    Irp->IoStatus.Status = Status;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    ExReleaseResourceLite(&Vcb->tree_lock);
    
exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    return Status;
}

NTSTATUS STDCALL drv_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    BOOL top_level;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb;
    ccb* ccb;
    FILE_FULL_EA_INFORMATION* ffei;
    ULONG retlen = 0;
    
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    ffei = map_user_buffer(Irp);
    if (!ffei) {
        ERR("could not get output buffer\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (!FileObject) {
        ERR("no file object\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    fcb = FileObject->FsContext;
    
    if (!fcb) {
        ERR("no fcb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    ccb = FileObject->FsContext2;
    
    if (!ccb) {
        ERR("no ccb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_READ_EA | FILE_WRITE_EA))) {
        WARN("insufficient privileges\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    Status = STATUS_SUCCESS;
    
    if (fcb->ea_xattr.Length == 0)
        goto end2;
    
    if (IrpSp->Parameters.QueryEa.EaList) {
        FILE_FULL_EA_INFORMATION *ea, *out;
        FILE_GET_EA_INFORMATION* in;
        
        in = IrpSp->Parameters.QueryEa.EaList;
        do {
            STRING s;
            
            s.Length = s.MaximumLength = in->EaNameLength;
            s.Buffer = in->EaName;
            
            RtlUpperString(&s, &s);
            
            if (in->NextEntryOffset == 0)
                break;
            
            in = (FILE_GET_EA_INFORMATION*)(((UINT8*)in) + in->NextEntryOffset);
        } while (TRUE);
        
        ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;
        out = NULL;
        
        do {
            BOOL found = FALSE;
            
            in = IrpSp->Parameters.QueryEa.EaList;
            do {
                if (in->EaNameLength == ea->EaNameLength &&
                    RtlCompareMemory(in->EaName, ea->EaName, in->EaNameLength) == in->EaNameLength) {
                    found = TRUE;
                    break;
                }
                
                if (in->NextEntryOffset == 0)
                    break;
            
                in = (FILE_GET_EA_INFORMATION*)(((UINT8*)in) + in->NextEntryOffset);
            } while (TRUE);
            
            if (found) {
                UINT8 padding = retlen % 4 > 0 ? (4 - (retlen % 4)) : 0;
                
                if (offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength > IrpSp->Parameters.QueryEa.Length - retlen - padding) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    retlen = 0;
                    goto end2;
                }
                
                retlen += padding;
            
                if (out) {
                    out->NextEntryOffset = offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + out->EaNameLength + 1 + out->EaValueLength + padding;
                    out = (FILE_FULL_EA_INFORMATION*)(((UINT8*)out) + out->NextEntryOffset);
                } else
                    out = ffei;
                    
                out->NextEntryOffset = 0;
                out->Flags = ea->Flags;
                out->EaNameLength = ea->EaNameLength;
                out->EaValueLength = ea->EaValueLength;
                RtlCopyMemory(out->EaName, ea->EaName, ea->EaNameLength + ea->EaValueLength + 1);
                
                retlen += offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength;
                
                if (IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
                    break;
            }
            
            if (ea->NextEntryOffset == 0)
                break;
            
            ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
        } while (TRUE);
    } else {
        FILE_FULL_EA_INFORMATION *ea, *out;
        ULONG index;
        
        if (IrpSp->Flags & SL_INDEX_SPECIFIED) {
            // The index is 1-based
            if (IrpSp->Parameters.QueryEa.EaIndex == 0) {
                Status = STATUS_NONEXISTENT_EA_ENTRY;
                goto end;
            } else
                index = IrpSp->Parameters.QueryEa.EaIndex - 1;
        } else if (IrpSp->Flags & SL_RESTART_SCAN)
            index = ccb->ea_index = 0;
        else
            index = ccb->ea_index;
        
        ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;
        
        if (index > 0) {
            ULONG i;
            
            for (i = 0; i < index; i++) {
                if (ea->NextEntryOffset == 0) // last item
                    goto end2;
                
                ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
            }
        }
        
        out = NULL;
        
        do {
            UINT8 padding = retlen % 4 > 0 ? (4 - (retlen % 4)) : 0;
            
            if (offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength > IrpSp->Parameters.QueryEa.Length - retlen - padding) {
                Status = retlen == 0 ? STATUS_BUFFER_TOO_SMALL : STATUS_BUFFER_OVERFLOW;
                goto end2;
            }
            
            retlen += padding;
        
            if (out) {
                out->NextEntryOffset = offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + out->EaNameLength + 1 + out->EaValueLength + padding;
                out = (FILE_FULL_EA_INFORMATION*)(((UINT8*)out) + out->NextEntryOffset);
            } else
                out = ffei;
                
            out->NextEntryOffset = 0;
            out->Flags = ea->Flags;
            out->EaNameLength = ea->EaNameLength;
            out->EaValueLength = ea->EaValueLength;
            RtlCopyMemory(out->EaName, ea->EaName, ea->EaNameLength + ea->EaValueLength + 1);
            
            retlen += offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength;
            
            if (!(IrpSp->Flags & SL_INDEX_SPECIFIED))
                ccb->ea_index++;
            
            if (ea->NextEntryOffset == 0 || IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
                break;
            
            ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
        } while (TRUE);
    }
    
end2:
    ExReleaseResourceLite(fcb->Header.Resource);
    
end:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW ? retlen : 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

typedef struct {
    ANSI_STRING name;
    ANSI_STRING value;
    UCHAR flags;
    LIST_ENTRY list_entry;
} ea_item;

NTSTATUS STDCALL drv_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOL top_level;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb;
    ccb* ccb;
    FILE_FULL_EA_INFORMATION* ffei;
    ULONG offset;
    LIST_ENTRY ealist;
    ea_item* item;
    FILE_FULL_EA_INFORMATION* ea;
    LIST_ENTRY* le;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }
    
    ffei = map_user_buffer(Irp);
    if (!ffei) {
        ERR("could not get output buffer\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    Status = IoCheckEaBufferValidity(ffei, IrpSp->Parameters.SetEa.Length, &offset);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
        goto end;
    }
    
    if (!FileObject) {
        ERR("no file object\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    fcb = FileObject->FsContext;
    
    if (!fcb) {
        ERR("no fcb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    ccb = FileObject->FsContext2;
    
    if (!ccb) {
        ERR("no ccb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_EA)) {
        WARN("insufficient privileges\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    InitializeListHead(&ealist);
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->ea_xattr.Length > 0) {
        ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;
        
        do {
            item = ExAllocatePoolWithTag(PagedPool, sizeof(ea_item), ALLOC_TAG);
            if (!item) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end2;
            }
            
            item->name.Length = item->name.MaximumLength = ea->EaNameLength;
            item->name.Buffer = ea->EaName;
            
            item->value.Length = item->value.MaximumLength = ea->EaValueLength;
            item->value.Buffer = &ea->EaName[ea->EaNameLength + 1];
            
            item->flags = ea->Flags;
            
            InsertTailList(&ealist, &item->list_entry);
            
            if (ea->NextEntryOffset == 0)
                break;
            
            ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
        } while (TRUE);
    }
    
    ea = ffei;
    
    do {
        STRING s;
        BOOL found = FALSE;
        
        s.Length = s.MaximumLength = ea->EaNameLength;
        s.Buffer = ea->EaName;
        
        RtlUpperString(&s, &s);
        
        le = ealist.Flink;
        while (le != &ealist) {
            item = CONTAINING_RECORD(le, ea_item, list_entry);
            
            if (item->name.Length == s.Length &&
                RtlCompareMemory(item->name.Buffer, s.Buffer, s.Length) == s.Length) {
                item->flags = ea->Flags;
                item->value.Length = item->value.MaximumLength = ea->EaValueLength;
                item->value.Buffer = &ea->EaName[ea->EaNameLength + 1];
                found = TRUE;
                break;
            }
            
            le = le->Flink;
        }
        
        if (!found) {
            item = ExAllocatePoolWithTag(PagedPool, sizeof(ea_item), ALLOC_TAG);
            if (!item) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end2;
            }
            
            item->name.Length = item->name.MaximumLength = ea->EaNameLength;
            item->name.Buffer = ea->EaName;
            
            item->value.Length = item->value.MaximumLength = ea->EaValueLength;
            item->value.Buffer = &ea->EaName[ea->EaNameLength + 1];
            
            item->flags = ea->Flags;
            
            InsertTailList(&ealist, &item->list_entry);
        }
        
        if (ea->NextEntryOffset == 0)
            break;
        
        ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
    } while (TRUE);
    
    // remove entries with zero-length value
    le = ealist.Flink;
    while (le != &ealist) {
        LIST_ENTRY* le2 = le->Flink;
        
        item = CONTAINING_RECORD(le, ea_item, list_entry);
        
        if (item->value.Length == 0) {
            RemoveEntryList(&item->list_entry);
            ExFreePool(item);
        }
        
        le = le2;
    }
    
    if (IsListEmpty(&ealist)) {
        fcb->ealen = 0;
        
        if (fcb->ea_xattr.Buffer)
            ExFreePool(fcb->ea_xattr.Buffer);
        
        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = 0;
        fcb->ea_xattr.Buffer = NULL;
    } else {
        ULONG size = 0;
        char *buf, *oldbuf;
        
        le = ealist.Flink;
        while (le != &ealist) {
            item = CONTAINING_RECORD(le, ea_item, list_entry);
            
            if (size % 4 > 0)
                size += 4 - (size % 4);
            
            size += offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + item->name.Length + 1 + item->value.Length;
            
            le = le->Flink;
        }
        
        buf = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
        if (!buf) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end2;
        }
        
        oldbuf = fcb->ea_xattr.Buffer;
        
        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = size;
        fcb->ea_xattr.Buffer = buf;
        
        fcb->ealen = 4;
        ea = NULL;
        
        le = ealist.Flink;
        while (le != &ealist) {
            item = CONTAINING_RECORD(le, ea_item, list_entry);
            
            if (ea) {
                ea->NextEntryOffset = offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + ea->EaValueLength;
                
                if (ea->NextEntryOffset % 4 > 0)
                    ea->NextEntryOffset += 4 - (ea->NextEntryOffset % 4);
                
                ea = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ea) + ea->NextEntryOffset);
            } else
                ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;
            
            ea->NextEntryOffset = 0;
            ea->Flags = item->flags;
            ea->EaNameLength = item->name.Length;
            ea->EaValueLength = item->value.Length;
            
            RtlCopyMemory(ea->EaName, item->name.Buffer, item->name.Length);
            ea->EaName[item->name.Length] = 0;
            RtlCopyMemory(&ea->EaName[item->name.Length + 1], item->value.Buffer, item->value.Length);
            
            fcb->ealen += 5 + item->name.Length + item->value.Length;
            
            le = le->Flink;
        }
        
        if (oldbuf)
            ExFreePool(oldbuf);
    }
    
    fcb->ea_changed = TRUE;
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;
    
    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;
    
    fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    send_notification_fileref(ccb->fileref, FILE_NOTIFY_CHANGE_EA, FILE_ACTION_MODIFIED);
    
    Status = STATUS_SUCCESS;
    
end2:
    ExReleaseResourceLite(fcb->Header.Resource);
    
    while (!IsListEmpty(&ealist)) {
        le = RemoveHeadList(&ealist);
        
        item = CONTAINING_RECORD(le, ea_item, list_entry);
        
        ExFreePool(item);
    }
    
end:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
