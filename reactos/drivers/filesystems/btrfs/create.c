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

#ifndef __REACTOS__
#include <sys/stat.h>
#endif /* __REACTOS__ */
#include "btrfs_drv.h"
#ifndef __REACTOS__
#include <winioctl.h>
#endif

extern PDEVICE_OBJECT devobj;

static WCHAR datastring[] = L"::$DATA";

fcb* create_fcb(POOL_TYPE pool_type) {
    fcb* fcb;
    
    fcb = ExAllocatePoolWithTag(PagedPool, sizeof(struct _fcb), ALLOC_TAG);
    if (!fcb) {
        ERR("out of memory\n");
        return NULL;
    }
    
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("allocating fcb %p\n", fcb);
#endif
    RtlZeroMemory(fcb, sizeof(struct _fcb));
    
    fcb->Header.NodeTypeCode = BTRFS_NODE_TYPE_FCB;
    fcb->Header.NodeByteSize = sizeof(struct _fcb);
    
    fcb->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _fcb_nonpaged), ALLOC_TAG);
    if (!fcb->nonpaged) {
        ERR("out of memory\n");
        ExFreePool(fcb);
        return NULL;
    }
    RtlZeroMemory(fcb->nonpaged, sizeof(struct _fcb_nonpaged));
    
    ExInitializeResourceLite(&fcb->nonpaged->paging_resource);
    fcb->Header.PagingIoResource = &fcb->nonpaged->paging_resource;
    
    ExInitializeFastMutex(&fcb->nonpaged->HeaderMutex);
    FsRtlSetupAdvancedHeader(&fcb->Header, &fcb->nonpaged->HeaderMutex);
    
    fcb->refcount = 1;
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fcb %p: refcount now %i\n", fcb, fcb->refcount);
#endif
    
    ExInitializeResourceLite(&fcb->nonpaged->resource);
    fcb->Header.Resource = &fcb->nonpaged->resource;
    
    ExInitializeResourceLite(&fcb->nonpaged->dir_children_lock);
    
    FsRtlInitializeFileLock(&fcb->lock, NULL, NULL);
    
    InitializeListHead(&fcb->extents);
    InitializeListHead(&fcb->hardlinks);
    
    InitializeListHead(&fcb->dir_children_index);
    InitializeListHead(&fcb->dir_children_hash);
    InitializeListHead(&fcb->dir_children_hash_uc);
    
    return fcb;
}

file_ref* create_fileref() {
    file_ref* fr;
    
    fr = ExAllocatePoolWithTag(PagedPool, sizeof(file_ref), ALLOC_TAG);
    if (!fr) {
        ERR("out of memory\n");
        return NULL;
    }
    
    RtlZeroMemory(fr, sizeof(file_ref));
    
    fr->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(file_ref_nonpaged), ALLOC_TAG);
    if (!fr->nonpaged) {
        ERR("out of memory\n");
        ExFreePool(fr);
        return NULL;
    }
    
    fr->refcount = 1;
    
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fileref %p: refcount now 1\n", fr);
#endif
    
    InitializeListHead(&fr->children);
    
    ExInitializeResourceLite(&fr->nonpaged->children_lock);
    
    return fr;
}

static NTSTATUS STDCALL find_file_in_dir(device_extension* Vcb, PUNICODE_STRING filename, fcb* fcb,
                                         root** subvol, UINT64* inode, dir_child** pdc, BOOL case_sensitive, PIRP Irp) {
    NTSTATUS Status;
    UNICODE_STRING fnus;
    UINT32 hash;
    LIST_ENTRY* le;
    UINT8 c;
    
    if (!case_sensitive) {
        Status = RtlUpcaseUnicodeString(&fnus, filename, TRUE);
        
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            return Status;
        }
    } else
        fnus = *filename;
    
    hash = calc_crc32c(0xffffffff, (UINT8*)fnus.Buffer, fnus.Length);
    
    c = hash >> 24;
    
    ExAcquireResourceSharedLite(&fcb->nonpaged->dir_children_lock, TRUE);
    
    if (case_sensitive) {
        if (!fcb->hash_ptrs[c]) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }
        
        le = fcb->hash_ptrs[c];
        while (le != &fcb->dir_children_hash) {
            dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_hash);
            
            if (dc->hash == hash) {
                if (dc->name.Length == fnus.Length && RtlCompareMemory(dc->name.Buffer, fnus.Buffer, fnus.Length) == fnus.Length) {
                    if (dc->key.obj_type == TYPE_ROOT_ITEM) {
                        LIST_ENTRY* le2;
                        
                        *subvol = NULL;
                        
                        le2 = fcb->Vcb->roots.Flink;
                        while (le2 != &fcb->Vcb->roots) {
                            root* r2 = CONTAINING_RECORD(le2, root, list_entry);
                            
                            if (r2->id == dc->key.obj_id) {
                                *subvol = r2;
                                break;
                            }
                            
                            le2 = le2->Flink;
                        }
                        
                        *inode = SUBVOL_ROOT_INODE;
                    } else {
                        *subvol = fcb->subvol;
                        *inode = dc->key.obj_id;
                    }
                    
                    *pdc = dc;

                    Status = STATUS_SUCCESS;
                    goto end;
                }
            } else if (dc->hash > hash) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto end;
            }
            
            le = le->Flink;
        }
    } else {
        if (!fcb->hash_ptrs_uc[c]) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }
        
        le = fcb->hash_ptrs_uc[c];
        while (le != &fcb->dir_children_hash_uc) {
            dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_hash_uc);
            
            if (dc->hash_uc == hash) {
                if (dc->name_uc.Length == fnus.Length && RtlCompareMemory(dc->name_uc.Buffer, fnus.Buffer, fnus.Length) == fnus.Length) {
                    if (dc->key.obj_type == TYPE_ROOT_ITEM) {
                        LIST_ENTRY* le2;
                        
                        *subvol = NULL;
                        
                        le2 = fcb->Vcb->roots.Flink;
                        while (le2 != &fcb->Vcb->roots) {
                            root* r2 = CONTAINING_RECORD(le2, root, list_entry);
                            
                            if (r2->id == dc->key.obj_id) {
                                *subvol = r2;
                                break;
                            }
                            
                            le2 = le2->Flink;
                        }
                        
                        *inode = SUBVOL_ROOT_INODE;
                    } else {
                        *subvol = fcb->subvol;
                        *inode = dc->key.obj_id;
                    }
                    
                    *pdc = dc;
                    
                    Status = STATUS_SUCCESS;
                    goto end;
                }
            } else if (dc->hash_uc > hash) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto end;
            }
            
            le = le->Flink;
        }
    }
    
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

end:
    ExReleaseResourceLite(&fcb->nonpaged->dir_children_lock);
    
    if (!case_sensitive)
        ExFreePool(fnus.Buffer);
    
    return Status;
}

static BOOL find_stream(device_extension* Vcb, fcb* fcb, PUNICODE_STRING stream, PUNICODE_STRING newstreamname, UINT32* hash, PANSI_STRING xattr, PIRP Irp) {
    NTSTATUS Status;
    ULONG utf8len;
    char* utf8;
    UINT32 crc32;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL success = FALSE, b;
    
    static char xapref[] = "user.";
    ULONG xapreflen = strlen(xapref);
    
    TRACE("(%p, %p, %.*S)\n", Vcb, fcb, stream->Length / sizeof(WCHAR), stream->Buffer);
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 1 returned %08x\n", Status);
        return FALSE;
    }
    
    TRACE("utf8len = %u\n", utf8len);
    
    utf8 = ExAllocatePoolWithTag(PagedPool, xapreflen + utf8len + 1, ALLOC_TAG);
    if (!utf8) {
        ERR("out of memory\n");
        goto end;
    }
    
    RtlCopyMemory(utf8, xapref, xapreflen);
    
    Status = RtlUnicodeToUTF8N(&utf8[xapreflen], utf8len, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 2 returned %08x\n", Status);
        goto end;
    }
    
    utf8len += xapreflen;
    utf8[utf8len] = 0;
    
    TRACE("utf8 = %s\n", utf8);
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8, utf8len);
    TRACE("crc32 = %08x\n", crc32);
    
    if ((crc32 == EA_DOSATTRIB_HASH && utf8len == strlen(EA_DOSATTRIB) && RtlCompareMemory(utf8, EA_DOSATTRIB, utf8len) == utf8len) || 
        (crc32 == EA_EA_HASH && utf8len == strlen(EA_EA) && RtlCompareMemory(utf8, EA_EA, utf8len) == utf8len)) {
        return FALSE;
    }
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!keycmp(tp.item->key, searchkey)) {
        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        } else {
            ULONG len = tp.item->size, xasize;
            DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
            
            TRACE("found match on hash\n");
            
            while (len > 0) {
                if (len < sizeof(DIR_ITEM) || len < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                if (RtlCompareMemory(di->name, utf8, utf8len) == utf8len) {
                    TRACE("found exact match for %s\n", utf8);
                    
                    *hash = tp.item->key.offset;
                    
                    xattr->Buffer = ExAllocatePoolWithTag(PagedPool, di->n + 1, ALLOC_TAG);
                    if (!xattr->Buffer) {
                        ERR("out of memory\n");
                        goto end;
                    }
                    
                    xattr->Length = xattr->MaximumLength = di->n;
                    RtlCopyMemory(xattr->Buffer, di->name, di->n);
                    xattr->Buffer[di->n] = 0;
                    
                    success = TRUE;
                    goto end;
                }
                
                xasize = sizeof(DIR_ITEM) - 1 + di->m + di->n;
                
                if (len > xasize) {
                    len -= xasize;
                    di = (DIR_ITEM*)&di->name[di->m + di->n];
                } else
                    break;
            }
        }
    }
    
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    do {
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_XATTR_ITEM && tp.item->key.offset != crc32) {
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            } else {
                ULONG len = tp.item->size, xasize;
                DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
                ULONG utf16len;
                
                TRACE("found xattr with hash %08x\n", (UINT32)tp.item->key.offset);
                
                while (len > 0) {
                    if (len < sizeof(DIR_ITEM) || len < sizeof(DIR_ITEM) - 1 + di->m + di->n) {
                        ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        break;
                    }
                    
                    if (di->n > xapreflen && RtlCompareMemory(di->name, xapref, xapreflen) == xapreflen) {
                        TRACE("found potential xattr %.*s\n", di->n, di->name);
                    }
                    
                    Status = RtlUTF8ToUnicodeN(NULL, 0, &utf16len, &di->name[xapreflen], di->n - xapreflen);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                    } else {
                        WCHAR* utf16 = ExAllocatePoolWithTag(PagedPool, utf16len, ALLOC_TAG);
                        if (!utf16) {
                            ERR("out of memory\n");
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(utf16, utf16len, &utf16len, &di->name[xapreflen], di->n - xapreflen);

                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                        } else {
                            UNICODE_STRING us;
                            
                            us.Buffer = utf16;
                            us.Length = us.MaximumLength = (USHORT)utf16len;
                            
                            if (FsRtlAreNamesEqual(stream, &us, TRUE, NULL)) {
                                TRACE("found case-insensitive match for %s\n", utf8);
                                
                                *newstreamname = us;
                                *hash = tp.item->key.offset;
                                
                                xattr->Buffer = ExAllocatePoolWithTag(PagedPool, di->n + 1, ALLOC_TAG);
                                if (!xattr->Buffer) {
                                    ERR("out of memory\n");
                                    ExFreePool(utf16);
                                    goto end;
                                }
                                
                                xattr->Length = xattr->MaximumLength = di->n;
                                RtlCopyMemory(xattr->Buffer, di->name, di->n);
                                xattr->Buffer[di->n] = 0;
                                
                                success = TRUE;
                                goto end;
                            }
                        }
                        
                        ExFreePool(utf16);
                    }
                    
                    xasize = sizeof(DIR_ITEM) - 1 + di->m + di->n;
                    
                    if (len > xasize) {
                        len -= xasize;
                        di = (DIR_ITEM*)&di->name[di->m + di->n];
                    } else
                        break;
                }
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
        if (b) {
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);
    
end:
    ExFreePool(utf8);
    
    return success;
}

static NTSTATUS split_path(PUNICODE_STRING path, UNICODE_STRING** parts, ULONG* num_parts, BOOL* stream) {
    ULONG len, i, j, np;
    BOOL has_stream;
    UNICODE_STRING* ps;
    WCHAR* buf;
    
    np = 1;
    
    len = path->Length / sizeof(WCHAR);
    if (len > 0 && (path->Buffer[len - 1] == '/' || path->Buffer[len - 1] == '\\'))
        len--;
    
    has_stream = FALSE;
    for (i = 0; i < len; i++) {
        if (path->Buffer[i] == '/' || path->Buffer[i] == '\\') {
            np++;
            has_stream = FALSE;
        } else if (path->Buffer[i] == ':') {
            has_stream = TRUE;
        }
    }
    
    if (has_stream)
        np++;
    
    ps = ExAllocatePoolWithTag(PagedPool, np * sizeof(UNICODE_STRING), ALLOC_TAG);
    if (!ps) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(ps, np * sizeof(UNICODE_STRING));
    
    buf = path->Buffer;
    
    j = 0;
    for (i = 0; i < len; i++) {
        if (path->Buffer[i] == '/' || path->Buffer[i] == '\\') {
            ps[j].Buffer = buf;
            ps[j].Length = (&path->Buffer[i] - buf) * sizeof(WCHAR);
            ps[j].MaximumLength = ps[j].Length;
            
            buf = &path->Buffer[i+1];
            j++;
        }
    }
    
    ps[j].Buffer = buf;
    ps[j].Length = (&path->Buffer[i] - buf) * sizeof(WCHAR);
    ps[j].MaximumLength = ps[j].Length;
    
    if (has_stream) {
        static WCHAR datasuf[] = {':','$','D','A','T','A',0};
        UNICODE_STRING dsus;
        
        dsus.Buffer = datasuf;
        dsus.Length = dsus.MaximumLength = wcslen(datasuf) * sizeof(WCHAR);
        
        for (i = 0; i < ps[j].Length / sizeof(WCHAR); i++) {
            if (ps[j].Buffer[i] == ':') {
                ps[j+1].Buffer = &ps[j].Buffer[i+1];
                ps[j+1].Length = ps[j].Length - (i * sizeof(WCHAR)) - sizeof(WCHAR);
                
                ps[j].Length = i * sizeof(WCHAR);
                ps[j].MaximumLength = ps[j].Length;
                
                j++;
                
                break;
            }
        }
        
        // FIXME - should comparison be case-insensitive?
        // remove :$DATA suffix
        if (ps[j].Length >= dsus.Length && RtlCompareMemory(&ps[j].Buffer[(ps[j].Length - dsus.Length)/sizeof(WCHAR)], dsus.Buffer, dsus.Length) == dsus.Length)
            ps[j].Length -= dsus.Length;
        
        if (ps[j].Length == 0) {
            np--;
            has_stream = FALSE;
        }
    }
    
    // if path is just stream name, remove first empty item
    if (has_stream && path->Length >= sizeof(WCHAR) && path->Buffer[0] == ':') {
        ps[0] = ps[1];
        np--;
    }

//     for (i = 0; i < np; i++) {
//         ERR("part %u: %u, (%.*S)\n", i, ps[i].Length, ps[i].Length / sizeof(WCHAR), ps[i].Buffer);
//     }
    
    *num_parts = np;
    *parts = ps;
    *stream = has_stream;
    
    return STATUS_SUCCESS;
}

