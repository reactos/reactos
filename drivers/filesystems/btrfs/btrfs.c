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

#ifdef _DEBUG
#define DEBUG
#endif

#include "btrfs_drv.h"
#ifndef __REACTOS__
#ifndef _MSC_VER
#include <cpuid.h>
#else
#include <intrin.h>
#endif
#endif
#include "btrfs.h"
#ifndef __REACTOS__
#include <winioctl.h>
#else
#include <rtlfuncs.h>
#endif

#define INCOMPAT_SUPPORTED (BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF | BTRFS_INCOMPAT_FLAGS_DEFAULT_SUBVOL | BTRFS_INCOMPAT_FLAGS_BIG_METADATA | \
                            BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF | BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)
#define COMPAT_RO_SUPPORTED 0

static WCHAR device_name[] = {'\\','B','t','r','f','s',0};
static WCHAR dosdevice_name[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','B','t','r','f','s',0};

PDRIVER_OBJECT drvobj;
PDEVICE_OBJECT devobj;
#ifndef __REACTOS__
BOOL have_sse42 = FALSE;
#endif
UINT64 num_reads = 0;
LIST_ENTRY uid_map_list;
LIST_ENTRY volumes;
LIST_ENTRY VcbList;
ERESOURCE global_loading_lock;
UINT32 debug_log_level = 0;
BOOL log_started = FALSE;
UNICODE_STRING log_device, log_file;

#ifdef _DEBUG
PFILE_OBJECT comfo = NULL;
PDEVICE_OBJECT comdo = NULL;
HANDLE log_handle = NULL;
#endif

int __security_cookie = __LINE__;

static NTSTATUS STDCALL close_file(device_extension* Vcb, PFILE_OBJECT FileObject);

typedef struct {
    KEVENT Event;
    IO_STATUS_BLOCK iosb;
} read_context;

#ifdef _DEBUG
static NTSTATUS STDCALL dbg_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_context* context = conptr;
    
//     DbgPrint("dbg_completion\n");
    
    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);
    
//     return STATUS_SUCCESS;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifdef DEBUG_LONG_MESSAGES
void STDCALL _debug_message(const char* func, const char* file, unsigned int line, char* s, ...) {
#else
void STDCALL _debug_message(const char* func, char* s, ...) {
#endif
    LARGE_INTEGER offset;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PIRP Irp;
    va_list ap;
    char *buf2 = NULL, *buf;
    read_context* context = NULL;
    UINT32 length;
    
    buf2 = ExAllocatePoolWithTag(NonPagedPool, 1024, ALLOC_TAG);
    
    if (!buf2) {
        DbgPrint("Couldn't allocate buffer in debug_message\n");
        return;
    }
    
#ifdef DEBUG_LONG_MESSAGES
    sprintf(buf2, "%p:%s:%s:%u:", PsGetCurrentThreadId(), func, file, line);
#else
    sprintf(buf2, "%p:%s:", PsGetCurrentThreadId(), func);
#endif
    buf = &buf2[strlen(buf2)];
    
    va_start(ap, s);
    vsprintf(buf, s, ap);
    
    if (!log_started || (log_device.Length == 0 && log_file.Length == 0)) {
        DbgPrint(buf2);
    } else if (log_device.Length > 0) {
        if (!comdo) {
            DbgPrint("comdo is NULL :-(\n");
            DbgPrint(buf2);
            goto exit2;
        }
        
        length = (UINT32)strlen(buf2);
        
        offset.u.LowPart = 0;
        offset.u.HighPart = 0;
        
        context = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_context), ALLOC_TAG);
        if (!context) {
            DbgPrint("Couldn't allocate context in debug_message\n");
            return;
        }
        
        RtlZeroMemory(context, sizeof(read_context));
        
        KeInitializeEvent(&context->Event, NotificationEvent, FALSE);

    //     status = ZwWriteFile(comh, NULL, NULL, NULL, &io, buf2, strlen(buf2), &offset, NULL);
        
        Irp = IoAllocateIrp(comdo->StackSize, FALSE);
        
        if (!Irp) {
            DbgPrint("IoAllocateIrp failed\n");
            goto exit2;
        }
        
        IrpSp = IoGetNextIrpStackLocation(Irp);
        IrpSp->MajorFunction = IRP_MJ_WRITE;
        
        if (comdo->Flags & DO_BUFFERED_IO) {
            Irp->AssociatedIrp.SystemBuffer = buf2;

            Irp->Flags = IRP_BUFFERED_IO;
        } else if (comdo->Flags & DO_DIRECT_IO) {
            Irp->MdlAddress = IoAllocateMdl(buf2, length, FALSE, FALSE, NULL);
            if (!Irp->MdlAddress) {
                DbgPrint("IoAllocateMdl failed\n");
                goto exit;
            }
            
            MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoWriteAccess);
        } else {
            Irp->UserBuffer = buf2;
        }

        IrpSp->Parameters.Write.Length = length;
        IrpSp->Parameters.Write.ByteOffset = offset;
        
        Irp->UserIosb = &context->iosb;

        Irp->UserEvent = &context->Event;

        IoSetCompletionRoutine(Irp, dbg_completion, context, TRUE, TRUE, TRUE);

        Status = IoCallDriver(comdo, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
            Status = context->iosb.Status;
        }
        
        if (comdo->Flags & DO_DIRECT_IO) {
            MmUnlockPages(Irp->MdlAddress);
            IoFreeMdl(Irp->MdlAddress);
        }
        
        if (!NT_SUCCESS(Status)) {
            DbgPrint("failed to write to COM1 - error %08x\n", Status);
            goto exit;
        }
        
exit:
        IoFreeIrp(Irp);
    } else if (log_handle != NULL) {
        IO_STATUS_BLOCK iosb;
        
        length = (UINT32)strlen(buf2);
        
        Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, buf2, length, NULL, NULL);
        
        if (!NT_SUCCESS(Status)) {
            DbgPrint("failed to write to file - error %08x\n", Status);
        }
    }
    
exit2:
    va_end(ap);
    
    if (context)
        ExFreePool(context);
    
    if (buf2)
        ExFreePool(buf2);
}
#endif

ULONG sector_align( ULONG NumberToBeAligned, ULONG Alignment )
{
    if( Alignment & ( Alignment - 1 ) )
    {
        //
        //  Alignment not a power of 2
        //  Just returning
        //
        return NumberToBeAligned;
    }
    if( ( NumberToBeAligned & ( Alignment - 1 ) ) != 0 )
    {
        NumberToBeAligned = NumberToBeAligned + Alignment;
        NumberToBeAligned = NumberToBeAligned & ( ~ (Alignment-1) );
    }
    return NumberToBeAligned;
}

int keycmp(const KEY* key1, const KEY* key2) {
    if (key1->obj_id < key2->obj_id) {
        return -1;
    } else if (key1->obj_id > key2->obj_id) {
        return 1;
    }
    
    if (key1->obj_type < key2->obj_type) {
        return -1;
    } else if (key1->obj_type > key2->obj_type) {
        return 1;
    }
    
    if (key1->offset < key2->offset) {
        return -1;
    } else if (key1->offset > key2->offset) {
        return 1;
    }
    
    return 0;
}

BOOL is_top_level(PIRP Irp) {
    if (!IoGetTopLevelIrp()) {
        IoSetTopLevelIrp(Irp);
        return TRUE;
    }

    return FALSE;
}

static void STDCALL DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING dosdevice_nameW;

    ERR("DriverUnload\n");
    
    free_cache();
    
    IoUnregisterFileSystem(DriverObject->DeviceObject);
   
    dosdevice_nameW.Buffer = dosdevice_name;
    dosdevice_nameW.Length = dosdevice_nameW.MaximumLength = (USHORT)wcslen(dosdevice_name) * sizeof(WCHAR);

    IoDeleteSymbolicLink(&dosdevice_nameW);
    IoDeleteDevice(DriverObject->DeviceObject);
    
    while (!IsListEmpty(&uid_map_list)) {
        LIST_ENTRY* le = RemoveHeadList(&uid_map_list);
        uid_map* um = CONTAINING_RECORD(le, uid_map, listentry);
        
        ExFreePool(um->sid);

        ExFreePool(um);
    }
    
    // FIXME - free volumes and their devpaths
    
#ifdef _DEBUG
    if (comfo)
        ObDereferenceObject(comfo);
    
    if (log_handle)
        ZwClose(log_handle);
#endif
    
    ExDeleteResourceLite(&global_loading_lock);
    
    if (log_device.Buffer)
        ExFreePool(log_device.Buffer);
    
    if (log_file.Buffer)
        ExFreePool(log_file.Buffer);
}

BOOL STDCALL get_last_inode(device_extension* Vcb, root* r) {
    KEY searchkey;
    traverse_ptr tp, prev_tp;
    NTSTATUS Status;
    
    // get last entry
    searchkey.obj_id = 0xffffffffffffffff;
    searchkey.obj_type = 0xff;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, r, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    while (find_prev_item(Vcb, &tp, &prev_tp, FALSE)) {
        tp = prev_tp;
        
        TRACE("moving on to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        if (tp.item->key.obj_type == TYPE_INODE_ITEM || (tp.item->key.obj_type == TYPE_ROOT_ITEM && !(tp.item->key.obj_id & 0x8000000000000000))) {
            r->lastinode = tp.item->key.obj_id;
            TRACE("last inode for tree %llx is %llx\n", r->id, r->lastinode);
            return TRUE;
        }
    }
    
    r->lastinode = SUBVOL_ROOT_INODE;
    
    WARN("no INODE_ITEMs in tree %llx\n", r->id);
    
    return TRUE;
}

BOOL STDCALL get_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, UINT8** data, UINT16* datalen) {
    KEY searchkey;
    traverse_ptr tp;
    DIR_ITEM* xa;
    ULONG size, xasize;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %s, %08x, %p, %p)\n", Vcb, subvol->id, inode, name, crc32, data, datalen);
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        TRACE("could not find item (%llx,%x,%llx)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return FALSE;
    }
    
    if (tp.item->size < sizeof(DIR_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        return FALSE;
    }
    
    xa = (DIR_ITEM*)tp.item->data;
    size = tp.item->size;
    
    while (TRUE) {
        if (size < sizeof(DIR_ITEM) || size < (sizeof(DIR_ITEM) - 1 + xa->m + xa->n)) {
            WARN("(%llx,%x,%llx) is truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            return FALSE;
        }
        
        if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
            TRACE("found xattr %s in (%llx,%x,%llx)\n", name, searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            
            *datalen = xa->m;
            
            if (xa->m > 0) {
                *data = ExAllocatePoolWithTag(PagedPool, xa->m, ALLOC_TAG);
                if (!*data) {
                    ERR("out of memory\n");
                    return FALSE;
                }
                
                RtlCopyMemory(*data, &xa->name[xa->n], xa->m);
            } else
                *data = NULL;
            
            return TRUE;
        }
        
        xasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;

        if (size > xasize) {
            size -= xasize;
            xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
        } else
            break;
    }
    
    TRACE("xattr %s not found in (%llx,%x,%llx)\n", name, searchkey.obj_id, searchkey.obj_type, searchkey.offset);
    
    return FALSE;
}

NTSTATUS STDCALL set_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, UINT8* data, UINT16 datalen, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    ULONG xasize;
    DIR_ITEM* xa;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %s, %08x, %p, %u)\n", Vcb, subvol->id, inode, name, crc32, data, datalen);
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    xasize = sizeof(DIR_ITEM) - 1 + (ULONG)strlen(name) + datalen;
    
    // FIXME - make sure xasize not too big
    
    if (!keycmp(&tp.item->key, &searchkey)) { // key exists
        UINT8* newdata;
        ULONG size = tp.item->size;
        
        xa = (DIR_ITEM*)tp.item->data;
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        } else {
            while (TRUE) {
                ULONG oldxasize;
                
                if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                
                if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
                    UINT64 pos;
                    
                    // replace
                    newdata = ExAllocatePoolWithTag(PagedPool, tp.item->size + xasize - oldxasize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    pos = (UINT8*)xa - tp.item->data;
                    if (pos + oldxasize < tp.item->size) { // copy after changed xattr
                        RtlCopyMemory(newdata + pos + xasize, tp.item->data + pos + oldxasize, tp.item->size - pos - oldxasize);
                    }
                    
                    if (pos > 0) { // copy before changed xattr
                        RtlCopyMemory(newdata, tp.item->data, pos);
                        xa = (DIR_ITEM*)(newdata + pos);
                    } else
                        xa = (DIR_ITEM*)newdata;
                    
                    xa->key.obj_id = 0;
                    xa->key.obj_type = 0;
                    xa->key.offset = 0;
                    xa->transid = Vcb->superblock.generation;
                    xa->m = datalen;
                    xa->n = (UINT16)strlen(name);
                    xa->type = BTRFS_TYPE_EA;
                    RtlCopyMemory(xa->name, name, strlen(name));
                    RtlCopyMemory(xa->name + strlen(name), data, datalen);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, tp.item->size + xasize - oldxasize, NULL, rollback);
                    
                    break;
                }
                
                if (xa->m + xa->n >= size) { // FIXME - test this works
                    // not found, add to end of data
                    newdata = ExAllocatePoolWithTag(PagedPool, tp.item->size + xasize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newdata, tp.item->data, tp.item->size);
                    
                    xa = (DIR_ITEM*)((UINT8*)newdata + tp.item->size);
                    xa->key.obj_id = 0;
                    xa->key.obj_type = 0;
                    xa->key.offset = 0;
                    xa->transid = Vcb->superblock.generation;
                    xa->m = datalen;
                    xa->n = (UINT16)strlen(name);
                    xa->type = BTRFS_TYPE_EA;
                    RtlCopyMemory(xa->name, name, strlen(name));
                    RtlCopyMemory(xa->name + strlen(name), data, datalen);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, tp.item->size + xasize, NULL, rollback);
                    
                    break;
                } else {
                    xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
                    size -= oldxasize;
                }
            }
        }
    } else {
        // add new DIR_ITEM struct
        
        xa = ExAllocatePoolWithTag(PagedPool, xasize, ALLOC_TAG);
        if (!xa) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        xa->key.obj_id = 0;
        xa->key.obj_type = 0;
        xa->key.offset = 0;
        xa->transid = Vcb->superblock.generation;
        xa->m = datalen;
        xa->n = (UINT16)strlen(name);
        xa->type = BTRFS_TYPE_EA;
        RtlCopyMemory(xa->name, name, strlen(name));
        RtlCopyMemory(xa->name + strlen(name), data, datalen);
        
        insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, xa, xasize, NULL, rollback);
    }
    
    return STATUS_SUCCESS;
}

