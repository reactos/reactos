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

#include <sys/stat.h>
#include "btrfs_drv.h"

extern PDEVICE_OBJECT devobj;

BOOL STDCALL find_file_in_dir_with_crc32(device_extension* Vcb, PUNICODE_STRING filename, UINT32 crc32, root* r,
                                         UINT64 parinode, root** subvol, UINT64* inode, UINT8* type, PANSI_STRING utf8) {
    DIR_ITEM* di;
    KEY searchkey;
    traverse_ptr tp, tp2, next_tp;
    BOOL b;
    NTSTATUS Status;
    ULONG stringlen;
    
    TRACE("(%p, %.*S, %08x, %p, %llx, %p, %p, %p)\n", Vcb, filename->Length / sizeof(WCHAR), filename->Buffer, crc32, r, parinode, subvol, inode, type);
    
    searchkey.obj_id = parinode;
    searchkey.obj_type = TYPE_DIR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, r, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    TRACE("found item %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        UINT32 size = tp.item->size;
        
        // found by hash
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            WARN("(%llx;%llx,%x,%llx) was %u bytes, expected at least %u\n", r->id, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        } else {
            di = (DIR_ITEM*)tp.item->data;
            
            while (size > 0) {
                if (size < sizeof(DIR_ITEM) || size < (sizeof(DIR_ITEM) - 1 + di->m + di->n)) {
                    WARN("(%llx,%x,%llx) is truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                size -= sizeof(DIR_ITEM) - sizeof(char);
                size -= di->n;
                size -= di->m;
                
                Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, di->name, di->n);
                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                } else {
                    WCHAR* utf16 = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                    UNICODE_STRING us;
                    
                    if (!utf16) {
                        ERR("out of memory\n");
                        free_traverse_ptr(&tp);
                        return FALSE;
                    }
                    
                    Status = RtlUTF8ToUnicodeN(utf16, stringlen, &stringlen, di->name, di->n);

                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                    } else {
                        us.Buffer = utf16;
                        us.Length = us.MaximumLength = (USHORT)stringlen;
                        
                        if (FsRtlAreNamesEqual(filename, &us, TRUE, NULL)) {
                            if (di->key.obj_type == TYPE_ROOT_ITEM) {
                                root* fcbroot = Vcb->roots;
                                while (fcbroot && fcbroot->id != di->key.obj_id)
                                    fcbroot = fcbroot->next;
                                
                                *subvol = fcbroot;
                                *inode = SUBVOL_ROOT_INODE;
                                *type = BTRFS_TYPE_DIRECTORY;
                            } else {
                                *subvol = r;
                                *inode = di->key.obj_id;
                                *type = di->type;
                            }
                            
                            if (utf8) {
                                utf8->MaximumLength = di->n;
                                utf8->Length = utf8->MaximumLength;
                                utf8->Buffer = ExAllocatePoolWithTag(PagedPool, utf8->MaximumLength, ALLOC_TAG);
                                if (!utf8->Buffer) {
                                    ERR("out of memory\n");
                                    free_traverse_ptr(&tp);
                                    ExFreePool(utf16);
                                    return FALSE;
                                }
                                
                                RtlCopyMemory(utf8->Buffer, di->name, di->n);
                            }
                            
                            free_traverse_ptr(&tp);
                            ExFreePool(utf16);
                            
                            TRACE("found %.*S by hash at (%llx,%llx)\n", filename->Length / sizeof(WCHAR), filename->Buffer, (*subvol)->id, *inode);
                            
                            return TRUE;
                        }
                    }
                    
                    ExFreePool(utf16);
                }
                
                di = (DIR_ITEM*)&di->name[di->n + di->m];
            }
        }
    }
    
    searchkey.obj_id = parinode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = 2;
    
    Status = find_item(Vcb, r, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        free_traverse_ptr(&tp);
        return FALSE;
    }
    
    free_traverse_ptr(&tp);
    tp = tp2;
    
    TRACE("found item %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
    
    if (keycmp(&tp.item->key, &searchkey) == -1) {
        if (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            TRACE("moving on to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }
    
    if (tp.item->key.obj_id != parinode || tp.item->key.obj_type != TYPE_DIR_INDEX) {
        free_traverse_ptr(&tp);
        return FALSE;
    }
    
    b = TRUE;
    do {
        TRACE("key: %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        di = (DIR_ITEM*)tp.item->data;
        
        if (tp.item->size < sizeof(DIR_ITEM) || tp.item->size < (sizeof(DIR_ITEM) - 1 + di->m + di->n)) {
            WARN("(%llx,%x,%llx) is truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        } else {    
            TRACE("%.*s\n", di->n, di->name);
            
            Status = RtlUTF8ToUnicodeN(NULL, 0, &stringlen, di->name, di->n);
            if (!NT_SUCCESS(Status)) {
                ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
            } else {
                WCHAR* utf16 = ExAllocatePoolWithTag(PagedPool, stringlen, ALLOC_TAG);
                UNICODE_STRING us;
                
                if (!utf16) {
                    ERR("out of memory\n");
                    
                    free_traverse_ptr(&tp);
                    return FALSE;
                }
                
                Status = RtlUTF8ToUnicodeN(utf16, stringlen, &stringlen, di->name, di->n);

                if (!NT_SUCCESS(Status)) {
                    ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                } else {
                    us.Buffer = utf16;
                    us.Length = us.MaximumLength = (USHORT)stringlen;
                    
                    if (FsRtlAreNamesEqual(filename, &us, TRUE, NULL)) {
                        if (di->key.obj_type == TYPE_ROOT_ITEM) {
                            root* fcbroot = Vcb->roots;
                            while (fcbroot && fcbroot->id != di->key.obj_id)
                                fcbroot = fcbroot->next;
                            
                            *subvol = fcbroot;
                            *inode = SUBVOL_ROOT_INODE;
                            *type = BTRFS_TYPE_DIRECTORY;
                        } else {
                            *subvol = r;
                            *inode = di->key.obj_id;
                            *type = di->type;
                        }
                        TRACE("found %.*S at (%llx,%llx)\n", filename->Length / sizeof(WCHAR), filename->Buffer, (*subvol)->id, *inode);
                        
                        if (utf8) {
                            utf8->MaximumLength = di->n;
                            utf8->Length = utf8->MaximumLength;
                            utf8->Buffer = ExAllocatePoolWithTag(PagedPool, utf8->MaximumLength, ALLOC_TAG);
                            if (!utf8->Buffer) {
                                ERR("out of memory\n");
                                free_traverse_ptr(&tp);
                                ExFreePool(utf16);
                                
                                return FALSE;
                            }
                            
                            RtlCopyMemory(utf8->Buffer, di->name, di->n);
                        }
                        
                        free_traverse_ptr(&tp);
                        ExFreePool(utf16);
                        
                        return TRUE;
                    }
                }
                
                ExFreePool(utf16);
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
         
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            b = tp.item->key.obj_id == parinode && tp.item->key.obj_type == TYPE_DIR_INDEX;
        }
    } while (b);
    
    free_traverse_ptr(&tp);

    return FALSE;
}

fcb* create_fcb() {
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
    
    FsRtlInitializeFileLock(&fcb->lock, NULL, NULL);
    
    InitializeListHead(&fcb->children);
    
    return fcb;
}

static BOOL STDCALL find_file_in_dir(device_extension* Vcb, PUNICODE_STRING filename, root* r,
                                     UINT64 parinode, root** subvol, UINT64* inode, UINT8* type, PANSI_STRING utf8) {
    char* fn;
    UINT32 crc32;
    BOOL ret;
    ULONG utf8len;
    NTSTATUS Status;
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, filename->Buffer, filename->Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N 1 returned %08x\n", Status);
        return FALSE;
    }
    
    fn = ExAllocatePoolWithTag(PagedPool, utf8len, ALLOC_TAG);
    if (!fn) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    Status = RtlUnicodeToUTF8N(fn, utf8len, &utf8len, filename->Buffer, filename->Length);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(fn);
        ERR("RtlUnicodeToUTF8N 2 returned %08x\n", Status);
        return FALSE;
    }
    
    TRACE("%.*s\n", utf8len, fn);
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)fn, (ULONG)utf8len);
    TRACE("crc32c(%.*s) = %08x\n", utf8len, fn, crc32);
    
    ret = find_file_in_dir_with_crc32(Vcb, filename, crc32, r, parinode, subvol, inode, type, utf8);
    
    return ret;
}

static BOOL find_stream(device_extension* Vcb, fcb* fcb, PUNICODE_STRING stream, PUNICODE_STRING newstreamname, UINT32* size, UINT32* hash, PANSI_STRING xattr) {
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
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!keycmp(&tp.item->key, &searchkey)) {
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
                    
                    *size = di->m;
                    *hash = tp.item->key.offset;
                    
                    xattr->Buffer = ExAllocatePoolWithTag(PagedPool, di->n + 1, ALLOC_TAG);
                    if (!xattr->Buffer) {
                        ERR("out of memory\n");
                        free_traverse_ptr(&tp);
                        goto end;
                    }
                    
                    xattr->Length = xattr->MaximumLength = di->n;
                    RtlCopyMemory(xattr->Buffer, di->name, di->n);
                    xattr->Buffer[di->n] = 0;
                    
                    free_traverse_ptr(&tp);
                    
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
    
    free_traverse_ptr(&tp);
    
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
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
                            free_traverse_ptr(&tp);
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
                                *size = di->m;
                                *hash = tp.item->key.offset;
                                
                                xattr->Buffer = ExAllocatePoolWithTag(PagedPool, di->n + 1, ALLOC_TAG);
                                if (!xattr->Buffer) {
                                    ERR("out of memory\n");
                                    free_traverse_ptr(&tp);
                                    ExFreePool(utf16);
                                    goto end;
                                }
                                
                                xattr->Length = xattr->MaximumLength = di->n;
                                RtlCopyMemory(xattr->Buffer, di->name, di->n);
                                xattr->Buffer[di->n] = 0;
                                
                                free_traverse_ptr(&tp);
                                
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
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (next_tp.item->key.obj_id > fcb->inode || next_tp.item->key.obj_type > TYPE_XATTR_ITEM)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);
   
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

static fcb* search_fcb_children(fcb* dir, PUNICODE_STRING name) {
    LIST_ENTRY* le;
    fcb *c, *deleted = NULL;
    
    le = dir->children.Flink;
    while (le != &dir->children) {
        c = CONTAINING_RECORD(le, fcb, list_entry);
        
        if (c->refcount > 0 && FsRtlAreNamesEqual(&c->filepart, name, TRUE, NULL)) {
            if (c->deleted) {
                deleted = c;
            } else {
                c->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
                WARN("fcb %p: refcount now %i (%.*S)\n", c, c->refcount, c->full_filename.Length / sizeof(WCHAR), c->full_filename.Buffer);
#endif
                return c;
            }
        }
        
        le = le->Flink;
    }
    
    return deleted;
}

#ifdef DEBUG_FCB_REFCOUNTS
static void print_fcbs(device_extension* Vcb) {
    fcb* fcb = Vcb->fcbs;
    
    while (fcb) {
        ERR("fcb %p (%.*S): refcount %u\n", fcb, fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fcb->refcount);
        
        fcb = fcb->next;
    }
}
#endif

NTSTATUS get_fcb(device_extension* Vcb, fcb** pfcb, PUNICODE_STRING fnus, fcb* relatedfcb, BOOL parent) {
    fcb *dir, *sf, *sf2;
    ULONG i, num_parts;
    UNICODE_STRING fnus2;
    UNICODE_STRING* parts = NULL;
    BOOL has_stream;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %.*S, %p, %s)\n", Vcb, pfcb, fnus->Length / sizeof(WCHAR), fnus->Buffer, relatedfcb, parent ? "TRUE" : "FALSE");
    
#ifdef DEBUG_FCB_REFCOUNTS
    print_fcbs(Vcb);
#endif
    
    fnus2 = *fnus;
    
    if (fnus2.Length < sizeof(WCHAR) && !relatedfcb) {
        ERR("error - fnus was too short\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (relatedfcb) {
        dir = relatedfcb;
    } else {
        if (fnus2.Buffer[0] != '\\') {
            ERR("error - filename %.*S did not begin with \\\n", fnus2.Length / sizeof(WCHAR), fnus2.Buffer);
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }
        
        if (fnus2.Length == sizeof(WCHAR)) {
            *pfcb = Vcb->root_fcb;
            Vcb->root_fcb->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
            WARN("fcb %p: refcount now %i (root)\n", Vcb->root_fcb, Vcb->root_fcb->refcount);
#endif
            return STATUS_SUCCESS;
        }
        
        dir = Vcb->root_fcb;
        
        fnus2.Buffer++;
        fnus2.Length -= sizeof(WCHAR);
        fnus2.MaximumLength -= sizeof(WCHAR);
    }
    
    if (dir->type != BTRFS_TYPE_DIRECTORY && (fnus->Length < sizeof(WCHAR) || fnus->Buffer[0] != ':')) {
        WARN("passed relatedfcb which isn't a directory (%.*S) (fnus = %.*S)\n",
             relatedfcb->full_filename.Length / sizeof(WCHAR), relatedfcb->full_filename.Buffer, fnus->Length / sizeof(WCHAR), fnus->Buffer);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }
    
    if (fnus->Length == 0) {
        num_parts = 0;
    } else {
        Status = split_path(&fnus2, &parts, &num_parts, &has_stream);
        if (!NT_SUCCESS(Status)) {
            ERR("split_path returned %08x\n", Status);
            return Status;
        }
    }
    
    // FIXME - handle refcounts(?)
    sf = dir;
    dir->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fcb %p: refcount now %i (%.*S)\n", dir, dir->refcount, dir->full_filename.Length / sizeof(WCHAR), dir->full_filename.Buffer);
#endif
    
    if (parent) {
        num_parts--;
        
        if (has_stream) {
            num_parts--;
            has_stream = FALSE;
        }
    }
    
    if (num_parts == 0) {
        Status = STATUS_SUCCESS;
        *pfcb = dir;
        goto end2;
    }
    
    for (i = 0; i < num_parts; i++) {
        BOOL lastpart = (i == num_parts-1) || (i == num_parts-2 && has_stream);
        
        sf2 = search_fcb_children(sf, &parts[i]);
        
        if (sf2 && sf2->type != BTRFS_TYPE_DIRECTORY && !lastpart) {
            WARN("passed path including file as subdirectory\n");
            
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            goto end;
        }
        
        if (!sf2) {
            if (has_stream && i == num_parts - 1) {
                UNICODE_STRING streamname;
                ANSI_STRING xattr;
                UINT32 streamsize, streamhash;
                
                streamname.Buffer = NULL;
                streamname.Length = streamname.MaximumLength = 0;
                xattr.Buffer = NULL;
                xattr.Length = xattr.MaximumLength = 0;
                
                if (!find_stream(Vcb, sf, &parts[i], &streamname, &streamsize, &streamhash, &xattr)) {
                    TRACE("could not find stream %.*S\n", parts[i].Length / sizeof(WCHAR), parts[i].Buffer);
                    
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    goto end;
                } else {
                    ULONG fnlen;
                    
                    if (streamhash == EA_DOSATTRIB_HASH && xattr.Length == strlen(EA_DOSATTRIB) &&
                        RtlCompareMemory(xattr.Buffer, EA_DOSATTRIB, xattr.Length) == xattr.Length) {
                        WARN("not allowing user.DOSATTRIB to be opened as stream\n");
                    
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        goto end;
                    }
                    
                    sf2 = create_fcb();
                    if (!sf2) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
        
                    sf2->Vcb = Vcb;
        
                    if (streamname.Buffer) // case has changed
                        sf2->filepart = streamname;
                    else {
                        sf2->filepart.MaximumLength = sf2->filepart.Length = parts[i].Length;
                        sf2->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, sf2->filepart.MaximumLength, ALLOC_TAG);
                        if (!sf2->filepart.Buffer) {
                            ERR("out of memory\n");
                            free_fcb(sf2);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }   
                        
                        RtlCopyMemory(sf2->filepart.Buffer, parts[i].Buffer, parts[i].Length);
                    }
                    
                    sf2->par = sf;
                    
                    sf->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
                    WARN("fcb %p: refcount now %i (%.*S)\n", sf, sf->refcount, sf->full_filename.Length / sizeof(WCHAR), sf->full_filename.Buffer);
#endif
                    
                    sf2->subvol = sf->subvol;
                    sf2->inode = sf->inode;
                    sf2->type = sf->type;
                    sf2->ads = TRUE;
                    sf2->adssize = streamsize;
                    sf2->adshash = streamhash;
                    sf2->adsxattr = xattr;
                    
                    TRACE("stream found: size = %x, hash = %08x\n", sf2->adssize, sf2->adshash);
                    
                    if (Vcb->fcbs)
                        Vcb->fcbs->prev = sf2;
                    
                    sf2->next = Vcb->fcbs;
                    Vcb->fcbs = sf2;
                    
                    sf2->name_offset = sf->full_filename.Length / sizeof(WCHAR);
                    
                    if (sf != Vcb->root_fcb)
                        sf2->name_offset++;
                    
                    fnlen = (sf2->name_offset * sizeof(WCHAR)) + sf2->filepart.Length;
                    
                    sf2->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fnlen, ALLOC_TAG);
                    if (!sf2->full_filename.Buffer) {
                        ERR("out of memory\n");
                        free_fcb(sf2);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    sf2->full_filename.Length = sf2->full_filename.MaximumLength = fnlen;
                    RtlCopyMemory(sf2->full_filename.Buffer, sf->full_filename.Buffer, sf->full_filename.Length);
                    
                    sf2->full_filename.Buffer[sf->full_filename.Length / sizeof(WCHAR)] = ':';
                    
                    RtlCopyMemory(&sf2->full_filename.Buffer[sf2->name_offset], sf2->filepart.Buffer, sf2->filepart.Length);
                    
                    // FIXME - make sure all functions know that ADS FCBs won't have a valid SD or INODE_ITEM

                    TRACE("found stream %.*S (subvol = %p)\n", sf2->full_filename.Length / sizeof(WCHAR), sf2->full_filename.Buffer, sf->subvol);
                    
                    InsertTailList(&sf->children, &sf2->list_entry);
                }
            } else {
                root* subvol;
                UINT64 inode;
                UINT8 type;
                ANSI_STRING utf8;
                KEY searchkey;
                traverse_ptr tp;
                
                if (!find_file_in_dir(Vcb, &parts[i], sf->subvol, sf->inode, &subvol, &inode, &type, &utf8)) {
                    TRACE("could not find %.*S\n", parts[i].Length / sizeof(WCHAR), parts[i].Buffer);

                    Status = lastpart ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_OBJECT_PATH_NOT_FOUND;
                    goto end;
                } else if (type != BTRFS_TYPE_DIRECTORY && !lastpart) {
                    WARN("passed path including file as subdirectory\n");
                    
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    goto end;
                } else {
                    ULONG fnlen, strlen;
                    
                    sf2 = create_fcb();
                    if (!sf2) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    sf2->Vcb = Vcb;

                    Status = RtlUTF8ToUnicodeN(NULL, 0, &strlen, utf8.Buffer, utf8.Length);
                    if (!NT_SUCCESS(Status)) {
                        ERR("RtlUTF8ToUnicodeN 1 returned %08x\n", Status);
                        free_fcb(sf2);
                        goto end;
                    } else {
                        sf2->filepart.MaximumLength = sf2->filepart.Length = strlen;
                        sf2->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, sf2->filepart.MaximumLength, ALLOC_TAG);
                        if (!sf2->filepart.Buffer) {
                            ERR("out of memory\n");
                            free_fcb(sf2);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        Status = RtlUTF8ToUnicodeN(sf2->filepart.Buffer, strlen, &strlen, utf8.Buffer, utf8.Length);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("RtlUTF8ToUnicodeN 2 returned %08x\n", Status);
                            free_fcb(sf2);
                            goto end;
                        }
                    }
                    
                    sf2->par = sf;
                    
                    sf->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
                    WARN("fcb %p: refcount now %i (%.*S)\n", sf, sf->refcount, sf->full_filename.Length / sizeof(WCHAR), sf->full_filename.Buffer);
#endif
                    
                    sf2->subvol = subvol;
                    sf2->inode = inode;
                    sf2->type = type;
                    
                    if (Vcb->fcbs)
                        Vcb->fcbs->prev = sf2;
                    
                    sf2->next = Vcb->fcbs;
                    Vcb->fcbs = sf2;
                    
                    sf2->name_offset = sf->full_filename.Length / sizeof(WCHAR);
                    
                    if (sf != Vcb->root_fcb)
                        sf2->name_offset++;
                    
                    fnlen = (sf2->name_offset * sizeof(WCHAR)) + sf2->filepart.Length;
                    
                    sf2->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fnlen, ALLOC_TAG);
                    if (!sf2->full_filename.Buffer) {
                        ERR("out of memory\n");
                        free_fcb(sf2);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    sf2->full_filename.Length = sf2->full_filename.MaximumLength = fnlen;
                    RtlCopyMemory(sf2->full_filename.Buffer, sf->full_filename.Buffer, sf->full_filename.Length);
                    
                    if (sf != Vcb->root_fcb)
                        sf2->full_filename.Buffer[sf->full_filename.Length / sizeof(WCHAR)] = '\\';
                    
                    RtlCopyMemory(&sf2->full_filename.Buffer[sf2->name_offset], sf2->filepart.Buffer, sf2->filepart.Length);
                    
                    sf2->utf8 = utf8;
                    
                    searchkey.obj_id = sf2->inode;
                    searchkey.obj_type = TYPE_INODE_ITEM;
                    searchkey.offset = 0xffffffffffffffff;
                    
                    Status = find_item(sf2->Vcb, sf2->subvol, &tp, &searchkey, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        free_fcb(sf2);
                        goto end;
                    }
                    
                    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                        ERR("couldn't find INODE_ITEM for inode %llx in subvol %llx\n", sf2->inode, sf2->subvol->id);
                        Status = STATUS_INTERNAL_ERROR;
                        free_fcb(sf2);
                        free_traverse_ptr(&tp);
                        goto end;
                    }
                    
                    if (tp.item->size > 0)
                        RtlCopyMemory(&sf2->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
                    
                    free_traverse_ptr(&tp);
                    
                    sf2->atts = get_file_attributes(Vcb, &sf2->inode_item, sf2->subvol, sf2->inode, sf2->type, sf2->filepart.Buffer[0] == '.', FALSE);
                    
                    fcb_get_sd(sf2);
                    
                    TRACE("found %.*S (subvol = %p)\n", sf2->full_filename.Length / sizeof(WCHAR), sf2->full_filename.Buffer, subvol);
                    
                    InsertTailList(&sf->children, &sf2->list_entry);
                }
            }
        }
        
        if (i == num_parts - 1)
            break;
        
        free_fcb(sf);
        sf = sf2;
    }
    
    Status = STATUS_SUCCESS;
    *pfcb = sf2;
    
end:
    free_fcb(sf);
    
end2:
    if (parts)
        ExFreePool(parts);
    
#ifdef DEBUG_FCB_REFCOUNTS
    print_fcbs(Vcb);
#endif
    
    TRACE("returning %08x\n", Status);
    
    return Status;
}

