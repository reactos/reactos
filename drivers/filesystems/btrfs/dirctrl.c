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

#ifndef __REACTOS__
// not currently in mingw
#ifndef _MSC_VER
#define FileIdExtdDirectoryInformation (enum _FILE_INFORMATION_CLASS)60
#define FileIdExtdBothDirectoryInformation (enum _FILE_INFORMATION_CLASS)63

typedef struct _FILE_ID_EXTD_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    WCHAR FileName[1];
} FILE_ID_EXTD_DIR_INFORMATION, *PFILE_ID_EXTD_DIR_INFORMATION;

typedef struct _FILE_ID_EXTD_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_ID_EXTD_BOTH_DIR_INFORMATION, *PFILE_ID_EXTD_BOTH_DIR_INFORMATION;

#endif
#else
#define FileIdExtdDirectoryInformation (enum _FILE_INFORMATION_CLASS)60
#define FileIdExtdBothDirectoryInformation (enum _FILE_INFORMATION_CLASS)63
#endif // __REACTOS__

enum DirEntryType {
    DirEntryType_File,
    DirEntryType_Self,
    DirEntryType_Parent
};

typedef struct {
    KEY key;
    UNICODE_STRING name;
    uint8_t type;
    enum DirEntryType dir_entry_type;
    dir_child* dc;
} dir_entry;

ULONG get_reparse_tag_fcb(fcb* fcb) {
    ULONG tag;

    if (fcb->type == BTRFS_TYPE_SYMLINK)
        return IO_REPARSE_TAG_SYMLINK;
    else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length < sizeof(ULONG))
            return 0;

        RtlCopyMemory(&tag, fcb->reparse_xattr.Buffer, sizeof(ULONG));
    } else {
        NTSTATUS Status;
        ULONG br;

        Status = read_file(fcb, (uint8_t*)&tag, 0, sizeof(ULONG), &br, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08lx\n", Status);
            return 0;
        }
    }

    return tag;
}

ULONG get_reparse_tag(device_extension* Vcb, root* subvol, uint64_t inode, uint8_t type, ULONG atts, bool lxss, PIRP Irp) {
    fcb* fcb;
    ULONG tag = 0;
    NTSTATUS Status;

    if (type == BTRFS_TYPE_SYMLINK)
        return IO_REPARSE_TAG_SYMLINK;
    else if (lxss) {
        if (type == BTRFS_TYPE_SOCKET)
            return IO_REPARSE_TAG_AF_UNIX;
        else if (type == BTRFS_TYPE_FIFO)
            return IO_REPARSE_TAG_LX_FIFO;
        else if (type == BTRFS_TYPE_CHARDEV)
            return IO_REPARSE_TAG_LX_CHR;
        else if (type == BTRFS_TYPE_BLOCKDEV)
            return IO_REPARSE_TAG_LX_BLK;
    }

    if (type != BTRFS_TYPE_FILE && type != BTRFS_TYPE_DIRECTORY)
        return 0;

    if (!(atts & FILE_ATTRIBUTE_REPARSE_POINT))
        return 0;

    Status = open_fcb(Vcb, subvol, inode, type, NULL, false, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08lx\n", Status);
        return 0;
    }

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    tag = get_reparse_tag_fcb(fcb);

    ExReleaseResourceLite(fcb->Header.Resource);

    free_fcb(fcb);

    return tag;
}

static ULONG get_ea_len(device_extension* Vcb, root* subvol, uint64_t inode, PIRP Irp) {
    uint8_t* eadata;
    uint16_t len;

    if (get_xattr(Vcb, subvol, inode, EA_EA, EA_EA_HASH, &eadata, &len, Irp)) {
        ULONG offset;
        NTSTATUS Status;

        Status = IoCheckEaBufferValidity((FILE_FULL_EA_INFORMATION*)eadata, len, &offset);

        if (!NT_SUCCESS(Status)) {
            WARN("IoCheckEaBufferValidity returned %08lx (error at offset %lu)\n", Status, offset);
            ExFreePool(eadata);
            return 0;
        } else {
            FILE_FULL_EA_INFORMATION* eainfo;
            ULONG ealen;

            ealen = 4;
            eainfo = (FILE_FULL_EA_INFORMATION*)eadata;
            do {
                ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;

                if (eainfo->NextEntryOffset == 0)
                    break;

                eainfo = (FILE_FULL_EA_INFORMATION*)(((uint8_t*)eainfo) + eainfo->NextEntryOffset);
            } while (true);

            ExFreePool(eadata);

            return ealen;
        }
    } else
        return 0;
}

