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
    char* name;
    ULONG namelen;
    UINT8 type;
    enum DirEntryType dir_entry_type;
} dir_entry;

static NTSTATUS STDCALL query_dir_item(fcb* fcb, void* buf, LONG* len, PIRP Irp, dir_entry* de, root* r) {
    PIO_STACK_LOCATION IrpSp;
    UINT32 needed;
    UINT64 inode;
    INODE_ITEM ii;
    NTSTATUS Status;
    ULONG stringlen;
    BOOL dotfile;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    if (de->key.obj_type == TYPE_ROOT_ITEM) { // subvol
        r = fcb->Vcb->roots;
        while (r && r->id != de->key.obj_id)
            r = r->next;
        
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
                
                le = fcb->children.Flink;
                while (le != &fcb->children) {
                    struct _fcb* c = CONTAINING_RECORD(le, struct _fcb, list_entry);
                    
                    if (c->subvol == r && c->inode == inode) {
                        ii = c->inode_item;
                        found = TRUE;
                        break;
                    }
                    
                    le = le->Flink;
                }
                
                if (!found) {
                    KEY searchkey;
                    traverse_ptr tp;
                    
                    searchkey.obj_id = inode;
                    searchkey.obj_type = TYPE_INODE_ITEM;
                    searchkey.offset = 0xffffffffffffffff;
                    
                    Status = find_item(fcb->Vcb, r, &tp, &searchkey, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        return Status;
                    }
                    
                    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                        ERR("could not find inode item for inode %llx in root %llx\n", inode, r->id);
                        free_traverse_ptr(&tp);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    RtlZeroMemory(&ii, sizeof(INODE_ITEM));
                    
                    if (tp.item->size > 0)
                        RtlCopyMemory(&ii, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
                    
                    free_traverse_ptr(&tp);
                }
                
                break;
            }
            
            case DirEntryType_Self:
                ii = fcb->inode_item;
                r = fcb->subvol;
                inode = fcb->inode;
                break;
                
            case DirEntryType_Parent:
                ii = fcb->par->inode_item;
                r = fcb->par->subvol;
                inode = fcb->par->inode;
                break;
        }
    }
    
    // FICs which return the filename
    if (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation ||
        IrpSp->Parameters.QueryDirectory.FileInformationClass == FileNamesInformation) {
        
        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de->name, de->namelen);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            return Status;
        }
    }
    
    dotfile = de->name[0] == '.' && (de->name[1] != '.' || de->name[2] != 0) && (de->name[1] != 0);
    
    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
        case FileBothDirectoryInformation:
        {
            FILE_BOTH_DIR_INFORMATION* fbdi = buf;
            
            TRACE("FileBothDirectoryInformation\n");
            
            needed = sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                WARN("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            fbdi->NextEntryOffset = 0;
            fbdi->FileIndex = 0;
            fbdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fbdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fbdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fbdi->ChangeTime.QuadPart = 0;
            fbdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fbdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fbdi->FileAttributes = get_file_attributes(fcb->Vcb, &ii, r, inode, de->type, dotfile, FALSE);
            fbdi->FileNameLength = stringlen;
            fbdi->EaSize = de->type == BTRFS_TYPE_SYMLINK ? IO_REPARSE_TAG_SYMLINK : 0;
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
                WARN("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            fdi->NextEntryOffset = 0;
            fdi->FileIndex = 0;
            fdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fdi->ChangeTime.QuadPart = 0;
            fdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fdi->FileAttributes = get_file_attributes(fcb->Vcb, &ii, r, inode, de->type, dotfile, FALSE);
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
                WARN("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
           
            ffdi->NextEntryOffset = 0;
            ffdi->FileIndex = 0;
            ffdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            ffdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            ffdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            ffdi->ChangeTime.QuadPart = 0;
            ffdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            ffdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            ffdi->FileAttributes = get_file_attributes(fcb->Vcb, &ii, r, inode, de->type, dotfile, FALSE);
            ffdi->FileNameLength = stringlen;
            ffdi->EaSize = de->type == BTRFS_TYPE_SYMLINK ? IO_REPARSE_TAG_SYMLINK : 0;
            
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
                WARN("buffer overflow - %u > %u\n", needed, *len);
                return STATUS_BUFFER_OVERFLOW;
            }
            
//             if (!buf)
//                 return STATUS_INVALID_POINTER;
            
            fibdi->NextEntryOffset = 0;
            fibdi->FileIndex = 0;
            fibdi->CreationTime.QuadPart = unix_time_to_win(&ii.otime);
            fibdi->LastAccessTime.QuadPart = unix_time_to_win(&ii.st_atime);
            fibdi->LastWriteTime.QuadPart = unix_time_to_win(&ii.st_mtime);
            fibdi->ChangeTime.QuadPart = 0;
            fibdi->EndOfFile.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_size;
            fibdi->AllocationSize.QuadPart = de->type == BTRFS_TYPE_SYMLINK ? 0 : ii.st_blocks;
            fibdi->FileAttributes = get_file_attributes(fcb->Vcb, &ii, r, inode, de->type, dotfile, FALSE);
            fibdi->FileNameLength = stringlen;
            fibdi->EaSize = de->type == BTRFS_TYPE_SYMLINK ? IO_REPARSE_TAG_SYMLINK : 0;
            fibdi->ShortNameLength = 0;
//             fibdi->ShortName[12];
            fibdi->FileId.QuadPart = inode;
            
            Status = RtlUTF8ToUnicodeN(fibdi->FileName, stringlen, &stringlen, de->name, de->namelen);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                return Status;
            }
            
            *len -= needed;
            
            return STATUS_SUCCESS;
        }

        case FileIdFullDirectoryInformation:
            FIXME("STUB: FileIdFullDirectoryInformation\n");
            break;

        case FileNamesInformation:
        {
            FILE_NAMES_INFORMATION* fni = buf;
            
            TRACE("FileNamesInformation\n");
            
            needed = sizeof(FILE_NAMES_INFORMATION) - sizeof(WCHAR) + stringlen;
            
            if (needed > *len) {
                WARN("buffer overflow - %u > %u\n", needed, *len);
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

static NTSTATUS STDCALL next_dir_entry(fcb* fcb, UINT64* offset, dir_entry* de, traverse_ptr* tp) {
    KEY searchkey;
    traverse_ptr next_tp;
    DIR_ITEM* di;
    NTSTATUS Status;
    
    if (fcb->par) { // don't return . and .. if root directory
        if (*offset == 0) {
            de->key.obj_id = fcb->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Self;
            de->name = ".";
            de->namelen = 1;
            de->type = BTRFS_TYPE_DIRECTORY;
            
            *offset = 1;
            
            return STATUS_SUCCESS;
        } else if (*offset == 1) {
            de->key.obj_id = fcb->par->inode;
            de->key.obj_type = TYPE_INODE_ITEM;
            de->key.offset = 0;
            de->dir_entry_type = DirEntryType_Parent;
            de->name = "..";
            de->namelen = 2;
            de->type = BTRFS_TYPE_DIRECTORY;
            
            *offset = 2;
            
            return STATUS_SUCCESS;
        }
    }
    
    if (!tp->tree) {
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = *offset;
        
        Status = find_item(fcb->Vcb, fcb->subvol, tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            free_traverse_ptr(tp);
            tp->tree = NULL;
            return Status;
        }
        
        TRACE("found item %llx,%x,%llx\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
        
        if (keycmp(&tp->item->key, &searchkey) == -1) {
            if (find_next_item(fcb->Vcb, tp, &next_tp, FALSE)) {
                free_traverse_ptr(tp);
                *tp = next_tp;
                
                TRACE("moving on to %llx,%x,%llx\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
            }
        }
        
        if (tp->item->key.obj_id != searchkey.obj_id || tp->item->key.obj_type != searchkey.obj_type || tp->item->key.offset < *offset) {
            free_traverse_ptr(tp);
            tp->tree = NULL;
            return STATUS_NO_MORE_FILES;
        }
        
        *offset = tp->item->key.offset + 1;
        
        di = (DIR_ITEM*)tp->item->data;
        
        if (tp->item->size < sizeof(DIR_ITEM) || tp->item->size < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, tp->item->size, sizeof(DIR_ITEM));
            
            free_traverse_ptr(tp);
            tp->tree = NULL;
            return STATUS_INTERNAL_ERROR;
        }
        
        de->key = di->key;
        de->name = di->name;
        de->namelen = di->n;
        de->type = di->type;
        de->dir_entry_type = DirEntryType_File;
        
        return STATUS_SUCCESS;
    } else {
        if (find_next_item(fcb->Vcb, tp, &next_tp, FALSE)) {
            if (next_tp.item->key.obj_type == TYPE_DIR_INDEX && next_tp.item->key.obj_id == tp->item->key.obj_id) {
                free_traverse_ptr(tp);
                *tp = next_tp;
                
                *offset = tp->item->key.offset + 1;
                
                di = (DIR_ITEM*)tp->item->data;
                
                if (tp->item->size < sizeof(DIR_ITEM) || tp->item->size < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
                    ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset, tp->item->size, sizeof(DIR_ITEM));
                    
                    free_traverse_ptr(&next_tp);
                    return STATUS_INTERNAL_ERROR;
                }
        
                de->key = di->key;
                de->name = di->name;
                de->namelen = di->n;
                de->type = di->type;
                de->dir_entry_type = DirEntryType_File;
                
                return STATUS_SUCCESS;
            } else {
                free_traverse_ptr(&next_tp);
                return STATUS_NO_MORE_FILES;
            }
        } else
            return STATUS_NO_MORE_FILES;
    }
}

static NTSTATUS STDCALL query_directory(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status, status2;
    fcb* fcb;
    ccb* ccb;
    void* buf;
    UINT8 *curitem, *lastitem;
    LONG length;
    ULONG count;
    BOOL has_wildcard = FALSE, specific_file = FALSE;
//     UINT64 num_reads_orig;
    traverse_ptr tp;
    dir_entry de;
    
    TRACE("query directory\n");
    
//     get_uid(); // TESTING
    
//     num_reads_orig = num_reads;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb = IrpSp->FileObject->FsContext;
    ccb = IrpSp->FileObject->FsContext2;
    
    acquire_tree_lock(fcb->Vcb, FALSE);
    
    TRACE("%.*S\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
    
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
    
    if (IrpSp->Flags & SL_RESTART_SCAN) {
        ccb->query_dir_offset = 0;
        
        if (ccb->query_string.Buffer) {
            RtlFreeUnicodeString(&ccb->query_string);
            ccb->query_string.Buffer = NULL;
        }
    }
    
    if (IrpSp->Parameters.QueryDirectory.FileName) {
//         int i;
//         WCHAR* us;
        
        TRACE("QD filename: %.*S\n", IrpSp->Parameters.QueryDirectory.FileName->Length / sizeof(WCHAR), IrpSp->Parameters.QueryDirectory.FileName->Buffer);
        
//         if (IrpSp->Parameters.QueryDirectory.FileName->Length > 1 || IrpSp->Parameters.QueryDirectory.FileName->Buffer[0] != '*') {
//             specific_file = TRUE;
//             for (i = 0; i < IrpSp->Parameters.QueryDirectory.FileName->Length; i++) {
//                 if (IrpSp->Parameters.QueryDirectory.FileName->Buffer[i] == '?' || IrpSp->Parameters.QueryDirectory.FileName->Buffer[i] == '*') {
//                     has_wildcard = TRUE;
//                     specific_file = FALSE;
//                 }
//             }
//         }
        has_wildcard = TRUE;

        if (ccb->query_string.Buffer)
            RtlFreeUnicodeString(&ccb->query_string);
        
//         us = ExAllocatePoolWithTag(PagedPool, IrpSp->Parameters.QueryDirectory.FileName->Length + sizeof(WCHAR), ALLOC_TAG);
//         RtlCopyMemory(us, IrpSp->Parameters.QueryDirectory.FileName->Buffer, IrpSp->Parameters.QueryDirectory.FileName->Length);
//         us[IrpSp->Parameters.QueryDirectory.FileName->Length / sizeof(WCHAR)] = 0;
        
//         ccb->query_string = ExAllocatePoolWithTag(NonPagedPool, utf16_to_utf8_len(us), ALLOC_TAG);
//         utf16_to_utf8(us, ccb->query_string);
        
//         ccb->query_string.Buffer = ExAllocatePoolWithTag(PagedPool, IrpSp->Parameters.QueryDirectory.FileName->Length, ALLOC_TAG);
//         RtlCopyMemory(ccb->query_string.Buffer, IrpSp->Parameters.QueryDirectory.FileName->Buffer,
//                       IrpSp->Parameters.QueryDirectory.FileName->Length);
//         ccb->query_string.Length = IrpSp->Parameters.QueryDirectory.FileName->Length;
//         ccb->query_string.MaximumLength = IrpSp->Parameters.QueryDirectory.FileName->Length;
            RtlUpcaseUnicodeString(&ccb->query_string, IrpSp->Parameters.QueryDirectory.FileName, TRUE);
          
        ccb->has_wildcard = has_wildcard;
        ccb->specific_file = specific_file;
        
//         ExFreePool(us);
    } else {
        has_wildcard = ccb->has_wildcard;
        specific_file = ccb->specific_file;
    }
    
    if (ccb->query_string.Buffer) {
        TRACE("query string = %.*S\n", ccb->query_string.Length / sizeof(WCHAR), ccb->query_string.Buffer);
    }
    
    tp.tree = NULL;
    Status = next_dir_entry(fcb, &ccb->query_dir_offset, &de, &tp);
    
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_NO_MORE_FILES && IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
            Status = STATUS_NO_SUCH_FILE;
        goto end;
    }

    // FIXME - make this work
//     if (specific_file) {
//         UINT64 filesubvol, fileinode;
//         WCHAR* us;
//         
//         us = ExAllocatePoolWithTag(NonPagedPool, IrpSp->Parameters.QueryDirectory.FileName->Length + sizeof(WCHAR), ALLOC_TAG);
//         RtlCopyMemory(us, IrpSp->Parameters.QueryDirectory.FileName->Buffer, IrpSp->Parameters.QueryDirectory.FileName->Length);
//         us[IrpSp->Parameters.QueryDirectory.FileName->Length / sizeof(WCHAR)] = 0;
//         
//         if (!find_file_in_dir(fcb->Vcb, us, fcb->subvolume, fcb->inode, &filesubvol, &fileinode)) {
//             ExFreePool(us);
//             return STATUS_NO_MORE_FILES;
//         }
//         
//         ExFreePool(us);
//     }
    
    buf = map_user_buffer(Irp);
    
    if (Irp->MdlAddress && !buf) {
        ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        if (tp.tree) free_traverse_ptr(&tp);
        goto end;
    }
    
    length = IrpSp->Parameters.QueryDirectory.Length;
    
//     if (specific_file) {
    if (has_wildcard) {
        WCHAR* uni_fn;
        ULONG stringlen;
        UNICODE_STRING di_uni_fn;
        
        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            if (tp.tree) free_traverse_ptr(&tp);
            goto end;
        }
        
        uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
        if (!uni_fn) {
            ERR("out of memory\n");
            if (tp.tree) free_traverse_ptr(&tp);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
        
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
            if (tp.tree) free_traverse_ptr(&tp);
            goto end;
        }
        
        di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
        di_uni_fn.Buffer = uni_fn;
        
        while (!FsRtlIsNameInExpression(&ccb->query_string, &di_uni_fn, TRUE, NULL)) {
            Status = next_dir_entry(fcb, &ccb->query_dir_offset, &de, &tp);
            
            ExFreePool(uni_fn);
            if (NT_SUCCESS(Status)) {
                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                    if (tp.tree) free_traverse_ptr(&tp);
                    goto end;
                }
                
                uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                if (!uni_fn) {
                    ERR("out of memory\n");
                    if (tp.tree) free_traverse_ptr(&tp);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                    ExFreePool(uni_fn);
                    if (tp.tree) free_traverse_ptr(&tp);
                    goto end;
                }
                
                di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
                di_uni_fn.Buffer = uni_fn;
            } else {
                if (tp.tree) free_traverse_ptr(&tp);

                if (Status == STATUS_NO_MORE_FILES && IrpSp->Flags & SL_RETURN_SINGLE_ENTRY)
                    Status = STATUS_NO_SUCH_FILE;
                
                goto end;
            }
        }
        
        ExFreePool(uni_fn);
    }
    
    TRACE("file(0) = %.*s\n", de.namelen, de.name);
    TRACE("offset = %u\n", ccb->query_dir_offset - 1);

    Status = query_dir_item(fcb, buf, &length, Irp, &de, fcb->subvol);
    
    count = 0;
    if (NT_SUCCESS(Status) && !(IrpSp->Flags & SL_RETURN_SINGLE_ENTRY) && !specific_file) {
        lastitem = (UINT8*)buf;
        
        while (length > 0) {
            switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
                case FileBothDirectoryInformation:
                case FileDirectoryInformation:
                case FileIdBothDirectoryInformation:
                case FileFullDirectoryInformation:
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
                
                Status = next_dir_entry(fcb, &ccb->query_dir_offset, &de, &tp);
                if (NT_SUCCESS(Status)) {
                    if (has_wildcard) {
                        ULONG stringlen;
                        
                        Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, de.name, de.namelen);
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                            if (tp.tree) free_traverse_ptr(&tp);
                            goto end;
                        }
                        
                        uni_fn = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                        if (!uni_fn) {
                            ERR("out of memory\n");
                            if (tp.tree) free_traverse_ptr(&tp);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(uni_fn, stringlen, &stringlen, de.name, de.namelen);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN returned %08x\n", Status);
                            ExFreePool(uni_fn);
                            if (tp.tree) free_traverse_ptr(&tp);
                            goto end;
                        }
                        
                        di_uni_fn.Length = di_uni_fn.MaximumLength = stringlen;
                        di_uni_fn.Buffer = uni_fn;
                    }
                    
                    if (!has_wildcard || FsRtlIsNameInExpression(&ccb->query_string, &di_uni_fn, TRUE, NULL)) {
                        curitem = (UINT8*)buf + IrpSp->Parameters.QueryDirectory.Length - length;
                        count++;
                        
                        TRACE("file(%u) %u = %.*s\n", count, curitem - (UINT8*)buf, de.namelen, de.name);
                        TRACE("offset = %u\n", ccb->query_dir_offset - 1);
                        
                        status2 = query_dir_item(fcb, curitem, &length, Irp, &de, fcb->subvol);
                        
                        if (NT_SUCCESS(status2)) {
                            ULONG* lastoffset = (ULONG*)lastitem;
                            
                            *lastoffset = (ULONG)(curitem - lastitem);
                            
                            lastitem = curitem;
                        } else {
                            if (uni_fn) ExFreePool(uni_fn);

                            break;
                        }
                    }
                    
                    if (uni_fn) {
                        ExFreePool(uni_fn);
                        uni_fn = NULL;
                    }
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
    
    if (tp.tree) free_traverse_ptr(&tp);
    
end:
    release_tree_lock(fcb->Vcb, FALSE);
    
//     TRACE("query directory performed %u reads\n", (UINT32)(num_reads-num_reads_orig));
    TRACE("returning %08x\n", Status);

    return Status;
}

static NTSTATUS STDCALL notify_change_directory(device_extension* Vcb, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    NTSTATUS Status;
//     WCHAR fn[MAX_PATH];
    
    TRACE("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n");
    
    acquire_tree_lock(fcb->Vcb, FALSE);
    
    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    // FIXME - raise exception if FCB marked for deletion?
    
    TRACE("%.*S\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
    
    FsRtlNotifyFullChangeDirectory(Vcb->NotifySync, &Vcb->DirNotifyList, FileObject->FsContext2, (PSTRING)&fcb->full_filename,
        IrpSp->Flags & SL_WATCH_TREE, FALSE, IrpSp->Parameters.NotifyDirectory.CompletionFilter, Irp, NULL, NULL);
    
    Status = STATUS_PENDING;
    
end:
    release_tree_lock(fcb->Vcb, FALSE);
    
    return Status;
}

NTSTATUS STDCALL drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG func;
    BOOL top_level;

    TRACE("directory control\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    Irp->IoStatus.Information = 0;
    
    func = IrpSp->MinorFunction;
    
    switch (func) {
        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            Status = notify_change_directory(DeviceObject->DeviceExtension, Irp);
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

    if (func != IRP_MN_NOTIFY_CHANGE_DIRECTORY || Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