static NTSTATUS STDCALL attach_fcb_to_fileobject(device_extension* Vcb, fcb* fcb, PFILE_OBJECT FileObject) {
    FileObject->FsContext = fcb;
//     FileObject->FsContext2 = 0x0badc0de;//NULL;
    
    // FIXME - cache stuff
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL file_create2(PIRP Irp, device_extension* Vcb, PUNICODE_STRING fpus, fcb* parfcb, ULONG options, fcb** pfcb, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb* fcb;
    ULONG utf8len;
    char* utf8 = NULL;
    UINT32 crc32;
    UINT64 dirpos, inode;
    KEY searchkey;
    traverse_ptr tp;
    INODE_ITEM *dirii, *ii;
    UINT8 type;
    ULONG disize;
    DIR_ITEM *di, *di2;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ANSI_STRING utf8as;
    ULONG defda;
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, fpus->Buffer, fpus->Length);
    if (!NT_SUCCESS(Status))
        return Status;
    
    utf8 = ExAllocatePoolWithTag(PagedPool, utf8len + 1, ALLOC_TAG);
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
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8, utf8len);
    
    dirpos = find_next_dir_index(Vcb, parfcb->subvol, parfcb->inode);
    if (dirpos == 0) {
        Status = STATUS_INTERNAL_ERROR;
        ExFreePool(utf8);
        return Status;
    }
    
    TRACE("filename = %s, crc = %08x, dirpos = %llx\n", utf8, crc32, dirpos);
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    TRACE("parfcb->inode_item.st_size was %llx\n", parfcb->inode_item.st_size);
    parfcb->inode_item.st_size += utf8len * 2;
    TRACE("parfcb->inode_item.st_size was %llx\n", parfcb->inode_item.st_size);
    parfcb->inode_item.transid = Vcb->superblock.generation;
    parfcb->inode_item.sequence++;
    parfcb->inode_item.st_ctime = now;
    parfcb->inode_item.st_mtime = now;
    
    searchkey.obj_id = parfcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, parfcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("error - could not find INODE_ITEM for parent directory %llx in subvol %llx\n", parfcb->inode, parfcb->subvol->id);
        free_traverse_ptr(&tp);
        ExFreePool(utf8);
        return STATUS_INTERNAL_ERROR;
    }
    
    dirii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!dirii) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(dirii, &parfcb->inode_item, sizeof(INODE_ITEM));
    delete_tree_item(Vcb, &tp, rollback);
    
    insert_tree_item(Vcb, parfcb->subvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, dirii, sizeof(INODE_ITEM), NULL, rollback);
    
    free_traverse_ptr(&tp);
    
    if (parfcb->subvol->lastinode == 0)
        get_last_inode(Vcb, parfcb->subvol);
    
    inode = parfcb->subvol->lastinode + 1;
    
    type = options & FILE_DIRECTORY_FILE ? BTRFS_TYPE_DIRECTORY : BTRFS_TYPE_FILE;
    
    disize = sizeof(DIR_ITEM) - 1 + utf8len;
    di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    di->key.obj_id = inode;
    di->key.obj_type = TYPE_INODE_ITEM;
    di->key.offset = 0;
    di->transid = Vcb->superblock.generation;
    di->m = 0;
    di->n = (UINT16)utf8len;
    di->type = type;
    RtlCopyMemory(di->name, utf8, utf8len);
    
    insert_tree_item(Vcb, parfcb->subvol, parfcb->inode, TYPE_DIR_INDEX, dirpos, di, disize, NULL, rollback);
    
    di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
    if (!di2) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(di2, di, disize);
    
    Status = add_dir_item(Vcb, parfcb->subvol, parfcb->inode, crc32, di2, disize, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_dir_item returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }
    
    // FIXME - handle Irp->Overlay.AllocationSize
    
    utf8as.Buffer = utf8;
    utf8as.Length = utf8as.MaximumLength = utf8len;
    
    Status = add_inode_ref(Vcb, parfcb->subvol, inode, parfcb->inode, dirpos, &utf8as, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_inode_ref returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }
    
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
    
    if (IrpSp->Parameters.Create.FileAttributes != defda) {
        char val[64];
    
        sprintf(val, "0x%x", IrpSp->Parameters.Create.FileAttributes);
    
        Status = set_xattr(Vcb, parfcb->subvol, inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (UINT8*)val, strlen(val), rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            ExFreePool(utf8);
            return Status;
        }
    }
    
    parfcb->subvol->lastinode++;
    
    fcb = create_fcb();
    if (!fcb) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
        
    fcb->Vcb = Vcb;
    
    RtlZeroMemory(&fcb->inode_item, sizeof(INODE_ITEM));
    fcb->inode_item.generation = Vcb->superblock.generation;
    fcb->inode_item.transid = Vcb->superblock.generation;
    fcb->inode_item.st_size = 0;
    fcb->inode_item.st_blocks = 0;
    fcb->inode_item.block_group = 0;
    fcb->inode_item.st_nlink = 1;
