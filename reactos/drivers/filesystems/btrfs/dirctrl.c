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

enum DirEntryType {
    DirEntryType_File,
    DirEntryType_Self,
    DirEntryType_Parent
};

typedef struct {
    KEY key;
    BOOL name_alloc;
    char* name;
    ULONG namelen;
    UINT8 type;
    enum DirEntryType dir_entry_type;
} dir_entry;

ULONG STDCALL get_reparse_tag(device_extension* Vcb, root* subvol, UINT64 inode, UINT8 type, ULONG atts, PIRP Irp) {
    fcb* fcb;
    ULONG tag = 0, br;
    NTSTATUS Status;
    
    if (type == BTRFS_TYPE_SYMLINK) {
        if (called_from_lxss())
            return IO_REPARSE_TAG_LXSS_SYMLINK;
        else
            return IO_REPARSE_TAG_SYMLINK;
    }
    
    if (type != BTRFS_TYPE_FILE && type != BTRFS_TYPE_DIRECTORY)
        return 0;
    
    if (!(atts & FILE_ATTRIBUTE_REPARSE_POINT))
        return 0;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    Status = open_fcb(Vcb, subvol, inode, type, NULL, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        return 0;
    }
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    if (type == BTRFS_TYPE_DIRECTORY) {
        if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length < sizeof(ULONG))
            goto end;
        
        RtlCopyMemory(&tag, fcb->reparse_xattr.Buffer, sizeof(ULONG));
    } else {
        Status = read_file(fcb, (UINT8*)&tag, 0, sizeof(ULONG), &br, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08x\n", Status);
            goto end;
        }
        
        if (br < sizeof(ULONG))
            goto end;
    }
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);

    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    free_fcb(fcb);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    return tag;
}

static ULONG get_ea_len(device_extension* Vcb, root* subvol, UINT64 inode, PIRP Irp) {
    UINT8* eadata;
    UINT16 len;
    
    if (get_xattr(Vcb, subvol, inode, EA_EA, EA_EA_HASH, &eadata, &len, Irp)) {
        ULONG offset;
        NTSTATUS Status;
        
        Status = IoCheckEaBufferValidity((FILE_FULL_EA_INFORMATION*)eadata, len, &offset);
        
        if (!NT_SUCCESS(Status)) {
            WARN("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
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
                
                eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
            } while (TRUE);
            
            ExFreePool(eadata);
            
            return ealen;
        }
    } else
        return 0;
}

