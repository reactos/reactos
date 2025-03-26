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

extern tFsRtlValidateReparsePointBuffer fFsRtlValidateReparsePointBuffer;

typedef struct {
    uint32_t unknown;
    char name[1];
} REPARSE_DATA_BUFFER_LX_SYMLINK;

NTSTATUS get_reparse_point(PFILE_OBJECT FileObject, void* buffer, DWORD buflen, ULONG_PTR* retlen) {
    USHORT subnamelen, printnamelen, i;
    ULONG stringlen;
    DWORD reqlen;
    REPARSE_DATA_BUFFER* rdb = buffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    NTSTATUS Status;

    TRACE("(%p, %p, %lx, %p)\n", FileObject, buffer, buflen, retlen);

    if (!ccb)
        return STATUS_INVALID_PARAMETER;

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);
    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    if (fcb->type == BTRFS_TYPE_SYMLINK) {
        if (ccb->lxss) {
            reqlen = offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + sizeof(uint32_t);

            if (buflen < reqlen) {
                Status = STATUS_BUFFER_OVERFLOW;
                goto end;
            }

            rdb->ReparseTag = IO_REPARSE_TAG_LX_SYMLINK;
            rdb->ReparseDataLength = offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + sizeof(uint32_t);
            rdb->Reserved = 0;

            *((uint32_t*)rdb->GenericReparseBuffer.DataBuffer) = 1;

            *retlen = reqlen;
        } else {
            char* data;

            if (fcb->inode_item.st_size == 0 || fcb->inode_item.st_size > 0xffff) {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            data = ExAllocatePoolWithTag(PagedPool, (ULONG)fcb->inode_item.st_size, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            TRACE("data = %p, size = %I64x\n", data, fcb->inode_item.st_size);
            Status = read_file(fcb, (uint8_t*)data, 0, fcb->inode_item.st_size, NULL, NULL);

            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08lx\n", Status);
                ExFreePool(data);
                goto end;
            }

            Status = utf8_to_utf16(NULL, 0, &stringlen, data, (ULONG)fcb->inode_item.st_size);
            if (!NT_SUCCESS(Status)) {
                ERR("utf8_to_utf16 1 returned %08lx\n", Status);
                ExFreePool(data);
                goto end;
            }

            subnamelen = (uint16_t)stringlen;
            printnamelen = (uint16_t)stringlen;

            reqlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + subnamelen + printnamelen;

            if (buflen >= offsetof(REPARSE_DATA_BUFFER, ReparseDataLength))
                rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;

            if (buflen >= offsetof(REPARSE_DATA_BUFFER, Reserved))
                rdb->ReparseDataLength = (USHORT)(reqlen - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer));

            if (buflen >= offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.SubstituteNameOffset))
                rdb->Reserved = 0;

            if (buflen < reqlen) {
                ExFreePool(data);
                Status = STATUS_BUFFER_OVERFLOW;
                *retlen = min(buflen, offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.SubstituteNameOffset));
                goto end;
            }

            rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
            rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameOffset = subnamelen;
            rdb->SymbolicLinkReparseBuffer.PrintNameLength = printnamelen;
            rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;

            Status = utf8_to_utf16(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                                       stringlen, &stringlen, data, (ULONG)fcb->inode_item.st_size);

            if (!NT_SUCCESS(Status)) {
                ERR("utf8_to_utf16 2 returned %08lx\n", Status);
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

            Status = read_file(fcb, buffer, 0, buflen, &len, NULL);

            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08lx\n", Status);
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

            Status = *retlen == fcb->reparse_xattr.Length ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
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

static NTSTATUS set_symlink(PIRP Irp, file_ref* fileref, fcb* fcb, ccb* ccb, REPARSE_DATA_BUFFER* rdb, ULONG buflen, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    ULONG tlength;
    ANSI_STRING target;
    bool target_alloc = false;
    LARGE_INTEGER offset, time;
    BTRFS_TIME now;

    if (rdb->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
        UNICODE_STRING subname;
        ULONG minlen, len;

        minlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + sizeof(WCHAR);
        if (buflen < minlen) {
            WARN("buffer was less than minimum length (%lu < %lu)\n", buflen, minlen);
            return STATUS_INVALID_PARAMETER;
        }

        if (rdb->SymbolicLinkReparseBuffer.SubstituteNameLength < sizeof(WCHAR)) {
            WARN("rdb->SymbolicLinkReparseBuffer.SubstituteNameLength was too short\n");
            return STATUS_INVALID_PARAMETER;
        }

        subname.Buffer = &rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)];
        subname.MaximumLength = subname.Length = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength;

        TRACE("substitute name = %.*S\n", (int)(subname.Length / sizeof(WCHAR)), subname.Buffer);

        Status = utf16_to_utf8(NULL, 0, &len, subname.Buffer, subname.Length);
        if (!NT_SUCCESS(Status)) {
            ERR("utf16_to_utf8 1 failed with error %08lx\n", Status);
            return Status;
        }

        target.MaximumLength = target.Length = (USHORT)len;
        target.Buffer = ExAllocatePoolWithTag(PagedPool, target.MaximumLength, ALLOC_TAG);
        if (!target.Buffer) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        target_alloc = true;

        Status = utf16_to_utf8(target.Buffer, target.Length, &len, subname.Buffer, subname.Length);
        if (!NT_SUCCESS(Status)) {
            ERR("utf16_to_utf8 2 failed with error %08lx\n", Status);
            ExFreePool(target.Buffer);
            return Status;
        }

        for (USHORT i = 0; i < target.Length; i++) {
            if (target.Buffer[i] == '\\')
                target.Buffer[i] = '/';
        }
    } else if (rdb->ReparseTag == IO_REPARSE_TAG_LX_SYMLINK) {
        REPARSE_DATA_BUFFER_LX_SYMLINK* buf;

        if (buflen < offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + rdb->ReparseDataLength) {
            WARN("buffer was less than expected length (%lu < %lu)\n", buflen,
                 (unsigned long)(offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + rdb->ReparseDataLength));
            return STATUS_INVALID_PARAMETER;
        }

        buf = (REPARSE_DATA_BUFFER_LX_SYMLINK*)rdb->GenericReparseBuffer.DataBuffer;

        if (buflen < offsetof(REPARSE_DATA_BUFFER_LX_SYMLINK, name)) {
            WARN("buffer was less than minimum length (%u < %lu)\n", rdb->ReparseDataLength,
                 (unsigned long)(offsetof(REPARSE_DATA_BUFFER_LX_SYMLINK, name)));
            return STATUS_INVALID_PARAMETER;
        }

        target.Buffer = buf->name;
        target.Length = target.MaximumLength = rdb->ReparseDataLength - offsetof(REPARSE_DATA_BUFFER_LX_SYMLINK, name);
    } else {
        ERR("unexpected reparse tag %08lx\n", rdb->ReparseTag);
        return STATUS_INTERNAL_ERROR;
    }

    fcb->type = BTRFS_TYPE_SYMLINK;
    fcb->inode_item.st_mode &= ~__S_IFMT;
    fcb->inode_item.st_mode |= __S_IFLNK;
    fcb->inode_item.generation = fcb->Vcb->superblock.generation; // so we don't confuse btrfs send on Linux

    if (fileref && fileref->dc)
        fileref->dc->type = fcb->type;

    Status = truncate_file(fcb, 0, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("truncate_file returned %08lx\n", Status);

        if (target_alloc)
            ExFreePool(target.Buffer);

        return Status;
    }

    offset.QuadPart = 0;
    tlength = target.Length;
    Status = write_file2(fcb->Vcb, Irp, offset, target.Buffer, &tlength, false, true,
                            true, false, false, rollback);

    if (target_alloc)
        ExFreePool(target.Buffer);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->inode_item.transid = fcb->Vcb->superblock.generation;
    fcb->inode_item.sequence++;

    if (!ccb || !ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    if (!ccb || !ccb->user_set_write_time)
        fcb->inode_item.st_mtime = now;

    fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    if (fileref)
        mark_fileref_dirty(fileref);

    return Status;
}

