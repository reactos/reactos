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

#if (NTDDI_VERSION >= NTDDI_WIN10)
// not currently in mingw - introduced with Windows 10
#ifndef _MSC_VER
#define FileIdInformation (enum _FILE_INFORMATION_CLASS)59
#define FileHardLinkFullIdInformation (enum _FILE_INFORMATION_CLASS)62
#define FileDispositionInformationEx (enum _FILE_INFORMATION_CLASS)64
#define FileRenameInformationEx (enum _FILE_INFORMATION_CLASS)65
#define FileStatInformation (enum _FILE_INFORMATION_CLASS)68
#define FileStatLxInformation (enum _FILE_INFORMATION_CLASS)70
#define FileCaseSensitiveInformation (enum _FILE_INFORMATION_CLASS)71
#define FileLinkInformationEx (enum _FILE_INFORMATION_CLASS)72
#define FileStorageReserveIdInformation (enum _FILE_INFORMATION_CLASS)74

typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, *PFILE_ID_INFORMATION;

typedef struct _FILE_STAT_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
    ULONG ReparseTag;
    ULONG NumberOfLinks;
    ACCESS_MASK EffectiveAccess;
} FILE_STAT_INFORMATION, *PFILE_STAT_INFORMATION;

typedef struct _FILE_STAT_LX_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG         FileAttributes;
    ULONG         ReparseTag;
    ULONG         NumberOfLinks;
    ACCESS_MASK   EffectiveAccess;
    ULONG         LxFlags;
    ULONG         LxUid;
    ULONG         LxGid;
    ULONG         LxMode;
    ULONG         LxDeviceIdMajor;
    ULONG         LxDeviceIdMinor;
} FILE_STAT_LX_INFORMATION, *PFILE_STAT_LX_INFORMATION;

#define LX_FILE_METADATA_HAS_UID        0x01
#define LX_FILE_METADATA_HAS_GID        0x02
#define LX_FILE_METADATA_HAS_MODE       0x04
#define LX_FILE_METADATA_HAS_DEVICE_ID  0x08
#define LX_FILE_CASE_SENSITIVE_DIR      0x10

typedef struct _FILE_RENAME_INFORMATION_EX {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    };
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION_EX, *PFILE_RENAME_INFORMATION_EX;

typedef struct _FILE_DISPOSITION_INFORMATION_EX {
    ULONG Flags;
} FILE_DISPOSITION_INFORMATION_EX, *PFILE_DISPOSITION_INFORMATION_EX;

typedef struct _FILE_LINK_INFORMATION_EX {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    };
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION_EX, *PFILE_LINK_INFORMATION_EX;

typedef struct _FILE_CASE_SENSITIVE_INFORMATION {
    ULONG Flags;
} FILE_CASE_SENSITIVE_INFORMATION, *PFILE_CASE_SENSITIVE_INFORMATION;

