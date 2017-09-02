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

NTSTATUS get_reparse_point(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject, void* buffer, DWORD buflen, ULONG_PTR* retlen) {
    USHORT subnamelen, printnamelen, i;
    ULONG stringlen;
    DWORD reqlen;
    REPARSE_DATA_BUFFER* rdb = buffer;
    fcb* fcb = FileObject->FsContext;
    char* data;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p, %x, %p)\n", DeviceObject, FileObject, buffer, buflen, retlen);
    
    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    if (fcb->type == BTRFS_TYPE_SYMLINK) {
        if (called_from_lxss()) {
            reqlen = offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + sizeof(UINT32);
            
            if (buflen < reqlen) {
                Status = STATUS_BUFFER_OVERFLOW;
                goto end;
            }
            
            rdb->ReparseTag = IO_REPARSE_TAG_LXSS_SYMLINK;
            rdb->ReparseDataLength = offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + sizeof(UINT32);
            rdb->Reserved = 0;
            
            *((UINT32*)rdb->GenericReparseBuffer.DataBuffer) = 1;
            
            *retlen = reqlen;
        } else {
            data = ExAllocatePoolWithTag(PagedPool, fcb->inode_item.st_size, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            TRACE("data = %p, size = %x\n", data, fcb->inode_item.st_size);
            Status = read_file(fcb, (UINT8*)data, 0, fcb->inode_item.st_size, NULL, NULL, TRUE);
            
            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, data, fcb->inode_item.st_size);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            subnamelen = stringlen;
            printnamelen = stringlen;
            
            reqlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + subnamelen + printnamelen;
            
            if (buflen < reqlen) {
                Status = STATUS_BUFFER_OVERFLOW;
                goto end;
            }
            
            rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
            rdb->ReparseDataLength = reqlen - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer);
            rdb->Reserved = 0;
            
            rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
            rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameOffset = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameLength = printnamelen;
            rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;
            
            Status = RtlUTF8ToUnicodeN(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                                    stringlen, &stringlen, data, fcb->inode_item.st_size);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            for (i = 0; i < stringlen / sizeof(WCHAR); i++) {
                if (rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] == '/')
                    rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] = '\\';
            }
            
            RtlCopyMemory(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)],
                        &rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength);
            
            *retlen = reqlen;
            
            ExFreePool(data);
        }
        
        Status = STATUS_SUCCESS;
    } else if (fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) {
        if (fcb->type == BTRFS_TYPE_FILE) {
            ULONG len;
            
            Status = read_file(fcb, buffer, 0, buflen, &len, NULL, TRUE);
            
            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08x\n", Status);
            }
            
            *retlen = len;
        } else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
            if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length < sizeof(ULONG)) {
                Status = STATUS_NOT_A_REPARSE_POINT;
                goto end;
            }
            
            if (buflen > 0) {
                *retlen = min(buflen, fcb->reparse_xattr.Length);
                RtlCopyMemory(buffer, fcb->reparse_xattr.Buffer, *retlen);
            } else
                *retlen = 0;
        } else
            Status = STATUS_NOT_A_REPARSE_POINT;
    } else {
        Status = STATUS_NOT_A_REPARSE_POINT;
    }
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return Status;
}

