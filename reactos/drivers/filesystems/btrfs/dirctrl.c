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
    UNICODE_STRING name;
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
    
    Status = open_fcb(Vcb, subvol, inode, type, NULL, NULL, &fcb, PagedPool, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("open_fcb returned %08x\n", Status);
        return 0;
    }
    
    ExAcquireResourceSharedLite(fcb->Header.Resource, TRUE);
    
    if (type == BTRFS_TYPE_DIRECTORY) {
        if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length < sizeof(ULONG))
            goto end;
        
        RtlCopyMemory(&tag, fcb->reparse_xattr.Buffer, sizeof(ULONG));
    } else {
        Status = read_file(fcb, (UINT8*)&tag, 0, sizeof(ULONG), &br, NULL, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08x\n", Status);
            goto end;
        }
        
        if (br < sizeof(ULONG))
            goto end;
    }
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);

    free_fcb(fcb);
    
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
                        } else if (fcb2->inode > inode)
                            break;
                        
                        le = le->Flink;
                    }
                }
                
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
                        
                        BOOL dotfile = de->name.Length > sizeof(WCHAR) && de->name.Buffer[0] == '.';

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
    
    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
        case FileBothDirectoryInformation:
        {
            FILE_BOTH_DIR_INFORMATION* fbdi = buf;
            
            TRACE("FileBothDirectoryInformation\n");
            
            needed = sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
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
            fbdi->FileNameLength = de->name.Length;
            fbdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fbdi->ShortNameLength = 0;
//             fibdi->ShortName[12];
            
            RtlCopyMemory(fbdi->FileName, de->name.Buffer, de->name.Length);
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileDirectoryInformation:
        {
            FILE_DIRECTORY_INFORMATION* fdi = buf;
            
            TRACE("FileDirectoryInformation\n");
            
            needed = sizeof(FILE_DIRECTORY_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
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
            fdi->FileNameLength = de->name.Length;
            
            RtlCopyMemory(fdi->FileName, de->name.Buffer, de->name.Length);
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }
            
        case FileFullDirectoryInformation:
        {
            FILE_FULL_DIR_INFORMATION* ffdi = buf;
            
            TRACE("FileFullDirectoryInformation\n");
            
            needed = sizeof(FILE_FULL_DIR_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
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
            ffdi->FileNameLength = de->name.Length;
            ffdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            
            RtlCopyMemory(ffdi->FileName, de->name.Buffer, de->name.Length);
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileIdBothDirectoryInformation:
        {
            FILE_ID_BOTH_DIR_INFORMATION* fibdi = buf;
            
            TRACE("FileIdBothDirectoryInformation\n");
            
            needed = sizeof(FILE_ID_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
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
            fibdi->FileNameLength = de->name.Length;
            fibdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fibdi->ShortNameLength = 0;
//             fibdi->ShortName[12];
            fibdi->FileId.QuadPart = make_file_id(r, inode);
            
            RtlCopyMemory(fibdi->FileName, de->name.Buffer, de->name.Length);
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileIdFullDirectoryInformation:
        {
            FILE_ID_FULL_DIR_INFORMATION* fifdi = buf;
            
            TRACE("FileIdFullDirectoryInformation\n");
            
            needed = sizeof(FILE_ID_FULL_DIR_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
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
            fifdi->FileNameLength = de->name.Length;
            fifdi->EaSize = atts & FILE_ATTRIBUTE_REPARSE_POINT ? get_reparse_tag(fcb->Vcb, r, inode, de->type, atts, Irp) : ealen;
            fifdi->FileId.QuadPart = make_file_id(r, inode);
            
            RtlCopyMemory(fifdi->FileName, de->name.Buffer, de->name.Length);
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileNamesInformation:
        {
            FILE_NAMES_INFORMATION* fni = buf;
            
            TRACE("FileNamesInformation\n");
            
            needed = sizeof(FILE_NAMES_INFORMATION) - sizeof(WCHAR) + de->name.Length;
            
            if (needed > *len) {
                TRACE("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
            
            fni->NextEntryOffset = 0;
            fni->FileIndex = 0;
            fni->FileNameLength = de->name.Length;
            
            RtlCopyMemory(fni->FileName, de->name.Buffer, de->name.Length);
            
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

static NTSTATUS STDCALL next_dir_entry(file_ref* fileref, UINT64* offset, dir_entry* de, dir_child** pdc, PIRP Irp) {
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
    
    de->key = dc->key;
    de->name = dc->name;
    de->type = dc->type;
    de->dir_entry_type = DirEntryType_File;
    
    *offset = dc->index + 1;
    *pdc = dc;
    
    return STATUS_SUCCESS;
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
    dir_child* dc = NULL;
    
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
    ExAcquireResourceSharedLite(&fcb->Vcb->fcb_lock, TRUE);
    
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
                goto end2;
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
                goto end2;
            }
        }
    }
    
    if (ccb->query_string.Buffer) {
        TRACE("query string = %.*S\n", ccb->query_string.Length / sizeof(WCHAR), ccb->query_string.Buffer);
    }
    
    newoffset = ccb->query_dir_offset;
    
    ExAcquireResourceSharedLite(&fileref->fcb->nonpaged->dir_children_lock, TRUE);
    
    Status = next_dir_entry(fileref, &newoffset, &de, &dc, Irp);
    
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
        UNICODE_STRING us;
        LIST_ENTRY* le;
        UINT32 hash;
        UINT8 c;
        
        us.Buffer = NULL;
        
        if (!ccb->case_sensitive) {
            Status = RtlUpcaseUnicodeString(&us, &ccb->query_string, TRUE);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                goto end;
            }
            
            hash = calc_crc32c(0xffffffff, (UINT8*)us.Buffer, us.Length);
        } else
            hash = calc_crc32c(0xffffffff, (UINT8*)ccb->query_string.Buffer, ccb->query_string.Length);
        
        c = hash >> 24;
        
        if (ccb->case_sensitive) {
            if (fileref->fcb->hash_ptrs[c]) {
                le = fileref->fcb->hash_ptrs[c];
                while (le != &fileref->fcb->dir_children_hash) {
                    dir_child* dc2 = CONTAINING_RECORD(le, dir_child, list_entry_hash);
                    
                    if (dc2->hash == hash) {
                        if (dc2->name.Length == ccb->query_string.Length && RtlCompareMemory(dc2->name.Buffer, ccb->query_string.Buffer, ccb->query_string.Length) == ccb->query_string.Length) {
                            found = TRUE;
                            
                            de.key = dc2->key;
                            de.name = dc2->name;
                            de.type = dc2->type;
                            de.dir_entry_type = DirEntryType_File;
                            
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
                            found = TRUE;
                            
                            de.key = dc2->key;
                            de.name = dc2->name;
                            de.type = dc2->type;
                            de.dir_entry_type = DirEntryType_File;
                            
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
            Status = next_dir_entry(fileref, &newoffset, &de, &dc, Irp);
            
            if (NT_SUCCESS(Status))
                ccb->query_dir_offset = newoffset;
            else {
                if (Status == STATUS_NO_MORE_FILES && initial)
                    Status = STATUS_NO_SUCH_FILE;
                
                goto end;
            }
        }
    }
    
    TRACE("file(0) = %.*S\n", de.name.Length / sizeof(WCHAR), de.name.Buffer);
    TRACE("offset = %u\n", ccb->query_dir_offset - 1);

    Status = query_dir_item(fcb, fileref, buf, &length, Irp, &de, fcb->subvol);

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
                newoffset = ccb->query_dir_offset;
                Status = next_dir_entry(fileref, &newoffset, &de, &dc, Irp);
                if (NT_SUCCESS(Status)) {
                    if (!has_wildcard || FsRtlIsNameInExpression(&ccb->query_string, &de.name, !ccb->case_sensitive, NULL)) {
                        curitem = (UINT8*)buf + IrpSp->Parameters.QueryDirectory.Length - length;
                        count++;
                        
                        TRACE("file(%u) %u = %.*S\n", count, curitem - (UINT8*)buf, de.name.Length / sizeof(WCHAR), de.name.Buffer);
                        TRACE("offset = %u\n", ccb->query_dir_offset - 1);
                        
                        status2 = query_dir_item(fcb, fileref, curitem, &length, Irp, &de, fcb->subvol);
                        
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
    
end2:
    ExReleaseResourceLite(&fcb->Vcb->fcb_lock);
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