//     fcb->inode_item.st_uid = UID_NOBODY; // FIXME?
    fcb->inode_item.st_gid = GID_NOBODY; // FIXME?
    fcb->inode_item.st_mode = parfcb ? (parfcb->inode_item.st_mode & ~S_IFDIR) : 0755; // use parent's permissions by default
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
    
    // inherit nodatacow flag from parent directory
    if (parfcb->inode_item.flags & BTRFS_INODE_NODATACOW) {
        fcb->inode_item.flags |= BTRFS_INODE_NODATACOW;
        
        if (type != BTRFS_TYPE_DIRECTORY)
            fcb->inode_item.flags |= BTRFS_INODE_NODATASUM;
    }
    
//     fcb->Header.IsFastIoPossible = TRUE;
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;
    
    fcb->atts = IrpSp->Parameters.Create.FileAttributes;
    
    if (options & FILE_DELETE_ON_CLOSE)
        fcb->delete_on_close = TRUE;
    
    fcb->par = parfcb;
    parfcb->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("fcb %p: refcount now %i (%.*S)\n", parfcb, parfcb->refcount, parfcb->full_filename.Length / sizeof(WCHAR), parfcb->full_filename.Buffer);
#endif
    fcb->subvol = parfcb->subvol;
    fcb->inode = inode;
    fcb->type = type;
    
    fcb->utf8.MaximumLength = fcb->utf8.Length = utf8len;
    fcb->utf8.Buffer = utf8;
    
    Status = fcb_get_new_sd(fcb, IrpSp->Parameters.Create.SecurityContext->AccessState);
    
    if (!NT_SUCCESS(Status)) {
        ERR("fcb_get_new_sd returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }

    fcb->filepart = *fpus;
        
    Status = set_xattr(Vcb, parfcb->subvol, inode, EA_NTACL, EA_NTACL_HASH, (UINT8*)fcb->sd, RtlLengthSecurityDescriptor(fcb->sd), rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("set_xattr returned %08x\n", Status);
        ExFreePool(utf8);
        return Status;
    }
    
    fcb->full_filename.Length = parfcb->full_filename.Length + (parfcb->full_filename.Length == sizeof(WCHAR) ? 0 : sizeof(WCHAR)) + fcb->filepart.Length;
    fcb->full_filename.MaximumLength = fcb->full_filename.Length;
    fcb->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->full_filename.Length, ALLOC_TAG);
    if (!fcb->full_filename.Buffer) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(fcb->full_filename.Buffer, parfcb->full_filename.Buffer, parfcb->full_filename.Length);
    
    if (parfcb->full_filename.Length > sizeof(WCHAR))
        fcb->full_filename.Buffer[parfcb->full_filename.Length / sizeof(WCHAR)] = '\\';
    
    RtlCopyMemory(&fcb->full_filename.Buffer[(parfcb->full_filename.Length / sizeof(WCHAR)) + (parfcb->full_filename.Length == sizeof(WCHAR) ? 0 : 1)], fcb->filepart.Buffer, fcb->filepart.Length);
    
    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        ExFreePool(utf8);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
    insert_tree_item(Vcb, parfcb->subvol, inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
    
    *pfcb = fcb;
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;
    
    InsertTailList(&fcb->par->children, &fcb->list_entry);
    
    TRACE("created new file %.*S in subvol %llx, inode %llx\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer, fcb->subvol->id, fcb->inode);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL file_create(PIRP Irp, device_extension* Vcb, PFILE_OBJECT FileObject, PUNICODE_STRING fnus, ULONG disposition, ULONG options, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    fcb *fcb, *parfcb = NULL;
    ULONG i, j;
    ULONG utf8len;
    ccb* ccb;
    static WCHAR datasuf[] = {':','$','D','A','T','A',0};
    UNICODE_STRING dsus, fpus, stream;
            
    TRACE("(%p, %p, %p, %.*S, %x, %x)\n", Irp, Vcb, FileObject, fnus->Length / sizeof(WCHAR), fnus->Buffer, disposition, options);
    
    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;
    
    dsus.Buffer = datasuf;
    dsus.Length = dsus.MaximumLength = wcslen(datasuf) * sizeof(WCHAR);
    fpus.Buffer = NULL;
    
    // FIXME - apparently you can open streams using RelatedFileObject. How can we test this?
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    Status = get_fcb(Vcb, &parfcb, fnus, FileObject->RelatedFileObject ? FileObject->RelatedFileObject->FsContext : NULL, TRUE);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    if (!NT_SUCCESS(Status))
        goto end;
    
    if (parfcb->type != BTRFS_TYPE_DIRECTORY) {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto end;
    }
    
    if (parfcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    i = (fnus->Length / sizeof(WCHAR))-1;
    while ((fnus->Buffer[i] == '\\' || fnus->Buffer[i] == '/') && i > 0) { i--; }
    
    j = i;
    
    while (i > 0 && fnus->Buffer[i-1] != '\\' && fnus->Buffer[i-1] != '/') { i--; }
    
    fpus.MaximumLength = (j - i + 2) * sizeof(WCHAR);
    fpus.Buffer = ExAllocatePoolWithTag(PagedPool, fpus.MaximumLength, ALLOC_TAG);
    if (!fpus.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    fpus.Length = (j - i + 1) * sizeof(WCHAR);
    
    RtlCopyMemory(fpus.Buffer, &fnus->Buffer[i], (j - i + 1) * sizeof(WCHAR));
    fpus.Buffer[j - i + 1] = 0;
    
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
        struct _fcb* newpar;
        static char xapref[] = "user.";
        ULONG xapreflen = strlen(xapref), fnlen;
        LARGE_INTEGER time;
        BTRFS_TIME now;
        KEY searchkey;
        traverse_ptr tp;
        INODE_ITEM* ii;
        
        TRACE("fpus = %.*S\n", fpus.Length / sizeof(WCHAR), fpus.Buffer);
        TRACE("stream = %.*S\n", stream.Length / sizeof(WCHAR), stream.Buffer);
        
        ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
        Status = get_fcb(Vcb, &newpar, &fpus, parfcb, FALSE);
        ExReleaseResourceLite(&Vcb->fcb_lock);
        
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            Status = file_create2(Irp, Vcb, &fpus, parfcb, options, &newpar, rollback);
        
            if (!NT_SUCCESS(Status)) {
                ERR("file_create2 returned %08x\n", Status);
                goto end;
            }
        } else if (!NT_SUCCESS(Status)) {
            ERR("get_fcb returned %08x\n", Status);
            goto end;
        }
        
        free_fcb(parfcb);
        parfcb = newpar;
        
        if (newpar->type != BTRFS_TYPE_FILE && newpar->type != BTRFS_TYPE_SYMLINK) {
            WARN("parent not file or symlink\n");
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
        
        if (options & FILE_DIRECTORY_FILE) {
            WARN("tried to create directory as stream\n");
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
            
        fcb = create_fcb();
        if (!fcb) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        fcb->Vcb = Vcb;
        
//         fcb->Header.IsFastIoPossible = TRUE;
        fcb->Header.AllocationSize.QuadPart = 0;
        fcb->Header.FileSize.QuadPart = 0;
        fcb->Header.ValidDataLength.QuadPart = 0;
        
        if (options & FILE_DELETE_ON_CLOSE)
            fcb->delete_on_close = TRUE;
        
        fcb->par = parfcb;
        parfcb->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
        WARN("fcb %p: refcount now %i (%.*S)\n", parfcb, parfcb->refcount, parfcb->full_filename.Length / sizeof(WCHAR), parfcb->full_filename.Buffer);
#endif
        fcb->subvol = parfcb->subvol;
        fcb->inode = parfcb->inode;
        fcb->type = parfcb->type;
        
        fcb->ads = TRUE;
        fcb->adssize = 0;
        
        Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, stream.Buffer, stream.Length);
        if (!NT_SUCCESS(Status))
            goto end;
        
        fcb->adsxattr.Length = utf8len + xapreflen;
        fcb->adsxattr.MaximumLength = fcb->adsxattr.Length + 1;
        fcb->adsxattr.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->adsxattr.MaximumLength, ALLOC_TAG);
        if (!fcb->adsxattr.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(fcb->adsxattr.Buffer, xapref, xapreflen);
        
        Status = RtlUnicodeToUTF8N(&fcb->adsxattr.Buffer[xapreflen], utf8len, &utf8len, stream.Buffer, stream.Length);
        if (!NT_SUCCESS(Status)) {
            free_fcb(fcb);
            goto end;
        }
        
        fcb->adsxattr.Buffer[fcb->adsxattr.Length] = 0;
        
        TRACE("adsxattr = %s\n", fcb->adsxattr.Buffer);
        
        fcb->adshash = calc_crc32c(0xfffffffe, (UINT8*)fcb->adsxattr.Buffer, fcb->adsxattr.Length);
        TRACE("adshash = %08x\n", fcb->adshash);

        fcb->name_offset = parfcb->full_filename.Length / sizeof(WCHAR);
        if (parfcb != Vcb->root_fcb)
            fcb->name_offset++;

        fcb->filepart.MaximumLength = fcb->filepart.Length = stream.Length;
        fcb->filepart.Buffer = ExAllocatePoolWithTag(PagedPool, fcb->filepart.MaximumLength, ALLOC_TAG);
        if (!fcb->filepart.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlCopyMemory(fcb->filepart.Buffer, stream.Buffer, stream.Length);
        
        fnlen = (fcb->name_offset * sizeof(WCHAR)) + fcb->filepart.Length;

        fcb->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fnlen, ALLOC_TAG);
        if (!fcb->full_filename.Buffer) {
            ERR("out of memory\n");
            free_fcb(fcb);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        fcb->full_filename.Length = fcb->full_filename.MaximumLength = fnlen;
        RtlCopyMemory(fcb->full_filename.Buffer, parfcb->full_filename.Buffer, parfcb->full_filename.Length);

        fcb->full_filename.Buffer[parfcb->full_filename.Length / sizeof(WCHAR)] = ':';

        RtlCopyMemory(&fcb->full_filename.Buffer[fcb->name_offset], fcb->filepart.Buffer, fcb->filepart.Length);
        TRACE("full_filename = %.*S\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
        
        InsertTailList(&fcb->par->children, &fcb->list_entry);
        
        Status = set_xattr(Vcb, parfcb->subvol, parfcb->inode, fcb->adsxattr.Buffer, fcb->adshash, (UINT8*)"", 0, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            free_fcb(fcb);
            goto end;
        }
        
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
        
        parfcb->inode_item.transid = Vcb->superblock.generation;
        parfcb->inode_item.sequence++;
        parfcb->inode_item.st_ctime = now;
        
        searchkey.obj_id = parfcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, parfcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            delete_tree_item(Vcb, &tp, rollback);
        } else {
            WARN("could not find INODE_ITEM for inode %llx in subvol %llx\n", searchkey.obj_id, parfcb->subvol->id);
        }
        
        free_traverse_ptr(&tp);
        
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
    
        RtlCopyMemory(ii, &parfcb->inode_item, sizeof(INODE_ITEM));
        
        insert_tree_item(Vcb, parfcb->subvol, parfcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
        
        parfcb->subvol->root_item.ctransid = Vcb->superblock.generation;
        parfcb->subvol->root_item.ctime = now;
        
        ExFreePool(fpus.Buffer);
        fpus.Buffer = NULL;
    } else {
        Status = file_create2(Irp, Vcb, &fpus, parfcb, options, &fcb, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("file_create2 returned %08x\n", Status);
            goto end;
        }
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    
    if (Vcb->fcbs)
        Vcb->fcbs->prev = fcb;
    
    fcb->next = Vcb->fcbs;
    Vcb->fcbs = fcb;
    
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    Status = attach_fcb_to_fileobject(Vcb, fcb, FileObject);
    
    ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
    if (!ccb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlZeroMemory(ccb, sizeof(*ccb));
    ccb->NodeType = BTRFS_NODE_TYPE_CCB;
    ccb->NodeSize = sizeof(ccb);
    ccb->disposition = disposition;
    ccb->options = options;
    ccb->query_dir_offset = 0;
    RtlInitUnicodeString(&ccb->query_string, NULL);
    ccb->has_wildcard = FALSE;
    ccb->specific_file = FALSE;
    
    InterlockedIncrement(&fcb->open_count);
    
    FileObject->FsContext2 = ccb;

    FileObject->SectionObjectPointer = &fcb->nonpaged->segment_object;
    
    TRACE("returning FCB %p with parent %p\n", fcb, parfcb);
    
    Status = consider_write(Vcb);
    
    if (NT_SUCCESS(Status)) {
        ULONG fnlen;

        fcb->name_offset = fcb->par->full_filename.Length / sizeof(WCHAR);
                
        if (fcb->par != Vcb->root_fcb)
            fcb->name_offset++;
        
        fnlen = (fcb->name_offset * sizeof(WCHAR)) + fcb->filepart.Length;
        
        fcb->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, fnlen, ALLOC_TAG);
        if (!fcb->full_filename.Buffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }   
        
        fcb->full_filename.Length = fcb->full_filename.MaximumLength = fnlen;
        RtlCopyMemory(fcb->full_filename.Buffer, fcb->par->full_filename.Buffer, fcb->par->full_filename.Length);
        
        if (fcb->par != Vcb->root_fcb)
            fcb->full_filename.Buffer[fcb->par->full_filename.Length / sizeof(WCHAR)] = '\\';
        
        RtlCopyMemory(&fcb->full_filename.Buffer[fcb->name_offset], fcb->filepart.Buffer, fcb->filepart.Length);
        
        FsRtlNotifyFullReportChange(Vcb->NotifySync, &Vcb->DirNotifyList, (PSTRING)&fcb->full_filename, fcb->name_offset * sizeof(WCHAR), NULL, NULL,
                                    options & FILE_DIRECTORY_FILE ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                    FILE_ACTION_ADDED, NULL);
        
        goto end2;
    }
    
end:    
    if (fpus.Buffer)
        ExFreePool(fpus.Buffer);
    
end2:
    if (parfcb)
        free_fcb(parfcb);
    
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

static NTSTATUS update_inode_item(device_extension* Vcb, root* subvol, UINT64 inode, INODE_ITEM* ii, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    INODE_ITEM* newii;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        delete_tree_item(Vcb, &tp, rollback);
    } else {
        WARN("could not find INODE_ITEM for inode %llx in subvol %llx\n", searchkey.obj_id, subvol->id);
    }
    
    free_traverse_ptr(&tp);
    
    newii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!newii) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(newii, ii, sizeof(INODE_ITEM));
    
    insert_tree_item(Vcb, subvol, inode, TYPE_INODE_ITEM, 0, newii, sizeof(INODE_ITEM), NULL, rollback);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL create_file(PDEVICE_OBJECT DeviceObject, PIRP Irp, LIST_ENTRY* rollback) {
    PFILE_OBJECT FileObject;
    ULONG RequestedDisposition;
    ULONG options;
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG access;
    PACCESS_STATE access_state = Stack->Parameters.Create.SecurityContext->AccessState;
    
    Irp->IoStatus.Information = 0;
    
    RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
    options = Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
    
    if (options & FILE_DIRECTORY_FILE && RequestedDisposition == FILE_SUPERSEDE) {
        WARN("error - supersede requested with FILE_DIRECTORY_FILE\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    FileObject = Stack->FileObject;

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
    
    TRACE("(%.*S)\n", FileObject->FileName.Length / sizeof(WCHAR), FileObject->FileName.Buffer);
    TRACE("FileObject = %p\n", FileObject);
    
    if (Vcb->readonly && (RequestedDisposition == FILE_SUPERSEDE || RequestedDisposition == FILE_CREATE || RequestedDisposition == FILE_OVERWRITE)) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto exit;
    }
    
    // FIXME - if Vcb->readonly or subvol readonly, don't allow the write ACCESS_MASK flags

    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    Status = get_fcb(Vcb, &fcb, &FileObject->FileName, FileObject->RelatedFileObject ? FileObject->RelatedFileObject->FsContext : NULL, Stack->Flags & SL_OPEN_TARGET_DIRECTORY);
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    if (NT_SUCCESS(Status) && fcb->deleted) {
        free_fcb(fcb);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    if (NT_SUCCESS(Status)) {
        if (RequestedDisposition == FILE_CREATE) {
            TRACE("file %.*S already exists, returning STATUS_OBJECT_NAME_COLLISION\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
            Status = STATUS_OBJECT_NAME_COLLISION;
            free_fcb(fcb);
            goto exit;
        }
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        if (RequestedDisposition == FILE_OPEN || RequestedDisposition == FILE_OVERWRITE) {
            TRACE("file doesn't exist, returning STATUS_OBJECT_NAME_NOT_FOUND\n");
            goto exit;
        }
    } else {
        TRACE("get_fcb returned %08x\n", Status);
        goto exit;
    }
    
    if (NT_SUCCESS(Status)) { // file already exists
        struct _fcb* sf;
        
        if (Vcb->readonly && RequestedDisposition == FILE_OVERWRITE_IF) {
            Status = STATUS_MEDIA_WRITE_PROTECTED;
            free_fcb(fcb);
            goto exit;
        }
        
        if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY && (RequestedDisposition == FILE_SUPERSEDE ||
            RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF)) {
            Status = STATUS_ACCESS_DENIED;
            free_fcb(fcb);
            goto exit;
        }
        
        TRACE("deleted = %s\n", fcb->deleted ? "TRUE" : "FALSE");
        
        sf = fcb;
        while (sf) {
            if (sf->delete_on_close) {
                WARN("could not open as deletion pending\n");
                Status = STATUS_DELETE_PENDING;
                free_fcb(fcb);
                goto exit;
            }
            sf = sf->par;
        }
        
        if (fcb->type == BTRFS_TYPE_SYMLINK && !(options & FILE_OPEN_REPARSE_POINT))  {
            if (!follow_symlink(fcb, FileObject)) {
                ERR("follow_symlink failed\n");
                Status = STATUS_INTERNAL_ERROR;
                free_fcb(fcb);
                goto exit;
            }
            
            Status = STATUS_REPARSE;
            Irp->IoStatus.Information = IO_REPARSE;
            free_fcb(fcb);
            goto exit;
        }
        
        if (!SeAccessCheck(fcb->sd, &access_state->SubjectSecurityContext, FALSE, access_state->OriginalDesiredAccess, 0, NULL,
            IoGetFileObjectGenericMapping(), Stack->Flags & SL_FORCE_ACCESS_CHECK ? UserMode : Irp->RequestorMode, &access, &Status)) {
            WARN("SeAccessCheck failed, returning %08x\n", Status);
            free_fcb(fcb);
            goto exit;
        }
        
        if (fcb->open_count > 0) {
            Status = IoCheckShareAccess(access, Stack->Parameters.Create.ShareAccess, FileObject, &fcb->share_access, TRUE);
            
            if (!NT_SUCCESS(Status)) {
                WARN("IoCheckShareAccess failed, returning %08x\n", Status);
                free_fcb(fcb);
                goto exit;
            }
        } else {
            IoSetShareAccess(access, Stack->Parameters.Create.ShareAccess, FileObject, &fcb->share_access);
        }

        if (access & FILE_WRITE_DATA || options & FILE_DELETE_ON_CLOSE) {
            if (!MmFlushImageSection(&fcb->nonpaged->segment_object, MmFlushForWrite)) {
                Status = (options & FILE_DELETE_ON_CLOSE) ? STATUS_CANNOT_DELETE : STATUS_SHARING_VIOLATION;
                free_fcb(fcb);
                goto exit;
            }
        }
        
        if (RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF || RequestedDisposition == FILE_SUPERSEDE) {
            ULONG defda;
            LIST_ENTRY changed_sector_list;
            
            if ((RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF) && fcb->atts & FILE_ATTRIBUTE_READONLY) {
                WARN("cannot overwrite readonly file\n");
                Status = STATUS_ACCESS_DENIED;
                free_fcb(fcb);
                goto exit;
            }
    
            // FIXME - where did we get this from?
//             if (fcb->refcount > 1) {
//                 WARN("cannot overwrite open file (fcb = %p, refcount = %u)\n", fcb, fcb->refcount);
//                 Status = STATUS_ACCESS_DENIED;
//                 free_fcb(fcb);
//                 goto exit;
//             }
            InitializeListHead(&changed_sector_list);
            
            // FIXME - make sure not ADS!
            Status = truncate_file(fcb, fcb->inode_item.st_size, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("truncate_file returned %08x\n", Status);
                free_fcb(fcb);
                goto exit;
            }
            
            Status = update_inode_item(Vcb, fcb->subvol, fcb->inode, &fcb->inode_item, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("update_inode_item returned %08x\n", Status);
                free_fcb(fcb);
                goto exit;
            }
            
            defda = get_file_attributes(Vcb, &fcb->inode_item, fcb->subvol, fcb->inode, fcb->type, fcb->filepart.Length > 0 && fcb->filepart.Buffer[0] == '.', TRUE);
            
            if (RequestedDisposition == FILE_SUPERSEDE)
                fcb->atts = Stack->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
            else
                fcb->atts |= Stack->Parameters.Create.FileAttributes | FILE_ATTRIBUTE_ARCHIVE;
            
            if (Stack->Parameters.Create.FileAttributes != defda) {
                char val[64];
            
                sprintf(val, "0x%x", Stack->Parameters.Create.FileAttributes);
            
                Status = set_xattr(Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (UINT8*)val, strlen(val), rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("set_xattr returned %08x\n", Status);
                    free_fcb(fcb);
                    goto exit;
                }
            } else
                delete_xattr(Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, rollback);
            
            // FIXME - truncate streams
            // FIXME - do we need to alter parent directory's times?
            // FIXME - send notifications
            
            Status = consider_write(fcb->Vcb);
            if (!NT_SUCCESS(Status)) {
                ERR("consider_write returned %08x\n", Status);
                free_fcb(fcb);
                goto exit;
            }
        }
    
//         fcb->Header.IsFastIoPossible = TRUE;
        
        if (fcb->ads) {
            fcb->Header.AllocationSize.QuadPart = fcb->adssize;
            fcb->Header.FileSize.QuadPart = fcb->adssize;
            fcb->Header.ValidDataLength.QuadPart = fcb->adssize;
        } else if (fcb->inode_item.st_size == 0 || (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK)) {
            fcb->Header.AllocationSize.QuadPart = 0;
            fcb->Header.FileSize.QuadPart = 0;
            fcb->Header.ValidDataLength.QuadPart = 0;
        } else {
            KEY searchkey;
            traverse_ptr tp;
            EXTENT_DATA* ed;
            
            searchkey.obj_id = fcb->inode;
            searchkey.obj_type = TYPE_EXTENT_DATA;
            searchkey.offset = 0xffffffffffffffff;
            
            Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                free_fcb(fcb);
                goto exit;
            }
            
            if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                ERR("error - could not find EXTENT_DATA items for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                free_traverse_ptr(&tp);
                free_fcb(fcb);
                Status = STATUS_INTERNAL_ERROR;
                goto exit;
            }
            
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %llx bytes, expected at least %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                    tp.item->size, sizeof(EXTENT_DATA));
                free_traverse_ptr(&tp);
                free_fcb(fcb);
                Status = STATUS_INTERNAL_ERROR;
                goto exit;
            }
            
            ed = (EXTENT_DATA*)tp.item->data;
            
            if (ed->type == EXTENT_TYPE_INLINE)
                fcb->Header.AllocationSize.QuadPart = fcb->inode_item.st_size;
            else
                fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
            
            fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
            fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
            
            free_traverse_ptr(&tp);
        }
    
        if (options & FILE_NON_DIRECTORY_FILE && fcb->type == BTRFS_TYPE_DIRECTORY) {
            free_fcb(fcb);
            Status = STATUS_FILE_IS_A_DIRECTORY;
            goto exit;
        } else if (options & FILE_DIRECTORY_FILE && fcb->type != BTRFS_TYPE_DIRECTORY) {
            TRACE("returning STATUS_NOT_A_DIRECTORY (type = %u, path = %.*S)\n", fcb->type, fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
            free_fcb(fcb);
            Status = STATUS_NOT_A_DIRECTORY;
            goto exit;
        }
    
        Status = attach_fcb_to_fileobject(Vcb, fcb, FileObject);
        
        if (options & FILE_DELETE_ON_CLOSE)
            fcb->delete_on_close = TRUE;
        
        ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ccb), ALLOC_TAG);
        if (!ccb) {
            ERR("out of memory\n");
            free_fcb(fcb);
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
        
        FileObject->FsContext2 = ccb;
        
        FileObject->SectionObjectPointer = &fcb->nonpaged->segment_object;
        
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
        
        InterlockedIncrement(&fcb->open_count);
    } else {
        Status = file_create(Irp, DeviceObject->DeviceExtension, FileObject, &FileObject->FileName, RequestedDisposition, options, rollback);
        Irp->IoStatus.Information = NT_SUCCESS(Status) ? FILE_CREATED : 0;
        
//         if (!NT_SUCCESS(Status))
//             free_fcb(fcb);
    }
    
    if (NT_SUCCESS(Status) && !(options & FILE_NO_INTERMEDIATE_BUFFERING))
        FileObject->Flags |= FO_CACHE_SUPPORTED;
    
exit:
    if (NT_SUCCESS(Status)) {
        if (!FileObject->Vpb)
            FileObject->Vpb = DeviceObject->Vpb;
    } else {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
            TRACE("returning %08x\n", Status);
    }
    
    return Status;
}

NTSTATUS STDCALL drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    device_extension* Vcb = NULL;
    BOOL top_level;
    LIST_ENTRY rollback;
    
    TRACE("create (flags = %x)\n", Irp->Flags);
    
    InitializeListHead(&rollback);
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    /* return success if just called for FS device object */
    if (DeviceObject == devobj)  {
        TRACE("create called for FS device object\n");
        
        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;

        goto exit;
    }
    
    Vcb = DeviceObject->DeviceExtension;
    ExAcquireResourceSharedLite(&Vcb->load_lock, TRUE);
    
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
        
        TRACE("open operation for volume\n");

        if (RequestedDisposition != FILE_OPEN &&
            RequestedDisposition != FILE_OPEN_IF)
        {
            Status = STATUS_ACCESS_DENIED;
            goto exit;
        }

        if (RequestedOptions & FILE_DIRECTORY_FILE)
        {
            Status = STATUS_NOT_A_DIRECTORY;
            goto exit;
        }

        Vcb->volume_fcb->refcount++;
#ifdef DEBUG_FCB_REFCOUNTS
        WARN("fcb %p: refcount now %i (volume)\n", Vcb->volume_fcb, Vcb->volume_fcb->refcount);
#endif
        attach_fcb_to_fileobject(Vcb, Vcb->volume_fcb, IrpSp->FileObject);
// //         NtfsAttachFCBToFileObject(DeviceExt, DeviceExt->VolumeFcb, FileObject);
// //         DeviceExt->VolumeFcb->RefCount++;
// 
        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
    } else {
        BOOL exclusive;
        ULONG disposition;
        
        TRACE("file name: %.*S\n", IrpSp->FileObject->FileName.Length / sizeof(WCHAR), IrpSp->FileObject->FileName.Buffer);
        
        if (IrpSp->FileObject->RelatedFileObject) {
            fcb* relfcb = IrpSp->FileObject->RelatedFileObject->FsContext;
            
            if (relfcb)
                TRACE("related file name = %.*S\n", relfcb->full_filename.Length / sizeof(WCHAR), relfcb->full_filename.Buffer);
        }
        
        disposition = ((IrpSp->Parameters.Create.Options >> 24) & 0xff);
        
        // We acquire the lock exclusively if there's the possibility we might be writing
        exclusive = disposition != FILE_OPEN;

        acquire_tree_lock(Vcb, exclusive); 
        
//         ExAcquireResourceExclusiveLite(&Vpb->DirResource, TRUE);
    //     Status = NtfsCreateFile(DeviceObject,
    //                             Irp);
        Status = create_file(DeviceObject, Irp, &rollback);
//         ExReleaseResourceLite(&Vpb->DirResource);
        
        if (exclusive && !NT_SUCCESS(Status))
            do_rollback(Vcb, &rollback);
        else
            clear_rollback(&rollback);
        
        release_tree_lock(Vcb, exclusive);
        
//         Status = STATUS_ACCESS_DENIED;
    }
    
exit:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT );
//     IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    
    TRACE("create returning %08x\n", Status);
    
    ExReleaseResourceLite(&Vcb->load_lock);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