BOOL STDCALL delete_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    DIR_ITEM* xa;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %s, %08x)\n", Vcb, subvol->id, inode, name, crc32);
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (!keycmp(&tp.item->key, &searchkey)) { // key exists
        ULONG size = tp.item->size;
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            
            return FALSE;
        } else {
            xa = (DIR_ITEM*)tp.item->data;
            
            while (TRUE) {
                ULONG oldxasize;
                
                if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        
                    return FALSE;
                }
                
                oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                
                if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
                    ULONG newsize;
                    UINT8 *newdata, *dioff;
                    
                    newsize = tp.item->size - (sizeof(DIR_ITEM) - 1 + xa->n + xa->m);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (newsize == 0) {
                        TRACE("xattr %s deleted\n", name);
                        
                        return TRUE;
                    }

                    // FIXME - deleting collisions almost certainly works, but we should test it properly anyway
                    newdata = ExAllocatePoolWithTag(PagedPool, newsize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return FALSE;
                    }

                    if ((UINT8*)xa > tp.item->data) {
                        RtlCopyMemory(newdata, tp.item->data, (UINT8*)xa - tp.item->data);
                        dioff = newdata + ((UINT8*)xa - tp.item->data);
                    } else {
                        dioff = newdata;
                    }
                    
                    if ((UINT8*)&xa->name[xa->n+xa->m] - tp.item->data < tp.item->size)
                        RtlCopyMemory(dioff, &xa->name[xa->n+xa->m], tp.item->size - ((UINT8*)&xa->name[xa->n+xa->m] - tp.item->data));
                    
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, newsize, NULL, rollback);
                    
                        
                    return TRUE;
                }
                
                if (xa->m + xa->n >= size) { // FIXME - test this works
                    WARN("xattr %s not found\n", name);

                    return FALSE;
                } else {
                    xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
                    size -= oldxasize;
                }
            }
        }
    } else {
        WARN("xattr %s not found\n", name);
        
        return FALSE;
    }
}

NTSTATUS add_dir_item(device_extension* Vcb, root* subvol, UINT64 inode, UINT32 crc32, DIR_ITEM* di, ULONG disize, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    UINT8* di2;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_DIR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey)) {
        ULONG maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
        
        if (tp.item->size + disize > maxlen) {
            WARN("DIR_ITEM was longer than maxlen (%u + %u > %u)\n", tp.item->size, disize, maxlen);
            return STATUS_INTERNAL_ERROR;
        }
        
        di2 = ExAllocatePoolWithTag(PagedPool, tp.item->size + disize, ALLOC_TAG);
        if (!di2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(di2, tp.item->data, tp.item->size);
        
        RtlCopyMemory(di2 + tp.item->size, di, disize);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        insert_tree_item(Vcb, subvol, inode, TYPE_DIR_ITEM, crc32, di2, tp.item->size + disize, NULL, rollback);
        
        ExFreePool(di);
    } else {
        insert_tree_item(Vcb, subvol, inode, TYPE_DIR_ITEM, crc32, di, disize, NULL, rollback);
    }
    
    return STATUS_SUCCESS;
}

UINT64 find_next_dir_index(device_extension* Vcb, root* subvol, UINT64 inode) {
    KEY searchkey;
    traverse_ptr tp, prev_tp;
    UINT64 dirpos;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_DIR_INDEX + 1;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (!find_prev_item(Vcb, &tp, &prev_tp, FALSE)) {
            tp = prev_tp;
            
            TRACE("moving back to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == TYPE_DIR_INDEX) {
        dirpos = tp.item->key.offset + 1;
    } else
        dirpos = 2;
    
    return dirpos;
}

static NTSTATUS STDCALL drv_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    BOOL top_level;

    TRACE("close\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (DeviceObject == devobj) {
        TRACE("Closing file system\n");
        Status = STATUS_SUCCESS;
        goto exit;
    }
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    // FIXME - unmount if called for volume
    // FIXME - call FsRtlNotifyUninitializeSync(&Vcb->NotifySync) if unmounting
    
    Status = close_file(DeviceObject->DeviceExtension, IrpSp->FileObject);

exit:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    TRACE("returning %08x\n", Status);

    return Status;
}

static NTSTATUS STDCALL drv_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    BOOL top_level;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
//     ERR("recursive = %s\n", Irp != IoGetTopLevelIrp() ? "TRUE" : "FALSE");
    
    _SEH2_TRY {
        if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
            CcMdlWriteComplete(IrpSp->FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress);
            
            Irp->MdlAddress = NULL;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            Status = write_file(DeviceObject, Irp);
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    
    Irp->IoStatus.Status = Status;

    TRACE("wrote %u bytes\n", Irp->IoStatus.Information);
    
    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    TRACE("returning %08x\n", Status);

    return Status;
}

static NTSTATUS STDCALL drv_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    FIXME("STUB: query ea\n");
    Status = STATUS_NOT_IMPLEMENTED;
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL drv_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    FIXME("STUB: set ea\n");
    Status = STATUS_NOT_IMPLEMENTED;
    
    if (Vcb->readonly)
        Status = STATUS_MEDIA_WRITE_PROTECTED;
    
    // FIXME - return STATUS_ACCESS_DENIED if subvol readonly
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL drv_flush_buffers(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    BOOL top_level;

    TRACE("flush buffers\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    Status = STATUS_SUCCESS;
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    
    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        CcFlushCache(&fcb->nonpaged->segment_object, NULL, 0, &Irp->IoStatus);
        
        if (fcb->Header.PagingIoResource) {
            ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, TRUE);
            ExReleaseResourceLite(fcb->Header.PagingIoResource);
        }
        
        Status = Irp->IoStatus.Status;
    }
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL drv_query_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG BytesCopied = 0;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level;
    
#ifndef __REACTOS__
    // An unfortunate necessity - we have to lie about our FS type. MPR!MprGetConnection polls for this,
    // and compares it to a whitelist. If it doesn't match, it will return ERROR_NO_NET_OR_BAD_PATH,
    // which prevents UAC from working.
    // FIXME - only lie if we detect that we're being called by mpr.dll
    
    WCHAR* fs_name = L"NTFS";
    ULONG fs_name_len = 4 * sizeof(WCHAR);
#else
    WCHAR* fs_name = L"Btrfs";
    ULONG fs_name_len = 5 * sizeof(WCHAR);
#endif

    TRACE("query volume information\n");
    
    FsRtlEnterFileSystem();
    top_level = is_top_level(Irp);
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    Status = STATUS_NOT_IMPLEMENTED;
    
    switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {
        case FileFsAttributeInformation:
        {
            FILE_FS_ATTRIBUTE_INFORMATION* data = Irp->AssociatedIrp.SystemBuffer;
            BOOL overflow = FALSE;
            ULONG orig_fs_name_len = fs_name_len;
            
            TRACE("FileFsAttributeInformation\n");
            
            if (IrpSp->Parameters.QueryVolume.Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION) - sizeof(WCHAR) + fs_name_len) {
                if (IrpSp->Parameters.QueryVolume.Length > sizeof(FILE_FS_ATTRIBUTE_INFORMATION) - sizeof(WCHAR))
                    fs_name_len = IrpSp->Parameters.QueryVolume.Length - sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + sizeof(WCHAR);
                else
                    fs_name_len = 0;
                
                overflow = TRUE;
            }
            
            data->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH |
                                         FILE_UNICODE_ON_DISK | FILE_NAMED_STREAMS | FILE_SUPPORTS_HARD_LINKS | FILE_PERSISTENT_ACLS |
                                         FILE_SUPPORTS_REPARSE_POINTS;
            if (Vcb->readonly)
                data->FileSystemAttributes |= FILE_READ_ONLY_VOLUME;
                                         
            // should also be FILE_FILE_COMPRESSION when supported
            data->MaximumComponentNameLength = 255; // FIXME - check
            data->FileSystemNameLength = orig_fs_name_len;
            RtlCopyMemory(data->FileSystemName, fs_name, fs_name_len);
            
            BytesCopied = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) - sizeof(WCHAR) + fs_name_len;
            Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
            break;
        }

        case FileFsControlInformation:
            FIXME("STUB: FileFsControlInformation\n");
            break;

        case FileFsDeviceInformation:
            FIXME("STUB: FileFsDeviceInformation\n");
            break;

        case FileFsDriverPathInformation:
            FIXME("STUB: FileFsDriverPathInformation\n");
            break;

        case FileFsFullSizeInformation:
        {
            FILE_FS_FULL_SIZE_INFORMATION* ffsi = Irp->AssociatedIrp.SystemBuffer;
            UINT64 totalsize, freespace;
            
            TRACE("FileFsFullSizeInformation\n");
            
            // FIXME - calculate correctly for RAID
            totalsize = Vcb->superblock.total_bytes / Vcb->superblock.sector_size;
            freespace = (Vcb->superblock.total_bytes - Vcb->superblock.bytes_used) / Vcb->superblock.sector_size;
            
            ffsi->TotalAllocationUnits.QuadPart = totalsize;
            ffsi->ActualAvailableAllocationUnits.QuadPart = freespace;
            ffsi->CallerAvailableAllocationUnits.QuadPart = ffsi->ActualAvailableAllocationUnits.QuadPart;
            ffsi->SectorsPerAllocationUnit = 1;
            ffsi->BytesPerSector = Vcb->superblock.sector_size;
            
            BytesCopied = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;
            
            break;
        }

        case FileFsObjectIdInformation:
            FIXME("STUB: FileFsObjectIdInformation\n");
            break;

        case FileFsSizeInformation:
        {
            FILE_FS_SIZE_INFORMATION* ffsi = Irp->AssociatedIrp.SystemBuffer;
            UINT64 totalsize, freespace;
            
            TRACE("FileFsSizeInformation\n");
            
            // FIXME - calculate correctly for RAID
            // FIXME - is this returning the right free space?
            totalsize = Vcb->superblock.dev_item.num_bytes / Vcb->superblock.sector_size;
            freespace = (Vcb->superblock.dev_item.num_bytes - Vcb->superblock.dev_item.bytes_used) / Vcb->superblock.sector_size;
            
            ffsi->TotalAllocationUnits.QuadPart = totalsize;
            ffsi->AvailableAllocationUnits.QuadPart = freespace;
            ffsi->SectorsPerAllocationUnit = 1;
            ffsi->BytesPerSector = Vcb->superblock.sector_size;
            
            BytesCopied = sizeof(FILE_FS_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;
            
            break;
        }

        case FileFsVolumeInformation:
        {
            FILE_FS_VOLUME_INFORMATION* data = Irp->AssociatedIrp.SystemBuffer;
            FILE_FS_VOLUME_INFORMATION ffvi;
            BOOL overflow = FALSE;
            ULONG label_len, orig_label_len;
            
            TRACE("FileFsVolumeInformation\n");
            TRACE("max length = %u\n", IrpSp->Parameters.QueryVolume.Length);
            
            acquire_tree_lock(Vcb, FALSE);
            
//             orig_label_len = label_len = (ULONG)(wcslen(Vcb->label) * sizeof(WCHAR));
            RtlUTF8ToUnicodeN(NULL, 0, &label_len, Vcb->superblock.label, (ULONG)strlen(Vcb->superblock.label));
            orig_label_len = label_len;
            
            if (IrpSp->Parameters.QueryVolume.Length < sizeof(FILE_FS_VOLUME_INFORMATION) - sizeof(WCHAR) + label_len) {
                if (IrpSp->Parameters.QueryVolume.Length > sizeof(FILE_FS_VOLUME_INFORMATION) - sizeof(WCHAR))
                    label_len = IrpSp->Parameters.QueryVolume.Length - sizeof(FILE_FS_VOLUME_INFORMATION) + sizeof(WCHAR);
                else
                    label_len = 0;
                
                overflow = TRUE;
            }
            
            TRACE("label_len = %u\n", label_len);
            
            ffvi.VolumeCreationTime.QuadPart = 0; // FIXME
            ffvi.VolumeSerialNumber = Vcb->superblock.uuid.uuid[12] << 24 | Vcb->superblock.uuid.uuid[13] << 16 | Vcb->superblock.uuid.uuid[14] << 8 | Vcb->superblock.uuid.uuid[15];
            ffvi.VolumeLabelLength = orig_label_len;
            ffvi.SupportsObjects = FALSE;
            
            RtlCopyMemory(data, &ffvi, min(sizeof(FILE_FS_VOLUME_INFORMATION) - sizeof(WCHAR), IrpSp->Parameters.QueryVolume.Length));
            
            if (label_len > 0) {
                ULONG bytecount;
                
//                 RtlCopyMemory(&data->VolumeLabel[0], Vcb->label, label_len);
                RtlUTF8ToUnicodeN(&data->VolumeLabel[0], label_len, &bytecount, Vcb->superblock.label, (ULONG)strlen(Vcb->superblock.label));
                TRACE("label = %.*S\n", label_len / sizeof(WCHAR), data->VolumeLabel);
            }
            
            release_tree_lock(Vcb, FALSE);

            BytesCopied = sizeof(FILE_FS_VOLUME_INFORMATION) - sizeof(WCHAR) + label_len;
            Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
            break;
        }

        default:
            Status = STATUS_INVALID_PARAMETER;
            WARN("unknown FsInformatClass %u\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
            break;
    }
    
//     if (NT_SUCCESS(Status) && IrpSp->Parameters.QueryVolume.Length < BytesCopied) { // FIXME - should not copy anything if overflow
//         WARN("overflow: %u < %u\n", IrpSp->Parameters.QueryVolume.Length, BytesCopied);
//         BytesCopied = IrpSp->Parameters.QueryVolume.Length;
//         Status = STATUS_BUFFER_OVERFLOW;
//     }

    Irp->IoStatus.Status = Status;
    
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        Irp->IoStatus.Information = 0;
    else
        Irp->IoStatus.Information = BytesCopied;
    
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    TRACE("query volume information returning %08x\n", Status);

    return Status;
}

static NTSTATUS STDCALL read_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_context* context = conptr;
    
//     DbgPrint("read_completion\n");
    
    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);
    
//     return STATUS_SUCCESS;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// static void test_tree_deletion(device_extension* Vcb) {
//     KEY searchkey/*, endkey*/;
//     traverse_ptr tp, next_tp;
//     root* r;
//     
//     searchkey.obj_id = 0x100;
//     searchkey.obj_type = 0x54;
//     searchkey.offset = 0xca4ab2f5;
//     
// //     endkey.obj_id = 0x100;
// //     endkey.obj_type = 0x60;
// //     endkey.offset = 0x15a;
//     
//     r = Vcb->roots;
//     while (r && r->id != 0x102)
//         r = r->next;
//     
//     if (!r) {
//         ERR("error - could not find root\n");
//         return;
//     }
//     
//     if (!find_item(Vcb, r, &tp, &searchkey, NULL, FALSE)) {
//         ERR("error - could not find key\n");
//         return;
//     }
//     
//     while (TRUE/*keycmp(&tp.item->key, &endkey) < 1*/) {
//         tp.item->ignore = TRUE;
//         add_to_tree_cache(tc, tp.tree);
//         
//         if (find_next_item(Vcb, &tp, &next_tp, NULL, FALSE)) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//         } else
//             break;
//     }
//     
//     free_traverse_ptr(&tp);
// }

// static void test_tree_splitting(device_extension* Vcb) {
//     int i;
//     
//     for (i = 0; i < 1000; i++) {
//         char* data = ExAllocatePoolWithTag(PagedPool, 4, ALLOC_TAG);
//         
//         insert_tree_item(Vcb, Vcb->extent_root, 0, 0xfd, i, data, 4, NULL);
//     }
// }

// static void test_dropping_tree(device_extension* Vcb) {
//     LIST_ENTRY* le = Vcb->roots.Flink;
//     
//     while (le != &Vcb->roots) {
//         root* r = CONTAINING_RECORD(le, root, list_entry);
//         
//         if (r->id == 0x101) {
//             RemoveEntryList(&r->list_entry);
//             InsertTailList(&Vcb->drop_roots, &r->list_entry);
//             return;
//         }
//         
//         le = le->Flink;
//     }
// }