// #ifdef DEBUG_FCB_REFCOUNTS
// static void print_fcbs(device_extension* Vcb) {
//     fcb* fcb = Vcb->fcbs;
//     
//     while (fcb) {
//         ERR("fcb %p (%.*S): refcount %u\n", fcb, fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fcb->refcount);
//         
//         fcb = fcb->next;
//     }
// }
// #endif

static file_ref* search_fileref_children(file_ref* dir, PUNICODE_STRING name, BOOL case_sensitive) {
    LIST_ENTRY* le;
    file_ref *c, *deleted = NULL;
    NTSTATUS Status;
    UNICODE_STRING ucus;
#ifdef DEBUG_FCB_REFCOUNTS
    ULONG rc;
#endif
    
    if (case_sensitive) {
        le = dir->children.Flink;
        while (le != &dir->children) {
            c = CONTAINING_RECORD(le, file_ref, list_entry);
            
            if (c->refcount > 0 && c->filepart.Length == name->Length &&
                RtlCompareMemory(c->filepart.Buffer, name->Buffer, name->Length) == name->Length) {
                if (c->deleted) {
                    deleted = c;
                } else {
#ifdef DEBUG_FCB_REFCOUNTS
                    rc = InterlockedIncrement(&c->refcount);
                    WARN("fileref %p: refcount now %i (%S)\n", c, rc, file_desc_fileref(c));
#else
                    InterlockedIncrement(&c->refcount);
#endif
                    return c;
                }
            }
            
            le = le->Flink;
        }
        
        goto end;
    }

    Status = RtlUpcaseUnicodeString(&ucus, name, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        return NULL;
    }
        
    le = dir->children.Flink;
    while (le != &dir->children) {
        c = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (c->refcount > 0 && c->filepart_uc.Length == ucus.Length &&
            RtlCompareMemory(c->filepart_uc.Buffer, ucus.Buffer, ucus.Length) == ucus.Length) {
            if (c->deleted) {
                deleted = c;
            } else {
#ifdef DEBUG_FCB_REFCOUNTS
                rc = InterlockedIncrement(&c->refcount);
                WARN("fileref %p: refcount now %i (%S)\n", c, rc, file_desc_fileref(c));
#else
                InterlockedIncrement(&c->refcount);
#endif
                ExFreePool(ucus.Buffer);
                
                return c;
            }
        }
        
        le = le->Flink;
    }
    
    ExFreePool(ucus.Buffer);
    
end:
    if (deleted)
        increase_fileref_refcount(deleted);
    
    return deleted;
}

NTSTATUS load_csum(device_extension* Vcb, UINT32* csum, UINT64 start, UINT64 length, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    UINT64 i, j;
    BOOL b;
    
    searchkey.obj_id = EXTENT_CSUM_ID;
    searchkey.obj_type = TYPE_EXTENT_CSUM;
    searchkey.offset = start;
    
    Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    i = 0;
    do {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            ULONG readlen;
            
            if (start < tp.item->key.offset)
                j = 0;
            else
                j = ((start - tp.item->key.offset) / Vcb->superblock.sector_size) + i;
            
            if (j * sizeof(UINT32) > tp.item->size || tp.item->key.offset > start + (i * Vcb->superblock.sector_size)) {
                ERR("checksum not found for %llx\n", start + (i * Vcb->superblock.sector_size));
                return STATUS_INTERNAL_ERROR;
            }
            
            readlen = min((tp.item->size / sizeof(UINT32)) - j, length - i);
            RtlCopyMemory(&csum[i], tp.item->data + (j * sizeof(UINT32)), readlen * sizeof(UINT32));
            i += readlen;
            
            if (i == length)
                break;
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
        
        if (b)
            tp = next_tp;
    } while (b);
    
    if (i < length) {
        ERR("could not read checksums: offset %llx, length %llx sectors\n", start, length);
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS load_dir_children(fcb* fcb, BOOL ignore_size, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    
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
    
    if (!ignore_size && fcb->inode_item.st_size == 0)
        return STATUS_SUCCESS;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = 2;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(tp.item->key, searchkey) == -1) {
        if (find_next_item(fcb->Vcb, &tp, &next_tp, FALSE, Irp)) {
            tp = next_tp;
            TRACE("moving on to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }
    
    while (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
        dir_child* dc;
        ULONG utf16len;
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            goto cont;
        }
        
        if (di->n == 0) {
            WARN("(%llx,%x,%llx): DIR_ITEM name length is zero\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            goto cont;
        }
        
        Status = RtlUTF8ToUnicodeN(NULL, 0, &utf16len, di->name, di->n);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
            goto cont;
        }

        dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
        if (!dc) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        dc->key = di->key;
        dc->index = tp.item->key.offset;
        dc->type = di->type;
        dc->fileref = NULL;
        
        dc->utf8.MaximumLength = dc->utf8.Length = di->n;
        dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, di->n, ALLOC_TAG);
        if (!dc->utf8.Buffer) {
            ERR("out of memory\n");
            ExFreePool(dc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(dc->utf8.Buffer, di->name, di->n);
        
        dc->name.MaximumLength = dc->name.Length = utf16len;
        dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, dc->name.MaximumLength, ALLOC_TAG);
        if (!dc->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        Status = RtlUTF8ToUnicodeN(dc->name.Buffer, utf16len, &utf16len, di->name, di->n);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc->name.Buffer);
            ExFreePool(dc);
            goto cont;
        }
        
        Status = RtlUpcaseUnicodeString(&dc->name_uc, &dc->name, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
            ExFreePool(dc->utf8.Buffer);
            ExFreePool(dc->name.Buffer);
            ExFreePool(dc);
            goto cont;
        }
        
        dc->hash = calc_crc32c(0xffffffff, (UINT8*)dc->name.Buffer, dc->name.Length);
        dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)dc->name_uc.Buffer, dc->name_uc.Length);
        
        InsertTailList(&fcb->dir_children_index, &dc->list_entry_index);
        
        insert_dir_child_into_hash_lists(fcb, dc);
        
cont:
        if (find_next_item(fcb->Vcb, &tp, &next_tp, FALSE, Irp))
            tp = next_tp;
        else
            break;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS open_fcb(device_extension* Vcb, root* subvol, UINT64 inode, UINT8 type, PANSI_STRING utf8, fcb* parent, fcb** pfcb, POOL_TYPE pooltype, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    fcb* fcb;
    BOOL atts_set = FALSE, sd_set = FALSE, no_data;
    LIST_ENTRY* lastle = NULL;
    EXTENT_DATA* ed = NULL;
    
    if (!IsListEmpty(&subvol->fcbs)) {
        LIST_ENTRY* le = subvol->fcbs.Flink;
                                
        while (le != &subvol->fcbs) {
            fcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
            
            if (fcb->inode == inode) {
                if (!fcb->ads) {
#ifdef DEBUG_FCB_REFCOUNTS
                    LONG rc = InterlockedIncrement(&fcb->refcount);

                    WARN("fcb %p: refcount now %i (subvol %llx, inode %llx)\n", fcb, rc, fcb->subvol->id, fcb->inode);
#else
                    InterlockedIncrement(&fcb->refcount);
#endif

                    *pfcb = fcb;
                    return STATUS_SUCCESS;
                }
            } else if (fcb->inode > inode) {
                lastle = le->Blink;
                break;
            }
            
            le = le->Flink;
        }
    }
    
    fcb = create_fcb(pooltype);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fcb->Vcb = Vcb;
    
    fcb->subvol = subvol;
    fcb->inode = inode;
    fcb->type = type;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        WARN("couldn't find INODE_ITEM for inode %llx in subvol %llx\n", inode, subvol->id);
        free_fcb(fcb);
        return STATUS_INVALID_PARAMETER;
    }
    
    if (tp.item->size > 0)
        RtlCopyMemory(&fcb->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
    
    if (fcb->type == 0) { // guess the type from the inode mode, if the caller doesn't know already
        if (fcb->inode_item.st_mode & __S_IFDIR)
            fcb->type = BTRFS_TYPE_DIRECTORY;
        else if (fcb->inode_item.st_mode & __S_IFCHR)
            fcb->type = BTRFS_TYPE_CHARDEV;
        else if (fcb->inode_item.st_mode & __S_IFBLK)
            fcb->type = BTRFS_TYPE_BLOCKDEV;
        else if (fcb->inode_item.st_mode & __S_IFIFO)
            fcb->type = BTRFS_TYPE_FIFO;
        else if (fcb->inode_item.st_mode & __S_IFLNK)
            fcb->type = BTRFS_TYPE_SYMLINK;
        else if (fcb->inode_item.st_mode & __S_IFSOCK)
            fcb->type = BTRFS_TYPE_SOCKET;
        else
            fcb->type = BTRFS_TYPE_FILE;
    }
    
    no_data = fcb->inode_item.st_size == 0 || (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK);
    
    while (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
        tp = next_tp;
        
        if (tp.item->key.obj_id > inode)
            break;
        
        if ((no_data && tp.item->key.obj_type > TYPE_XATTR_ITEM) || tp.item->key.obj_type > TYPE_EXTENT_DATA)
            break;
        
        if (fcb->inode_item.st_nlink > 1 && tp.item->key.obj_type == TYPE_INODE_REF) {
            ULONG len;
            INODE_REF* ir;
            
            len = tp.item->size;
            ir = (INODE_REF*)tp.item->data;
            
            while (len >= sizeof(INODE_REF) - 1) {
                hardlink* hl;
                ULONG stringlen;
                
                hl = ExAllocatePoolWithTag(pooltype, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                hl->parent = tp.item->key.offset;
                hl->index = ir->index;
                
                hl->utf8.Length = hl->utf8.MaximumLength = ir->n;
                
                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(pooltype, hl->utf8.MaximumLength, ALLOC_TAG);
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
                    hl->name.Buffer = ExAllocatePoolWithTag(pooltype, hl->name.MaximumLength, ALLOC_TAG);
                    
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
                
                len -= sizeof(INODE_REF) - 1 + ir->n;
                ir = (INODE_REF*)&ir->name[ir->n];
            }
        } else if (fcb->inode_item.st_nlink > 1 && tp.item->key.obj_type == TYPE_INODE_EXTREF) {
            ULONG len;
            INODE_EXTREF* ier;
            
            len = tp.item->size;
            ier = (INODE_EXTREF*)tp.item->data;
            
            while (len >= sizeof(INODE_EXTREF) - 1) {
                hardlink* hl;
                ULONG stringlen;
                
                hl = ExAllocatePoolWithTag(pooltype, sizeof(hardlink), ALLOC_TAG);
                if (!hl) {
                    ERR("out of memory\n");
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                hl->parent = ier->dir;
                hl->index = ier->index;
                
                hl->utf8.Length = hl->utf8.MaximumLength = ier->n;
                
                if (hl->utf8.Length > 0) {
                    hl->utf8.Buffer = ExAllocatePoolWithTag(pooltype, hl->utf8.MaximumLength, ALLOC_TAG);
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
                    hl->name.Buffer = ExAllocatePoolWithTag(pooltype, hl->name.MaximumLength, ALLOC_TAG);
                    
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
                
                len -= sizeof(INODE_EXTREF) - 1 + ier->n;
                ier = (INODE_EXTREF*)&ier->name[ier->n];
            }
        } else if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
            if (tp.item->size < sizeof(DIR_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
                continue;
            }
            
            if (tp.item->key.offset == EA_REPARSE_HASH) {
                UINT8* xattrdata;
                UINT16 xattrlen;
                
                if (extract_xattr(tp.item->data, tp.item->size, EA_REPARSE, &xattrdata, &xattrlen)) {
                    fcb->reparse_xattr.Buffer = (char*)xattrdata;
                    fcb->reparse_xattr.Length = fcb->reparse_xattr.MaximumLength = xattrlen;
                }
            } else if (tp.item->key.offset == EA_EA_HASH) {
                UINT8* eadata;
                UINT16 ealen;
                
                if (extract_xattr(tp.item->data, tp.item->size, EA_EA, &eadata, &ealen)) {
                    ULONG offset;
                    
                    Status = IoCheckEaBufferValidity((FILE_FULL_EA_INFORMATION*)eadata, ealen, &offset);
                    
                    if (!NT_SUCCESS(Status)) {
                        WARN("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
                        ExFreePool(eadata);
                    } else {
                        FILE_FULL_EA_INFORMATION* eainfo;
                        fcb->ea_xattr.Buffer = (char*)eadata;
                        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = ealen;
                        
                        fcb->ealen = 4;
                        
                        // calculate ealen
                        eainfo = (FILE_FULL_EA_INFORMATION*)eadata;
                        do {
                            fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;
                            
                            if (eainfo->NextEntryOffset == 0)
                                break;
                            
                            eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
                        } while (TRUE);
                    }
                }
            } else if (tp.item->key.offset == EA_DOSATTRIB_HASH) {
                UINT8* xattrdata;
                UINT16 xattrlen;
                
                if (extract_xattr(tp.item->data, tp.item->size, EA_DOSATTRIB, &xattrdata, &xattrlen)) {
                    if (get_file_attributes_from_xattr((char*)xattrdata, xattrlen, &fcb->atts)) {
                        atts_set = TRUE;
                        
                        if (fcb->type == BTRFS_TYPE_DIRECTORY)
                            fcb->atts |= FILE_ATTRIBUTE_DIRECTORY;
                        else if (fcb->type == BTRFS_TYPE_SYMLINK)
                            fcb->atts |= FILE_ATTRIBUTE_REPARSE_POINT;
                    }
                    
                    ExFreePool(xattrdata);
                }
            } else if (tp.item->key.offset == EA_NTACL_HASH) {
                UINT16 buflen;
                
                if (extract_xattr(tp.item->data, tp.item->size, EA_NTACL, (UINT8**)&fcb->sd, &buflen)) {
                    if (get_sd_from_xattr(fcb, buflen)) {
                        sd_set = TRUE;
                    } else
                        ExFreePool(fcb->sd);
                }
            }
        } else if (tp.item->key.obj_type == TYPE_EXTENT_DATA) {
            extent* ext;
            BOOL unique = FALSE;
            
            ed = (EXTENT_DATA*)tp.item->data;
            
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %llx bytes, expected at least %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                    tp.item->size, sizeof(EXTENT_DATA));
                
                free_fcb(fcb);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];
                
                if (tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    ERR("(%llx,%x,%llx) was %llx bytes, expected at least %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                        tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
                
                    free_fcb(fcb);
                    return STATUS_INTERNAL_ERROR;
                }
                
                if (ed2->address == 0 && ed2->size == 0) // sparse
                    continue;
                
                if (ed2->size != 0 && is_tree_unique(Vcb, tp.tree, Irp))
                    unique = is_extent_unique(Vcb, ed2->address, ed2->size, Irp);
            }
            
            ext = ExAllocatePoolWithTag(pooltype, sizeof(extent), ALLOC_TAG);
            if (!ext) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            ext->data = ExAllocatePoolWithTag(pooltype, tp.item->size, ALLOC_TAG);
            if (!ext->data) {
                ERR("out of memory\n");
                ExFreePool(ext);
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            ext->offset = tp.item->key.offset;
            RtlCopyMemory(ext->data, tp.item->data, tp.item->size);
            ext->datalen = tp.item->size;
            ext->unique = unique;
            ext->ignore = FALSE;
            ext->inserted = FALSE;
            
            if (ed->type == EXTENT_TYPE_REGULAR && !(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];
                UINT64 len;
                
                len = (ed->compression == BTRFS_COMPRESSION_NONE ? ed2->num_bytes : ed2->size) / Vcb->superblock.sector_size;
                
                ext->csum = ExAllocatePoolWithTag(NonPagedPool, len * sizeof(UINT32), ALLOC_TAG);
                if (!ext->csum) {
                    ERR("out of memory\n");
                    ExFreePool(ext);
                    free_fcb(fcb);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                Status = load_csum(Vcb, ext->csum, ed2->address + (ed->compression == BTRFS_COMPRESSION_NONE ? ed2->offset : 0), len, Irp);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("load_csum returned %08x\n", Status);
                    ExFreePool(ext);
                    free_fcb(fcb);
                    return Status;
                }
            } else
                ext->csum = NULL;
            
            InsertTailList(&fcb->extents, &ext->list_entry);
        }
    }
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        Status = load_dir_children(fcb, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("load_dir_children returned %08x\n", Status);
            free_fcb(fcb);
            return Status;
        }
    }
    
    if (no_data) {
        fcb->Header.AllocationSize.QuadPart = 0;
        fcb->Header.FileSize.QuadPart = 0;
        fcb->Header.ValidDataLength.QuadPart = 0;
    } else {
        if (ed && ed->type == EXTENT_TYPE_INLINE)
            fcb->Header.AllocationSize.QuadPart = fcb->inode_item.st_size;
        else
            fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
        
        fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
        fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
    }
    
    if (!atts_set)
        fcb->atts = get_file_attributes(Vcb, &fcb->inode_item, fcb->subvol, fcb->inode, fcb->type, utf8 && utf8->Buffer[0] == '.', TRUE, Irp);
    
    if (!sd_set)
        fcb_get_sd(fcb, parent, FALSE, Irp);
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY && fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT && fcb->reparse_xattr.Length == 0) {
        fcb->atts &= ~FILE_ATTRIBUTE_REPARSE_POINT;
        
        if (!Vcb->readonly && !(subvol->root_item.flags & BTRFS_SUBVOL_READONLY)) {
            fcb->atts_changed = TRUE;
            mark_fcb_dirty(fcb);
        }
    }
    
    if (lastle)
        InsertHeadList(lastle, &fcb->list_entry);
    else
        InsertTailList(&subvol->fcbs, &fcb->list_entry);
    
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);
    
    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    *pfcb = fcb;
    return STATUS_SUCCESS;
}

NTSTATUS open_fcb_stream(device_extension* Vcb, root* subvol, UINT64 inode, ANSI_STRING* xattr,
                         UINT32 streamhash, fcb* parent, fcb** pfcb, PIRP Irp) {
    fcb* fcb;
    UINT8* xattrdata;
    UINT16 xattrlen, overhead;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    LIST_ENTRY* lastle = NULL;
    
    if (!IsListEmpty(&subvol->fcbs)) {
        LIST_ENTRY* le = subvol->fcbs.Flink;
                                
        while (le != &subvol->fcbs) {
            fcb = CONTAINING_RECORD(le, struct _fcb, list_entry);
            
            if (fcb->inode == inode) {
                if (fcb->ads && fcb->adsxattr.Length == xattr->Length &&
                    RtlCompareMemory(fcb->adsxattr.Buffer, xattr->Buffer, fcb->adsxattr.Length) == fcb->adsxattr.Length) {
#ifdef DEBUG_FCB_REFCOUNTS
                    LONG rc = InterlockedIncrement(&fcb->refcount);

                    WARN("fcb %p: refcount now %i (subvol %llx, inode %llx)\n", fcb, rc, fcb->subvol->id, fcb->inode);
#else
                    InterlockedIncrement(&fcb->refcount);
#endif

                    *pfcb = fcb;
                    return STATUS_SUCCESS;
                }
            } else if (fcb->inode > inode) {
                lastle = le->Blink;
                break;
            }
                            
            le = le->Flink;
        }
    }
    
    fcb = create_fcb(PagedPool);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
      
    if (!get_xattr(Vcb, parent->subvol, parent->inode, xattr->Buffer, streamhash, &xattrdata, &xattrlen, Irp)) {
        ERR("get_xattr failed\n");
        free_fcb(fcb);
        return STATUS_INTERNAL_ERROR;
    }

    fcb->Vcb = Vcb;
    
    fcb->subvol = parent->subvol;
    fcb->inode = parent->inode;
    fcb->type = parent->type;
    fcb->ads = TRUE;
    fcb->adshash = streamhash;
    fcb->adsxattr = *xattr;
    
    // find XATTR_ITEM overhead and hence calculate maximum length
    
    searchkey.obj_id = parent->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = streamhash;

    Status = find_item(Vcb, parent->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    if (keycmp(tp.item->key, searchkey)) {
        ERR("error - could not find key for xattr\n");
        free_fcb(fcb);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size < xattrlen) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, xattrlen);
        free_fcb(fcb);
        return STATUS_INTERNAL_ERROR;
    }
    
    overhead = tp.item->size - xattrlen;
    
    fcb->adsmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - overhead;
    
    fcb->adsdata.Buffer = (char*)xattrdata;
    fcb->adsdata.Length = fcb->adsdata.MaximumLength = xattrlen;
    
    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = xattrlen;
    fcb->Header.FileSize.QuadPart = xattrlen;
    fcb->Header.ValidDataLength.QuadPart = xattrlen;
    
    TRACE("stream found: size = %x, hash = %08x\n", xattrlen, fcb->adshash);
    
    if (lastle)
        InsertHeadList(lastle, &fcb->list_entry);
    else
        InsertTailList(&fcb->subvol->fcbs, &fcb->list_entry);
    
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);
    
    *pfcb = fcb;
    
    return STATUS_SUCCESS;
}