static NTSTATUS STDCALL query_dir_item(fcb* fcb, file_ref* fileref, void* buf, LONG* len, PIRP Irp, dir_entry* de, root* r) {
    PIO_STACK_LOCATION IrpSp;
    UINT32 needed;
    UINT64 inode;
    INODE_ITEM ii;
    NTSTATUS Status;
    ULONG stringlen;
    ULONG atts, ealen;
    
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
        
        if (!r) {
            ERR("could not find root %llx\n", de->key.obj_id);
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        
        inode = SUBVOL_ROOT_INODE;
    } else {
        inode = de->key.obj_id;
    }
    
    if (IrpSp->Parameters.QueryDirectory.FileInformationClass != FileNamesInformation) { // FIXME - object ID and reparse point classes too?
        switch (de->dir_entry_type) {
            case DirEntryType_File:
            {
                LIST_ENTRY* le;
                BOOL found = FALSE;
                
                ExAcquireResourceSharedLite(&fcb->Vcb->fcb_lock, TRUE);
                if (!IsListEmpty(&r->fcbs)) {
                    le = r->fcbs.Flink;
                    while (le != &r->fcbs) {
                        struct _fcb* fcb2 = CONTAINING_RECORD(le, struct _fcb, list_entry);
                        
                        if (fcb2->inode == inode && !fcb2->ads) {
                            ii = fcb2->inode_item;
                            atts = fcb2->atts;
                            ealen = fcb2->ealen;
                            found = TRUE;
                            break;
                        }
                        
                        le = le->Flink;
                    }
                }
                ExReleaseResourceLite(&fcb->Vcb->fcb_lock);
                
                if (!found) {
                    KEY searchkey;
                    traverse_ptr tp;
                    
                    searchkey.obj_id = inode;
                    searchkey.obj_type = TYPE_INODE_ITEM;
                    searchkey.offset = 0xffffffffffffffff;
                    
                    Status = find_item(fcb->Vcb, r, &tp, &searchkey, FALSE, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        return Status;
                    }
                    
                    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                        ERR("could not find inode item for inode %llx in root %llx\n", inode, r->id);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    RtlZeroMemory(&ii, sizeof(INODE_ITEM));
                    
                    if (tp.item->size > 0)
                        RtlCopyMemory(&ii, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
                    
                    if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileDirectoryInformation ||
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation || 
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdFullDirectoryInformation) {
                        
                        BOOL dotfile = de->namelen > 1 && de->name[0] == '.';

                        atts = get_file_attributes(fcb->Vcb, &ii, r, inode, de->type, dotfile, FALSE, Irp);
                    }
                    
                    if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation || 
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation || 
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation || 
                        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdFullDirectoryInformation) {
                        ealen = get_ea_len(fcb->Vcb, r, inode, Irp);
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
    }
    
    // FICs which return the filename
    if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdFullDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileNamesInformation) {
        
        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de->name, de->namelen);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            return Status;
        }
    }
    
    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
        case FileBothDirectoryInformation:
        {
            FILE_BOTH_DIR_INFORMATION* fbdi = buf;
            
            TRACE("FileBothDirectoryInformation\n");
            
            needed = sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            fbdi->NextEntryOffset = 0;
            fbdi->FileIndex = 0;
            fbdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fbdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fbdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fbdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fbdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fbdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fbdi->FileAttributes = atts;
            fbdi->FileNameLength = stringlen;
            fbdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fbdi->ShortNameLength = 0;
//             fibdi->ShortName[12];
            
            Status = RtlUTF8ToUnicodeN(fbdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileDirectoryInformation:
        {
            FILE_DIRECTORY_INFORMATION* fdi = buf;
            
            TRACE("FileDirectoryInformation\n");
            
            needed = sizeof(FILE_DIRECTORY_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            fdi->NextEntryOffset = 0;
            fdi->FileIndex = 0;
            fdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fdi->FileAttributes = atts;
            fdi->FileNameLength = stringlen;
            
            Status = RtlUTF8ToUnicodeN(fdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }
            
        case FileFullDirectoryInformation:
        {
            FILE_FULL_DIR_INFORMATION* ffdi = buf;
            
            TRACE("FileFullDirectoryInformation\n");
            
            needed = sizeof(FILE_FULL_DIR_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            ffdi->NextEntryOffset = 0;
            ffdi->FileIndex = 0;
            ffdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            ffdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            ffdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            ffdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            ffdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            ffdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            ffdi->FileAttributes = atts;
            ffdi->FileNameLength = stringlen;
            ffdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            
            Status = RtlUTF8ToUnicodeN(ffdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileIdBothDirectoryInformation:
        {
            FILE_ID_BOTH_DIR_INFORMATION* fibdi = buf;
            
            TRACE("FileIdBothDirectoryInformation\n");
            
            needed = sizeof(FILE_ID_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
            
//             if (!buf)
//                 return STATUS_INVALID_POINTER;
            
            fibdi->NextEntryOffset = 0;
            fibdi->FileIndex = 0;
            fibdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fibdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fibdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fibdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fibdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fibdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fibdi->FileAttributes = atts;
            fibdi->FileNameLength = stringlen;
            fibdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fibdi->ShortNameLength = 0;
//             fibdi->ShortName[12];
            fibdi->FileId.QuadPart = make_file_id(r, inode);
            
            Status = RtlUTF8ToUnicodeN(fibdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileIdFullDirectoryInformation:
        {
            FILE_ID_FULL_DIR_INFORMATION* fifdi = buf;
            
            TRACE("FileIdFullDirectoryInformation\n");
            
            needed = sizeof(FILE_ID_FULL_DIR_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
            
//             if (!buf)
//                 return STATUS_INVALID_POINTER;
            
            fifdi->NextEntryOffset = 0;
            fifdi->FileIndex = 0;
            fifdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fifdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fifdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fifdi->ChangeTime.QuadPart = unix_time_to_win(&ii.st_ctime);
            fifdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fifdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fifdi->FileAttributes = atts;
            fifdi->FileNameLength = stringlen;
            fifdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fifdi->FileId.QuadPart = make_file_id(r, inode);
            
            Status = RtlUTF8ToUnicodeN(fifdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileNamesInformation:
        {
            FILE_NAMES_INFORMATION* fni = buf;
            
            TRACE("FileNamesInformation\n");
            
            needed = sizeof(FILE_NAMES_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
            
            fni->NextEntryOffset = 0;
            fni->FileIndex = 0;
            fni->FileNameLength = stringlen;
            
            Status = RtlUTF8ToUnicodeN(fni->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileObjectIdInformation:
            FIXME("STUB: FileObjectIdInformation\n");
            return STATUS_NOT_IMPLEMENTED;

        case FileQuotaInformation:
            FIXME("STUB: FileQuotaInformation\n");
            return STATUS_NOT_IMPLEMENTED;

        case FileReparsePointInformation:
            FIXME("STUB: FileReparsePointInformation\n");
            return STATUS_NOT_IMPLEMENTED;

        default:
            WARN("Unknown FileInformationClass %u\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
            return STATUS_NOT_IMPLEMENTED;
    }
    
    return STATUS_NO_MORE_FILES;
}

static NTSTATUS STDCALL next_dir_entry(file_ref* fileref, UINT64* offset, dir_entry* de, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    DIR_ITEM* di;
    NTSTATUS Status;
    file_ref* fr;
    LIST_ENTRY* le;
    char* name;
    
    if (fileref->parent) { // don't return . and .. if root directory
        if (*offset == 0) {
            de->key.obj_id = fileref->fcb->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Self;
            de->name = ".";
            de->name_alloc = FALSE;
            de->namelen = 1;
            de->type = BTRFS_TYPE_DIRECTORY;
            
            *offset = 1;
            
            return STATUS_SUCCESS;
        } else if (*offset == 1) {
            de->key.obj_id = fileref->parent->fcb->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Parent;
            de->name = "..";
            de->name_alloc = FALSE;
            de->namelen = 2;
            de->type = BTRFS_TYPE_DIRECTORY;
            
            *offset = 2;
            
            return STATUS_SUCCESS;
        }
    }
    
    if (*offset < 2)
        *offset = 2;
    
    ExAcquireResourceSharedLite(&fileref->nonpaged->children_lock, TRUE);
    
    fr = NULL;
    le = fileref->children.Flink;
    
    // skip entries before offset
    while (le != &fileref->children) {
        file_ref* fr2 = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (fr2->index >= *offset) {
            fr = fr2;
            break;
        }
        
        le = le->Flink;
    }
    
    do {
        if (fr && fr->index == *offset) {
            if (!fr->deleted) {
                if (fr->fcb->subvol == fileref->fcb->subvol) {
                    de->key.obj_id = fr->fcb->inode;
                    de->key.obj_type = TYPE_INODE_ITEM;
                    de->key.offset = 0;
                } else {
                    de->key.obj_id = fr->fcb->subvol->id;
                    de->key.obj_type = TYPE_ROOT_ITEM;
                    de->key.offset = 0;
                }
                
                name = fr->utf8.Buffer;
                de->namelen = fr->utf8.Length;
                de->type = fr->fcb->type;
                de->dir_entry_type = DirEntryType_File;
                
                (*offset)++;
                
                Status = STATUS_SUCCESS;
                goto end;
            } else {
                (*offset)++;
                fr = fr->list_entry.Flink == &fileref->children ? NULL : CONTAINING_RECORD(fr->list_entry.Flink, file_ref, list_entry);
                continue;
            }
        }
        
        searchkey.obj_id = fileref->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = *offset;
        
        Status = find_item(fileref->fcb->Vcb, fileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (keycmp(tp.item->key, searchkey) == -1) {
            if (find_next_item(fileref->fcb->Vcb, &tp, &next_tp, FALSE, Irp))
                tp = next_tp;
        }
        
        if (keycmp(tp.item->key, searchkey) != -1 && tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            do {
                if (fr) {
                    if (fr->index <= tp.item->key.offset && !fr->deleted) {
                        if (fr->fcb->subvol == fileref->fcb->subvol) {
                            de->key.obj_id = fr->fcb->inode;
                            de->key.obj_type = TYPE_INODE_ITEM;
                            de->key.offset = 0;
                        } else {
                            de->key.obj_id = fr->fcb->subvol->id;
                            de->key.obj_type = TYPE_ROOT_ITEM;
                            de->key.offset = 0;
                        }
                        
                        name = fr->utf8.Buffer;
                        de->namelen = fr->utf8.Length;
                        de->type = fr->fcb->type;
                        de->dir_entry_type = DirEntryType_File;
                        
                        *offset = fr->index + 1;
                        
                        Status = STATUS_SUCCESS;
                        goto end;
                    }
                    
                    if (fr->index == tp.item->key.offset && fr->deleted)
                        break;
                    
                    fr = fr->list_entry.Flink == &fileref->children ? NULL : CONTAINING_RECORD(fr->list_entry.Flink, file_ref, list_entry);
                }
            } while (fr && fr->index < tp.item->key.offset);
            
            if (fr && fr->index == tp.item->key.offset && fr->deleted) {
                *offset = fr->index + 1;
                fr = fr->list_entry.Flink == &fileref->children ? NULL : CONTAINING_RECORD(fr->list_entry.Flink, file_ref, list_entry);
                continue;
            }
            
            *offset = tp.item->key.offset + 1;
            
            di = (DIR_ITEM*)tp.item->data;
            
            if (tp.item->size < sizeof(DIR_ITEM) || tp.item->size < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            de->key = di->key;
            name = di->name;
            de->namelen = di->n;
            de->type = di->type;
            de->dir_entry_type = DirEntryType_File;
            
            Status = STATUS_SUCCESS;
            goto end;
        } else {
            if (fr) {
                if (fr->fcb->subvol == fileref->fcb->subvol) {
                    de->key.obj_id = fr->fcb->inode;
                    de->key.obj_type = TYPE_INODE_ITEM;
                    de->key.offset = 0;
                } else {
                    de->key.obj_id = fr->fcb->subvol->id;
                    de->key.obj_type = TYPE_ROOT_ITEM;
                    de->key.offset = 0;
                }
                
                name = fr->utf8.Buffer;
                de->namelen = fr->utf8.Length;
                de->type = fr->fcb->type;
                de->dir_entry_type = DirEntryType_File;
                
                *offset = fr->index + 1;
                
                Status = STATUS_SUCCESS;
                goto end;
            } else {
                Status = STATUS_NO_MORE_FILES;
                goto end;
            }
        }
    } while (TRUE);
    
end:
    ExReleaseResourceLite(&fileref->nonpaged->children_lock);
    
    if (NT_SUCCESS(Status)) {
        de->name_alloc = TRUE;
        
        de->name = ExAllocatePoolWithTag(PagedPool, de->namelen, ALLOC_TAG);
        if (!de->name) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(de->name, name, de->namelen);
    } else
        de->name_alloc = FALSE;
    
    return Status;
}

static NTSTATUS STDCALL query_directory(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status, status2;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    void* buf;
    UINT8 *curitem, *lastitem;
    LONG length;
    ULONG count;
    BOOL has_wildcard = FALSE, specific_file = FALSE, initial;
//     UINT64 num_reads_orig;
    dir_entry de;
    UINT64 newoffset;
    ANSI_STRING utf8;
    
    TRACE("query directory\n");
    
//     get_uid(); // TESTING
    
//     num_reads_orig = num_reads;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb = IrpSp->FileObject->FsContext;
    ccb = IrpSp->FileObject->FsContext2;
    fileref = ccb ? ccb->fileref : NULL;
    
    utf8.Buffer = NULL;
    
    if (!fileref)
        return STATUS_INVALID_PARAMETER;

    if (!ccb) {
        ERR("ccb was NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_LIST_DIRECTORY)) {
        WARN("insufficient privileges\n");
        return STATUS_ACCESS_DENIED;
    }
    
    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    
    TRACE("%S\n", file_desc(IrpSp->FileObject));
    
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
            TRACE("    unknown flags: %u\n", flags);
    }
    
    initial = !ccb->query_string.Buffer;
    
    if (IrpSp->Flags & SL_RESTART_SCAN) {
        ccb->query_dir_offset = 0;
        
        if (ccb->query_string.Buffer) {
            RtlFreeUnicodeString(&ccb->query_string);
            ccb->query_string.Buffer = NULL;
        }
    }
    
    if (IrpSp->Parameters.QueryDirectory.FileName && IrpSp->Parameters.QueryDirectory.FileName->Length > 1) {
        TRACE("QD filename: %.*S\n", IrpSp->Parameters.QueryDirectory.FileName->Length / sizeof(WCHAR), IrpSp->Parameters.QueryDirectory.FileName->Buffer);
        
        if (IrpSp->Parameters.QueryDirectory.FileName->Buffer[0] != '*') {
            specific_file = TRUE;
            if (!ccb->case_sensitive || FsRtlDoesNameContainWildCards(IrpSp->Parameters.QueryDirectory.FileName)) {
                has_wildcard = TRUE;
                specific_file = FALSE;
            }
        }

        if (ccb->query_string.Buffer)
            RtlFreeUnicodeString(&ccb->query_string);
        
        if (has_wildcard)
            RtlUpcaseUnicodeString(&ccb->query_string, IrpSp->Parameters.QueryDirectory.FileName, TRUE);
        else {
            ccb->query_string.Buffer = ExAllocatePoolWithTag(PagedPool, IrpSp->Parameters.QueryDirectory.FileName->Length, ALLOC_TAG);
            if (!ccb->query_string.Buffer) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
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
            initial = FALSE;
            
            if (specific_file) {
                Status = STATUS_NO_MORE_FILES;
                goto end;
            }
        }
    }
    
    if (ccb->query_string.Buffer) {
        TRACE("query string = %.*S\n", ccb->query_string.Length / sizeof(WCHAR), ccb->query_string.Buffer);
    }
    
    newoffset = ccb->query_dir_offset;
    Status = next_dir_entry(fileref, &newoffset, &de, Irp);
    
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_NO_MORE_FILES && initial)
            Status = STATUS_NO_SUCH_FILE;
        goto end;
    }
    
    ccb->query_dir_offset = newoffset;

    buf = map_user_buffer(Irp);
    
    if (Irp->MdlAddress && !buf) {
        ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    length = IrpSp->Parameters.QueryDirectory.Length;
    
    if (specific_file) {
        BOOL found = FALSE;
        root* found_subvol;
        UINT64 found_inode, found_index;
        UINT8 found_type;
        UNICODE_STRING us;
        LIST_ENTRY* le;
        
        us.Buffer = NULL;
        
        if (!ccb->case_sensitive) {
            Status = RtlUpcaseUnicodeString(&us, &ccb->query_string, TRUE);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                goto end;
            }
        }
        
        ExAcquireResourceSharedLite(&fileref->nonpaged->children_lock, TRUE);
        
        le = fileref->children.Flink;
        while (le != &fileref->children) {
            file_ref* fr2 = CONTAINING_RECORD(le, file_ref, list_entry);
            
            if (!fr2->deleted) {
                if (!ccb->case_sensitive && fr2->filepart_uc.Length == us.Length &&
                    RtlCompareMemory(fr2->filepart_uc.Buffer, us.Buffer, us.Length) == us.Length)
                    found = TRUE;
                else if (ccb->case_sensitive && fr2->filepart.Length == ccb->query_string.Length &&
                    RtlCompareMemory(fr2->filepart.Buffer, ccb->query_string.Buffer, ccb->query_string.Length) == ccb->query_string.Length)
                    found = TRUE;
            }
                
            if (found) {
                if (fr2->fcb->subvol == fcb->subvol) {
                    de.key.obj_id = fr2->fcb->inode;
                    de.key.obj_type = TYPE_INODE_ITEM;
                    de.key.offset = 0;
                } else {
                    de.key.obj_id = fr2->fcb->subvol->id;
                    de.key.obj_type = TYPE_ROOT_ITEM;
                    de.key.offset = 0;
                }
                
                de.name = ExAllocatePoolWithTag(PagedPool, fr2->utf8.Length, ALLOC_TAG);
                if (!de.name) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                RtlCopyMemory(de.name, fr2->utf8.Buffer, fr2->utf8.Length);
                
                de.name_alloc = TRUE;
                de.namelen = fr2->utf8.Length;
                de.type = fr2->fcb->type;
                de.dir_entry_type = DirEntryType_File;
                break;
            }
                
            le = le->Flink;
        }
        
        ExReleaseResourceLite(&fileref->nonpaged->children_lock);
        
        if (us.Buffer)
            ExFreePool(us.Buffer);
        
        if (!found) {
            Status = find_file_in_dir(fcb->Vcb, &ccb->query_string, fileref, &found_subvol, &found_inode, &found_type, &found_index, &utf8, FALSE, Irp);
            
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_NO_SUCH_FILE;
                goto end;
            }
            
            if (found_subvol == fcb->subvol) {
                de.key.obj_id = found_inode;
                de.key.obj_type = TYPE_INODE_ITEM;
                de.key.offset = 0;
            } else {
                de.key.obj_id = found_subvol->id;
                de.key.obj_type = TYPE_ROOT_ITEM;
                de.key.offset = 0;
            }
            
            de.name = utf8.Buffer;
            de.name_alloc = FALSE;
            de.namelen = utf8.Length;
            de.type = found_type;
            de.dir_entry_type = DirEntryType_File;
        }
    } else if (has_wildcard) {
        WCHAR* uni_fn;
        ULONG stringlen;
        UNICODE_STRING di_uni_fn;
        
        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            goto end;
        }
        
        uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
        if (!uni_fn) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
        
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            goto end;
        }
        
        di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
        di_uni_fn.Buffer = uni_fn;
        
        while (!FsRtlIsNameInExpression(&ccb->query_string, &di_uni_fn, !ccb->case_sensitive, NULL)) {
            if (de.name_alloc)
                ExFreePool(de.name);
            
            newoffset = ccb->query_dir_offset;
            Status = next_dir_entry(fileref, &newoffset, &de, Irp);
            
            ExFreePool(uni_fn);
            if (NT_SUCCESS(Status)) {
                ccb->query_dir_offset = newoffset;
                
                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                    goto end;
                }
                
                uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                if (!uni_fn) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                    ExFreePool(uni_fn);
                    goto end;
                }
                
                di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
                di_uni_fn.Buffer = uni_fn;
            } else {
                if (Status == STATUS_NO_MORE_FILES && initial)
                    Status = STATUS_NO_SUCH_FILE;
                
                goto end;
            }
        }
        
        ExFreePool(uni_fn);
    }
    
    TRACE("file(0) = %.*s\n", de.namelen, de.name);
    TRACE("offset = %u\n", ccb->query_dir_offset - 1);

    Status = query_dir_item(fcb, fileref, buf, &length, Irp, &de, fcb->subvol);
    
    if (de.name_alloc)
        ExFreePool(de.name);
    
    count = 0;
    if (NT_SUCCESS(Status) && !(IrpSp->Flags & SL_RETURN_SINGLE_ENTRY) && !specific_file) {
        lastitem = (UINT8*)buf;
        
        while (length > 0) {
            switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
                case FileBothDirectoryInformation:
                case FileDirectoryInformation:
                case FileIdBothDirectoryInformation:
                case FileFullDirectoryInformation:
                case FileIdFullDirectoryInformation:
                    length -= length % 8;
                    break;
                    
                case FileNamesInformation:
                    length -= length % 4;
                    break;
                    
                default:
                    WARN("unhandled file information class %u\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
                    break;
            }
            
            if (length > 0) {
                WCHAR* uni_fn = NULL;
                UNICODE_STRING di_uni_fn;
                
                newoffset = ccb->query_dir_offset;
                Status = next_dir_entry(fileref, &newoffset, &de, Irp);
                if (NT_SUCCESS(Status)) {
                    if (has_wildcard) {
                        ULONG stringlen;
                        
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                            if (de.name_alloc) ExFreePool(de.name);
                            goto end;
                        }
                        
                        uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!uni_fn) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            if (de.name_alloc) ExFreePool(de.name);
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                            ExFreePool(uni_fn);
                            if (de.name_alloc) ExFreePool(de.name);
                            goto end;
                        }
                        
                        di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
                        di_uni_fn.Buffer = uni_fn;
                    }
                    
                    if (!has_wildcard || FsRtlIsNameInExpression(&ccb->query_string, &di_uni_fn, !ccb->case_sensitive, NULL)) {
                        curitem = (UINT8*)buf + IrpSp->Parameters.QueryDirectory.Length - length;
                        count++;
                        
                        TRACE("file(%u) %u = %.*s\n", count, curitem - (UINT8*)buf, de.namelen, de.name);
                        TRACE("offset = %u\n", ccb->query_dir_offset - 1);
                        
                        status2 = query_dir_item(fcb, fileref, curitem, &length, Irp, &de, fcb->subvol);
                        
                        if (NT_SUCCESS(status2)) {
                            ULONG* lastoffset = (ULONG*)lastitem;
                            
                            *lastoffset = (ULONG)(curitem - lastitem);
                            ccb->query_dir_offset = newoffset;
                            
                            lastitem = curitem;
                        } else {
                            if (uni_fn) ExFreePool(uni_fn);
                            if (de.name_alloc) ExFreePool(de.name);
                            break;
                        }
                    } else
                        ccb->query_dir_offset = newoffset;
                    
                    if (uni_fn) {
                        ExFreePool(uni_fn);
                        uni_fn = NULL;
                    }
                    
                    if (de.name_alloc)
                        ExFreePool(de.name);
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
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
    
    TRACE("returning %08x\n", Status);
    
    if (utf8.Buffer)
        ExFreePool(utf8.Buffer);

    return Status;
}

static NTSTATUS STDCALL notify_change_directory(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb->fileref;
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
    
    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    // FIXME - raise exception if FCB marked for deletion?
    
    TRACE("%S\n", file_desc(FileObject));

    if (ccb->filename.Length == 0) {
        Status = fileref_get_filename(fileref, &ccb->filename, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("fileref_get_filename returned %08x\n", Status);
            goto end;
        }
    }
    
    FsRtlNotifyFilterChangeDirectory(Vcb->NotifySync, &Vcb->DirNotifyList, FileObject->FsContext2, (PSTRING)&ccb->filename,
                                     IrpSp->Flags & SL_WATCH_TREE, FALSE, IrpSp->Parameters.NotifyDirectory.CompletionFilter, Irp,
                                     NULL, NULL, NULL);
    
    Status = STATUS_PENDING;
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
    
    return Status;
}

NTSTATUS STDCALL drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG func;
    BOOL top_level;
    device_extension* Vcb = DeviceObject->DeviceExtension;

    TRACE("directory control\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    Irp->IoStatus.Information = 0;
    
    func = IrpSp->MinorFunction;
    
    switch (func) {
        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            Status = notify_change_directory(Vcb, Irp);
            break;
            
        case IRP_MN_QUERY_DIRECTORY:
            Status = query_directory(DeviceObject, Irp);
            break;
            
        default:
            WARN("unknown minor %u\n", func);
            Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Status = Status;
            break;
    }
    
    if (Status == STATUS_PENDING)
        goto exit;
    
    Irp->IoStatus.Status = Status;

//     if (Irp->UserIosb)
//         *Irp->UserIosb = Irp->IoStatus;
        
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    
exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