NTSTATUS create_root(device_extension* Vcb, UINT64 id, root** rootptr, BOOL no_tree, UINT64 offset, LIST_ENTRY* rollback) {
    root* r;
    tree* t;
    ROOT_ITEM* ri;
    traverse_ptr tp;
    
    r = ExAllocatePoolWithTag(PagedPool, sizeof(root), ALLOC_TAG);
    if (!r) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    r->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(root_nonpaged), ALLOC_TAG);
    if (!r->nonpaged) {
        ERR("out of memory\n");
        ExFreePool(r);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (!no_tree) {
        t = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
        if (!t) {
            ERR("out of memory\n");
            ExFreePool(r->nonpaged);
            ExFreePool(r);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
    if (!ri) {
        ERR("out of memory\n");
        
        if (!no_tree)
            ExFreePool(t);
        
        ExFreePool(r->nonpaged);
        ExFreePool(r);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    r->id = id;
    r->treeholder.address = 0;
    r->treeholder.generation = Vcb->superblock.generation;
    r->treeholder.tree = no_tree ? NULL : t;
    r->lastinode = 0;
    r->path.Buffer = NULL;
    RtlZeroMemory(&r->root_item, sizeof(ROOT_ITEM));
    r->root_item.num_references = 1;
    InitializeListHead(&r->fcbs);
    
    RtlCopyMemory(ri, &r->root_item, sizeof(ROOT_ITEM));
    
    // We ask here for a traverse_ptr to the item we're inserting, so we can
    // copy some of the tree's variables
    
    if (!insert_tree_item(Vcb, Vcb->root_root, id, TYPE_ROOT_ITEM, offset, ri, sizeof(ROOT_ITEM), &tp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ri);
        
        if (!no_tree)
            ExFreePool(t);
        
        ExFreePool(r->nonpaged);
        ExFreePool(r);
        return STATUS_INTERNAL_ERROR;
    }
        
    ExInitializeResourceLite(&r->nonpaged->load_tree_lock);
    
    InsertTailList(&Vcb->roots, &r->list_entry);
    
    if (!no_tree) {
        t->header.fs_uuid = tp.tree->header.fs_uuid;
        t->header.address = 0;
        t->header.flags = HEADER_FLAG_MIXED_BACKREF | 1; // 1 == "written"? Why does the Linux driver record this?
        t->header.chunk_tree_uuid = tp.tree->header.chunk_tree_uuid;
        t->header.generation = Vcb->superblock.generation;
        t->header.tree_id = id;
        t->header.num_items = 0;
        t->header.level = 0;

        t->has_address = FALSE;
        t->size = 0;
        t->Vcb = Vcb;
        t->parent = NULL;
        t->paritem = NULL;
        t->root = r;
        
        InitializeListHead(&t->itemlist);
    
        t->new_address = 0;
        t->has_new_address = FALSE;
        t->flags = tp.tree->flags;
        
        InsertTailList(&Vcb->trees, &t->list_entry);
        
        t->write = TRUE;
        Vcb->write_trees++;
    }
    
    *rootptr = r;

    return STATUS_SUCCESS;
}

// static void test_creating_root(device_extension* Vcb) {
//     NTSTATUS Status;
//     LIST_ENTRY rollback;
//     UINT64 id;
//     root* r;
//     
//     InitializeListHead(&rollback);
//     
//     if (Vcb->root_root->lastinode == 0)
//         get_last_inode(Vcb, Vcb->root_root);
//     
//     id = Vcb->root_root->lastinode > 0x100 ? (Vcb->root_root->lastinode + 1) : 0x101;
//     Status = create_root(Vcb, id, &r, &rollback);
//     
//     if (!NT_SUCCESS(Status)) {
//         ERR("create_root returned %08x\n", Status);
//         do_rollback(Vcb, &rollback);
//     } else {
//         Vcb->root_root->lastinode = id;
//         clear_rollback(&rollback);
//     }
// }

static NTSTATUS STDCALL set_label(device_extension* Vcb, FILE_FS_LABEL_INFORMATION* ffli) {
    ULONG utf8len;
    NTSTATUS Status;
    
    TRACE("label = %.*S\n", ffli->VolumeLabelLength / sizeof(WCHAR), ffli->VolumeLabel);
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, ffli->VolumeLabel, ffli->VolumeLabelLength);
    if (!NT_SUCCESS(Status))
        goto end;
    
    if (utf8len > MAX_LABEL_SIZE) {
        Status = STATUS_INVALID_VOLUME_LABEL;
        goto end;
    }
    
    // FIXME - check for '/' and '\\' and reject
    
    acquire_tree_lock(Vcb, TRUE);
    
//     utf8 = ExAllocatePoolWithTag(PagedPool, utf8len + 1, ALLOC_TAG);
    
    Status = RtlUnicodeToUTF8N((PCHAR)&Vcb->superblock.label, MAX_LABEL_SIZE * sizeof(WCHAR), &utf8len, ffli->VolumeLabel, ffli->VolumeLabelLength);
    if (!NT_SUCCESS(Status))
        goto release;
    
    if (utf8len < MAX_LABEL_SIZE * sizeof(WCHAR))
        RtlZeroMemory(Vcb->superblock.label + utf8len, (MAX_LABEL_SIZE * sizeof(WCHAR)) - utf8len);
    
//     test_tree_deletion(Vcb); // TESTING
//     test_tree_splitting(Vcb);
//     test_dropping_tree(Vcb);
//     test_creating_root(Vcb);
    
    Status = consider_write(Vcb);
    
release:  
    release_tree_lock(Vcb, TRUE);

end:
    TRACE("returning %08x\n", Status);

    return Status;
}

static NTSTATUS STDCALL drv_set_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOL top_level;

    TRACE("set volume information\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    Status = STATUS_NOT_IMPLEMENTED;
    
    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }
    
    if (Vcb->removing) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    switch (IrpSp->Parameters.SetVolume.FsInformationClass) {
        case FileFsControlInformation:
            FIXME("STUB: FileFsControlInformation\n");
            break;

        case FileFsLabelInformation:
            TRACE("FileFsLabelInformation\n");
    
            Status = set_label(Vcb, Irp->AssociatedIrp.SystemBuffer);
            break;

        case FileFsObjectIdInformation:
            FIXME("STUB: FileFsObjectIdInformation\n");
            break;

        default:
            WARN("Unrecognized FsInformationClass 0x%x\n", IrpSp->Parameters.SetVolume.FsInformationClass);
            break;
    }
    
end:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS delete_dir_item(device_extension* Vcb, root* subvol, UINT64 parinode, UINT32 crc32, PANSI_STRING utf8, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = parinode;
    searchkey.obj_type = TYPE_DIR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (tp.item->size < sizeof(DIR_ITEM)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        } else {
            DIR_ITEM* di;
            LONG len;
            
            di = (DIR_ITEM*)tp.item->data;
            len = tp.item->size;
            
            do {
                if (di->n == utf8->Length && RtlCompareMemory(di->name, utf8->Buffer, di->n) == di->n) {
                    ULONG newlen = tp.item->size - (sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newdi = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *dioff;
                        
                        if (!newdi) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)di > tp.item->data) {
                            RtlCopyMemory(newdi, tp.item->data, (UINT8*)di - tp.item->data);
                            dioff = newdi + ((UINT8*)di - tp.item->data);
                        } else {
                            dioff = newdi;
                        }
                        
                        if ((UINT8*)&di->name[di->n + di->m] - tp.item->data < tp.item->size)
                            RtlCopyMemory(dioff, &di->name[di->n + di->m], tp.item->size - ((UINT8*)&di->name[di->n + di->m] - tp.item->data));
                        
                        insert_tree_item(Vcb, subvol, parinode, TYPE_DIR_ITEM, crc32, newdi, newlen, NULL, rollback);
                    }
                    
                    break;
                }
                
                len -= sizeof(DIR_ITEM) - sizeof(char) + di->n + di->m;
                di = (DIR_ITEM*)&di->name[di->n + di->m];
            } while (len > 0);
        }
    } else {
        WARN("could not find DIR_ITEM for crc32 %08x\n", crc32);
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS delete_inode_ref(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 parinode, PANSI_STRING utf8, UINT64* index, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    BOOL changed = FALSE;
    NTSTATUS Status;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_REF;
    searchkey.offset = parinode;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (tp.item->size < sizeof(INODE_REF)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_REF));
        } else {
            INODE_REF* ir;
            ULONG len;
            
            ir = (INODE_REF*)tp.item->data;
            len = tp.item->size;
            
            do {
                ULONG itemlen;
                
                if (len < sizeof(INODE_REF) || len < sizeof(INODE_REF) - 1 + ir->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                itemlen = sizeof(INODE_REF) - sizeof(char) + ir->n;
                
                if (ir->n == utf8->Length && RtlCompareMemory(ir->name, utf8->Buffer, ir->n) == ir->n) {
                    ULONG newlen = tp.item->size - itemlen;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    changed = TRUE;
                    
                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newir = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *iroff;
                        
                        if (!newir) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)ir > tp.item->data) {
                            RtlCopyMemory(newir, tp.item->data, (UINT8*)ir - tp.item->data);
                            iroff = newir + ((UINT8*)ir - tp.item->data);
                        } else {
                            iroff = newir;
                        }
                        
                        if ((UINT8*)&ir->name[ir->n] - tp.item->data < tp.item->size)
                            RtlCopyMemory(iroff, &ir->name[ir->n], tp.item->size - ((UINT8*)&ir->name[ir->n] - tp.item->data));
                        
                        insert_tree_item(Vcb, subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newir, newlen, NULL, rollback);
                    }
                    
                    if (index)
                        *index = ir->index;
                    
                    break;
                }
                
                if (len > itemlen) {
                    len -= itemlen;
                    ir = (INODE_REF*)&ir->name[ir->n];
                } else
                    break;
            } while (len > 0);
            
            if (!changed) {
                WARN("found INODE_REF entry, but couldn't find filename\n");
            }
        }
    } else {
        WARN("could not find INODE_REF entry for inode %llx in %llx\n", searchkey.obj_id, searchkey.offset);
    }
    
    if (changed)
        return STATUS_SUCCESS;
    
    if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF))
        return STATUS_INTERNAL_ERROR;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_INODE_EXTREF;
    searchkey.offset = calc_crc32c((UINT32)parinode, (UINT8*)utf8->Buffer, utf8->Length);
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (tp.item->size < sizeof(INODE_EXTREF)) {
            WARN("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_EXTREF));
        } else {
            INODE_EXTREF* ier;
            ULONG len;
            
            ier = (INODE_EXTREF*)tp.item->data;
            len = tp.item->size;
            
            do {
                ULONG itemlen;
                
                if (len < sizeof(INODE_EXTREF) || len < sizeof(INODE_EXTREF) - 1 + ier->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                itemlen = sizeof(INODE_EXTREF) - sizeof(char) + ier->n;
                
                if (ier->dir == parinode && ier->n == utf8->Length && RtlCompareMemory(ier->name, utf8->Buffer, ier->n) == ier->n) {
                    ULONG newlen = tp.item->size - itemlen;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    changed = TRUE;
                    
                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newier = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *ieroff;
                        
                        if (!newier) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)ier > tp.item->data) {
                            RtlCopyMemory(newier, tp.item->data, (UINT8*)ier - tp.item->data);
                            ieroff = newier + ((UINT8*)ier - tp.item->data);
                        } else {
                            ieroff = newier;
                        }
                        
                        if ((UINT8*)&ier->name[ier->n] - tp.item->data < tp.item->size)
                            RtlCopyMemory(ieroff, &ier->name[ier->n], tp.item->size - ((UINT8*)&ier->name[ier->n] - tp.item->data));
                        
                        insert_tree_item(Vcb, subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newier, newlen, NULL, rollback);
                    }
                    
                    if (index)
                        *index = ier->index;
                    
                    break;
                }
                
                if (len > itemlen) {
                    len -= itemlen;
                    ier = (INODE_EXTREF*)&ier->name[ier->n];
                } else
                    break;
            } while (len > 0);
        }
    } else {
        WARN("couldn't find INODE_EXTREF entry either (offset = %08x)\n", (UINT32)searchkey.offset);
    }
    
    return changed ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}

static NTSTATUS delete_subvol(file_ref* fileref, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 index;
    KEY searchkey;
    traverse_ptr tp;
    UINT32 crc32;
    ROOT_ITEM* ri;
    BOOL no_ref = FALSE;
    fcb* fcb = fileref->fcb;
    
    // delete ROOT_REF in root tree
    
    Status = delete_root_ref(fcb->Vcb, fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, &fileref->utf8, &index, rollback);
    
    // A bug in Linux means that if you create a snapshot of a subvol containing another subvol,
    // the ROOT_REF and ROOT_BACKREF items won't be created, nor will num_references of ROOT_ITEM
    // be increased. In this case, we just unlink the subvol from its parent, and don't worry
    // about anything else.
    
    if (Status == STATUS_NOT_FOUND)
        no_ref = TRUE;
    else if (!NT_SUCCESS(Status)) {
        ERR("delete_root_ref returned %08x\n", Status);
        return Status;
    }
    
    if (!no_ref) {
        // delete ROOT_BACKREF in root tree
        
        Status = update_root_backref(fcb->Vcb, fcb->subvol->id, fileref->parent->fcb->subvol->id, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_root_backref returned %08x\n", Status);
            return Status;
        }
    }
    
    // delete DIR_ITEM in parent
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->utf8.Buffer, fileref->utf8.Length);
    Status = delete_dir_item(fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, crc32, &fileref->utf8, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_dir_item returned %08x\n", Status);
        return Status;
    }
    
    // delete DIR_INDEX in parent
    
    if (!no_ref) {
        searchkey.obj_id = fileref->parent->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = index;
        
        Status = find_item(fcb->Vcb, fileref->parent->fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item 1 returned %08x\n", Status);
            return Status;
        }
    
        if (!keycmp(&searchkey, &tp.item->key)) {
            delete_tree_item(fcb->Vcb, &tp, rollback);
            TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    } else {
        BOOL b;
        traverse_ptr next_tp;
        
        // If we have no ROOT_REF, we have to look through all the DIR_INDEX entries manually :-(
        
        searchkey.obj_id = fileref->parent->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = 0;
        
        Status = find_item(fcb->Vcb, fileref->parent->fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item 1 returned %08x\n", Status);
            return Status;
        }
        
        do {
            if (tp.item->key.obj_type == TYPE_DIR_INDEX && tp.item->size >= sizeof(DIR_ITEM)) {
                DIR_ITEM* di = (DIR_ITEM*)tp.item->data;
                
                if (di->key.obj_id == fcb->subvol->id && di->key.obj_type == TYPE_ROOT_ITEM && di->n == fileref->utf8.Length &&
                    tp.item->size >= sizeof(DIR_ITEM) - 1 + di->m + di->n && RtlCompareMemory(fileref->utf8.Buffer, di->name, di->n) == di->n) {
                    delete_tree_item(fcb->Vcb, &tp, rollback);
                    break;
                }
            }
        
            b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE);
            
            if (b) {
                tp = next_tp;
                
                if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
                    break;
            }
        } while (b);
    }
    
    if (no_ref)
        return STATUS_SUCCESS;
    
    if (fcb->subvol->root_item.num_references > 1) {
        UINT64 offset;
        
        // change ROOT_ITEM num_references
        
        fcb->subvol->root_item.num_references--;
        
        searchkey.obj_id = fcb->subvol->id;
        searchkey.obj_type = TYPE_ROOT_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(fcb->Vcb, fcb->Vcb->root_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item 2 returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            delete_tree_item(fcb->Vcb, &tp, rollback);
            TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            offset = tp.item->key.offset;
        } else {
            ERR("could not find ROOT_ITEM for subvol %llx\n", fcb->subvol->id);
            offset = 0;
        }
        
        ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
        if (!ri) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ri, &fcb->subvol->root_item, sizeof(ROOT_ITEM));
        
        if (!insert_tree_item(fcb->Vcb, fcb->Vcb->root_root, fcb->subvol->id, TYPE_ROOT_ITEM, offset, ri, sizeof(ROOT_ITEM), NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        RemoveEntryList(&fcb->subvol->list_entry);
        
        InsertTailList(&fcb->Vcb->drop_roots, &fcb->subvol->list_entry);
    }
    
    return STATUS_SUCCESS;
}