NTSTATUS set_reparse_point2(fcb* fcb, REPARSE_DATA_BUFFER* rdb, ULONG buflen, ccb* ccb, file_ref* fileref, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    ULONG tag;

    if (fcb->type == BTRFS_TYPE_SYMLINK) {
        WARN("tried to set a reparse point on an existing symlink\n");
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME - fail if we already have the attribute FILE_ATTRIBUTE_REPARSE_POINT

    // FIXME - die if not file or directory

    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0) {
        TRACE("directory not empty\n");
        return STATUS_DIRECTORY_NOT_EMPTY;
    }

    if (buflen < sizeof(ULONG)) {
        WARN("buffer was not long enough to hold tag\n");
        return STATUS_INVALID_BUFFER_SIZE;
    }

    Status = fFsRtlValidateReparsePointBuffer(buflen, rdb);
    if (!NT_SUCCESS(Status)) {
        ERR("FsRtlValidateReparsePointBuffer returned %08lx\n", Status);
        return Status;
    }

    tag = *(ULONG*)rdb;

    if (tag == IO_REPARSE_TAG_MOUNT_POINT && fcb->type != BTRFS_TYPE_DIRECTORY)
        return STATUS_NOT_A_DIRECTORY;

    if (fcb->type == BTRFS_TYPE_FILE &&
        ((tag == IO_REPARSE_TAG_SYMLINK && rdb->SymbolicLinkReparseBuffer.Flags & SYMLINK_FLAG_RELATIVE) || tag == IO_REPARSE_TAG_LX_SYMLINK)) {
        Status = set_symlink(Irp, fileref, fcb, ccb, rdb, buflen, rollback);
        fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;
    } else {
        LARGE_INTEGER offset, time;
        BTRFS_TIME now;

        if (fcb->type == BTRFS_TYPE_DIRECTORY || fcb->type == BTRFS_TYPE_CHARDEV || fcb->type == BTRFS_TYPE_BLOCKDEV) { // store as xattr
            ANSI_STRING buf;

            buf.Buffer = ExAllocatePoolWithTag(PagedPool, buflen, ALLOC_TAG);
            if (!buf.Buffer) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            buf.Length = buf.MaximumLength = (uint16_t)buflen;

            if (fcb->reparse_xattr.Buffer)
                ExFreePool(fcb->reparse_xattr.Buffer);

            fcb->reparse_xattr = buf;
            RtlCopyMemory(buf.Buffer, rdb, buflen);

            fcb->reparse_xattr_changed = true;

            Status = STATUS_SUCCESS;
        } else { // otherwise, store as file data
            Status = truncate_file(fcb, 0, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("truncate_file returned %08lx\n", Status);
                return Status;
            }

            offset.QuadPart = 0;

            Status = write_file2(fcb->Vcb, Irp, offset, rdb, &buflen, false, true, true, false, false, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("write_file2 returned %08lx\n", Status);
                return Status;
            }
        }

        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;

        if (!ccb || !ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;

        if (!ccb || !ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;

        fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = true;

        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;

        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);
    }

    return STATUS_SUCCESS;
}

NTSTATUS set_reparse_point(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    void* buffer = Irp->AssociatedIrp.SystemBuffer;
    REPARSE_DATA_BUFFER* rdb = buffer;
    DWORD buflen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status = STATUS_SUCCESS;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    LIST_ENTRY rollback;

    TRACE("(%p)\n", Irp);

    InitializeListHead(&rollback);

    if (!FileObject) {
        ERR("FileObject was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    // IFSTest insists on this, for some reason...
    if (Irp->UserBuffer)
        return STATUS_INVALID_PARAMETER;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA))) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    fileref = ccb->fileref;

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fcb->ads) {
        fileref = fileref->parent;
        fcb = fileref->fcb;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    Status = set_reparse_point2(fcb, rdb, buflen, ccb, fileref, Irp, &rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("set_reparse_point2 returned %08lx\n", Status);
        goto end;
    }

    queue_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED, NULL);