typedef struct _FILE_LINK_ENTRY_FULL_ID_INFORMATION {
    ULONG NextEntryOffset;
    FILE_ID_128 ParentFileId;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_ENTRY_FULL_ID_INFORMATION, *PFILE_LINK_ENTRY_FULL_ID_INFORMATION;

typedef struct _FILE_LINKS_FULL_ID_INFORMATION {
    ULONG BytesNeeded;
    ULONG EntriesReturned;
    FILE_LINK_ENTRY_FULL_ID_INFORMATION Entry;
} FILE_LINKS_FULL_ID_INFORMATION, *PFILE_LINKS_FULL_ID_INFORMATION;

#define FILE_RENAME_REPLACE_IF_EXISTS                       0x001
#define FILE_RENAME_POSIX_SEMANTICS                         0x002
#define FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE          0x004
#define FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE    0x008
#define FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE             0x010
#define FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE             0x020
#define FILE_RENAME_IGNORE_READONLY_ATTRIBUTE               0x040
#define FILE_RENAME_FORCE_RESIZE_TARGET_SR                  0x080
#define FILE_RENAME_FORCE_RESIZE_SOURCE_SR                  0x100

#define FILE_DISPOSITION_DELETE                         0x1
#define FILE_DISPOSITION_POSIX_SEMANTICS                0x2
#define FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK      0x4
#define FILE_DISPOSITION_ON_CLOSE                       0x8

#define FILE_LINK_REPLACE_IF_EXISTS                       0x001
#define FILE_LINK_POSIX_SEMANTICS                         0x002
#define FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE    0x008
#define FILE_LINK_NO_INCREASE_AVAILABLE_SPACE             0x010
#define FILE_LINK_NO_DECREASE_AVAILABLE_SPACE             0x020
#define FILE_LINK_IGNORE_READONLY_ATTRIBUTE               0x040
#define FILE_LINK_FORCE_RESIZE_TARGET_SR                  0x080
#define FILE_LINK_FORCE_RESIZE_SOURCE_SR                  0x100

#else

#define FILE_RENAME_INFORMATION_EX FILE_RENAME_INFORMATION
#define FILE_LINK_INFORMATION_EX FILE_LINK_INFORMATION

#endif
#endif

#ifdef __REACTOS__
typedef struct _FILE_RENAME_INFORMATION_EX {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    };
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION_EX, *PFILE_RENAME_INFORMATION_EX;

typedef struct _FILE_DISPOSITION_INFORMATION_EX {
    ULONG Flags;
} FILE_DISPOSITION_INFORMATION_EX, *PFILE_DISPOSITION_INFORMATION_EX;

typedef struct _FILE_LINK_INFORMATION_EX {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    };
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION_EX, *PFILE_LINK_INFORMATION_EX;

#define FILE_RENAME_REPLACE_IF_EXISTS                       0x001
#define FILE_RENAME_POSIX_SEMANTICS                         0x002
#define FILE_RENAME_IGNORE_READONLY_ATTRIBUTE               0x040

#define FILE_DISPOSITION_DELETE                         0x1
#define FILE_DISPOSITION_POSIX_SEMANTICS                0x2
#define FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK      0x4

#define FILE_LINK_REPLACE_IF_EXISTS                       0x001
#define FILE_LINK_POSIX_SEMANTICS                         0x002
#define FILE_LINK_IGNORE_READONLY_ATTRIBUTE               0x040
#endif

static NTSTATUS set_basic_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_BASIC_INFORMATION* fbi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    ULONG defda, filter = 0;
    bool inode_item_changed = false;
    NTSTATUS Status;

    if (fcb->ads) {
        if (fileref && fileref->parent)
            fcb = fileref->parent->fcb;
        else {
            ERR("stream did not have fileref\n");
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    TRACE("file = %p, attributes = %lx\n", FileObject, fbi->FileAttributes);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fbi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY && fcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("attempted to set FILE_ATTRIBUTE_DIRECTORY on non-directory\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (fcb->inode == SUBVOL_ROOT_INODE && is_subvol_readonly(fcb->subvol, Irp) &&
        (fbi->FileAttributes == 0 || fbi->FileAttributes & FILE_ATTRIBUTE_READONLY)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    // don't allow readonly subvol to be made r/w if send operation running on it
    if (fcb->inode == SUBVOL_ROOT_INODE && fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY &&
        fcb->subvol->send_ops > 0) {
        Status = STATUS_DEVICE_NOT_READY;
        goto end;
    }

    // times of -2 are some sort of undocumented behaviour to do with LXSS

    if (fbi->CreationTime.QuadPart == -2)
        fbi->CreationTime.QuadPart = 0;

    if (fbi->LastAccessTime.QuadPart == -2)
        fbi->LastAccessTime.QuadPart = 0;

    if (fbi->LastWriteTime.QuadPart == -2)
        fbi->LastWriteTime.QuadPart = 0;

    if (fbi->ChangeTime.QuadPart == -2)
        fbi->ChangeTime.QuadPart = 0;

    if (fbi->CreationTime.QuadPart == -1)
        ccb->user_set_creation_time = true;
    else if (fbi->CreationTime.QuadPart != 0) {
        win_time_to_unix(fbi->CreationTime, &fcb->inode_item.otime);
        inode_item_changed = true;
        filter |= FILE_NOTIFY_CHANGE_CREATION;

        ccb->user_set_creation_time = true;
    }

    if (fbi->LastAccessTime.QuadPart == -1)
        ccb->user_set_access_time = true;
    else if (fbi->LastAccessTime.QuadPart != 0) {
        win_time_to_unix(fbi->LastAccessTime, &fcb->inode_item.st_atime);
        inode_item_changed = true;
        filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;

        ccb->user_set_access_time = true;
    }

    if (fbi->LastWriteTime.QuadPart == -1)
        ccb->user_set_write_time = true;
    else if (fbi->LastWriteTime.QuadPart != 0) {
        win_time_to_unix(fbi->LastWriteTime, &fcb->inode_item.st_mtime);
        inode_item_changed = true;
        filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;

        ccb->user_set_write_time = true;
    }

    if (fbi->ChangeTime.QuadPart == -1)
        ccb->user_set_change_time = true;
    else if (fbi->ChangeTime.QuadPart != 0) {
        win_time_to_unix(fbi->ChangeTime, &fcb->inode_item.st_ctime);
        inode_item_changed = true;
        // no filter for this

        ccb->user_set_change_time = true;
    }

    // FileAttributes == 0 means don't set - undocumented, but seen in fastfat
    if (fbi->FileAttributes != 0) {
        LARGE_INTEGER time;
        BTRFS_TIME now;

        fbi->FileAttributes &= ~FILE_ATTRIBUTE_NORMAL;

        defda = get_file_attributes(Vcb, fcb->subvol, fcb->inode, fcb->type, fileref && fileref->dc && fileref->dc->name.Length >= sizeof(WCHAR) && fileref->dc->name.Buffer[0] == '.',
                                    true, Irp);

        if (fcb->type == BTRFS_TYPE_DIRECTORY)
            fbi->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        else if (fcb->type == BTRFS_TYPE_SYMLINK)
            fbi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;

        fcb->atts_changed = true;

        if (fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)
            fbi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;

        if (defda == fbi->FileAttributes)
            fcb->atts_deleted = true;
        else if (fcb->inode == SUBVOL_ROOT_INODE && (defda | FILE_ATTRIBUTE_READONLY) == (fbi->FileAttributes | FILE_ATTRIBUTE_READONLY))
            fcb->atts_deleted = true;

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

        inode_item_changed = true;

        filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    }

    if (inode_item_changed) {
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;
        fcb->inode_item_changed = true;

        mark_fcb_dirty(fcb);
    }

    if (filter != 0)
        queue_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

static NTSTATUS set_disposition_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, bool ex) {
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    ULONG atts, flags;
    NTSTATUS Status;

    if (!fileref)
        return STATUS_INVALID_PARAMETER;

    if (ex) {
        FILE_DISPOSITION_INFORMATION_EX* fdi = Irp->AssociatedIrp.SystemBuffer;

        flags = fdi->Flags;
    } else {
        FILE_DISPOSITION_INFORMATION* fdi = Irp->AssociatedIrp.SystemBuffer;

        flags = fdi->DeleteFile ? FILE_DISPOSITION_DELETE : 0;
    }

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    TRACE("changing delete_on_close to %s for fcb %p\n", flags & FILE_DISPOSITION_DELETE ? "true" : "false", fcb);

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

    TRACE("atts = %lx\n", atts);

    if (atts & FILE_ATTRIBUTE_READONLY) {
        TRACE("not allowing readonly file to be deleted\n");
        Status = STATUS_CANNOT_DELETE;
        goto end;
    }

    if (fcb->inode == SUBVOL_ROOT_INODE && fcb->subvol->id == BTRFS_ROOT_FSTREE) {
        WARN("not allowing \\$Root to be deleted\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    // FIXME - can we skip this bit for subvols?
    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0 && (!fileref || fileref->fcb != Vcb->dummy_fcb)) {
        TRACE("directory not empty\n");
        Status = STATUS_DIRECTORY_NOT_EMPTY;
        goto end;
    }

    if (!MmFlushImageSection(&fcb->nonpaged->segment_object, MmFlushForDelete)) {
        if (!ex || flags & FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK) {
            TRACE("trying to delete file which is being mapped as an image\n");
            Status = STATUS_CANNOT_DELETE;
            goto end;
        }
    }

    ccb->fileref->delete_on_close = flags & FILE_DISPOSITION_DELETE;

    FileObject->DeletePending = flags & FILE_DISPOSITION_DELETE;

    if (flags & FILE_DISPOSITION_DELETE && flags & FILE_DISPOSITION_POSIX_SEMANTICS)
        ccb->fileref->posix_delete = true;

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    // send notification that directory is about to be deleted
    if (NT_SUCCESS(Status) && flags & FILE_DISPOSITION_DELETE && fcb->type == BTRFS_TYPE_DIRECTORY) {
        FsRtlNotifyFullChangeDirectory(Vcb->NotifySync, &Vcb->DirNotifyList, FileObject->FsContext,
                                       NULL, false, false, 0, NULL, NULL, NULL);
    }

    return Status;
}

bool has_open_children(file_ref* fileref) {
    LIST_ENTRY* le = fileref->children.Flink;

    if (IsListEmpty(&fileref->children))
        return false;

    while (le != &fileref->children) {
        file_ref* c = CONTAINING_RECORD(le, file_ref, list_entry);

        if (c->open_count > 0)
            return true;

        if (has_open_children(c))
            return true;

        le = le->Flink;
    }

    return false;
}

static NTSTATUS duplicate_fcb(fcb* oldfcb, fcb** pfcb) {
    device_extension* Vcb = oldfcb->Vcb;
    fcb* fcb;
    LIST_ENTRY* le;

    // FIXME - we can skip a lot of this if the inode is about to be deleted

    fcb = create_fcb(Vcb, PagedPool); // FIXME - what if we duplicate the paging file?
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
        fcb->ads = true;
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
    fcb->inode_item_changed = true;

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
            extent* ext2 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);

            if (!ext2) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ext2->offset = ext->offset;
            ext2->datalen = ext->datalen;

            if (ext2->datalen > 0)
                RtlCopyMemory(&ext2->extent_data, &ext->extent_data, ext2->datalen);

            ext2->unique = false;
            ext2->ignore = false;
            ext2->inserted = true;

            if (ext->csum) {
                ULONG len;
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                if (ext->extent_data.compression == BTRFS_COMPRESSION_NONE)
                    len = (ULONG)ed2->num_bytes;
                else
                    len = (ULONG)ed2->size;

                len = (len * sizeof(uint32_t)) >> Vcb->sector_shift;

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

    fcb->prop_compression = oldfcb->prop_compression;

    le = oldfcb->xattrs.Flink;
    while (le != &oldfcb->xattrs) {
        xattr* xa = CONTAINING_RECORD(le, xattr, list_entry);

        if (xa->valuelen > 0) {
            xattr* xa2;

            xa2 = ExAllocatePoolWithTag(PagedPool, offsetof(xattr, data[0]) + xa->namelen + xa->valuelen, ALLOC_TAG);

            if (!xa2) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            xa2->namelen = xa->namelen;
            xa2->valuelen = xa->valuelen;
            xa2->dirty = xa->dirty;
            memcpy(xa2->data, xa->data, xa->namelen + xa->valuelen);

            InsertTailList(&fcb->xattrs, &xa2->list_entry);
        }

        le = le->Flink;
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

static NTSTATUS add_children_to_move_list(device_extension* Vcb, move_entry* me, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY* le;

    ExAcquireResourceSharedLite(&me->fileref->fcb->nonpaged->dir_children_lock, true);

    le = me->fileref->fcb->dir_children_index.Flink;

    while (le != &me->fileref->fcb->dir_children_index) {
        dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_index);
        file_ref* fr;
        move_entry* me2;

        Status = open_fileref_child(Vcb, me->fileref, &dc->name, true, true, dc->index == 0 ? true : false, PagedPool, &fr, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref_child returned %08lx\n", Status);
            ExReleaseResourceLite(&me->fileref->fcb->nonpaged->dir_children_lock);
            return Status;
        }

        me2 = ExAllocatePoolWithTag(PagedPool, sizeof(move_entry), ALLOC_TAG);
        if (!me2) {
            ERR("out of memory\n");
            ExReleaseResourceLite(&me->fileref->fcb->nonpaged->dir_children_lock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        me2->fileref = fr;
        me2->dummyfcb = NULL;
        me2->dummyfileref = NULL;
        me2->parent = me;

        InsertHeadList(&me->list_entry, &me2->list_entry);

        le = le->Flink;
    }

    ExReleaseResourceLite(&me->fileref->fcb->nonpaged->dir_children_lock);

    return STATUS_SUCCESS;
}

void remove_dir_child_from_hash_lists(fcb* fcb, dir_child* dc) {
    uint8_t c;

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

static NTSTATUS create_directory_fcb(device_extension* Vcb, root* r, fcb* parfcb, fcb** pfcb) {
    NTSTATUS Status;
    fcb* fcb;
    SECURITY_SUBJECT_CONTEXT subjcont;
    PSID owner;
    BOOLEAN defaulted;
    LARGE_INTEGER time;
    BTRFS_TIME now;

    fcb = create_fcb(Vcb, PagedPool);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->Vcb = Vcb;

    fcb->subvol = r;
    fcb->inode = InterlockedIncrement64(&r->lastinode);
    fcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&fcb->inode, sizeof(uint64_t));
    fcb->type = BTRFS_TYPE_DIRECTORY;

    fcb->inode_item.generation = Vcb->superblock.generation;
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_nlink = 1;
    fcb->inode_item.st_mode = __S_IFDIR | inherit_mode(parfcb, true);
    fcb->inode_item.st_atime = fcb->inode_item.st_ctime = fcb->inode_item.st_mtime = fcb->inode_item.otime = now;
    fcb->inode_item.st_gid = GID_NOBODY;

    fcb->atts = get_file_attributes(Vcb, fcb->subvol, fcb->inode, fcb->type, false, true, NULL);

    SeCaptureSubjectContext(&subjcont);

    Status = SeAssignSecurity(parfcb->sd, NULL, (void**)&fcb->sd, true, &subjcont, IoGetFileObjectGenericMapping(), PagedPool);

    if (!NT_SUCCESS(Status)) {
        reap_fcb(fcb);
        ERR("SeAssignSecurity returned %08lx\n", Status);
        return Status;
    }

    if (!fcb->sd) {
        reap_fcb(fcb);
        ERR("SeAssignSecurity returned NULL security descriptor\n");
        return STATUS_INTERNAL_ERROR;
    }

    Status = RtlGetOwnerSecurityDescriptor(fcb->sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetOwnerSecurityDescriptor returned %08lx\n", Status);
        fcb->inode_item.st_uid = UID_NOBODY;
        fcb->sd_dirty = true;
    } else {
        fcb->inode_item.st_uid = sid_to_uid(owner);
        fcb->sd_dirty = fcb->inode_item.st_uid == UID_NOBODY;
    }

    find_gid(fcb, parfcb, &subjcont);

    fcb->inode_item_changed = true;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;

    fcb->created = true;

    if (parfcb->inode_item.flags & BTRFS_INODE_COMPRESS)
        fcb->inode_item.flags |= BTRFS_INODE_COMPRESS;

    fcb->prop_compression = parfcb->prop_compression;
    fcb->prop_compression_changed = fcb->prop_compression != PropCompression_None;

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

    acquire_fcb_lock_exclusive(Vcb);
    add_fcb_to_subvol(fcb);
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);
    r->fcbs_version++;
    release_fcb_lock(Vcb);

    mark_fcb_dirty(fcb);

    *pfcb = fcb;

    return STATUS_SUCCESS;
}

void add_fcb_to_subvol(_In_ _Requires_exclusive_lock_held_(_Curr_->Vcb->fcb_lock) fcb* fcb) {
    LIST_ENTRY* lastle = NULL;
    uint32_t hash = fcb->hash;

    if (fcb->subvol->fcbs_ptrs[hash >> 24]) {
        LIST_ENTRY* le = fcb->subvol->fcbs_ptrs[hash >> 24];

        while (le != &fcb->subvol->fcbs) {
            struct _fcb* fcb2 = CONTAINING_RECORD(le, struct _fcb, list_entry);

            if (fcb2->hash > hash) {
                lastle = le->Blink;
                break;
            }

            le = le->Flink;
        }
    }

    if (!lastle) {
        uint8_t c = hash >> 24;

        if (c != 0xff) {
            uint8_t d = c + 1;

            do {
                if (fcb->subvol->fcbs_ptrs[d]) {
                    lastle = fcb->subvol->fcbs_ptrs[d]->Blink;
                    break;
                }

                d++;
            } while (d != 0);
        }
    }

    if (lastle) {
        InsertHeadList(lastle, &fcb->list_entry);

        if (lastle == &fcb->subvol->fcbs || (CONTAINING_RECORD(lastle, struct _fcb, list_entry)->hash >> 24) != (hash >> 24))
            fcb->subvol->fcbs_ptrs[hash >> 24] = &fcb->list_entry;
    } else {
        InsertTailList(&fcb->subvol->fcbs, &fcb->list_entry);

        if (fcb->list_entry.Blink == &fcb->subvol->fcbs || (CONTAINING_RECORD(fcb->list_entry.Blink, struct _fcb, list_entry)->hash >> 24) != (hash >> 24))
            fcb->subvol->fcbs_ptrs[hash >> 24] = &fcb->list_entry;
    }
}

void remove_fcb_from_subvol(_In_ _Requires_exclusive_lock_held_(_Curr_->Vcb->fcb_lock) fcb* fcb) {
    uint8_t c = fcb->hash >> 24;

    if (fcb->subvol->fcbs_ptrs[c] == &fcb->list_entry) {
        if (fcb->list_entry.Flink != &fcb->subvol->fcbs && (CONTAINING_RECORD(fcb->list_entry.Flink, struct _fcb, list_entry)->hash >> 24) == c)
            fcb->subvol->fcbs_ptrs[c] = fcb->list_entry.Flink;
        else
            fcb->subvol->fcbs_ptrs[c] = NULL;
    }

    RemoveEntryList(&fcb->list_entry);
}

static NTSTATUS move_across_subvols(file_ref* fileref, ccb* ccb, file_ref* destdir, PANSI_STRING utf8, PUNICODE_STRING fnus, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY move_list, *le;
    move_entry* me;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    file_ref* origparent;

    // FIXME - make sure me->dummyfileref and me->dummyfcb get freed properly

    InitializeListHead(&move_list);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    acquire_fcb_lock_exclusive(fileref->fcb->Vcb);

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

        ExAcquireResourceSharedLite(me->fileref->fcb->Header.Resource, true);

        if (!me->fileref->fcb->ads && me->fileref->fcb->subvol == origparent->fcb->subvol) {
            Status = add_children_to_move_list(fileref->fcb->Vcb, me, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("add_children_to_move_list returned %08lx\n", Status);
                ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
                goto end;
            }
        }

        ExReleaseResourceLite(me->fileref->fcb->Header.Resource);

        le = le->Flink;
    }

    send_notification_fileref(fileref, fileref->fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED, NULL);

    // loop through list and create new inodes

    le = move_list.Flink;
    while (le != &move_list) {
        me = CONTAINING_RECORD(le, move_entry, list_entry);

        if (me->fileref->fcb->inode != SUBVOL_ROOT_INODE && me->fileref->fcb != fileref->fcb->Vcb->dummy_fcb) {
            if (!me->dummyfcb) {
                ULONG defda;

                ExAcquireResourceExclusiveLite(me->fileref->fcb->Header.Resource, true);

                Status = duplicate_fcb(me->fileref->fcb, &me->dummyfcb);
                if (!NT_SUCCESS(Status)) {
                    ERR("duplicate_fcb returned %08lx\n", Status);
                    ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
                    goto end;
                }

                me->dummyfcb->subvol = me->fileref->fcb->subvol;
                me->dummyfcb->inode = me->fileref->fcb->inode;
                me->dummyfcb->hash = me->fileref->fcb->hash;

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
                    me->fileref->fcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&me->fileref->fcb->inode, sizeof(uint64_t));
                    me->fileref->fcb->inode_item.st_nlink = 1;

                    defda = get_file_attributes(me->fileref->fcb->Vcb, me->fileref->fcb->subvol, me->fileref->fcb->inode,
                                                me->fileref->fcb->type, me->fileref->dc && me->fileref->dc->name.Length >= sizeof(WCHAR) && me->fileref->dc->name.Buffer[0] == '.',
                                                true, Irp);

                    me->fileref->fcb->sd_dirty = !!me->fileref->fcb->sd;
                    me->fileref->fcb->atts_changed = defda != me->fileref->fcb->atts;
                    me->fileref->fcb->extents_changed = !IsListEmpty(&me->fileref->fcb->extents);
                    me->fileref->fcb->reparse_xattr_changed = !!me->fileref->fcb->reparse_xattr.Buffer;
                    me->fileref->fcb->ea_changed = !!me->fileref->fcb->ea_xattr.Buffer;
                    me->fileref->fcb->xattrs_changed = !IsListEmpty(&me->fileref->fcb->xattrs);
                    me->fileref->fcb->inode_item_changed = true;

                    le2 = me->fileref->fcb->xattrs.Flink;
                    while (le2 != &me->fileref->fcb->xattrs) {
                        xattr* xa = CONTAINING_RECORD(le2, xattr, list_entry);

                        xa->dirty = true;

                        le2 = le2->Flink;
                    }

                    if (le == move_list.Flink) { // first entry
                        me->fileref->fcb->inode_item.transid = me->fileref->fcb->Vcb->superblock.generation;
                        me->fileref->fcb->inode_item.sequence++;

                        if (!ccb->user_set_change_time)
                            me->fileref->fcb->inode_item.st_ctime = now;
                    }

                    le2 = me->fileref->fcb->extents.Flink;
                    while (le2 != &me->fileref->fcb->extents) {
                        extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                        if (!ext->ignore && (ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC)) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                            if (ed2->size != 0) {
                                chunk* c = get_chunk_from_address(me->fileref->fcb->Vcb, ed2->address);

                                if (!c) {
                                    ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
                                } else {
                                    Status = update_changed_extent_ref(me->fileref->fcb->Vcb, c, ed2->address, ed2->size, me->fileref->fcb->subvol->id, me->fileref->fcb->inode,
                                                                       ext->offset - ed2->offset, 1, me->fileref->fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);

                                    if (!NT_SUCCESS(Status)) {
                                        ERR("update_changed_extent_ref returned %08lx\n", Status);
                                        ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
                                        goto end;
                                    }
                                }

                            }
                        }

                        le2 = le2->Flink;
                    }

                    add_fcb_to_subvol(me->dummyfcb);
                    remove_fcb_from_subvol(me->fileref->fcb);
                    add_fcb_to_subvol(me->fileref->fcb);
                } else {
                    me->fileref->fcb->subvol = me->parent->fileref->fcb->subvol;
                    me->fileref->fcb->inode = me->parent->fileref->fcb->inode;
                    me->fileref->fcb->hash = me->parent->fileref->fcb->hash;

                    // put stream after parent in FCB list
                    InsertHeadList(&me->parent->fileref->fcb->list_entry, &me->fileref->fcb->list_entry);
                }

                me->fileref->fcb->created = true;

                InsertTailList(&me->fileref->fcb->Vcb->all_fcbs, &me->dummyfcb->list_entry_all);

                while (!IsListEmpty(&me->fileref->fcb->hardlinks)) {
                    hardlink* hl = CONTAINING_RECORD(RemoveHeadList(&me->fileref->fcb->hardlinks), hardlink, list_entry);

                    if (hl->name.Buffer)
                        ExFreePool(hl->name.Buffer);

                    if (hl->utf8.Buffer)
                        ExFreePool(hl->utf8.Buffer);

                    ExFreePool(hl);
                }

                me->fileref->fcb->inode_item_changed = true;
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
                ExAcquireResourceExclusiveLite(me->fileref->fcb->Header.Resource, true);
                me->fileref->fcb->inode_item.st_nlink++;
                me->fileref->fcb->inode_item_changed = true;
                ExReleaseResourceLite(me->fileref->fcb->Header.Resource);
            }
        }

        le = le->Flink;
    }

    fileref->fcb->subvol->root_item.ctransid = fileref->fcb->Vcb->superblock.generation;
    fileref->fcb->subvol->root_item.ctime = now;

    // loop through list and create new filerefs

    le = move_list.Flink;
    while (le != &move_list) {
        hardlink* hl;
        bool name_changed = false;

        me = CONTAINING_RECORD(le, move_entry, list_entry);

        me->dummyfileref = create_fileref(fileref->fcb->Vcb);
        if (!me->dummyfileref) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        if (me->fileref->fcb == me->fileref->fcb->Vcb->dummy_fcb) {
            root* r = me->parent ? me->parent->fileref->fcb->subvol : destdir->fcb->subvol;

            Status = create_directory_fcb(me->fileref->fcb->Vcb, r, me->fileref->parent->fcb, &me->fileref->fcb);
            if (!NT_SUCCESS(Status)) {
                ERR("create_directory_fcb returned %08lx\n", Status);
                goto end;
            }

            me->fileref->dc->key.obj_id = me->fileref->fcb->inode;
            me->fileref->dc->key.obj_type = TYPE_INODE_ITEM;

            me->dummyfileref->fcb = me->fileref->fcb->Vcb->dummy_fcb;
        } else if (me->fileref->fcb->inode == SUBVOL_ROOT_INODE) {
            me->dummyfileref->fcb = me->fileref->fcb;

            me->fileref->fcb->subvol->parent = le == move_list.Flink ? destdir->fcb->subvol->id : me->parent->fileref->fcb->subvol->id;
        } else
            me->dummyfileref->fcb = me->dummyfcb;

        InterlockedIncrement(&me->dummyfileref->fcb->refcount);

        me->dummyfileref->oldutf8 = me->fileref->oldutf8;
        me->dummyfileref->oldindex = me->fileref->dc->index;

        if (le == move_list.Flink && (me->fileref->dc->utf8.Length != utf8->Length || RtlCompareMemory(me->fileref->dc->utf8.Buffer, utf8->Buffer, utf8->Length) != utf8->Length))
            name_changed = true;

        if (!me->dummyfileref->oldutf8.Buffer) {
            me->dummyfileref->oldutf8.Buffer = ExAllocatePoolWithTag(PagedPool, me->fileref->dc->utf8.Length, ALLOC_TAG);
            if (!me->dummyfileref->oldutf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlCopyMemory(me->dummyfileref->oldutf8.Buffer, me->fileref->dc->utf8.Buffer, me->fileref->dc->utf8.Length);

            me->dummyfileref->oldutf8.Length = me->dummyfileref->oldutf8.MaximumLength = me->fileref->dc->utf8.Length;
        }

        me->dummyfileref->delete_on_close = me->fileref->delete_on_close;
        me->dummyfileref->deleted = me->fileref->deleted;

        me->dummyfileref->created = me->fileref->created;
        me->fileref->created = true;

        me->dummyfileref->parent = me->parent ? me->parent->dummyfileref : origparent;
        increase_fileref_refcount(me->dummyfileref->parent);

        ExAcquireResourceExclusiveLite(&me->dummyfileref->parent->fcb->nonpaged->dir_children_lock, true);
        InsertTailList(&me->dummyfileref->parent->children, &me->dummyfileref->list_entry);
        ExReleaseResourceLite(&me->dummyfileref->parent->fcb->nonpaged->dir_children_lock);

        if (me->dummyfileref->fcb->type == BTRFS_TYPE_DIRECTORY)
            me->dummyfileref->fcb->fileref = me->dummyfileref;

        if (!me->parent) {
            RemoveEntryList(&me->fileref->list_entry);

            increase_fileref_refcount(destdir);

            if (me->fileref->dc) {
                // remove from old parent
                ExAcquireResourceExclusiveLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock, true);
                RemoveEntryList(&me->fileref->dc->list_entry_index);
                remove_dir_child_from_hash_lists(me->fileref->parent->fcb, me->fileref->dc);
                ExReleaseResourceLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock);

                me->fileref->parent->fcb->inode_item.st_size -= me->fileref->dc->utf8.Length * 2;
                me->fileref->parent->fcb->inode_item.transid = me->fileref->fcb->Vcb->superblock.generation;
                me->fileref->parent->fcb->inode_item.sequence++;
                me->fileref->parent->fcb->inode_item.st_ctime = now;
                me->fileref->parent->fcb->inode_item.st_mtime = now;
                me->fileref->parent->fcb->inode_item_changed = true;
                mark_fcb_dirty(me->fileref->parent->fcb);

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

                    me->fileref->dc->utf8.Length = me->fileref->dc->utf8.MaximumLength = utf8->Length;
                    RtlCopyMemory(me->fileref->dc->utf8.Buffer, utf8->Buffer, utf8->Length);

                    me->fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, fnus->Length, ALLOC_TAG);
                    if (!me->fileref->dc->name.Buffer) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }

                    me->fileref->dc->name.Length = me->fileref->dc->name.MaximumLength = fnus->Length;
                    RtlCopyMemory(me->fileref->dc->name.Buffer, fnus->Buffer, fnus->Length);

                    Status = RtlUpcaseUnicodeString(&fileref->dc->name_uc, &fileref->dc->name, true);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
                        goto end;
                    }

                    me->fileref->dc->hash = calc_crc32c(0xffffffff, (uint8_t*)me->fileref->dc->name.Buffer, me->fileref->dc->name.Length);
                    me->fileref->dc->hash_uc = calc_crc32c(0xffffffff, (uint8_t*)me->fileref->dc->name_uc.Buffer, me->fileref->dc->name_uc.Length);
                }

                if (me->fileref->dc->key.obj_type == TYPE_INODE_ITEM)
                    me->fileref->dc->key.obj_id = me->fileref->fcb->inode;

                // add to new parent

                ExAcquireResourceExclusiveLite(&destdir->fcb->nonpaged->dir_children_lock, true);

                if (IsListEmpty(&destdir->fcb->dir_children_index))
                    me->fileref->dc->index = 2;
                else {
                    dir_child* dc2 = CONTAINING_RECORD(destdir->fcb->dir_children_index.Blink, dir_child, list_entry_index);

                    me->fileref->dc->index = max(2, dc2->index + 1);
                }

                InsertTailList(&destdir->fcb->dir_children_index, &me->fileref->dc->list_entry_index);
                insert_dir_child_into_hash_lists(destdir->fcb, me->fileref->dc);
                ExReleaseResourceLite(&destdir->fcb->nonpaged->dir_children_lock);
            }

            free_fileref(me->fileref->parent);
            me->fileref->parent = destdir;

            ExAcquireResourceExclusiveLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock, true);
            InsertTailList(&me->fileref->parent->children, &me->fileref->list_entry);
            ExReleaseResourceLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock);

            TRACE("me->fileref->parent->fcb->inode_item.st_size (inode %I64x) was %I64x\n", me->fileref->parent->fcb->inode, me->fileref->parent->fcb->inode_item.st_size);
            me->fileref->parent->fcb->inode_item.st_size += me->fileref->dc->utf8.Length * 2;
            TRACE("me->fileref->parent->fcb->inode_item.st_size (inode %I64x) now %I64x\n", me->fileref->parent->fcb->inode, me->fileref->parent->fcb->inode_item.st_size);
            me->fileref->parent->fcb->inode_item.transid = me->fileref->fcb->Vcb->superblock.generation;
            me->fileref->parent->fcb->inode_item.sequence++;
            me->fileref->parent->fcb->inode_item.st_ctime = now;
            me->fileref->parent->fcb->inode_item.st_mtime = now;
            me->fileref->parent->fcb->inode_item_changed = true;
            mark_fcb_dirty(me->fileref->parent->fcb);
        } else {
            if (me->fileref->dc) {
                ExAcquireResourceExclusiveLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock, true);
                RemoveEntryList(&me->fileref->dc->list_entry_index);

                if (!me->fileref->fcb->ads)
                    remove_dir_child_from_hash_lists(me->fileref->parent->fcb, me->fileref->dc);

                ExReleaseResourceLite(&me->fileref->parent->fcb->nonpaged->dir_children_lock);

                ExAcquireResourceExclusiveLite(&me->parent->fileref->fcb->nonpaged->dir_children_lock, true);

                if (me->fileref->fcb->ads)
                    InsertHeadList(&me->parent->fileref->fcb->dir_children_index, &me->fileref->dc->list_entry_index);
                else {
                    if (me->fileref->fcb->inode != SUBVOL_ROOT_INODE)
                        me->fileref->dc->key.obj_id = me->fileref->fcb->inode;

                    if (IsListEmpty(&me->parent->fileref->fcb->dir_children_index))
                        me->fileref->dc->index = 2;
                    else {
                        dir_child* dc2 = CONTAINING_RECORD(me->parent->fileref->fcb->dir_children_index.Blink, dir_child, list_entry_index);

                        me->fileref->dc->index = max(2, dc2->index + 1);
                    }

                    InsertTailList(&me->parent->fileref->fcb->dir_children_index, &me->fileref->dc->list_entry_index);
                    insert_dir_child_into_hash_lists(me->parent->fileref->fcb, me->fileref->dc);
                }

                ExReleaseResourceLite(&me->parent->fileref->fcb->nonpaged->dir_children_lock);
            }
        }

        if (!me->dummyfileref->fcb->ads) {
            Status = delete_fileref(me->dummyfileref, NULL, false, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_fileref returned %08lx\n", Status);
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
            hl->index = me->fileref->dc->index;

            hl->utf8.Length = hl->utf8.MaximumLength = me->fileref->dc->utf8.Length;
            hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);
            if (!hl->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExFreePool(hl);
                goto end;
            }

            RtlCopyMemory(hl->utf8.Buffer, me->fileref->dc->utf8.Buffer, me->fileref->dc->utf8.Length);

            hl->name.Length = hl->name.MaximumLength = me->fileref->dc->name.Length;
            hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);
            if (!hl->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExFreePool(hl->utf8.Buffer);
                ExFreePool(hl);
                goto end;
            }

            RtlCopyMemory(hl->name.Buffer, me->fileref->dc->name.Buffer, me->fileref->dc->name.Length);

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
            Status = delete_fileref(me->dummyfileref, NULL, false, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_fileref returned %08lx\n", Status);
                goto end;
            }
        }

        le = le->Flink;
    }

    destdir->fcb->subvol->root_item.ctransid = destdir->fcb->Vcb->superblock.generation;
    destdir->fcb->subvol->root_item.ctime = now;

    me = CONTAINING_RECORD(move_list.Flink, move_entry, list_entry);
    send_notification_fileref(fileref, fileref->fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);
    send_notification_fileref(me->dummyfileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
    send_notification_fileref(fileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

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

    destdir->fcb->subvol->fcbs_version++;
    fileref->fcb->subvol->fcbs_version++;

    release_fcb_lock(fileref->fcb->Vcb);

    return Status;
}

void insert_dir_child_into_hash_lists(fcb* fcb, dir_child* dc) {
    bool inserted;
    LIST_ENTRY* le;
    uint8_t c, d;

    c = dc->hash >> 24;

    inserted = false;

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
            inserted = true;
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

    inserted = false;

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
            inserted = true;
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

static NTSTATUS rename_stream_to_file(device_extension* Vcb, file_ref* fileref, ccb* ccb, ULONG flags,
                                      PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    file_ref* ofr;
    ANSI_STRING adsdata;
    dir_child* dc;
    fcb* dummyfcb;

    if (fileref->fcb->type != BTRFS_TYPE_FILE)
        return STATUS_INVALID_PARAMETER;

    if (!(flags & FILE_RENAME_IGNORE_READONLY_ATTRIBUTE) && fileref->parent->fcb->atts & FILE_ATTRIBUTE_READONLY) {
        WARN("trying to rename stream on readonly file\n");
        return STATUS_ACCESS_DENIED;
    }

    if (Irp->RequestorMode == UserMode && ccb && !(ccb->access & DELETE)) {
        WARN("insufficient permissions\n");
        return STATUS_ACCESS_DENIED;
    }

    if (!(flags & FILE_RENAME_REPLACE_IF_EXISTS)) // file will always exist
        return STATUS_OBJECT_NAME_COLLISION;

    // FIXME - POSIX overwrites of stream?

    ofr = fileref->parent;

    if (ofr->open_count > 0) {
        WARN("trying to overwrite open file\n");
        return STATUS_ACCESS_DENIED;
    }

    if (ofr->fcb->inode_item.st_size > 0) {
        WARN("can only overwrite existing stream if it is zero-length\n");
        return STATUS_INVALID_PARAMETER;
    }

    dummyfcb = create_fcb(Vcb, PagedPool);
    if (!dummyfcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy parent fcb onto this one

    fileref->fcb->subvol = ofr->fcb->subvol;
    fileref->fcb->inode = ofr->fcb->inode;
    fileref->fcb->hash = ofr->fcb->hash;
    fileref->fcb->type = ofr->fcb->type;
    fileref->fcb->inode_item = ofr->fcb->inode_item;

    fileref->fcb->sd = ofr->fcb->sd;
    ofr->fcb->sd = NULL;

    fileref->fcb->deleted = ofr->fcb->deleted;
    fileref->fcb->atts = ofr->fcb->atts;

    fileref->fcb->reparse_xattr = ofr->fcb->reparse_xattr;
    ofr->fcb->reparse_xattr.Buffer = NULL;
    ofr->fcb->reparse_xattr.Length = ofr->fcb->reparse_xattr.MaximumLength = 0;

    fileref->fcb->ea_xattr = ofr->fcb->ea_xattr;
    ofr->fcb->ea_xattr.Buffer = NULL;
    ofr->fcb->ea_xattr.Length = ofr->fcb->ea_xattr.MaximumLength = 0;

    fileref->fcb->ealen = ofr->fcb->ealen;

    while (!IsListEmpty(&ofr->fcb->hardlinks)) {
        InsertTailList(&fileref->fcb->hardlinks, RemoveHeadList(&ofr->fcb->hardlinks));
    }

    fileref->fcb->inode_item_changed = true;
    fileref->fcb->prop_compression = ofr->fcb->prop_compression;

    while (!IsListEmpty(&ofr->fcb->xattrs)) {
        InsertTailList(&fileref->fcb->xattrs, RemoveHeadList(&ofr->fcb->xattrs));
    }

    fileref->fcb->marked_as_orphan = ofr->fcb->marked_as_orphan;
    fileref->fcb->case_sensitive = ofr->fcb->case_sensitive;
    fileref->fcb->case_sensitive_set = ofr->fcb->case_sensitive_set;

    while (!IsListEmpty(&ofr->fcb->dir_children_index)) {
        InsertTailList(&fileref->fcb->dir_children_index, RemoveHeadList(&ofr->fcb->dir_children_index));
    }

    while (!IsListEmpty(&ofr->fcb->dir_children_hash)) {
        InsertTailList(&fileref->fcb->dir_children_hash, RemoveHeadList(&ofr->fcb->dir_children_hash));
    }

    while (!IsListEmpty(&ofr->fcb->dir_children_hash_uc)) {
        InsertTailList(&fileref->fcb->dir_children_hash_uc, RemoveHeadList(&ofr->fcb->dir_children_hash_uc));
    }

    fileref->fcb->hash_ptrs = ofr->fcb->hash_ptrs;
    fileref->fcb->hash_ptrs_uc = ofr->fcb->hash_ptrs_uc;

    ofr->fcb->hash_ptrs = NULL;
    ofr->fcb->hash_ptrs_uc = NULL;

    fileref->fcb->sd_dirty = ofr->fcb->sd_dirty;
    fileref->fcb->sd_deleted = ofr->fcb->sd_deleted;
    fileref->fcb->atts_changed = ofr->fcb->atts_changed;
    fileref->fcb->atts_deleted = ofr->fcb->atts_deleted;
    fileref->fcb->extents_changed = true;
    fileref->fcb->reparse_xattr_changed = ofr->fcb->reparse_xattr_changed;
    fileref->fcb->ea_changed = ofr->fcb->ea_changed;
    fileref->fcb->prop_compression_changed = ofr->fcb->prop_compression_changed;
    fileref->fcb->xattrs_changed = ofr->fcb->xattrs_changed;
    fileref->fcb->created = ofr->fcb->created;
    fileref->fcb->ads = false;

    if (fileref->fcb->adsxattr.Buffer) {
        ExFreePool(fileref->fcb->adsxattr.Buffer);
        fileref->fcb->adsxattr.Length = fileref->fcb->adsxattr.MaximumLength = 0;
        fileref->fcb->adsxattr.Buffer = NULL;
    }

    adsdata = fileref->fcb->adsdata;

    fileref->fcb->adsdata.Buffer = NULL;
    fileref->fcb->adsdata.Length = fileref->fcb->adsdata.MaximumLength = 0;

    acquire_fcb_lock_exclusive(Vcb);

    RemoveEntryList(&fileref->fcb->list_entry);
    InsertHeadList(ofr->fcb->list_entry.Blink, &fileref->fcb->list_entry);

    if (fileref->fcb->subvol->fcbs_ptrs[fileref->fcb->hash >> 24] == &ofr->fcb->list_entry)
        fileref->fcb->subvol->fcbs_ptrs[fileref->fcb->hash >> 24] = &fileref->fcb->list_entry;

    RemoveEntryList(&ofr->fcb->list_entry);

    release_fcb_lock(Vcb);

    ofr->fcb->list_entry.Flink = ofr->fcb->list_entry.Blink = NULL;

    mark_fcb_dirty(fileref->fcb);

    // mark old parent fcb so it gets ignored by flush_fcb
    ofr->fcb->created = true;
    ofr->fcb->deleted = true;

    mark_fcb_dirty(ofr->fcb);

    // copy parent fileref onto this one

    fileref->oldutf8 = ofr->oldutf8;
    ofr->oldutf8.Buffer = NULL;
    ofr->oldutf8.Length = ofr->oldutf8.MaximumLength = 0;

    fileref->oldindex = ofr->oldindex;
    fileref->delete_on_close = ofr->delete_on_close;
    fileref->posix_delete = ofr->posix_delete;
    fileref->deleted = ofr->deleted;
    fileref->created = ofr->created;

    fileref->parent = ofr->parent;

    RemoveEntryList(&fileref->list_entry);
    InsertHeadList(ofr->list_entry.Blink, &fileref->list_entry);
    RemoveEntryList(&ofr->list_entry);
    ofr->list_entry.Flink = ofr->list_entry.Blink = NULL;

    while (!IsListEmpty(&ofr->children)) {
        file_ref* fr = CONTAINING_RECORD(RemoveHeadList(&ofr->children), file_ref, list_entry);

        free_fileref(fr->parent);

        fr->parent = fileref;
        InterlockedIncrement(&fileref->refcount);

        InsertTailList(&fileref->children, &fr->list_entry);
    }

    dc = fileref->dc;

    fileref->dc = ofr->dc;
    fileref->dc->fileref = fileref;

    mark_fileref_dirty(fileref);

    // mark old parent fileref so it gets ignored by flush_fileref
    ofr->created = true;
    ofr->deleted = true;

    // write file data

    fileref->fcb->inode_item.st_size = adsdata.Length;

    if (adsdata.Length > 0) {
        bool make_inline = adsdata.Length <= Vcb->options.max_inline;

        if (make_inline) {
            EXTENT_DATA* ed = ExAllocatePoolWithTag(PagedPool, (uint16_t)(offsetof(EXTENT_DATA, data[0]) + adsdata.Length), ALLOC_TAG);
            if (!ed) {
                ERR("out of memory\n");
                ExFreePool(adsdata.Buffer);
                reap_fcb(dummyfcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ed->generation = Vcb->superblock.generation;
            ed->decoded_size = adsdata.Length;
            ed->compression = BTRFS_COMPRESSION_NONE;
            ed->encryption = BTRFS_ENCRYPTION_NONE;
            ed->encoding = BTRFS_ENCODING_NONE;
            ed->type = EXTENT_TYPE_INLINE;

            RtlCopyMemory(ed->data, adsdata.Buffer, adsdata.Length);

            ExFreePool(adsdata.Buffer);

            Status = add_extent_to_fcb(fileref->fcb, 0, ed, (uint16_t)(offsetof(EXTENT_DATA, data[0]) + adsdata.Length), false, NULL, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_extent_to_fcb returned %08lx\n", Status);
                ExFreePool(ed);
                reap_fcb(dummyfcb);
                return Status;
            }

            ExFreePool(ed);
        } else if (adsdata.Length & (Vcb->superblock.sector_size - 1)) {
            char* newbuf = ExAllocatePoolWithTag(PagedPool, (uint16_t)sector_align(adsdata.Length, Vcb->superblock.sector_size), ALLOC_TAG);
            if (!newbuf) {
                ERR("out of memory\n");
                ExFreePool(adsdata.Buffer);
                reap_fcb(dummyfcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(newbuf, adsdata.Buffer, adsdata.Length);
            RtlZeroMemory(newbuf + adsdata.Length, (uint16_t)(sector_align(adsdata.Length, Vcb->superblock.sector_size) - adsdata.Length));

            ExFreePool(adsdata.Buffer);

            adsdata.Buffer = newbuf;
            adsdata.Length = adsdata.MaximumLength = (uint16_t)sector_align(adsdata.Length, Vcb->superblock.sector_size);
        }

        if (!make_inline) {
            Status = do_write_file(fileref->fcb, 0, adsdata.Length, adsdata.Buffer, Irp, false, 0, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("do_write_file returned %08lx\n", Status);
                ExFreePool(adsdata.Buffer);
                reap_fcb(dummyfcb);
                return Status;
            }

            ExFreePool(adsdata.Buffer);
        }

        fileref->fcb->inode_item.st_blocks = adsdata.Length;
        fileref->fcb->inode_item_changed = true;
    }

    RemoveEntryList(&dc->list_entry_index);

    if (dc->utf8.Buffer)
        ExFreePool(dc->utf8.Buffer);

    if (dc->name.Buffer)
        ExFreePool(dc->name.Buffer);

    if (dc->name_uc.Buffer)
        ExFreePool(dc->name_uc.Buffer);

    ExFreePool(dc);

    // FIXME - csums?

    // add dummy deleted xattr with old name

    dummyfcb->Vcb = Vcb;
    dummyfcb->subvol = fileref->fcb->subvol;
    dummyfcb->inode = fileref->fcb->inode;
    dummyfcb->hash = fileref->fcb->hash;
    dummyfcb->adsxattr = fileref->fcb->adsxattr;
    dummyfcb->adshash = fileref->fcb->adshash;
    dummyfcb->ads = true;
    dummyfcb->deleted = true;

    acquire_fcb_lock_exclusive(Vcb);
    add_fcb_to_subvol(dummyfcb);
    InsertTailList(&Vcb->all_fcbs, &dummyfcb->list_entry_all);
    dummyfcb->subvol->fcbs_version++;
    release_fcb_lock(Vcb);

    // FIXME - dummyfileref as well?

    mark_fcb_dirty(dummyfcb);

    free_fcb(dummyfcb);

    return STATUS_SUCCESS;
}

static NTSTATUS rename_stream(device_extension* Vcb, file_ref* fileref, ccb* ccb, FILE_RENAME_INFORMATION_EX* fri,
                              ULONG flags, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UNICODE_STRING fn;
    file_ref* sf = NULL;
    uint16_t newmaxlen;
    ULONG utf8len;
    ANSI_STRING utf8;
    UNICODE_STRING utf16, utf16uc;
    ANSI_STRING adsxattr;
    uint32_t crc32;
    fcb* dummyfcb;

    static const WCHAR datasuf[] = L":$DATA";
    static const char xapref[] = "user.";

    if (!fileref) {
        ERR("fileref not set\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!fileref->parent) {
        ERR("fileref->parent not set\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fri->FileNameLength < sizeof(WCHAR)) {
        WARN("filename too short\n");
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (fri->FileName[0] != ':') {
        WARN("destination filename must begin with a colon\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && ccb && !(ccb->access & DELETE)) {
        WARN("insufficient permissions\n");
        return STATUS_ACCESS_DENIED;
    }

    fn.Buffer = &fri->FileName[1];
    fn.Length = fn.MaximumLength = (USHORT)(fri->FileNameLength - sizeof(WCHAR));

    // remove :$DATA suffix
    if (fn.Length >= sizeof(datasuf) - sizeof(WCHAR) &&
        RtlCompareMemory(&fn.Buffer[(fn.Length - sizeof(datasuf) + sizeof(WCHAR))/sizeof(WCHAR)], datasuf, sizeof(datasuf) - sizeof(WCHAR)) == sizeof(datasuf) - sizeof(WCHAR))
        fn.Length -= sizeof(datasuf) - sizeof(WCHAR);

    if (fn.Length == 0)
        return rename_stream_to_file(Vcb, fileref, ccb, flags, Irp, rollback);

    Status = check_file_name_valid(&fn, false, true);
    if (!NT_SUCCESS(Status)) {
        WARN("invalid stream name %.*S\n", (int)(fn.Length / sizeof(WCHAR)), fn.Buffer);
        return Status;
    }

    if (!(flags & FILE_RENAME_IGNORE_READONLY_ATTRIBUTE) && fileref->parent->fcb->atts & FILE_ATTRIBUTE_READONLY) {
        WARN("trying to rename stream on readonly file\n");
        return STATUS_ACCESS_DENIED;
    }

    Status = open_fileref_child(Vcb, fileref->parent, &fn, fileref->parent->fcb->case_sensitive, true, true, PagedPool, &sf, Irp);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        if (Status == STATUS_SUCCESS) {
            if (fileref == sf || sf->deleted) {
                free_fileref(sf);
                sf = NULL;
            } else {
                if (!(flags & FILE_RENAME_REPLACE_IF_EXISTS)) {
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto end;
                }

                // FIXME - POSIX overwrites of stream?

                if (sf->open_count > 0) {
                    WARN("trying to overwrite open file\n");
                    Status = STATUS_ACCESS_DENIED;
                    goto end;
                }

                if (sf->fcb->adsdata.Length > 0) {
                    WARN("can only overwrite existing stream if it is zero-length\n");
                    Status = STATUS_INVALID_PARAMETER;
                    goto end;
                }

                Status = delete_fileref(sf, NULL, false, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_fileref returned %08lx\n", Status);
                    goto end;
                }
            }
        } else {
            ERR("open_fileref_child returned %08lx\n", Status);
            goto end;
        }
    }

    Status = utf16_to_utf8(NULL, 0, &utf8len, fn.Buffer, fn.Length);
    if (!NT_SUCCESS(Status))
        goto end;

    utf8.MaximumLength = utf8.Length = (uint16_t)utf8len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    Status = utf16_to_utf8(utf8.Buffer, utf8len, &utf8len, fn.Buffer, fn.Length);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(utf8.Buffer);
        goto end;
    }

    adsxattr.Length = adsxattr.MaximumLength = sizeof(xapref) - 1 + utf8.Length;
    adsxattr.Buffer = ExAllocatePoolWithTag(PagedPool, adsxattr.MaximumLength, ALLOC_TAG);
    if (!adsxattr.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        goto end;
    }

    RtlCopyMemory(adsxattr.Buffer, xapref, sizeof(xapref) - 1);
    RtlCopyMemory(&adsxattr.Buffer[sizeof(xapref) - 1], utf8.Buffer, utf8.Length);

    // don't allow if it's one of our reserved names

    if ((adsxattr.Length == sizeof(EA_DOSATTRIB) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_DOSATTRIB, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_EA) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_EA, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_REPARSE) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_REPARSE, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_CASE_SENSITIVE) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_CASE_SENSITIVE, adsxattr.Length) == adsxattr.Length)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        ExFreePool(utf8.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    utf16.Length = utf16.MaximumLength = fn.Length;
    utf16.Buffer = ExAllocatePoolWithTag(PagedPool, utf16.MaximumLength, ALLOC_TAG);
    if (!utf16.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    RtlCopyMemory(utf16.Buffer, fn.Buffer, fn.Length);

    newmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) -
                offsetof(DIR_ITEM, name[0]);

    if (newmaxlen < adsxattr.Length) {
        WARN("cannot rename as data too long\n");
        Status = STATUS_INVALID_PARAMETER;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    newmaxlen -= adsxattr.Length;

    if (newmaxlen < fileref->fcb->adsdata.Length) {
        WARN("cannot rename as data too long\n");
        Status = STATUS_INVALID_PARAMETER;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    Status = RtlUpcaseUnicodeString(&utf16uc, &fn, true);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    // add dummy deleted xattr with old name

    dummyfcb = create_fcb(Vcb, PagedPool);
    if (!dummyfcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(utf16uc.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    dummyfcb->Vcb = Vcb;
    dummyfcb->subvol = fileref->fcb->subvol;
    dummyfcb->inode = fileref->fcb->inode;
    dummyfcb->hash = fileref->fcb->hash;
    dummyfcb->adsxattr = fileref->fcb->adsxattr;
    dummyfcb->adshash = fileref->fcb->adshash;
    dummyfcb->ads = true;
    dummyfcb->deleted = true;

    acquire_fcb_lock_exclusive(Vcb);
    add_fcb_to_subvol(dummyfcb);
    InsertTailList(&Vcb->all_fcbs, &dummyfcb->list_entry_all);
    dummyfcb->subvol->fcbs_version++;
    release_fcb_lock(Vcb);

    mark_fcb_dirty(dummyfcb);

    free_fcb(dummyfcb);

    // change fcb values

    fileref->dc->utf8 = utf8;
    fileref->dc->name = utf16;
    fileref->dc->name_uc = utf16uc;

    crc32 = calc_crc32c(0xfffffffe, (uint8_t*)adsxattr.Buffer, adsxattr.Length);

    fileref->fcb->adsxattr = adsxattr;
    fileref->fcb->adshash = crc32;
    fileref->fcb->adsmaxlen = newmaxlen;

    fileref->fcb->created = true;

    mark_fcb_dirty(fileref->fcb);

    Status = STATUS_SUCCESS;

end:
    if (sf)
        free_fileref(sf);

    return Status;
}

static NTSTATUS rename_file_to_stream(device_extension* Vcb, file_ref* fileref, ccb* ccb, FILE_RENAME_INFORMATION_EX* fri,
                                      ULONG flags, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UNICODE_STRING fn;
    file_ref* sf = NULL;
    uint16_t newmaxlen;
    ULONG utf8len;
    ANSI_STRING utf8;
    UNICODE_STRING utf16, utf16uc;
    ANSI_STRING adsxattr, adsdata;
    uint32_t crc32;
    fcb* dummyfcb;
    file_ref* dummyfileref;
    dir_child* dc;
    LIST_ENTRY* le;

    static const WCHAR datasuf[] = L":$DATA";
    static const char xapref[] = "user.";

    if (!fileref) {
        ERR("fileref not set\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fri->FileNameLength < sizeof(WCHAR)) {
        WARN("filename too short\n");
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (fri->FileName[0] != ':') {
        WARN("destination filename must begin with a colon\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && ccb && !(ccb->access & DELETE)) {
        WARN("insufficient permissions\n");
        return STATUS_ACCESS_DENIED;
    }

    if (fileref->fcb->type != BTRFS_TYPE_FILE)
        return STATUS_INVALID_PARAMETER;

    fn.Buffer = &fri->FileName[1];
    fn.Length = fn.MaximumLength = (USHORT)(fri->FileNameLength - sizeof(WCHAR));

    // remove :$DATA suffix
    if (fn.Length >= sizeof(datasuf) - sizeof(WCHAR) &&
        RtlCompareMemory(&fn.Buffer[(fn.Length - sizeof(datasuf) + sizeof(WCHAR))/sizeof(WCHAR)], datasuf, sizeof(datasuf) - sizeof(WCHAR)) == sizeof(datasuf) - sizeof(WCHAR))
        fn.Length -= sizeof(datasuf) - sizeof(WCHAR);

    if (fn.Length == 0) {
        WARN("not allowing overwriting file with itself\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = check_file_name_valid(&fn, false, true);
    if (!NT_SUCCESS(Status)) {
        WARN("invalid stream name %.*S\n", (int)(fn.Length / sizeof(WCHAR)), fn.Buffer);
        return Status;
    }

    if (!(flags & FILE_RENAME_IGNORE_READONLY_ATTRIBUTE) && fileref->fcb->atts & FILE_ATTRIBUTE_READONLY) {
        WARN("trying to rename stream on readonly file\n");
        return STATUS_ACCESS_DENIED;
    }

    Status = open_fileref_child(Vcb, fileref, &fn, fileref->fcb->case_sensitive, true, true, PagedPool, &sf, Irp);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        if (Status == STATUS_SUCCESS) {
            if (fileref == sf || sf->deleted) {
                free_fileref(sf);
                sf = NULL;
            } else {
                if (!(flags & FILE_RENAME_REPLACE_IF_EXISTS)) {
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto end;
                }

                // FIXME - POSIX overwrites of stream?

                if (sf->open_count > 0) {
                    WARN("trying to overwrite open file\n");
                    Status = STATUS_ACCESS_DENIED;
                    goto end;
                }

                if (sf->fcb->adsdata.Length > 0) {
                    WARN("can only overwrite existing stream if it is zero-length\n");
                    Status = STATUS_INVALID_PARAMETER;
                    goto end;
                }

                Status = delete_fileref(sf, NULL, false, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_fileref returned %08lx\n", Status);
                    goto end;
                }
            }
        } else {
            ERR("open_fileref_child returned %08lx\n", Status);
            goto end;
        }
    }

    Status = utf16_to_utf8(NULL, 0, &utf8len, fn.Buffer, fn.Length);
    if (!NT_SUCCESS(Status))
        goto end;

    utf8.MaximumLength = utf8.Length = (uint16_t)utf8len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    Status = utf16_to_utf8(utf8.Buffer, utf8len, &utf8len, fn.Buffer, fn.Length);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(utf8.Buffer);
        goto end;
    }

    adsxattr.Length = adsxattr.MaximumLength = sizeof(xapref) - 1 + utf8.Length;
    adsxattr.Buffer = ExAllocatePoolWithTag(PagedPool, adsxattr.MaximumLength, ALLOC_TAG);
    if (!adsxattr.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        goto end;
    }

    RtlCopyMemory(adsxattr.Buffer, xapref, sizeof(xapref) - 1);
    RtlCopyMemory(&adsxattr.Buffer[sizeof(xapref) - 1], utf8.Buffer, utf8.Length);

    // don't allow if it's one of our reserved names

    if ((adsxattr.Length == sizeof(EA_DOSATTRIB) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_DOSATTRIB, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_EA) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_EA, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_REPARSE) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_REPARSE, adsxattr.Length) == adsxattr.Length) ||
        (adsxattr.Length == sizeof(EA_CASE_SENSITIVE) - sizeof(WCHAR) && RtlCompareMemory(adsxattr.Buffer, EA_CASE_SENSITIVE, adsxattr.Length) == adsxattr.Length)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        ExFreePool(utf8.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    utf16.Length = utf16.MaximumLength = fn.Length;
    utf16.Buffer = ExAllocatePoolWithTag(PagedPool, utf16.MaximumLength, ALLOC_TAG);
    if (!utf16.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    RtlCopyMemory(utf16.Buffer, fn.Buffer, fn.Length);

    newmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) -
                offsetof(DIR_ITEM, name[0]);

    if (newmaxlen < adsxattr.Length) {
        WARN("cannot rename as data too long\n");
        Status = STATUS_INVALID_PARAMETER;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    newmaxlen -= adsxattr.Length;

    if (newmaxlen < fileref->fcb->inode_item.st_size) {
        WARN("cannot rename as data too long\n");
        Status = STATUS_INVALID_PARAMETER;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    Status = RtlUpcaseUnicodeString(&utf16uc, &fn, true);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(adsxattr.Buffer);
        goto end;
    }

    // read existing file data

    if (fileref->fcb->inode_item.st_size > 0) {
        ULONG bytes_read;

        adsdata.Length = adsdata.MaximumLength = (uint16_t)fileref->fcb->inode_item.st_size;

        adsdata.Buffer = ExAllocatePoolWithTag(PagedPool, adsdata.MaximumLength, ALLOC_TAG);
        if (!adsdata.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ExFreePool(utf8.Buffer);
            ExFreePool(utf16.Buffer);
            ExFreePool(utf16uc.Buffer);
            ExFreePool(adsxattr.Buffer);
            goto end;
        }

        Status = read_file(fileref->fcb, (uint8_t*)adsdata.Buffer, 0, adsdata.Length, &bytes_read, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ExFreePool(utf8.Buffer);
            ExFreePool(utf16.Buffer);
            ExFreePool(utf16uc.Buffer);
            ExFreePool(adsxattr.Buffer);
            ExFreePool(adsdata.Buffer);
            goto end;
        }

        if (bytes_read < fileref->fcb->inode_item.st_size) {
            ERR("short read\n");
            Status = STATUS_INTERNAL_ERROR;
            ExFreePool(utf8.Buffer);
            ExFreePool(utf16.Buffer);
            ExFreePool(utf16uc.Buffer);
            ExFreePool(adsxattr.Buffer);
            ExFreePool(adsdata.Buffer);
            goto end;
        }
    } else
        adsdata.Buffer = NULL;

    dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
    if (!dc) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(utf16uc.Buffer);
        ExFreePool(adsxattr.Buffer);

        if (adsdata.Buffer)
            ExFreePool(adsdata.Buffer);

        goto end;
    }

    // add dummy deleted fcb with old name

    Status = duplicate_fcb(fileref->fcb, &dummyfcb);
    if (!NT_SUCCESS(Status)) {
        ERR("duplicate_fcb returned %08lx\n", Status);
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(utf16uc.Buffer);
        ExFreePool(adsxattr.Buffer);

        if (adsdata.Buffer)
            ExFreePool(adsdata.Buffer);

        ExFreePool(dc);

        goto end;
    }

    dummyfileref = create_fileref(Vcb);
    if (!dummyfileref) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(utf8.Buffer);
        ExFreePool(utf16.Buffer);
        ExFreePool(utf16uc.Buffer);
        ExFreePool(adsxattr.Buffer);

        if (adsdata.Buffer)
            ExFreePool(adsdata.Buffer);

        ExFreePool(dc);

        reap_fcb(dummyfcb);

        goto end;
    }

    dummyfileref->fcb = dummyfcb;

    dummyfcb->Vcb = Vcb;
    dummyfcb->subvol = fileref->fcb->subvol;
    dummyfcb->inode = fileref->fcb->inode;
    dummyfcb->hash = fileref->fcb->hash;

    if (fileref->fcb->inode_item.st_size > 0) {
        Status = excise_extents(Vcb, dummyfcb, 0, sector_align(fileref->fcb->inode_item.st_size, Vcb->superblock.sector_size),
                                Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            ExFreePool(utf8.Buffer);
            ExFreePool(utf16.Buffer);
            ExFreePool(utf16uc.Buffer);
            ExFreePool(adsxattr.Buffer);
            ExFreePool(adsdata.Buffer);
            ExFreePool(dc);

            reap_fileref(Vcb, dummyfileref);
            reap_fcb(dummyfcb);

            goto end;
        }

        dummyfcb->inode_item.st_size = 0;
        dummyfcb->Header.AllocationSize.QuadPart = 0;
        dummyfcb->Header.FileSize.QuadPart = 0;
        dummyfcb->Header.ValidDataLength.QuadPart = 0;
    }

    dummyfcb->hash_ptrs = fileref->fcb->hash_ptrs;
    dummyfcb->hash_ptrs_uc = fileref->fcb->hash_ptrs_uc;
    dummyfcb->created = fileref->fcb->created;

    le = fileref->fcb->extents.Flink;
    while (le != &fileref->fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        ext->ignore = true;

        le = le->Flink;
    }

    while (!IsListEmpty(&fileref->fcb->dir_children_index)) {
        InsertTailList(&dummyfcb->dir_children_index, RemoveHeadList(&fileref->fcb->dir_children_index));
    }

    while (!IsListEmpty(&fileref->fcb->dir_children_hash)) {
        InsertTailList(&dummyfcb->dir_children_hash, RemoveHeadList(&fileref->fcb->dir_children_hash));
    }

    while (!IsListEmpty(&fileref->fcb->dir_children_hash_uc)) {
        InsertTailList(&dummyfcb->dir_children_hash_uc, RemoveHeadList(&fileref->fcb->dir_children_hash_uc));
    }

    InsertTailList(&Vcb->all_fcbs, &dummyfcb->list_entry_all);

    InsertHeadList(fileref->fcb->list_entry.Blink, &dummyfcb->list_entry);

    if (fileref->fcb->subvol->fcbs_ptrs[dummyfcb->hash >> 24] == &fileref->fcb->list_entry)
        fileref->fcb->subvol->fcbs_ptrs[dummyfcb->hash >> 24] = &dummyfcb->list_entry;

    RemoveEntryList(&fileref->fcb->list_entry);
    fileref->fcb->list_entry.Flink = fileref->fcb->list_entry.Blink = NULL;

    mark_fcb_dirty(dummyfcb);

    // create dummy fileref

    dummyfileref->oldutf8 = fileref->oldutf8;
    dummyfileref->oldindex = fileref->oldindex;
    dummyfileref->delete_on_close = fileref->delete_on_close;
    dummyfileref->posix_delete = fileref->posix_delete;
    dummyfileref->deleted = fileref->deleted;
    dummyfileref->created = fileref->created;
    dummyfileref->parent = fileref->parent;

    while (!IsListEmpty(&fileref->children)) {
        file_ref* fr = CONTAINING_RECORD(RemoveHeadList(&fileref->children), file_ref, list_entry);

        free_fileref(fr->parent);

        fr->parent = dummyfileref;
        InterlockedIncrement(&dummyfileref->refcount);

        InsertTailList(&dummyfileref->children, &fr->list_entry);
    }

    InsertTailList(fileref->list_entry.Blink, &dummyfileref->list_entry);

    RemoveEntryList(&fileref->list_entry);
    InsertTailList(&dummyfileref->children, &fileref->list_entry);

    dummyfileref->dc = fileref->dc;
    dummyfileref->dc->fileref = dummyfileref;

    mark_fileref_dirty(dummyfileref);

    // change fcb values

    fileref->fcb->hash_ptrs = NULL;
    fileref->fcb->hash_ptrs_uc = NULL;

    fileref->fcb->ads = true;

    fileref->oldutf8.Length = fileref->oldutf8.MaximumLength = 0;
    fileref->oldutf8.Buffer = NULL;

    RtlZeroMemory(dc, sizeof(dir_child));

    dc->utf8 = utf8;
    dc->name = utf16;
    dc->hash = calc_crc32c(0xffffffff, (uint8_t*)dc->name.Buffer, dc->name.Length);
    dc->name_uc = utf16uc;
    dc->hash_uc = calc_crc32c(0xffffffff, (uint8_t*)dc->name_uc.Buffer, dc->name_uc.Length);
    dc->fileref = fileref;
    InsertTailList(&dummyfcb->dir_children_index, &dc->list_entry_index);

    fileref->dc = dc;
    fileref->parent = dummyfileref;

    crc32 = calc_crc32c(0xfffffffe, (uint8_t*)adsxattr.Buffer, adsxattr.Length);

    fileref->fcb->adsxattr = adsxattr;
    fileref->fcb->adshash = crc32;
    fileref->fcb->adsmaxlen = newmaxlen;
    fileref->fcb->adsdata = adsdata;

    fileref->fcb->created = true;

    mark_fcb_dirty(fileref->fcb);

    Status = STATUS_SUCCESS;

end:
    if (sf)
        free_fileref(sf);

    return Status;
}

static NTSTATUS set_rename_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, PFILE_OBJECT tfo, bool ex) {
    FILE_RENAME_INFORMATION_EX* fri = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref *fileref = ccb ? ccb->fileref : NULL, *oldfileref = NULL, *related = NULL, *fr2 = NULL;
    WCHAR* fn;
    ULONG fnlen, utf8len, origutf8len;
    UNICODE_STRING fnus;
    ANSI_STRING utf8;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    LIST_ENTRY rollback, *le;
    hardlink* hl;
    SECURITY_SUBJECT_CONTEXT subjcont;
    ACCESS_MASK access;
    ULONG flags;

    InitializeListHead(&rollback);

    if (ex)
        flags = fri->Flags;
    else
        flags = fri->ReplaceIfExists ? FILE_RENAME_REPLACE_IF_EXISTS : 0;

    TRACE("tfo = %p\n", tfo);
    TRACE("Flags = %lx\n", flags);
    TRACE("RootDirectory = %p\n", fri->RootDirectory);
    TRACE("FileName = %.*S\n", (int)(fri->FileNameLength / sizeof(WCHAR)), fri->FileName);

    fn = fri->FileName;
    fnlen = fri->FileNameLength / sizeof(WCHAR);

    if (!tfo) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref set and no directory given\n");
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        LONG i;

        while (fnlen > 0 && (fri->FileName[fnlen - 1] == '/' || fri->FileName[fnlen - 1] == '\\')) {
            fnlen--;
        }

        if (fnlen == 0)
            return STATUS_INVALID_PARAMETER;

        for (i = fnlen - 1; i >= 0; i--) {
            if (fri->FileName[i] == '\\' || fri->FileName[i] == '/') {
                fn = &fri->FileName[i+1];
                fnlen -= i + 1;
                break;
            }
        }
    }

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fcb->inode == SUBVOL_ROOT_INODE && fcb->subvol->id == BTRFS_ROOT_FSTREE) {
        WARN("not allowing \\$Root to be renamed\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    if (fcb->ads) {
        if (FileObject->SectionObjectPointer && FileObject->SectionObjectPointer->DataSectionObject) {
            IO_STATUS_BLOCK iosb;

            CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &iosb);
            if (!NT_SUCCESS(iosb.Status)) {
                ERR("CcFlushCache returned %08lx\n", iosb.Status);
                Status = iosb.Status;
                goto end;
            }
        }

        Status = rename_stream(Vcb, fileref, ccb, fri, flags, Irp, &rollback);
        goto end;
    } else if (fnlen >= 1 && fn[0] == ':') {
        if (FileObject->SectionObjectPointer && FileObject->SectionObjectPointer->DataSectionObject) {
            IO_STATUS_BLOCK iosb;

            CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &iosb);
            if (!NT_SUCCESS(iosb.Status)) {
                ERR("CcFlushCache returned %08lx\n", iosb.Status);
                Status = iosb.Status;
                goto end;
            }
        }

        Status = rename_file_to_stream(Vcb, fileref, ccb, fri, flags, Irp, &rollback);
        goto end;
    }

    fnus.Buffer = fn;
    fnus.Length = fnus.MaximumLength = (uint16_t)(fnlen * sizeof(WCHAR));

    TRACE("fnus = %.*S\n", (int)(fnus.Length / sizeof(WCHAR)), fnus.Buffer);

    Status = check_file_name_valid(&fnus, false, false);
    if (!NT_SUCCESS(Status))
        goto end;

    origutf8len = fileref->dc->utf8.Length;

    Status = utf16_to_utf8(NULL, 0, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto end;

    utf8.MaximumLength = utf8.Length = (uint16_t)utf8len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    Status = utf16_to_utf8(utf8.Buffer, utf8len, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
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

    Status = open_fileref(Vcb, &oldfileref, &fnus, related, false, NULL, NULL, PagedPool, ccb->case_sensitive,  Irp);

    if (NT_SUCCESS(Status)) {
        TRACE("destination file already exists\n");

        if (fileref != oldfileref && !oldfileref->deleted) {
            if (!(flags & FILE_RENAME_REPLACE_IF_EXISTS)) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            } else if (fileref == oldfileref) {
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (!(flags & FILE_RENAME_POSIX_SEMANTICS) && (oldfileref->open_count > 0 || has_open_children(oldfileref)) && !oldfileref->deleted) {
                WARN("trying to overwrite open file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (!(flags & FILE_RENAME_IGNORE_READONLY_ATTRIBUTE) && oldfileref->fcb->atts & FILE_ATTRIBUTE_READONLY) {
                WARN("trying to overwrite readonly file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (oldfileref->fcb->type == BTRFS_TYPE_DIRECTORY) {
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
        Status = open_fileref(Vcb, &related, &fnus, NULL, true, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08lx\n", Status);
            goto end;
        }
    }

    if (related->fcb == Vcb->dummy_fcb) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    SeCaptureSubjectContext(&subjcont);

    if (!SeAccessCheck(related->fcb->sd, &subjcont, false, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0, NULL,
        IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
        SeReleaseSubjectContext(&subjcont);
        TRACE("SeAccessCheck failed, returning %08lx\n", Status);
        goto end;
    }

    SeReleaseSubjectContext(&subjcont);

    if (has_open_children(fileref)) {
        WARN("trying to rename file with open children\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    if (oldfileref) {
        SeCaptureSubjectContext(&subjcont);

        if (!SeAccessCheck(oldfileref->fcb->sd, &subjcont, false, DELETE, 0, NULL,
                           IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
            SeReleaseSubjectContext(&subjcont);
            TRACE("SeAccessCheck failed, returning %08lx\n", Status);
            goto end;
        }

        SeReleaseSubjectContext(&subjcont);

        if (oldfileref->open_count > 0 && flags & FILE_RENAME_POSIX_SEMANTICS) {
            oldfileref->delete_on_close = true;
            oldfileref->posix_delete = true;
        }

        Status = delete_fileref(oldfileref, NULL, oldfileref->open_count > 0 && flags & FILE_RENAME_POSIX_SEMANTICS, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_fileref returned %08lx\n", Status);
            goto end;
        }
    }

    if (fileref->parent->fcb->subvol != related->fcb->subvol && (fileref->fcb->subvol == fileref->parent->fcb->subvol || fileref->fcb == Vcb->dummy_fcb)) {
        Status = move_across_subvols(fileref, ccb, related, &utf8, &fnus, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("move_across_subvols returned %08lx\n", Status);
        }
        goto end;
    }

    if (related == fileref->parent) { // keeping file in same directory
        UNICODE_STRING oldfn, newfn;
        USHORT name_offset;
        ULONG reqlen, oldutf8len;

        oldfn.Length = oldfn.MaximumLength = 0;

        Status = fileref_get_filename(fileref, &oldfn, &name_offset, &reqlen);
        if (Status != STATUS_BUFFER_OVERFLOW) {
            ERR("fileref_get_filename returned %08lx\n", Status);
            goto end;
        }

        oldfn.Buffer = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);
        if (!oldfn.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        oldfn.MaximumLength = (uint16_t)reqlen;

        Status = fileref_get_filename(fileref, &oldfn, &name_offset, &reqlen);
        if (!NT_SUCCESS(Status)) {
            ERR("fileref_get_filename returned %08lx\n", Status);
            ExFreePool(oldfn.Buffer);
            goto end;
        }

        oldutf8len = fileref->dc->utf8.Length;

        if (!fileref->created && !fileref->oldutf8.Buffer) {
            fileref->oldutf8.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->dc->utf8.Length, ALLOC_TAG);
            if (!fileref->oldutf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            fileref->oldutf8.Length = fileref->oldutf8.MaximumLength = fileref->dc->utf8.Length;
            RtlCopyMemory(fileref->oldutf8.Buffer, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);
        }

        TRACE("renaming %.*S to %.*S\n", (int)(fileref->dc->name.Length / sizeof(WCHAR)), fileref->dc->name.Buffer, (int)(fnus.Length / sizeof(WCHAR)), fnus.Buffer);

        mark_fileref_dirty(fileref);

        if (fileref->dc) {
            ExAcquireResourceExclusiveLite(&fileref->parent->fcb->nonpaged->dir_children_lock, true);

            ExFreePool(fileref->dc->utf8.Buffer);
            ExFreePool(fileref->dc->name.Buffer);
            ExFreePool(fileref->dc->name_uc.Buffer);

            fileref->dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.Length, ALLOC_TAG);
            if (!fileref->dc->utf8.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                ExFreePool(oldfn.Buffer);
                goto end;
            }

            fileref->dc->utf8.Length = fileref->dc->utf8.MaximumLength = utf8.Length;
            RtlCopyMemory(fileref->dc->utf8.Buffer, utf8.Buffer, utf8.Length);

            fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);
            if (!fileref->dc->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                ExFreePool(oldfn.Buffer);
                goto end;
            }

            fileref->dc->name.Length = fileref->dc->name.MaximumLength = fnus.Length;
            RtlCopyMemory(fileref->dc->name.Buffer, fnus.Buffer, fnus.Length);

            Status = RtlUpcaseUnicodeString(&fileref->dc->name_uc, &fileref->dc->name, true);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
                ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
                ExFreePool(oldfn.Buffer);
                goto end;
            }

            remove_dir_child_from_hash_lists(fileref->parent->fcb, fileref->dc);

            fileref->dc->hash = calc_crc32c(0xffffffff, (uint8_t*)fileref->dc->name.Buffer, fileref->dc->name.Length);
            fileref->dc->hash_uc = calc_crc32c(0xffffffff, (uint8_t*)fileref->dc->name_uc.Buffer, fileref->dc->name_uc.Length);

            insert_dir_child_into_hash_lists(fileref->parent->fcb, fileref->dc);

            ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);
        }

        newfn.Length = newfn.MaximumLength = 0;

        Status = fileref_get_filename(fileref, &newfn, &name_offset, &reqlen);
        if (Status != STATUS_BUFFER_OVERFLOW) {
            ERR("fileref_get_filename returned %08lx\n", Status);
            ExFreePool(oldfn.Buffer);
            goto end;
        }

        newfn.Buffer = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);
        if (!newfn.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ExFreePool(oldfn.Buffer);
            goto end;
        }

        newfn.MaximumLength = (uint16_t)reqlen;

        Status = fileref_get_filename(fileref, &newfn, &name_offset, &reqlen);
        if (!NT_SUCCESS(Status)) {
            ERR("fileref_get_filename returned %08lx\n", Status);
            ExFreePool(oldfn.Buffer);
            ExFreePool(newfn.Buffer);
            goto end;
        }

        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        if (fcb != Vcb->dummy_fcb && (fileref->parent->fcb->subvol == fcb->subvol || !is_subvol_readonly(fcb->subvol, Irp))) {
            fcb->inode_item.transid = Vcb->superblock.generation;
            fcb->inode_item.sequence++;

            if (!ccb->user_set_change_time)
                fcb->inode_item.st_ctime = now;

            fcb->inode_item_changed = true;
            mark_fcb_dirty(fcb);
        }

        // update parent's INODE_ITEM

        related->fcb->inode_item.transid = Vcb->superblock.generation;
        TRACE("related->fcb->inode_item.st_size (inode %I64x) was %I64x\n", related->fcb->inode, related->fcb->inode_item.st_size);
        related->fcb->inode_item.st_size = related->fcb->inode_item.st_size + (2 * utf8.Length) - (2* oldutf8len);
        TRACE("related->fcb->inode_item.st_size (inode %I64x) now %I64x\n", related->fcb->inode, related->fcb->inode_item.st_size);
        related->fcb->inode_item.sequence++;
        related->fcb->inode_item.st_ctime = now;
        related->fcb->inode_item.st_mtime = now;

        related->fcb->inode_item_changed = true;
        mark_fcb_dirty(related->fcb);
        send_notification_fileref(related, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

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

    send_notification_fileref(fileref, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED, NULL);

    fr2 = create_fileref(Vcb);

    fr2->fcb = fileref->fcb;
    fr2->fcb->refcount++;

    fr2->oldutf8 = fileref->oldutf8;
    fr2->oldindex = fileref->dc->index;
    fr2->delete_on_close = fileref->delete_on_close;
    fr2->deleted = true;
    fr2->created = fileref->created;
    fr2->parent = fileref->parent;
    fr2->dc = NULL;

    if (!fr2->oldutf8.Buffer) {
        fr2->oldutf8.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->dc->utf8.Length, ALLOC_TAG);
        if (!fr2->oldutf8.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(fr2->oldutf8.Buffer, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        fr2->oldutf8.Length = fr2->oldutf8.MaximumLength = fileref->dc->utf8.Length;
    }

    if (fr2->fcb->type == BTRFS_TYPE_DIRECTORY)
        fr2->fcb->fileref = fr2;

    if (fileref->fcb->inode == SUBVOL_ROOT_INODE)
        fileref->fcb->subvol->parent = related->fcb->subvol->id;

    fileref->oldutf8.Length = fileref->oldutf8.MaximumLength = 0;
    fileref->oldutf8.Buffer = NULL;
    fileref->deleted = false;
    fileref->created = true;
    fileref->parent = related;

    ExAcquireResourceExclusiveLite(&fileref->parent->fcb->nonpaged->dir_children_lock, true);
    InsertHeadList(&fileref->list_entry, &fr2->list_entry);
    RemoveEntryList(&fileref->list_entry);
    ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);

    mark_fileref_dirty(fr2);
    mark_fileref_dirty(fileref);

    if (fileref->dc) {
        // remove from old parent
        ExAcquireResourceExclusiveLite(&fr2->parent->fcb->nonpaged->dir_children_lock, true);
        RemoveEntryList(&fileref->dc->list_entry_index);
        remove_dir_child_from_hash_lists(fr2->parent->fcb, fileref->dc);
        ExReleaseResourceLite(&fr2->parent->fcb->nonpaged->dir_children_lock);

        if (fileref->dc->utf8.Length != utf8.Length || RtlCompareMemory(fileref->dc->utf8.Buffer, utf8.Buffer, utf8.Length) != utf8.Length) {
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

            fileref->dc->utf8.Length = fileref->dc->utf8.MaximumLength = utf8.Length;
            RtlCopyMemory(fileref->dc->utf8.Buffer, utf8.Buffer, utf8.Length);

            fileref->dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);
            if (!fileref->dc->name.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            fileref->dc->name.Length = fileref->dc->name.MaximumLength = fnus.Length;
            RtlCopyMemory(fileref->dc->name.Buffer, fnus.Buffer, fnus.Length);

            Status = RtlUpcaseUnicodeString(&fileref->dc->name_uc, &fileref->dc->name, true);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
                goto end;
            }

            fileref->dc->hash = calc_crc32c(0xffffffff, (uint8_t*)fileref->dc->name.Buffer, fileref->dc->name.Length);
            fileref->dc->hash_uc = calc_crc32c(0xffffffff, (uint8_t*)fileref->dc->name_uc.Buffer, fileref->dc->name_uc.Length);
        }

        // add to new parent
        ExAcquireResourceExclusiveLite(&related->fcb->nonpaged->dir_children_lock, true);

        if (IsListEmpty(&related->fcb->dir_children_index))
            fileref->dc->index = 2;
        else {
            dir_child* dc2 = CONTAINING_RECORD(related->fcb->dir_children_index.Blink, dir_child, list_entry_index);

            fileref->dc->index = max(2, dc2->index + 1);
        }

        InsertTailList(&related->fcb->dir_children_index, &fileref->dc->list_entry_index);
        insert_dir_child_into_hash_lists(related->fcb, fileref->dc);
        ExReleaseResourceLite(&related->fcb->nonpaged->dir_children_lock);
    }

    ExAcquireResourceExclusiveLite(&related->fcb->nonpaged->dir_children_lock, true);
    InsertTailList(&related->children, &fileref->list_entry);
    ExReleaseResourceLite(&related->fcb->nonpaged->dir_children_lock);

    if (fcb->inode_item.st_nlink > 1) {
        // add new hardlink entry to fcb

        hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
        if (!hl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        hl->parent = related->fcb->inode;
        hl->index = fileref->dc->index;

        hl->name.Length = hl->name.MaximumLength = fnus.Length;
        hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, hl->name.MaximumLength, ALLOC_TAG);

        if (!hl->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(hl->name.Buffer, fnus.Buffer, fnus.Length);

        hl->utf8.Length = hl->utf8.MaximumLength = fileref->dc->utf8.Length;
        hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);

        if (!hl->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl->name.Buffer);
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(hl->utf8.Buffer, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        InsertTailList(&fcb->hardlinks, &hl->list_entry);
    }

    // delete old hardlink entry from fcb

    le = fcb->hardlinks.Flink;
    while (le != &fcb->hardlinks) {
        hl = CONTAINING_RECORD(le, hardlink, list_entry);

        if (hl->parent == fr2->parent->fcb->inode && hl->index == fr2->oldindex) {
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

    if (fcb != Vcb->dummy_fcb && (fileref->parent->fcb->subvol == fcb->subvol || !is_subvol_readonly(fcb->subvol, Irp))) {
        fcb->inode_item.transid = Vcb->superblock.generation;
        fcb->inode_item.sequence++;

        if (!ccb->user_set_change_time)
            fcb->inode_item.st_ctime = now;

        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);
    }

    // update new parent's INODE_ITEM

    related->fcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("related->fcb->inode_item.st_size (inode %I64x) was %I64x\n", related->fcb->inode, related->fcb->inode_item.st_size);
    related->fcb->inode_item.st_size += 2 * utf8len;
    TRACE("related->fcb->inode_item.st_size (inode %I64x) now %I64x\n", related->fcb->inode, related->fcb->inode_item.st_size);
    related->fcb->inode_item.sequence++;
    related->fcb->inode_item.st_ctime = now;
    related->fcb->inode_item.st_mtime = now;

    related->fcb->inode_item_changed = true;
    mark_fcb_dirty(related->fcb);

    // update old parent's INODE_ITEM

    fr2->parent->fcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("fr2->parent->fcb->inode_item.st_size (inode %I64x) was %I64x\n", fr2->parent->fcb->inode, fr2->parent->fcb->inode_item.st_size);
    fr2->parent->fcb->inode_item.st_size -= 2 * origutf8len;
    TRACE("fr2->parent->fcb->inode_item.st_size (inode %I64x) now %I64x\n", fr2->parent->fcb->inode, fr2->parent->fcb->inode_item.st_size);
    fr2->parent->fcb->inode_item.sequence++;
    fr2->parent->fcb->inode_item.st_ctime = now;
    fr2->parent->fcb->inode_item.st_mtime = now;

    free_fileref(fr2);

    fr2->parent->fcb->inode_item_changed = true;
    mark_fcb_dirty(fr2->parent->fcb);

    send_notification_fileref(fileref, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);
    send_notification_fileref(related, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);
    send_notification_fileref(fr2->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:
    if (oldfileref)
        free_fileref(oldfileref);

    if (!NT_SUCCESS(Status) && related)
        free_fileref(related);

    if (!NT_SUCCESS(Status) && fr2)
        free_fileref(fr2);

    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->fileref_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

NTSTATUS stream_set_end_of_file_information(device_extension* Vcb, uint16_t end, fcb* fcb, file_ref* fileref, bool advance_only) {
    LARGE_INTEGER time;
    BTRFS_TIME now;

    TRACE("setting new end to %x bytes (currently %x)\n", end, fcb->adsdata.Length);

    if (!fileref || !fileref->parent) {
        ERR("no fileref for stream\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (end < fcb->adsdata.Length) {
        if (advance_only)
            return STATUS_SUCCESS;

        TRACE("truncating stream to %x bytes\n", end);

        fcb->adsdata.Length = end;
    } else if (end > fcb->adsdata.Length) {
        TRACE("extending stream to %x bytes\n", end);

        if (end > fcb->adsmaxlen) {
            ERR("error - xattr too long (%u > %lu)\n", end, fcb->adsmaxlen);
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

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fileref->parent->fcb->inode_item.transid = Vcb->superblock.generation;
    fileref->parent->fcb->inode_item.sequence++;
    fileref->parent->fcb->inode_item.st_ctime = now;

    fileref->parent->fcb->inode_item_changed = true;
    mark_fcb_dirty(fileref->parent->fcb);

    fileref->parent->fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fileref->parent->fcb->subvol->root_item.ctime = now;

    return STATUS_SUCCESS;
}

static NTSTATUS set_end_of_file_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, bool advance_only, bool prealloc) {
    FILE_END_OF_FILE_INFORMATION* feofi = Irp->AssociatedIrp.SystemBuffer;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    NTSTATUS Status;
    LARGE_INTEGER time;
    CC_FILE_SIZES ccfs;
    LIST_ENTRY rollback;
    bool set_size = false;
    ULONG filter;

    if (!fileref) {
        ERR("fileref is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    InitializeListHead(&rollback);

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fileref ? fileref->deleted : fcb->deleted) {
        Status = STATUS_FILE_CLOSED;
        goto end;
    }

    if (fcb->ads) {
        if (feofi->EndOfFile.QuadPart > 0xffff) {
            Status = STATUS_DISK_FULL;
            goto end;
        }

        if (feofi->EndOfFile.QuadPart < 0) {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        Status = stream_set_end_of_file_information(Vcb, (uint16_t)feofi->EndOfFile.QuadPart, fcb, fileref, advance_only);

        if (NT_SUCCESS(Status)) {
            ccfs.AllocationSize = fcb->Header.AllocationSize;
            ccfs.FileSize = fcb->Header.FileSize;
            ccfs.ValidDataLength = fcb->Header.ValidDataLength;
            set_size = true;
        }

        filter = FILE_NOTIFY_CHANGE_STREAM_SIZE;

        if (!ccb->user_set_write_time) {
            KeQuerySystemTime(&time);
            win_time_to_unix(time, &fileref->parent->fcb->inode_item.st_mtime);
            filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;

            fileref->parent->fcb->inode_item_changed = true;
            mark_fcb_dirty(fileref->parent->fcb);
        }

        queue_notification_fcb(fileref->parent, filter, FILE_ACTION_MODIFIED_STREAM, &fileref->dc->name);

        goto end;
    }

    TRACE("file: %p\n", FileObject);
    TRACE("paging IO: %s\n", Irp->Flags & IRP_PAGING_IO ? "true" : "false");
    TRACE("FileObject: AllocationSize = %I64x, FileSize = %I64x, ValidDataLength = %I64x\n",
        fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);

    TRACE("setting new end to %I64x bytes (currently %I64x)\n", feofi->EndOfFile.QuadPart, fcb->inode_item.st_size);

    if ((uint64_t)feofi->EndOfFile.QuadPart < fcb->inode_item.st_size) {
        if (advance_only) {
            Status = STATUS_SUCCESS;
            goto end;
        }

        TRACE("truncating file to %I64x bytes\n", feofi->EndOfFile.QuadPart);

        if (!MmCanFileBeTruncated(&fcb->nonpaged->segment_object, &feofi->EndOfFile)) {
            Status = STATUS_USER_MAPPED_FILE;
            goto end;
        }

        Status = truncate_file(fcb, feofi->EndOfFile.QuadPart, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - truncate_file failed\n");
            goto end;
        }
    } else if ((uint64_t)feofi->EndOfFile.QuadPart > fcb->inode_item.st_size) {
        TRACE("extending file to %I64x bytes\n", feofi->EndOfFile.QuadPart);

        Status = extend_file(fcb, fileref, feofi->EndOfFile.QuadPart, prealloc, NULL, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("error - extend_file failed\n");
            goto end;
        }
    } else if ((uint64_t)feofi->EndOfFile.QuadPart == fcb->inode_item.st_size && advance_only) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    ccfs.AllocationSize = fcb->Header.AllocationSize;
    ccfs.FileSize = fcb->Header.FileSize;
    ccfs.ValidDataLength = fcb->Header.ValidDataLength;
    set_size = true;

    filter = FILE_NOTIFY_CHANGE_SIZE;

    if (!ccb->user_set_write_time) {
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &fcb->inode_item.st_mtime);
        filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    }

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);
    queue_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);

    if (set_size) {
        _SEH2_TRY {
            CcSetFileSizes(FileObject, &ccfs);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status))
            ERR("CcSetFileSizes threw exception %08lx\n", Status);
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS set_position_information(PFILE_OBJECT FileObject, PIRP Irp) {
    FILE_POSITION_INFORMATION* fpi = (FILE_POSITION_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;

    TRACE("setting the position on %p to %I64x\n", FileObject, fpi->CurrentByteOffset.QuadPart);

    // FIXME - make sure aligned for FO_NO_INTERMEDIATE_BUFFERING

    FileObject->CurrentByteOffset = fpi->CurrentByteOffset;

    return STATUS_SUCCESS;
}

static NTSTATUS set_link_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject, PFILE_OBJECT tfo, bool ex) {
    FILE_LINK_INFORMATION_EX* fli = Irp->AssociatedIrp.SystemBuffer;
    fcb *fcb = FileObject->FsContext, *tfofcb, *parfcb;
    ccb* ccb = FileObject->FsContext2;
    file_ref *fileref = ccb ? ccb->fileref : NULL, *oldfileref = NULL, *related = NULL, *fr2 = NULL;
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
    ULONG flags;

    InitializeListHead(&rollback);

    // FIXME - check fli length
    // FIXME - don't ignore fli->RootDirectory

    if (ex)
        flags = fli->Flags;
    else
        flags = fli->ReplaceIfExists ? FILE_LINK_REPLACE_IF_EXISTS : 0;

    TRACE("flags = %lx\n", flags);
    TRACE("RootDirectory = %p\n", fli->RootDirectory);
    TRACE("FileNameLength = %lx\n", fli->FileNameLength);
    TRACE("FileName = %.*S\n", (int)(fli->FileNameLength / sizeof(WCHAR)), fli->FileName);

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

        while (fnlen > 0 && (fli->FileName[fnlen - 1] == '/' || fli->FileName[fnlen - 1] == '\\')) {
            fnlen--;
        }

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

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

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

    if (fcb->inode_item.st_nlink >= 65535) {
        Status = STATUS_TOO_MANY_LINKS;
        goto end;
    }

    fnus.Buffer = fn;
    fnus.Length = fnus.MaximumLength = (uint16_t)(fnlen * sizeof(WCHAR));

    TRACE("fnus = %.*S\n", (int)(fnus.Length / sizeof(WCHAR)), fnus.Buffer);

    Status = check_file_name_valid(&fnus, false, false);
    if (!NT_SUCCESS(Status))
        goto end;

    Status = utf16_to_utf8(NULL, 0, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto end;

    utf8.MaximumLength = utf8.Length = (uint16_t)utf8len;
    utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8.MaximumLength, ALLOC_TAG);
    if (!utf8.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    Status = utf16_to_utf8(utf8.Buffer, utf8len, &utf8len, fn, (ULONG)fnlen * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto end;

    if (tfo && tfo->FsContext2) {
        struct _ccb* relatedccb = tfo->FsContext2;

        related = relatedccb->fileref;
        increase_fileref_refcount(related);
    }

    Status = open_fileref(Vcb, &oldfileref, &fnus, related, false, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

    if (NT_SUCCESS(Status)) {
        if (!oldfileref->deleted) {
            WARN("destination file already exists\n");

            if (!(flags & FILE_LINK_REPLACE_IF_EXISTS)) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto end;
            } else if (fileref == oldfileref) {
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (!(flags & FILE_LINK_POSIX_SEMANTICS) && (oldfileref->open_count > 0 || has_open_children(oldfileref)) && !oldfileref->deleted) {
                WARN("trying to overwrite open file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (!(flags & FILE_LINK_IGNORE_READONLY_ATTRIBUTE) && oldfileref->fcb->atts & FILE_ATTRIBUTE_READONLY) {
                WARN("trying to overwrite readonly file\n");
                Status = STATUS_ACCESS_DENIED;
                goto end;
            } else if (oldfileref->fcb->type == BTRFS_TYPE_DIRECTORY) {
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
        Status = open_fileref(Vcb, &related, &fnus, NULL, true, NULL, NULL, PagedPool, ccb->case_sensitive, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("open_fileref returned %08lx\n", Status);
            goto end;
        }
    }

    SeCaptureSubjectContext(&subjcont);

    if (!SeAccessCheck(related->fcb->sd, &subjcont, false, FILE_ADD_FILE, 0, NULL,
                       IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
        SeReleaseSubjectContext(&subjcont);
        TRACE("SeAccessCheck failed, returning %08lx\n", Status);
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

        if (!SeAccessCheck(oldfileref->fcb->sd, &subjcont, false, DELETE, 0, NULL,
                           IoGetFileObjectGenericMapping(), Irp->RequestorMode, &access, &Status)) {
            SeReleaseSubjectContext(&subjcont);
            TRACE("SeAccessCheck failed, returning %08lx\n", Status);
            goto end;
        }

        SeReleaseSubjectContext(&subjcont);

        if (oldfileref->open_count > 0 && flags & FILE_RENAME_POSIX_SEMANTICS) {
            oldfileref->delete_on_close = true;
            oldfileref->posix_delete = true;
        }

        Status = delete_fileref(oldfileref, NULL, oldfileref->open_count > 0 && flags & FILE_RENAME_POSIX_SEMANTICS, Irp, &rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_fileref returned %08lx\n", Status);
            goto end;
        }
    }

    fr2 = create_fileref(Vcb);

    fr2->fcb = fcb;
    fcb->refcount++;

    fr2->created = true;
    fr2->parent = related;

    Status = add_dir_child(related->fcb, fcb->inode, false, &utf8, &fnus, fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08lx\n", Status);

    fr2->dc = dc;
    dc->fileref = fr2;

    ExAcquireResourceExclusiveLite(&related->fcb->nonpaged->dir_children_lock, true);
    InsertTailList(&related->children, &fr2->list_entry);
    ExReleaseResourceLite(&related->fcb->nonpaged->dir_children_lock);

    // add hardlink for existing fileref, if it's not there already
    if (IsListEmpty(&fcb->hardlinks)) {
        hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
        if (!hl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        hl->parent = fileref->parent->fcb->inode;
        hl->index = fileref->dc->index;

        hl->name.Length = hl->name.MaximumLength = fnus.Length;
        hl->name.Buffer = ExAllocatePoolWithTag(PagedPool, fnus.Length, ALLOC_TAG);

        if (!hl->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(hl->name.Buffer, fnus.Buffer, fnus.Length);

        hl->utf8.Length = hl->utf8.MaximumLength = fileref->dc->utf8.Length;
        hl->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, hl->utf8.MaximumLength, ALLOC_TAG);

        if (!hl->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(hl->name.Buffer);
            ExFreePool(hl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(hl->utf8.Buffer, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        InsertTailList(&fcb->hardlinks, &hl->list_entry);
    }

    hl = ExAllocatePoolWithTag(PagedPool, sizeof(hardlink), ALLOC_TAG);
    if (!hl) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    hl->parent = related->fcb->inode;
    hl->index = dc->index;

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
    ExFreePool(utf8.Buffer);

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

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    // update parent's INODE_ITEM

    parfcb->inode_item.transid = Vcb->superblock.generation;
    TRACE("parfcb->inode_item.st_size (inode %I64x) was %I64x\n", parfcb->inode, parfcb->inode_item.st_size);
    parfcb->inode_item.st_size += 2 * utf8len;
    TRACE("parfcb->inode_item.st_size (inode %I64x) now %I64x\n", parfcb->inode, parfcb->inode_item.st_size);
    parfcb->inode_item.sequence++;
    parfcb->inode_item.st_ctime = now;

    parfcb->inode_item_changed = true;
    mark_fcb_dirty(parfcb);

    send_notification_fileref(fr2, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED, NULL);

    Status = STATUS_SUCCESS;

end:
    if (oldfileref)
        free_fileref(oldfileref);

    if (!NT_SUCCESS(Status) && related)
        free_fileref(related);

    if (!NT_SUCCESS(Status) && fr2)
        free_fileref(fr2);

    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&Vcb->fileref_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS set_valid_data_length_information(device_extension* Vcb, PIRP Irp, PFILE_OBJECT FileObject) {
    FILE_VALID_DATA_LENGTH_INFORMATION* fvdli = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    NTSTATUS Status;
    LARGE_INTEGER time;
    CC_FILE_SIZES ccfs;
    LIST_ENTRY rollback;
    bool set_size = false;
    ULONG filter;

    if (IrpSp->Parameters.SetFile.Length < sizeof(FILE_VALID_DATA_LENGTH_INFORMATION)) {
        ERR("input buffer length was %lu, expected %Iu\n", IrpSp->Parameters.SetFile.Length, sizeof(FILE_VALID_DATA_LENGTH_INFORMATION));
        return STATUS_INVALID_PARAMETER;
    }

    if (!fileref) {
        ERR("fileref is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    InitializeListHead(&rollback);

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fcb->atts & FILE_ATTRIBUTE_SPARSE_FILE) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (fvdli->ValidDataLength.QuadPart <= fcb->Header.ValidDataLength.QuadPart || fvdli->ValidDataLength.QuadPart > fcb->Header.FileSize.QuadPart) {
        TRACE("invalid VDL of %I64u (current VDL = %I64u, file size = %I64u)\n", fvdli->ValidDataLength.QuadPart,
              fcb->Header.ValidDataLength.QuadPart, fcb->Header.FileSize.QuadPart);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (fileref ? fileref->deleted : fcb->deleted) {
        Status = STATUS_FILE_CLOSED;
        goto end;
    }

    // This function doesn't really do anything - the fsctl can only increase the value of ValidDataLength,
    // and we set it to the max anyway.

    ccfs.AllocationSize = fcb->Header.AllocationSize;
    ccfs.FileSize = fcb->Header.FileSize;
    ccfs.ValidDataLength = fvdli->ValidDataLength;
    set_size = true;

    filter = FILE_NOTIFY_CHANGE_SIZE;

    if (!ccb->user_set_write_time) {
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &fcb->inode_item.st_mtime);
        filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    }

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    queue_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    ExReleaseResourceLite(fcb->Header.Resource);

    if (set_size) {
        _SEH2_TRY {
            CcSetFileSizes(FileObject, &ccfs);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status))
            ERR("CcSetFileSizes threw exception %08lx\n", Status);
        else
            fcb->Header.AllocationSize = ccfs.AllocationSize;
    }

    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

#ifndef __REACTOS__
static NTSTATUS set_case_sensitive_information(PIRP Irp) {
    FILE_CASE_SENSITIVE_INFORMATION* fcsi = (FILE_CASE_SENSITIVE_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(FILE_CASE_SENSITIVE_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    PFILE_OBJECT FileObject = IrpSp->FileObject;

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    fcb* fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    if (!(fcb->atts & FILE_ATTRIBUTE_DIRECTORY)) {
        WARN("cannot set case-sensitive flag on anything other than directory\n");
        return STATUS_INVALID_PARAMETER;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);

    fcb->case_sensitive = fcsi->Flags & FILE_CS_FLAG_CASE_SENSITIVE_DIR;
    mark_fcb_dirty(fcb);

    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return STATUS_SUCCESS;
}
#endif

_Dispatch_type_(IRP_MJ_SET_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    fcb* fcb = IrpSp->FileObject->FsContext;
    ccb* ccb = IrpSp->FileObject->FsContext2;
    bool top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    Irp->IoStatus.Information = 0;

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
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

    if (fcb != Vcb->dummy_fcb && is_subvol_readonly(fcb->subvol, Irp) && IrpSp->Parameters.SetFile.FileInformationClass != FilePositionInformation &&
#ifndef __REACTOS__
        (fcb->inode != SUBVOL_ROOT_INODE || (IrpSp->Parameters.SetFile.FileInformationClass != FileBasicInformation && IrpSp->Parameters.SetFile.FileInformationClass != FileRenameInformation && IrpSp->Parameters.SetFile.FileInformationClass != FileRenameInformationEx))) {
#else
        (fcb->inode != SUBVOL_ROOT_INODE || (IrpSp->Parameters.SetFile.FileInformationClass != FileBasicInformation && IrpSp->Parameters.SetFile.FileInformationClass != FileRenameInformation))) {
#endif
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Status = STATUS_NOT_IMPLEMENTED;

    TRACE("set information\n");

    FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

    switch (IrpSp->Parameters.SetFile.FileInformationClass) {
        case FileAllocationInformation:
        {
            TRACE("FileAllocationInformation\n");

            if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_DATA)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }

            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, false, true);
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

            Status = set_disposition_information(Vcb, Irp, IrpSp->FileObject, false);

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

            Status = set_end_of_file_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.AdvanceOnly, false);

            break;
        }

        case FileLinkInformation:
            TRACE("FileLinkInformation\n");
            Status = set_link_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject, false);
            break;

        case FilePositionInformation:
            TRACE("FilePositionInformation\n");
            Status = set_position_information(IrpSp->FileObject, Irp);
            break;

        case FileRenameInformation:
            TRACE("FileRenameInformation\n");
            Status = set_rename_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject, false);
            break;

        case FileValidDataLengthInformation:
        {
            TRACE("FileValidDataLengthInformation\n");

            if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }

            Status = set_valid_data_length_information(Vcb, Irp, IrpSp->FileObject);

            break;
        }

#ifndef __REACTOS__
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
        case FileDispositionInformationEx:
        {
            TRACE("FileDispositionInformationEx\n");

            if (Irp->RequestorMode == UserMode && !(ccb->access & DELETE)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }

            Status = set_disposition_information(Vcb, Irp, IrpSp->FileObject, true);

            break;
        }

        case FileRenameInformationEx:
            TRACE("FileRenameInformationEx\n");
            Status = set_rename_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject, true);
            break;

        case FileLinkInformationEx:
            TRACE("FileLinkInformationEx\n");
            Status = set_link_information(Vcb, Irp, IrpSp->FileObject, IrpSp->Parameters.SetFile.FileObject, true);
            break;

        case FileCaseSensitiveInformation:
            TRACE("FileCaseSensitiveInformation\n");

            if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_WRITE_ATTRIBUTES)) {
                WARN("insufficient privileges\n");
                Status = STATUS_ACCESS_DENIED;
                break;
            }

            Status = set_case_sensitive_information(Irp);
            break;

        case FileStorageReserveIdInformation:
            WARN("unimplemented FileInformationClass FileStorageReserveIdInformation\n");
            break;

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif

        default:
            WARN("unknown FileInformationClass %u\n", IrpSp->Parameters.SetFile.FileInformationClass);
    }

end:
    Irp->IoStatus.Status = Status;

    TRACE("returning %08lx\n", Status);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS fill_in_file_basic_information(FILE_BASIC_INFORMATION* fbi, INODE_ITEM* ii, LONG* length, fcb* fcb, file_ref* fileref) {
    RtlZeroMemory(fbi, sizeof(FILE_BASIC_INFORMATION));

    *length -= sizeof(FILE_BASIC_INFORMATION);

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fbi->CreationTime = fbi->LastAccessTime = fbi->LastWriteTime = fbi->ChangeTime = time;
    } else {
        fbi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
        fbi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
        fbi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
        fbi->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    }

    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        } else
            fbi->FileAttributes = fileref->parent->fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fileref->parent->fcb->atts;
    } else
        fbi->FileAttributes = fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fcb->atts;

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_network_open_information(FILE_NETWORK_OPEN_INFORMATION* fnoi, fcb* fcb, file_ref* fileref, LONG* length) {
    INODE_ITEM* ii;

    if (*length < (LONG)sizeof(FILE_NETWORK_OPEN_INFORMATION)) {
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

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fnoi->CreationTime = fnoi->LastAccessTime = fnoi->LastWriteTime = fnoi->ChangeTime = time;
    } else {
        fnoi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
        fnoi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
        fnoi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
        fnoi->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    }

    if (fcb->ads) {
        fnoi->AllocationSize.QuadPart = fnoi->EndOfFile.QuadPart = fcb->adsdata.Length;
        fnoi->FileAttributes = fileref->parent->fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fileref->parent->fcb->atts;
    } else {
        fnoi->AllocationSize.QuadPart = fcb_alloc_size(fcb);
        fnoi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fnoi->FileAttributes = fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fcb->atts;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_standard_information(FILE_STANDARD_INFORMATION* fsi, fcb* fcb, file_ref* fileref, LONG* length) {
    RtlZeroMemory(fsi, sizeof(FILE_STANDARD_INFORMATION));

    *length -= sizeof(FILE_STANDARD_INFORMATION);

    if (fcb->ads) {
        if (!fileref || !fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        }

        fsi->AllocationSize.QuadPart = fsi->EndOfFile.QuadPart = fcb->adsdata.Length;
        fsi->NumberOfLinks = fileref->parent->fcb->inode_item.st_nlink;
        fsi->Directory = false;
    } else {
        fsi->AllocationSize.QuadPart = fcb_alloc_size(fcb);
        fsi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fcb->inode_item.st_mode);
    }

    TRACE("length = %I64u\n", fsi->EndOfFile.QuadPart);

    fsi->DeletePending = fileref ? fileref->delete_on_close : false;

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_internal_information(FILE_INTERNAL_INFORMATION* fii, fcb* fcb, LONG* length) {
    *length -= sizeof(FILE_INTERNAL_INFORMATION);

    fii->IndexNumber.QuadPart = make_file_id(fcb->subvol, fcb->inode);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_ea_information(FILE_EA_INFORMATION* eai, fcb* fcb, LONG* length) {
    *length -= sizeof(FILE_EA_INFORMATION);

    /* This value appears to be the size of the structure NTFS stores on disk, and not,
     * as might be expected, the size of FILE_FULL_EA_INFORMATION (which is what we store).
     * The formula is 4 bytes as a header, followed by 5 + NameLength + ValueLength for each
     * item. */

    eai->EaSize = fcb->ealen;

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_position_information(FILE_POSITION_INFORMATION* fpi, PFILE_OBJECT FileObject, LONG* length) {
    RtlZeroMemory(fpi, sizeof(FILE_POSITION_INFORMATION));

    *length -= sizeof(FILE_POSITION_INFORMATION);

    fpi->CurrentByteOffset = FileObject->CurrentByteOffset;

    return STATUS_SUCCESS;
}

NTSTATUS fileref_get_filename(file_ref* fileref, PUNICODE_STRING fn, USHORT* name_offset, ULONG* preqlen) {
    file_ref* fr;
    NTSTATUS Status;
    ULONG reqlen = 0;
    USHORT offset;
    bool overflow = false;

    // FIXME - we need a lock on filerefs' filepart

    if (fileref == fileref->fcb->Vcb->root_fileref) {
        if (fn->MaximumLength >= sizeof(WCHAR)) {
            fn->Buffer[0] = '\\';
            fn->Length = sizeof(WCHAR);

            if (name_offset)
                *name_offset = 0;

            return STATUS_SUCCESS;
        } else {
            if (preqlen)
                *preqlen = sizeof(WCHAR);
            fn->Length = 0;

            return STATUS_BUFFER_OVERFLOW;
        }
    }

    fr = fileref;
    offset = 0;

    while (fr->parent) {
        USHORT movelen;

        if (!fr->dc)
            return STATUS_INTERNAL_ERROR;

        if (!overflow) {
            if (fr->dc->name.Length + sizeof(WCHAR) + fn->Length > fn->MaximumLength)
                overflow = true;
        }

        if (overflow)
            movelen = fn->MaximumLength - fr->dc->name.Length - sizeof(WCHAR);
        else
            movelen = fn->Length;

        if ((!overflow || fn->MaximumLength > fr->dc->name.Length + sizeof(WCHAR)) && movelen > 0) {
            RtlMoveMemory(&fn->Buffer[(fr->dc->name.Length / sizeof(WCHAR)) + 1], fn->Buffer, movelen);
            offset += fr->dc->name.Length + sizeof(WCHAR);
        }

        if (fn->MaximumLength >= sizeof(WCHAR)) {
            fn->Buffer[0] = fr->fcb->ads ? ':' : '\\';
            fn->Length += sizeof(WCHAR);

            if (fn->MaximumLength > sizeof(WCHAR)) {
                RtlCopyMemory(&fn->Buffer[1], fr->dc->name.Buffer, min(fr->dc->name.Length, fn->MaximumLength - sizeof(WCHAR)));
                fn->Length += fr->dc->name.Length;
            }

            if (fn->Length > fn->MaximumLength) {
                fn->Length = fn->MaximumLength;
                overflow = true;
            }
        }

        reqlen += sizeof(WCHAR) + fr->dc->name.Length;

        fr = fr->parent;
    }

    offset += sizeof(WCHAR);

    if (overflow) {
        if (preqlen)
            *preqlen = reqlen;
        Status = STATUS_BUFFER_OVERFLOW;
    } else {
        if (name_offset)
            *name_offset = offset;

        Status = STATUS_SUCCESS;
    }

    return Status;
}

static NTSTATUS fill_in_file_name_information(FILE_NAME_INFORMATION* fni, fcb* fcb, file_ref* fileref, LONG* length) {
    ULONG reqlen;
    UNICODE_STRING fn;
    NTSTATUS Status;
    static const WCHAR datasuf[] = {':','$','D','A','T','A',0};
    uint16_t datasuflen = sizeof(datasuf) - sizeof(WCHAR);

    if (!fileref) {
        ERR("called without fileref\n");
        return STATUS_INVALID_PARAMETER;
    }

    *length -= (LONG)offsetof(FILE_NAME_INFORMATION, FileName[0]);

    TRACE("maximum length is %li\n", *length);
    fni->FileNameLength = 0;

    fni->FileName[0] = 0;

    fn.Buffer = fni->FileName;
    fn.Length = 0;
    fn.MaximumLength = (uint16_t)*length;

    Status = fileref_get_filename(fileref, &fn, NULL, &reqlen);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("fileref_get_filename returned %08lx\n", Status);
        return Status;
    }

    if (fcb->ads) {
        if (Status == STATUS_BUFFER_OVERFLOW)
            reqlen += datasuflen;
        else {
            if (fn.Length + datasuflen > fn.MaximumLength) {
                RtlCopyMemory(&fn.Buffer[fn.Length / sizeof(WCHAR)], datasuf, fn.MaximumLength - fn.Length);
                reqlen += datasuflen;
                Status = STATUS_BUFFER_OVERFLOW;
            } else {
                RtlCopyMemory(&fn.Buffer[fn.Length / sizeof(WCHAR)], datasuf, datasuflen);
                fn.Length += datasuflen;
            }
        }
    }

    if (Status == STATUS_BUFFER_OVERFLOW) {
        *length = -1;
        fni->FileNameLength = reqlen;
        TRACE("%.*S (truncated)\n", (int)(fn.Length / sizeof(WCHAR)), fn.Buffer);
    } else {
        *length -= fn.Length;
        fni->FileNameLength = fn.Length;
        TRACE("%.*S\n", (int)(fn.Length / sizeof(WCHAR)), fn.Buffer);
    }

    return Status;
}

static NTSTATUS fill_in_file_attribute_information(FILE_ATTRIBUTE_TAG_INFORMATION* ati, fcb* fcb, ccb* ccb, LONG* length) {
    *length -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);

    if (fcb->ads) {
        if (!ccb->fileref || !ccb->fileref->parent) {
            ERR("no fileref for stream\n");
            return STATUS_INTERNAL_ERROR;
        }

        ati->FileAttributes = ccb->fileref->parent->fcb->atts;
    } else
        ati->FileAttributes = fcb->atts;

    if (!(ati->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        ati->ReparseTag = 0;
    else
        ati->ReparseTag = get_reparse_tag_fcb(fcb);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_stream_information(FILE_STREAM_INFORMATION* fsi, file_ref* fileref, LONG* length) {
    LONG reqsize;
    LIST_ENTRY* le;
    FILE_STREAM_INFORMATION *entry, *lastentry;
    NTSTATUS Status;

    static const WCHAR datasuf[] = L":$DATA";
    UNICODE_STRING suf;

    if (!fileref) {
        ERR("fileref was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    suf.Buffer = (WCHAR*)datasuf;
    suf.Length = suf.MaximumLength = sizeof(datasuf) - sizeof(WCHAR);

    if (fileref->fcb->type != BTRFS_TYPE_DIRECTORY)
        reqsize = sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR);
    else
        reqsize = 0;

    ExAcquireResourceSharedLite(&fileref->fcb->nonpaged->dir_children_lock, true);

    le = fileref->fcb->dir_children_index.Flink;
    while (le != &fileref->fcb->dir_children_index) {
        dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_index);

        if (dc->index == 0) {
            reqsize = (ULONG)sector_align(reqsize, sizeof(LONGLONG));
            reqsize += sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + dc->name.Length;
        } else
            break;

        le = le->Flink;
    }

    TRACE("length = %li, reqsize = %lu\n", *length, reqsize);

    if (reqsize > *length) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto end;
    }

    entry = fsi;
    lastentry = NULL;

    if (fileref->fcb->type != BTRFS_TYPE_DIRECTORY) {
        ULONG off;

        entry->NextEntryOffset = 0;
        entry->StreamNameLength = suf.Length + sizeof(WCHAR);
        entry->StreamSize.QuadPart = fileref->fcb->inode_item.st_size;
        entry->StreamAllocationSize.QuadPart = fcb_alloc_size(fileref->fcb);

        entry->StreamName[0] = ':';
        RtlCopyMemory(&entry->StreamName[1], suf.Buffer, suf.Length);

        off = (ULONG)sector_align(sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR), sizeof(LONGLONG));

        lastentry = entry;
        entry = (FILE_STREAM_INFORMATION*)((uint8_t*)entry + off);
    }

    le = fileref->fcb->dir_children_index.Flink;
    while (le != &fileref->fcb->dir_children_index) {
        dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_index);

        if (dc->index == 0) {
            ULONG off;

            entry->NextEntryOffset = 0;
            entry->StreamNameLength = dc->name.Length + suf.Length + sizeof(WCHAR);

            if (dc->fileref)
                entry->StreamSize.QuadPart = dc->fileref->fcb->adsdata.Length;
            else
                entry->StreamSize.QuadPart = dc->size;

            entry->StreamAllocationSize.QuadPart = entry->StreamSize.QuadPart;

            entry->StreamName[0] = ':';

            RtlCopyMemory(&entry->StreamName[1], dc->name.Buffer, dc->name.Length);
            RtlCopyMemory(&entry->StreamName[1 + (dc->name.Length / sizeof(WCHAR))], suf.Buffer, suf.Length);

            if (lastentry)
                lastentry->NextEntryOffset = (uint32_t)((uint8_t*)entry - (uint8_t*)lastentry);

            off = (ULONG)sector_align(sizeof(FILE_STREAM_INFORMATION) - sizeof(WCHAR) + suf.Length + sizeof(WCHAR) + dc->name.Length, sizeof(LONGLONG));

            lastentry = entry;
            entry = (FILE_STREAM_INFORMATION*)((uint8_t*)entry + off);
        } else
            break;

        le = le->Flink;
    }

    *length -= reqsize;

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&fileref->fcb->nonpaged->dir_children_lock);

    return Status;
}

#ifndef __REACTOS__
static NTSTATUS fill_in_file_standard_link_information(FILE_STANDARD_LINK_INFORMATION* fsli, fcb* fcb, file_ref* fileref, LONG* length) {
    TRACE("FileStandardLinkInformation\n");

    // FIXME - NumberOfAccessibleLinks should subtract open links which have been marked as delete_on_close

    fsli->NumberOfAccessibleLinks = fcb->inode_item.st_nlink;
    fsli->TotalNumberOfLinks = fcb->inode_item.st_nlink;
    fsli->DeletePending = fileref ? fileref->delete_on_close : false;
    fsli->Directory = (!fcb->ads && fcb->type == BTRFS_TYPE_DIRECTORY) ? true : false;

    *length -= sizeof(FILE_STANDARD_LINK_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_hard_link_information(FILE_LINKS_INFORMATION* fli, file_ref* fileref, PIRP Irp, LONG* length) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    LONG bytes_needed;
    FILE_LINK_ENTRY_INFORMATION* feli;
    bool overflow = false;
    fcb* fcb = fileref->fcb;
    ULONG len;

    if (fcb->ads)
        return STATUS_INVALID_PARAMETER;

    if (*length < (LONG)offsetof(FILE_LINKS_INFORMATION, Entry))
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(fli, *length);

    bytes_needed = offsetof(FILE_LINKS_INFORMATION, Entry);
    len = bytes_needed;
    feli = NULL;

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    if (fcb->inode == SUBVOL_ROOT_INODE) {
        ULONG namelen;

        if (fcb == fcb->Vcb->root_fileref->fcb)
            namelen = sizeof(WCHAR);
        else
            namelen = fileref->dc->name.Length;

        bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) - sizeof(WCHAR) + namelen;

        if (bytes_needed > *length)
            overflow = true;

        if (!overflow) {
            feli = &fli->Entry;

            feli->NextEntryOffset = 0;
            feli->ParentFileId = 0; // we use an inode of 0 to mean the parent of a subvolume

            if (fcb == fcb->Vcb->root_fileref->fcb) {
                feli->FileNameLength = 1;
                feli->FileName[0] = '.';
            } else {
                feli->FileNameLength = fileref->dc->name.Length / sizeof(WCHAR);
                RtlCopyMemory(feli->FileName, fileref->dc->name.Buffer, fileref->dc->name.Length);
            }

            fli->EntriesReturned++;

            len = bytes_needed;
        }
    } else {
        ExAcquireResourceExclusiveLite(&fcb->Vcb->fileref_lock, true);

        if (IsListEmpty(&fcb->hardlinks)) {
            if (!fileref->dc) {
                ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) + fileref->dc->name.Length - sizeof(WCHAR);

            if (bytes_needed > *length)
                overflow = true;

            if (!overflow) {
                feli = &fli->Entry;

                feli->NextEntryOffset = 0;
                feli->ParentFileId = fileref->parent->fcb->inode;
                feli->FileNameLength = fileref->dc->name.Length / sizeof(WCHAR);
                RtlCopyMemory(feli->FileName, fileref->dc->name.Buffer, fileref->dc->name.Length);

                fli->EntriesReturned++;

                len = bytes_needed;
            }
        } else {
            le = fcb->hardlinks.Flink;
            while (le != &fcb->hardlinks) {
                hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);
                file_ref* parfr;

                TRACE("parent %I64x, index %I64x, name %.*S\n", hl->parent, hl->index, (int)(hl->name.Length / sizeof(WCHAR)), hl->name.Buffer);

                Status = open_fileref_by_inode(fcb->Vcb, fcb->subvol, hl->parent, &parfr, Irp);

                if (!NT_SUCCESS(Status)) {
                    ERR("open_fileref_by_inode returned %08lx\n", Status);
                } else if (!parfr->deleted) {
                    LIST_ENTRY* le2;
                    bool found = false, deleted = false;
                    UNICODE_STRING* fn = NULL;

                    le2 = parfr->children.Flink;
                    while (le2 != &parfr->children) {
                        file_ref* fr2 = CONTAINING_RECORD(le2, file_ref, list_entry);

                        if (fr2->dc && fr2->dc->index == hl->index) {
                            found = true;
                            deleted = fr2->deleted;

                            if (!deleted)
                                fn = &fr2->dc->name;

                            break;
                        }

                        le2 = le2->Flink;
                    }

                    if (!found)
                        fn = &hl->name;

                    if (!deleted) {
                        TRACE("fn = %.*S (found = %u)\n", (int)(fn->Length / sizeof(WCHAR)), fn->Buffer, found);

                        if (feli)
                            bytes_needed = (LONG)sector_align(bytes_needed, 8);

                        bytes_needed += sizeof(FILE_LINK_ENTRY_INFORMATION) + fn->Length - sizeof(WCHAR);

                        if (bytes_needed > *length)
                            overflow = true;

                        if (!overflow) {
                            if (feli) {
                                feli->NextEntryOffset = (ULONG)sector_align(sizeof(FILE_LINK_ENTRY_INFORMATION) + ((feli->FileNameLength - 1) * sizeof(WCHAR)), 8);
                                feli = (FILE_LINK_ENTRY_INFORMATION*)((uint8_t*)feli + feli->NextEntryOffset);
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

        ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
    }

    fli->BytesNeeded = bytes_needed;

    *length -= len;

    Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

static NTSTATUS fill_in_hard_link_full_id_information(FILE_LINKS_FULL_ID_INFORMATION* flfii, file_ref* fileref, PIRP Irp, LONG* length) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    LONG bytes_needed;
    FILE_LINK_ENTRY_FULL_ID_INFORMATION* flefii;
    bool overflow = false;
    fcb* fcb = fileref->fcb;
    ULONG len;

    if (fcb->ads)
        return STATUS_INVALID_PARAMETER;

    if (*length < (LONG)offsetof(FILE_LINKS_FULL_ID_INFORMATION, Entry))
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(flfii, *length);

    bytes_needed = offsetof(FILE_LINKS_FULL_ID_INFORMATION, Entry);
    len = bytes_needed;
    flefii = NULL;

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    if (fcb->inode == SUBVOL_ROOT_INODE) {
        ULONG namelen;

        if (fcb == fcb->Vcb->root_fileref->fcb)
            namelen = sizeof(WCHAR);
        else
            namelen = fileref->dc->name.Length;

        bytes_needed += offsetof(FILE_LINK_ENTRY_FULL_ID_INFORMATION, FileName[0]) + namelen;

        if (bytes_needed > *length)
            overflow = true;

        if (!overflow) {
            flefii = &flfii->Entry;

            flefii->NextEntryOffset = 0;

            if (fcb == fcb->Vcb->root_fileref->fcb) {
                RtlZeroMemory(&flefii->ParentFileId.Identifier[0], sizeof(FILE_ID_128));
                flefii->FileNameLength = 1;
                flefii->FileName[0] = '.';
            } else {
                RtlCopyMemory(&flefii->ParentFileId.Identifier[0], &fileref->parent->fcb->inode, sizeof(uint64_t));
                RtlCopyMemory(&flefii->ParentFileId.Identifier[sizeof(uint64_t)], &fileref->parent->fcb->subvol->id, sizeof(uint64_t));

                flefii->FileNameLength = fileref->dc->name.Length / sizeof(WCHAR);
                RtlCopyMemory(flefii->FileName, fileref->dc->name.Buffer, fileref->dc->name.Length);
            }

            flfii->EntriesReturned++;

            len = bytes_needed;
        }
    } else {
        ExAcquireResourceExclusiveLite(&fcb->Vcb->fileref_lock, true);

        if (IsListEmpty(&fcb->hardlinks)) {
            bytes_needed += offsetof(FILE_LINK_ENTRY_FULL_ID_INFORMATION, FileName[0]) + fileref->dc->name.Length;

            if (bytes_needed > *length)
                overflow = true;

            if (!overflow) {
                flefii = &flfii->Entry;

                flefii->NextEntryOffset = 0;

                RtlCopyMemory(&flefii->ParentFileId.Identifier[0], &fileref->parent->fcb->inode, sizeof(uint64_t));
                RtlCopyMemory(&flefii->ParentFileId.Identifier[sizeof(uint64_t)], &fileref->parent->fcb->subvol->id, sizeof(uint64_t));

                flefii->FileNameLength = fileref->dc->name.Length / sizeof(WCHAR);
                RtlCopyMemory(flefii->FileName, fileref->dc->name.Buffer, fileref->dc->name.Length);

                flfii->EntriesReturned++;

                len = bytes_needed;
            }
        } else {
            le = fcb->hardlinks.Flink;
            while (le != &fcb->hardlinks) {
                hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);
                file_ref* parfr;

                TRACE("parent %I64x, index %I64x, name %.*S\n", hl->parent, hl->index, (int)(hl->name.Length / sizeof(WCHAR)), hl->name.Buffer);

                Status = open_fileref_by_inode(fcb->Vcb, fcb->subvol, hl->parent, &parfr, Irp);

                if (!NT_SUCCESS(Status)) {
                    ERR("open_fileref_by_inode returned %08lx\n", Status);
                } else if (!parfr->deleted) {
                    LIST_ENTRY* le2;
                    bool found = false, deleted = false;
                    UNICODE_STRING* fn = NULL;

                    le2 = parfr->children.Flink;
                    while (le2 != &parfr->children) {
                        file_ref* fr2 = CONTAINING_RECORD(le2, file_ref, list_entry);

                        if (fr2->dc->index == hl->index) {
                            found = true;
                            deleted = fr2->deleted;

                            if (!deleted)
                                fn = &fr2->dc->name;

                            break;
                        }

                        le2 = le2->Flink;
                    }

                    if (!found)
                        fn = &hl->name;

                    if (!deleted) {
                        TRACE("fn = %.*S (found = %u)\n", (int)(fn->Length / sizeof(WCHAR)), fn->Buffer, found);

                        if (flefii)
                            bytes_needed = (LONG)sector_align(bytes_needed, 8);

                        bytes_needed += offsetof(FILE_LINK_ENTRY_FULL_ID_INFORMATION, FileName[0]) + fn->Length;

                        if (bytes_needed > *length)
                            overflow = true;

                        if (!overflow) {
                            if (flefii) {
                                flefii->NextEntryOffset = (ULONG)sector_align(offsetof(FILE_LINK_ENTRY_FULL_ID_INFORMATION, FileName[0]) + (flefii->FileNameLength * sizeof(WCHAR)), 8);
                                flefii = (FILE_LINK_ENTRY_FULL_ID_INFORMATION*)((uint8_t*)flefii + flefii->NextEntryOffset);
                            } else
                                flefii = &flfii->Entry;

                            flefii->NextEntryOffset = 0;

                            RtlCopyMemory(&flefii->ParentFileId.Identifier[0], &parfr->fcb->inode, sizeof(uint64_t));
                            RtlCopyMemory(&flefii->ParentFileId.Identifier[sizeof(uint64_t)], &parfr->fcb->subvol->id, sizeof(uint64_t));

                            flefii->FileNameLength = fn->Length / sizeof(WCHAR);
                            RtlCopyMemory(flefii->FileName, fn->Buffer, fn->Length);

                            flfii->EntriesReturned++;

                            len = bytes_needed;
                        }
                    }

                    free_fileref(parfr);
                }

                le = le->Flink;
            }
        }

        ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
    }

    flfii->BytesNeeded = bytes_needed;

    *length -= len;

    Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;

    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

static NTSTATUS fill_in_file_id_information(FILE_ID_INFORMATION* fii, fcb* fcb, LONG* length) {
    RtlCopyMemory(&fii->VolumeSerialNumber, &fcb->Vcb->superblock.uuid.uuid[8], sizeof(uint64_t));
    RtlCopyMemory(&fii->FileId.Identifier[0], &fcb->inode, sizeof(uint64_t));
    RtlCopyMemory(&fii->FileId.Identifier[sizeof(uint64_t)], &fcb->subvol->id, sizeof(uint64_t));

    *length -= sizeof(FILE_ID_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_stat_information(FILE_STAT_INFORMATION* fsi, fcb* fcb, ccb* ccb, LONG* length) {
    INODE_ITEM* ii;

    fsi->FileId.QuadPart = make_file_id(fcb->subvol, fcb->inode);

    if (fcb->ads)
        ii = &ccb->fileref->parent->fcb->inode_item;
    else
        ii = &fcb->inode_item;

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fsi->CreationTime = fsi->LastAccessTime = fsi->LastWriteTime = fsi->ChangeTime = time;
    } else {
        fsi->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
        fsi->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
        fsi->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
        fsi->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    }

    if (fcb->ads) {
        fsi->AllocationSize.QuadPart = fsi->EndOfFile.QuadPart = fcb->adsdata.Length;
        fsi->FileAttributes = ccb->fileref->parent->fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : ccb->fileref->parent->fcb->atts;
    } else {
        fsi->AllocationSize.QuadPart = fcb_alloc_size(fcb);
        fsi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsi->FileAttributes = fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fcb->atts;
    }

    if (fcb->type == BTRFS_TYPE_SOCKET)
        fsi->ReparseTag = IO_REPARSE_TAG_AF_UNIX;
    else if (fcb->type == BTRFS_TYPE_FIFO)
        fsi->ReparseTag = IO_REPARSE_TAG_LX_FIFO;
    else if (fcb->type == BTRFS_TYPE_CHARDEV)
        fsi->ReparseTag = IO_REPARSE_TAG_LX_CHR;
    else if (fcb->type == BTRFS_TYPE_BLOCKDEV)
        fsi->ReparseTag = IO_REPARSE_TAG_LX_BLK;
    else if (!(fsi->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        fsi->ReparseTag = 0;
    else
        fsi->ReparseTag = get_reparse_tag_fcb(fcb);

    if (fcb->type == BTRFS_TYPE_SOCKET || fcb->type == BTRFS_TYPE_FIFO || fcb->type == BTRFS_TYPE_CHARDEV || fcb->type == BTRFS_TYPE_BLOCKDEV)
        fsi->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;

    if (fcb->ads)
        fsi->NumberOfLinks = ccb->fileref->parent->fcb->inode_item.st_nlink;
    else
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;

    fsi->EffectiveAccess = ccb->access;

    *length -= sizeof(FILE_STAT_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_stat_lx_information(FILE_STAT_LX_INFORMATION* fsli, fcb* fcb, ccb* ccb, LONG* length) {
    INODE_ITEM* ii;

    fsli->FileId.QuadPart = make_file_id(fcb->subvol, fcb->inode);

    if (fcb->ads)
        ii = &ccb->fileref->parent->fcb->inode_item;
    else
        ii = &fcb->inode_item;

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fsli->CreationTime = fsli->LastAccessTime = fsli->LastWriteTime = fsli->ChangeTime = time;
    } else {
        fsli->CreationTime.QuadPart = unix_time_to_win(&ii->otime);
        fsli->LastAccessTime.QuadPart = unix_time_to_win(&ii->st_atime);
        fsli->LastWriteTime.QuadPart = unix_time_to_win(&ii->st_mtime);
        fsli->ChangeTime.QuadPart = unix_time_to_win(&ii->st_ctime);
    }

    if (fcb->ads) {
        fsli->AllocationSize.QuadPart = fsli->EndOfFile.QuadPart = fcb->adsdata.Length;
        fsli->FileAttributes = ccb->fileref->parent->fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : ccb->fileref->parent->fcb->atts;
    } else {
        fsli->AllocationSize.QuadPart = fcb_alloc_size(fcb);
        fsli->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsli->FileAttributes = fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fcb->atts;
    }

    if (fcb->type == BTRFS_TYPE_SOCKET)
        fsli->ReparseTag = IO_REPARSE_TAG_AF_UNIX;
    else if (fcb->type == BTRFS_TYPE_FIFO)
        fsli->ReparseTag = IO_REPARSE_TAG_LX_FIFO;
    else if (fcb->type == BTRFS_TYPE_CHARDEV)
        fsli->ReparseTag = IO_REPARSE_TAG_LX_CHR;
    else if (fcb->type == BTRFS_TYPE_BLOCKDEV)
        fsli->ReparseTag = IO_REPARSE_TAG_LX_BLK;
    else if (!(fsli->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        fsli->ReparseTag = 0;
    else
        fsli->ReparseTag = get_reparse_tag_fcb(fcb);

    if (fcb->type == BTRFS_TYPE_SOCKET || fcb->type == BTRFS_TYPE_FIFO || fcb->type == BTRFS_TYPE_CHARDEV || fcb->type == BTRFS_TYPE_BLOCKDEV)
        fsli->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;

    if (fcb->ads)
        fsli->NumberOfLinks = ccb->fileref->parent->fcb->inode_item.st_nlink;
    else
        fsli->NumberOfLinks = fcb->inode_item.st_nlink;

    fsli->EffectiveAccess = ccb->access;
    fsli->LxFlags = LX_FILE_METADATA_HAS_UID | LX_FILE_METADATA_HAS_GID | LX_FILE_METADATA_HAS_MODE | LX_FILE_METADATA_HAS_DEVICE_ID;

    if (fcb->case_sensitive)
        fsli->LxFlags |= LX_FILE_CASE_SENSITIVE_DIR;

    fsli->LxUid = ii->st_uid;
    fsli->LxGid = ii->st_gid;
    fsli->LxMode = ii->st_mode;

    if (ii->st_mode & __S_IFBLK || ii->st_mode & __S_IFCHR) {
        fsli->LxDeviceIdMajor = (ii->st_rdev & 0xFFFFFFFFFFF00000) >> 20;
        fsli->LxDeviceIdMinor = (ii->st_rdev & 0xFFFFF);
    } else {
        fsli->LxDeviceIdMajor = 0;
        fsli->LxDeviceIdMinor = 0;
    }

    *length -= sizeof(FILE_STAT_LX_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS fill_in_file_case_sensitive_information(FILE_CASE_SENSITIVE_INFORMATION* fcsi, fcb* fcb, LONG* length) {
    fcsi->Flags = fcb->case_sensitive ? FILE_CS_FLAG_CASE_SENSITIVE_DIR : 0;

    *length -= sizeof(FILE_CASE_SENSITIVE_INFORMATION);

    return STATUS_SUCCESS;
}

#endif // __REACTOS__

static NTSTATUS fill_in_file_compression_information(FILE_COMPRESSION_INFORMATION* fci, LONG* length, fcb* fcb) {
    *length -= sizeof(FILE_COMPRESSION_INFORMATION);

    memset(fci, 0, sizeof(FILE_COMPRESSION_INFORMATION));

    if (fcb->ads)
        fci->CompressedFileSize.QuadPart = fcb->adsdata.Length;
    else if (!S_ISDIR(fcb->inode_item.st_mode))
        fci->CompressedFileSize.QuadPart = fcb->inode_item.st_size;

    return STATUS_SUCCESS;
}

static NTSTATUS query_info(device_extension* Vcb, PFILE_OBJECT FileObject, PIRP Irp) {
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

            // Access, mode, and alignment are all filled in by the kernel

            if (length > 0)
                fill_in_file_basic_information(&fai->BasicInformation, ii, &length, fcb, fileref);

            if (length > 0)
                fill_in_file_standard_information(&fai->StandardInformation, fcb, fileref, &length);

            if (length > 0)
                fill_in_file_internal_information(&fai->InternalInformation, fcb, &length);

            if (length > 0)
                fill_in_file_ea_information(&fai->EaInformation, fcb, &length);

            length -= sizeof(FILE_ACCESS_INFORMATION);

            if (length > 0)
                fill_in_file_position_information(&fai->PositionInformation, FileObject, &length);

            length -= sizeof(FILE_MODE_INFORMATION);

            length -= sizeof(FILE_ALIGNMENT_INFORMATION);

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

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
            Status = fill_in_file_attribute_information(ati, fcb, ccb, &length);
            ExReleaseResourceLite(&Vcb->tree_lock);

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
        {
            FILE_COMPRESSION_INFORMATION* fci = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileCompressionInformation\n");

            Status = fill_in_file_compression_information(fci, &length, fcb);
            break;
        }

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

            Status = fill_in_file_stream_information(fsi, fileref, &length);

            break;
        }

#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileHardLinkInformation:
        {
            FILE_LINKS_INFORMATION* fli = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileHardLinkInformation\n");

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
            Status = fill_in_hard_link_information(fli, fileref, Irp, &length);
            ExReleaseResourceLite(&Vcb->tree_lock);

            break;
        }

        case FileNormalizedNameInformation:
        {
            FILE_NAME_INFORMATION* fni = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileNormalizedNameInformation\n");

            Status = fill_in_file_name_information(fni, fcb, fileref, &length);

            break;
        }

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

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
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

        case FileStatInformation:
        {
            FILE_STAT_INFORMATION* fsi = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_STAT_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }

            TRACE("FileStatInformation\n");

            Status = fill_in_file_stat_information(fsi, fcb, ccb, &length);

            break;
        }

        case FileStatLxInformation:
        {
            FILE_STAT_LX_INFORMATION* fsli = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_STAT_LX_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }

            TRACE("FileStatLxInformation\n");

            Status = fill_in_file_stat_lx_information(fsli, fcb, ccb, &length);

            break;
        }

        case FileCaseSensitiveInformation:
        {
            FILE_CASE_SENSITIVE_INFORMATION* fcsi = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_CASE_SENSITIVE_INFORMATION)) {
                WARN("overflow\n");
                Status = STATUS_BUFFER_OVERFLOW;
                goto exit;
            }

            TRACE("FileCaseSensitiveInformation\n");

            Status = fill_in_file_case_sensitive_information(fcsi, fcb, &length);

            break;
        }

        case FileHardLinkFullIdInformation:
        {
            FILE_LINKS_FULL_ID_INFORMATION* flfii = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileHardLinkFullIdInformation\n");

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
            Status = fill_in_hard_link_full_id_information(flfii, fileref, Irp, &length);
            ExReleaseResourceLite(&Vcb->tree_lock);

            break;
        }
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
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
    TRACE("query_info returning %08lx\n", Status);

    return Status;
}

_Dispatch_type_(IRP_MJ_QUERY_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    fcb* fcb;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Irp->IoStatus.Information = 0;

    TRACE("query information\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    fcb = IrpSp->FileObject->FsContext;
    TRACE("fcb = %p\n", fcb);
    TRACE("fcb->subvol = %p\n", fcb->subvol);

    Status = query_info(fcb->Vcb, IrpSp->FileObject, Irp);

end:
    TRACE("returning %08lx\n", Status);

    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

_Dispatch_type_(IRP_MJ_QUERY_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    bool top_level;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb;
    ccb* ccb;
    FILE_FULL_EA_INFORMATION* ffei;
    ULONG retlen = 0;

    FsRtlEnterFileSystem();

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ffei = map_user_buffer(Irp, NormalPagePriority);
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

    if (fcb->ads)
        fcb = ccb->fileref->parent->fcb;

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    if (fcb->ea_xattr.Length == 0) {
        Status = STATUS_NO_EAS_ON_FILE;
        goto end2;
    }

    Status = STATUS_SUCCESS;

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

            in = (FILE_GET_EA_INFORMATION*)(((uint8_t*)in) + in->NextEntryOffset);
        } while (true);

        ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;
        out = NULL;

        do {
            bool found = false;

            in = IrpSp->Parameters.QueryEa.EaList;
            do {
                if (in->EaNameLength == ea->EaNameLength &&
                    RtlCompareMemory(in->EaName, ea->EaName, in->EaNameLength) == in->EaNameLength) {
                    found = true;
                    break;
                }

                if (in->NextEntryOffset == 0)
                    break;

                in = (FILE_GET_EA_INFORMATION*)(((uint8_t*)in) + in->NextEntryOffset);
            } while (true);

            if (found) {
                uint8_t padding = retlen % 4 > 0 ? (4 - (retlen % 4)) : 0;

                if (offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength > IrpSp->Parameters.QueryEa.Length - retlen - padding) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    retlen = 0;
                    goto end2;
                }

                retlen += padding;

                if (out) {
                    out->NextEntryOffset = (ULONG)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + out->EaNameLength + 1 + out->EaValueLength + padding;
                    out = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)out) + out->NextEntryOffset);
                } else
                    out = ffei;

                out->NextEntryOffset = 0;
                out->Flags = ea->Flags;
                out->EaNameLength = ea->EaNameLength;
                out->EaValueLength = ea->EaValueLength;
                RtlCopyMemory(out->EaName, ea->EaName, ea->EaNameLength + ea->EaValueLength + 1);

                retlen += (ULONG)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength;

                if (IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
                    break;
            }

            if (ea->NextEntryOffset == 0)
                break;

            ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
        } while (true);
    } else {
        FILE_FULL_EA_INFORMATION *ea, *out;
        ULONG index;

        if (IrpSp->Flags & SL_INDEX_SPECIFIED) {
            // The index is 1-based
            if (IrpSp->Parameters.QueryEa.EaIndex == 0) {
                Status = STATUS_NONEXISTENT_EA_ENTRY;
                goto end2;
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
                if (ea->NextEntryOffset == 0) { // last item
                    Status = STATUS_NO_MORE_EAS;
                    goto end2;
                }

                ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
            }
        }

        out = NULL;

        do {
            uint8_t padding = retlen % 4 > 0 ? (4 - (retlen % 4)) : 0;

            if (offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength > IrpSp->Parameters.QueryEa.Length - retlen - padding) {
                Status = retlen == 0 ? STATUS_BUFFER_TOO_SMALL : STATUS_BUFFER_OVERFLOW;
                goto end2;
            }

            retlen += padding;

            if (out) {
                out->NextEntryOffset = (ULONG)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + out->EaNameLength + 1 + out->EaValueLength + padding;
                out = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)out) + out->NextEntryOffset);
            } else
                out = ffei;

            out->NextEntryOffset = 0;
            out->Flags = ea->Flags;
            out->EaNameLength = ea->EaNameLength;
            out->EaValueLength = ea->EaValueLength;
            RtlCopyMemory(out->EaName, ea->EaName, ea->EaNameLength + ea->EaValueLength + 1);

            retlen += (ULONG)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + 1 + ea->EaValueLength;

            if (!(IrpSp->Flags & SL_INDEX_SPECIFIED))
                ccb->ea_index++;

            if (ea->NextEntryOffset == 0 || IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
                break;

            ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
        } while (true);
    }

end2:
    ExReleaseResourceLite(fcb->Header.Resource);

end:
    TRACE("returning %08lx\n", Status);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW ? retlen : 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

_Dispatch_type_(IRP_MJ_SET_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    bool top_level;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    FILE_FULL_EA_INFORMATION* ffei;
    ULONG offset;
    LIST_ENTRY ealist;
    ea_item* item;
    FILE_FULL_EA_INFORMATION* ea;
    LIST_ENTRY* le;
    LARGE_INTEGER time;
    BTRFS_TIME now;

    FsRtlEnterFileSystem();

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }

    ffei = map_user_buffer(Irp, NormalPagePriority);
    if (!ffei) {
        ERR("could not get output buffer\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = IoCheckEaBufferValidity(ffei, IrpSp->Parameters.SetEa.Length, &offset);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCheckEaBufferValidity returned %08lx (error at offset %lu)\n", Status, offset);
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

    if (fcb->ads) {
        fileref = ccb->fileref->parent;
        fcb = fileref->fcb;
    } else
        fileref = ccb->fileref;

    InitializeListHead(&ealist);

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

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

            ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
        } while (true);
    }

    ea = ffei;

    do {
        STRING s;
        bool found = false;

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
                found = true;
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

        ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
    } while (true);

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

    // handle LXSS values
    le = ealist.Flink;
    while (le != &ealist) {
        LIST_ENTRY* le2 = le->Flink;

        item = CONTAINING_RECORD(le, ea_item, list_entry);

        if (item->name.Length == sizeof(lxuid) - 1 && RtlCompareMemory(item->name.Buffer, lxuid, item->name.Length) == item->name.Length) {
            if (item->value.Length < sizeof(uint32_t)) {
                ERR("uid value was shorter than expected\n");
                Status = STATUS_INVALID_PARAMETER;
                goto end2;
            }

            if (Irp->RequestorMode == KernelMode || ccb->access & FILE_WRITE_ATTRIBUTES) {
                RtlCopyMemory(&fcb->inode_item.st_uid, item->value.Buffer, sizeof(uint32_t));
                fcb->sd_dirty = true;
                fcb->sd_deleted = false;
            }

            RemoveEntryList(&item->list_entry);
            ExFreePool(item);
        } else if (item->name.Length == sizeof(lxgid) - 1 && RtlCompareMemory(item->name.Buffer, lxgid, item->name.Length) == item->name.Length) {
            if (item->value.Length < sizeof(uint32_t)) {
                ERR("gid value was shorter than expected\n");
                Status = STATUS_INVALID_PARAMETER;
                goto end2;
            }

            if (Irp->RequestorMode == KernelMode || ccb->access & FILE_WRITE_ATTRIBUTES)
                RtlCopyMemory(&fcb->inode_item.st_gid, item->value.Buffer, sizeof(uint32_t));

            RemoveEntryList(&item->list_entry);
            ExFreePool(item);
        } else if (item->name.Length == sizeof(lxmod) - 1 && RtlCompareMemory(item->name.Buffer, lxmod, item->name.Length) == item->name.Length) {
            if (item->value.Length < sizeof(uint32_t)) {
                ERR("mode value was shorter than expected\n");
                Status = STATUS_INVALID_PARAMETER;
                goto end2;
            }

            if (Irp->RequestorMode == KernelMode || ccb->access & FILE_WRITE_ATTRIBUTES) {
                uint32_t allowed = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_ISGID | S_ISVTX | S_ISUID;
                uint32_t val;

                RtlCopyMemory(&val, item->value.Buffer, sizeof(uint32_t));

                fcb->inode_item.st_mode &= ~allowed;
                fcb->inode_item.st_mode |= val & allowed;
            }

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
        uint16_t size = 0;
        char *buf, *oldbuf;

        le = ealist.Flink;
        while (le != &ealist) {
            item = CONTAINING_RECORD(le, ea_item, list_entry);

            if (size % 4 > 0)
                size += 4 - (size % 4);

            size += (uint16_t)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + item->name.Length + 1 + item->value.Length;

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
                ea->NextEntryOffset = (ULONG)offsetof(FILE_FULL_EA_INFORMATION, EaName[0]) + ea->EaNameLength + ea->EaValueLength;

                if (ea->NextEntryOffset % 4 > 0)
                    ea->NextEntryOffset += 4 - (ea->NextEntryOffset % 4);

                ea = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)ea) + ea->NextEntryOffset);
            } else
                ea = (FILE_FULL_EA_INFORMATION*)fcb->ea_xattr.Buffer;

            ea->NextEntryOffset = 0;
            ea->Flags = item->flags;
            ea->EaNameLength = (UCHAR)item->name.Length;
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

    fcb->ea_changed = true;

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.sequence++;

    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    send_notification_fileref(fileref, FILE_NOTIFY_CHANGE_EA, FILE_ACTION_MODIFIED, NULL);

    Status = STATUS_SUCCESS;

end2:
    ExReleaseResourceLite(fcb->Header.Resource);

    while (!IsListEmpty(&ealist)) {
        le = RemoveHeadList(&ealist);

        item = CONTAINING_RECORD(le, ea_item, list_entry);

        ExFreePool(item);
    }

end:
    TRACE("returning %08lx\n", Status);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