static WCHAR* file_desc_fcb(fcb* fcb) {
    char s[60];
    UNICODE_STRING us;
    ANSI_STRING as;
    
    if (fcb->debug_desc)
        return fcb->debug_desc;
    
    fcb->debug_desc = ExAllocatePoolWithTag(PagedPool, 60 * sizeof(WCHAR), ALLOC_TAG);
    if (!fcb->debug_desc)
        return L"(memory error)";
    
    // I know this is pretty hackish...
    // GCC doesn't like %llx in sprintf, and MSVC won't let us use swprintf
    // without the CRT, which breaks drivers.
    
    sprintf(s, "subvol %x, inode %x", (UINT32)fcb->subvol->id, (UINT32)fcb->inode);
    
    as.Buffer = s;
    as.Length = as.MaximumLength = strlen(s);
    
    us.Buffer = fcb->debug_desc;
    us.MaximumLength = 60 * sizeof(WCHAR);
    us.Length = 0;
    
    RtlAnsiStringToUnicodeString(&us, &as, FALSE);
    
    us.Buffer[us.Length / sizeof(WCHAR)] = 0;
    
    return fcb->debug_desc;
}

WCHAR* file_desc_fileref(file_ref* fileref) {
    if (fileref->debug_desc)
        return fileref->debug_desc;
    
    fileref->debug_desc = ExAllocatePoolWithTag(PagedPool, fileref->full_filename.Length + sizeof(WCHAR), ALLOC_TAG);
    if (!fileref->debug_desc)
        return L"(memory error)";
    
    RtlCopyMemory(fileref->debug_desc, fileref->full_filename.Buffer, fileref->full_filename.Length);
    fileref->debug_desc[fileref->full_filename.Length / sizeof(WCHAR)] = 0;
    
    return fileref->debug_desc;
}

WCHAR* file_desc(PFILE_OBJECT FileObject) {
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    
    if (fileref)
        return file_desc_fileref(fileref);
    else
        return file_desc_fcb(fcb);
}

void send_notification_fileref(file_ref* fileref, ULONG filter_match, ULONG action) {
    fcb* fcb = fileref->fcb;
    
    FsRtlNotifyFullReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&fileref->full_filename, fileref->name_offset * sizeof(WCHAR),
                                NULL, NULL, filter_match, action, NULL);
}

NTSTATUS delete_fileref(file_ref* fileref, PFILE_OBJECT FileObject, LIST_ENTRY* rollback) {
    ULONG bytecount;
    NTSTATUS Status;
    char* utf8 = NULL;
    UINT32 crc32;
    KEY searchkey;
    traverse_ptr tp, tp2;
    UINT64 parinode, index;
    root* parsubvol;
    INODE_ITEM *ii, *dirii;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    LIST_ENTRY changed_sector_list;
    fcb* fcb = fileref->fcb;
#ifdef _DEBUG
    LARGE_INTEGER freq, time1, time2;
#endif
    
    if (fileref->deleted || fcb->deleted) {
        WARN("trying to delete already-deleted file\n");
        return STATUS_SUCCESS;
    }
    
    if (fileref == fcb->Vcb->root_fileref) {
        ERR("error - trying to delete root FCB\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (fcb->inode == SUBVOL_ROOT_INODE) {
        Status = delete_subvol(fileref, rollback);
        
        if (!NT_SUCCESS(Status))
            goto exit;
        else {
            parinode = fileref->parent->fcb->inode;
            parsubvol = fileref->parent->fcb->subvol;
            bytecount = fileref->utf8.Length;
            goto success2;
        }
    }
    
#ifdef _DEBUG
    time1 = KeQueryPerformanceCounter(&freq);
#endif
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
    if (fcb->ads) {
        char* s;
        TRACE("deleting ADS\n");
        
        s = ExAllocatePoolWithTag(PagedPool, fcb->adsxattr.Length + 1, ALLOC_TAG);
        if (!s) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        RtlCopyMemory(s, fcb->adsxattr.Buffer, fcb->adsxattr.Length);
        s[fcb->adsxattr.Length] = 0;
        
        if (!delete_xattr(fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, s, fcb->adshash, rollback)) {
            ERR("failed to delete xattr %s\n", s);
        }
        
        ExFreePool(s);
        
        fileref->parent->fcb->inode_item.transid = fcb->Vcb->superblock.generation;
        fileref->parent->fcb->inode_item.sequence++;
        fileref->parent->fcb->inode_item.st_ctime = now;
        
        searchkey.obj_id = fileref->parent->fcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(fcb->Vcb, fileref->parent->fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto exit;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("error - could not find INODE_ITEM for inode %llx in subvol %llx\n", fileref->parent->fcb->inode, fileref->parent->fcb->subvol->id);
            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }

        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        RtlCopyMemory(ii, &fileref->parent->fcb->inode_item, sizeof(INODE_ITEM));
        delete_tree_item(fcb->Vcb, &tp, rollback);
        
        insert_tree_item(fcb->Vcb, fileref->parent->fcb->subvol, searchkey.obj_id, searchkey.obj_type, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
        
        fileref->parent->fcb->subvol->root_item.ctransid = fcb->Vcb->superblock.generation;
        fileref->parent->fcb->subvol->root_item.ctime = now;
        
        goto success;
    }
    
    Status = RtlUnicodeToUTF8N(NULL, 0, &bytecount, fileref->filepart.Buffer, fileref->filepart.Length);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeToUTF8N failed with error %08x\n", Status);
        return Status;
    }
    
    utf8 = ExAllocatePoolWithTag(PagedPool, bytecount + 1, ALLOC_TAG);
    if (!utf8) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlUnicodeToUTF8N(utf8, bytecount, &bytecount, fileref->filepart.Buffer, fileref->filepart.Length);
    utf8[bytecount] = 0;
    
    crc32 = calc_crc32c(0xfffffffe, (UINT8*)utf8, bytecount);

    TRACE("deleting %.*S\n", file_desc_fileref(fileref));
    
    if (fileref->parent->fcb->subvol == fcb->subvol)
        parinode = fileref->parent->fcb->inode;
    else
        parinode = SUBVOL_ROOT_INODE;
    
    parsubvol = fcb->subvol;
    
    // delete DIR_ITEM (0x54)
    
    Status = delete_dir_item(fcb->Vcb, fcb->subvol, parinode, crc32, &fileref->utf8, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_dir_item returned %08x\n", Status);
        return Status;
    }
    
    // delete INODE_REF (0xc)
    
    index = 0;
    
    Status = delete_inode_ref(fcb->Vcb, fcb->subvol, fcb->inode, parinode, &fileref->utf8, &index, rollback);
    
    // delete DIR_INDEX (0x60)
    
    searchkey.obj_id = parinode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = index;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        delete_tree_item(fcb->Vcb, &tp, rollback);
        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
    }
    
    // delete INODE_ITEM (0x1)
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto exit;
    }
    
    tp = tp2;
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("error - INODE_ITEM not found\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    if (tp.item->size < sizeof(INODE_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(INODE_ITEM));
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    ii = (INODE_ITEM*)tp.item->data;
    TRACE("nlink = %u\n", ii->st_nlink);
    
    if (ii->st_nlink > 1) {
        INODE_ITEM* newii;
        
        newii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!newii) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        RtlCopyMemory(newii, ii, sizeof(INODE_ITEM));
        newii->st_nlink--;
        newii->transid = fcb->Vcb->superblock.generation;
        newii->sequence++;
        newii->st_ctime = now;
        
        TRACE("replacing (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        delete_tree_item(fcb->Vcb, &tp, rollback);
        
        if (!insert_tree_item(fcb->Vcb, fcb->subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newii, sizeof(INODE_ITEM), NULL, rollback))
            ERR("error - failed to insert item\n");
        
        goto success2;
    }
    
    delete_tree_item(fcb->Vcb, &tp, rollback);
    TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
    
    fcb->deleted = TRUE;
    
    // delete XATTR_ITEM (0x18)
    
    while (find_next_item(fcb->Vcb, &tp, &tp2, FALSE)) {
        tp = tp2;
        
        if (tp.item->key.obj_id == fcb->inode) {
            // FIXME - do metadata thing here too?
            if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
                delete_tree_item(fcb->Vcb, &tp, rollback);
                TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            }
        } else
            break;
    }
    
    // excise extents
    
    InitializeListHead(&changed_sector_list);
    
    if (fcb->type != BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0) {
        Status = excise_extents(fcb->Vcb, fcb, 0, sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), &changed_sector_list, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08x\n", Status);
            goto exit;
        }
        
        if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM))
            update_checksum_tree(fcb->Vcb, &changed_sector_list, rollback);
    }
    
success2:
    // update INODE_ITEM of parent
    
    searchkey.obj_id = parinode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, parsubvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_tree returned %08x\n", Status);
        goto exit;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("error - could not find INODE_ITEM for parent directory %llx in subvol %llx\n", parinode, parsubvol->id);
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    TRACE("fileref->parent->fcb->inode_item.st_size was %llx\n", fileref->parent->fcb->inode_item.st_size);
    fileref->parent->fcb->inode_item.st_size -= bytecount * 2;
    TRACE("fileref->parent->fcb->inode_item.st_size now %llx\n", fileref->parent->fcb->inode_item.st_size);
    fileref->parent->fcb->inode_item.transid = fcb->Vcb->superblock.generation;
    fileref->parent->fcb->inode_item.sequence++;
    fileref->parent->fcb->inode_item.st_ctime = now;
    fileref->parent->fcb->inode_item.st_mtime = now;

    dirii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!dirii) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    RtlCopyMemory(dirii, &fileref->parent->fcb->inode_item, sizeof(INODE_ITEM));
    delete_tree_item(fcb->Vcb, &tp, rollback);
    
    insert_tree_item(fcb->Vcb, parsubvol, searchkey.obj_id, searchkey.obj_type, searchkey.offset, dirii, sizeof(INODE_ITEM), NULL, rollback);
    
    parsubvol->root_item.ctransid = fcb->Vcb->superblock.generation;
    parsubvol->root_item.ctime = now;
    
success:
    consider_write(fcb->Vcb);
    
    fileref->deleted = TRUE;
    
    fcb->Header.AllocationSize.QuadPart = 0;
    fcb->Header.FileSize.QuadPart = 0;
    fcb->Header.ValidDataLength.QuadPart = 0;
    
    if (FileObject && FileObject->PrivateCacheMap) {
        CC_FILE_SIZES ccfs;
        
        ccfs.AllocationSize = fcb->Header.AllocationSize;
        ccfs.FileSize = fcb->Header.FileSize;
        ccfs.ValidDataLength = fcb->Header.ValidDataLength;
        
        CcSetFileSizes(FileObject, &ccfs);
    }
    
    // FIXME - set deleted flag of any open FCBs for ADS
    
    if (FileObject && FileObject->FsContext2) {
        ccb* ccb = FileObject->FsContext2;
        
        if (ccb->fileref)
            send_notification_fileref(ccb->fileref, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED);
    }
    
#ifdef _DEBUG
    time2 = KeQueryPerformanceCounter(NULL);
#endif
    
    TRACE("time = %u (freq = %u)\n", (UINT32)(time2.QuadPart - time1.QuadPart), (UINT32)freq.QuadPart);
    
    Status = STATUS_SUCCESS;
    
exit:
    if (utf8)
        ExFreePool(utf8);
    
    return Status;
}

void _free_fcb(fcb* fcb, const char* func, const char* file, unsigned int line) {
    LONG rc;

    rc = InterlockedDecrement(&fcb->refcount);
    
#ifdef DEBUG_FCB_REFCOUNTS
//     WARN("fcb %p: refcount now %i (%.*S)\n", fcb, rc, fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
#ifdef DEBUG_LONG_MESSAGES
    _debug_message(func, file, line, "fcb %p: refcount now %i (subvol %llx, inode %llx)\n", fcb, rc, fcb->subvol ? fcb->subvol->id : 0, fcb->inode);
#else
    _debug_message(func, "fcb %p: refcount now %i (subvol %llx, inode %llx)\n", fcb, rc, fcb->subvol ? fcb->subvol->id : 0, fcb->inode);
#endif
#endif
    
    if (rc > 0)
        return;
    
    ExAcquireResourceExclusiveLite(&fcb->Vcb->fcb_lock, TRUE);
   
    ExDeleteResourceLite(&fcb->nonpaged->resource);
    ExDeleteResourceLite(&fcb->nonpaged->paging_resource);
    ExFreePool(fcb->nonpaged);
    
    if (fcb->list_entry.Flink)
        RemoveEntryList(&fcb->list_entry);
    
    if (fcb->sd)
        ExFreePool(fcb->sd);
    
    if (fcb->adsxattr.Buffer)
        ExFreePool(fcb->adsxattr.Buffer);
    
    if (fcb->debug_desc)
        ExFreePool(fcb->debug_desc);
    
    FsRtlUninitializeFileLock(&fcb->lock);
    
    ExReleaseResourceLite(&fcb->Vcb->fcb_lock);
    
    ExFreePool(fcb);
#ifdef DEBUG_FCB_REFCOUNTS
#ifdef DEBUG_LONG_MESSAGES
    _debug_message(func, file, line, "freeing fcb %p\n", fcb);
#else
    _debug_message(func, "freeing fcb %p\n", fcb);
#endif
#endif
}