end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(fcb->Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return Status;
}

NTSTATUS delete_reparse_point(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    REPARSE_DATA_BUFFER* rdb = Irp->AssociatedIrp.SystemBuffer;
    DWORD buflen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    LIST_ENTRY rollback;

    TRACE("(%p)\n", Irp);

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
        return STATUS_INVALID_PARAMETER;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

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
        fileref->fcb->inode_item.generation = fileref->fcb->Vcb->superblock.generation; // so we don't confuse btrfs send on Linux
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

        fileref->fcb->inode_item_changed = true;
        mark_fcb_dirty(fileref->fcb);

        fileref->fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fileref->fcb->subvol->root_item.ctime = now;
    } else if (fcb->type == BTRFS_TYPE_FILE) {
        LARGE_INTEGER time;
        BTRFS_TIME now;

        // FIXME - do we need to check that the reparse tags match?

        Status = truncate_file(fcb, 0, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("truncate_file returned %08lx\n", Status);
            goto end;
        }

        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = true;

        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;

        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;

        if (!ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;

        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);

        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
    } else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        LARGE_INTEGER time;
        BTRFS_TIME now;

        // FIXME - do we need to check that the reparse tags match?

        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        fcb->atts_changed = true;

        if (fcb->reparse_xattr.Buffer) {
            ExFreePool(fcb->reparse_xattr.Buffer);
            fcb->reparse_xattr.Buffer = NULL;
        }

        fcb->reparse_xattr_changed = true;

        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fcb->inode_item.sequence++;

        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;

        if (!ccb->user_set_write_time)
            fcb->inode_item.st_mtime = now;

        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);

        fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fcb->subvol->root_item.ctime = now;
    } else {
        ERR("unsupported file type %u\n", fcb->type);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_SUCCESS;

    queue_notification_fcb(fileref, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_ACTION_MODIFIED, NULL);

end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(fcb->Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return Status;
}