void insert_fileref_child(file_ref* parent, file_ref* child, BOOL do_lock) {
    if (do_lock)
        ExAcquireResourceExclusiveLite(&parent->nonpaged->children_lock, TRUE);
    
    if (IsListEmpty(&parent->children))
        InsertTailList(&parent->children, &child->list_entry);
    else {
        LIST_ENTRY* le = parent->children.Flink;
        file_ref* fr1 = CONTAINING_RECORD(le, file_ref, list_entry);
        
        if (child->index < fr1->index)
            InsertHeadList(&parent->children, &child->list_entry);
        else {
            while (le != &parent->children) {
                file_ref* fr2 = (le->Flink == &parent->children) ? NULL : CONTAINING_RECORD(le->Flink, file_ref, list_entry);
                
                if (child->index >= fr1->index && (!fr2 || fr2->index > child->index)) {
                    InsertHeadList(&fr1->list_entry, &child->list_entry);
                    break;
                }
                
                fr1 = fr2;
                le = le->Flink;
            }
        }
    }
    
    if (do_lock)
        ExReleaseResourceLite(&parent->nonpaged->children_lock);
}

static NTSTATUS open_fileref_child(device_extension* Vcb, file_ref* sf, PUNICODE_STRING name, BOOL case_sensitive, BOOL lastpart, BOOL streampart,
                                   POOL_TYPE pooltype, file_ref** psf2, PIRP Irp) {
    NTSTATUS Status;
    file_ref* sf2;
        
    if (streampart) {
        UNICODE_STRING streamname;
        ANSI_STRING xattr;
        UINT32 streamhash;
        
        sf2 = search_fileref_children(sf, name, case_sensitive);
        
        if (sf2) {
            if (sf2->deleted) {
                TRACE("element in path has been deleted\n");
                free_fileref(sf2);
                return lastpart ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_OBJECT_PATH_NOT_FOUND;
            }
            
            *psf2 = sf2;
    
            return STATUS_SUCCESS;
        }
        
        streamname.Buffer = NULL;
        streamname.Length = streamname.MaximumLength = 0;
        xattr.Buffer = NULL;
        xattr.Length = xattr.MaximumLength = 0;
        
        // FIXME - check if already opened
        
        if (!find_stream(Vcb, sf->fcb, name, &streamname, &streamhash, &xattr, Irp)) {
            TRACE("could not find stream %.*S\n", name->Length / sizeof(WCHAR), name->Buffer);
            
            return STATUS_OBJECT_NAME_NOT_FOUND;
        } else {
            fcb* fcb;

            if (streamhash == EA_DOSATTRIB_HASH && xattr.Length == strlen(EA_DOSATTRIB) &&
                RtlCompareMemory(xattr.Buffer, EA_DOSATTRIB, xattr.Length) == xattr.Length) {
                WARN("not allowing user.DOSATTRIB to be opened as stream\n");
            
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }
            
            Status = open_fcb_stream(Vcb, sf->fcb->subvol, sf->fcb->inode, &xattr, streamhash, sf->fcb, &fcb, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("open_fcb_stream returned %08x\n", Status);
                return Status;
            }
            
            sf2 = create_fileref();
            if (!sf2) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sf2->fcb = fcb;

            if (streamname.Buffer) // case has changed
                sf2->filepart = streamname;
            else {
                sf2->filepart.MaximumLength = sf2->filepart.Length = name->Length;
                sf2->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, sf2->filepart.MaximumLength, ALLOC_TAG);
                if (!sf2->filepart.Buffer) {
                    ERR("out of memory\n");
                    free_fileref(sf2);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(sf2->filepart.Buffer, name->Buffer, name->Length);
            }
            
            Status = RtlUpcaseUnicodeString(&sf2->filepart_uc, &sf2->filepart, TRUE);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
                free_fileref(sf2);
                return Status;
            }
            
            // FIXME - make sure all functions know that ADS FCBs won't have a valid SD or INODE_ITEM

            sf2->parent = (struct _file_ref*)sf;
            insert_fileref_child(sf, sf2, TRUE);
            
            increase_fileref_refcount(sf);
        }
    } else {
        root* subvol;
        UINT64 inode;
        dir_child* dc;
        
        Status = find_file_in_dir(Vcb, name, sf->fcb, &subvol, &inode, &dc, case_sensitive, Irp);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            TRACE("could not find %.*S\n", name->Length / sizeof(WCHAR), name->Buffer);

            return lastpart ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_OBJECT_PATH_NOT_FOUND;
        } else if (!NT_SUCCESS(Status)) {
            ERR("find_file_in_dir returned %08x\n", Status);
            return Status;
        } else {
            fcb* fcb;
            
            if (dc->fileref) {
                if (!lastpart && dc->type != BTRFS_TYPE_DIRECTORY) {
                    WARN("passed path including file as subdirectory\n");
                    return STATUS_OBJECT_PATH_NOT_FOUND;
                }
                
                InterlockedIncrement(&dc->fileref->refcount);
                *psf2 = dc->fileref;
                return STATUS_SUCCESS;
            }
            
            Status = open_fcb(Vcb, subvol, inode, dc->type, &dc->utf8, sf->fcb, &fcb, pooltype, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("open_fcb returned %08x\n", Status);
                return Status;
            }
            
            if (dc->type != BTRFS_TYPE_DIRECTORY && !lastpart && !(fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)) {
                WARN("passed path including file as subdirectory\n");
                free_fcb(fcb);
                return STATUS_OBJECT_PATH_NOT_FOUND;
            }

            sf2 = create_fileref();
            if (!sf2) {
                ERR("out of memory\n");
                free_fcb(fcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sf2->fcb = fcb;
            
            if (dc->type == BTRFS_TYPE_DIRECTORY)
                fcb->fileref = sf2;
            
            sf2->index = dc->index;
            sf2->dc = dc;
            dc->fileref = sf2;
            
            sf2->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, dc->utf8.Length, ALLOC_TAG);
            if (!sf2->utf8.Buffer) {
                ERR("out of memory\n");
                free_fileref(sf2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sf2->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, dc->name.Length, ALLOC_TAG);
            if (!sf2->filepart.Buffer) {
                ERR("out of memory\n");
                free_fileref(sf2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sf2->filepart_uc.Buffer = ExAllocatePoolWithTag(PagedPool, dc->name_uc.Length, ALLOC_TAG);
            if (!sf2->filepart_uc.Buffer) {
                ERR("out of memory\n");
                free_fileref(sf2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sf2->utf8.Length = sf2->utf8.MaximumLength = dc->utf8.Length;
            RtlCopyMemory(sf2->utf8.Buffer, dc->utf8.Buffer, dc->utf8.Length);
            
            sf2->filepart.Length = sf2->filepart.MaximumLength = dc->name.Length;
            RtlCopyMemory(sf2->filepart.Buffer, dc->name.Buffer, dc->name.Length);
            
            sf2->filepart_uc.Length = sf2->filepart_uc.MaximumLength = dc->name_uc.Length;
            RtlCopyMemory(sf2->filepart_uc.Buffer, dc->name_uc.Buffer, dc->name_uc.Length);
            
            sf2->parent = (struct _file_ref*)sf;
            
            insert_fileref_child(sf, sf2, TRUE);
            
            increase_fileref_refcount(sf);
        }
    }
    
    *psf2 = sf2;
    
    return STATUS_SUCCESS;
}

NTSTATUS open_fileref(device_extension* Vcb, file_ref** pfr, PUNICODE_STRING fnus, file_ref* related, BOOL parent, USHORT* parsed, ULONG* fn_offset,
                      POOL_TYPE pooltype, BOOL case_sensitive, PIRP Irp) {
    UNICODE_STRING fnus2;
    file_ref *dir, *sf, *sf2;
    ULONG i, num_parts;
    UNICODE_STRING* parts = NULL;
    BOOL has_stream;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p, %u, %p)\n", Vcb, pfr, related, parent, parsed);

#ifdef DEBUG    
    if (!ExIsResourceAcquiredExclusiveLite(&Vcb->fcb_lock) && !ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
        ERR("fcb_lock not acquired exclusively\n");
        int3;
    }
#endif
    
    if (Vcb->removing || Vcb->locked)
        return STATUS_ACCESS_DENIED;
    
    fnus2 = *fnus;
    
    if (fnus2.Length < sizeof(WCHAR) && !related) {
        ERR("error - fnus was too short\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (related && fnus->Length == 0) {
        increase_fileref_refcount(related);
        
        *pfr = related;
        return STATUS_SUCCESS;
    }
    
    if (related) {
        dir = related;
    } else {
        if (fnus2.Buffer[0] != '\\') {
            ERR("error - filename %.*S did not begin with \\\n", fnus2.Length / sizeof(WCHAR), fnus2.Buffer);
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }
        
        if (fnus2.Length == sizeof(WCHAR)) {
            if (Vcb->root_fileref->open_count == 0) { // don't allow root to be opened on unmounted FS
                ULONG cc;
                IO_STATUS_BLOCK iosb;
                
                Status = dev_ioctl(first_device(Vcb)->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), TRUE, &iosb);
                
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            
            increase_fileref_refcount(Vcb->root_fileref);
            *pfr = Vcb->root_fileref;
            
            if (fn_offset)
                *fn_offset = 0;
            
            return STATUS_SUCCESS;
        }
        
        dir = Vcb->root_fileref;
        
        fnus2.Buffer++;
        fnus2.Length -= sizeof(WCHAR);
        fnus2.MaximumLength -= sizeof(WCHAR);
    }
    
    if (dir->fcb->type != BTRFS_TYPE_DIRECTORY && (fnus->Length < sizeof(WCHAR) || fnus->Buffer[0] != ':')) {
        WARN("passed related fileref which isn't a directory (%S) (fnus = %.*S)\n",
             file_desc_fileref(related), fnus->Length / sizeof(WCHAR), fnus->Buffer);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }
    
    if (fnus->Length == 0) {
        num_parts = 0;
    } else if (fnus->Length == wcslen(datastring) * sizeof(WCHAR) &&
               RtlCompareMemory(fnus->Buffer, datastring, wcslen(datastring) * sizeof(WCHAR)) == wcslen(datastring) * sizeof(WCHAR)) {
        num_parts = 0;
    } else {
        Status = split_path(&fnus2, &parts, &num_parts, &has_stream);
        if (!NT_SUCCESS(Status)) {
            ERR("split_path returned %08x\n", Status);
            return Status;
        }
    }
    
    sf = dir;
    increase_fileref_refcount(dir);
    
    if (parent) {
        num_parts--;
        
        if (has_stream && num_parts > 0) {
            num_parts--;
            has_stream = FALSE;
        }
    }
    
    if (num_parts == 0) {
        Status = STATUS_SUCCESS;
        *pfr = dir;
        
        if (fn_offset)
            *fn_offset = 0;
            
        goto end2;
    }
    
    for (i = 0; i < num_parts; i++) {
        BOOL lastpart = (i == num_parts-1) || (i == num_parts-2 && has_stream);
        
        Status = open_fileref_child(Vcb, sf, &parts[i], case_sensitive, lastpart, has_stream && i == num_parts - 1, pooltype, &sf2, Irp);
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_OBJECT_PATH_NOT_FOUND || Status == STATUS_OBJECT_NAME_NOT_FOUND)
                TRACE("open_fileref_child returned %08x\n", Status);
            else
                ERR("open_fileref_child returned %08x\n", Status);
            
            goto end;
        }
        
        if (i == num_parts - 1) {
            if (fn_offset)
                *fn_offset = parts[has_stream ? (num_parts - 2) : (num_parts - 1)].Buffer - fnus->Buffer;
            
            break;
        }
        
        if (sf2->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) {
            Status = STATUS_REPARSE;
            
            if (parsed)
                *parsed = (parts[i+1].Buffer - fnus->Buffer - 1) * sizeof(WCHAR);
            
            break;
        }
        
        free_fileref(sf);
        sf = sf2;
    }
    
    if (Status != STATUS_REPARSE)
        Status = STATUS_SUCCESS;
    *pfr = sf2;
    
end:
    free_fileref(sf);
    
end2:
    if (parts)
        ExFreePool(parts);
    
    TRACE("returning %08x\n", Status);
    
    return Status;
}

NTSTATUS fcb_get_last_dir_index(fcb* fcb, UINT64* index, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, prev_tp;
    NTSTATUS Status;
    
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    if (fcb->last_dir_index != 0) {
        *index = fcb->last_dir_index;
        fcb->last_dir_index++;
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_DIR_INDEX + 1;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type >= searchkey.obj_type)) {
        if (find_prev_item(fcb->Vcb, &tp, &prev_tp, FALSE, Irp))
            tp = prev_tp;
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_DIR_INDEX) {
        fcb->last_dir_index = tp.item->key.offset + 1;
    } else
        fcb->last_dir_index = 2;
    
    *index = fcb->last_dir_index;
    fcb->last_dir_index++;
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(fcb->Header.Resource);
    
    return Status;
}

NTSTATUS add_dir_child(fcb* fcb, UINT64 inode, BOOL subvol, UINT64 index, PANSI_STRING utf8, PUNICODE_STRING name, PUNICODE_STRING name_uc, UINT8 type, dir_child** pdc) {
    dir_child* dc;
    
    dc = ExAllocatePoolWithTag(PagedPool, sizeof(dir_child), ALLOC_TAG);
    if (!dc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    dc->utf8.Buffer = ExAllocatePoolWithTag(PagedPool, utf8->Length, ALLOC_TAG);
    if (!dc->utf8.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    dc->name.Buffer = ExAllocatePoolWithTag(PagedPool, name->Length, ALLOC_TAG);
    if (!dc->name.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    dc->name_uc.Buffer = ExAllocatePoolWithTag(PagedPool, name_uc->Length, ALLOC_TAG);
    if (!dc->name_uc.Buffer) {
        ERR("out of memory\n");
        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc->name.Buffer);
        ExFreePool(dc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    dc->key.obj_id = inode;
    dc->key.obj_type = subvol ? TYPE_ROOT_ITEM : TYPE_INODE_ITEM;
    dc->key.offset = 0;
    dc->index = index;
    dc->type = type;
    dc->fileref = NULL;
    
    dc->utf8.Length = dc->utf8.MaximumLength = utf8->Length;
    RtlCopyMemory(dc->utf8.Buffer, utf8->Buffer, utf8->Length);
    
    dc->name.Length = dc->name.MaximumLength = name->Length;
    RtlCopyMemory(dc->name.Buffer, name->Buffer, name->Length);
    
    dc->name_uc.Length = dc->name_uc.MaximumLength = name_uc->Length;
    RtlCopyMemory(dc->name_uc.Buffer, name_uc->Buffer, name_uc->Length);
    
    dc->hash = calc_crc32c(0xffffffff, (UINT8*)dc->name.Buffer, dc->name.Length);
    dc->hash_uc = calc_crc32c(0xffffffff, (UINT8*)dc->name_uc.Buffer, dc->name_uc.Length);
    
    ExAcquireResourceExclusiveLite(&fcb->nonpaged->dir_children_lock, TRUE);
    
    InsertTailList(&fcb->dir_children_index, &dc->list_entry_index);
    
    insert_dir_child_into_hash_lists(fcb, dc);
    
    ExReleaseResourceLite(&fcb->nonpaged->dir_children_lock);
    
    *pdc = dc;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL file_create2(PIRP Irp, device_extension* Vcb, PUNICODE_STRING fpus, file_ref* parfileref, ULONG options,
                                     FILE_FULL_EA_INFORMATION* ea, ULONG ealen, file_ref** pfr, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb* fcb;
    ULONG utf8len;
    char* utf8 = NULL;
    UINT64 dirpos, inode;
    UINT8 type;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POOL_TYPE pool_type = IrpSp->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
    ULONG defda;
    file_ref* fileref;
    dir_child* dc;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG rc;
#endif
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, fpus->Buffer, fpus->Length);
    if (!NT_SUCCESS(Status))
        return Status;
    
    utf8 = ExAllocatePoolWithTag(pool_type, utf8len + 1, ALLOC_TAG);
    if (!utf8) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = RtlUnicodeToUTF8N(utf8, utf8len, &utf8len, fpus->Buffer, fpus->Length);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(utf8);
        return Status;
    }
    
    utf8[utf8len] = 0;
    
    Status = fcb_get_last_dir_index(parfileref->fcb, &dirpos, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_last_dir_index returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    TRACE("create file %.*S\n", fpus->Length / sizeof(WCHAR), fpus->Buffer);
    ExAcquireResourceExclusiveLite(parfileref->fcb->Header.Resource, TRUE);
    TRACE("parfileref->fcb->inode_item.st_size (inode %llx) was %llx\n", parfileref->fcb->inode, parfileref->fcb->inode_item.st_size);
    parfileref->fcb->inode_item.st_size += utf8len * 2;
    TRACE("parfileref->fcb->inode_item.st_size (inode %llx) now %llx\n", parfileref->fcb->inode, parfileref->fcb->inode_item.st_size);
    parfileref->fcb->inode_item.transid = Vcb->superblock.generation;
    parfileref->fcb->inode_item.sequence++;
    parfileref->fcb->inode_item.st_ctime = now;
    parfileref->fcb->inode_item.st_mtime = now;
    ExReleaseResourceLite(parfileref->fcb->Header.Resource);
    
    parfileref->fcb->inode_item_changed = TRUE;
    mark_fcb_dirty(parfileref->fcb);
    
    inode = InterlockedIncrement64(&parfileref->fcb->subvol->lastinode);
    
    type = options & FILE_DIRECTORY_FILE ? BTRFS_TYPE_DIRECTORY : BTRFS_TYPE_FILE;
    
    // FIXME - link FILE_ATTRIBUTE_READONLY to st_mode
    
    TRACE("requested attributes = %x\n", IrpSp->Parameters.Create.FileAttributes);
    
    IrpSp->Parameters.Create.FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    
    defda = 0;
    
    if (utf8[0] == '.')
        defda |= FILE_ATTRIBUTE_HIDDEN;
    
    if (options & FILE_DIRECTORY_FILE) {
        defda |= FILE_ATTRIBUTE_DIRECTORY;
        IrpSp->Parameters.Create.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    
    TRACE("defda = %x\n", defda);
    
    if (IrpSp->Parameters.Create.FileAttributes == FILE_ATTRIBUTE_NORMAL)
        IrpSp->Parameters.Create.FileAttributes = defda;
    
    fcb = create_fcb(pool_type);
    if (!fcb) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fcb->Vcb = Vcb;

    if (IrpSp->Flags & SL_OPEN_PAGING_FILE) {
        fcb->Header.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
        Vcb->disallow_dismount = TRUE;
    }

    fcb->inode_item.generation = Vcb->superblock.generation;
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size = 0;
    fcb->inode_item.st_blocks = 0;
    fcb->inode_item.block_group = 0;
    fcb->inode_item.st_nlink = 1;
//     fcb->inode_item.st_uid = UID_NOBODY; // FIXME?
    fcb->inode_item.st_gid = GID_NOBODY; // FIXME?
    fcb->inode_item.st_mode = parfileref->fcb ? (parfileref->fcb->inode_item.st_mode & ~S_IFDIR) : 0755; // use parent's permissions by default
    fcb->inode_item.st_rdev = 0;
    fcb->inode_item.flags = 0;
    fcb->inode_item.sequence = 1;
    fcb->inode_item.st_atime = now;
    fcb->inode_item.st_ctime = now;
    fcb->inode_item.st_mtime = now;
    fcb->inode_item.otime = now;
    
    if (type == BTRFS_TYPE_DIRECTORY)
        fcb->inode_item.st_mode |= S_IFDIR;
    else {
        fcb->inode_item.st_mode |= S_IFREG;
        fcb->inode_item.st_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH); // remove executable bit if not directory
    }
    
    if (IrpSp->Flags & SL_OPEN_PAGING_FILE) {
        fcb->inode_item.flags = BTRFS_INODE_NODATACOW | BTRFS_INODE_NODATASUM | BTRFS_INODE_NOCOMPRESS;
    } else {
        // inherit nodatacow flag from parent directory
        if (parfileref->fcb->inode_item.flags & BTRFS_INODE_NODATACOW) {
            fcb->inode_item.flags |= BTRFS_INODE_NODATACOW;
            
            if (type != BTRFS_TYPE_DIRECTORY)
                fcb->inode_item.flags |= BTRFS_INODE_NODATASUM;
        }
        
        if (parfileref->fcb->inode_item.flags & BTRFS_INODE_COMPRESS)
            fcb->inode_item.flags |= BTRFS_INODE_COMPRESS;
    }
    
    fcb->inode_item_changed = TRUE;
    
    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;
    
    fcb->atts = IrpSp->Parameters.Create.FileAttributes;
    fcb->atts_changed = fcb->atts != defda;
    
#ifdef DEBUG_FCB_REFCOUNTS
    rc = InterlockedIncrement(&parfileref->fcb->refcount);
    WARN("fcb %p: refcount now %i (%S)\n", parfileref->fcb, rc, file_desc_fileref(parfileref));
#else
    InterlockedIncrement(&parfileref->fcb->refcount);
#endif
    fcb->subvol = parfileref->fcb->subvol;
    fcb->inode = inode;
    fcb->type = type;
    
    Status = fcb_get_new_sd(fcb, parfileref, IrpSp->Parameters.Create.SecurityContext->AccessState);
    
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_new_sd returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    fcb->sd_dirty = TRUE;
    
    if (ea && ealen > 0) {
        FILE_FULL_EA_INFORMATION* eainfo;
        
        fcb->ealen = 4;
        
        // capitalize EA names
        eainfo = ea;
        do {
            STRING s;
            
            s.Length = s.MaximumLength = eainfo->EaNameLength;
            s.Buffer = eainfo->EaName;
            
            RtlUpperString(&s, &s);
            
            fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;
            
            if (eainfo->NextEntryOffset == 0)
                break;
            
            eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
        } while (TRUE);
        
        fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(pool_type, ealen, ALLOC_TAG);
        if (!fcb->ea_xattr.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        fcb->ea_xattr.Length = fcb->ea_xattr.MaximumLength = ealen;
        RtlCopyMemory(fcb->ea_xattr.Buffer, ea, ealen);
        
        fcb->ea_changed = TRUE;
    }
    
    fileref = create_fileref();
    if (!fileref) {
        ERR("out of memory\n");
        free_fcb(fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fileref->fcb = fcb;
    fileref->index = dirpos;

    fileref->utf8.MaximumLength = fileref->utf8.Length = utf8len;
    fileref->utf8.Buffer = utf8;
    
    fileref->filepart.Length = fileref->filepart.MaximumLength = fpus->Length;
    
    if (fileref->filepart.Length == 0)
        fileref->filepart.Buffer = NULL;
    else {
        fileref->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fileref->filepart.Length, ALLOC_TAG);
        
        if (!fileref->filepart.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fileref->filepart.Buffer, fpus->Buffer, fpus->Length);
    }
    
    Status = RtlUpcaseUnicodeString(&fileref->filepart_uc, &fileref->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        free_fileref(fileref);
        return Status;
    }

    if (Irp->Overlay.AllocationSize.QuadPart > 0 && !write_fcb_compressed(fcb)) {
        Status = extend_file(fcb, fileref, Irp->Overlay.AllocationSize.QuadPart, TRUE, NULL, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("extend_file returned %08x\n", Status);
            free_fileref(fileref);
            return Status;
        }
    }
    
    fcb->created = TRUE;
    mark_fcb_dirty(fcb);
    
    fileref->created = TRUE;
    mark_fileref_dirty(fileref);
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;
    
    fileref->parent = parfileref;

    insert_fileref_child(parfileref, fileref, TRUE);
    
    Status = add_dir_child(fileref->parent->fcb, fcb->inode, FALSE, fileref->index, &fileref->utf8, &fileref->filepart, &fileref->filepart_uc, fcb->type, &dc);
    if (!NT_SUCCESS(Status))
        WARN("add_dir_child returned %08x\n", Status);
    
    fileref->dc = dc;
    dc->fileref = fileref;
    
    increase_fileref_refcount(parfileref);
    
    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs) {
            ERR("out of memory\n");
            free_fileref(fileref);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlZeroMemory(fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);
        
        fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
        if (!fcb->hash_ptrs_uc) {
            ERR("out of memory\n");
            free_fileref(fileref);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlZeroMemory(fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);
    }
 
    InsertTailList(&fcb->subvol->fcbs, &fcb->list_entry);
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);
    
    *pfr = fileref;
    
    if (type == BTRFS_TYPE_DIRECTORY)
        fileref->fcb->fileref = fileref;
    
    TRACE("created new file %S in subvol %llx, inode %llx\n", file_desc_fileref(fileref), fcb->subvol->id, fcb->inode);
    
    return STATUS_SUCCESS;
}

static NTSTATUS create_stream(device_extension* Vcb, file_ref** pfileref, file_ref** pparfileref, PUNICODE_STRING fpus, PUNICODE_STRING stream,
                              PIRP Irp, ULONG options, POOL_TYPE pool_type, BOOL case_sensitive, LIST_ENTRY* rollback) {
    file_ref *fileref, *newpar, *parfileref;
    fcb* fcb;
    static char xapref[] = "user.";
    static WCHAR DOSATTRIB[] = L"DOSATTRIB";
    static WCHAR EA[] = L"EA";
    ULONG xapreflen = strlen(xapref), overhead;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    ULONG utf8len;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG rc;
#endif
    
    TRACE("fpus = %.*S\n", fpus->Length / sizeof(WCHAR), fpus->Buffer);
    TRACE("stream = %.*S\n", stream->Length / sizeof(WCHAR), stream->Buffer);
    
    parfileref = *pparfileref;
    
    Status = open_fileref(Vcb, &newpar, fpus, parfileref, FALSE, NULL, NULL, PagedPool, case_sensitive, Irp);
    
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        UNICODE_STRING fpus2;
        
        if (!is_file_name_valid(fpus))
            return STATUS_OBJECT_NAME_INVALID;
        
        fpus2.Length = fpus2.MaximumLength = fpus->Length;
        fpus2.Buffer = ExAllocatePoolWithTag(pool_type, fpus2.MaximumLength, ALLOC_TAG);
        
        if (!fpus2.Buffer) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(fpus2.Buffer, fpus->Buffer, fpus2.Length);
        
        Status = file_create2(Irp, Vcb, &fpus2, parfileref, options, NULL, 0, &newpar, rollback);
    
        if (!NT_SUCCESS(Status)) {
            ERR("file_create2 returned %08x\n", Status);
            ExFreePool(fpus2.Buffer);
            return Status;
        }
        
        send_notification_fileref(newpar, options & FILE_DIRECTORY_FILE ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
        send_notification_fcb(newpar->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    } else if (!NT_SUCCESS(Status)) {
        ERR("open_fileref returned %08x\n", Status);
        return Status;
    }
    
    free_fileref(parfileref);
    
    parfileref = newpar;
    *pparfileref = parfileref;
    
    if (parfileref->fcb->type != BTRFS_TYPE_FILE && parfileref->fcb->type != BTRFS_TYPE_SYMLINK && parfileref->fcb->type != BTRFS_TYPE_DIRECTORY) {
        WARN("parent not file, directory, or symlink\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    if (options & FILE_DIRECTORY_FILE) {
        WARN("tried to create directory as stream\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    if ((stream->Length == wcslen(DOSATTRIB) * sizeof(WCHAR) && RtlCompareMemory(stream->Buffer, DOSATTRIB, stream->Length) == stream->Length) || 
        (stream->Length == wcslen(EA) * sizeof(WCHAR) && RtlCompareMemory(stream->Buffer, EA, stream->Length) == stream->Length)) {
        return STATUS_OBJECT_NAME_INVALID;
    }
        
    fcb = create_fcb(pool_type);
    if (!fcb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fcb->Vcb = Vcb;
    
    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;
    
#ifdef DEBUG_FCB_REFCOUNTS
    rc = InterlockedIncrement(&parfileref->fcb->refcount);
    WARN("fcb %p: refcount now %i (%S)\n", parfileref->fcb, rc, file_desc_fileref(parfileref));
#else
    InterlockedIncrement(&parfileref->fcb->refcount);
#endif
    fcb->subvol = parfileref->fcb->subvol;
    fcb->inode = parfileref->fcb->inode;
    fcb->type = parfileref->fcb->type;
    
    fcb->ads = TRUE;
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 1 returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    fcb->adsxattr.Length = utf8len + xapreflen;
    fcb->adsxattr.MaximumLength = fcb->adsxattr.Length + 1;
    fcb->adsxattr.Buffer = ExAllocatePoolWithTag(pool_type, fcb->adsxattr.MaximumLength, ALLOC_TAG);
    if (!fcb->adsxattr.Buffer) {
        ERR("out of memory\n");
        free_fcb(fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(fcb->adsxattr.Buffer, xapref, xapreflen);
    
    Status = RtlUnicodeToUTF8N(&fcb->adsxattr.Buffer[xapreflen], utf8len, &utf8len, stream->Buffer, stream->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 2 returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    fcb->adsxattr.Buffer[fcb->adsxattr.Length] = 0;
    
    TRACE("adsxattr = %s\n", fcb->adsxattr.Buffer);
    
    fcb->adshash = calc_crc32c(0xfffffffe, (UINT8*)fcb->adsxattr.Buffer, fcb->adsxattr.Length);
    TRACE("adshash = %08x\n", fcb->adshash);
    
    searchkey.obj_id = parfileref->fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = fcb->adshash;
    
    Status = find_item(Vcb, parfileref->fcb->subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        free_fcb(fcb);
        return Status;
    }
    
    if (!keycmp(tp.item->key, searchkey))
        overhead = tp.item->size;
    else
        overhead = 0;
    
    fcb->adsmaxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - (sizeof(DIR_ITEM) - 1);
    
    if (utf8len + xapreflen + overhead > fcb->adsmaxlen) {
        WARN("not enough room for new DIR_ITEM (%u + %u > %u)", utf8len + xapreflen, overhead, fcb->adsmaxlen);
        free_fcb(fcb);
        return STATUS_DISK_FULL;
    } else
        fcb->adsmaxlen -= overhead + utf8len + xapreflen;
    
    fileref = create_fileref();
    if (!fileref) {
        ERR("out of memory\n");
        free_fcb(fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fileref->fcb = fcb;

    fileref->filepart.MaximumLength = fileref->filepart.Length = stream->Length;
    fileref->filepart.Buffer = ExAllocatePoolWithTag(pool_type, fileref->filepart.MaximumLength, ALLOC_TAG);
    if (!fileref->filepart.Buffer) {
        ERR("out of memory\n");
        free_fileref(fileref);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(fileref->filepart.Buffer, stream->Buffer, stream->Length);
    
    Status = RtlUpcaseUnicodeString(&fileref->filepart_uc, &fileref->filepart, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUpcaseUnicodeString returned %08x\n", Status);
        free_fileref(fileref);
        return Status;
    }
    
    mark_fcb_dirty(fcb);
    mark_fileref_dirty(fileref);
    
    InsertHeadList(&parfileref->fcb->list_entry, &fcb->list_entry); // insert in list after parent fcb
    InsertTailList(&Vcb->all_fcbs, &fcb->list_entry_all);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    parfileref->fcb->inode_item.transid = Vcb->superblock.generation;
    parfileref->fcb->inode_item.sequence++;
    parfileref->fcb->inode_item.st_ctime = now;
    parfileref->fcb->inode_item_changed = TRUE;
    
    mark_fcb_dirty(parfileref->fcb);
    
    parfileref->fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    parfileref->fcb->subvol->root_item.ctime = now;
    
    fileref->parent = (struct _file_ref*)parfileref;
    
    insert_fileref_child(parfileref, fileref, TRUE);
    
    increase_fileref_refcount(parfileref);
    
    *pfileref = fileref;
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL file_create(PIRP Irp, device_extension* Vcb, PFILE_OBJECT FileObject, file_ref* related, BOOL loaded_related,
                                    PUNICODE_STRING fnus, ULONG disposition, ULONG options, LIST_ENTRY* rollback) {
    NTSTATUS Status;
//     fcb *fcb, *parfcb = NULL;
    file_ref *fileref, *parfileref = NULL;
    ULONG i, j, fn_offset;
//     ULONG utf8len;
    ccb* ccb;
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    UNICODE_STRING dsus, fpus, stream;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POOL_TYPE pool_type = IrpSp->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
    PACCESS_STATE access_state = IrpSp->Parameters.Create.SecurityContext->AccessState;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG oc;
#endif

    TRACE("(%p, %p, %p, %.*S, %x, %x)\n", Irp, Vcb, FileObject, fnus->Length / sizeof(WCHAR), fnus->Buffer, disposition, options);
    
    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;
    
    dsus.Buffer = datasuf;
    dsus.Length = dsus.MaximumLength = wcslen(datasuf) * sizeof(WCHAR);
    fpus.Buffer = NULL;
    
    if (!loaded_related) {
        Status = open_fileref(Vcb, &parfileref, fnus, related, TRUE, NULL, NULL, pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, Irp);
        
        if (!NT_SUCCESS(Status))
            goto end;
    } else
        parfileref = related;
    
    if (parfileref->fcb->type != BTRFS_TYPE_DIRECTORY && (fnus->Length < sizeof(WCHAR) || fnus->Buffer[0] != ':')) {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto end;
    }
    
    if (parfileref->fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    i = (fnus->Length / sizeof(WCHAR))-1;
    while ((fnus->Buffer[i] == '\\' || fnus->Buffer[i] == '/') && i > 0) { i--; }
    
    j = i;
    
    while (i > 0 && fnus->Buffer[i-1] != '\\' && fnus->Buffer[i-1] != '/') { i--; }
    
    fpus.MaximumLength = (j - i + 2) * sizeof(WCHAR);
    fpus.Buffer = ExAllocatePoolWithTag(pool_type, fpus.MaximumLength, ALLOC_TAG);
    if (!fpus.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fpus.Length = (j - i + 1) * sizeof(WCHAR);
    
    RtlCopyMemory(fpus.Buffer, &fnus->Buffer[i], (j - i + 1) * sizeof(WCHAR));
    fpus.Buffer[j - i + 1] = 0;
    
    fn_offset = i;
    
    if (fpus.Length > dsus.Length) { // check for :$DATA suffix
        UNICODE_STRING lb;
                
        lb.Buffer = &fpus.Buffer[(fpus.Length - dsus.Length)/sizeof(WCHAR)];
        lb.Length = lb.MaximumLength = dsus.Length;
        
        TRACE("lb = %.*S\n", lb.Length/sizeof(WCHAR), lb.Buffer);
        
        if (FsRtlAreNamesEqual(&dsus, &lb, TRUE, NULL)) {
            TRACE("ignoring :$DATA suffix\n");
            
            fpus.Length -= lb.Length;
            
            if (fpus.Length > sizeof(WCHAR) && fpus.Buffer[(fpus.Length-1)/sizeof(WCHAR)] == ':')
                fpus.Length -= sizeof(WCHAR);
            
            TRACE("fpus = %.*S\n", fpus.Length / sizeof(WCHAR), fpus.Buffer);
        }
    }
    
    stream.Length = 0;
    
    for (i = 0; i < fpus.Length/sizeof(WCHAR); i++) {
        if (fpus.Buffer[i] == ':') {
            stream.Length = fpus.Length - (i*sizeof(WCHAR)) - sizeof(WCHAR);
            stream.Buffer = &fpus.Buffer[i+1];
            fpus.Buffer[i] = 0;
            fpus.Length = i * sizeof(WCHAR);
            break;
        }
    }
    
    if (stream.Length > 0) {
        Status = create_stream(Vcb, &fileref, &parfileref, &fpus, &stream, Irp, options, pool_type, IrpSp->Flags & SL_CASE_SENSITIVE, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("create_stream returned %08x\n", Status);
            goto end;
        }
    } else {
        if (!is_file_name_valid(&fpus)) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto end;
        }
        
        if (Irp->AssociatedIrp.SystemBuffer && IrpSp->Parameters.Create.EaLength > 0) {
            ULONG offset;
            
            Status = IoCheckEaBufferValidity(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Create.EaLength, &offset);
            if (!NT_SUCCESS(Status)) {
                ERR("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
                goto end;
            }
        }
        
        Status = file_create2(Irp, Vcb, &fpus, parfileref, options, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Create.EaLength,
                              &fileref, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("file_create2 returned %08x\n", Status);
            goto end;
        }
        
        send_notification_fileref(fileref, options & FILE_DIRECTORY_FILE ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_ADDED);
        send_notification_fcb(fileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED);
    }
    
    FileObject->FsContext = fileref->fcb;
    
    ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
    if (!ccb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        free_fileref(fileref);
        goto end;
    }
    
    RtlZeroMemory(ccb, sizeof(*ccb));
    
    ccb->fileref = fileref;
    
    ccb->NodeType = BTRFS_NODE_TYPE_CCB;
    ccb->NodeSize = sizeof(ccb);
    ccb->disposition = disposition;
    ccb->options = options;
    ccb->query_dir_offset = 0;
    RtlInitUnicodeString(&ccb->query_string, NULL);
    ccb->has_wildcard = FALSE;
    ccb->specific_file = FALSE;
    ccb->access = access_state->OriginalDesiredAccess;
    ccb->case_sensitive = IrpSp->Flags & SL_CASE_SENSITIVE;
    
#ifdef DEBUG_FCB_REFCOUNTS
    oc = InterlockedIncrement(&fileref->open_count);
    ERR("fileref %p: open_count now %i\n", fileref, oc);
#else
    InterlockedIncrement(&fileref->open_count);
#endif
    InterlockedIncrement(&Vcb->open_files);
    
    FileObject->FsContext2 = ccb;
    
    if (fn_offset > 0) {
        FileObject->FileName.Length -= fn_offset * sizeof(WCHAR);
        RtlMoveMemory(&FileObject->FileName.Buffer[0], &FileObject->FileName.Buffer[fn_offset], FileObject->FileName.Length);
    }

    FileObject->SectionObjectPointer = &fileref->fcb->nonpaged->segment_object;
    
//     TRACE("returning FCB %p with parent %p\n", fcb, parfcb);
    
//     ULONG fnlen;
// 
//     fcb->name_offset = fcb->par->full_filename.Length / sizeof(WCHAR);
//             
//     if (fcb->par != Vcb->root_fcb)
//         fcb->name_offset++;
//     
//     fnlen = (fcb->name_offset * sizeof(WCHAR)) + fcb->filepart.Length;
//     
//     fcb->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fnlen, ALLOC_TAG);
//     if (!fcb->full_filename.Buffer) {
//         ERR("out of memory\n");
//         Status = STATUS_INSUFFICIENT_RESOURCES;
//         goto end;
//     }   
//     
//     fcb->full_filename.Length = fcb->full_filename.MaximumLength = fnlen;
//     RtlCopyMemory(fcb->full_filename.Buffer, fcb->par->full_filename.Buffer, fcb->par->full_filename.Length);
//     
//     if (fcb->par != Vcb->root_fcb)
//         fcb->full_filename.Buffer[fcb->par->full_filename.Length / sizeof(WCHAR)] = '\\';
//     
//     RtlCopyMemory(&fcb->full_filename.Buffer[fcb->name_offset], fcb->filepart.Buffer, fcb->filepart.Length);
        
    goto end2;
    
end:    
    if (fpus.Buffer)
        ExFreePool(fpus.Buffer);
    
end2:
    if (parfileref && !loaded_related)
        free_fileref(parfileref);
    
    return Status;
}

static __inline void debug_create_options(ULONG RequestedOptions) {
    if (RequestedOptions != 0) {
        ULONG options = RequestedOptions;
        
        TRACE("requested options:\n");
        
        if (options & FILE_DIRECTORY_FILE) {
            TRACE("    FILE_DIRECTORY_FILE\n");
            options &= ~FILE_DIRECTORY_FILE;
        }

        if (options & FILE_WRITE_THROUGH) {
            TRACE("    FILE_WRITE_THROUGH\n");
            options &= ~FILE_WRITE_THROUGH;
        }

        if (options & FILE_SEQUENTIAL_ONLY) {
            TRACE("    FILE_SEQUENTIAL_ONLY\n");
            options &= ~FILE_SEQUENTIAL_ONLY;
        }

        if (options & FILE_NO_INTERMEDIATE_BUFFERING) {
            TRACE("    FILE_NO_INTERMEDIATE_BUFFERING\n");
            options &= ~FILE_NO_INTERMEDIATE_BUFFERING;
        }

        if (options & FILE_SYNCHRONOUS_IO_ALERT) {
            TRACE("    FILE_SYNCHRONOUS_IO_ALERT\n");
            options &= ~FILE_SYNCHRONOUS_IO_ALERT;
        }

        if (options & FILE_SYNCHRONOUS_IO_NONALERT) {
            TRACE("    FILE_SYNCHRONOUS_IO_NONALERT\n");
            options &= ~FILE_SYNCHRONOUS_IO_NONALERT;
        }

        if (options & FILE_NON_DIRECTORY_FILE) {
            TRACE("    FILE_NON_DIRECTORY_FILE\n");
            options &= ~FILE_NON_DIRECTORY_FILE;
        }

        if (options & FILE_CREATE_TREE_CONNECTION) {
            TRACE("    FILE_CREATE_TREE_CONNECTION\n");
            options &= ~FILE_CREATE_TREE_CONNECTION;
        }

        if (options & FILE_COMPLETE_IF_OPLOCKED) {
            TRACE("    FILE_COMPLETE_IF_OPLOCKED\n");
            options &= ~FILE_COMPLETE_IF_OPLOCKED;
        }

        if (options & FILE_NO_EA_KNOWLEDGE) {
            TRACE("    FILE_NO_EA_KNOWLEDGE\n");
            options &= ~FILE_NO_EA_KNOWLEDGE;
        }
        
        if (options & FILE_OPEN_REMOTE_INSTANCE) {
            TRACE("    FILE_OPEN_REMOTE_INSTANCE\n");
            options &= ~FILE_OPEN_REMOTE_INSTANCE;
        }

        if (options & FILE_RANDOM_ACCESS) {
            TRACE("    FILE_RANDOM_ACCESS\n");
            options &= ~FILE_RANDOM_ACCESS;
        }

        if (options & FILE_DELETE_ON_CLOSE) {
            TRACE("    FILE_DELETE_ON_CLOSE\n");
            options &= ~FILE_DELETE_ON_CLOSE;
        }

        if (options & FILE_OPEN_BY_FILE_ID) {
            TRACE("    FILE_OPEN_BY_FILE_ID\n");
            options &= ~FILE_OPEN_BY_FILE_ID;
        }

        if (options & FILE_OPEN_FOR_BACKUP_INTENT) {
            TRACE("    FILE_OPEN_FOR_BACKUP_INTENT\n");
            options &= ~FILE_OPEN_FOR_BACKUP_INTENT;
        }
        
        if (options & FILE_NO_COMPRESSION) {
            TRACE("    FILE_NO_COMPRESSION\n");
            options &= ~FILE_NO_COMPRESSION;
        }

#if NTDDI_VERSION >= NTDDI_WIN7
        if (options & FILE_OPEN_REQUIRING_OPLOCK) {
            TRACE("    FILE_OPEN_REQUIRING_OPLOCK\n");
            options &= ~FILE_OPEN_REQUIRING_OPLOCK;
        }
        
        if (options & FILE_DISALLOW_EXCLUSIVE) {
            TRACE("    FILE_DISALLOW_EXCLUSIVE\n");
            options &= ~FILE_DISALLOW_EXCLUSIVE;
        }
#endif

        if (options & FILE_RESERVE_OPFILTER) {
            TRACE("    FILE_RESERVE_OPFILTER\n");
            options &= ~FILE_RESERVE_OPFILTER;
        }

        if (options & FILE_OPEN_REPARSE_POINT) {
            TRACE("    FILE_OPEN_REPARSE_POINT\n");
            options &= ~FILE_OPEN_REPARSE_POINT;
        }
        
        if (options & FILE_OPEN_NO_RECALL) {
            TRACE("    FILE_OPEN_NO_RECALL\n");
            options &= ~FILE_OPEN_NO_RECALL;
        }
        
        if (options & FILE_OPEN_FOR_FREE_SPACE_QUERY) {
            TRACE("    FILE_OPEN_FOR_FREE_SPACE_QUERY\n");
            options &= ~FILE_OPEN_FOR_FREE_SPACE_QUERY;
        }
        
        if (options)
            TRACE("    unknown options: %x\n", options);
    } else {
        TRACE("requested options: (none)\n");
    }
}

static NTSTATUS get_reparse_block(fcb* fcb, UINT8** data) {
    NTSTATUS Status;
    
    if (fcb->type == BTRFS_TYPE_FILE || fcb->type == BTRFS_TYPE_SYMLINK) {
        ULONG size, bytes_read, i;
        
        if (fcb->type == BTRFS_TYPE_FILE && fcb->inode_item.st_size < sizeof(ULONG)) {
            WARN("file was too short to be a reparse point\n");
            return STATUS_INVALID_PARAMETER;
        }
        
        // 0x10007 = 0xffff (maximum length of data buffer) + 8 bytes header
        size = min(0x10007, fcb->inode_item.st_size);
        
        *data = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
        if (!*data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        Status = read_file(fcb, *data, 0, size, &bytes_read, NULL, TRUE);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file_fcb returned %08x\n", Status);
            ExFreePool(*data);
            return Status;
        }
        
        if (fcb->type == BTRFS_TYPE_SYMLINK) {
            ULONG stringlen, subnamelen, printnamelen, reqlen;
            REPARSE_DATA_BUFFER* rdb;
            
            Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, (char*)*data, bytes_read);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                ExFreePool(*data);
                return Status;
            }
            
            subnamelen = stringlen;
            printnamelen = stringlen;
            
            reqlen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + subnamelen + printnamelen;
            
            rdb = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);
            
            if (!rdb) {
                ERR("out of memory\n");
                ExFreePool(*data);
                return STATUS_INSUFFICIENT_RESOURCES;
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
                                    stringlen, &stringlen, (char*)*data, size);

            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                ExFreePool(rdb);
                ExFreePool(*data);
                return Status;
            }
            
            for (i = 0; i < stringlen / sizeof(WCHAR); i++) {
                if (rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] == '/')
                    rdb->SymbolicLinkReparseBuffer.PathBuffer[(rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)) + i] = '\\';
            }
            
            RtlCopyMemory(&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)],
                        &rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength);
            
            ExFreePool(*data);
            
            *data = (UINT8*)rdb;
        } else {
            Status = FsRtlValidateReparsePointBuffer(bytes_read, (REPARSE_DATA_BUFFER*)*data);
            if (!NT_SUCCESS(Status)) {
                ERR("FsRtlValidateReparsePointBuffer returned %08x\n", Status);
                ExFreePool(*data);
                return Status;
            }
        }
    } else if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        if (!fcb->reparse_xattr.Buffer || fcb->reparse_xattr.Length == 0)
            return STATUS_INTERNAL_ERROR;
            
        if (fcb->reparse_xattr.Length < sizeof(ULONG)) {
            WARN("xattr was too short to be a reparse point\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        Status = FsRtlValidateReparsePointBuffer(fcb->reparse_xattr.Length, (REPARSE_DATA_BUFFER*)fcb->reparse_xattr.Buffer);
        if (!NT_SUCCESS(Status)) {
            ERR("FsRtlValidateReparsePointBuffer returned %08x\n", Status);
            return Status;
        }
        
        *data = ExAllocatePoolWithTag(PagedPool, fcb->reparse_xattr.Length, ALLOC_TAG);
        if (!*data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(*data, fcb->reparse_xattr.Buffer, fcb->reparse_xattr.Length);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL open_file(PDEVICE_OBJECT DeviceObject, PIRP Irp, LIST_ENTRY* rollback) {
    PFILE_OBJECT FileObject;
    ULONG RequestedDisposition;
    ULONG options;
    NTSTATUS Status;
    ccb* ccb;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    USHORT parsed;
    ULONG fn_offset = 0;
    file_ref *related, *fileref;
    POOL_TYPE pool_type = Stack->Flags & SL_OPEN_PAGING_FILE ? NonPagedPool : PagedPool;
    ACCESS_MASK granted_access;
    BOOL loaded_related = FALSE;
    UNICODE_STRING fn;
#ifdef DEBUG_FCB_REFCOUNTS
    LONG oc;
#endif
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
    UINT8 open_type = 0;
    
    time1 = KeQueryPerformanceCounter(NULL);
#endif
    
    Irp->IoStatus.Information = 0;
    
    RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
    options = Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
    
    if (options & FILE_DIRECTORY_FILE && RequestedDisposition == FILE_SUPERSEDE) {
        WARN("error - supersede requested with FILE_DIRECTORY_FILE\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }

    FileObject = Stack->FileObject;
    
    if (FileObject->RelatedFileObject && FileObject->RelatedFileObject->FsContext2) {
        struct _ccb* relatedccb = FileObject->RelatedFileObject->FsContext2;
        
        related = relatedccb->fileref;
    } else
        related = NULL;
    
    debug_create_options(options);
    
    switch (RequestedDisposition) {
        case FILE_SUPERSEDE:
            TRACE("requested disposition: FILE_SUPERSEDE\n");
            break;
            
        case FILE_CREATE:
            TRACE("requested disposition: FILE_CREATE\n");
            break;
            
        case FILE_OPEN:
            TRACE("requested disposition: FILE_OPEN\n");
            break;
            
        case FILE_OPEN_IF:
            TRACE("requested disposition: FILE_OPEN_IF\n");
            break;
            
        case FILE_OVERWRITE:
            TRACE("requested disposition: FILE_OVERWRITE\n");
            break;
            
        case FILE_OVERWRITE_IF:
            TRACE("requested disposition: FILE_OVERWRITE_IF\n");
            break;
            
        default:
            ERR("unknown disposition: %x\n", RequestedDisposition);
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
    }
    
    fn = FileObject->FileName;
    
    TRACE("(%.*S)\n", fn.Length / sizeof(WCHAR), fn.Buffer);
    TRACE("FileObject = %p\n", FileObject);
    
    if (Vcb->readonly && (RequestedDisposition == FILE_SUPERSEDE || RequestedDisposition == FILE_CREATE || RequestedDisposition == FILE_OVERWRITE)) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto exit2;
    }
    
    if (Vcb->readonly && Stack->Parameters.Create.SecurityContext->DesiredAccess &
        (FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES | DELETE | WRITE_OWNER | WRITE_DAC)) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto exit2;
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);

    if (options & FILE_OPEN_BY_FILE_ID) {
        if (fn.Length == sizeof(UINT64) && related && RequestedDisposition == FILE_OPEN) {
            UINT64 inode;
            
            RtlCopyMemory(&inode, fn.Buffer, sizeof(UINT64));
            
            if (related->fcb == Vcb->root_fileref->fcb && inode == 0)
                inode = Vcb->root_fileref->fcb->inode;
            
            if (inode == 0) { // we use 0 to mean the parent of a subvolume
                fileref = related->parent;
                increase_fileref_refcount(fileref);
                Status = STATUS_SUCCESS;
            } else {
                Status = open_fileref_by_inode(Vcb, related->fcb->subvol, inode, &fileref, Irp);
            }
        } else {
            WARN("FILE_OPEN_BY_FILE_ID only supported for inodes\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
        }
    } else {
        if (related && fn.Length != 0 && fn.Buffer[0] == '\\') {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto exit;
        }
        
        if (!related && RequestedDisposition != FILE_OPEN && !(Stack->Flags & SL_OPEN_TARGET_DIRECTORY)) {
            ULONG fnoff;
            
            Status = open_fileref(Vcb, &related, &fn, NULL, TRUE, &parsed, &fnoff,
                                  pool_type, Stack->Flags & SL_CASE_SENSITIVE, Irp);
            
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_OBJECT_PATH_NOT_FOUND;
            else if (Status == STATUS_REPARSE)
                fileref = related;
            else if (NT_SUCCESS(Status)) {
                fnoff *= sizeof(WCHAR);
                fnoff += related->filepart.Length + sizeof(WCHAR);
                
                if (related->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) {
                    Status = STATUS_REPARSE;
                    fileref = related;
                    parsed = fnoff - sizeof(WCHAR);
                } else {
                    fn.Buffer = &fn.Buffer[fnoff / sizeof(WCHAR)];
                    fn.Length -= fnoff;

                    Status = open_fileref(Vcb, &fileref, &fn, related, Stack->Flags & SL_OPEN_TARGET_DIRECTORY, &parsed, &fn_offset,
                                        pool_type, Stack->Flags & SL_CASE_SENSITIVE, Irp);
                    
                    loaded_related = TRUE;
                }
                
            }
        } else {
            Status = open_fileref(Vcb, &fileref, &fn, related, Stack->Flags & SL_OPEN_TARGET_DIRECTORY, &parsed, &fn_offset,
                                  pool_type, Stack->Flags & SL_CASE_SENSITIVE, Irp);
        }
    }
    
    if (Status == STATUS_REPARSE) {
        REPARSE_DATA_BUFFER* data;
        
        ExAcquireResourceSharedLite(fileref->fcb->Header.Resource, TRUE);
        Status = get_reparse_block(fileref->fcb, (UINT8**)&data);
        ExReleaseResourceLite(fileref->fcb->Header.Resource);
        
        if (!NT_SUCCESS(Status)) {
            ERR("get_reparse_block returned %08x\n", Status);
            
            free_fileref(fileref);
            goto exit;
        }
        
        Status = STATUS_REPARSE;
        RtlCopyMemory(&Irp->IoStatus.Information, data, sizeof(ULONG));
        
        data->Reserved = FileObject->FileName.Length - parsed;
        
        Irp->Tail.Overlay.AuxiliaryBuffer = (void*)data;
        
        free_fileref(fileref);
        goto exit;
    }
    
    if (NT_SUCCESS(Status) && fileref->deleted) {
        free_fileref(fileref);
        
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    if (NT_SUCCESS(Status)) {
        if (RequestedDisposition == FILE_CREATE) {
            TRACE("file %S already exists, returning STATUS_OBJECT_NAME_COLLISION\n", file_desc_fileref(fileref));
            Status = STATUS_OBJECT_NAME_COLLISION;
            free_fileref(fileref);
            goto exit;
        }
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        if (RequestedDisposition == FILE_OPEN || RequestedDisposition == FILE_OVERWRITE) {
            TRACE("file doesn't exist, returning STATUS_OBJECT_NAME_NOT_FOUND\n");
            goto exit;
        }
    } else if (Status == STATUS_OBJECT_PATH_NOT_FOUND) {
        TRACE("open_fileref returned %08x\n", Status);
        goto exit;
    } else {
        ERR("open_fileref returned %08x\n", Status);
        goto exit;
    }
    
    if (NT_SUCCESS(Status)) { // file already exists
        file_ref* sf;
        
        if (RequestedDisposition == FILE_SUPERSEDE || RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF) {
            LARGE_INTEGER zero;
            
#ifdef DEBUG_STATS
            open_type = 1;
#endif
            if (fileref->fcb->type == BTRFS_TYPE_DIRECTORY || fileref->fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY) {
                Status = STATUS_ACCESS_DENIED;
                free_fileref(fileref);
                goto exit;
            }
            
            if (Vcb->readonly) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                free_fileref(fileref);
                goto exit;
            }
            
            zero.QuadPart = 0;
            if (!MmCanFileBeTruncated(&fileref->fcb->nonpaged->segment_object, &zero)) {
                Status = STATUS_USER_MAPPED_FILE;
                free_fileref(fileref);
                goto exit;
            }
        }
        
        SeLockSubjectContext(&Stack->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
        
        if (!SeAccessCheck(fileref->fcb->ads ? fileref->parent->fcb->sd : fileref->fcb->sd,
                           &Stack->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                           FALSE, Stack->Parameters.Create.SecurityContext->DesiredAccess, 0, NULL,
                           IoGetFileObjectGenericMapping(), Stack->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode,
                           &granted_access, &Status)) {
            SeUnlockSubjectContext(&Stack->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
            WARN("SeAccessCheck failed, returning %08x\n", Status);
            goto exit;
        }
        
        SeUnlockSubjectContext(&Stack->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
        
        // We allow a subvolume root to be opened read-write even if its readonly flag is set, so it can be cleared
        if (fileref->fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY && granted_access &
            (FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES | DELETE | WRITE_OWNER | WRITE_DAC) &&
            fileref->fcb->inode != SUBVOL_ROOT_INODE) {
            Status = STATUS_ACCESS_DENIED;
            free_fileref(fileref);
            goto exit;
        }
        
        TRACE("deleted = %s\n", fileref->deleted ? "TRUE" : "FALSE");
        
        sf = fileref;
        while (sf) {
            if (sf->delete_on_close) {
                WARN("could not open as deletion pending\n");
                Status = STATUS_DELETE_PENDING;
                
                free_fileref(fileref);
                goto exit;
            }
            sf = sf->parent;
        }

        if (fileref->fcb->atts & FILE_ATTRIBUTE_READONLY) {
            ACCESS_MASK allowed = DELETE | READ_CONTROL | WRITE_OWNER | WRITE_DAC |
                                    SYNCHRONIZE | ACCESS_SYSTEM_SECURITY | FILE_READ_DATA |
                                    FILE_READ_EA | FILE_WRITE_EA | FILE_READ_ATTRIBUTES |
                                    FILE_WRITE_ATTRIBUTES | FILE_EXECUTE | FILE_LIST_DIRECTORY |
                                    FILE_TRAVERSE;

            if (fileref->fcb->type == BTRFS_TYPE_DIRECTORY)
                allowed |= FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | FILE_DELETE_CHILD;
            
            if (granted_access & ~allowed) {
                Status = STATUS_ACCESS_DENIED;
                free_fileref(fileref);
                goto exit;
            }
        }
        
        if (options & FILE_DELETE_ON_CLOSE && (fileref == Vcb->root_fileref || Vcb->readonly ||
            fileref->fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY || fileref->fcb->atts & FILE_ATTRIBUTE_READONLY)) {
            Status = STATUS_CANNOT_DELETE;
            free_fileref(fileref);
            goto exit;
        }
        
        if ((fileref->fcb->type == BTRFS_TYPE_SYMLINK || fileref->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT) && !(options & FILE_OPEN_REPARSE_POINT))  {
            REPARSE_DATA_BUFFER* data;
            
            /* How reparse points work from the point of view of the filesystem appears to
             * undocumented. When returning STATUS_REPARSE, MSDN encourages us to return
             * IO_REPARSE in Irp->IoStatus.Information, but that means we have to do our own
             * translation. If we instead return the reparse tag in Information, and store
             * a pointer to the reparse data buffer in Irp->Tail.Overlay.AuxiliaryBuffer,
             * IopSymlinkProcessReparse will do the translation for us. */
            
            Status = get_reparse_block(fileref->fcb, (UINT8**)&data);
            if (!NT_SUCCESS(Status)) {
                ERR("get_reparse_block returned %08x\n", Status);
                free_fileref(fileref);
                goto exit;
            }
            
            Status = STATUS_REPARSE;
            Irp->IoStatus.Information = data->ReparseTag;
            
            if (fn.Buffer[(fn.Length / sizeof(WCHAR)) - 1] == '\\')
                data->Reserved = sizeof(WCHAR);
            
            Irp->Tail.Overlay.AuxiliaryBuffer = (void*)data;
            
            free_fileref(fileref);
            goto exit;
        }
        
        if (fileref->fcb->type == BTRFS_TYPE_DIRECTORY && !fileref->fcb->ads) {
            if (options & FILE_NON_DIRECTORY_FILE && !(fileref->fcb->atts & FILE_ATTRIBUTE_REPARSE_POINT)) {
                free_fileref(fileref);
                Status = STATUS_FILE_IS_A_DIRECTORY;
                goto exit;
            }
        } else if (options & FILE_DIRECTORY_FILE) {
            TRACE("returning STATUS_NOT_A_DIRECTORY (type = %u, %S)\n", fileref->fcb->type, file_desc_fileref(fileref));
            free_fileref(fileref);
            Status = STATUS_NOT_A_DIRECTORY;
            goto exit;
        }
        
        if (fileref->open_count > 0) {
            Status = IoCheckShareAccess(granted_access, Stack->Parameters.Create.ShareAccess, FileObject, &fileref->fcb->share_access, TRUE);
            
            if (!NT_SUCCESS(Status)) {
                WARN("IoCheckShareAccess failed, returning %08x\n", Status);
                
                free_fileref(fileref);
                goto exit;
            }
        } else {
            IoSetShareAccess(granted_access, Stack->Parameters.Create.ShareAccess, FileObject, &fileref->fcb->share_access);
        }

        if (granted_access & FILE_WRITE_DATA || options & FILE_DELETE_ON_CLOSE) {
            if (!MmFlushImageSection(&fileref->fcb->nonpaged->segment_object, MmFlushForWrite)) {
                Status = (options & FILE_DELETE_ON_CLOSE) ? STATUS_CANNOT_DELETE : STATUS_SHARING_VIOLATION;
                
                free_fileref(fileref);
                goto exit;
            }
        }
        
        if (RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF || RequestedDisposition == FILE_SUPERSEDE) {
            ULONG defda, oldatts, filter;
            LARGE_INTEGER time;
            BTRFS_TIME now;
            
            if ((RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF) && fileref->fcb->atts & FILE_ATTRIBUTE_READONLY) {
                WARN("cannot overwrite readonly file\n");
                Status = STATUS_ACCESS_DENIED;
                free_fileref(fileref);
                goto exit;
            }
    
            // FIXME - where did we get this from?
//             if (fcb->refcount > 1) {
//                 WARN("cannot overwrite open file (fcb = %p, refcount = %u)\n", fcb, fcb->refcount);
//                 Status = STATUS_ACCESS_DENIED;
//                 free_fcb(fcb);
//                 goto exit;
//             }
            
            // FIXME - make sure not ADS!
            Status = truncate_file(fileref->fcb, 0, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("truncate_file returned %08x\n", Status);
                free_fileref(fileref);
                goto exit;
            }
            
            if (Irp->Overlay.AllocationSize.QuadPart > 0) {
                Status = extend_file(fileref->fcb, fileref, Irp->Overlay.AllocationSize.QuadPart, TRUE, NULL, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("extend_file returned %08x\n", Status);
                    free_fileref(fileref);
                    goto exit;
                }
            }
            
            if (Irp->AssociatedIrp.SystemBuffer && Stack->Parameters.Create.EaLength > 0) {
                ULONG offset;
                FILE_FULL_EA_INFORMATION* eainfo;
                
                Status = IoCheckEaBufferValidity(Irp->AssociatedIrp.SystemBuffer, Stack->Parameters.Create.EaLength, &offset);
                if (!NT_SUCCESS(Status)) {
                    ERR("IoCheckEaBufferValidity returned %08x (error at offset %u)\n", Status, offset);
                    free_fileref(fileref);
                    goto exit;
                }
                
                fileref->fcb->ealen = 4;
                
                // capitalize EA name
                eainfo = Irp->AssociatedIrp.SystemBuffer;
                do {
                    STRING s;
                    
                    s.Length = s.MaximumLength = eainfo->EaNameLength;
                    s.Buffer = eainfo->EaName;
                    
                    RtlUpperString(&s, &s);
                    
                    fileref->fcb->ealen += 5 + eainfo->EaNameLength + eainfo->EaValueLength;
                    
                    if (eainfo->NextEntryOffset == 0)
                        break;
                    
                    eainfo = (FILE_FULL_EA_INFORMATION*)(((UINT8*)eainfo) + eainfo->NextEntryOffset);
                } while (TRUE);
                
                if (fileref->fcb->ea_xattr.Buffer)
                    ExFreePool(fileref->fcb->ea_xattr.Buffer);
                
                fileref->fcb->ea_xattr.Buffer = ExAllocatePoolWithTag(pool_type, Stack->Parameters.Create.EaLength, ALLOC_TAG);
                if (!fileref->fcb->ea_xattr.Buffer) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    
                    free_fileref(fileref);
                    goto exit;
                }
                
                fileref->fcb->ea_xattr.Length = fileref->fcb->ea_xattr.MaximumLength = Stack->Parameters.Create.EaLength;
                RtlCopyMemory(fileref->fcb->ea_xattr.Buffer, Irp->AssociatedIrp.SystemBuffer, Stack->Parameters.Create.EaLength);
            } else {
                if (fileref->fcb->ea_xattr.Length > 0) {
                    ExFreePool(fileref->fcb->ea_xattr.Buffer);
                    fileref->fcb->ea_xattr.Buffer = NULL;
                    fileref->fcb->ea_xattr.Length = fileref->fcb->ea_xattr.MaximumLength = 0;
                    
                    fileref->fcb->ea_changed = TRUE;
                    fileref->fcb->ealen = 0;
                }
            }
            
            filter = FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
            
            mark_fcb_dirty(fileref->fcb);
            
            oldatts = fileref->fcb->atts;
            
            defda = get_file_attributes(Vcb, &fileref->fcb->inode_item, fileref->fcb->subvol, fileref->fcb->inode, fileref->fcb->type,
                                        fileref->filepart.Length > 0 && fileref->filepart.Buffer[0] == '.', TRUE, Irp);
            
            if (RequestedDisposition == FILE_SUPERSEDE)
                fileref->fcb->atts = Stack->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
            else
                fileref->fcb->atts |= Stack->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
            
            if (fileref->fcb->atts != oldatts) {
                fileref->fcb->atts_changed = TRUE;
                fileref->fcb->atts_deleted = Stack->Parameters.Create.FileAttributes == defda;
                filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
            }
            
            KeQuerySystemTime(&time);
            win_time_to_unix(time, &now);
            
            fileref->fcb->inode_item.transid = Vcb->superblock.generation;
            fileref->fcb->inode_item.sequence++;
            fileref->fcb->inode_item.st_ctime = now;
            fileref->fcb->inode_item.st_mtime = now;
            fileref->fcb->inode_item_changed = TRUE;

            // FIXME - truncate streams
            // FIXME - do we need to alter parent directory's times?
            
            send_notification_fcb(fileref, filter, FILE_ACTION_MODIFIED);
        } else {
            if (options & FILE_NO_EA_KNOWLEDGE && fileref->fcb->ea_xattr.Length > 0) {
                FILE_FULL_EA_INFORMATION* ffei = (FILE_FULL_EA_INFORMATION*)fileref->fcb->ea_xattr.Buffer;
                
                do {
                    if (ffei->Flags & FILE_NEED_EA) {
                        WARN("returning STATUS_ACCESS_DENIED as no EA knowledge\n");
                        free_fileref(fileref);
                        Status = STATUS_ACCESS_DENIED;
                        goto exit;
                    }
                    
                    if (ffei->NextEntryOffset == 0)
                        break;
                    
                    ffei = (FILE_FULL_EA_INFORMATION*)(((UINT8*)ffei) + ffei->NextEntryOffset);
                } while (TRUE);
            }
        }
    
        FileObject->FsContext = fileref->fcb;
        
        ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
        if (!ccb) {
            ERR("out of memory\n");
            free_fileref(fileref);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        RtlZeroMemory(ccb, sizeof(*ccb));
        
        ccb->NodeType = BTRFS_NODE_TYPE_CCB;
        ccb->NodeSize = sizeof(ccb);
        ccb->disposition = RequestedDisposition;
        ccb->options = options;
        ccb->query_dir_offset = 0;
        RtlInitUnicodeString(&ccb->query_string, NULL);
        ccb->has_wildcard = FALSE;
        ccb->specific_file = FALSE;
        ccb->access = granted_access;
        ccb->case_sensitive = Stack->Flags & SL_CASE_SENSITIVE;
        
        ccb->fileref = fileref;
        
        FileObject->FsContext2 = ccb;
        
        if (fn_offset > 0) {
            fn.Length -= fn_offset * sizeof(WCHAR);
            RtlMoveMemory(&fn.Buffer[0], &fn.Buffer[fn_offset], fn.Length);
        }
        
        FileObject->SectionObjectPointer = &fileref->fcb->nonpaged->segment_object;
        
        if (NT_SUCCESS(Status)) {
            switch (RequestedDisposition) {
                case FILE_SUPERSEDE:
                    Irp->IoStatus.Information = FILE_SUPERSEDED;
                    break;
                    
                case FILE_OPEN:
                case FILE_OPEN_IF:
                    Irp->IoStatus.Information = FILE_OPENED;
                    break;
                    
                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                    Irp->IoStatus.Information = FILE_OVERWRITTEN;
                    break;
            }
        }
        
        // Make sure paging files don't have any extents marked as being prealloc,
        // as this would mean we'd have to lock exclusively when writing.
        if (Stack->Flags & SL_OPEN_PAGING_FILE) {
            LIST_ENTRY* le;
            BOOL changed = FALSE;
            
            ExAcquireResourceExclusiveLite(fileref->fcb->Header.Resource, TRUE);
            
            le = fileref->fcb->extents.Flink;
            
            while (le != &fileref->fcb->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);
                
                if (ext->data->type == EXTENT_TYPE_PREALLOC) {
                    ext->data->type = EXTENT_TYPE_REGULAR;
                    changed = TRUE;
                }
                
                le = le->Flink;
            }
            
            ExReleaseResourceLite(fileref->fcb->Header.Resource);
            
            if (changed) {
                fileref->fcb->extents_changed = TRUE;
                mark_fcb_dirty(fileref->fcb);
            }
            
            fileref->fcb->Header.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
            Vcb->disallow_dismount = TRUE;
        }
        
#ifdef DEBUG_FCB_REFCOUNTS
        oc = InterlockedIncrement(&fileref->open_count);
        ERR("fileref %p: open_count now %i\n", fileref, oc);
#else
        InterlockedIncrement(&fileref->open_count);
#endif
        InterlockedIncrement(&Vcb->open_files);
    } else {
#ifdef DEBUG_STATS
        open_type = 2;
#endif
        Status = file_create(Irp, DeviceObject->DeviceExtension, FileObject, related, loaded_related, &fn, RequestedDisposition, options, rollback);
        Irp->IoStatus.Information = NT_SUCCESS(Status) ? FILE_CREATED : 0;
    }
    
    if (NT_SUCCESS(Status) && !(options & FILE_NO_INTERMEDIATE_BUFFERING))
        FileObject->Flags |= FO_CACHE_SUPPORTED;
    
exit:
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
exit2:
    if (loaded_related)
        free_fileref(related);
    
    if (NT_SUCCESS(Status)) {
        if (!FileObject->Vpb)
            FileObject->Vpb = DeviceObject->Vpb;
    } else {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
            TRACE("returning %08x\n", Status);
    }
    
#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);
    
    if (open_type == 0) {
        Vcb->stats.open_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_opens++;
    } else if (open_type == 1) {
        Vcb->stats.overwrite_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_overwrites++;
    } else if (open_type == 2) {
        Vcb->stats.create_total_time += time2.QuadPart - time1.QuadPart;
        Vcb->stats.num_creates++;
    }
#endif
    
    return Status;
}

NTSTATUS verify_vcb(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        
        if (dev->removable) {
            ULONG cc;
            IO_STATUS_BLOCK iosb;
            
            Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), TRUE, &iosb);
            
            if (!NT_SUCCESS(Status)) {
                ERR("dev_ioctl returned %08x\n", Status);
                goto end;
            }
            
            if (iosb.Information < sizeof(ULONG)) {
                ERR("iosb.Information was too short\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            if (cc != dev->change_count) {
                PDEVICE_OBJECT devobj;
                
                dev->devobj->Flags |= DO_VERIFY_VOLUME;
                
                devobj = IoGetDeviceToVerify(Irp->Tail.Overlay.Thread);
                IoSetDeviceToVerify(Irp->Tail.Overlay.Thread, NULL);
                
                if (!devobj) {
                    devobj = IoGetDeviceToVerify(PsGetCurrentThread());
                    IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
                }
                
                devobj = Vcb->Vpb ? Vcb->Vpb->RealDevice : NULL;
                
                if (devobj)
                    IoVerifyVolume(devobj, FALSE);
                
                Status = STATUS_VERIFY_REQUIRED;
                goto end;
            }
        }
        
        le = le->Flink;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&Vcb->tree_lock);
    
    return Status;
}

static BOOL has_manage_volume_privilege(ACCESS_STATE* access_state, KPROCESSOR_MODE processor_mode) {
    PRIVILEGE_SET privset;

    privset.PrivilegeCount = 1;
    privset.Control = PRIVILEGE_SET_ALL_NECESSARY;
    privset.Privilege[0].Luid = RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE);
    privset.Privilege[0].Attributes = 0;

    return SePrivilegeCheck(&privset, &access_state->SubjectSecurityContext, processor_mode) ? TRUE : FALSE;
}

NTSTATUS STDCALL drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level, locked = FALSE;
    LIST_ENTRY rollback;
    
    TRACE("create (flags = %x)\n", Irp->Flags);
    
    InitializeListHead(&rollback);
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    /* return success if just called for FS device object */
    if (DeviceObject == devobj || (Vcb && Vcb->type == VCB_TYPE_PARTITION0))  {
        TRACE("create called for FS device object\n");
        
        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;

        goto exit;
    }
    
    Status = verify_vcb(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("verify_vcb returned %08x\n", Status);
        goto exit;
    }
    
    ExAcquireResourceSharedLite(&Vcb->load_lock, TRUE);
    locked = TRUE;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    if (IrpSp->Flags != 0) {
        UINT32 flags = IrpSp->Flags;
        
        TRACE("flags:\n");
        
        if (flags & SL_CASE_SENSITIVE) {
            TRACE("SL_CASE_SENSITIVE\n");
            flags &= ~SL_CASE_SENSITIVE;
        }
        
        if (flags & SL_FORCE_ACCESS_CHECK) {
            TRACE("SL_FORCE_ACCESS_CHECK\n");
            flags &= ~SL_FORCE_ACCESS_CHECK;
        }
        
        if (flags & SL_OPEN_PAGING_FILE) {
            TRACE("SL_OPEN_PAGING_FILE\n");
            flags &= ~SL_OPEN_PAGING_FILE;
        }
        
        if (flags & SL_OPEN_TARGET_DIRECTORY) {
            TRACE("SL_OPEN_TARGET_DIRECTORY\n");
            flags &= ~SL_OPEN_TARGET_DIRECTORY;
        }
        
        if (flags & SL_STOP_ON_SYMLINK) {
            TRACE("SL_STOP_ON_SYMLINK\n");
            flags &= ~SL_STOP_ON_SYMLINK;
        }
        
        if (flags)
            WARN("unknown flags: %x\n", flags);
    } else {
        TRACE("flags: (none)\n");
    }
    
//     Vpb = DeviceObject->DeviceExtension;
    
//     TRACE("create called for something other than FS device object\n");
    
    // opening volume
    // FIXME - also check if RelatedFileObject is Vcb
    if (IrpSp->FileObject->FileName.Length == 0 && !IrpSp->FileObject->RelatedFileObject) {
        ULONG RequestedDisposition = ((IrpSp->Parameters.Create.Options >> 24) & 0xff);
        ULONG RequestedOptions = IrpSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
#ifdef DEBUG_FCB_REFCOUNTS
        LONG rc, oc;
#endif
        ccb* ccb;
        
        TRACE("open operation for volume\n");

        if (RequestedDisposition != FILE_OPEN && RequestedDisposition != FILE_OPEN_IF) {
            Status = STATUS_ACCESS_DENIED;
            goto exit;
        }

        if (RequestedOptions & FILE_DIRECTORY_FILE) {
            Status = STATUS_NOT_A_DIRECTORY;
            goto exit;
        }
        
        if (Vcb->removing) {
            Status = STATUS_ACCESS_DENIED;
            goto exit;
        }
        
        ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
        if (!ccb) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        RtlZeroMemory(ccb, sizeof(*ccb));
        
        ccb->NodeType = BTRFS_NODE_TYPE_CCB;
        ccb->NodeSize = sizeof(ccb);
        ccb->disposition = RequestedDisposition;
        ccb->options = RequestedOptions;
        ccb->access = IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;
        ccb->manage_volume_privilege = has_manage_volume_privilege(IrpSp->Parameters.Create.SecurityContext->AccessState,
                                                                   IrpSp->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode);

#ifdef DEBUG_FCB_REFCOUNTS
        rc = InterlockedIncrement(&Vcb->volume_fcb->refcount);
        WARN("fcb %p: refcount now %i (volume)\n", Vcb->volume_fcb, rc);
#else
        InterlockedIncrement(&Vcb->volume_fcb->refcount);
#endif
        IrpSp->FileObject->FsContext = Vcb->volume_fcb;
        IrpSp->FileObject->FsContext2 = ccb;
        
        IrpSp->FileObject->SectionObjectPointer = &Vcb->volume_fcb->nonpaged->segment_object;

        if (!IrpSp->FileObject->Vpb)
            IrpSp->FileObject->Vpb = DeviceObject->Vpb;
        
        InterlockedIncrement(&Vcb->open_files);

        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
    } else {
        BOOL skip_lock;
        
        TRACE("file name: %.*S\n", IrpSp->FileObject->FileName.Length / sizeof(WCHAR), IrpSp->FileObject->FileName.Buffer);
        
        if (IrpSp->FileObject->RelatedFileObject)
            TRACE("related file = %S\n", file_desc(IrpSp->FileObject->RelatedFileObject));
        
        // Don't lock again if we're being called from within CcCopyRead etc.
        skip_lock = ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock);

        if (!skip_lock)
            ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
        
        Status = open_file(DeviceObject, Irp, &rollback);
        
        if (!NT_SUCCESS(Status))
            do_rollback(Vcb, &rollback);
        else
            clear_rollback(Vcb, &rollback);
        
        if (!skip_lock)
            ExReleaseResourceLite(&Vcb->tree_lock);
        
//         Status = STATUS_ACCESS_DENIED;
    }
    
exit:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT );
//     IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    
    TRACE("create returning %08x\n", Status);
    
    if (locked)
        ExReleaseResourceLite(&Vcb->load_lock);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