void _free_fileref(file_ref* fr, const char* func, const char* file, unsigned int line) {
    LONG rc;

    rc = InterlockedDecrement(&fr->refcount);
    
#ifdef DEBUG_FCB_REFCOUNTS
#ifdef DEBUG_LONG_MESSAGES
    _debug_message(func, file, line, "fileref %p: refcount now %i\n", fr, rc);
#else
    _debug_message(func, "fileref %p: refcount now %i\n", fr, rc);
#endif
#endif
    
#ifdef _DEBUG
    if (rc < 0) {
        ERR("fileref %p: refcount now %i\n", fr, rc);
        int3;
    }
#endif
    
    if (rc > 0)
        return;
    
    // FIXME - do we need a file_ref lock?
    
    // FIXME - do delete if needed
    
    if (fr->filepart.Buffer)
        ExFreePool(fr->filepart.Buffer);
    
    if (fr->utf8.Buffer)
        ExFreePool(fr->utf8.Buffer);
    
    if (fr->full_filename.Buffer)
        ExFreePool(fr->full_filename.Buffer);
    
    if (fr->debug_desc)
        ExFreePool(fr->debug_desc);
    
    // FIXME - throw error if children not empty
    
    free_fcb(fr->fcb);
    
    if (fr->list_entry.Flink)
        RemoveEntryList(&fr->list_entry);
    
    if (fr->parent)
        free_fileref((file_ref*)fr->parent);
    
    ExFreePool(fr);
}

static NTSTATUS STDCALL close_file(device_extension* Vcb, PFILE_OBJECT FileObject) {
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref = NULL;
    
    TRACE("FileObject = %p\n", FileObject);
    
    fcb = FileObject->FsContext;
    if (!fcb) {
        TRACE("FCB was NULL, returning success\n");
        return STATUS_SUCCESS;
    }
    
    ccb = FileObject->FsContext2;
    
    TRACE("close called for %S (fcb == %p)\n", file_desc(FileObject), fcb);
    
    FsRtlNotifyCleanup(Vcb->NotifySync, &Vcb->DirNotifyList, ccb);
    
    // FIXME - make sure notification gets sent if file is being deleted
    
    if (ccb) {    
        if (ccb->query_string.Buffer)
            RtlFreeUnicodeString(&ccb->query_string);
        
        // FIXME - use refcounts for fileref
        fileref = ccb->fileref;
        
        ExFreePool(ccb);
    }
    
    CcUninitializeCacheMap(FileObject, NULL, NULL);
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);
    
    if (fileref)
        free_fileref(fileref);
    else
        free_fcb(fcb);
    
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    return STATUS_SUCCESS;
}

void STDCALL uninit(device_extension* Vcb, BOOL flush) {
    chunk* c;
    space* s;
    UINT64 i;
    LIST_ENTRY rollback;
    
    if (flush) {
        InitializeListHead(&rollback);
        
        acquire_tree_lock(Vcb, TRUE);

        if (Vcb->write_trees > 0)
            do_write(Vcb, &rollback);
        
        free_trees(Vcb);
        
        clear_rollback(&rollback);

        release_tree_lock(Vcb, TRUE);
    }
    
    // FIXME - stop async threads
    
    free_fcb(Vcb->volume_fcb);
    free_fileref(Vcb->root_fileref);
    
    // FIXME - free any open fcbs?

    while (!IsListEmpty(&Vcb->roots)) {
        LIST_ENTRY* le = RemoveHeadList(&Vcb->roots);
        root* r = CONTAINING_RECORD(le, root, list_entry);

        ExDeleteResourceLite(&r->nonpaged->load_tree_lock);
        ExFreePool(r->nonpaged);
        ExFreePool(r);
    }
    
    while (!IsListEmpty(&Vcb->chunks)) {
        LIST_ENTRY* le = RemoveHeadList(&Vcb->chunks);
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        while (!IsListEmpty(&c->space)) {
            LIST_ENTRY* le2 = RemoveHeadList(&c->space);
            s = CONTAINING_RECORD(le2, space, list_entry);
            
            ExFreePool(s);
        }
        
        if (c->devices)
            ExFreePool(c->devices);
        
        ExFreePool(c->chunk_item);
        ExFreePool(c);
    }
    
    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        while (!IsListEmpty(&Vcb->devices[i].disk_holes)) {
            LIST_ENTRY* le = RemoveHeadList(&Vcb->devices[i].disk_holes);
            disk_hole* dh = CONTAINING_RECORD(le, disk_hole, listentry);
            
            ExFreePool(dh);
        }
    }
    
    ExFreePool(Vcb->devices);
    
    ExDeleteResourceLite(&Vcb->fcb_lock);
    ExDeleteResourceLite(&Vcb->load_lock);
    ExDeleteResourceLite(&Vcb->tree_lock);
    
    ZwClose(Vcb->flush_thread_handle);
}

static NTSTATUS STDCALL drv_cleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb;
    BOOL top_level;

    TRACE("cleanup\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (DeviceObject == devobj) {
        TRACE("closing file system\n");
        Status = STATUS_SUCCESS;
        goto exit;
    }
    
    if (FileObject && FileObject->FsContext) {
        LONG oc;
        ccb* ccb;
        file_ref* fileref;
        
        fcb = FileObject->FsContext;
        ccb = FileObject->FsContext2;
        fileref = ccb ? ccb->fileref : NULL;
        
        TRACE("cleanup called for FileObject %p\n", FileObject);
        TRACE("fcb %p (%S), refcount = %u, open_count = %u\n", fcb, file_desc(FileObject), fcb->refcount, fcb->open_count);
        
        IoRemoveShareAccess(FileObject, &fcb->share_access);
        
        oc = InterlockedDecrement(&fcb->open_count);
#ifdef DEBUG_FCB_REFCOUNTS
        ERR("fcb %p: open_count now %i\n", fcb, oc);
#endif
        
        if (ccb && ccb->options & FILE_DELETE_ON_CLOSE && fileref)
            fileref->delete_on_close = TRUE;
        
        if (fileref && fileref->delete_on_close && fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0)
            fileref->delete_on_close = FALSE;
        
        if (oc == 0) {
            if (fileref && fileref->delete_on_close && fileref != fcb->Vcb->root_fileref && fcb != fcb->Vcb->volume_fcb) {
                LIST_ENTRY rollback;
                InitializeListHead(&rollback);
                
                acquire_tree_lock(fcb->Vcb, TRUE);
                
                Status = delete_fileref(fileref, FileObject, &rollback);
                
                if (NT_SUCCESS(Status)) {
                    LARGE_INTEGER newlength;

                    if (FileObject->Flags & FO_CACHE_SUPPORTED && fcb->nonpaged->segment_object.DataSectionObject)
                        CcPurgeCacheSection(&fcb->nonpaged->segment_object, NULL, 0, FALSE);
                    
                    newlength.QuadPart = 0;
                    
                    if (!CcUninitializeCacheMap(FileObject, &newlength, NULL)) {
                        TRACE("CcUninitializeCacheMap failed\n");
                    }
                    
                    clear_rollback(&rollback);
                } else
                    do_rollback(fcb->Vcb, &rollback);
                
                release_tree_lock(fcb->Vcb, TRUE);
            } else if (FileObject->Flags & FO_CACHE_SUPPORTED && fcb->nonpaged->segment_object.DataSectionObject) {
                IO_STATUS_BLOCK iosb;
                CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &iosb);
                
                if (!NT_SUCCESS(iosb.Status)) {
                    ERR("CcFlushCache returned %08x\n", iosb.Status);
                }

                ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, TRUE);
                ExReleaseResourceLite(fcb->Header.PagingIoResource);

                CcPurgeCacheSection(&fcb->nonpaged->segment_object, NULL, 0, FALSE);
                
                TRACE("flushed cache on close (FileObject = %p, fcb = %p, AllocationSize = %llx, FileSize = %llx, ValidDataLength = %llx)\n",
                      FileObject, fcb, fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
            }
            
            if (fcb->Vcb && fcb != fcb->Vcb->volume_fcb)
                CcUninitializeCacheMap(FileObject, NULL, NULL);
        }
        
        FileObject->Flags |= FO_CLEANUP_COMPLETE;
    }
    
    Status = STATUS_SUCCESS;

exit:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

ULONG STDCALL get_file_attributes(device_extension* Vcb, INODE_ITEM* ii, root* r, UINT64 inode, UINT8 type, BOOL dotfile, BOOL ignore_xa) {
    ULONG att;
    char* eaval;
    UINT16 ealen;
    
    // ii can be NULL
    
    if (!ignore_xa && get_xattr(Vcb, r, inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (UINT8**)&eaval, &ealen)) {
        if (ealen > 2) {
            if (eaval[0] == '0' && eaval[1] == 'x') {
                int i;
                ULONG dosnum = 0;

                for (i = 2; i < ealen; i++) {
                    dosnum *= 0x10;
                    
                    if (eaval[i] >= '0' && eaval[i] <= '9')
                        dosnum |= eaval[i] - '0';
                    else if (eaval[i] >= 'a' && eaval[i] <= 'f')
                        dosnum |= eaval[i] + 10 - 'a';
                    else if (eaval[i] >= 'A' && eaval[i] <= 'F')
                        dosnum |= eaval[i] + 10 - 'a';
                }
                
                TRACE("DOSATTRIB: %08x\n", dosnum);

                ExFreePool(eaval);
                
                if (type == BTRFS_TYPE_DIRECTORY)
                    dosnum |= FILE_ATTRIBUTE_DIRECTORY;
                else if (type == BTRFS_TYPE_SYMLINK)
                    dosnum |= FILE_ATTRIBUTE_REPARSE_POINT;
                
                return dosnum;
            }
        }
        
        ExFreePool(eaval);
    }
    
    switch (type) {
        case BTRFS_TYPE_DIRECTORY:
            att = FILE_ATTRIBUTE_DIRECTORY;
            break;
            
        case BTRFS_TYPE_SYMLINK:
            att = FILE_ATTRIBUTE_REPARSE_POINT;
            break;
           
        default:
            att = 0;
            break;
    }
    
    if (dotfile) {
        att |= FILE_ATTRIBUTE_HIDDEN;
    }
    
    att |= FILE_ATTRIBUTE_ARCHIVE;
    
    // FIXME - get READONLY from ii->st_mode
    // FIXME - return SYSTEM for block/char devices?
    
    if (att == 0)
        att = FILE_ATTRIBUTE_NORMAL;
    
    return att;
}

// static int STDCALL utf8icmp(char* a, char* b) {
//     return strcmp(a, b); // FIXME - don't treat as ASCII
// }

NTSTATUS sync_read_phys(PDEVICE_OBJECT DeviceObject, LONGLONG StartingOffset, ULONG Length, PUCHAR Buffer) {
    IO_STATUS_BLOCK* IoStatus;
    LARGE_INTEGER Offset;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    read_context* context;
    
    num_reads++;
    
    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context, sizeof(read_context));
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    IoStatus = ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_STATUS_BLOCK), ALLOC_TAG);
    if (!IoStatus) {
        ERR("out of memory\n");
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Offset.QuadPart = StartingOffset;

//     Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, DeviceObject, Buffer, Length, &Offset, /*&Event*/NULL, IoStatus);
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    
    if (!Irp) {
        ERR("IoAllocateIrp failed\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_READ;
    
    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        FIXME("FIXME - buffered IO\n");
    } else if (DeviceObject->Flags & DO_DIRECT_IO) {
//         TRACE("direct IO\n");
        
        Irp->MdlAddress = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
        if (!Irp->MdlAddress) {
            ERR("IoAllocateMdl failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
    //         IoFreeIrp(Irp);
            goto exit;
//         } else {
//             TRACE("got MDL %p from buffer %p\n", Irp->MdlAddress, Buffer);
        }
        
        MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoWriteAccess);
    } else {
//         TRACE("neither buffered nor direct IO\n");
        Irp->UserBuffer = Buffer;
    }

    IrpSp->Parameters.Read.Length = Length;
    IrpSp->Parameters.Read.ByteOffset = Offset;
    
    Irp->UserIosb = IoStatus;
//     Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    
    Irp->UserEvent = &context->Event;

//     IoQueueThreadIrp(Irp);
    
    IoSetCompletionRoutine(Irp, read_completion, context, TRUE, TRUE, TRUE);

//     if (Override)
//     {
//         Stack = IoGetNextIrpStackLocation(Irp);
//         Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
//     }

//     TRACE("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);

//     TRACE("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING) {
//         TRACE("Operation pending\n");
        KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
//         TRACE("Getting IO Status... for %p\n", Irp);
        Status = context->iosb.Status;
    }
    
    if (DeviceObject->Flags & DO_DIRECT_IO) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }
    
exit:
    IoFreeIrp(Irp);

    ExFreePool(IoStatus);
    ExFreePool(context);
    
    return Status;
}