static NTSTATUS query_dir_item(fcb* fcb, ccb* ccb, void* buf, LONG* len, PIRP Irp, dir_entry* de, root* r) {
    PIO_STACK_LOCATION IrpSp;
    LONG needed;
    uint64_t inode;
    INODE_ITEM ii;
    NTSTATUS Status;
    ULONG atts = 0, ealen = 0;
    file_ref* fileref = ccb->fileref;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (de->key.obj_type == TYPE_ROOT_ITEM) { // subvol
        LIST_ENTRY* le;

        r = NULL;

        le = fcb->Vcb->roots.Flink;
        while (le != &fcb->Vcb->roots) {
            root* subvol = CONTAINING_RECORD(le, root, list_entry);

            if (subvol->id == de->key.obj_id) {
                r = subvol;
                break;
            }

            le = le->Flink;
        }

        if (r && r->parent != fcb->subvol->id && (!de->dc || !de->dc->root_dir))
            r = NULL;

        inode = SUBVOL_ROOT_INODE;
    } else {
        inode = de->key.obj_id;
    }

    if (IrpSp->Parameters.QueryDirectory.FileInformationClass != FileNamesInformation) { // FIXME - object ID and reparse point classes too?
        switch (de->dir_entry_type) {
            case DirEntryType_File:
            {
                if (!r) {
                    LARGE_INTEGER time;

                    ii = fcb->Vcb->dummy_fcb->inode_item;
                    atts = fcb->Vcb->dummy_fcb->atts;
                    ealen = fcb->Vcb->dummy_fcb->ealen;

                    KeQuerySystemTime(&time);
                    win_time_to_unix(time, &ii.otime);
                    ii.st_atime = ii.st_mtime = ii.st_ctime = ii.otime;
                } else {
                    bool found = false;

                    if (de->dc && de->dc->fileref && de->dc->fileref->fcb) {
                        ii = de->dc->fileref->fcb->inode_item;
                        atts = de->dc->fileref->fcb->atts;
                        ealen = de->dc->fileref->fcb->ealen;
                        found = true;
                    }

                    if (!found) {
                        KEY searchkey;
                        traverse_ptr tp;

                        searchkey.obj_id = inode;
                        searchkey.obj_type = TYPE_INODE_ITEM;
                        searchkey.offset = 0xffffffffffffffff;

                        Status = find_item(fcb->Vcb, r, &tp, &searchkey, false, Irp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("error - find_item returned %08lx\n", Status);
                            return Status;
                        }

                        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                            ERR("could not find inode item for inode %I64x in root %I64x\n", inode, r->id);
                            return STATUS_INTERNAL_ERROR;
                        }

                        RtlZeroMemory(&ii, sizeof(INODE_ITEM));

                        if (tp.item->size > 0)
                            RtlCopyMemory(&ii, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));

                        if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdFullDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdExtdDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdExtdBothDirectoryInformation) {

                            bool dotfile = de->name.Length > sizeof(WCHAR) && de->name.Buffer[0] == '.';

                            atts = get_file_attributes(fcb->Vcb, r, inode, de->type, dotfile, false, Irp);
                        }

                        if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdFullDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdExtdDirectoryInformation ||
                            IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdExtdBothDirectoryInformation) {
                            ealen = get_ea_len(fcb->Vcb, r, inode, Irp);
                        }
                    }
                }

                break;
            }

            case DirEntryType_Self:
                ii = fcb->inode_item;
                r = fcb->subvol;
                inode = fcb->inode;
                atts = fcb->atts;
                ealen = fcb->ealen;
                break;

            case DirEntryType_Parent:
                if (fileref && fileref->parent) {
                    ii = fileref->parent->fcb->inode_item;
                    r = fileref->parent->fcb->subvol;
                    inode = fileref->parent->fcb->inode;
                    atts = fileref->parent->fcb->atts;
                    ealen = fileref->parent->fcb->ealen;
                } else {
                    ERR("no fileref\n");
                    return STATUS_INTERNAL_ERROR;
                }
                break;
        }

        if (atts == 0)
            atts = FILE_ATTRIBUTE_NORMAL;
    }

    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
        case FileBothDirectoryInformation:
        {
            FILE_BOTH_DIR_INFORMATION* fbdi = buf;

            TRACE("FileBothDirectoryInformation\n");

            needed = offsetof(FILE_BOTH_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fbdi->NextEntryOffset = 0;
            fbdi->FileIndex = 0;
            fbdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fbdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fbdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fbdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fbdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fbdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fbdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fbdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fbdi->FileAttributes = atts;
            fbdi->FileNameLength = de->name.Length;
            fbdi->EaSize = (r && atts & FILE_ATTRIBUTE_REPARSE_POINT) ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp) : ealen;
            fbdi->ShortNameLength = 0;

            RtlCopyMemory(fbdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        case FileDirectoryInformation:
        {
            FILE_DIRECTORY_INFORMATION* fdi = buf;

            TRACE("FileDirectoryInformation\n");

            needed = offsetof(FILE_DIRECTORY_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fdi->NextEntryOffset = 0;
            fdi->FileIndex = 0;
            fdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fdi->FileAttributes = atts;
            fdi->FileNameLength = de->name.Length;

            RtlCopyMemory(fdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        case FileFullDirectoryInformation:
        {
            FILE_FULL_DIR_INFORMATION* ffdi = buf;

            TRACE("FileFullDirectoryInformation\n");

            needed = offsetof(FILE_FULL_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            ffdi->NextEntryOffset = 0;
            ffdi->FileIndex = 0;
            ffdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            ffdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            ffdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            ffdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            ffdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                ffdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                ffdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                ffdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            ffdi->FileAttributes = atts;
            ffdi->FileNameLength = de->name.Length;
            ffdi->EaSize = (r && atts & FILE_ATTRIBUTE_REPARSE_POINT) ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp) : ealen;

            RtlCopyMemory(ffdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        case FileIdBothDirectoryInformation:
        {
            FILE_ID_BOTH_DIR_INFORMATION* fibdi = buf;

            TRACE("FileIdBothDirectoryInformation\n");

            needed = offsetof(FILE_ID_BOTH_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fibdi->NextEntryOffset = 0;
            fibdi->FileIndex = 0;
            fibdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fibdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fibdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fibdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fibdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fibdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fibdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fibdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fibdi->FileAttributes = atts;
            fibdi->FileNameLength = de->name.Length;
            fibdi->EaSize = (r && atts & FILE_ATTRIBUTE_REPARSE_POINT) ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp) : ealen;
            fibdi->ShortNameLength = 0;
            fibdi->FileId.QuadPart = r ? make_file_id(r, inode) : make_file_id(fcb->Vcb->dummy_fcb->subvol, fcb->Vcb->dummy_fcb->inode);

            RtlCopyMemory(fibdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        case FileIdFullDirectoryInformation:
        {
            FILE_ID_FULL_DIR_INFORMATION* fifdi = buf;

            TRACE("FileIdFullDirectoryInformation\n");

            needed = offsetof(FILE_ID_FULL_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fifdi->NextEntryOffset = 0;
            fifdi->FileIndex = 0;
            fifdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fifdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fifdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fifdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fifdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fifdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fifdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fifdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fifdi->FileAttributes = atts;
            fifdi->FileNameLength = de->name.Length;
            fifdi->EaSize = (r && atts & FILE_ATTRIBUTE_REPARSE_POINT) ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp) : ealen;
            fifdi->FileId.QuadPart = r ? make_file_id(r, inode) : make_file_id(fcb->Vcb->dummy_fcb->subvol, fcb->Vcb->dummy_fcb->inode);

            RtlCopyMemory(fifdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileIdExtdDirectoryInformation:
        {
            FILE_ID_EXTD_DIR_INFORMATION* fiedi = buf;

            TRACE("FileIdExtdDirectoryInformation\n");

            needed = offsetof(FILE_ID_EXTD_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fiedi->NextEntryOffset = 0;
            fiedi->FileIndex = 0;
            fiedi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fiedi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fiedi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fiedi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fiedi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fiedi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fiedi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fiedi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fiedi->FileAttributes = atts;
            fiedi->FileNameLength = de->name.Length;
            fiedi->EaSize = ealen;
            fiedi->ReparsePointTag = get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp);

            RtlCopyMemory(&fiedi->FileId.Identifier[0], &fcb->inode, sizeof(uint64_t));
            RtlCopyMemory(&fiedi->FileId.Identifier[sizeof(uint64_t)], &fcb->subvol->id, sizeof(uint64_t));

            RtlCopyMemory(fiedi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        case FileIdExtdBothDirectoryInformation:
        {
            FILE_ID_EXTD_BOTH_DIR_INFORMATION* fiebdi = buf;

            TRACE("FileIdExtdBothDirectoryInformation\n");

            needed = offsetof(FILE_ID_EXTD_BOTH_DIR_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fiebdi->NextEntryOffset = 0;
            fiebdi->FileIndex = 0;
            fiebdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fiebdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fiebdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fiebdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fiebdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;

            if (de->type == BTRFS_TYPE_SYMLINK)
                fiebdi->AllocationSize.QuadPart = 0;
            else if (atts & FILE_ATTRIBUTE_SPARSE_FILE)
                fiebdi->AllocationSize.QuadPart = ii.st_blocks;
            else
                fiebdi->AllocationSize.QuadPart = sector_align(ii.st_size, fcb->Vcb->superblock.sector_size);

            fiebdi->FileAttributes = atts;
            fiebdi->FileNameLength = de->name.Length;
            fiebdi->EaSize = ealen;
            fiebdi->ReparsePointTag = get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, ccb->lxss, Irp);

            RtlCopyMemory(&fiebdi->FileId.Identifier[0], &fcb->inode, sizeof(uint64_t));
            RtlCopyMemory(&fiebdi->FileId.Identifier[sizeof(uint64_t)], &fcb->subvol->id, sizeof(uint64_t));

            fiebdi->ShortNameLength = 0;

            RtlCopyMemory(fiebdi->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }
#endif

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

        case FileNamesInformation:
        {
            FILE_NAMES_INFORMATION* fni = buf;

            TRACE("FileNamesInformation\n");

            needed = offsetof(FILE_NAMES_INFORMATION, FileName) + de->name.Length;

            if (needed > *len) {
                TRACE("buffer overflow - %li > %lu\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }

            fni->NextEntryOffset = 0;
            fni->FileIndex = 0;
            fni->FileNameLength = de->name.Length;

            RtlCopyMemory(fni->FileName, de->name.Buffer, de->name.Length);

            *len -= needed;

            return STATUS_SUCCESS;
        }

        default:
            WARN("Unknown FileInformationClass %u\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
            return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_NO_MORE_FILES;
}

static NTSTATUS next_dir_entry(file_ref* fileref, uint64_t* offset, dir_entry* de, dir_child** pdc) {
    LIST_ENTRY* le;
    dir_child* dc;

    if (*pdc) {
        dir_child* dc2 = *pdc;

        if (dc2->list_entry_index.Flink != &fileref->fcb->dir_children_index)
            dc = CONTAINING_RECORD(dc2->list_entry_index.Flink, dir_child, list_entry_index);
        else
            dc = NULL;

        goto next;
    }

    if (fileref->parent) { // don't return . and .. if root directory
        if (*offset == 0) {
            de->key.obj_id = fileref->fcb->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Self;
            de->name.Buffer = L".";
            de->name.Length = de->name.MaximumLength = sizeof(WCHAR);
            de->type = BTRFS_TYPE_DIRECTORY;

            *offset = 1;
            *pdc = NULL;

            return STATUS_SUCCESS;
        } else if (*offset == 1) {
            de->key.obj_id = fileref->parent->fcb->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Parent;
            de->name.Buffer = L"..";
            de->name.Length = de->name.MaximumLength = sizeof(WCHAR) * 2;
            de->type = BTRFS_TYPE_DIRECTORY;

            *offset = 2;
            *pdc = NULL;

            return STATUS_SUCCESS;
        }
    }

    if (*offset < 2)
        *offset = 2;

    dc = NULL;
    le = fileref->fcb->dir_children_index.Flink;

    // skip entries before offset
    while (le != &fileref->fcb->dir_children_index) {
        dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_index);

        if (dc2->index >= *offset) {
            dc = dc2;
            break;
        }

        le = le->Flink;
    }

next:
    if (!dc)
        return STATUS_NO_MORE_FILES;

    if (dc->root_dir && fileref->parent) { // hide $Root dir unless in apparent root, to avoid recursion
        if (dc->list_entry_index.Flink == &fileref->fcb->dir_children_index)
            return STATUS_NO_MORE_FILES;

        dc = CONTAINING_RECORD(dc->list_entry_index.Flink, dir_child, list_entry_index);
    }

    de->key = dc->key;
    de->name = dc->name;
    de->type = dc->type;
    de->dir_entry_type = DirEntryType_File;
    de->dc = dc;

    *offset = dc->index + 1;
    *pdc = dc;

    return STATUS_SUCCESS;
}

static NTSTATUS query_directory(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status, status2;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    device_extension* Vcb;
    void* buf;
    uint8_t *curitem, *lastitem;
    LONG length;
    ULONG count;
    bool has_wildcard = false, specific_file = false, initial;
    dir_entry de;
    uint64_t newoffset;
    dir_child* dc = NULL;

    TRACE("query directory\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb = IrpSp->FileObject->FsContext;
    ccb = IrpSp->FileObject->FsContext2;
    fileref = ccb ? ccb->fileref : NULL;

    if (!fileref)
        return STATUS_INVALID_PARAMETER;

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!fcb) {
        ERR("fcb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_LIST_DIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    Vcb = fcb->Vcb;

    if (!Vcb) {
        ERR("Vcb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (fileref->fcb == Vcb->dummy_fcb)
        return STATUS_NO_MORE_FILES;

    if (IrpSp->Flags == 0) {
        TRACE("QD flags: (none)\n");
    } else {
        ULONG flags = IrpSp->Flags;

        TRACE("QD flags:\n");

        if (flags & SL_INDEX_SPECIFIED) {
            TRACE("    SL_INDEX_SPECIFIED\n");
            flags &= ~SL_INDEX_SPECIFIED;
        }

        if (flags & SL_RESTART_SCAN) {
            TRACE("    SL_RESTART_SCAN\n");
            flags &= ~SL_RESTART_SCAN;
        }

        if (flags & SL_RETURN_SINGLE_ENTRY) {
            TRACE("    SL_RETURN_SINGLE_ENTRY\n");
            flags &= ~SL_RETURN_SINGLE_ENTRY;
        }

        if (flags != 0)
            TRACE("    unknown flags: %lu\n", flags);
    }

    if (IrpSp->Flags & SL_RESTART_SCAN) {
        ccb->query_dir_offset = 0;

        if (ccb->query_string.Buffer) {
            RtlFreeUnicodeString(&ccb->query_string);
            ccb->query_string.Buffer = NULL;
        }

        ccb->has_wildcard = false;
        ccb->specific_file = false;
    }

    initial = !ccb->query_string.Buffer;

    if (IrpSp->Parameters.QueryDirectory.FileName && IrpSp->Parameters.QueryDirectory.FileName->Length > 1) {
        TRACE("QD filename: %.*S\n", (int)(IrpSp->Parameters.QueryDirectory.FileName->Length / sizeof(WCHAR)), IrpSp->Parameters.QueryDirectory.FileName->Buffer);

        if (IrpSp->Parameters.QueryDirectory.FileName->Length > sizeof(WCHAR) || IrpSp->Parameters.QueryDirectory.FileName->Buffer[0] != L'*') {
            specific_file = true;

            if (FsRtlDoesNameContainWildCards(IrpSp->Parameters.QueryDirectory.FileName)) {
                has_wildcard = true;
                specific_file = false;
            } else if (!initial)
                return STATUS_NO_MORE_FILES;
        }

        if (ccb->query_string.Buffer)
            RtlFreeUnicodeString(&ccb->query_string);

        if (has_wildcard)
            RtlUpcaseUnicodeString(&ccb->query_string, IrpSp->Parameters.QueryDirectory.FileName, true);
        else {
            ccb->query_string.Buffer = ExAllocatePoolWithTag(PagedPool, IrpSp->Parameters.QueryDirectory.FileName->Length, ALLOC_TAG);
            if (!ccb->query_string.Buffer) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ccb->query_string.Length = ccb->query_string.MaximumLength = IrpSp->Parameters.QueryDirectory.FileName->Length;
            RtlCopyMemory(ccb->query_string.Buffer, IrpSp->Parameters.QueryDirectory.FileName->Buffer, IrpSp->Parameters.QueryDirectory.FileName->Length);
        }

        ccb->has_wildcard = has_wildcard;
        ccb->specific_file = specific_file;
    } else {
        has_wildcard = ccb->has_wildcard;
        specific_file = ccb->specific_file;

        if (!(IrpSp->Flags & SL_RESTART_SCAN)) {
            initial = false;

            if (specific_file)
                return STATUS_NO_MORE_FILES;
        }
    }

    if (ccb->query_string.Buffer) {
        TRACE("query string = %.*S\n", (int)(ccb->query_string.Length / sizeof(WCHAR)), ccb->query_string.Buffer);
    }

    newoffset = ccb->query_dir_offset;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

    ExAcquireResourceSharedLite(&fileref->fcb->nonpaged->dir_children_lock, true);

    Status = next_dir_entry(fileref, &newoffset, &de, &dc);

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_NO_MORE_FILES && initial)
            Status = STATUS_NO_SUCH_FILE;
        goto end;
    }

    ccb->query_dir_offset = newoffset;

    buf = map_user_buffer(Irp, NormalPagePriority);

    if (Irp->MdlAddress && !buf) {
        ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    length = IrpSp->Parameters.QueryDirectory.Length;

    if (specific_file) {
        bool found = false;
        UNICODE_STRING us;
        LIST_ENTRY* le;
        uint32_t hash;
        uint8_t c;

        us.Buffer = NULL;

        if (!ccb->case_sensitive) {
            Status = RtlUpcaseUnicodeString(&us, &ccb->query_string, true);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08lx\n", Status);
                goto end;
            }

            hash = calc_crc32c(0xffffffff, (uint8_t*)us.Buffer, us.Length);
        } else
            hash = calc_crc32c(0xffffffff, (uint8_t*)ccb->query_string.Buffer, ccb->query_string.Length);

        c = hash >> 24;

        if (ccb->case_sensitive) {
            if (fileref->fcb->hash_ptrs[c]) {
                le = fileref->fcb->hash_ptrs[c];
                while (le != &fileref->fcb->dir_children_hash) {
                    dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_hash);

                    if (dc2->hash == hash) {
                        if (dc2->name.Length == ccb->query_string.Length && RtlCompareMemory(dc2->name.Buffer, ccb->query_string.Buffer, ccb->query_string.Length) == ccb->query_string.Length) {
                            found = true;

                            de.key = dc2->key;
                            de.name = dc2->name;
                            de.type = dc2->type;
                            de.dir_entry_type = DirEntryType_File;
                            de.dc = dc2;

                            break;
                        }
                    } else if (dc2->hash > hash)
                        break;

                    le = le->Flink;
                }
            }
        } else {
            if (fileref->fcb->hash_ptrs_uc[c]) {
                le = fileref->fcb->hash_ptrs_uc[c];
                while (le != &fileref->fcb->dir_children_hash_uc) {
                    dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_hash_uc);

                    if (dc2->hash_uc == hash) {
                        if (dc2->name_uc.Length == us.Length && RtlCompareMemory(dc2->name_uc.Buffer, us.Buffer, us.Length) == us.Length) {
                            found = true;

                            de.key = dc2->key;
                            de.name = dc2->name;
                            de.type = dc2->type;
                            de.dir_entry_type = DirEntryType_File;
                            de.dc = dc2;

                            break;
                        }
                    } else if (dc2->hash_uc > hash)
                        break;

                    le = le->Flink;
                }
            }
        }

        if (us.Buffer)
            ExFreePool(us.Buffer);

        if (!found) {
            Status = STATUS_NO_SUCH_FILE;
            goto end;
        }
    } else if (has_wildcard) {
        while (!FsRtlIsNameInExpression(&ccb->query_string, &de.name, !ccb->case_sensitive, NULL)) {
            newoffset = ccb->query_dir_offset;
            Status = next_dir_entry(fileref, &newoffset, &de, &dc);

            if (NT_SUCCESS(Status))
                ccb->query_dir_offset = newoffset;
            else {
                if (Status == STATUS_NO_MORE_FILES && initial)
                    Status = STATUS_NO_SUCH_FILE;

                goto end;
            }
        }
    }

    TRACE("file(0) = %.*S\n", (int)(de.name.Length / sizeof(WCHAR)), de.name.Buffer);
    TRACE("offset = %I64u\n", ccb->query_dir_offset - 1);

    Status = query_dir_item(fcb, ccb, buf, &length, Irp, &de, fcb->subvol);

    count = 0;
    if (NT_SUCCESS(Status) && !(IrpSp->Flags & SL_RETURN_SINGLE_ENTRY) && !specific_file) {
        lastitem = (uint8_t*)buf;

        while (length > 0) {
            switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
                case FileBothDirectoryInformation:
                case FileDirectoryInformation:
                case FileIdBothDirectoryInformation:
                case FileFullDirectoryInformation:
                case FileIdFullDirectoryInformation:
                case FileIdExtdDirectoryInformation:
                case FileIdExtdBothDirectoryInformation:
                    length -= length % 8;
                    break;
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

                case FileNamesInformation:
                    length -= length % 4;
                    break;

                default:
                    WARN("unhandled file information class %u\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
                    break;
            }

            if (length > 0) {
                newoffset = ccb->query_dir_offset;
                Status = next_dir_entry(fileref, &newoffset, &de, &dc);
                if (NT_SUCCESS(Status)) {
                    if (!has_wildcard || FsRtlIsNameInExpression(&ccb->query_string, &de.name, !ccb->case_sensitive, NULL)) {
                        curitem = (uint8_t*)buf + IrpSp->Parameters.QueryDirectory.Length - length;
                        count++;

                        TRACE("file(%lu) %Iu = %.*S\n", count, curitem - (uint8_t*)buf, (int)(de.name.Length / sizeof(WCHAR)), de.name.Buffer);
                        TRACE("offset = %I64u\n", ccb->query_dir_offset - 1);

                        status2 = query_dir_item(fcb, ccb, curitem, &length, Irp, &de, fcb->subvol);

                        if (NT_SUCCESS(status2)) {
                            ULONG* lastoffset = (ULONG*)lastitem;

                            *lastoffset = (ULONG)(curitem - lastitem);
                            ccb->query_dir_offset = newoffset;

                            lastitem = curitem;
                        } else
                            break;
                    } else
                        ccb->query_dir_offset = newoffset;
                } else {
                    if (Status == STATUS_NO_MORE_FILES)
                        Status = STATUS_SUCCESS;

                    break;
                }
            } else
                break;
        }
    }

    Irp->IoStatus.Information = IrpSp->Parameters.QueryDirectory.Length - length;

end:
    ExReleaseResourceLite(&fileref->fcb->nonpaged->dir_children_lock);

    ExReleaseResourceLite(&Vcb->tree_lock);

    TRACE("returning %08lx\n", Status);

    return Status;
}

static NTSTATUS notify_change_directory(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    NTSTATUS Status;

    TRACE("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n");

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!fileref) {
        ERR("no fileref\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_LIST_DIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // FIXME - raise exception if FCB marked for deletion?

    TRACE("FileObject %p\n", FileObject);

    if (ccb->filename.Length == 0) {
        ULONG reqlen;

        ccb->filename.MaximumLength = ccb->filename.Length = 0;

        Status = fileref_get_filename(fileref, &ccb->filename, NULL, &reqlen);
        if (Status == STATUS_BUFFER_OVERFLOW) {
            ccb->filename.Buffer = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);
            if (!ccb->filename.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            ccb->filename.MaximumLength = (uint16_t)reqlen;

            Status = fileref_get_filename(fileref, &ccb->filename, NULL, &reqlen);
            if (!NT_SUCCESS(Status)) {
                ERR("fileref_get_filename returned %08lx\n", Status);
                goto end;
            }
        } else {
            ERR("fileref_get_filename returned %08lx\n", Status);
            goto end;
        }
    }

    FsRtlNotifyFilterChangeDirectory(Vcb->NotifySync, &Vcb->DirNotifyList, FileObject->FsContext2, (PSTRING)&ccb->filename,
                                     IrpSp->Flags & SL_WATCH_TREE, false, IrpSp->Parameters.NotifyDirectory.CompletionFilter, Irp,
                                     NULL, NULL, NULL);

    Status = STATUS_PENDING;

end:
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return Status;
}

_Dispatch_type_(IRP_MJ_DIRECTORY_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG func;
    bool top_level;
    device_extension* Vcb = DeviceObject->DeviceExtension;

    FsRtlEnterFileSystem();

    TRACE("directory control\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    func = IrpSp->MinorFunction;

    switch (func) {
        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            Status = notify_change_directory(Vcb, Irp);
            break;

        case IRP_MN_QUERY_DIRECTORY:
            Status = query_directory(Irp);
            break;

        default:
            WARN("unknown minor %lu\n", func);
            Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Status = Status;
            break;
    }

    if (Status == STATUS_PENDING)
        goto exit;

end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_DISK_INCREMENT);

exit:
    TRACE("returning %08lx\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