static NTSTATUS set_symlink(PIRP Irp, file_ref* fileref, ccb* ccb, REPARSE_DATA_BUFFER* rdb, ULONG buflen, BOOL write, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    ULONG minlen;
    ULONG tlength;
    UNICODE_STRING subname;
    ANSI_STRING target;
    LARGE_INTEGER offset, time;
    BTRFS_TIME now;
    USHORT i;
    
    if (write) {
        minlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + sizeof(WCHAR);
        if (buflen < minlen) {
            WARN("buffer was less than minimum length (%u < %u)\n", buflen, minlen);
            return STATUS_INVALID_PARAMETER;
        }
        
        subname.Buffer = &rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)];
        subname.MaximumLength = subname.Length = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength;
        
        TRACE("substitute name = %.*S\n", subname.Length / sizeof(WCHAR), subname.Buffer);
    }
    
    fileref->fcb->type = BTRFS_TYPE_SYMLINK;
    
    fileref->fcb->inode_item.st_mode |= __S_IFLNK;
    
    if (fileref->dc)
        fileref->dc->type = fileref->fcb->type;
    
    if (write) {
        Status = truncate_file(fileref->fcb, 0, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("truncate_file returned %08x\n", Status);
            return Status;
        }
        
        Status = RtlUnicodeToUTF8N(NULL, 0, (PULONG)&target.Length, subname.Buffer, subname.Length);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUnicodeToUTF8N 1 failed with error %08x\n", Status);
            return Status;
        }
        
        target.MaximumLength = target.Length;
        target.Buffer = ExAllocatePoolWithTag(PagedPool, target.MaximumLength, ALLOC_TAG);
        if (!target.Buffer) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        Status = RtlUnicodeToUTF8N(target.Buffer, target.Length, (PULONG)&target.Length, subname.Buffer, subname.Length);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUnicodeToUTF8N 2 failed with error %08x\n", Status);
            ExFreePool(target.Buffer);
            return Status;
        }
        
        for (i = 0; i < target.Length; i++) {
            if (target.Buffer[i] == '\\')
                target.Buffer[i] = '/';
        }
        
        offset.QuadPart = 0;
        tlength = target.Length;
        Status = write_file2(fileref->fcb->Vcb, Irp, offset, target.Buffer, &tlength, FALSE, TRUE,
                            TRUE, FALSE, rollback);
        ExFreePool(target.Buffer);
    } else
        Status = STATUS_SUCCESS;
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fileref->fcb->inode_item.transid = fileref->fcb->Vcb->superblock.generation;
    fileref->fcb->inode_item.sequence++;
    
    if (!ccb->user_set_change_time)
        fileref->fcb->inode_item.st_ctime = now;
    
    if (!ccb->user_set_write_time)
        fileref->fcb->inode_item.st_mtime = now;
    
    fileref->fcb->subvol->root_item.ctransid = fileref->fcb->Vcb->superblock.generation;
    fileref->fcb->subvol->root_item.ctime = now;
    
    fileref->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(fileref->fcb);
    
    mark_fileref_dirty(fileref);
    
    return Status;
}