static NTSTATUS STDCALL read_superblock(device_extension* Vcb, PDEVICE_OBJECT device) {
    NTSTATUS Status;
    superblock* sb;
    unsigned int i, to_read;
    UINT32 crc32;
    
    to_read = sector_align(sizeof(superblock), device->SectorSize);
    
    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    i = 0;
    
    while (superblock_addrs[i] > 0) {
        if (i > 0 && superblock_addrs[i] + sizeof(superblock) > Vcb->length)
            break;
        
        Status = sync_read_phys(device, superblock_addrs[i], to_read, (PUCHAR)sb);
        if (!NT_SUCCESS(Status)) {
            ERR("Failed to read superblock %u: %08x\n", i, Status);
            ExFreePool(sb);
            return Status;
        }
        
        TRACE("got superblock %u!\n", i);

        if (i == 0 || sb->generation > Vcb->superblock.generation)
            RtlCopyMemory(&Vcb->superblock, sb, sizeof(superblock));
        
        i++;
    }
    
    ExFreePool(sb);
    
    crc32 = calc_crc32c(0xffffffff, (UINT8*)&Vcb->superblock.uuid, (ULONG)sizeof(superblock) - sizeof(Vcb->superblock.checksum));
    crc32 = ~crc32;
    TRACE("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)Vcb->superblock.checksum));
    
    if (crc32 != *((UINT32*)Vcb->superblock.checksum))
        return STATUS_INTERNAL_ERROR; // FIXME - correct error?
    
    TRACE("label is %s\n", Vcb->superblock.label);
//     utf8_to_utf16(Vcb->superblock.label, Vcb->label, MAX_LABEL_SIZE * sizeof(WCHAR));
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL dev_ioctl(PDEVICE_OBJECT DeviceObject, ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferSize,
                           PVOID OutputBuffer, ULONG OutputBufferSize, BOOLEAN Override, IO_STATUS_BLOCK* iosb)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        OutputBufferSize,
                                        FALSE,
                                        &Event,
                                        &IoStatus);

    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    if (Override) {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    
    if (iosb)
        *iosb = IoStatus;

    return Status;
}

// static void STDCALL find_chunk_root(device_extension* Vcb) {
//     UINT32 i;
//     KEY* key;
//     
//     i = 0;
//     while (i < Vcb->superblock.n) {
//         key = &Vcb->superblock.sys_chunk_array[i];
//         i += sizeof(KEY);
//     }
//     
//     // FIXME
// }

// static void STDCALL insert_ltp(device_extension* Vcb, log_to_phys* ltp) {
//     if (!Vcb->log_to_phys) {
//         Vcb->log_to_phys = ltp;
//         ltp->next = NULL;
//         return;
//     }
//     
//     // FIXME - these should be ordered
//     ltp->next = Vcb->log_to_phys;
//     Vcb->log_to_phys = ltp;
// }

static NTSTATUS STDCALL add_root(device_extension* Vcb, UINT64 id, UINT64 addr, traverse_ptr* tp) {
    root* r = ExAllocatePoolWithTag(PagedPool, sizeof(root), ALLOC_TAG);
    if (!r) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    r->id = id;
    r->path.Buffer = NULL;
    r->treeholder.address = addr;
    r->treeholder.tree = NULL;
    init_tree_holder(&r->treeholder);
    InitializeListHead(&r->fcbs);

    r->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(root_nonpaged), ALLOC_TAG);
    if (!r->nonpaged) {
        ERR("out of memory\n");
        ExFreePool(r);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ExInitializeResourceLite(&r->nonpaged->load_tree_lock);
    
    r->lastinode = 0;
    
    if (tp) {
        RtlCopyMemory(&r->root_item, tp->item->data, min(sizeof(ROOT_ITEM), tp->item->size));
        if (tp->item->size < sizeof(ROOT_ITEM))
            RtlZeroMemory(((UINT8*)&r->root_item) + tp->item->size, sizeof(ROOT_ITEM) - tp->item->size);
    }
    
    InsertTailList(&Vcb->roots, &r->list_entry);
    
    switch (r->id) {
        case BTRFS_ROOT_ROOT:
            Vcb->root_root = r;
            break;
            
        case BTRFS_ROOT_EXTENT:
            Vcb->extent_root = r;
            break;
            
        case BTRFS_ROOT_CHUNK:
            Vcb->chunk_root = r;
            break;
            
        case BTRFS_ROOT_DEVTREE:
            Vcb->dev_root = r;
            break;
            
        case BTRFS_ROOT_CHECKSUM:
            Vcb->checksum_root = r;
            break;
            
        case BTRFS_ROOT_UUID:
            Vcb->uuid_root = r;
            break;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL look_for_roots(device_extension* Vcb) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    BOOL b;
    NTSTATUS Status;
    
    searchkey.obj_id = 0;
    searchkey.obj_type = 0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_tree returned %08x\n", Status);
        return Status;
    }
    
    do {
        TRACE("(%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        if (tp.item->key.obj_type == TYPE_ROOT_ITEM) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            if (tp.item->size < offsetof(ROOT_ITEM, byte_limit)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, offsetof(ROOT_ITEM, byte_limit));
            } else {
                TRACE("root %llx - address %llx\n", tp.item->key.obj_id, ri->block_number);
                
                Status = add_root(Vcb, tp.item->key.obj_id, ri->block_number, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_root returned %08x\n", Status);
                    return Status;
                }
            }
        }
    
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (b)
            tp = next_tp;
    } while (b);
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_disk_hole(LIST_ENTRY* disk_holes, UINT64 address, UINT64 size) {
    disk_hole* dh = ExAllocatePoolWithTag(PagedPool, sizeof(disk_hole), ALLOC_TAG);
    
    if (!dh) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    dh->address = address;
    dh->size = size;
    dh->provisional = FALSE;
    
    InsertTailList(disk_holes, &dh->listentry);
    
    return STATUS_SUCCESS;
}

static NTSTATUS find_disk_holes(device_extension* Vcb, device* dev) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    UINT64 lastaddr;
    NTSTATUS Status;
    
    InitializeListHead(&dev->disk_holes);
    
    searchkey.obj_id = dev->devitem.dev_id;
    searchkey.obj_type = TYPE_DEV_EXTENT;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_tree returned %08x\n", Status);
        return Status;
    }
    
    lastaddr = 0;
    
    do {
        if (tp.item->key.obj_id == dev->devitem.dev_id && tp.item->key.obj_type == TYPE_DEV_EXTENT) {
            if (tp.item->size >= sizeof(DEV_EXTENT)) {
                DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;
                
                if (tp.item->key.offset > lastaddr) {
                    Status = add_disk_hole(&dev->disk_holes, lastaddr, tp.item->key.offset - lastaddr);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_disk_hole returned %08x\n", Status);
                        return Status;
                    }
                }

                lastaddr = tp.item->key.offset + de->length;
            } else {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DEV_EXTENT));
            }
        }
    
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (b) {
            tp = next_tp;
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    if (lastaddr < dev->devitem.num_bytes) {
        Status = add_disk_hole(&dev->disk_holes, lastaddr, dev->devitem.num_bytes - lastaddr);
        if (!NT_SUCCESS(Status)) {
            ERR("add_disk_hole returned %08x\n", Status);
            return Status;
        }
    }
    
    // FIXME - free disk_holes when unmounting
    
    return STATUS_SUCCESS;
}

device* find_device_from_uuid(device_extension* Vcb, BTRFS_UUID* uuid) {
    UINT64 i;
    
    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        TRACE("device %llx, uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", i,
            Vcb->devices[i].devitem.device_uuid.uuid[0], Vcb->devices[i].devitem.device_uuid.uuid[1], Vcb->devices[i].devitem.device_uuid.uuid[2], Vcb->devices[i].devitem.device_uuid.uuid[3], Vcb->devices[i].devitem.device_uuid.uuid[4], Vcb->devices[i].devitem.device_uuid.uuid[5], Vcb->devices[i].devitem.device_uuid.uuid[6], Vcb->devices[i].devitem.device_uuid.uuid[7],
            Vcb->devices[i].devitem.device_uuid.uuid[8], Vcb->devices[i].devitem.device_uuid.uuid[9], Vcb->devices[i].devitem.device_uuid.uuid[10], Vcb->devices[i].devitem.device_uuid.uuid[11], Vcb->devices[i].devitem.device_uuid.uuid[12], Vcb->devices[i].devitem.device_uuid.uuid[13], Vcb->devices[i].devitem.device_uuid.uuid[14], Vcb->devices[i].devitem.device_uuid.uuid[15]);
        
        if (Vcb->devices[i].devobj && RtlCompareMemory(&Vcb->devices[i].devitem.device_uuid, uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            TRACE("returning device %llx\n", i);
            return &Vcb->devices[i];
        }
    }
    
    WARN("could not find device with uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
         uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3], uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
         uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11], uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    
    return NULL;
}

static NTSTATUS STDCALL load_chunk_root(device_extension* Vcb) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    BOOL b;
    chunk* c;
    UINT64 i;
    NTSTATUS Status;

    searchkey.obj_id = 0;
    searchkey.obj_type = 0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    do {
        TRACE("(%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        
        if (tp.item->key.obj_id == 1 && tp.item->key.obj_type == TYPE_DEV_ITEM && tp.item->key.offset == 1) {
            // FIXME - this is a hack; make this work with multiple devices!
            if (tp.item->size > 0)
                RtlCopyMemory(&Vcb->devices[0].devitem, tp.item->data, min(tp.item->size, sizeof(DEV_ITEM)));
        } else if (tp.item->key.obj_type == TYPE_CHUNK_ITEM) {
            if (tp.item->size < sizeof(CHUNK_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(CHUNK_ITEM));
            } else {            
                c = ExAllocatePoolWithTag(PagedPool, sizeof(chunk), ALLOC_TAG);
                
                if (!c) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                c->size = tp.item->size;
                c->offset = tp.item->key.offset;
                c->used = c->oldused = 0;
                c->space_changed = FALSE;
                c->cache_inode = 0;
                c->cache_size = 0;
                
                c->chunk_item = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!c->chunk_item) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            
                RtlCopyMemory(c->chunk_item, tp.item->data, tp.item->size);
                
                if (c->chunk_item->num_stripes > 0) {
                    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
                    
                    c->devices = ExAllocatePoolWithTag(PagedPool, sizeof(device*) * c->chunk_item->num_stripes, ALLOC_TAG);
                    
                    if (!c->devices) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    for (i = 0; i < c->chunk_item->num_stripes; i++) {
                        c->devices[i] = find_device_from_uuid(Vcb, &cis[i].dev_uuid);
                        TRACE("device %llu = %p\n", i, c->devices[i]);
                    }
                } else
                    c->devices = NULL;
                
                InitializeListHead(&c->space);

                InsertTailList(&Vcb->chunks, &c->list_entry);
            }
        }
    
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (b)
            tp = next_tp;
    } while (b);
    
    Vcb->log_to_phys_loaded = TRUE;
    
    return STATUS_SUCCESS;
}

void protect_superblocks(device_extension* Vcb, chunk* c) {
    int i = 0, j;
    UINT64 off_start, off_end;
    
    // FIXME - this will need modifying for RAID
    
    while (superblock_addrs[i] != 0) {
        CHUNK_ITEM* ci = c->chunk_item;
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        
        for (j = 0; j < ci->num_stripes; j++) {
            if (cis[j].offset + ci->size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                TRACE("cut out superblock in chunk %llx\n", c->offset);
                
                // The Linux driver protects the whole stripe in which the superblock lives
                
                off_start = ((superblock_addrs[i] - cis[j].offset) / c->chunk_item->stripe_length) * c->chunk_item->stripe_length;
                off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), c->chunk_item->stripe_length);
                
                add_to_space_list(c, c->offset + off_start, off_end - off_start, SPACE_TYPE_USED);
            }
        }
        
        i++;
    }
}

static NTSTATUS STDCALL find_chunk_usage(device_extension* Vcb) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;
    
// c00000,c0,800000
// block_group_item size=7f0000 chunktreeid=100 flags=1
    
    searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        searchkey.obj_id = c->offset;
        searchkey.offset = c->chunk_item->size;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&searchkey, &tp.item->key)) {
            if (tp.item->size >= sizeof(BLOCK_GROUP_ITEM)) {
                bgi = (BLOCK_GROUP_ITEM*)tp.item->data;
                
                c->used = c->oldused = bgi->used;
                
                TRACE("chunk %llx has %llx bytes used\n", c->offset, c->used);
            } else {
                ERR("(%llx;%llx,%x,%llx) is %u bytes, expected %u\n",
                    Vcb->extent_root->id, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
            }
        }
        
//         if (addr >= c->offset && (addr - c->offset) < c->chunk_item->size && c->chunk_item->num_stripes > 0) {
//             cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
// 
//             return (addr - c->offset) + cis->offset;
//         }
        
        // FIXME - make sure we free occasionally after doing one of these, or we
        // might use up a lot of memory with a big disk.
        
        Status = load_free_space_cache(Vcb, c);
        if (!NT_SUCCESS(Status)) {
            ERR("load_free_space_cache returned %08x\n", Status);
            return Status;
        }        
        
        protect_superblocks(Vcb, c);

        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

// static void STDCALL root_test(device_extension* Vcb) {
//     root* r;
//     KEY searchkey;
//     traverse_ptr tp, next_tp;
//     BOOL b;
//     
//     r = Vcb->roots;
//     while (r) {
//         if (r->id == 0x102)
//             break;
//         r = r->next;
//     }
//     
//     if (!r) {
//         ERR("Could not find root tree.\n");
//         return;
//     }
//     
//     searchkey.obj_id = 0x1b6;
//     searchkey.obj_type = 0xb;
//     searchkey.offset = 0;
//     
//     if (!find_item(Vcb, r, &tp, &searchkey, NULL, FALSE)) {
//         ERR("Could not find first item.\n");
//         return;
//     }
//     
//     b = TRUE;
//     do {
//         TRACE("%x,%x,%x\n", (UINT32)tp.item->key.obj_id, tp.item->key.obj_type, (UINT32)tp.item->key.offset);
//         
//         b = find_prev_item(Vcb, &tp, &next_tp, NULL, FALSE);
//         
//         if (b) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//         }
//     } while (b);
//     
//     free_traverse_ptr(&tp);
// }

static NTSTATUS load_sys_chunks(device_extension* Vcb) {
    KEY key;
    ULONG n = Vcb->superblock.n;
    
    while (n > 0) {
        if (n > sizeof(KEY)) {
            RtlCopyMemory(&key, &Vcb->superblock.sys_chunk_array[Vcb->superblock.n - n], sizeof(KEY));
            n -= sizeof(KEY);
        } else
            return STATUS_SUCCESS;
        
        TRACE("bootstrap: %llx,%x,%llx\n", key.obj_id, key.obj_type, key.offset);
        
        if (key.obj_type == TYPE_CHUNK_ITEM) {
            CHUNK_ITEM* ci;
            ULONG cisize;
            sys_chunk* sc;
            
            if (n < sizeof(CHUNK_ITEM))
                return STATUS_SUCCESS;
            
            ci = (CHUNK_ITEM*)&Vcb->superblock.sys_chunk_array[Vcb->superblock.n - n];
            cisize = sizeof(CHUNK_ITEM) + (ci->num_stripes * sizeof(CHUNK_ITEM_STRIPE));
            
            if (n < cisize)
                return STATUS_SUCCESS;
            
            sc = ExAllocatePoolWithTag(PagedPool, sizeof(sys_chunk), ALLOC_TAG);
            
            if (!sc) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sc->key = key;
            sc->size = cisize;
            sc->data = ExAllocatePoolWithTag(PagedPool, sc->size, ALLOC_TAG);
            
            if (!sc->data) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(sc->data, ci, sc->size);
            InsertTailList(&Vcb->sys_chunks, &sc->list_entry);
            
            n -= cisize;
        } else {
            ERR("unexpected item %llx,%x,%llx in bootstrap\n", key.obj_id, key.obj_type, key.offset);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

static root* find_default_subvol(device_extension* Vcb) {
    root* subvol;
    UINT64 inode;
    UINT8 type;
    UNICODE_STRING filename;
    LIST_ENTRY* le;
    
    static WCHAR fn[] = L"default";
    static UINT32 crc32 = 0x8dbfc2d2;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_DEFAULT_SUBVOL) {
        filename.Buffer = fn;
        filename.Length = filename.MaximumLength = (USHORT)wcslen(fn) * sizeof(WCHAR);
        
        if (!find_file_in_dir_with_crc32(Vcb, &filename, crc32, Vcb->root_root, Vcb->superblock.root_dir_objectid, &subvol, &inode, &type, NULL))
            WARN("couldn't find default subvol DIR_ITEM, using default tree\n");
        else
            return subvol;
    }
    
    le = Vcb->roots.Flink;
    while (le != &Vcb->roots) {
        root* r = CONTAINING_RECORD(le, root, list_entry);
        
        if (r->id == BTRFS_ROOT_FSTREE)
            return r;
        
        le = le->Flink;
    }
    
    return NULL;
}

static BOOL is_device_removable(PDEVICE_OBJECT devobj) {
    NTSTATUS Status;
    STORAGE_HOTPLUG_INFO shi;
    
    Status = dev_ioctl(devobj, IOCTL_STORAGE_GET_HOTPLUG_INFO, NULL, 0, &shi, sizeof(STORAGE_HOTPLUG_INFO), TRUE, NULL);
    
    if (!NT_SUCCESS(Status)) {
        ERR("dev_ioctl returned %08x\n", Status);
        return FALSE;
    }
    
    return shi.MediaRemovable != 0 ? TRUE : FALSE;
}

static ULONG get_device_change_count(PDEVICE_OBJECT devobj) {
    NTSTATUS Status;
    ULONG cc;
    IO_STATUS_BLOCK iosb;
    
    Status = dev_ioctl(devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), TRUE, &iosb);
    
    if (!NT_SUCCESS(Status)) {
        ERR("dev_ioctl returned %08x\n", Status);
        return 0;
    }
    
    if (iosb.Information < sizeof(ULONG)) {
        ERR("iosb.Information was too short\n");
        return 0;
    }
    
    return cc;
}

static NTSTATUS STDCALL mount_vol(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT NewDeviceObject = NULL;
    PDEVICE_OBJECT DeviceToMount;
    NTSTATUS Status;
    device_extension* Vcb = NULL;
    PARTITION_INFORMATION_EX piex;
    UINT64 i;
    LIST_ENTRY* le;
    KEY searchkey;
    traverse_ptr tp;
    fcb* root_fcb = NULL;
    
    TRACE("mount_vol called\n");
    
    if (DeviceObject != devobj)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;

//     Status = NtfsHasFileSystem(DeviceToMount);
//     if (!NT_SUCCESS(Status))
//     {
//         goto ByeBye;
//     }

    Status = dev_ioctl(DeviceToMount, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0,
                       &piex, sizeof(piex), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("error reading partition information: %08x\n", Status);
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }

    Status = IoCreateDevice(drvobj,
                            sizeof(device_extension),
                            NULL,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &NewDeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08x\n", Status);
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }
    
//     TRACE("DEV_ITEM = %x, superblock = %x\n", sizeof(DEV_ITEM), sizeof(superblock));

    NewDeviceObject->Flags |= DO_DIRECT_IO;
    Vcb = (PVOID)NewDeviceObject->DeviceExtension;
    RtlZeroMemory(Vcb, sizeof(device_extension));
    
    ExInitializeResourceLite(&Vcb->tree_lock);
    Vcb->tree_lock_counter = 0;
    Vcb->open_trees = 0;
    Vcb->write_trees = 0;

    ExInitializeResourceLite(&Vcb->fcb_lock);
    ExInitializeResourceLite(&Vcb->DirResource);

    ExAcquireResourceExclusiveLite(&global_loading_lock, TRUE);
    InsertTailList(&VcbList, &Vcb->list_entry);
    ExReleaseResourceLite(&global_loading_lock);

    ExInitializeResourceLite(&Vcb->load_lock);
    ExAcquireResourceExclusiveLite(&Vcb->load_lock, TRUE);

//     Vcb->Identifier.Type = NTFS_TYPE_VCB;
//     Vcb->Identifier.Size = sizeof(NTFS_TYPE_VCB);
// 
//     Status = NtfsGetVolumeData(DeviceToMount,
//                                Vcb);
//     if (!NT_SUCCESS(Status))
//         goto ByeBye;
    
//     Vcb->device = DeviceToMount;
    DeviceToMount->Flags |= DO_DIRECT_IO;
    
//     Status = dev_ioctl(DeviceToMount, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
//                        &Vcb->geometry, sizeof(DISK_GEOMETRY), TRUE);
//     if (!NT_SUCCESS(Status)) {
//         ERR("error reading disk geometry: %08x\n", Status);
//         goto exit;
//     } else {
//         TRACE("media type = %u, cylinders = %u, tracks per cylinder = %u, sectors per track = %u, bytes per sector = %u\n",
//                       Vcb->geometry.MediaType, Vcb->geometry.Cylinders, Vcb->geometry.TracksPerCylinder,
//                       Vcb->geometry.SectorsPerTrack, Vcb->geometry.BytesPerSector);
//     }
    
    Vcb->length = piex.PartitionLength.QuadPart;
    TRACE("partition length = %u\n", piex.PartitionLength);

    Status = read_superblock(Vcb, DeviceToMount);
    if (!NT_SUCCESS(Status)) {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }

    if (Vcb->superblock.magic != BTRFS_MAGIC) {
        ERR("not a BTRFS volume\n");
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    } else {
        TRACE("btrfs magic found\n");
    }

    if (Vcb->superblock.incompat_flags & ~INCOMPAT_SUPPORTED) {
        WARN("cannot mount because of unsupported incompat flags (%llx)\n", Vcb->superblock.incompat_flags & ~INCOMPAT_SUPPORTED);
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }
    
    le = volumes.Flink;
    while (le != &volumes) {
        volume* v = CONTAINING_RECORD(le, volume, list_entry);
        
        if (RtlCompareMemory(&Vcb->superblock.uuid, &v->fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID) && v->devnum < Vcb->superblock.dev_item.dev_id) {
            // skipping over device in RAID which isn't the first one
            // FIXME - hide this in My Computer
            Status = STATUS_UNRECOGNIZED_VOLUME;
            goto exit;
        }
        
        le = le->Flink;
    }
    
    // FIXME - remove this when we can
    if (Vcb->superblock.num_devices > 1) {
        WARN("not mounting - multiple devices not yet implemented\n");
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }
    
    Vcb->readonly = FALSE;
    if (Vcb->superblock.compat_ro_flags & ~COMPAT_RO_SUPPORTED) {
        WARN("mounting read-only because of unsupported flags (%llx)\n", Vcb->superblock.compat_ro_flags & ~COMPAT_RO_SUPPORTED);
        Vcb->readonly = TRUE;
    }
    
    Vcb->superblock.generation++;
    Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF;
    
    Vcb->devices = ExAllocatePoolWithTag(PagedPool, sizeof(device) * Vcb->superblock.num_devices, ALLOC_TAG);
    if (!Vcb->devices) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    Vcb->devices[0].devobj = DeviceToMount;
    RtlCopyMemory(&Vcb->devices[0].devitem, &Vcb->superblock.dev_item, sizeof(DEV_ITEM));
    Vcb->devices[0].removable = is_device_removable(Vcb->devices[0].devobj);
    Vcb->devices[0].change_count = Vcb->devices[0].removable ? get_device_change_count(Vcb->devices[0].devobj) : 0;
    
    if (Vcb->superblock.num_devices > 1)
        RtlZeroMemory(&Vcb->devices[1], sizeof(DEV_ITEM) * (Vcb->superblock.num_devices - 1));
    
    // FIXME - find other devices, if there are any
    
    TRACE("DeviceToMount = %p\n", DeviceToMount);
    TRACE("Stack->Parameters.MountVolume.Vpb = %p\n", Stack->Parameters.MountVolume.Vpb);

    NewDeviceObject->StackSize = DeviceToMount->StackSize + 1;
    NewDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
//     find_chunk_root(Vcb);
//     Vcb->chunk_root_phys_addr = Vcb->superblock.chunk_tree_addr; // FIXME - map from logical to physical (bootstrapped)
    
//     Vcb->root_tree_phys_addr = logical_to_physical(Vcb, Vcb->superblock.root_tree_addr);
    
    InitializeListHead(&Vcb->roots);
    InitializeListHead(&Vcb->drop_roots);
    
    Vcb->log_to_phys_loaded = FALSE;
    
    Vcb->max_inline = Vcb->superblock.node_size / 2;
    
//     Vcb->write_trees = NULL;

    add_root(Vcb, BTRFS_ROOT_CHUNK, Vcb->superblock.chunk_tree_addr, NULL);
    
    if (!Vcb->chunk_root) {
        ERR("Could not load chunk root.\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    InitializeListHead(&Vcb->sys_chunks);
    Status = load_sys_chunks(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("load_sys_chunks returned %08x\n", Status);
        goto exit;
    }
    
    InitializeListHead(&Vcb->chunks);
    InitializeListHead(&Vcb->trees);
    
    InitializeListHead(&Vcb->DirNotifyList);

    FsRtlNotifyInitializeSync(&Vcb->NotifySync);
    
    Status = load_chunk_root(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("load_chunk_root returned %08x\n", Status);
        goto exit;
    }
    
    add_root(Vcb, BTRFS_ROOT_ROOT, Vcb->superblock.root_tree_addr, NULL);
    
    if (!Vcb->root_root) {
        ERR("Could not load root of roots.\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    Status = look_for_roots(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("look_for_roots returned %08x\n", Status);
        goto exit;
    }
    
    Status = find_chunk_usage(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("find_chunk_usage returned %08x\n", Status);
        goto exit;
    }
        
    // We've already increased the generation by one
    if (!Vcb->readonly && Vcb->superblock.generation - 1 != Vcb->superblock.cache_generation) {
        WARN("generation was %llx, free-space cache generation was %llx; clearing cache...\n", Vcb->superblock.generation - 1, Vcb->superblock.cache_generation);
        Status = clear_free_space_cache(Vcb);
        if (!NT_SUCCESS(Status)) {
            ERR("clear_free_space_cache returned %08x\n", Status);
            goto exit;
        }
    }
    
    Vcb->volume_fcb = create_fcb();
    if (!Vcb->volume_fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    Vcb->volume_fcb->Vcb = Vcb;
    Vcb->volume_fcb->sd = NULL;
    
    root_fcb = create_fcb();
    if (!root_fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    root_fcb->Vcb = Vcb;
    root_fcb->inode = SUBVOL_ROOT_INODE;
    root_fcb->type = BTRFS_TYPE_DIRECTORY;
    
#ifdef DEBUG_FCB_REFCOUNTS
    WARN("volume FCB = %p\n", Vcb->volume_fcb);
    WARN("root FCB = %p\n", root_fcb);
#endif
    
    root_fcb->subvol = find_default_subvol(Vcb);

    if (!root_fcb->subvol) {
        ERR("could not find top subvol\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    searchkey.obj_id = root_fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, root_fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto exit;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("couldn't find INODE_ITEM for root directory\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    if (tp.item->size > 0)
        RtlCopyMemory(&root_fcb->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));
    
    fcb_get_sd(root_fcb, NULL);
    
    root_fcb->atts = get_file_attributes(Vcb, &root_fcb->inode_item, root_fcb->subvol, root_fcb->inode, root_fcb->type, FALSE, FALSE);
    
    Vcb->root_fileref = create_fileref();
    if (!Vcb->root_fileref) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    Vcb->root_fileref->fcb = root_fcb;
    InsertTailList(&root_fcb->subvol->fcbs, &root_fcb->list_entry);
    
    Vcb->root_fileref->full_filename.Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(WCHAR), ALLOC_TAG);
    
    if (!Vcb->root_fileref->full_filename.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    
    Vcb->root_fileref->full_filename.Buffer[0] = '\\';
    Vcb->root_fileref->full_filename.Length = Vcb->root_fileref->full_filename.MaximumLength = sizeof(WCHAR);

    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        Status = find_disk_holes(Vcb, &Vcb->devices[i]);
        if (!NT_SUCCESS(Status)) {
            ERR("find_disk_holes returned %08x\n", Status);
            goto exit;
        }
    }
    
//     root_test(Vcb);
    
//     Vcb->StreamFileObject = IoCreateStreamFileObject(NULL,
//                                                      Vcb->StorageDevice);
// 
//     InitializeListHead(&Vcb->FcbListHead);
// 
//     Fcb = NtfsCreateFCB(NULL, Vcb);
//     if (Fcb == NULL)
//     {
//         Status = STATUS_INSUFFICIENT_RESOURCES;
//         goto ByeBye;
//     }
// 
//     Ccb = ExAllocatePoolWithTag(NonPagedPool,
//                                 sizeof(NTFS_CCB),
//                                 TAG_CCB);
//     if (Ccb == NULL)
//     {
//         Status =  STATUS_INSUFFICIENT_RESOURCES;
//         goto ByeBye;
//     }
// 
//     RtlZeroMemory(Ccb, sizeof(NTFS_CCB));
// 
//     Ccb->Identifier.Type = NTFS_TYPE_CCB;
//     Ccb->Identifier.Size = sizeof(NTFS_TYPE_CCB);
// 
//     Vcb->StreamFileObject->FsContext = Fcb;
//     Vcb->StreamFileObject->FsContext2 = Ccb;
//     Vcb->StreamFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
//     Vcb->StreamFileObject->PrivateCacheMap = NULL;
//     Vcb->StreamFileObject->Vpb = Vcb->Vpb;
//     Ccb->PtrFileObject = Vcb->StreamFileObject;
//     Fcb->FileObject = Vcb->StreamFileObject;
//     Fcb->Vcb = (PDEVICE_EXTENSION)Vcb->StorageDevice;
// 
//     Fcb->Flags = FCB_IS_VOLUME_STREAM;
// 
//     Fcb->RFCB.FileSize.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector;
//     Fcb->RFCB.ValidDataLength.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector;
//     Fcb->RFCB.AllocationSize.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector; /* Correct? */
// 
//     CcInitializeCacheMap(Vcb->StreamFileObject,
//                          (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
//                          FALSE,
//                          &(NtfsGlobalData->CacheMgrCallbacks),
//                          Fcb);
// 
//     ExInitializeResourceLite(&Vcb->LogToPhysLock);

    KeInitializeSpinLock(&Vcb->FcbListLock);
// 
//     /* Get serial number */
//     NewDeviceObject->Vpb->SerialNumber = Vcb->NtfsInfo.SerialNumber;
// 
//     /* Get volume label */
//     NewDeviceObject->Vpb->VolumeLabelLength = Vcb->NtfsInfo.VolumeLabelLength;
//     RtlCopyMemory(NewDeviceObject->Vpb->VolumeLabel,
//                   Vcb->NtfsInfo.VolumeLabel,
//                   Vcb->NtfsInfo.VolumeLabelLength);

    NewDeviceObject->Vpb = Stack->Parameters.MountVolume.Vpb;
    Stack->Parameters.MountVolume.Vpb->DeviceObject = NewDeviceObject;
    Stack->Parameters.MountVolume.Vpb->RealDevice = DeviceToMount;
    Stack->Parameters.MountVolume.Vpb->Flags |= VPB_MOUNTED;
    NewDeviceObject->Vpb->VolumeLabelLength = 4; // FIXME
    NewDeviceObject->Vpb->VolumeLabel[0] = '?';
    NewDeviceObject->Vpb->VolumeLabel[1] = 0;
    NewDeviceObject->Vpb->ReferenceCount++; // FIXME - should we deref this at any point?
    
    Status = PsCreateSystemThread(&Vcb->flush_thread_handle, 0, NULL, NULL, NULL, flush_thread, NewDeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        goto exit;
    }
    
    Status = STATUS_SUCCESS;

exit:
//     if (!NT_SUCCESS(Status))
//     {
//         /* Cleanup */
//         if (Vcb && Vcb->StreamFileObject)
//             ObDereferenceObject(Vcb->StreamFileObject);
// 
//         if (Fcb)
//             ExFreePool(Fcb);
// 
//         if (Ccb)
//             ExFreePool(Ccb);
// 
//         if (NewDeviceObject)
//             IoDeleteDevice(NewDeviceObject);
//     }

    if (Vcb) {
        ExReleaseResourceLite(&Vcb->load_lock);
    }

    if (!NT_SUCCESS(Status)) {
        if (Vcb) {
            if (Vcb->root_fileref)
                free_fileref(Vcb->root_fileref);
            else if (root_fcb)
                free_fcb(root_fcb);

            if (Vcb->volume_fcb)
                free_fcb(Vcb->volume_fcb);

            ExDeleteResourceLite(&Vcb->tree_lock);
            ExDeleteResourceLite(&Vcb->load_lock);
            ExDeleteResourceLite(&Vcb->fcb_lock);
            ExDeleteResourceLite(&Vcb->DirResource);

            if (Vcb->devices)
                ExFreePoolWithTag(Vcb->devices, ALLOC_TAG);

            RemoveEntryList(&Vcb->list_entry);
        }

        if (NewDeviceObject)
            IoDeleteDevice(NewDeviceObject);
    }

    TRACE("mount_vol done (status: %lx)\n", Status);

    return Status;
}

static NTSTATUS STDCALL drv_file_system_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS status;
    BOOL top_level;

    TRACE("file system control\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    status = STATUS_NOT_IMPLEMENTED;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    Irp->IoStatus.Information = 0;
    
    switch (IrpSp->MinorFunction) {
        case IRP_MN_MOUNT_VOLUME:
            TRACE("IRP_MN_MOUNT_VOLUME\n");
            
//             Irp->IoStatus.Status = STATUS_SUCCESS;
            status = mount_vol(DeviceObject, Irp);
//             IrpSp->Parameters.MountVolume.DeviceObject = 0x0badc0de;
//             IrpSp->Parameters.MountVolume.Vpb = 0xdeadbeef;
            
//             IoCompleteRequest( Irp, IO_DISK_INCREMENT );
            
//             return Irp->IoStatus.Status;
            break;
            
        case IRP_MN_KERNEL_CALL:
            TRACE("IRP_MN_KERNEL_CALL\n");
            
            status = fsctl_request(DeviceObject, Irp, IrpSp->Parameters.FileSystemControl.FsControlCode, FALSE);
            break;
            
        case IRP_MN_USER_FS_REQUEST:
            TRACE("IRP_MN_USER_FS_REQUEST\n");
            
            status = fsctl_request(DeviceObject, Irp, IrpSp->Parameters.FileSystemControl.FsControlCode, TRUE);
            break;
            
        case IRP_MN_VERIFY_VOLUME:
            FIXME("STUB: IRP_MN_VERIFY_VOLUME\n");
            break;
           
        default:
            break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return status;
}

static NTSTATUS STDCALL drv_lock_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb* fcb = IrpSp->FileObject->FsContext;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    TRACE("lock control\n");
    
    Status = FsRtlProcessFileLock(&fcb->lock, Irp, NULL);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    return Status;
}

static NTSTATUS STDCALL drv_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    fcb* fcb;
    BOOL top_level;

    FIXME("STUB: device control\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    Irp->IoStatus.Information = 0;
    
    WARN("control code = %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
    
    if (!FileObject) {
        ERR("FileObject was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        
    }
    
    fcb = FileObject->FsContext;
    
    if (!fcb) {
        ERR("FCB was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb == Vcb->volume_fcb) {
        FIXME("FIXME - pass through\n");
        Status = STATUS_NOT_IMPLEMENTED;
    } else {
        TRACE("filename = %S\n", file_desc(FileObject));
        
        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
                TRACE("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n");
                Status = STATUS_INVALID_PARAMETER;
                break;

            default:
                WARN("unknown control code %x (DeviceType = %x, Access = %x, Function = %x, Method = %x)\n",
                                        IrpSp->Parameters.DeviceIoControl.IoControlCode, (IrpSp->Parameters.DeviceIoControl.IoControlCode & 0xff0000) >> 16,
                                        (IrpSp->Parameters.DeviceIoControl.IoControlCode & 0xc000) >> 14, (IrpSp->Parameters.DeviceIoControl.IoControlCode & 0x3ffc) >> 2,
                                        IrpSp->Parameters.DeviceIoControl.IoControlCode & 0x3);
                Status = STATUS_INVALID_PARAMETER;
                break;
        }
    }
    
end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS STDCALL drv_shutdown(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    BOOL top_level;

    ERR("shutdown\n");
    
    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    Status = STATUS_SUCCESS;

    while (!IsListEmpty(&VcbList)) {
        LIST_ENTRY* le = RemoveHeadList(&VcbList);
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);
        
        TRACE("shutting down Vcb %p\n", Vcb);
        
        uninit(Vcb, TRUE);
    }
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}

BOOL is_file_name_valid(PUNICODE_STRING us) {
    ULONG i;
    
    if (us->Length < sizeof(WCHAR))
        return FALSE;
    
    if (us->Length > 255 * sizeof(WCHAR))
        return FALSE;
    
    for (i = 0; i < us->Length / sizeof(WCHAR); i++) {
        if (us->Buffer[i] == '/' || us->Buffer[i] == '<' || us->Buffer[i] == '>' || us->Buffer[i] == ':' || us->Buffer[i] == '"' ||
            us->Buffer[i] == '|' || us->Buffer[i] == '?' || us->Buffer[i] == '*' || (us->Buffer[i] >= 1 && us->Buffer[i] <= 31))
            return FALSE;
    }
    
    if (us->Buffer[0] == '.' && (us->Length == sizeof(WCHAR) || (us->Length == 2 * sizeof(WCHAR) && us->Buffer[1] == '.')))
        return FALSE;
    
    return TRUE;
}

#ifdef _DEBUG
static void STDCALL init_serial() {
    NTSTATUS Status;
    
    Status = IoGetDeviceObjectPointer(&log_device, FILE_WRITE_DATA, &comfo, &comdo);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
    }
}
#endif

#ifndef __REACTOS__
static void STDCALL check_cpu() {
    unsigned int cpuInfo[4];
#ifndef _MSC_VER
    __get_cpuid(1, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);
    have_sse42 = cpuInfo[2] & bit_SSE4_2;
#else
   __cpuid(cpuInfo, 1);
   have_sse42 = cpuInfo[2] & (1 << 20);
#endif

    if (have_sse42)
        TRACE("SSE4.2 is supported\n");
    else
        TRACE("SSE4.2 not supported\n");
}
#endif

static void STDCALL read_registry(PUNICODE_STRING regpath) {
    HANDLE h;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    ULONG dispos;
    NTSTATUS Status;
    WCHAR* path;
    ULONG kvfilen, retlen, i;
    KEY_VALUE_FULL_INFORMATION* kvfi;
    
    const WCHAR mappings[] = L"\\Mappings";
#ifndef __REACTOS__
    static WCHAR def_log_file[] = L"\\??\\C:\\btrfs.log";
#endif
    
    path = ExAllocatePoolWithTag(PagedPool, regpath->Length + (wcslen(mappings) * sizeof(WCHAR)), ALLOC_TAG);
    if (!path) {
        ERR("out of memory\n");
        return;
    }
    
    RtlCopyMemory(path, regpath->Buffer, regpath->Length);
    RtlCopyMemory((UINT8*)path + regpath->Length, mappings, wcslen(mappings) * sizeof(WCHAR));
    
    us.Buffer = path;
    us.Length = us.MaximumLength = regpath->Length + ((USHORT)wcslen(mappings) * sizeof(WCHAR));
    
    InitializeObjectAttributes(&oa, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    // FIXME - keep open and do notify for changes
    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);
    
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08x\n", Status);
        ExFreePool(path);
        return;
    }

    if (dispos == REG_OPENED_EXISTING_KEY) {
        kvfilen = sizeof(KEY_VALUE_FULL_INFORMATION) + 256;
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
        
        if (!kvfi) {
            ERR("out of memory\n");
            ExFreePool(path);
            ZwClose(h);
            return;
        }
        
        i = 0;
        do {
            Status = ZwEnumerateValueKey(h, i, KeyValueFullInformation, kvfi, kvfilen, &retlen);
            
            if (NT_SUCCESS(Status) && kvfi->DataLength > 0) {
                UINT32 val = 0;
                
                RtlCopyMemory(&val, (UINT8*)kvfi + kvfi->DataOffset, min(kvfi->DataLength, sizeof(UINT32)));
                
                TRACE("entry %u = %.*S = %u\n", i, kvfi->NameLength / sizeof(WCHAR), kvfi->Name, val);
                
                add_user_mapping(kvfi->Name, kvfi->NameLength / sizeof(WCHAR), val);
            }
            
            i = i + 1;
        } while (Status != STATUS_NO_MORE_ENTRIES);
    }
    
    ZwClose(h);

    ExFreePool(path);
    
#ifdef _DEBUG
    InitializeObjectAttributes(&oa, regpath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);
    
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08x\n", Status);
        return;
    }
    
    RtlInitUnicodeString(&us, L"DebugLogLevel");
    
    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
    
    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
        
        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }
        
        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
        
        if (NT_SUCCESS(Status)) {
            if (kvfi->Type == REG_DWORD && kvfi->DataLength >= sizeof(UINT32)) {
                RtlCopyMemory(&debug_log_level, ((UINT8*)kvfi) + kvfi->DataOffset, sizeof(UINT32));
            } else {
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }

                Status = ZwSetValueKey(h, &us, 0, REG_DWORD, &debug_log_level, sizeof(debug_log_level));
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwSetValueKey reutrned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, REG_DWORD, &debug_log_level, sizeof(debug_log_level));
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey reutrned %08x\n", Status);
        }
    } else {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
    
    RtlInitUnicodeString(&us, L"LogDevice");
    
    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
    
    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
        
        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }
        
        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
        
        if (NT_SUCCESS(Status)) {
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_device.Length = log_device.MaximumLength = kvfi->DataLength;
                log_device.Buffer = ExAllocatePoolWithTag(PagedPool, kvfi->DataLength, ALLOC_TAG);
                
                if (!log_device.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_device.Buffer, ((UINT8*)kvfi) + kvfi->DataOffset, kvfi->DataLength);
                
                if (log_device.Buffer[(log_device.Length / sizeof(WCHAR)) - 1] == 0)
                    log_device.Length -= sizeof(WCHAR);
            } else {
                ERR("LogDevice was type %u, length %u\n", kvfi->Type, kvfi->DataLength);
                
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
    
    RtlInitUnicodeString(&us, L"LogFile");
    
    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
    
    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
        
        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }
        
        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);
        
        if (NT_SUCCESS(Status)) {
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_file.Length = log_file.MaximumLength = kvfi->DataLength;
                log_file.Buffer = ExAllocatePoolWithTag(PagedPool, kvfi->DataLength, ALLOC_TAG);
                
                if (!log_file.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_file.Buffer, ((UINT8*)kvfi) + kvfi->DataOffset, kvfi->DataLength);
                
                if (log_file.Buffer[(log_file.Length / sizeof(WCHAR)) - 1] == 0)
                    log_file.Length -= sizeof(WCHAR);
            } else {
                ERR("LogFile was type %u, length %u\n", kvfi->Type, kvfi->DataLength);
                
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, REG_SZ, def_log_file, (wcslen(def_log_file) + 1) * sizeof(WCHAR));
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08x\n", Status);
        }
    } else {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
    
    if (log_file.Length == 0) {
        log_file.Length = log_file.MaximumLength = wcslen(def_log_file) * sizeof(WCHAR);
        log_file.Buffer = ExAllocatePoolWithTag(PagedPool, log_file.MaximumLength, ALLOC_TAG);
        
        if (!log_file.Buffer) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }
        
        RtlCopyMemory(log_file.Buffer, def_log_file, log_file.Length);
    }
    
    ZwClose(h);
#endif
}