NTSTATUS set_reparse_point(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    void* buffer = Irp->AssociatedIrp.SystemBuffer;
    REPARSE_DATA_BUFFER* rdb = buffer;
    DWORD buflen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status = STATUS_SUCCESS;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    ULONG tag;
    LIST_ENTRY rollback;
    
    TRACE("(%p, %p)\n", DeviceObject, Irp);
    
    InitializeListHead(&rollback);
    
    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;
    
    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    // It isn't documented what permissions FSCTL_SET_REPARSE_POINT needs, but CreateSymbolicLinkW
    // creates a file with FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE.
    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }
    
    fileref = ccb->fileref;
    
    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    TRACE("%S\n", file_desc(FileObject));
    
    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->type == BTRFS_TYPE_SYMLINK) {
        WARN("tried to set a reparse point on an existing symlink\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    // FIXME - fail if we already have the attribute FILE_ATTRIBUTE_REPARSE_POINT
    
    // FIXME - die if not file or directory
    // FIXME - die if ADS
    
    if (buflen < sizeof(ULONG)) {
        WARN("buffer was not long enough to hold tag\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    RtlCopyMemory(&tag, buffer, sizeof(ULONG));
    
    if (fcb->type == BTRFS_TYPE_FILE &&
        ((tag == IO_REPARSE_TAG_SYMLINK && rdb->SymbolicLinkReparseBuffer.Flags & SYMLINK_FLAG_RELATIVE) || tag == IO_REPARSE_TAG_LXSS_SYMLINK)) {
        Status = set_symlink(Irp, fileref, ccb, rdb, buflen, tag == IO_REPARSE_TAG_SYMLINK, &rollback);
        fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;
    } else {
        LARGE_INTEGER offset, time;
        BTRFS_TIME now;
        
        if (fcb->type == BTRFS_TYPE_DIRECTORY) { // for directories, store as xattr
            ANSI_STRING buf;
            
            buf.Buffer = ExAllocatePoolWithTag(PagedPool, buflen, ALLOC_TAG);
            if (!buf.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            buf.Length = buf.MaximumLength = buflen;
            
            if (fcb->reparse_xattr.Buffer)
                ExFreePool(fcb->reparse_xattr.Buffer);
            
            fcb->reparse_xattr = buf;
            RtlCopyMemory(fcb->reparse_xattr.Buffer, buffer, buflen);
            
            fcb->reparse_xattr_changed = TRUE;
            
            Status = STATUS_SUCCESS;
        } else { // otherwise, store as file data
            Status = truncate_file(fcb, 0, Irp, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("truncate_file returned %08x\n", Status);
                goto end;
            }
            
            offset.QuadPart = 0;
            
            Status = write_file2(fcb->Vcb, Irp, offset, buffer, &buflen, FALSE, TRUE, TRUE, FALSE, &rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("write_file2 returned %08x\n", Status);
                goto end;
            }
        }
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        
        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;
        
        if (!ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;
        
        fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = TRUE;
        
        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
        
        fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(fcb);
    }
    
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED);
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(fcb->Vcb, &rollback);
    else
        do_rollback(fcb->Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
    
    return Status;
}

NTSTATUS delete_reparse_point(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    REPARSE_DATA_BUFFER* rdb = Irp->AssociatedIrp.SystemBuffer;
    DWORD buflen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    LIST_ENTRY rollback;
    
    TRACE("(%p, %p)\n", DeviceObject, Irp);
    
    InitializeListHead(&rollback);
    
    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    fcb = FileObject->FsContext;
    
    if (!fcb) {
        ERR("fcb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    ccb = FileObject->FsContext2;
    
    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }
    
    fileref = ccb->fileref;
    
    if (!fileref) {
        ERR("fileref was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    TRACE("%S\n", file_desc(FileObject));
    
    if (buflen < offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer)) {
        ERR("buffer was too short\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (rdb->ReparseDataLength > 0) {
        WARN("rdb->ReparseDataLength was not zero\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb->ads) {
        WARN("tried to delete reparse point on ADS\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb->type == BTRFS_TYPE_SYMLINK) {
        LARGE_INTEGER time;
        BTRFS_TIME now;
        
        if (rdb->ReparseTag != IO_REPARSE_TAG_SYMLINK) {
            WARN("reparse tag was not IO_REPARSE_TAG_SYMLINK\n");
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        fileref->fcb->type = BTRFS_TYPE_FILE;
        fileref->fcb->inode_item.st_mode &= ~__S_IFLNK;
        fileref->fcb->inode_item.st_mode |= __S_IFREG;
        fileref->fcb->inode_item.transid = fileref->fcb->Vcb->superblock.generation;
        fileref->fcb->inode_item.sequence++;
        
        if (!ccb->user_set_change_time)
            fileref->fcb->inode_item.st_ctime = now;
        
        if (!ccb->user_set_write_time)
            fileref->fcb->inode_item.st_mtime = now;
        
        fileref->fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        
        if (fileref->dc)
            fileref->dc->type = fileref->fcb->type;
        
        mark_fileref_dirty(fileref);
        
        fileref->fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(fileref->fcb);

        fileref->fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fileref->fcb->subvol->root_item.ctime = now;
    } else if (fcb->type == BTRFS_TYPE_FILE) {
        LARGE_INTEGER time;
        BTRFS_TIME now;
        
        // FIXME - do we need to check that the reparse tags match?
        
        Status = truncate_file(fcb, 0, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("truncate_file returned %08x\n", Status);
            goto end;
        }
        
        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = TRUE;
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        
        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;
        
        if (!ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;

        fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(fcb);

        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
    } else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        LARGE_INTEGER time;
        BTRFS_TIME now;
        
        // FIXME - do we need to check that the reparse tags match?
        
        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = TRUE;
        
        if (fcb->reparse_xattr.Buffer) {
            ExFreePool(fcb->reparse_xattr.Buffer);
            fcb->reparse_xattr.Buffer = NULL;
        }
        
        fcb->reparse_xattr_changed = TRUE;
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        
        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;
        
        if (!ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;

        fcb->inode_item_changed = TRUE;
        mark_fcb_dirty(fcb);

        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
    } else {
        ERR("unsupported file type %u\n", fcb->type);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    Status = STATUS_SUCCESS;
    
    send_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED);
    
end:
    if (NT_SUCCESS(Status))
        clear_rollback(fcb->Vcb, &rollback);
    else
        do_rollback(fcb->Vcb, &rollback);
    
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
    
    return Status;
}