#ifdef _DEBUG
static void init_logging() {
    if (log_device.Length > 0)
        init_serial();
    else if (log_file.Length > 0) {
        NTSTATUS Status;
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK iosb;
        char* dateline;
        LARGE_INTEGER time;
        TIME_FIELDS tf;
        
        InitializeObjectAttributes(&oa, &log_file, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
        
        Status = ZwCreateFile(&log_handle, FILE_WRITE_DATA, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                              FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_ALERT, NULL, 0);
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwCreateFile returned %08x\n", Status);
            return;
        }
        
        if (iosb.Information == FILE_OPENED) { // already exists
            FILE_STANDARD_INFORMATION fsi;
            FILE_POSITION_INFORMATION fpi;
            
            static char delim[] = "\n---\n";
            
            // move to end of file
            
            Status = ZwQueryInformationFile(log_handle, &iosb, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
            
            if (!NT_SUCCESS(Status)) {
                ERR("ZwQueryInformationFile returned %08x\n", Status);
                return;
            }
            
            fpi.CurrentByteOffset = fsi.EndOfFile;
            
            Status = ZwSetInformationFile(log_handle, &iosb, &fpi, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation);
            
            if (!NT_SUCCESS(Status)) {
                ERR("ZwSetInformationFile returned %08x\n", Status);
                return;
            }

            Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, delim, strlen(delim), NULL, NULL);
        
            if (!NT_SUCCESS(Status)) {
                ERR("ZwWriteFile returned %08x\n", Status);
                return;
            }
        }
        
        dateline = ExAllocatePoolWithTag(PagedPool, 256, ALLOC_TAG);
        
        if (!dateline) {
            ERR("out of memory\n");
            return;
        }
        
        KeQuerySystemTime(&time);
        
        RtlTimeToTimeFields(&time, &tf);
        
        sprintf(dateline, "Starting logging at %04u-%02u-%02u %02u:%02u:%02u\n", tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second);

        Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, dateline, strlen(dateline), NULL, NULL);
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwWriteFile returned %08x\n", Status);
            return;
        }
        
        ExFreePool(dateline);
    }
}
#endif

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING device_nameW;
    UNICODE_STRING dosdevice_nameW;
    
    InitializeListHead(&uid_map_list);
    
    log_device.Buffer = NULL;
    log_device.Length = log_device.MaximumLength = 0;
    log_file.Buffer = NULL;
    log_file.Length = log_file.MaximumLength = 0;
    
    read_registry(RegistryPath);
    
#ifdef _DEBUG
    if (debug_log_level > 0)
        init_logging();
    
    log_started = TRUE;
#endif

    TRACE("DriverEntry\n");
   
#ifndef __REACTOS__
    check_cpu();
#endif
   
//    TRACE("check CRC32C: %08x\n", calc_crc32c((UINT8*)"123456789", 9)); // should be e3069283
   
    drvobj = DriverObject;

    DriverObject->DriverUnload = DriverUnload;

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)drv_create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)drv_close;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)drv_read;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)drv_write;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)drv_query_information;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)drv_set_information;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)drv_query_ea;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)drv_set_ea;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)drv_flush_buffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)drv_query_volume_information;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)drv_set_volume_information;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)drv_cleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)drv_directory_control;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)drv_file_system_control;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)drv_lock_control;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)drv_device_control;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)drv_shutdown;
    DriverObject->MajorFunction[IRP_MJ_PNP]                      = (PDRIVER_DISPATCH)drv_pnp;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]           = (PDRIVER_DISPATCH)drv_query_security;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]             = (PDRIVER_DISPATCH)drv_set_security;

    init_fast_io_dispatch(&DriverObject->FastIoDispatch);

    device_nameW.Buffer = device_name;
    device_nameW.Length = device_nameW.MaximumLength = (USHORT)wcslen(device_name) * sizeof(WCHAR);
    dosdevice_nameW.Buffer = dosdevice_name;
    dosdevice_nameW.Length = dosdevice_nameW.MaximumLength = (USHORT)wcslen(dosdevice_name) * sizeof(WCHAR);

    Status = IoCreateDevice(DriverObject, 0, &device_nameW, FILE_DEVICE_DISK_FILE_SYSTEM, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08x\n", Status);
        return Status;
    }
    
    devobj = DeviceObject;
    
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoCreateSymbolicLink(&dosdevice_nameW, &device_nameW);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateSymbolicLink returned %08x\n", Status);
        return Status;
    }
    
    Status = init_cache();
    if (!NT_SUCCESS(Status)) {
        ERR("init_cache returned %08x\n", Status);
        return Status;
    }

    InitializeListHead(&volumes);
    look_for_vols(&volumes);
    
    InitializeListHead(&VcbList);
    ExInitializeResourceLite(&global_loading_lock);
    
    IoRegisterFileSystem(DeviceObject);

    return STATUS_SUCCESS;
}
