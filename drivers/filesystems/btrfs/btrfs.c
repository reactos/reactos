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

#ifdef _DEBUG
#define DEBUG
#endif

#include "btrfs_drv.h"
#include "xxhash.h"
#include "crc32c.h"
#ifndef __REACTOS__
#ifndef _MSC_VER
#include <cpuid.h>
#else
#include <intrin.h>
#endif
#endif // __REACTOS__
#include <ntddscsi.h>
#include "btrfs.h"
#include <ata.h>

#ifndef _MSC_VER
#include <initguid.h>
#include <ntddstor.h>
#undef INITGUID
#endif

#include <ntdddisk.h>
#include <ntddvol.h>

#ifdef _MSC_VER
#include <initguid.h>
#include <ntddstor.h>
#undef INITGUID
#endif

#include <ntstrsafe.h>

#define INCOMPAT_SUPPORTED (BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF | BTRFS_INCOMPAT_FLAGS_DEFAULT_SUBVOL | BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS | \
                            BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO | BTRFS_INCOMPAT_FLAGS_BIG_METADATA | BTRFS_INCOMPAT_FLAGS_RAID56 | \
                            BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF | BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA | BTRFS_INCOMPAT_FLAGS_NO_HOLES | \
                            BTRFS_INCOMPAT_FLAGS_COMPRESS_ZSTD | BTRFS_INCOMPAT_FLAGS_METADATA_UUID | BTRFS_INCOMPAT_FLAGS_RAID1C34)
#define COMPAT_RO_SUPPORTED (BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE | BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE_VALID | \
                             BTRFS_COMPAT_RO_FLAGS_VERITY)

static const WCHAR device_name[] = {'\\','B','t','r','f','s',0};
static const WCHAR dosdevice_name[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','B','t','r','f','s',0};

DEFINE_GUID(BtrfsBusInterface, 0x4d414874, 0x6865, 0x6761, 0x6d, 0x65, 0x83, 0x69, 0x17, 0x9a, 0x7d, 0x1d);

PDRIVER_OBJECT drvobj;
PDEVICE_OBJECT master_devobj, busobj;
uint64_t num_reads = 0;
LIST_ENTRY uid_map_list, gid_map_list;
LIST_ENTRY VcbList;
ERESOURCE global_loading_lock;
uint32_t debug_log_level = 0;
uint32_t mount_compress = 0;
uint32_t mount_compress_force = 0;
uint32_t mount_compress_type = 0;
uint32_t mount_zlib_level = 3;
uint32_t mount_zstd_level = 3;
uint32_t mount_flush_interval = 30;
uint32_t mount_max_inline = 2048;
uint32_t mount_skip_balance = 0;
uint32_t mount_no_barrier = 0;
uint32_t mount_no_trim = 0;
uint32_t mount_clear_cache = 0;
uint32_t mount_allow_degraded = 0;
uint32_t mount_readonly = 0;
uint32_t mount_no_root_dir = 0;
uint32_t no_pnp = 0;
bool log_started = false;
UNICODE_STRING log_device, log_file, registry_path;
tPsUpdateDiskCounters fPsUpdateDiskCounters;
tCcCopyReadEx fCcCopyReadEx;
tCcCopyWriteEx fCcCopyWriteEx;
tCcSetAdditionalCacheAttributesEx fCcSetAdditionalCacheAttributesEx;
tFsRtlUpdateDiskCounters fFsRtlUpdateDiskCounters;
tIoUnregisterPlugPlayNotificationEx fIoUnregisterPlugPlayNotificationEx;
tFsRtlGetEcpListFromIrp fFsRtlGetEcpListFromIrp;
tFsRtlGetNextExtraCreateParameter fFsRtlGetNextExtraCreateParameter;
tFsRtlValidateReparsePointBuffer fFsRtlValidateReparsePointBuffer;
tFsRtlCheckLockForOplockRequest fFsRtlCheckLockForOplockRequest;
tFsRtlAreThereCurrentOrInProgressFileLocks fFsRtlAreThereCurrentOrInProgressFileLocks;
bool diskacc = false;
void *notification_entry = NULL, *notification_entry2 = NULL, *notification_entry3 = NULL;
ERESOURCE pdo_list_lock, mapping_lock;
LIST_ENTRY pdo_list;
bool finished_probing = false;
HANDLE degraded_wait_handle = NULL, mountmgr_thread_handle = NULL;
bool degraded_wait = true;
KEVENT mountmgr_thread_event;
bool shutting_down = false;
ERESOURCE boot_lock;
bool is_windows_8;
extern uint64_t boot_subvol;

#ifdef _DEBUG
PFILE_OBJECT comfo = NULL;
PDEVICE_OBJECT comdo = NULL;
HANDLE log_handle = NULL;
ERESOURCE log_lock;
HANDLE serial_thread_handle = NULL;

static void init_serial(bool first_time);
#endif

static NTSTATUS close_file(_In_ PFILE_OBJECT FileObject, _In_ PIRP Irp);
static void __stdcall do_xor_basic(uint8_t* buf1, uint8_t* buf2, uint32_t len);

xor_func do_xor = do_xor_basic;

typedef struct {
    KEVENT Event;
    IO_STATUS_BLOCK iosb;
} read_context;

// no longer in Windows headers??
extern BOOLEAN WdmlibRtlIsNtDdiVersionAvailable(ULONG Version);

#ifdef _DEBUG
_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall dbg_completion(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PVOID conptr) {
    read_context* context = conptr;

    UNUSED(DeviceObject);

    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

#define DEBUG_MESSAGE_LEN 1024

#ifdef DEBUG_LONG_MESSAGES
void _debug_message(_In_ const char* func, _In_ const char* file, _In_ unsigned int line, _In_ char* s, ...) {
#else
void _debug_message(_In_ const char* func, _In_ char* s, ...) {
#endif
    LARGE_INTEGER offset;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PIRP Irp;
    va_list ap;
    char *buf2, *buf;
    read_context context;
    uint32_t length;

    buf2 = ExAllocatePoolWithTag(NonPagedPool, DEBUG_MESSAGE_LEN, ALLOC_TAG);

    if (!buf2) {
        DbgPrint("Couldn't allocate buffer in debug_message\n");
        return;
    }

#ifdef DEBUG_LONG_MESSAGES
    sprintf(buf2, "%p:%s:%s:%u:", (void*)PsGetCurrentThread(), func, file, line);
#else
    sprintf(buf2, "%p:%s:", (void*)PsGetCurrentThread(), func);
#endif
    buf = &buf2[strlen(buf2)];

    va_start(ap, s);

    RtlStringCbVPrintfA(buf, DEBUG_MESSAGE_LEN - strlen(buf2), s, ap);

    ExAcquireResourceSharedLite(&log_lock, true);

    if (!log_started || (log_device.Length == 0 && log_file.Length == 0)) {
        DbgPrint(buf2);
    } else if (log_device.Length > 0) {
        if (!comdo) {
            DbgPrint(buf2);
            goto exit2;
        }

        length = (uint32_t)strlen(buf2);

        offset.u.LowPart = 0;
        offset.u.HighPart = 0;

        RtlZeroMemory(&context, sizeof(read_context));

        KeInitializeEvent(&context.Event, NotificationEvent, false);

        Irp = IoAllocateIrp(comdo->StackSize, false);

        if (!Irp) {
            DbgPrint("IoAllocateIrp failed\n");
            goto exit2;
        }

        IrpSp = IoGetNextIrpStackLocation(Irp);
        IrpSp->MajorFunction = IRP_MJ_WRITE;
        IrpSp->FileObject = comfo;

        if (comdo->Flags & DO_BUFFERED_IO) {
            Irp->AssociatedIrp.SystemBuffer = buf2;

            Irp->Flags = IRP_BUFFERED_IO;
        } else if (comdo->Flags & DO_DIRECT_IO) {
            Irp->MdlAddress = IoAllocateMdl(buf2, length, false, false, NULL);
            if (!Irp->MdlAddress) {
                DbgPrint("IoAllocateMdl failed\n");
                goto exit;
            }

            MmBuildMdlForNonPagedPool(Irp->MdlAddress);
        } else {
            Irp->UserBuffer = buf2;
        }

        IrpSp->Parameters.Write.Length = length;
        IrpSp->Parameters.Write.ByteOffset = offset;

        Irp->UserIosb = &context.iosb;

        Irp->UserEvent = &context.Event;

        IoSetCompletionRoutine(Irp, dbg_completion, &context, true, true, true);

        Status = IoCallDriver(comdo, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&context.Event, Executive, KernelMode, false, NULL);
            Status = context.iosb.Status;
        }

        if (comdo->Flags & DO_DIRECT_IO)
            IoFreeMdl(Irp->MdlAddress);

        if (!NT_SUCCESS(Status)) {
            DbgPrint("failed to write to COM1 - error %08lx\n", Status);
            goto exit;
        }

exit:
        IoFreeIrp(Irp);
    } else if (log_handle != NULL) {
        IO_STATUS_BLOCK iosb;

        length = (uint32_t)strlen(buf2);

        Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, buf2, length, NULL, NULL);

        if (!NT_SUCCESS(Status)) {
            DbgPrint("failed to write to file - error %08lx\n", Status);
        }
    }

exit2:
    ExReleaseResourceLite(&log_lock);

    va_end(ap);

    if (buf2)
        ExFreePool(buf2);
}
#endif

bool is_top_level(_In_ PIRP Irp) {
    if (!IoGetTopLevelIrp()) {
        IoSetTopLevelIrp(Irp);
        return true;
    }

    return false;
}

static void __stdcall do_xor_basic(uint8_t* buf1, uint8_t* buf2, uint32_t len) {
    uint32_t j;

#if defined(_ARM_) || defined(_ARM64_)
    uint64x2_t x1, x2;

    if (((uintptr_t)buf1 & 0xf) == 0 && ((uintptr_t)buf2 & 0xf) == 0) {
        while (len >= 16) {
            x1 = vld1q_u64((const uint64_t*)buf1);
            x2 = vld1q_u64((const uint64_t*)buf2);
            x1 = veorq_u64(x1, x2);
            vst1q_u64((uint64_t*)buf1, x1);

            buf1 += 16;
            buf2 += 16;
            len -= 16;
        }
    }
#endif

#if defined(_AMD64_) || defined(_ARM64_)
    while (len > 8) {
        *(uint64_t*)buf1 ^= *(uint64_t*)buf2;
        buf1 += 8;
        buf2 += 8;
        len -= 8;
    }
#endif

    while (len > 4) {
        *(uint32_t*)buf1 ^= *(uint32_t*)buf2;
        buf1 += 4;
        buf2 += 4;
        len -= 4;
    }

    for (j = 0; j < len; j++) {
        *buf1 ^= *buf2;
        buf1++;
        buf2++;
    }
}

_Function_class_(DRIVER_UNLOAD)
static void __stdcall DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING dosdevice_nameW;

    TRACE("(%p)\n", DriverObject);

    dosdevice_nameW.Buffer = (WCHAR*)dosdevice_name;
    dosdevice_nameW.Length = dosdevice_nameW.MaximumLength = sizeof(dosdevice_name) - sizeof(WCHAR);

    IoDeleteSymbolicLink(&dosdevice_nameW);
    IoDeleteDevice(DriverObject->DeviceObject);

    while (!IsListEmpty(&uid_map_list)) {
        LIST_ENTRY* le = RemoveHeadList(&uid_map_list);
        uid_map* um = CONTAINING_RECORD(le, uid_map, listentry);

        ExFreePool(um->sid);

        ExFreePool(um);
    }

    while (!IsListEmpty(&gid_map_list)) {
        gid_map* gm = CONTAINING_RECORD(RemoveHeadList(&gid_map_list), gid_map, listentry);

        ExFreePool(gm->sid);
        ExFreePool(gm);
    }

    // FIXME - free volumes and their devpaths

#ifdef _DEBUG
    if (comfo)
        ObDereferenceObject(comfo);

    if (log_handle)
        ZwClose(log_handle);
#endif

    ExDeleteResourceLite(&global_loading_lock);
    ExDeleteResourceLite(&pdo_list_lock);

    if (log_device.Buffer)
        ExFreePool(log_device.Buffer);

    if (log_file.Buffer)
        ExFreePool(log_file.Buffer);

    if (registry_path.Buffer)
        ExFreePool(registry_path.Buffer);

#ifdef _DEBUG
    ExDeleteResourceLite(&log_lock);
#endif
    ExDeleteResourceLite(&mapping_lock);
}

static bool get_last_inode(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_opt_ PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, prev_tp;
    NTSTATUS Status;

    // get last entry
    searchkey.obj_id = 0xffffffffffffffff;
    searchkey.obj_type = 0xff;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, r, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return false;
    }

    if ((tp.item->key.obj_type == TYPE_INODE_ITEM || tp.item->key.obj_type == TYPE_ROOT_ITEM) && tp.item->key.obj_id <= BTRFS_LAST_FREE_OBJECTID) {
        r->lastinode = tp.item->key.obj_id;
        TRACE("last inode for tree %I64x is %I64x\n", r->id, r->lastinode);
        return true;
    }

    while (find_prev_item(Vcb, &tp, &prev_tp, Irp)) {
        tp = prev_tp;

        TRACE("moving on to %I64x,%x,%I64x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

        if ((tp.item->key.obj_type == TYPE_INODE_ITEM || tp.item->key.obj_type == TYPE_ROOT_ITEM) && tp.item->key.obj_id <= BTRFS_LAST_FREE_OBJECTID) {
            r->lastinode = tp.item->key.obj_id;
            TRACE("last inode for tree %I64x is %I64x\n", r->id, r->lastinode);
            return true;
        }
    }

    r->lastinode = SUBVOL_ROOT_INODE;

    WARN("no INODE_ITEMs in tree %I64x\n", r->id);

    return true;
}

_Success_(return)
static bool extract_xattr(_In_reads_bytes_(size) void* item, _In_ USHORT size, _In_z_ char* name, _Out_ uint8_t** data, _Out_ uint16_t* datalen) {
    DIR_ITEM* xa = (DIR_ITEM*)item;
    USHORT xasize;

    while (true) {
        if (size < sizeof(DIR_ITEM) || size < (sizeof(DIR_ITEM) - 1 + xa->m + xa->n)) {
            WARN("DIR_ITEM is truncated\n");
            return false;
        }

        if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
            TRACE("found xattr %s\n", name);

            *datalen = xa->m;

            if (xa->m > 0) {
                *data = ExAllocatePoolWithTag(PagedPool, xa->m, ALLOC_TAG);
                if (!*data) {
                    ERR("out of memory\n");
                    return false;
                }

                RtlCopyMemory(*data, &xa->name[xa->n], xa->m);
            } else
                *data = NULL;

            return true;
        }

        xasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;

        if (size > xasize) {
            size -= xasize;
            xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
        } else
            break;
    }

    TRACE("xattr %s not found\n", name);

    return false;
}

_Success_(return)
bool get_xattr(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* subvol, _In_ uint64_t inode, _In_z_ char* name, _In_ uint32_t crc32,
               _Out_ uint8_t** data, _Out_ uint16_t* datalen, _In_opt_ PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    TRACE("(%p, %I64x, %I64x, %s, %08x, %p, %p)\n", Vcb, subvol->id, inode, name, crc32, data, datalen);

    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;

    Status = find_item(Vcb, subvol, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return false;
    }

    if (keycmp(tp.item->key, searchkey)) {
        TRACE("could not find item (%I64x,%x,%I64x)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return false;
    }

    if (tp.item->size < sizeof(DIR_ITEM)) {
        ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        return false;
    }

    return extract_xattr(tp.item->data, tp.item->size, name, data, datalen);
}

_Dispatch_type_(IRP_MJ_CLOSE)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_close(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("close\n");

    top_level = is_top_level(Irp);

    if (DeviceObject == master_devobj) {
        TRACE("Closing file system\n");
        Status = STATUS_SUCCESS;
        goto end;
    } else if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_close(DeviceObject, Irp);
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    // FIXME - call FsRtlNotifyUninitializeSync(&Vcb->NotifySync) if unmounting

    Status = close_file(IrpSp->FileObject, Irp);

end:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("returning %08lx\n", Status);

    FsRtlExitFileSystem();

    return Status;
}

_Dispatch_type_(IRP_MJ_FLUSH_BUFFERS)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_flush_buffers(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("flush buffers\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_SUCCESS;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_SUCCESS;
        goto end;
    }

    if (fcb == Vcb->volume_fcb) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

    Irp->IoStatus.Information = 0;

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Status = Status;

    if (fcb->type != BTRFS_TYPE_DIRECTORY) {
        CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &Irp->IoStatus);

        if (fcb->Header.PagingIoResource) {
            ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, true);
            ExReleaseResourceLite(fcb->Header.PagingIoResource);
        }

        Status = Irp->IoStatus.Status;
    }

end:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    TRACE("returning %08lx\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

static void calculate_total_space(_In_ device_extension* Vcb, _Out_ uint64_t* totalsize, _Out_ uint64_t* freespace) {
    uint64_t nfactor, dfactor, sectors_used;

    if (Vcb->data_flags & BLOCK_FLAG_DUPLICATE || Vcb->data_flags & BLOCK_FLAG_RAID1 || Vcb->data_flags & BLOCK_FLAG_RAID10) {
        nfactor = 1;
        dfactor = 2;
    } else if (Vcb->data_flags & BLOCK_FLAG_RAID5) {
        nfactor = Vcb->superblock.num_devices - 1;
        dfactor = Vcb->superblock.num_devices;
    } else if (Vcb->data_flags & BLOCK_FLAG_RAID6) {
        nfactor = Vcb->superblock.num_devices - 2;
        dfactor = Vcb->superblock.num_devices;
    } else if (Vcb->data_flags & BLOCK_FLAG_RAID1C3) {
        nfactor = 1;
        dfactor = 3;
    } else if (Vcb->data_flags & BLOCK_FLAG_RAID1C4) {
        nfactor = 1;
        dfactor = 4;
    } else {
        nfactor = 1;
        dfactor = 1;
    }

    sectors_used = (Vcb->superblock.bytes_used >> Vcb->sector_shift) * nfactor / dfactor;

    *totalsize = (Vcb->superblock.total_bytes >> Vcb->sector_shift) * nfactor / dfactor;
    *freespace = sectors_used > *totalsize ? 0 : (*totalsize - sectors_used);
}

#ifndef __REACTOS__
#define INIT_UNICODE_STRING(var, val) UNICODE_STRING us##var; us##var.Buffer = (WCHAR*)val; us##var.Length = us##var.MaximumLength = sizeof(val) - sizeof(WCHAR);

// This function exists because we have to lie about our FS type in certain situations.
// MPR!MprGetConnection queries the FS type, and compares it to a whitelist. If it doesn't match,
// it will return ERROR_NO_NET_OR_BAD_PATH, which prevents UAC from working.
// The command mklink refuses to create hard links on anything other than NTFS, so we have to
// blacklist cmd.exe too.

static bool lie_about_fs_type() {
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;
    PPEB peb;
    LIST_ENTRY* le;
    ULONG retlen;
#ifdef _AMD64_
    ULONG_PTR wow64info;
#endif

    INIT_UNICODE_STRING(mpr, L"MPR.DLL");
    INIT_UNICODE_STRING(cmd, L"CMD.EXE");
    INIT_UNICODE_STRING(fsutil, L"FSUTIL.EXE");
    INIT_UNICODE_STRING(storsvc, L"STORSVC.DLL");

    /* Not doing a Volkswagen, honest! Some IFS tests won't run if not recognized FS. */
    INIT_UNICODE_STRING(ifstest, L"IFSTEST.EXE");

    if (!PsGetCurrentProcess())
        return false;

#ifdef _AMD64_
    Status = ZwQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information, &wow64info, sizeof(wow64info), NULL);

    if (NT_SUCCESS(Status) && wow64info != 0)
        return true;
#endif

    Status = ZwQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &retlen);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwQueryInformationProcess returned %08lx\n", Status);
        return false;
    }

    if (!pbi.PebBaseAddress)
        return false;

    peb = pbi.PebBaseAddress;

    if (!peb->Ldr)
        return false;

    le = peb->Ldr->InMemoryOrderModuleList.Flink;
    while (le != &peb->Ldr->InMemoryOrderModuleList) {
        LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(le, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        bool blacklist = false;

        if (entry->FullDllName.Length >= usmpr.Length) {
            UNICODE_STRING name;

            name.Buffer = &entry->FullDllName.Buffer[(entry->FullDllName.Length - usmpr.Length) / sizeof(WCHAR)];
            name.Length = name.MaximumLength = usmpr.Length;

            blacklist = FsRtlAreNamesEqual(&name, &usmpr, true, NULL);
        }

        if (!blacklist && entry->FullDllName.Length >= uscmd.Length) {
            UNICODE_STRING name;

            name.Buffer = &entry->FullDllName.Buffer[(entry->FullDllName.Length - uscmd.Length) / sizeof(WCHAR)];
            name.Length = name.MaximumLength = uscmd.Length;

            blacklist = FsRtlAreNamesEqual(&name, &uscmd, true, NULL);
        }

        if (!blacklist && entry->FullDllName.Length >= usfsutil.Length) {
            UNICODE_STRING name;

            name.Buffer = &entry->FullDllName.Buffer[(entry->FullDllName.Length - usfsutil.Length) / sizeof(WCHAR)];
            name.Length = name.MaximumLength = usfsutil.Length;

            blacklist = FsRtlAreNamesEqual(&name, &usfsutil, true, NULL);
        }

        if (!blacklist && entry->FullDllName.Length >= usstorsvc.Length) {
            UNICODE_STRING name;

            name.Buffer = &entry->FullDllName.Buffer[(entry->FullDllName.Length - usstorsvc.Length) / sizeof(WCHAR)];
            name.Length = name.MaximumLength = usstorsvc.Length;

            blacklist = FsRtlAreNamesEqual(&name, &usstorsvc, true, NULL);
        }

        if (!blacklist && entry->FullDllName.Length >= usifstest.Length) {
            UNICODE_STRING name;

            name.Buffer = &entry->FullDllName.Buffer[(entry->FullDllName.Length - usifstest.Length) / sizeof(WCHAR)];
            name.Length = name.MaximumLength = usifstest.Length;

            blacklist = FsRtlAreNamesEqual(&name, &usifstest, true, NULL);
        }

        if (blacklist) {
            void** frames;
            ULONG i, num_frames;

            frames = ExAllocatePoolWithTag(PagedPool, 256 * sizeof(void*), ALLOC_TAG);
            if (!frames) {
                ERR("out of memory\n");
                return false;
            }

            num_frames = RtlWalkFrameChain(frames, 256, 1);

            for (i = 0; i < num_frames; i++) {
                // entry->Reserved3[1] appears to be the image size
                if (frames[i] >= entry->DllBase && (ULONG_PTR)frames[i] <= (ULONG_PTR)entry->DllBase + (ULONG_PTR)entry->Reserved3[1]) {
                    ExFreePool(frames);
                    return true;
                }
            }

            ExFreePool(frames);
        }

        le = le->Flink;
    }

    return false;
}
#endif // __REACTOS__

// version of RtlUTF8ToUnicodeN for Vista and below
NTSTATUS utf8_to_utf16(WCHAR* dest, ULONG dest_max, ULONG* dest_len, char* src, ULONG src_len) {
    NTSTATUS Status = STATUS_SUCCESS;
    uint8_t* in = (uint8_t*)src;
    uint16_t* out = (uint16_t*)dest;
    ULONG needed = 0, left = dest_max / sizeof(uint16_t);

    for (ULONG i = 0; i < src_len; i++) {
        uint32_t cp;

        if (!(in[i] & 0x80))
            cp = in[i];
        else if ((in[i] & 0xe0) == 0xc0) {
            if (i == src_len - 1 || (in[i+1] & 0xc0) != 0x80) {
                cp = 0xfffd;
                Status = STATUS_SOME_NOT_MAPPED;
            } else {
                cp = ((in[i] & 0x1f) << 6) | (in[i+1] & 0x3f);
                i++;
            }
        } else if ((in[i] & 0xf0) == 0xe0) {
            if (i >= src_len - 2 || (in[i+1] & 0xc0) != 0x80 || (in[i+2] & 0xc0) != 0x80) {
                cp = 0xfffd;
                Status = STATUS_SOME_NOT_MAPPED;
            } else {
                cp = ((in[i] & 0xf) << 12) | ((in[i+1] & 0x3f) << 6) | (in[i+2] & 0x3f);
                i += 2;
            }
        } else if ((in[i] & 0xf8) == 0xf0) {
            if (i >= src_len - 3 || (in[i+1] & 0xc0) != 0x80 || (in[i+2] & 0xc0) != 0x80 || (in[i+3] & 0xc0) != 0x80) {
                cp = 0xfffd;
                Status = STATUS_SOME_NOT_MAPPED;
            } else {
                cp = ((in[i] & 0x7) << 18) | ((in[i+1] & 0x3f) << 12) | ((in[i+2] & 0x3f) << 6) | (in[i+3] & 0x3f);
                i += 3;
            }
        } else {
            cp = 0xfffd;
            Status = STATUS_SOME_NOT_MAPPED;
        }

        if (cp > 0x10ffff) {
            cp = 0xfffd;
            Status = STATUS_SOME_NOT_MAPPED;
        }

        if (dest) {
            if (cp <= 0xffff) {
                if (left < 1)
                    return STATUS_BUFFER_OVERFLOW;

                *out = (uint16_t)cp;
                out++;

                left--;
            } else {
                if (left < 2)
                    return STATUS_BUFFER_OVERFLOW;

                cp -= 0x10000;

                *out = 0xd800 | ((cp & 0xffc00) >> 10);
                out++;

                *out = 0xdc00 | (cp & 0x3ff);
                out++;

                left -= 2;
            }
        }

        if (cp <= 0xffff)
            needed += sizeof(uint16_t);
        else
            needed += 2 * sizeof(uint16_t);
    }

    if (dest_len)
        *dest_len = needed;

    return Status;
}

// version of RtlUnicodeToUTF8N for Vista and below
NTSTATUS utf16_to_utf8(char* dest, ULONG dest_max, ULONG* dest_len, WCHAR* src, ULONG src_len) {
    NTSTATUS Status = STATUS_SUCCESS;
    uint16_t* in = (uint16_t*)src;
    uint8_t* out = (uint8_t*)dest;
    ULONG in_len = src_len / sizeof(uint16_t);
    ULONG needed = 0, left = dest_max;

    for (ULONG i = 0; i < in_len; i++) {
        uint32_t cp = *in;
        in++;

        if ((cp & 0xfc00) == 0xd800) {
            if (i == in_len - 1 || (*in & 0xfc00) != 0xdc00) {
                cp = 0xfffd;
                Status = STATUS_SOME_NOT_MAPPED;
            } else {
                cp = (cp & 0x3ff) << 10;
                cp |= *in & 0x3ff;
                cp += 0x10000;

                in++;
                i++;
            }
        } else if ((cp & 0xfc00) == 0xdc00) {
            cp = 0xfffd;
            Status = STATUS_SOME_NOT_MAPPED;
        }

        if (cp > 0x10ffff) {
            cp = 0xfffd;
            Status = STATUS_SOME_NOT_MAPPED;
        }

        if (dest) {
            if (cp < 0x80) {
                if (left < 1)
                    return STATUS_BUFFER_OVERFLOW;

                *out = (uint8_t)cp;
                out++;

                left--;
            } else if (cp < 0x800) {
                if (left < 2)
                    return STATUS_BUFFER_OVERFLOW;

                *out = 0xc0 | ((cp & 0x7c0) >> 6);
                out++;

                *out = 0x80 | (cp & 0x3f);
                out++;

                left -= 2;
            } else if (cp < 0x10000) {
                if (left < 3)
                    return STATUS_BUFFER_OVERFLOW;

                *out = 0xe0 | ((cp & 0xf000) >> 12);
                out++;

                *out = 0x80 | ((cp & 0xfc0) >> 6);
                out++;

                *out = 0x80 | (cp & 0x3f);
                out++;

                left -= 3;
            } else {
                if (left < 4)
                    return STATUS_BUFFER_OVERFLOW;

                *out = 0xf0 | ((cp & 0x1c0000) >> 18);
                out++;

                *out = 0x80 | ((cp & 0x3f000) >> 12);
                out++;

                *out = 0x80 | ((cp & 0xfc0) >> 6);
                out++;

                *out = 0x80 | (cp & 0x3f);
                out++;

                left -= 4;
            }
        }

        if (cp < 0x80)
            needed++;
        else if (cp < 0x800)
            needed += 2;
        else if (cp < 0x10000)
            needed += 3;
        else
            needed += 4;
    }

    if (dest_len)
        *dest_len = needed;

    return Status;
}

_Dispatch_type_(IRP_MJ_QUERY_VOLUME_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_query_volume_information(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG BytesCopied = 0;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("query volume information\n");
    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Status = STATUS_NOT_IMPLEMENTED;

    switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {
        case FileFsAttributeInformation:
        {
            FILE_FS_ATTRIBUTE_INFORMATION* data = Irp->AssociatedIrp.SystemBuffer;
            bool overflow = false;
#ifndef __REACTOS__
            static const WCHAR ntfs[] = L"NTFS";
#endif
            static const WCHAR btrfs[] = L"Btrfs";
            const WCHAR* fs_name;
            ULONG fs_name_len, orig_fs_name_len;

#ifndef __REACTOS__
            if (Irp->RequestorMode == UserMode && lie_about_fs_type()) {
                fs_name = ntfs;
                orig_fs_name_len = fs_name_len = sizeof(ntfs) - sizeof(WCHAR);
            } else {
                fs_name = btrfs;
                orig_fs_name_len = fs_name_len = sizeof(btrfs) - sizeof(WCHAR);
            }
#else
            fs_name = btrfs;
            orig_fs_name_len = fs_name_len = sizeof(btrfs) - sizeof(WCHAR);
#endif

            TRACE("FileFsAttributeInformation\n");

            if (IrpSp->Parameters.QueryVolume.Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION) - sizeof(WCHAR) + fs_name_len) {
                if (IrpSp->Parameters.QueryVolume.Length > sizeof(FILE_FS_ATTRIBUTE_INFORMATION) - sizeof(WCHAR))
                    fs_name_len = IrpSp->Parameters.QueryVolume.Length - sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + sizeof(WCHAR);
                else
                    fs_name_len = 0;

                overflow = true;
            }

            data->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH |
                                         FILE_UNICODE_ON_DISK | FILE_NAMED_STREAMS | FILE_SUPPORTS_HARD_LINKS | FILE_PERSISTENT_ACLS |
                                         FILE_SUPPORTS_REPARSE_POINTS | FILE_SUPPORTS_SPARSE_FILES | FILE_SUPPORTS_OBJECT_IDS |
                                         FILE_SUPPORTS_OPEN_BY_FILE_ID | FILE_SUPPORTS_EXTENDED_ATTRIBUTES | FILE_SUPPORTS_BLOCK_REFCOUNTING |
                                         FILE_SUPPORTS_POSIX_UNLINK_RENAME;
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

        case FileFsDeviceInformation:
        {
            FILE_FS_DEVICE_INFORMATION* ffdi = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileFsDeviceInformation\n");

            ffdi->DeviceType = FILE_DEVICE_DISK;

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);
            ffdi->Characteristics = Vcb->Vpb->RealDevice->Characteristics;
            ExReleaseResourceLite(&Vcb->tree_lock);

            if (Vcb->readonly)
                ffdi->Characteristics |= FILE_READ_ONLY_DEVICE;
            else
                ffdi->Characteristics &= ~FILE_READ_ONLY_DEVICE;

            BytesCopied = sizeof(FILE_FS_DEVICE_INFORMATION);
            Status = STATUS_SUCCESS;

            break;
        }

        case FileFsFullSizeInformation:
        {
            FILE_FS_FULL_SIZE_INFORMATION* ffsi = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileFsFullSizeInformation\n");

            calculate_total_space(Vcb, (uint64_t*)&ffsi->TotalAllocationUnits.QuadPart, (uint64_t*)&ffsi->ActualAvailableAllocationUnits.QuadPart);
            ffsi->CallerAvailableAllocationUnits.QuadPart = ffsi->ActualAvailableAllocationUnits.QuadPart;
            ffsi->SectorsPerAllocationUnit = Vcb->superblock.sector_size / 512;
            ffsi->BytesPerSector = 512;

            BytesCopied = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;

            break;
        }

        case FileFsObjectIdInformation:
        {
            FILE_FS_OBJECTID_INFORMATION* ffoi = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileFsObjectIdInformation\n");

            RtlCopyMemory(ffoi->ObjectId, &Vcb->superblock.uuid.uuid[0], sizeof(UCHAR) * 16);
            RtlZeroMemory(ffoi->ExtendedInfo, sizeof(ffoi->ExtendedInfo));

            BytesCopied = sizeof(FILE_FS_OBJECTID_INFORMATION);
            Status = STATUS_SUCCESS;

            break;
        }

        case FileFsSizeInformation:
        {
            FILE_FS_SIZE_INFORMATION* ffsi = Irp->AssociatedIrp.SystemBuffer;

            TRACE("FileFsSizeInformation\n");

            calculate_total_space(Vcb, (uint64_t*)&ffsi->TotalAllocationUnits.QuadPart, (uint64_t*)&ffsi->AvailableAllocationUnits.QuadPart);
            ffsi->SectorsPerAllocationUnit = Vcb->superblock.sector_size / 512;
            ffsi->BytesPerSector = 512;

            BytesCopied = sizeof(FILE_FS_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;

            break;
        }

        case FileFsVolumeInformation:
        {
            FILE_FS_VOLUME_INFORMATION* data = Irp->AssociatedIrp.SystemBuffer;
            FILE_FS_VOLUME_INFORMATION ffvi;
            bool overflow = false;
            ULONG label_len, orig_label_len;

            TRACE("FileFsVolumeInformation\n");
            TRACE("max length = %lu\n", IrpSp->Parameters.QueryVolume.Length);

            ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

            Status = utf8_to_utf16(NULL, 0, &label_len, Vcb->superblock.label, (ULONG)strlen(Vcb->superblock.label));
            if (!NT_SUCCESS(Status)) {
                ERR("utf8_to_utf16 returned %08lx\n", Status);
                ExReleaseResourceLite(&Vcb->tree_lock);
                break;
            }

            orig_label_len = label_len;

            if (IrpSp->Parameters.QueryVolume.Length < offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + label_len) {
                if (IrpSp->Parameters.QueryVolume.Length > offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel))
                    label_len = IrpSp->Parameters.QueryVolume.Length - offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel);
                else
                    label_len = 0;

                overflow = true;
            }

            TRACE("label_len = %lu\n", label_len);

            RtlZeroMemory(&ffvi, offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel));

            ffvi.VolumeSerialNumber = Vcb->superblock.uuid.uuid[12] << 24 | Vcb->superblock.uuid.uuid[13] << 16 | Vcb->superblock.uuid.uuid[14] << 8 | Vcb->superblock.uuid.uuid[15];
            ffvi.VolumeLabelLength = orig_label_len;

            RtlCopyMemory(data, &ffvi, min(offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel), IrpSp->Parameters.QueryVolume.Length));

            if (label_len > 0) {
                ULONG bytecount;

                Status = utf8_to_utf16(&data->VolumeLabel[0], label_len, &bytecount, Vcb->superblock.label, (ULONG)strlen(Vcb->superblock.label));
                if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL) {
                    ERR("utf8_to_utf16 returned %08lx\n", Status);
                    ExReleaseResourceLite(&Vcb->tree_lock);
                    break;
                }

                TRACE("label = %.*S\n", (int)(label_len / sizeof(WCHAR)), data->VolumeLabel);
            }

            ExReleaseResourceLite(&Vcb->tree_lock);

            BytesCopied = offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + label_len;
            Status = overflow ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
            break;
        }

#ifndef __REACTOS__
#ifdef _MSC_VER // not in mingw yet
        case FileFsSectorSizeInformation:
        {
            FILE_FS_SECTOR_SIZE_INFORMATION* data = Irp->AssociatedIrp.SystemBuffer;

            data->LogicalBytesPerSector = Vcb->superblock.sector_size;
            data->PhysicalBytesPerSectorForAtomicity = Vcb->superblock.sector_size;
            data->PhysicalBytesPerSectorForPerformance = Vcb->superblock.sector_size;
            data->FileSystemEffectivePhysicalBytesPerSectorForAtomicity = Vcb->superblock.sector_size;
            data->ByteOffsetForSectorAlignment = 0;
            data->ByteOffsetForPartitionAlignment = 0;

            data->Flags = SSINFO_FLAGS_ALIGNED_DEVICE | SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE;

            if (Vcb->trim && !Vcb->options.no_trim)
                data->Flags |= SSINFO_FLAGS_TRIM_ENABLED;

            BytesCopied = sizeof(FILE_FS_SECTOR_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;

            break;
        }
#endif
#endif /* __REACTOS__ */

        default:
            Status = STATUS_INVALID_PARAMETER;
            WARN("unknown FsInformationClass %u\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
            break;
    }

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        Irp->IoStatus.Information = 0;
    else
        Irp->IoStatus.Information = BytesCopied;

end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("query volume information returning %08lx\n", Status);

    FsRtlExitFileSystem();

    return Status;
}

_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall read_completion(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PVOID conptr) {
    read_context* context = conptr;

    UNUSED(DeviceObject);

    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS create_root(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ uint64_t id,
                     _Out_ root** rootptr, _In_ bool no_tree, _In_ uint64_t offset, _In_opt_ PIRP Irp) {
    NTSTATUS Status;
    root* r;
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

    ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
    if (!ri) {
        ERR("out of memory\n");

        ExFreePool(r->nonpaged);
        ExFreePool(r);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    r->id = id;
    r->treeholder.address = 0;
    r->treeholder.generation = Vcb->superblock.generation;
    r->treeholder.tree = NULL;
    r->lastinode = 0;
    r->dirty = false;
    r->received = false;
    r->reserved = NULL;
    r->parent = 0;
    r->send_ops = 0;
    RtlZeroMemory(&r->root_item, sizeof(ROOT_ITEM));
    r->root_item.num_references = 1;
    r->fcbs_version = 0;
    r->checked_for_orphans = true;
    r->dropped = false;
    InitializeListHead(&r->fcbs);
    RtlZeroMemory(r->fcbs_ptrs, sizeof(LIST_ENTRY*) * 256);

    RtlCopyMemory(ri, &r->root_item, sizeof(ROOT_ITEM));

    // We ask here for a traverse_ptr to the item we're inserting, so we can
    // copy some of the tree's variables

    Status = insert_tree_item(Vcb, Vcb->root_root, id, TYPE_ROOT_ITEM, offset, ri, sizeof(ROOT_ITEM), &tp, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08lx\n", Status);
        ExFreePool(ri);
        ExFreePool(r->nonpaged);
        ExFreePool(r);
        return Status;
    }

    ExInitializeResourceLite(&r->nonpaged->load_tree_lock);

    InsertTailList(&Vcb->roots, &r->list_entry);

    if (!no_tree) {
        tree* t = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
        if (!t) {
            ERR("out of memory\n");

            delete_tree_item(Vcb, &tp);

            ExFreePool(r->nonpaged);
            ExFreePool(r);
            ExFreePool(ri);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        t->nonpaged = NULL;

        t->is_unique = true;
        t->uniqueness_determined = true;
        t->buf = NULL;

        r->treeholder.tree = t;

        RtlZeroMemory(&t->header, sizeof(tree_header));
        t->header.fs_uuid = tp.tree->header.fs_uuid;
        t->header.address = 0;
        t->header.flags = HEADER_FLAG_MIXED_BACKREF | 1; // 1 == "written"? Why does the Linux driver record this?
        t->header.chunk_tree_uuid = tp.tree->header.chunk_tree_uuid;
        t->header.generation = Vcb->superblock.generation;
        t->header.tree_id = id;
        t->header.num_items = 0;
        t->header.level = 0;

        t->has_address = false;
        t->size = 0;
        t->Vcb = Vcb;
        t->parent = NULL;
        t->paritem = NULL;
        t->root = r;

        InitializeListHead(&t->itemlist);

        t->new_address = 0;
        t->has_new_address = false;
        t->updated_extents = false;

        InsertTailList(&Vcb->trees, &t->list_entry);
        t->list_entry_hash.Flink = NULL;

        t->write = true;
        Vcb->need_write = true;
    }

    *rootptr = r;

    return STATUS_SUCCESS;
}

static NTSTATUS set_label(_In_ device_extension* Vcb, _In_ FILE_FS_LABEL_INFORMATION* ffli) {
    ULONG utf8len;
    NTSTATUS Status;
    ULONG vollen, i;

    TRACE("label = %.*S\n", (int)(ffli->VolumeLabelLength / sizeof(WCHAR)), ffli->VolumeLabel);

    vollen = ffli->VolumeLabelLength;

    for (i = 0; i < ffli->VolumeLabelLength / sizeof(WCHAR); i++) {
        if (ffli->VolumeLabel[i] == 0) {
            vollen = i * sizeof(WCHAR);
            break;
        } else if (ffli->VolumeLabel[i] == '/' || ffli->VolumeLabel[i] == '\\') {
            Status = STATUS_INVALID_VOLUME_LABEL;
            goto end;
        }
    }

    if (vollen == 0) {
        utf8len = 0;
    } else {
        Status = utf16_to_utf8(NULL, 0, &utf8len, ffli->VolumeLabel, vollen);
        if (!NT_SUCCESS(Status))
            goto end;

        if (utf8len > MAX_LABEL_SIZE) {
            Status = STATUS_INVALID_VOLUME_LABEL;
            goto end;
        }
    }

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    if (utf8len > 0) {
        Status = utf16_to_utf8((PCHAR)&Vcb->superblock.label, MAX_LABEL_SIZE, &utf8len, ffli->VolumeLabel, vollen);
        if (!NT_SUCCESS(Status))
            goto release;
    } else
        Status = STATUS_SUCCESS;

    if (utf8len < MAX_LABEL_SIZE)
        RtlZeroMemory(Vcb->superblock.label + utf8len, MAX_LABEL_SIZE - utf8len);

    Vcb->need_write = true;

release:
    ExReleaseResourceLite(&Vcb->tree_lock);

end:
    TRACE("returning %08lx\n", Status);

    return Status;
}

_Dispatch_type_(IRP_MJ_SET_VOLUME_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_set_volume_information(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("set volume information\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_NOT_IMPLEMENTED;

    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }

    if (Vcb->removing || Vcb->locked) {
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

    TRACE("returning %08lx\n", Status);

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

void send_notification_fileref(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream) {
    UNICODE_STRING fn;
    NTSTATUS Status;
    ULONG reqlen;
    USHORT name_offset;
    fcb* fcb = fileref->fcb;

    fn.Length = fn.MaximumLength = 0;
    Status = fileref_get_filename(fileref, &fn, NULL, &reqlen);
    if (Status != STATUS_BUFFER_OVERFLOW) {
        ERR("fileref_get_filename returned %08lx\n", Status);
        return;
    }

    if (reqlen > 0xffff) {
        WARN("reqlen was too long for FsRtlNotifyFilterReportChange\n");
        return;
    }

    fn.Buffer = ExAllocatePoolWithTag(PagedPool, reqlen, ALLOC_TAG);
    if (!fn.Buffer) {
        ERR("out of memory\n");
        return;
    }

    fn.MaximumLength = (USHORT)reqlen;
    fn.Length = 0;

    Status = fileref_get_filename(fileref, &fn, &name_offset, &reqlen);
    if (!NT_SUCCESS(Status)) {
        ERR("fileref_get_filename returned %08lx\n", Status);
        ExFreePool(fn.Buffer);
        return;
    }

    FsRtlNotifyFilterReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&fn, name_offset,
                                  (PSTRING)stream, NULL, filter_match, action, NULL, NULL);
    ExFreePool(fn.Buffer);
}

static void send_notification_fcb(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream) {
    fcb* fcb = fileref->fcb;
    LIST_ENTRY* le;
    NTSTATUS Status;

    // no point looking for hardlinks if st_nlink == 1
    if (fileref->fcb->inode_item.st_nlink == 1) {
        ExAcquireResourceExclusiveLite(&fcb->Vcb->fileref_lock, true);
        send_notification_fileref(fileref, filter_match, action, stream);
        ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
        return;
    }

    ExAcquireResourceExclusiveLite(&fcb->Vcb->fileref_lock, true);

    le = fcb->hardlinks.Flink;
    while (le != &fcb->hardlinks) {
        hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);
        file_ref* parfr;

        Status = open_fileref_by_inode(fcb->Vcb, fcb->subvol, hl->parent, &parfr, NULL);

        if (!NT_SUCCESS(Status))
            ERR("open_fileref_by_inode returned %08lx\n", Status);
        else if (!parfr->deleted) {
            UNICODE_STRING fn;
            ULONG pathlen;

            fn.Length = fn.MaximumLength = 0;
            Status = fileref_get_filename(parfr, &fn, NULL, &pathlen);
            if (Status != STATUS_BUFFER_OVERFLOW) {
                ERR("fileref_get_filename returned %08lx\n", Status);
                free_fileref(parfr);
                break;
            }

            if (parfr != fcb->Vcb->root_fileref)
                pathlen += sizeof(WCHAR);

            if (pathlen + hl->name.Length > 0xffff) {
                WARN("pathlen + hl->name.Length was too long for FsRtlNotifyFilterReportChange\n");
                free_fileref(parfr);
                break;
            }

            fn.MaximumLength = (USHORT)(pathlen + hl->name.Length);
            fn.Buffer = ExAllocatePoolWithTag(PagedPool, fn.MaximumLength, ALLOC_TAG);
            if (!fn.Buffer) {
                ERR("out of memory\n");
                free_fileref(parfr);
                break;
            }

            Status = fileref_get_filename(parfr, &fn, NULL, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("fileref_get_filename returned %08lx\n", Status);
                free_fileref(parfr);
                ExFreePool(fn.Buffer);
                break;
            }

            if (parfr != fcb->Vcb->root_fileref) {
                fn.Buffer[(pathlen / sizeof(WCHAR)) - 1] = '\\';
                fn.Length += sizeof(WCHAR);
            }

            RtlCopyMemory(&fn.Buffer[pathlen / sizeof(WCHAR)], hl->name.Buffer, hl->name.Length);
            fn.Length += hl->name.Length;

            FsRtlNotifyFilterReportChange(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, (PSTRING)&fn, (USHORT)pathlen,
                                          (PSTRING)stream, NULL, filter_match, action, NULL, NULL);

            ExFreePool(fn.Buffer);

            free_fileref(parfr);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
}

typedef struct {
    file_ref* fileref;
    ULONG filter_match;
    ULONG action;
    PUNICODE_STRING stream;
    PIO_WORKITEM work_item;
} notification_fcb;

_Function_class_(IO_WORKITEM_ROUTINE)
static void __stdcall notification_work_item(PDEVICE_OBJECT DeviceObject, PVOID con) {
    notification_fcb* nf = con;

    UNUSED(DeviceObject);

    ExAcquireResourceSharedLite(&nf->fileref->fcb->Vcb->tree_lock, TRUE); // protect us from fileref being reaped

    send_notification_fcb(nf->fileref, nf->filter_match, nf->action, nf->stream);

    free_fileref(nf->fileref);

    ExReleaseResourceLite(&nf->fileref->fcb->Vcb->tree_lock);

    IoFreeWorkItem(nf->work_item);

    ExFreePool(nf);
}

void queue_notification_fcb(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream) {
    notification_fcb* nf;
    PIO_WORKITEM work_item;

    nf = ExAllocatePoolWithTag(PagedPool, sizeof(notification_fcb), ALLOC_TAG);
    if (!nf) {
        ERR("out of memory\n");
        return;
    }

    work_item = IoAllocateWorkItem(master_devobj);
    if (!work_item) {
        ERR("out of memory\n");
        ExFreePool(nf);
        return;
    }

    InterlockedIncrement(&fileref->refcount);

    nf->fileref = fileref;
    nf->filter_match = filter_match;
    nf->action = action;
    nf->stream = stream;
    nf->work_item = work_item;

    IoQueueWorkItem(work_item, notification_work_item, DelayedWorkQueue, nf);
}

void mark_fcb_dirty(_In_ fcb* fcb) {
    if (!fcb->dirty) {
#ifdef DEBUG_FCB_REFCOUNTS
        LONG rc;
#endif
        fcb->dirty = true;

#ifdef DEBUG_FCB_REFCOUNTS
        rc = InterlockedIncrement(&fcb->refcount);
        WARN("fcb %p: refcount now %i\n", fcb, rc);
#else
        InterlockedIncrement(&fcb->refcount);
#endif

        ExAcquireResourceExclusiveLite(&fcb->Vcb->dirty_fcbs_lock, true);
        InsertTailList(&fcb->Vcb->dirty_fcbs, &fcb->list_entry_dirty);
        ExReleaseResourceLite(&fcb->Vcb->dirty_fcbs_lock);
    }

    fcb->Vcb->need_write = true;
}

void mark_fileref_dirty(_In_ file_ref* fileref) {
    if (!fileref->dirty) {
        fileref->dirty = true;
        increase_fileref_refcount(fileref);

        ExAcquireResourceExclusiveLite(&fileref->fcb->Vcb->dirty_filerefs_lock, true);
        InsertTailList(&fileref->fcb->Vcb->dirty_filerefs, &fileref->list_entry_dirty);
        ExReleaseResourceLite(&fileref->fcb->Vcb->dirty_filerefs_lock);
    }

    fileref->fcb->Vcb->need_write = true;
}

#ifdef DEBUG_FCB_REFCOUNTS
void _free_fcb(_Inout_ fcb* fcb, _In_ const char* func) {
    LONG rc = InterlockedDecrement(&fcb->refcount);
#else
void free_fcb(_Inout_ fcb* fcb) {
    InterlockedDecrement(&fcb->refcount);
#endif

#ifdef DEBUG_FCB_REFCOUNTS
    ERR("fcb %p (%s): refcount now %i (subvol %I64x, inode %I64x)\n", fcb, func, rc, fcb->subvol ? fcb->subvol->id : 0, fcb->inode);
#endif
}

void reap_fcb(fcb* fcb) {
    uint8_t c = fcb->hash >> 24;

    if (fcb->subvol && fcb->subvol->fcbs_ptrs[c] == &fcb->list_entry) {
        if (fcb->list_entry.Flink != &fcb->subvol->fcbs && (CONTAINING_RECORD(fcb->list_entry.Flink, struct _fcb, list_entry)->hash >> 24) == c)
            fcb->subvol->fcbs_ptrs[c] = fcb->list_entry.Flink;
        else
            fcb->subvol->fcbs_ptrs[c] = NULL;
    }

    if (fcb->list_entry.Flink) {
        RemoveEntryList(&fcb->list_entry);

        if (fcb->subvol && fcb->subvol->dropped && IsListEmpty(&fcb->subvol->fcbs)) {
            ExDeleteResourceLite(&fcb->subvol->nonpaged->load_tree_lock);
            ExFreePool(fcb->subvol->nonpaged);
            ExFreePool(fcb->subvol);
        }
    }

    if (fcb->list_entry_all.Flink)
        RemoveEntryList(&fcb->list_entry_all);

    ExDeleteResourceLite(&fcb->nonpaged->resource);
    ExDeleteResourceLite(&fcb->nonpaged->paging_resource);
    ExDeleteResourceLite(&fcb->nonpaged->dir_children_lock);

    ExFreeToNPagedLookasideList(&fcb->Vcb->fcb_np_lookaside, fcb->nonpaged);

    if (fcb->sd)
        ExFreePool(fcb->sd);

    if (fcb->adsxattr.Buffer)
        ExFreePool(fcb->adsxattr.Buffer);

    if (fcb->reparse_xattr.Buffer)
        ExFreePool(fcb->reparse_xattr.Buffer);

    if (fcb->ea_xattr.Buffer)
        ExFreePool(fcb->ea_xattr.Buffer);

    if (fcb->adsdata.Buffer)
        ExFreePool(fcb->adsdata.Buffer);

    while (!IsListEmpty(&fcb->extents)) {
        LIST_ENTRY* le = RemoveHeadList(&fcb->extents);
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (ext->csum)
            ExFreePool(ext->csum);

        ExFreePool(ext);
    }

    while (!IsListEmpty(&fcb->hardlinks)) {
        LIST_ENTRY* le = RemoveHeadList(&fcb->hardlinks);
        hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);

        if (hl->name.Buffer)
            ExFreePool(hl->name.Buffer);

        if (hl->utf8.Buffer)
            ExFreePool(hl->utf8.Buffer);

        ExFreePool(hl);
    }

    while (!IsListEmpty(&fcb->xattrs)) {
        xattr* xa = CONTAINING_RECORD(RemoveHeadList(&fcb->xattrs), xattr, list_entry);

        ExFreePool(xa);
    }

    while (!IsListEmpty(&fcb->dir_children_index)) {
        LIST_ENTRY* le = RemoveHeadList(&fcb->dir_children_index);
        dir_child* dc = CONTAINING_RECORD(le, dir_child, list_entry_index);

        ExFreePool(dc->utf8.Buffer);
        ExFreePool(dc->name.Buffer);
        ExFreePool(dc->name_uc.Buffer);
        ExFreePool(dc);
    }

    if (fcb->hash_ptrs)
        ExFreePool(fcb->hash_ptrs);

    if (fcb->hash_ptrs_uc)
        ExFreePool(fcb->hash_ptrs_uc);

    FsRtlUninitializeFileLock(&fcb->lock);
    FsRtlUninitializeOplock(fcb_oplock(fcb));

    if (fcb->pool_type == NonPagedPool)
        ExFreePool(fcb);
    else
        ExFreeToPagedLookasideList(&fcb->Vcb->fcb_lookaside, fcb);
}

void reap_fcbs(device_extension* Vcb) {
    LIST_ENTRY* le;

    le = Vcb->all_fcbs.Flink;
    while (le != &Vcb->all_fcbs) {
        fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_all);
        LIST_ENTRY* le2 = le->Flink;

        if (fcb->refcount == 0)
            reap_fcb(fcb);

        le = le2;
    }
}

void free_fileref(_Inout_ file_ref* fr) {
#if defined(_DEBUG) || defined(DEBUG_FCB_REFCOUNTS)
    LONG rc = InterlockedDecrement(&fr->refcount);

#ifdef DEBUG_FCB_REFCOUNTS
    ERR("fileref %p: refcount now %i\n", fr, rc);
#endif

#ifdef _DEBUG
    if (rc < 0) {
        ERR("fileref %p: refcount now %li\n", fr, rc);
        int3;
    }
#endif
#else
    InterlockedDecrement(&fr->refcount);
#endif
}

void reap_fileref(device_extension* Vcb, file_ref* fr) {
    // FIXME - do we need a file_ref lock?

    // FIXME - do delete if needed

    // FIXME - throw error if children not empty

    if (fr->fcb->fileref == fr)
        fr->fcb->fileref = NULL;

    if (fr->dc) {
        if (fr->fcb->ads)
            fr->dc->size = fr->fcb->adsdata.Length;

        fr->dc->fileref = NULL;
    }

    if (fr->list_entry.Flink)
        RemoveEntryList(&fr->list_entry);

    if (fr->parent)
        free_fileref(fr->parent);

    free_fcb(fr->fcb);

    if (fr->oldutf8.Buffer)
        ExFreePool(fr->oldutf8.Buffer);

    ExFreeToPagedLookasideList(&Vcb->fileref_lookaside, fr);
}

void reap_filerefs(device_extension* Vcb, file_ref* fr) {
    LIST_ENTRY* le;

    // FIXME - recursion is a bad idea in kernel mode

    le = fr->children.Flink;
    while (le != &fr->children) {
        file_ref* c = CONTAINING_RECORD(le, file_ref, list_entry);
        LIST_ENTRY* le2 = le->Flink;

        reap_filerefs(Vcb, c);

        le = le2;
    }

    if (fr->refcount == 0)
        reap_fileref(Vcb, fr);
}

static NTSTATUS close_file(_In_ PFILE_OBJECT FileObject, _In_ PIRP Irp) {
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref = NULL;
    LONG open_files;

    UNUSED(Irp);

    TRACE("FileObject = %p\n", FileObject);

    fcb = FileObject->FsContext;
    if (!fcb) {
        TRACE("FCB was NULL, returning success\n");
        return STATUS_SUCCESS;
    }

    open_files = InterlockedDecrement(&fcb->Vcb->open_files);

    ccb = FileObject->FsContext2;

    TRACE("close called for fcb %p)\n", fcb);

    // FIXME - make sure notification gets sent if file is being deleted

    if (ccb) {
        if (ccb->query_string.Buffer)
            RtlFreeUnicodeString(&ccb->query_string);

        if (ccb->filename.Buffer)
            ExFreePool(ccb->filename.Buffer);

        // FIXME - use refcounts for fileref
        fileref = ccb->fileref;

        if (fcb->Vcb->running_sends > 0) {
            bool send_cancelled = false;

            ExAcquireResourceExclusiveLite(&fcb->Vcb->send_load_lock, true);

            if (ccb->send) {
                ccb->send->cancelling = true;
                send_cancelled = true;
                KeSetEvent(&ccb->send->cleared_event, 0, false);
            }

            ExReleaseResourceLite(&fcb->Vcb->send_load_lock);

            if (send_cancelled) {
                while (ccb->send) {
                    ExAcquireResourceExclusiveLite(&fcb->Vcb->send_load_lock, true);
                    ExReleaseResourceLite(&fcb->Vcb->send_load_lock);
                }
            }
        }

        ExFreePool(ccb);
    }

    CcUninitializeCacheMap(FileObject, NULL, NULL);

    if (open_files == 0 && fcb->Vcb->removing) {
        uninit(fcb->Vcb);
        return STATUS_SUCCESS;
    }

    if (!(fcb->Vcb->Vpb->Flags & VPB_MOUNTED))
        return STATUS_SUCCESS;

    if (fileref)
        free_fileref(fileref);
    else
        free_fcb(fcb);

    return STATUS_SUCCESS;
}

void uninit(_In_ device_extension* Vcb) {
    uint64_t i;
    KIRQL irql;
    NTSTATUS Status;
    LIST_ENTRY* le;
    LARGE_INTEGER time;

    if (!Vcb->removing) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);
        Vcb->removing = true;
        ExReleaseResourceLite(&Vcb->tree_lock);
    }

    if (Vcb->vde && Vcb->vde->mounted_device == Vcb->devobj)
        Vcb->vde->mounted_device = NULL;

    IoAcquireVpbSpinLock(&irql);
    Vcb->Vpb->Flags &= ~VPB_MOUNTED;
    Vcb->Vpb->Flags |= VPB_DIRECT_WRITES_ALLOWED;
    Vcb->Vpb->DeviceObject = NULL;
    IoReleaseVpbSpinLock(irql);

    // FIXME - needs global_loading_lock to be held
    if (Vcb->list_entry.Flink)
        RemoveEntryList(&Vcb->list_entry);

    if (Vcb->balance.thread) {
        Vcb->balance.paused = false;
        Vcb->balance.stopping = true;
        KeSetEvent(&Vcb->balance.event, 0, false);
        KeWaitForSingleObject(&Vcb->balance.finished, Executive, KernelMode, false, NULL);
    }

    if (Vcb->scrub.thread) {
        Vcb->scrub.paused = false;
        Vcb->scrub.stopping = true;
        KeSetEvent(&Vcb->scrub.event, 0, false);
        KeWaitForSingleObject(&Vcb->scrub.finished, Executive, KernelMode, false, NULL);
    }

    if (Vcb->running_sends != 0) {
        bool send_cancelled = false;

        ExAcquireResourceExclusiveLite(&Vcb->send_load_lock, true);

        le = Vcb->send_ops.Flink;
        while (le != &Vcb->send_ops) {
            send_info* send = CONTAINING_RECORD(le, send_info, list_entry);

            if (!send->cancelling) {
                send->cancelling = true;
                send_cancelled = true;
                send->ccb = NULL;
                KeSetEvent(&send->cleared_event, 0, false);
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->send_load_lock);

        if (send_cancelled) {
            while (Vcb->running_sends != 0) {
                ExAcquireResourceExclusiveLite(&Vcb->send_load_lock, true);
                ExReleaseResourceLite(&Vcb->send_load_lock);
            }
        }
    }

    Status = registry_mark_volume_unmounted(&Vcb->superblock.uuid);
    if (!NT_SUCCESS(Status) && Status != STATUS_TOO_LATE)
        WARN("registry_mark_volume_unmounted returned %08lx\n", Status);

    for (i = 0; i < Vcb->calcthreads.num_threads; i++) {
        Vcb->calcthreads.threads[i].quit = true;
    }

    KeSetEvent(&Vcb->calcthreads.event, 0, false);

    for (i = 0; i < Vcb->calcthreads.num_threads; i++) {
        KeWaitForSingleObject(&Vcb->calcthreads.threads[i].finished, Executive, KernelMode, false, NULL);

        ZwClose(Vcb->calcthreads.threads[i].handle);
    }

    ExFreePool(Vcb->calcthreads.threads);

    time.QuadPart = 0;
    KeSetTimer(&Vcb->flush_thread_timer, time, NULL); // trigger the timer early
    KeWaitForSingleObject(&Vcb->flush_thread_finished, Executive, KernelMode, false, NULL);

    reap_fcb(Vcb->volume_fcb);
    reap_fcb(Vcb->dummy_fcb);

    if (Vcb->root_file)
        ObDereferenceObject(Vcb->root_file);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->cache) {
            reap_fcb(c->cache);
            c->cache = NULL;
        }

        le = le->Flink;
    }

    while (!IsListEmpty(&Vcb->all_fcbs)) {
        fcb* fcb = CONTAINING_RECORD(Vcb->all_fcbs.Flink, struct _fcb, list_entry_all);

        reap_fcb(fcb);
    }

    while (!IsListEmpty(&Vcb->sys_chunks)) {
        sys_chunk* sc = CONTAINING_RECORD(RemoveHeadList(&Vcb->sys_chunks), sys_chunk, list_entry);

        if (sc->data)
            ExFreePool(sc->data);

        ExFreePool(sc);
    }

    while (!IsListEmpty(&Vcb->roots)) {
        root* r = CONTAINING_RECORD(RemoveHeadList(&Vcb->roots), root, list_entry);

        ExDeleteResourceLite(&r->nonpaged->load_tree_lock);
        ExFreePool(r->nonpaged);
        ExFreePool(r);
    }

    while (!IsListEmpty(&Vcb->chunks)) {
        chunk* c = CONTAINING_RECORD(RemoveHeadList(&Vcb->chunks), chunk, list_entry);

        while (!IsListEmpty(&c->space)) {
            LIST_ENTRY* le2 = RemoveHeadList(&c->space);
            space* s = CONTAINING_RECORD(le2, space, list_entry);

            ExFreePool(s);
        }

        while (!IsListEmpty(&c->deleting)) {
            LIST_ENTRY* le2 = RemoveHeadList(&c->deleting);
            space* s = CONTAINING_RECORD(le2, space, list_entry);

            ExFreePool(s);
        }

        if (c->devices)
            ExFreePool(c->devices);

        if (c->cache)
            reap_fcb(c->cache);

        ExDeleteResourceLite(&c->range_locks_lock);
        ExDeleteResourceLite(&c->partial_stripes_lock);
        ExDeleteResourceLite(&c->lock);
        ExDeleteResourceLite(&c->changed_extents_lock);

        ExFreePool(c->chunk_item);
        ExFreePool(c);
    }

    while (!IsListEmpty(&Vcb->devices)) {
        device* dev = CONTAINING_RECORD(RemoveHeadList(&Vcb->devices), device, list_entry);

        while (!IsListEmpty(&dev->space)) {
            LIST_ENTRY* le2 = RemoveHeadList(&dev->space);
            space* s = CONTAINING_RECORD(le2, space, list_entry);

            ExFreePool(s);
        }

        ExFreePool(dev);
    }

    ExAcquireResourceExclusiveLite(&Vcb->scrub.stats_lock, true);
    while (!IsListEmpty(&Vcb->scrub.errors)) {
        scrub_error* err = CONTAINING_RECORD(RemoveHeadList(&Vcb->scrub.errors), scrub_error, list_entry);

        ExFreePool(err);
    }
    ExReleaseResourceLite(&Vcb->scrub.stats_lock);

    ExDeleteResourceLite(&Vcb->fcb_lock);
    ExDeleteResourceLite(&Vcb->fileref_lock);
    ExDeleteResourceLite(&Vcb->load_lock);
    ExDeleteResourceLite(&Vcb->tree_lock);
    ExDeleteResourceLite(&Vcb->chunk_lock);
    ExDeleteResourceLite(&Vcb->dirty_fcbs_lock);
    ExDeleteResourceLite(&Vcb->dirty_filerefs_lock);
    ExDeleteResourceLite(&Vcb->dirty_subvols_lock);
    ExDeleteResourceLite(&Vcb->scrub.stats_lock);
    ExDeleteResourceLite(&Vcb->send_load_lock);

    ExDeletePagedLookasideList(&Vcb->tree_data_lookaside);
    ExDeletePagedLookasideList(&Vcb->traverse_ptr_lookaside);
    ExDeletePagedLookasideList(&Vcb->batch_item_lookaside);
    ExDeletePagedLookasideList(&Vcb->fileref_lookaside);
    ExDeletePagedLookasideList(&Vcb->fcb_lookaside);
    ExDeletePagedLookasideList(&Vcb->name_bit_lookaside);
    ExDeleteNPagedLookasideList(&Vcb->range_lock_lookaside);
    ExDeleteNPagedLookasideList(&Vcb->fcb_np_lookaside);

    ZwClose(Vcb->flush_thread_handle);

    if (Vcb->devobj->AttachedDevice)
        IoDetachDevice(Vcb->devobj);

    IoDeleteDevice(Vcb->devobj);
}

static NTSTATUS delete_fileref_fcb(_In_ file_ref* fileref, _In_opt_ PFILE_OBJECT FileObject, _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;

    // excise extents

    if (fileref->fcb->type != BTRFS_TYPE_DIRECTORY && fileref->fcb->inode_item.st_size > 0) {
        Status = excise_extents(fileref->fcb->Vcb, fileref->fcb, 0, sector_align(fileref->fcb->inode_item.st_size, fileref->fcb->Vcb->superblock.sector_size), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            return Status;
        }
    }

    fileref->fcb->Header.AllocationSize.QuadPart = 0;
    fileref->fcb->Header.FileSize.QuadPart = 0;
    fileref->fcb->Header.ValidDataLength.QuadPart = 0;

    if (FileObject) {
        CC_FILE_SIZES ccfs;

        ccfs.AllocationSize = fileref->fcb->Header.AllocationSize;
        ccfs.FileSize = fileref->fcb->Header.FileSize;
        ccfs.ValidDataLength = fileref->fcb->Header.ValidDataLength;

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            CcSetFileSizes(FileObject, &ccfs);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("CcSetFileSizes threw exception %08lx\n", Status);
            return Status;
        }
    }

    fileref->fcb->deleted = true;

    le = fileref->children.Flink;
    while (le != &fileref->children) {
        file_ref* fr2 = CONTAINING_RECORD(le, file_ref, list_entry);

        if (fr2->fcb->ads) {
            fr2->fcb->deleted = true;
            mark_fcb_dirty(fr2->fcb);
        }

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

NTSTATUS delete_fileref(_In_ file_ref* fileref, _In_opt_ PFILE_OBJECT FileObject, _In_ bool make_orphan, _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback) {
    LARGE_INTEGER newlength, time;
    BTRFS_TIME now;
    NTSTATUS Status;
    ULONG utf8len = 0;

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    ExAcquireResourceExclusiveLite(fileref->fcb->Header.Resource, true);

    if (fileref->deleted) {
        ExReleaseResourceLite(fileref->fcb->Header.Resource);
        return STATUS_SUCCESS;
    }

    if (fileref->fcb->subvol->send_ops > 0) {
        ExReleaseResourceLite(fileref->fcb->Header.Resource);
        return STATUS_ACCESS_DENIED;
    }

    fileref->deleted = true;
    mark_fileref_dirty(fileref);

    // delete INODE_ITEM (0x1)

    TRACE("nlink = %u\n", fileref->fcb->inode_item.st_nlink);

    if (!fileref->fcb->ads) {
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            LIST_ENTRY* le;

            mark_fcb_dirty(fileref->fcb);

            fileref->fcb->inode_item_changed = true;

            if (fileref->fcb->inode_item.st_nlink > 1 || make_orphan) {
                fileref->fcb->inode_item.st_nlink--;
                fileref->fcb->inode_item.transid = fileref->fcb->Vcb->superblock.generation;
                fileref->fcb->inode_item.sequence++;
                fileref->fcb->inode_item.st_ctime = now;
            } else {
                Status = delete_fileref_fcb(fileref, FileObject, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_fileref_fcb returned %08lx\n", Status);
                    ExReleaseResourceLite(fileref->fcb->Header.Resource);
                    return Status;
                }
            }

            if (fileref->dc) {
                le = fileref->fcb->hardlinks.Flink;
                while (le != &fileref->fcb->hardlinks) {
                    hardlink* hl = CONTAINING_RECORD(le, hardlink, list_entry);

                    if (hl->parent == fileref->parent->fcb->inode && hl->index == fileref->dc->index) {
                        RemoveEntryList(&hl->list_entry);

                        if (hl->name.Buffer)
                            ExFreePool(hl->name.Buffer);

                        if (hl->utf8.Buffer)
                            ExFreePool(hl->utf8.Buffer);

                        ExFreePool(hl);
                        break;
                    }

                    le = le->Flink;
                }
            }
        } else if (fileref->fcb->subvol->parent == fileref->parent->fcb->subvol->id) { // valid subvolume
            if (fileref->fcb->subvol->root_item.num_references > 1) {
                fileref->fcb->subvol->root_item.num_references--;

                mark_fcb_dirty(fileref->fcb); // so ROOT_ITEM gets updated
            } else {
                LIST_ENTRY* le;

                // FIXME - we need a lock here

                RemoveEntryList(&fileref->fcb->subvol->list_entry);

                InsertTailList(&fileref->fcb->Vcb->drop_roots, &fileref->fcb->subvol->list_entry);

                le = fileref->children.Flink;
                while (le != &fileref->children) {
                    file_ref* fr2 = CONTAINING_RECORD(le, file_ref, list_entry);

                    if (fr2->fcb->ads) {
                        fr2->fcb->deleted = true;
                        mark_fcb_dirty(fr2->fcb);
                    }

                    le = le->Flink;
                }
            }
        }
    } else {
        fileref->fcb->deleted = true;
        mark_fcb_dirty(fileref->fcb);
    }

    // remove dir_child from parent

    if (fileref->dc) {
        TRACE("delete file %.*S\n", (int)(fileref->dc->name.Length / sizeof(WCHAR)), fileref->dc->name.Buffer);

        ExAcquireResourceExclusiveLite(&fileref->parent->fcb->nonpaged->dir_children_lock, true);
        RemoveEntryList(&fileref->dc->list_entry_index);

        if (!fileref->fcb->ads)
            remove_dir_child_from_hash_lists(fileref->parent->fcb, fileref->dc);

        ExReleaseResourceLite(&fileref->parent->fcb->nonpaged->dir_children_lock);

        if (!fileref->oldutf8.Buffer)
            fileref->oldutf8 = fileref->dc->utf8;
        else
            ExFreePool(fileref->dc->utf8.Buffer);

        utf8len = fileref->dc->utf8.Length;

        fileref->oldindex = fileref->dc->index;

        ExFreePool(fileref->dc->name.Buffer);
        ExFreePool(fileref->dc->name_uc.Buffer);
        ExFreePool(fileref->dc);

        fileref->dc = NULL;
    }

    // update INODE_ITEM of parent

    ExAcquireResourceExclusiveLite(fileref->parent->fcb->Header.Resource, true);

    fileref->parent->fcb->inode_item.transid = fileref->fcb->Vcb->superblock.generation;
    fileref->parent->fcb->inode_item.sequence++;
    fileref->parent->fcb->inode_item.st_ctime = now;

    if (!fileref->fcb->ads) {
        TRACE("fileref->parent->fcb->inode_item.st_size (inode %I64x) was %I64x\n", fileref->parent->fcb->inode, fileref->parent->fcb->inode_item.st_size);
        fileref->parent->fcb->inode_item.st_size -= utf8len * 2;
        TRACE("fileref->parent->fcb->inode_item.st_size (inode %I64x) now %I64x\n", fileref->parent->fcb->inode, fileref->parent->fcb->inode_item.st_size);
        fileref->parent->fcb->inode_item.st_mtime = now;
    }

    fileref->parent->fcb->inode_item_changed = true;
    ExReleaseResourceLite(fileref->parent->fcb->Header.Resource);

    if (!fileref->fcb->ads && fileref->parent->dc)
        send_notification_fcb(fileref->parent, FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_ACTION_MODIFIED, NULL);

    mark_fcb_dirty(fileref->parent->fcb);

    fileref->fcb->subvol->root_item.ctransid = fileref->fcb->Vcb->superblock.generation;
    fileref->fcb->subvol->root_item.ctime = now;

    newlength.QuadPart = 0;

    if (FileObject && !CcUninitializeCacheMap(FileObject, &newlength, NULL))
        TRACE("CcUninitializeCacheMap failed\n");

    ExReleaseResourceLite(fileref->fcb->Header.Resource);

    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_CLEANUP)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_cleanup(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    fcb* fcb = FileObject->FsContext;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("cleanup\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;
        goto exit;
    } else if (DeviceObject == master_devobj) {
        TRACE("closing file system\n");
        Status = STATUS_SUCCESS;
        goto exit;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (FileObject->Flags & FO_CLEANUP_COMPLETE) {
        TRACE("FileObject %p already cleaned up\n", FileObject);
        Status = STATUS_SUCCESS;
        goto exit;
    }

    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

    // We have to use the pointer to Vcb stored in the fcb, as we can receive cleanup
    // messages belonging to other devices.

    if (FileObject && FileObject->FsContext) {
        ccb* ccb;
        file_ref* fileref;
        bool locked = true;

        ccb = FileObject->FsContext2;
        fileref = ccb ? ccb->fileref : NULL;

        TRACE("cleanup called for FileObject %p\n", FileObject);
        TRACE("fileref %p, refcount = %li, open_count = %li\n", fileref, fileref ? fileref->refcount : 0, fileref ? fileref->open_count : 0);

        ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);

        ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

        IoRemoveShareAccess(FileObject, &fcb->share_access);

        FsRtlFastUnlockAll(&fcb->lock, FileObject, IoGetRequestorProcess(Irp), NULL);

        if (ccb)
            FsRtlNotifyCleanup(fcb->Vcb->NotifySync, &fcb->Vcb->DirNotifyList, ccb);

        if (ccb && ccb->options & FILE_DELETE_ON_CLOSE && fileref)
            fileref->delete_on_close = true;

        if (fileref && fileref->delete_on_close && fcb->type == BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0 && fcb != fcb->Vcb->dummy_fcb)
            fileref->delete_on_close = false;

        if (fcb->Vcb->locked && fcb->Vcb->locked_fileobj == FileObject) {
            TRACE("unlocking volume\n");
            do_unlock_volume(fcb->Vcb);
            FsRtlNotifyVolumeEvent(FileObject, FSRTL_VOLUME_UNLOCK);
        }

        if (ccb && ccb->reserving) {
            fcb->subvol->reserved = NULL;
            ccb->reserving = false;
            // FIXME - flush all of subvol's fcbs
        }

        if (fileref) {
            LONG oc = InterlockedDecrement(&fileref->open_count);
#ifdef DEBUG_FCB_REFCOUNTS
            ERR("fileref %p: open_count now %i\n", fileref, oc);
#endif

            if (oc == 0 || (fileref->delete_on_close && fileref->posix_delete)) {
                if (!fcb->Vcb->removing) {
                    if (oc == 0 && fileref->fcb->inode_item.st_nlink == 0 && fileref != fcb->Vcb->root_fileref &&
                        fcb != fcb->Vcb->volume_fcb && !fcb->ads) { // last handle closed on POSIX-deleted file
                        LIST_ENTRY rollback;

                        InitializeListHead(&rollback);

                        Status = delete_fileref_fcb(fileref, FileObject, Irp, &rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_fileref_fcb returned %08lx\n", Status);
                            do_rollback(fcb->Vcb, &rollback);
                            ExReleaseResourceLite(fileref->fcb->Header.Resource);
                            ExReleaseResourceLite(&fcb->Vcb->tree_lock);
                            goto exit;
                        }

                        clear_rollback(&rollback);

                        mark_fcb_dirty(fileref->fcb);
                    } else if (fileref->delete_on_close && fileref != fcb->Vcb->root_fileref && fcb != fcb->Vcb->volume_fcb) {
                        LIST_ENTRY rollback;

                        InitializeListHead(&rollback);

                        if (!fileref->fcb->ads || fileref->dc) {
                            if (fileref->fcb->ads) {
                                send_notification_fileref(fileref->parent, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                                        FILE_ACTION_REMOVED, &fileref->dc->name);
                            } else
                                send_notification_fileref(fileref, fcb->type == BTRFS_TYPE_DIRECTORY ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME, FILE_ACTION_REMOVED, NULL);
                        }

                        ExReleaseResourceLite(fcb->Header.Resource);
                        locked = false;

                        // fileref_lock needs to be acquired before fcb->Header.Resource
                        ExAcquireResourceExclusiveLite(&fcb->Vcb->fileref_lock, true);

                        Status = delete_fileref(fileref, FileObject, oc > 0 && fileref->posix_delete, Irp, &rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_fileref returned %08lx\n", Status);
                            do_rollback(fcb->Vcb, &rollback);
                            ExReleaseResourceLite(&fcb->Vcb->fileref_lock);
                            ExReleaseResourceLite(&fcb->Vcb->tree_lock);
                            goto exit;
                        }

                        ExReleaseResourceLite(&fcb->Vcb->fileref_lock);

                        clear_rollback(&rollback);
                    } else if (FileObject->Flags & FO_CACHE_SUPPORTED && FileObject->SectionObjectPointer->DataSectionObject) {
                        IO_STATUS_BLOCK iosb;

                        if (locked) {
                            ExReleaseResourceLite(fcb->Header.Resource);
                            locked = false;
                        }

                        CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &iosb);

                        if (!NT_SUCCESS(iosb.Status))
                            ERR("CcFlushCache returned %08lx\n", iosb.Status);

                        if (!ExIsResourceAcquiredSharedLite(fcb->Header.PagingIoResource)) {
                            ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, true);
                            ExReleaseResourceLite(fcb->Header.PagingIoResource);
                        }

                        CcPurgeCacheSection(FileObject->SectionObjectPointer, NULL, 0, false);

                        TRACE("flushed cache on close (FileObject = %p, fcb = %p, AllocationSize = %I64x, FileSize = %I64x, ValidDataLength = %I64x)\n",
                            FileObject, fcb, fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
                    }
                }

                if (fcb->Vcb && fcb != fcb->Vcb->volume_fcb)
                    CcUninitializeCacheMap(FileObject, NULL, NULL);
            }
        }

        if (locked)
            ExReleaseResourceLite(fcb->Header.Resource);

        ExReleaseResourceLite(&fcb->Vcb->tree_lock);

        FileObject->Flags |= FO_CLEANUP_COMPLETE;
    }

    Status = STATUS_SUCCESS;

exit:
    TRACE("returning %08lx\n", Status);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

_Success_(return)
bool get_file_attributes_from_xattr(_In_reads_bytes_(len) char* val, _In_ uint16_t len, _Out_ ULONG* atts) {
    if (len > 2 && val[0] == '0' && val[1] == 'x') {
        int i;
        ULONG dosnum = 0;

        for (i = 2; i < len; i++) {
            dosnum *= 0x10;

            if (val[i] >= '0' && val[i] <= '9')
                dosnum |= val[i] - '0';
            else if (val[i] >= 'a' && val[i] <= 'f')
                dosnum |= val[i] + 10 - 'a';
            else if (val[i] >= 'A' && val[i] <= 'F')
                dosnum |= val[i] + 10 - 'a';
        }

        TRACE("DOSATTRIB: %08lx\n", dosnum);

        *atts = dosnum;

        return true;
    }

    return false;
}

ULONG get_file_attributes(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ uint64_t inode,
                          _In_ uint8_t type, _In_ bool dotfile, _In_ bool ignore_xa, _In_opt_ PIRP Irp) {
    ULONG att;
    char* eaval;
    uint16_t ealen;

    if (!ignore_xa && get_xattr(Vcb, r, inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (uint8_t**)&eaval, &ealen, Irp)) {
        ULONG dosnum = 0;

        if (get_file_attributes_from_xattr(eaval, ealen, &dosnum)) {
            ExFreePool(eaval);

            if (type == BTRFS_TYPE_DIRECTORY)
                dosnum |= FILE_ATTRIBUTE_DIRECTORY;
            else if (type == BTRFS_TYPE_SYMLINK)
                dosnum |= FILE_ATTRIBUTE_REPARSE_POINT;

            if (type != BTRFS_TYPE_DIRECTORY)
                dosnum &= ~FILE_ATTRIBUTE_DIRECTORY;

            if (inode == SUBVOL_ROOT_INODE) {
                if (r->root_item.flags & BTRFS_SUBVOL_READONLY)
                    dosnum |= FILE_ATTRIBUTE_READONLY;
                else
                    dosnum &= ~FILE_ATTRIBUTE_READONLY;
            }

            return dosnum;
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

    if (dotfile || (r->id == BTRFS_ROOT_FSTREE && inode == SUBVOL_ROOT_INODE))
        att |= FILE_ATTRIBUTE_HIDDEN;

    att |= FILE_ATTRIBUTE_ARCHIVE;

    if (inode == SUBVOL_ROOT_INODE) {
        if (r->root_item.flags & BTRFS_SUBVOL_READONLY)
            att |= FILE_ATTRIBUTE_READONLY;
        else
            att &= ~FILE_ATTRIBUTE_READONLY;
    }

    // FIXME - get READONLY from ii->st_mode
    // FIXME - return SYSTEM for block/char devices?

    if (att == 0)
        att = FILE_ATTRIBUTE_NORMAL;

    return att;
}

NTSTATUS sync_read_phys(_In_ PDEVICE_OBJECT DeviceObject, _In_ PFILE_OBJECT FileObject, _In_ uint64_t StartingOffset, _In_ ULONG Length,
                        _Out_writes_bytes_(Length) PUCHAR Buffer, _In_ bool override) {
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    read_context context;

    num_reads++;

    RtlZeroMemory(&context, sizeof(read_context));
    KeInitializeEvent(&context.Event, NotificationEvent, false);

    Offset.QuadPart = (LONGLONG)StartingOffset;

    Irp = IoAllocateIrp(DeviceObject->StackSize, false);

    if (!Irp) {
        ERR("IoAllocateIrp failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->Flags |= IRP_NOCACHE;
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_READ;
    IrpSp->FileObject = FileObject;

    if (override)
        IrpSp->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, ALLOC_TAG);
        if (!Irp->AssociatedIrp.SystemBuffer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;

        Irp->UserBuffer = Buffer;
    } else if (DeviceObject->Flags & DO_DIRECT_IO) {
        Irp->MdlAddress = IoAllocateMdl(Buffer, Length, false, false, NULL);
        if (!Irp->MdlAddress) {
            ERR("IoAllocateMdl failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoWriteAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(Irp->MdlAddress);
            goto exit;
        }
    } else
        Irp->UserBuffer = Buffer;

    IrpSp->Parameters.Read.Length = Length;
    IrpSp->Parameters.Read.ByteOffset = Offset;

    Irp->UserIosb = &IoStatus;

    Irp->UserEvent = &context.Event;

    IoSetCompletionRoutine(Irp, read_completion, &context, true, true, true);

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, false, NULL);
        Status = context.iosb.Status;
    }

    if (DeviceObject->Flags & DO_DIRECT_IO) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

exit:
    IoFreeIrp(Irp);

    return Status;
}

bool check_superblock_checksum(superblock* sb) {
    switch (sb->csum_type) {
        case CSUM_TYPE_CRC32C: {
            uint32_t crc32 = ~calc_crc32c(0xffffffff, (uint8_t*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));

            if (crc32 == *((uint32_t*)sb->checksum))
                return true;

            WARN("crc32 was %08x, expected %08x\n", crc32, *((uint32_t*)sb->checksum));

            break;
        }

        case CSUM_TYPE_XXHASH: {
            uint64_t hash = XXH64(&sb->uuid, sizeof(superblock) - sizeof(sb->checksum), 0);

            if (hash == *((uint64_t*)sb->checksum))
                return true;

            WARN("superblock hash was %I64x, expected %I64x\n", hash, *((uint64_t*)sb->checksum));

            break;
        }

        case CSUM_TYPE_SHA256: {
            uint8_t hash[SHA256_HASH_SIZE];

            calc_sha256(hash, &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));

            if (RtlCompareMemory(hash, sb, SHA256_HASH_SIZE) == SHA256_HASH_SIZE)
                return true;

            WARN("superblock hash was invalid\n");

            break;
        }

        case CSUM_TYPE_BLAKE2: {
            uint8_t hash[BLAKE2_HASH_SIZE];

            blake2b(hash, sizeof(hash), &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));

            if (RtlCompareMemory(hash, sb, BLAKE2_HASH_SIZE) == BLAKE2_HASH_SIZE)
                return true;

            WARN("superblock hash was invalid\n");

            break;
        }

        default:
            WARN("unrecognized csum type %x\n", sb->csum_type);
    }

    return false;
}

static NTSTATUS read_superblock(_In_ device_extension* Vcb, _In_ PDEVICE_OBJECT device, _In_ PFILE_OBJECT fileobj, _In_ uint64_t length) {
    NTSTATUS Status;
    superblock* sb;
    ULONG i, to_read;
    uint8_t valid_superblocks;

    to_read = device->SectorSize == 0 ? sizeof(superblock) : (ULONG)sector_align(sizeof(superblock), device->SectorSize);

    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (superblock_addrs[0] + to_read > length) {
        WARN("device was too short to have any superblock\n");
        ExFreePool(sb);
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    i = 0;
    valid_superblocks = 0;

    while (superblock_addrs[i] > 0) {
        if (i > 0 && superblock_addrs[i] + to_read > length)
            break;

        Status = sync_read_phys(device, fileobj, superblock_addrs[i], to_read, (PUCHAR)sb, false);
        if (!NT_SUCCESS(Status)) {
            ERR("Failed to read superblock %lu: %08lx\n", i, Status);
            ExFreePool(sb);
            return Status;
        }

        if (sb->magic != BTRFS_MAGIC) {
            if (i == 0) {
                TRACE("not a BTRFS volume\n");
                ExFreePool(sb);
                return STATUS_UNRECOGNIZED_VOLUME;
            }
        } else {
            TRACE("got superblock %lu!\n", i);

            if (sb->sector_size == 0)
                WARN("superblock sector size was 0\n");
            else if (sb->sector_size & (sb->sector_size - 1))
                WARN("superblock sector size was not power of 2\n");
            else if (sb->node_size < sizeof(tree_header) + sizeof(internal_node) || sb->node_size > 0x10000)
                WARN("invalid node size %x\n", sb->node_size);
            else if ((sb->node_size % sb->sector_size) != 0)
                WARN("node size %x was not a multiple of sector_size %x\n", sb->node_size, sb->sector_size);
            else if (check_superblock_checksum(sb) && (valid_superblocks == 0 || sb->generation > Vcb->superblock.generation)) {
                RtlCopyMemory(&Vcb->superblock, sb, sizeof(superblock));
                valid_superblocks++;
            }
        }

        i++;
    }

    ExFreePool(sb);

    if (valid_superblocks == 0) {
        ERR("could not find any valid superblocks\n");
        return STATUS_INTERNAL_ERROR;
    }

    TRACE("label is %s\n", Vcb->superblock.label);

    return STATUS_SUCCESS;
}

NTSTATUS dev_ioctl(_In_ PDEVICE_OBJECT DeviceObject, _In_ ULONG ControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize,
                   _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _In_ bool Override, _Out_opt_ IO_STATUS_BLOCK* iosb) {
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    IO_STATUS_BLOCK IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, false);

    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        OutputBufferSize,
                                        false,
                                        &Event,
                                        &IoStatus);

    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    if (Override) {
        IrpSp = IoGetNextIrpStackLocation(Irp);
        IrpSp->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, false, NULL);
        Status = IoStatus.Status;
    }

    if (iosb)
        *iosb = IoStatus;

    return Status;
}

_Requires_exclusive_lock_held_(Vcb->tree_lock)
static NTSTATUS add_root(_Inout_ device_extension* Vcb, _In_ uint64_t id, _In_ uint64_t addr,
                         _In_ uint64_t generation, _In_opt_ traverse_ptr* tp) {
    root* r = ExAllocatePoolWithTag(PagedPool, sizeof(root), ALLOC_TAG);
    if (!r) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    r->id = id;
    r->dirty = false;
    r->received = false;
    r->reserved = NULL;
    r->treeholder.address = addr;
    r->treeholder.tree = NULL;
    r->treeholder.generation = generation;
    r->parent = 0;
    r->send_ops = 0;
    r->fcbs_version = 0;
    r->checked_for_orphans = false;
    r->dropped = false;
    InitializeListHead(&r->fcbs);
    RtlZeroMemory(r->fcbs_ptrs, sizeof(LIST_ENTRY*) * 256);

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
            RtlZeroMemory(((uint8_t*)&r->root_item) + tp->item->size, sizeof(ROOT_ITEM) - tp->item->size);
    } else
        RtlZeroMemory(&r->root_item, sizeof(ROOT_ITEM));

    if (!Vcb->readonly && (r->id == BTRFS_ROOT_ROOT || r->id == BTRFS_ROOT_FSTREE || (r->id >= 0x100 && !(r->id & 0xf000000000000000)))) { // FS tree root
        // FIXME - don't call this if subvol is readonly (though we will have to if we ever toggle this flag)
        get_last_inode(Vcb, r, NULL);

        if (r->id == BTRFS_ROOT_ROOT && r->lastinode < 0x100)
            r->lastinode = 0x100;
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

        case BTRFS_ROOT_FREE_SPACE:
            Vcb->space_root = r;
            break;

        case BTRFS_ROOT_DATA_RELOC:
            Vcb->data_reloc_root = r;
            break;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS look_for_roots(_Requires_exclusive_lock_held_(_Curr_->tree_lock) _In_ device_extension* Vcb, _In_opt_ PIRP Irp) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    bool b;
    NTSTATUS Status;

    searchkey.obj_id = 0;
    searchkey.obj_type = 0;
    searchkey.offset = 0;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    do {
        TRACE("(%I64x,%x,%I64x)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

        if (tp.item->key.obj_type == TYPE_ROOT_ITEM) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;

            if (tp.item->size < offsetof(ROOT_ITEM, byte_limit)) {
                ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, offsetof(ROOT_ITEM, byte_limit));
            } else {
                TRACE("root %I64x - address %I64x\n", tp.item->key.obj_id, ri->block_number);

                Status = add_root(Vcb, tp.item->key.obj_id, ri->block_number, ri->generation, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_root returned %08lx\n", Status);
                    return Status;
                }
            }
        } else if (tp.item->key.obj_type == TYPE_ROOT_BACKREF && !IsListEmpty(&Vcb->roots)) {
            root* lastroot = CONTAINING_RECORD(Vcb->roots.Blink, root, list_entry);

            if (lastroot->id == tp.item->key.obj_id)
                lastroot->parent = tp.item->key.offset;
        }

        b = find_next_item(Vcb, &tp, &next_tp, false, Irp);

        if (b)
            tp = next_tp;
    } while (b);

    if (!Vcb->readonly && !Vcb->data_reloc_root) {
        root* reloc_root;
        INODE_ITEM* ii;
        uint16_t irlen;
        INODE_REF* ir;
        LARGE_INTEGER time;
        BTRFS_TIME now;

        WARN("data reloc root doesn't exist, creating it\n");

        Status = create_root(Vcb, BTRFS_ROOT_DATA_RELOC, &reloc_root, false, 0, Irp);

        if (!NT_SUCCESS(Status)) {
            ERR("create_root returned %08lx\n", Status);
            return Status;
        }

        reloc_root->root_item.inode.generation = 1;
        reloc_root->root_item.inode.st_size = 3;
        reloc_root->root_item.inode.st_blocks = Vcb->superblock.node_size;
        reloc_root->root_item.inode.st_nlink = 1;
        reloc_root->root_item.inode.st_mode = 040755;
        reloc_root->root_item.inode.flags = 0x80000000;
        reloc_root->root_item.inode.flags_ro = 0xffffffff;
        reloc_root->root_item.objid = SUBVOL_ROOT_INODE;
        reloc_root->root_item.bytes_used = Vcb->superblock.node_size;

        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);

        RtlZeroMemory(ii, sizeof(INODE_ITEM));
        ii->generation = Vcb->superblock.generation;
        ii->st_blocks = Vcb->superblock.node_size;
        ii->st_nlink = 1;
        ii->st_mode = 040755;
        ii->st_atime = now;
        ii->st_ctime = now;
        ii->st_mtime = now;

        Status = insert_tree_item(Vcb, reloc_root, SUBVOL_ROOT_INODE, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            ExFreePool(ii);
            return Status;
        }

        irlen = (uint16_t)offsetof(INODE_REF, name[0]) + 2;
        ir = ExAllocatePoolWithTag(PagedPool, irlen, ALLOC_TAG);
        if (!ir) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ir->index = 0;
        ir->n = 2;
        ir->name[0] = '.';
        ir->name[1] = '.';

        Status = insert_tree_item(Vcb, reloc_root, SUBVOL_ROOT_INODE, TYPE_INODE_REF, SUBVOL_ROOT_INODE, ir, irlen, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08lx\n", Status);
            ExFreePool(ir);
            return Status;
        }

        Vcb->data_reloc_root = reloc_root;
        Vcb->need_write = true;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS find_disk_holes(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ device* dev, _In_opt_ PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    bool b;
    uint64_t lastaddr;
    NTSTATUS Status;

    InitializeListHead(&dev->space);

    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = dev->devitem.dev_id;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, false, Irp);
    if (NT_SUCCESS(Status) && !keycmp(tp.item->key, searchkey))
        RtlCopyMemory(dev->stats, tp.item->data, min(sizeof(uint64_t) * 5, tp.item->size));

    searchkey.obj_id = dev->devitem.dev_id;
    searchkey.obj_type = TYPE_DEV_EXTENT;
    searchkey.offset = 0;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    lastaddr = 0;

    do {
        if (tp.item->key.obj_id == dev->devitem.dev_id && tp.item->key.obj_type == TYPE_DEV_EXTENT) {
            if (tp.item->size >= sizeof(DEV_EXTENT)) {
                DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;

                if (tp.item->key.offset > lastaddr) {
                    Status = add_space_entry(&dev->space, NULL, lastaddr, tp.item->key.offset - lastaddr);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_space_entry returned %08lx\n", Status);
                        return Status;
                    }
                }

                lastaddr = tp.item->key.offset + de->length;
            } else {
                ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DEV_EXTENT));
            }
        }

        b = find_next_item(Vcb, &tp, &next_tp, false, Irp);

        if (b) {
            tp = next_tp;
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);

    if (lastaddr < dev->devitem.num_bytes) {
        Status = add_space_entry(&dev->space, NULL, lastaddr, dev->devitem.num_bytes - lastaddr);
        if (!NT_SUCCESS(Status)) {
            ERR("add_space_entry returned %08lx\n", Status);
            return Status;
        }
    }

    // The Linux driver doesn't like to allocate chunks within the first megabyte of a device.

    space_list_subtract2(&dev->space, NULL, 0, 0x100000, NULL, NULL);

    return STATUS_SUCCESS;
}

static void add_device_to_list(_In_ device_extension* Vcb, _In_ device* dev) {
    LIST_ENTRY* le;

    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);

        if (dev2->devitem.dev_id > dev->devitem.dev_id) {
            InsertHeadList(le->Blink, &dev->list_entry);
            return;
        }

        le = le->Flink;
    }

    InsertTailList(&Vcb->devices, &dev->list_entry);
}

_Ret_maybenull_
device* find_device_from_uuid(_In_ device_extension* Vcb, _In_ BTRFS_UUID* uuid) {
    volume_device_extension* vde;
    pdo_device_extension* pdode;
    LIST_ENTRY* le;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        TRACE("device %I64x, uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", dev->devitem.dev_id,
            dev->devitem.device_uuid.uuid[0], dev->devitem.device_uuid.uuid[1], dev->devitem.device_uuid.uuid[2], dev->devitem.device_uuid.uuid[3], dev->devitem.device_uuid.uuid[4], dev->devitem.device_uuid.uuid[5], dev->devitem.device_uuid.uuid[6], dev->devitem.device_uuid.uuid[7],
            dev->devitem.device_uuid.uuid[8], dev->devitem.device_uuid.uuid[9], dev->devitem.device_uuid.uuid[10], dev->devitem.device_uuid.uuid[11], dev->devitem.device_uuid.uuid[12], dev->devitem.device_uuid.uuid[13], dev->devitem.device_uuid.uuid[14], dev->devitem.device_uuid.uuid[15]);

        if (RtlCompareMemory(&dev->devitem.device_uuid, uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            TRACE("returning device %I64x\n", dev->devitem.dev_id);
            return dev;
        }

        le = le->Flink;
    }

    vde = Vcb->vde;

    if (!vde)
        goto end;

    pdode = vde->pdode;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (Vcb->devices_loaded < Vcb->superblock.num_devices) {
        le = pdode->children.Flink;

        while (le != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

            if (RtlCompareMemory(uuid, &vc->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                device* dev;

                dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
                if (!dev) {
                    ExReleaseResourceLite(&pdode->child_lock);
                    ERR("out of memory\n");
                    return NULL;
                }

                RtlZeroMemory(dev, sizeof(device));
                dev->devobj = vc->devobj;
                dev->fileobj = vc->fileobj;
                dev->devitem.device_uuid = *uuid;
                dev->devitem.dev_id = vc->devid;
                dev->devitem.num_bytes = vc->size;
                dev->seeding = vc->seeding;
                dev->readonly = dev->seeding;
                dev->reloc = false;
                dev->removable = false;
                dev->disk_num = vc->disk_num;
                dev->part_num = vc->part_num;
                dev->num_trim_entries = 0;
                InitializeListHead(&dev->trim_list);

                add_device_to_list(Vcb, dev);
                Vcb->devices_loaded++;

                ExReleaseResourceLite(&pdode->child_lock);

                return dev;
            }

            le = le->Flink;
        }
    }

    ExReleaseResourceLite(&pdode->child_lock);

end:
    WARN("could not find device with uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
         uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3], uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
         uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11], uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);

    return NULL;
}

static bool is_device_removable(_In_ PDEVICE_OBJECT devobj) {
    NTSTATUS Status;
    STORAGE_HOTPLUG_INFO shi;

    Status = dev_ioctl(devobj, IOCTL_STORAGE_GET_HOTPLUG_INFO, NULL, 0, &shi, sizeof(STORAGE_HOTPLUG_INFO), true, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("dev_ioctl returned %08lx\n", Status);
        return false;
    }

    return shi.MediaRemovable != 0 ? true : false;
}

static ULONG get_device_change_count(_In_ PDEVICE_OBJECT devobj) {
    NTSTATUS Status;
    ULONG cc;
    IO_STATUS_BLOCK iosb;

    Status = dev_ioctl(devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), true, &iosb);

    if (!NT_SUCCESS(Status)) {
        ERR("dev_ioctl returned %08lx\n", Status);
        return 0;
    }

    if (iosb.Information < sizeof(ULONG)) {
        ERR("iosb.Information was too short\n");
        return 0;
    }

    return cc;
}

void init_device(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ bool get_nums) {
    NTSTATUS Status;
    ULONG aptelen;
    ATA_PASS_THROUGH_EX* apte;
    STORAGE_PROPERTY_QUERY spq;
    DEVICE_TRIM_DESCRIPTOR dtd;

    dev->removable = is_device_removable(dev->devobj);
    dev->change_count = dev->removable ? get_device_change_count(dev->devobj) : 0;

    if (get_nums) {
        STORAGE_DEVICE_NUMBER sdn;

        Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                           &sdn, sizeof(STORAGE_DEVICE_NUMBER), true, NULL);

        if (!NT_SUCCESS(Status)) {
            WARN("IOCTL_STORAGE_GET_DEVICE_NUMBER returned %08lx\n", Status);
            dev->disk_num = 0xffffffff;
            dev->part_num = 0xffffffff;
        } else {
            dev->disk_num = sdn.DeviceNumber;
            dev->part_num = sdn.PartitionNumber;
        }
    }

    dev->trim = false;
    dev->readonly = dev->seeding;
    dev->reloc = false;
    dev->num_trim_entries = 0;
    dev->stats_changed = false;
    InitializeListHead(&dev->trim_list);

    if (!dev->readonly) {
        Status = dev_ioctl(dev->devobj, IOCTL_DISK_IS_WRITABLE, NULL, 0,
                        NULL, 0, true, NULL);
        if (Status == STATUS_MEDIA_WRITE_PROTECTED)
            dev->readonly = true;
    }

    aptelen = sizeof(ATA_PASS_THROUGH_EX) + 512;
    apte = ExAllocatePoolWithTag(NonPagedPool, aptelen, ALLOC_TAG);
    if (!apte) {
        ERR("out of memory\n");
        return;
    }

    RtlZeroMemory(apte, aptelen);

    apte->Length = sizeof(ATA_PASS_THROUGH_EX);
    apte->AtaFlags = ATA_FLAGS_DATA_IN;
    apte->DataTransferLength = aptelen - sizeof(ATA_PASS_THROUGH_EX);
    apte->TimeOutValue = 3;
    apte->DataBufferOffset = apte->Length;
    apte->CurrentTaskFile[6] = IDE_COMMAND_IDENTIFY;

    Status = dev_ioctl(dev->devobj, IOCTL_ATA_PASS_THROUGH, apte, aptelen,
                       apte, aptelen, true, NULL);

    if (!NT_SUCCESS(Status))
        TRACE("IOCTL_ATA_PASS_THROUGH returned %08lx for IDENTIFY DEVICE\n", Status);
    else {
        IDENTIFY_DEVICE_DATA* idd = (IDENTIFY_DEVICE_DATA*)((uint8_t*)apte + sizeof(ATA_PASS_THROUGH_EX));

        if (idd->CommandSetSupport.FlushCache) {
            dev->can_flush = true;
            TRACE("FLUSH CACHE supported\n");
        } else
            TRACE("FLUSH CACHE not supported\n");
    }

    ExFreePool(apte);

#ifdef DEBUG_TRIM_EMULATION
    dev->trim = true;
    Vcb->trim = true;
#else
    spq.PropertyId = StorageDeviceTrimProperty;
    spq.QueryType = PropertyStandardQuery;
    spq.AdditionalParameters[0] = 0;

    Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
                       &dtd, sizeof(DEVICE_TRIM_DESCRIPTOR), true, NULL);

    if (NT_SUCCESS(Status)) {
        if (dtd.TrimEnabled) {
            dev->trim = true;
            Vcb->trim = true;
            TRACE("TRIM supported\n");
        } else
            TRACE("TRIM not supported\n");
    }
#endif

    RtlZeroMemory(dev->stats, sizeof(uint64_t) * 5);
}

static NTSTATUS load_chunk_root(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp) {
    traverse_ptr tp, next_tp;
    KEY searchkey;
    bool b;
    chunk* c;
    NTSTATUS Status;

    searchkey.obj_id = 0;
    searchkey.obj_type = 0;
    searchkey.offset = 0;

    Vcb->data_flags = 0;
    Vcb->metadata_flags = 0;
    Vcb->system_flags = 0;

    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        return Status;
    }

    do {
        TRACE("(%I64x,%x,%I64x)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

        if (tp.item->key.obj_id == 1 && tp.item->key.obj_type == TYPE_DEV_ITEM) {
            if (tp.item->size < sizeof(DEV_ITEM)) {
                ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DEV_ITEM));
            } else {
                DEV_ITEM* di = (DEV_ITEM*)tp.item->data;
                LIST_ENTRY* le;
                bool done = false;

                le = Vcb->devices.Flink;
                while (le != &Vcb->devices) {
                    device* dev = CONTAINING_RECORD(le, device, list_entry);

                    if (dev->devobj && RtlCompareMemory(&dev->devitem.device_uuid, &di->device_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                        RtlCopyMemory(&dev->devitem, tp.item->data, min(tp.item->size, sizeof(DEV_ITEM)));

                        if (le != Vcb->devices.Flink)
                            init_device(Vcb, dev, true);

                        done = true;
                        break;
                    }

                    le = le->Flink;
                }

                if (!done && Vcb->vde) {
                    volume_device_extension* vde = Vcb->vde;
                    pdo_device_extension* pdode = vde->pdode;

                    ExAcquireResourceSharedLite(&pdode->child_lock, true);

                    if (Vcb->devices_loaded < Vcb->superblock.num_devices) {
                        le = pdode->children.Flink;

                        while (le != &pdode->children) {
                            volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

                            if (RtlCompareMemory(&di->device_uuid, &vc->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                                device* dev;

                                dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
                                if (!dev) {
                                    ExReleaseResourceLite(&pdode->child_lock);
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                RtlZeroMemory(dev, sizeof(device));

                                dev->devobj = vc->devobj;
                                dev->fileobj = vc->fileobj;
                                RtlCopyMemory(&dev->devitem, di, min(tp.item->size, sizeof(DEV_ITEM)));
                                dev->seeding = vc->seeding;
                                init_device(Vcb, dev, false);

                                if (dev->devitem.num_bytes > vc->size) {
                                    WARN("device %I64x: DEV_ITEM says %I64x bytes, but Windows only reports %I64x\n", tp.item->key.offset,
                                         dev->devitem.num_bytes, vc->size);

                                    dev->devitem.num_bytes = vc->size;
                                }

                                dev->disk_num = vc->disk_num;
                                dev->part_num = vc->part_num;
                                add_device_to_list(Vcb, dev);
                                Vcb->devices_loaded++;

                                done = true;
                                break;
                            }

                            le = le->Flink;
                        }

                        if (!done) {
                            if (!Vcb->options.allow_degraded) {
                                ERR("volume not found: device %I64x, uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", tp.item->key.offset,
                                    di->device_uuid.uuid[0], di->device_uuid.uuid[1], di->device_uuid.uuid[2], di->device_uuid.uuid[3], di->device_uuid.uuid[4], di->device_uuid.uuid[5], di->device_uuid.uuid[6], di->device_uuid.uuid[7],
                                    di->device_uuid.uuid[8], di->device_uuid.uuid[9], di->device_uuid.uuid[10], di->device_uuid.uuid[11], di->device_uuid.uuid[12], di->device_uuid.uuid[13], di->device_uuid.uuid[14], di->device_uuid.uuid[15]);
                            } else {
                                device* dev;

                                dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
                                if (!dev) {
                                    ExReleaseResourceLite(&pdode->child_lock);
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                RtlZeroMemory(dev, sizeof(device));

                                // Missing device, so we keep dev->devobj as NULL
                                RtlCopyMemory(&dev->devitem, di, min(tp.item->size, sizeof(DEV_ITEM)));
                                InitializeListHead(&dev->trim_list);

                                add_device_to_list(Vcb, dev);
                                Vcb->devices_loaded++;
                            }
                        }
                    } else
                        ERR("unexpected device %I64x found\n", tp.item->key.offset);

                    ExReleaseResourceLite(&pdode->child_lock);
                }
            }
        } else if (tp.item->key.obj_type == TYPE_CHUNK_ITEM) {
            if (tp.item->size < sizeof(CHUNK_ITEM)) {
                ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(CHUNK_ITEM));
            } else {
                c = ExAllocatePoolWithTag(NonPagedPool, sizeof(chunk), ALLOC_TAG);

                if (!c) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                c->size = tp.item->size;
                c->offset = tp.item->key.offset;
                c->used = c->oldused = 0;
                c->cache = c->old_cache = NULL;
                c->created = false;
                c->readonly = false;
                c->reloc = false;
                c->cache_loaded = false;
                c->changed = false;
                c->space_changed = false;
                c->balance_num = 0;

                c->chunk_item = ExAllocatePoolWithTag(NonPagedPool, tp.item->size, ALLOC_TAG);

                if (!c->chunk_item) {
                    ERR("out of memory\n");
                    ExFreePool(c);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(c->chunk_item, tp.item->data, tp.item->size);

                if (c->chunk_item->type & BLOCK_FLAG_DATA && c->chunk_item->type > Vcb->data_flags)
                    Vcb->data_flags = c->chunk_item->type;

                if (c->chunk_item->type & BLOCK_FLAG_METADATA && c->chunk_item->type > Vcb->metadata_flags)
                    Vcb->metadata_flags = c->chunk_item->type;

                if (c->chunk_item->type & BLOCK_FLAG_SYSTEM && c->chunk_item->type > Vcb->system_flags)
                    Vcb->system_flags = c->chunk_item->type;

                if (c->chunk_item->type & BLOCK_FLAG_RAID10) {
                    if (c->chunk_item->sub_stripes == 0 || c->chunk_item->sub_stripes > c->chunk_item->num_stripes) {
                        ERR("chunk %I64x: invalid stripes (num_stripes %u, sub_stripes %u)\n", c->offset, c->chunk_item->num_stripes, c->chunk_item->sub_stripes);
                        ExFreePool(c->chunk_item);
                        ExFreePool(c);
                        return STATUS_INTERNAL_ERROR;
                    }
                }

                if (c->chunk_item->num_stripes > 0) {
                    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
                    uint16_t i;

                    c->devices = ExAllocatePoolWithTag(NonPagedPool, sizeof(device*) * c->chunk_item->num_stripes, ALLOC_TAG);

                    if (!c->devices) {
                        ERR("out of memory\n");
                        ExFreePool(c->chunk_item);
                        ExFreePool(c);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    for (i = 0; i < c->chunk_item->num_stripes; i++) {
                        c->devices[i] = find_device_from_uuid(Vcb, &cis[i].dev_uuid);
                        TRACE("device %u = %p\n", i, c->devices[i]);

                        if (!c->devices[i]) {
                            ERR("missing device\n");
                            ExFreePool(c->chunk_item);
                            ExFreePool(c);
                            return STATUS_INTERNAL_ERROR;
                        }

                        if (c->devices[i]->readonly)
                            c->readonly = true;
                    }
                } else {
                    ERR("chunk %I64x: number of stripes is 0\n", c->offset);
                    ExFreePool(c->chunk_item);
                    ExFreePool(c);
                    return STATUS_INTERNAL_ERROR;
                }

                ExInitializeResourceLite(&c->lock);
                ExInitializeResourceLite(&c->changed_extents_lock);

                InitializeListHead(&c->space);
                InitializeListHead(&c->space_size);
                InitializeListHead(&c->deleting);
                InitializeListHead(&c->changed_extents);

                InitializeListHead(&c->range_locks);
                ExInitializeResourceLite(&c->range_locks_lock);
                KeInitializeEvent(&c->range_locks_event, NotificationEvent, false);

                InitializeListHead(&c->partial_stripes);
                ExInitializeResourceLite(&c->partial_stripes_lock);

                c->last_alloc_set = false;

                c->last_stripe = 0;

                InsertTailList(&Vcb->chunks, &c->list_entry);

                c->list_entry_balance.Flink = NULL;
            }
        }

        b = find_next_item(Vcb, &tp, &next_tp, false, Irp);

        if (b)
            tp = next_tp;
    } while (b);

    Vcb->log_to_phys_loaded = true;

    if (Vcb->data_flags == 0)
        Vcb->data_flags = BLOCK_FLAG_DATA | (Vcb->superblock.num_devices > 1 ? BLOCK_FLAG_RAID0 : 0);

    if (Vcb->metadata_flags == 0)
        Vcb->metadata_flags = BLOCK_FLAG_METADATA | (Vcb->superblock.num_devices > 1 ? BLOCK_FLAG_RAID1 : BLOCK_FLAG_DUPLICATE);

    if (Vcb->system_flags == 0)
        Vcb->system_flags = BLOCK_FLAG_SYSTEM | (Vcb->superblock.num_devices > 1 ? BLOCK_FLAG_RAID1 : BLOCK_FLAG_DUPLICATE);

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS) {
        Vcb->metadata_flags |= BLOCK_FLAG_DATA;
        Vcb->data_flags = Vcb->metadata_flags;
    }

    return STATUS_SUCCESS;
}

void protect_superblocks(_Inout_ chunk* c) {
    uint16_t i = 0, j;
    uint64_t off_start, off_end;

    // The Linux driver also protects all the space before the first superblock.
    // I realize this confuses physical and logical addresses, but this is what btrfs-progs does -
    // evidently Linux assumes the chunk at 0 is always SINGLE.
    if (c->offset < superblock_addrs[0])
        space_list_subtract(c, c->offset, superblock_addrs[0] - c->offset, NULL);

    while (superblock_addrs[i] != 0) {
        CHUNK_ITEM* ci = c->chunk_item;
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];

        if (ci->type & BLOCK_FLAG_RAID0 || ci->type & BLOCK_FLAG_RAID10) {
            for (j = 0; j < ci->num_stripes; j++) {
                uint16_t sub_stripes = max(ci->sub_stripes, 1);

                if (cis[j].offset + (ci->size * ci->num_stripes / sub_stripes) > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
#ifdef _DEBUG
                    uint64_t startoff;
                    uint16_t startoffstripe;
#endif

                    TRACE("cut out superblock in chunk %I64x\n", c->offset);

                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % ci->stripe_length;
                    off_start *= ci->num_stripes / sub_stripes;
                    off_start += (j / sub_stripes) * ci->stripe_length;

                    off_end = off_start + ci->stripe_length;

#ifdef _DEBUG
                    get_raid0_offset(off_start, ci->stripe_length, ci->num_stripes / sub_stripes, &startoff, &startoffstripe);
                    TRACE("j = %u, startoffstripe = %u\n", j, startoffstripe);
                    TRACE("startoff = %I64x, superblock = %I64x\n", startoff + cis[j].offset, superblock_addrs[i]);
#endif

                    space_list_subtract(c, c->offset + off_start, off_end - off_start, NULL);
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID5) {
            uint64_t stripe_size = ci->size / (ci->num_stripes - 1);

            for (j = 0; j < ci->num_stripes; j++) {
                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    TRACE("cut out superblock in chunk %I64x\n", c->offset);

                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % ci->stripe_length;
                    off_start *= ci->num_stripes - 1;

                    off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), ci->stripe_length);
                    off_end *= ci->num_stripes - 1;

                    TRACE("cutting out %I64x, size %I64x\n", c->offset + off_start, off_end - off_start);

                    space_list_subtract(c, c->offset + off_start, off_end - off_start, NULL);
                }
            }
        } else if (ci->type & BLOCK_FLAG_RAID6) {
            uint64_t stripe_size = ci->size / (ci->num_stripes - 2);

            for (j = 0; j < ci->num_stripes; j++) {
                if (cis[j].offset + stripe_size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    TRACE("cut out superblock in chunk %I64x\n", c->offset);

                    off_start = superblock_addrs[i] - cis[j].offset;
                    off_start -= off_start % ci->stripe_length;
                    off_start *= ci->num_stripes - 2;

                    off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), ci->stripe_length);
                    off_end *= ci->num_stripes - 2;

                    TRACE("cutting out %I64x, size %I64x\n", c->offset + off_start, off_end - off_start);

                    space_list_subtract(c, c->offset + off_start, off_end - off_start, NULL);
                }
            }
        } else { // SINGLE, DUPLICATE, RAID1, RAID1C3, RAID1C4
            for (j = 0; j < ci->num_stripes; j++) {
                if (cis[j].offset + ci->size > superblock_addrs[i] && cis[j].offset <= superblock_addrs[i] + sizeof(superblock)) {
                    TRACE("cut out superblock in chunk %I64x\n", c->offset);

                    // The Linux driver protects the whole stripe in which the superblock lives

                    off_start = ((superblock_addrs[i] - cis[j].offset) / c->chunk_item->stripe_length) * c->chunk_item->stripe_length;
                    off_end = sector_align(superblock_addrs[i] - cis[j].offset + sizeof(superblock), c->chunk_item->stripe_length);

                    space_list_subtract(c, c->offset + off_start, off_end - off_start, NULL);
                }
            }
        }

        i++;
    }
}

NTSTATUS find_chunk_usage(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;

    searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;

    Vcb->superblock.bytes_used = 0;

    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        searchkey.obj_id = c->offset;
        searchkey.offset = c->chunk_item->size;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            return Status;
        }

        if (!keycmp(searchkey, tp.item->key)) {
            if (tp.item->size >= sizeof(BLOCK_GROUP_ITEM)) {
                bgi = (BLOCK_GROUP_ITEM*)tp.item->data;

                c->used = c->oldused = bgi->used;

                TRACE("chunk %I64x has %I64x bytes used\n", c->offset, c->used);

                Vcb->superblock.bytes_used += bgi->used;
            } else {
                ERR("(%I64x;%I64x,%x,%I64x) is %u bytes, expected %Iu\n",
                    Vcb->extent_root->id, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
            }
        }

        le = le->Flink;
    }

    Vcb->chunk_usage_found = true;

    return STATUS_SUCCESS;
}

static NTSTATUS load_sys_chunks(_In_ device_extension* Vcb) {
    KEY key;
    ULONG n = Vcb->superblock.n;

    while (n > 0) {
        if (n > sizeof(KEY)) {
            RtlCopyMemory(&key, &Vcb->superblock.sys_chunk_array[Vcb->superblock.n - n], sizeof(KEY));
            n -= sizeof(KEY);
        } else
            return STATUS_SUCCESS;

        TRACE("bootstrap: %I64x,%x,%I64x\n", key.obj_id, key.obj_type, key.offset);

        if (key.obj_type == TYPE_CHUNK_ITEM) {
            CHUNK_ITEM* ci;
            USHORT cisize;
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
                ExFreePool(sc);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(sc->data, ci, sc->size);
            InsertTailList(&Vcb->sys_chunks, &sc->list_entry);

            n -= cisize;
        } else {
            ERR("unexpected item %I64x,%x,%I64x in bootstrap\n", key.obj_id, key.obj_type, key.offset);
            return STATUS_INTERNAL_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

_Ret_maybenull_
root* find_default_subvol(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp) {
    LIST_ENTRY* le;

    static const char fn[] = "default";
    static uint32_t crc32 = 0x8dbfc2d2;

    if (Vcb->options.subvol_id != 0) {
        le = Vcb->roots.Flink;
        while (le != &Vcb->roots) {
            root* r = CONTAINING_RECORD(le, root, list_entry);

            if (r->id == Vcb->options.subvol_id)
                return r;

            le = le->Flink;
        }
    }

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_DEFAULT_SUBVOL) {
        NTSTATUS Status;
        KEY searchkey;
        traverse_ptr tp;
        DIR_ITEM* di;

        searchkey.obj_id = Vcb->superblock.root_dir_objectid;
        searchkey.obj_type = TYPE_DIR_ITEM;
        searchkey.offset = crc32;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, false, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08lx\n", Status);
            goto end;
        }

        if (keycmp(tp.item->key, searchkey)) {
            ERR("could not find (%I64x,%x,%I64x) in root tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            goto end;
        }

        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected at least %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            goto end;
        }

        di = (DIR_ITEM*)tp.item->data;

        if (tp.item->size < sizeof(DIR_ITEM) - 1 + di->n) {
            ERR("(%I64x,%x,%I64x) was %u bytes, expected %Iu\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM) - 1 + di->n);
            goto end;
        }

        if (di->n != strlen(fn) || RtlCompareMemory(di->name, fn, di->n) != di->n) {
            ERR("root DIR_ITEM had same CRC32, but was not \"default\"\n");
            goto end;
        }

        if (di->key.obj_type != TYPE_ROOT_ITEM) {
            ERR("default root has key (%I64x,%x,%I64x), expected subvolume\n", di->key.obj_id, di->key.obj_type, di->key.offset);
            goto end;
        }

        le = Vcb->roots.Flink;
        while (le != &Vcb->roots) {
            root* r = CONTAINING_RECORD(le, root, list_entry);

            if (r->id == di->key.obj_id)
                return r;

            le = le->Flink;
        }

        ERR("could not find root %I64x, using default instead\n", di->key.obj_id);
    }

end:
    le = Vcb->roots.Flink;
    while (le != &Vcb->roots) {
        root* r = CONTAINING_RECORD(le, root, list_entry);

        if (r->id == BTRFS_ROOT_FSTREE)
            return r;

        le = le->Flink;
    }

    return NULL;
}

void init_file_cache(_In_ PFILE_OBJECT FileObject, _In_ CC_FILE_SIZES* ccfs) {
    TRACE("(%p, %p)\n", FileObject, ccfs);

    CcInitializeCacheMap(FileObject, ccfs, false, &cache_callbacks, FileObject);

    if (diskacc)
        fCcSetAdditionalCacheAttributesEx(FileObject, CC_ENABLE_DISK_IO_ACCOUNTING);

    CcSetReadAheadGranularity(FileObject, READ_AHEAD_GRANULARITY);
}

uint32_t get_num_of_processors() {
    KAFFINITY p = KeQueryActiveProcessors();
    uint32_t r = 0;

    while (p != 0) {
        if (p & 1)
            r++;

        p >>= 1;
    }

    return r;
}

static NTSTATUS create_calc_threads(_In_ PDEVICE_OBJECT DeviceObject) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    OBJECT_ATTRIBUTES oa;
    ULONG i;

    Vcb->calcthreads.num_threads = get_num_of_processors();

    Vcb->calcthreads.threads = ExAllocatePoolWithTag(NonPagedPool, sizeof(drv_calc_thread) * Vcb->calcthreads.num_threads, ALLOC_TAG);
    if (!Vcb->calcthreads.threads) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&Vcb->calcthreads.job_list);
    KeInitializeSpinLock(&Vcb->calcthreads.spinlock);
    KeInitializeEvent(&Vcb->calcthreads.event, NotificationEvent, false);

    RtlZeroMemory(Vcb->calcthreads.threads, sizeof(drv_calc_thread) * Vcb->calcthreads.num_threads);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    for (i = 0; i < Vcb->calcthreads.num_threads; i++) {
        NTSTATUS Status;

        Vcb->calcthreads.threads[i].DeviceObject = DeviceObject;
        Vcb->calcthreads.threads[i].number = i;
        KeInitializeEvent(&Vcb->calcthreads.threads[i].finished, NotificationEvent, false);

        Status = PsCreateSystemThread(&Vcb->calcthreads.threads[i].handle, 0, &oa, NULL, NULL, calc_thread, &Vcb->calcthreads.threads[i]);
        if (!NT_SUCCESS(Status)) {
            ULONG j;

            ERR("PsCreateSystemThread returned %08lx\n", Status);

            for (j = 0; j < i; j++) {
                Vcb->calcthreads.threads[i].quit = true;
            }

            KeSetEvent(&Vcb->calcthreads.event, 0, false);

            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static bool is_btrfs_volume(_In_ PDEVICE_OBJECT DeviceObject) {
    NTSTATUS Status;
    MOUNTDEV_NAME mdn, *mdn2;
    ULONG mdnsize;

    Status = dev_ioctl(DeviceObject, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), true, NULL);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08lx\n", Status);
        return false;
    }

    mdnsize = (ULONG)offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

    mdn2 = ExAllocatePoolWithTag(PagedPool, mdnsize, ALLOC_TAG);
    if (!mdn2) {
        ERR("out of memory\n");
        return false;
    }

    Status = dev_ioctl(DeviceObject, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, mdnsize, true, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08lx\n", Status);
        ExFreePool(mdn2);
        return false;
    }

    if (mdn2->NameLength > (sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR)) &&
        RtlCompareMemory(mdn2->Name, BTRFS_VOLUME_PREFIX, sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR)) == sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR)) {
        ExFreePool(mdn2);
        return true;
    }

    ExFreePool(mdn2);

    return false;
}

static NTSTATUS get_device_pnp_name_guid(_In_ PDEVICE_OBJECT DeviceObject, _Out_ PUNICODE_STRING pnp_name, _In_ const GUID* guid) {
    NTSTATUS Status;
    WCHAR *list = NULL, *s;

    Status = IoGetDeviceInterfaces((PVOID)guid, NULL, 0, &list);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceInterfaces returned %08lx\n", Status);
        return Status;
    }

    s = list;
    while (s[0] != 0) {
        PFILE_OBJECT FileObject;
        PDEVICE_OBJECT devobj;
        UNICODE_STRING name;

        name.Length = name.MaximumLength = (USHORT)wcslen(s) * sizeof(WCHAR);
        name.Buffer = s;

        if (NT_SUCCESS(IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &FileObject, &devobj))) {
            if (DeviceObject == devobj || DeviceObject == FileObject->DeviceObject) {
                ObDereferenceObject(FileObject);

                pnp_name->Buffer = ExAllocatePoolWithTag(PagedPool, name.Length, ALLOC_TAG);
                if (!pnp_name->Buffer) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                RtlCopyMemory(pnp_name->Buffer, name.Buffer, name.Length);
                pnp_name->Length = pnp_name->MaximumLength = name.Length;

                Status = STATUS_SUCCESS;
                goto end;
            }

            ObDereferenceObject(FileObject);
        }

        s = &s[wcslen(s) + 1];
    }

    pnp_name->Length = pnp_name->MaximumLength = 0;
    pnp_name->Buffer = 0;

    Status = STATUS_NOT_FOUND;

end:
    if (list)
        ExFreePool(list);

    return Status;
}

NTSTATUS get_device_pnp_name(_In_ PDEVICE_OBJECT DeviceObject, _Out_ PUNICODE_STRING pnp_name, _Out_ const GUID** guid) {
    NTSTATUS Status;

    Status = get_device_pnp_name_guid(DeviceObject, pnp_name, &GUID_DEVINTERFACE_VOLUME);
    if (NT_SUCCESS(Status)) {
        *guid = &GUID_DEVINTERFACE_VOLUME;
        return Status;
    }

    Status = get_device_pnp_name_guid(DeviceObject, pnp_name, &GUID_DEVINTERFACE_HIDDEN_VOLUME);
    if (NT_SUCCESS(Status)) {
        *guid = &GUID_DEVINTERFACE_HIDDEN_VOLUME;
        return Status;
    }

    Status = get_device_pnp_name_guid(DeviceObject, pnp_name, &GUID_DEVINTERFACE_DISK);
    if (NT_SUCCESS(Status)) {
        *guid = &GUID_DEVINTERFACE_DISK;
        return Status;
    }

    return STATUS_NOT_FOUND;
}

_Success_(return>=0)
static NTSTATUS check_mount_device(_In_ PDEVICE_OBJECT DeviceObject, _Out_ bool* pno_pnp) {
    NTSTATUS Status;
    ULONG to_read;
    superblock* sb;
    // UNICODE_STRING pnp_name;
    // const GUID* guid;

    to_read = DeviceObject->SectorSize == 0 ? sizeof(superblock) : (ULONG)sector_align(sizeof(superblock), DeviceObject->SectorSize);

    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = sync_read_phys(DeviceObject, NULL, superblock_addrs[0], to_read, (PUCHAR)sb, true);
    if (!NT_SUCCESS(Status)) {
        ERR("sync_read_phys returned %08lx\n", Status);
        goto end;
    }

    if (sb->magic != BTRFS_MAGIC) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    if (!check_superblock_checksum(sb)) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    DeviceObject->Flags &= ~DO_VERIFY_VOLUME;

    // pnp_name.Buffer = NULL;

    // Status = get_device_pnp_name(DeviceObject, &pnp_name, &guid);
    // if (!NT_SUCCESS(Status)) {
    //     WARN("get_device_pnp_name returned %08lx\n", Status);
    //     pnp_name.Length = 0;
    // }

    // *pno_pnp = pnp_name.Length == 0;
    *pno_pnp = true;

    // if (pnp_name.Buffer)
    //     ExFreePool(pnp_name.Buffer);

    Status = STATUS_SUCCESS;

end:
    ExFreePool(sb);

    return Status;
}

static bool still_has_superblock(_In_ PDEVICE_OBJECT device, _In_ PFILE_OBJECT fileobj) {
    NTSTATUS Status;
    ULONG to_read;
    superblock* sb;

    if (!device)
        return false;

    to_read = device->SectorSize == 0 ? sizeof(superblock) : (ULONG)sector_align(sizeof(superblock), device->SectorSize);

    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return false;
    }

    Status = sync_read_phys(device, fileobj, superblock_addrs[0], to_read, (PUCHAR)sb, true);
    if (!NT_SUCCESS(Status)) {
        ERR("Failed to read superblock: %08lx\n", Status);
        ExFreePool(sb);
        return false;
    }

    if (sb->magic != BTRFS_MAGIC) {
        TRACE("not a BTRFS volume\n");
        ExFreePool(sb);
        return false;
    } else {
        if (!check_superblock_checksum(sb)) {
            ExFreePool(sb);
            return false;
        }
    }

    ObReferenceObject(device);

    while (device) {
        PDEVICE_OBJECT device2 = IoGetLowerDeviceObject(device);

        device->Flags &= ~DO_VERIFY_VOLUME;

        ObDereferenceObject(device);

        device = device2;
    }

    ExFreePool(sb);

    return true;
}

static void calculate_sector_shift(device_extension* Vcb) {
    uint32_t ss = Vcb->superblock.sector_size;

    Vcb->sector_shift = 0;

    while (!(ss & 1)) {
        Vcb->sector_shift++;
        ss >>= 1;
    }
}

static NTSTATUS mount_vol(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT NewDeviceObject = NULL;
    PDEVICE_OBJECT DeviceToMount, readobj;
    PFILE_OBJECT fileobj;
    NTSTATUS Status;
    device_extension* Vcb = NULL;
    LIST_ENTRY *le, batchlist;
    KEY searchkey;
    traverse_ptr tp;
    fcb* root_fcb = NULL;
    ccb* root_ccb = NULL;
    bool init_lookaside = false;
    device* dev;
    volume_device_extension* vde = NULL;
    pdo_device_extension* pdode = NULL;
    volume_child* vc;
    uint64_t readobjsize;
    OBJECT_ATTRIBUTES oa;
    device_extension* real_devext;
    KIRQL irql;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    if (DeviceObject != master_devobj)
        return STATUS_INVALID_DEVICE_REQUEST;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    DeviceToMount = IrpSp->Parameters.MountVolume.DeviceObject;

    real_devext = IrpSp->Parameters.MountVolume.Vpb->RealDevice->DeviceExtension;

    // Make sure we're not trying to mount the PDO
    if (IrpSp->Parameters.MountVolume.Vpb->RealDevice->DriverObject == drvobj && real_devext->type == VCB_TYPE_PDO)
        return STATUS_UNRECOGNIZED_VOLUME;

    if (!is_btrfs_volume(DeviceToMount)) {
        bool not_pnp = false;

        Status = check_mount_device(DeviceToMount, &not_pnp);
        if (!NT_SUCCESS(Status))
            WARN("check_mount_device returned %08lx\n", Status);

        if (!not_pnp) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            goto exit;
        }
    } else {
        PDEVICE_OBJECT pdo;

        pdo = DeviceToMount;

        ObReferenceObject(pdo);

        while (true) {
            PDEVICE_OBJECT pdo2 = IoGetLowerDeviceObject(pdo);

            ObDereferenceObject(pdo);

            if (!pdo2)
                break;
            else
                pdo = pdo2;
        }

        ExAcquireResourceSharedLite(&pdo_list_lock, true);

        le = pdo_list.Flink;
        while (le != &pdo_list) {
            pdo_device_extension* pdode2 = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

            if (pdode2->pdo == pdo) {
                vde = pdode2->vde;
                break;
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&pdo_list_lock);

        if (!vde || vde->type != VCB_TYPE_VOLUME) {
            vde = NULL;
            Status = STATUS_UNRECOGNIZED_VOLUME;
            goto exit;
        }
    }

    if (vde) {
        pdode = vde->pdode;

        ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

        le = pdode->children.Flink;
        while (le != &pdode->children) {
            LIST_ENTRY* le2 = le->Flink;

            vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

            if (!still_has_superblock(vc->devobj, vc->fileobj)) {
                remove_volume_child(vde, vc, false);

                if (pdode->num_children == 0) {
                    ERR("error - number of devices is zero\n");
                    Status = STATUS_INTERNAL_ERROR;
                    ExReleaseResourceLite(&pdode->child_lock);
                    goto exit;
                }

                Status = STATUS_DEVICE_NOT_READY;
                ExReleaseResourceLite(&pdode->child_lock);
                goto exit;
            }

            le = le2;
        }

        if (pdode->num_children == 0 || pdode->children_loaded == 0) {
            ERR("error - number of devices is zero\n");
            Status = STATUS_INTERNAL_ERROR;
            ExReleaseResourceLite(&pdode->child_lock);
            goto exit;
        }

        ExConvertExclusiveToSharedLite(&pdode->child_lock);

        vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

        readobj = vc->devobj;
        fileobj = vc->fileobj;
        readobjsize = vc->size;

        vde->device->Characteristics &= ~FILE_DEVICE_SECURE_OPEN;
    } else {
        GET_LENGTH_INFORMATION gli;

        vc = NULL;
        readobj = DeviceToMount;
        fileobj = NULL;

        Status = dev_ioctl(readobj, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                           &gli, sizeof(gli), true, NULL);

        if (!NT_SUCCESS(Status)) {
            ERR("error reading length information: %08lx\n", Status);
            goto exit;
        }

        readobjsize = gli.Length.QuadPart;
    }

    Status = IoCreateDevice(drvobj, sizeof(device_extension), NULL, FILE_DEVICE_DISK_FILE_SYSTEM, 0, false, &NewDeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08lx\n", Status);
        Status = STATUS_UNRECOGNIZED_VOLUME;

        if (pdode)
            ExReleaseResourceLite(&pdode->child_lock);

        goto exit;
    }

    NewDeviceObject->Flags |= DO_DIRECT_IO;

    // Some programs seem to expect that the sector size will be 512, for
    // FILE_NO_INTERMEDIATE_BUFFERING and the like.
    NewDeviceObject->SectorSize = min(DeviceToMount->SectorSize, 512);

    Vcb = (PVOID)NewDeviceObject->DeviceExtension;
    RtlZeroMemory(Vcb, sizeof(device_extension));
    Vcb->type = VCB_TYPE_FS;
    Vcb->vde = vde;

    ExInitializeResourceLite(&Vcb->tree_lock);
    Vcb->need_write = false;

    ExInitializeResourceLite(&Vcb->fcb_lock);
    ExInitializeResourceLite(&Vcb->fileref_lock);
    ExInitializeResourceLite(&Vcb->chunk_lock);
    ExInitializeResourceLite(&Vcb->dirty_fcbs_lock);
    ExInitializeResourceLite(&Vcb->dirty_filerefs_lock);
    ExInitializeResourceLite(&Vcb->dirty_subvols_lock);
    ExInitializeResourceLite(&Vcb->scrub.stats_lock);

    ExInitializeResourceLite(&Vcb->load_lock);
    ExAcquireResourceExclusiveLite(&Vcb->load_lock, true);

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    DeviceToMount->Flags |= DO_DIRECT_IO;

    Status = read_superblock(Vcb, readobj, fileobj, readobjsize);
    if (!NT_SUCCESS(Status)) {
        if (!IoIsErrorUserInduced(Status))
            Status = STATUS_UNRECOGNIZED_VOLUME;
        else if (Irp->Tail.Overlay.Thread)
            IoSetHardErrorOrVerifyDevice(Irp, readobj);

        if (pdode)
            ExReleaseResourceLite(&pdode->child_lock);

        goto exit;
    }

    if (!vde && Vcb->superblock.num_devices > 1) {
        ERR("cannot mount multi-device FS with non-PNP device\n");
        Status = STATUS_UNRECOGNIZED_VOLUME;

        if (pdode)
            ExReleaseResourceLite(&pdode->child_lock);

        goto exit;
    }

    Status = registry_load_volume_options(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("registry_load_volume_options returned %08lx\n", Status);

        if (pdode)
            ExReleaseResourceLite(&pdode->child_lock);

        goto exit;
    }

    if (pdode && RtlCompareMemory(&boot_uuid, &pdode->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID) && boot_subvol != 0)
        Vcb->options.subvol_id = boot_subvol;

    if (pdode && pdode->children_loaded < pdode->num_children && (!Vcb->options.allow_degraded || !finished_probing || degraded_wait)) {
        ERR("could not mount as %I64u device(s) missing\n", pdode->num_children - pdode->children_loaded);
        Status = STATUS_DEVICE_NOT_READY;
        ExReleaseResourceLite(&pdode->child_lock);
        goto exit;
    }

    if (pdode) {
        // Windows holds DeviceObject->DeviceLock, guaranteeing that mount_vol is serialized
        ExReleaseResourceLite(&pdode->child_lock);
    }

    if (Vcb->options.ignore) {
        TRACE("ignoring volume\n");
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }

    if (Vcb->superblock.incompat_flags & ~INCOMPAT_SUPPORTED) {
        WARN("cannot mount because of unsupported incompat flags (%I64x)\n", Vcb->superblock.incompat_flags & ~INCOMPAT_SUPPORTED);
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto exit;
    }

    if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_METADATA_UUID))
        Vcb->superblock.metadata_uuid = Vcb->superblock.uuid;

    Vcb->readonly = false;
    if (Vcb->superblock.compat_ro_flags & ~COMPAT_RO_SUPPORTED) {
        WARN("mounting read-only because of unsupported flags (%I64x)\n", Vcb->superblock.compat_ro_flags & ~COMPAT_RO_SUPPORTED);
        Vcb->readonly = true;
    }

    if (Vcb->options.readonly)
        Vcb->readonly = true;

    calculate_sector_shift(Vcb);

    Vcb->superblock.generation++;
    Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF;

    if (Vcb->superblock.log_tree_addr != 0) {
        FIXME("FIXME - replay transaction log (clearing for now)\n");
        Vcb->superblock.log_tree_addr = 0;
    }

    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C:
            Vcb->csum_size = sizeof(uint32_t);
            break;

        case CSUM_TYPE_XXHASH:
            Vcb->csum_size = sizeof(uint64_t);
            break;

        case CSUM_TYPE_SHA256:
            Vcb->csum_size = SHA256_HASH_SIZE;
            break;

        case CSUM_TYPE_BLAKE2:
            Vcb->csum_size = BLAKE2_HASH_SIZE;
            break;

        default:
            ERR("unrecognized csum type %x\n", Vcb->superblock.csum_type);
            break;
    }

    InitializeListHead(&Vcb->devices);
    dev = ExAllocatePoolWithTag(NonPagedPool, sizeof(device), ALLOC_TAG);
    if (!dev) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    dev->devobj = readobj;
    dev->fileobj = fileobj;
    RtlCopyMemory(&dev->devitem, &Vcb->superblock.dev_item, sizeof(DEV_ITEM));

    if (dev->devitem.num_bytes > readobjsize) {
        WARN("device %I64x: DEV_ITEM says %I64x bytes, but Windows only reports %I64x\n", dev->devitem.dev_id,
                dev->devitem.num_bytes, readobjsize);

        dev->devitem.num_bytes = readobjsize;
    }

    dev->seeding = Vcb->superblock.flags & BTRFS_SUPERBLOCK_FLAGS_SEEDING ? true : false;

    init_device(Vcb, dev, true);

    InsertTailList(&Vcb->devices, &dev->list_entry);
    Vcb->devices_loaded = 1;

    if (DeviceToMount->Flags & DO_SYSTEM_BOOT_PARTITION)
        Vcb->disallow_dismount = true;

    TRACE("DeviceToMount = %p\n", DeviceToMount);
    TRACE("IrpSp->Parameters.MountVolume.Vpb = %p\n", IrpSp->Parameters.MountVolume.Vpb);

    NewDeviceObject->StackSize = DeviceToMount->StackSize + 1;
    NewDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    InitializeListHead(&Vcb->roots);
    InitializeListHead(&Vcb->drop_roots);

    Vcb->log_to_phys_loaded = false;

    add_root(Vcb, BTRFS_ROOT_CHUNK, Vcb->superblock.chunk_tree_addr, Vcb->superblock.chunk_root_generation, NULL);

    if (!Vcb->chunk_root) {
        ERR("Could not load chunk root.\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    InitializeListHead(&Vcb->sys_chunks);
    Status = load_sys_chunks(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("load_sys_chunks returned %08lx\n", Status);
        goto exit;
    }

    InitializeListHead(&Vcb->chunks);
    InitializeListHead(&Vcb->trees);
    InitializeListHead(&Vcb->trees_hash);
    InitializeListHead(&Vcb->all_fcbs);
    InitializeListHead(&Vcb->dirty_fcbs);
    InitializeListHead(&Vcb->dirty_filerefs);
    InitializeListHead(&Vcb->dirty_subvols);
    InitializeListHead(&Vcb->send_ops);

    ExInitializeFastMutex(&Vcb->trees_list_mutex);

    InitializeListHead(&Vcb->DirNotifyList);
    InitializeListHead(&Vcb->scrub.errors);

    FsRtlNotifyInitializeSync(&Vcb->NotifySync);

    ExInitializePagedLookasideList(&Vcb->tree_data_lookaside, NULL, NULL, 0, sizeof(tree_data), ALLOC_TAG, 0);
    ExInitializePagedLookasideList(&Vcb->traverse_ptr_lookaside, NULL, NULL, 0, sizeof(traverse_ptr), ALLOC_TAG, 0);
    ExInitializePagedLookasideList(&Vcb->batch_item_lookaside, NULL, NULL, 0, sizeof(batch_item), ALLOC_TAG, 0);
    ExInitializePagedLookasideList(&Vcb->fileref_lookaside, NULL, NULL, 0, sizeof(file_ref), ALLOC_TAG, 0);
    ExInitializePagedLookasideList(&Vcb->fcb_lookaside, NULL, NULL, 0, sizeof(fcb), ALLOC_TAG, 0);
    ExInitializePagedLookasideList(&Vcb->name_bit_lookaside, NULL, NULL, 0, sizeof(name_bit), ALLOC_TAG, 0);
    ExInitializeNPagedLookasideList(&Vcb->range_lock_lookaside, NULL, NULL, 0, sizeof(range_lock), ALLOC_TAG, 0);
    ExInitializeNPagedLookasideList(&Vcb->fcb_np_lookaside, NULL, NULL, 0, sizeof(fcb_nonpaged), ALLOC_TAG, 0);
    init_lookaside = true;

    Vcb->Vpb = IrpSp->Parameters.MountVolume.Vpb;

    Status = load_chunk_root(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("load_chunk_root returned %08lx\n", Status);
        goto exit;
    }

    if (Vcb->superblock.num_devices > 1) {
        if (Vcb->devices_loaded < Vcb->superblock.num_devices && (!Vcb->options.allow_degraded || !finished_probing)) {
            ERR("could not mount as %I64u device(s) missing\n", Vcb->superblock.num_devices - Vcb->devices_loaded);

            IoRaiseInformationalHardError(IO_ERR_INTERNAL_ERROR, NULL, NULL);

            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }

        if (dev->readonly && !Vcb->readonly) {
            Vcb->readonly = true;

            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev2 = CONTAINING_RECORD(le, device, list_entry);

                if (dev2->readonly && !dev2->seeding)
                    break;

                if (!dev2->readonly) {
                    Vcb->readonly = false;
                    break;
                }

                le = le->Flink;
            }

            if (Vcb->readonly)
                WARN("setting volume to readonly\n");
        }
    } else {
        if (dev->readonly) {
            WARN("setting volume to readonly as device is readonly\n");
            Vcb->readonly = true;
        }
    }

    add_root(Vcb, BTRFS_ROOT_ROOT, Vcb->superblock.root_tree_addr, Vcb->superblock.generation - 1, NULL);

    if (!Vcb->root_root) {
        ERR("Could not load root of roots.\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    Status = look_for_roots(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("look_for_roots returned %08lx\n", Status);
        goto exit;
    }

    if (!Vcb->readonly) {
        Status = find_chunk_usage(Vcb, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_chunk_usage returned %08lx\n", Status);
            goto exit;
        }
    }

    InitializeListHead(&batchlist);

    // We've already increased the generation by one
    if (!Vcb->readonly && (
        Vcb->options.clear_cache ||
        (!(Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE) && Vcb->superblock.generation - 1 != Vcb->superblock.cache_generation) ||
        (Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE && !(Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE_VALID)))) {
        if (Vcb->options.clear_cache)
            WARN("ClearCache option was set, clearing cache...\n");
        else if (Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE && !(Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE_VALID))
            WARN("clearing free-space tree created by buggy Linux driver\n");
        else
            WARN("generation was %I64x, free-space cache generation was %I64x; clearing cache...\n", Vcb->superblock.generation - 1, Vcb->superblock.cache_generation);

        Status = clear_free_space_cache(Vcb, &batchlist, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("clear_free_space_cache returned %08lx\n", Status);
            clear_batch_list(Vcb, &batchlist);
            goto exit;
        }
    }

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08lx\n", Status);
        goto exit;
    }

    Vcb->volume_fcb = create_fcb(Vcb, NonPagedPool);
    if (!Vcb->volume_fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Vcb->volume_fcb->Vcb = Vcb;
    Vcb->volume_fcb->sd = NULL;

    Vcb->dummy_fcb = create_fcb(Vcb, NonPagedPool);
    if (!Vcb->dummy_fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Vcb->dummy_fcb->Vcb = Vcb;
    Vcb->dummy_fcb->type = BTRFS_TYPE_DIRECTORY;
    Vcb->dummy_fcb->inode = 2;
    Vcb->dummy_fcb->subvol = Vcb->root_root;
    Vcb->dummy_fcb->atts = FILE_ATTRIBUTE_DIRECTORY;
    Vcb->dummy_fcb->inode_item.st_nlink = 1;
    Vcb->dummy_fcb->inode_item.st_mode = __S_IFDIR;

    Vcb->dummy_fcb->hash_ptrs = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!Vcb->dummy_fcb->hash_ptrs) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(Vcb->dummy_fcb->hash_ptrs, sizeof(LIST_ENTRY*) * 256);

    Vcb->dummy_fcb->hash_ptrs_uc = ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY*) * 256, ALLOC_TAG);
    if (!Vcb->dummy_fcb->hash_ptrs_uc) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(Vcb->dummy_fcb->hash_ptrs_uc, sizeof(LIST_ENTRY*) * 256);

    root_fcb = create_fcb(Vcb, NonPagedPool);
    if (!root_fcb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    root_fcb->Vcb = Vcb;
    root_fcb->inode = SUBVOL_ROOT_INODE;
    root_fcb->hash = calc_crc32c(0xffffffff, (uint8_t*)&root_fcb->inode, sizeof(uint64_t));
    root_fcb->type = BTRFS_TYPE_DIRECTORY;

#ifdef DEBUG_FCB_REFCOUNTS
    WARN("volume FCB = %p\n", Vcb->volume_fcb);
    WARN("root FCB = %p\n", root_fcb);
#endif

    root_fcb->subvol = find_default_subvol(Vcb, Irp);

    if (!root_fcb->subvol) {
        ERR("could not find top subvol\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    Status = load_dir_children(Vcb, root_fcb, true, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("load_dir_children returned %08lx\n", Status);
        goto exit;
    }

    searchkey.obj_id = root_fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, root_fcb->subvol, &tp, &searchkey, false, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08lx\n", Status);
        goto exit;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("couldn't find INODE_ITEM for root directory\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    if (tp.item->size > 0)
        RtlCopyMemory(&root_fcb->inode_item, tp.item->data, min(sizeof(INODE_ITEM), tp.item->size));

    fcb_get_sd(root_fcb, NULL, true, Irp);

    root_fcb->atts = get_file_attributes(Vcb, root_fcb->subvol, root_fcb->inode, root_fcb->type, false, false, Irp);

    if (root_fcb->subvol->id == BTRFS_ROOT_FSTREE)
        root_fcb->atts &= ~FILE_ATTRIBUTE_HIDDEN;

    Vcb->root_fileref = create_fileref(Vcb);
    if (!Vcb->root_fileref) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Vcb->root_fileref->fcb = root_fcb;
    InsertTailList(&root_fcb->subvol->fcbs, &root_fcb->list_entry);
    InsertTailList(&Vcb->all_fcbs, &root_fcb->list_entry_all);

    root_fcb->subvol->fcbs_ptrs[root_fcb->hash >> 24] = &root_fcb->list_entry;

    root_fcb->fileref = Vcb->root_fileref;

    root_ccb = ExAllocatePoolWithTag(PagedPool, sizeof(ccb), ALLOC_TAG);
    if (!root_ccb) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Vcb->root_file = IoCreateStreamFileObject(NULL, DeviceToMount);
    Vcb->root_file->FsContext = root_fcb;
    Vcb->root_file->SectionObjectPointer = &root_fcb->nonpaged->segment_object;
    Vcb->root_file->Vpb = DeviceObject->Vpb;

    RtlZeroMemory(root_ccb, sizeof(ccb));
    root_ccb->NodeType = BTRFS_NODE_TYPE_CCB;
    root_ccb->NodeSize = sizeof(ccb);

    Vcb->root_file->FsContext2 = root_ccb;

    _SEH2_TRY {
        CcInitializeCacheMap(Vcb->root_file, (PCC_FILE_SIZES)(&root_fcb->Header.AllocationSize), false, &cache_callbacks, Vcb->root_file);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
        goto exit;
    } _SEH2_END;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev2 = CONTAINING_RECORD(le, device, list_entry);

        Status = find_disk_holes(Vcb, dev2, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_disk_holes returned %08lx\n", Status);
            goto exit;
        }

        le = le->Flink;
    }

    IoAcquireVpbSpinLock(&irql);

    NewDeviceObject->Vpb = IrpSp->Parameters.MountVolume.Vpb;
    IrpSp->Parameters.MountVolume.Vpb->DeviceObject = NewDeviceObject;
    IrpSp->Parameters.MountVolume.Vpb->Flags |= VPB_MOUNTED;
    NewDeviceObject->Vpb->VolumeLabelLength = 4; // FIXME
    NewDeviceObject->Vpb->VolumeLabel[0] = '?';
    NewDeviceObject->Vpb->VolumeLabel[1] = 0;
    NewDeviceObject->Vpb->ReferenceCount++;

    IoReleaseVpbSpinLock(irql);

    KeInitializeEvent(&Vcb->flush_thread_finished, NotificationEvent, false);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread(&Vcb->flush_thread_handle, 0, &oa, NULL, NULL, flush_thread, NewDeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08lx\n", Status);
        goto exit;
    }

    Status = create_calc_threads(NewDeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("create_calc_threads returned %08lx\n", Status);
        goto exit;
    }

    Status = registry_mark_volume_mounted(&Vcb->superblock.uuid);
    if (!NT_SUCCESS(Status))
        WARN("registry_mark_volume_mounted returned %08lx\n", Status);

    Status = look_for_balance_item(Vcb);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
        WARN("look_for_balance_item returned %08lx\n", Status);

    Status = STATUS_SUCCESS;

    if (vde)
        vde->mounted_device = NewDeviceObject;

    Vcb->devobj = NewDeviceObject;

    ExInitializeResourceLite(&Vcb->send_load_lock);

exit:
    if (Vcb) {
        ExReleaseResourceLite(&Vcb->tree_lock);
        ExReleaseResourceLite(&Vcb->load_lock);
    }

    if (!NT_SUCCESS(Status)) {
        if (Vcb) {
            if (init_lookaside) {
                ExDeletePagedLookasideList(&Vcb->tree_data_lookaside);
                ExDeletePagedLookasideList(&Vcb->traverse_ptr_lookaside);
                ExDeletePagedLookasideList(&Vcb->batch_item_lookaside);
                ExDeletePagedLookasideList(&Vcb->fileref_lookaside);
                ExDeletePagedLookasideList(&Vcb->fcb_lookaside);
                ExDeletePagedLookasideList(&Vcb->name_bit_lookaside);
                ExDeleteNPagedLookasideList(&Vcb->range_lock_lookaside);
                ExDeleteNPagedLookasideList(&Vcb->fcb_np_lookaside);
            }

            if (Vcb->root_file)
                ObDereferenceObject(Vcb->root_file);
            else if (Vcb->root_fileref)
                free_fileref(Vcb->root_fileref);
            else if (root_fcb)
                free_fcb(root_fcb);

            if (root_fcb && root_fcb->refcount == 0)
                reap_fcb(root_fcb);

            if (Vcb->volume_fcb)
                reap_fcb(Vcb->volume_fcb);

            ExDeleteResourceLite(&Vcb->tree_lock);
            ExDeleteResourceLite(&Vcb->load_lock);
            ExDeleteResourceLite(&Vcb->fcb_lock);
            ExDeleteResourceLite(&Vcb->fileref_lock);
            ExDeleteResourceLite(&Vcb->chunk_lock);
            ExDeleteResourceLite(&Vcb->dirty_fcbs_lock);
            ExDeleteResourceLite(&Vcb->dirty_filerefs_lock);
            ExDeleteResourceLite(&Vcb->dirty_subvols_lock);
            ExDeleteResourceLite(&Vcb->scrub.stats_lock);

            if (Vcb->devices.Flink) {
                while (!IsListEmpty(&Vcb->devices)) {
                    device* dev2 = CONTAINING_RECORD(RemoveHeadList(&Vcb->devices), device, list_entry);

                    ExFreePool(dev2);
                }
            }
        }

        if (NewDeviceObject)
            IoDeleteDevice(NewDeviceObject);
    } else {
        ExAcquireResourceExclusiveLite(&global_loading_lock, true);
        InsertTailList(&VcbList, &Vcb->list_entry);
        ExReleaseResourceLite(&global_loading_lock);

        FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_MOUNT);
    }

    TRACE("mount_vol done (status: %lx)\n", Status);

    return Status;
}

static NTSTATUS verify_device(_In_ device_extension* Vcb, _Inout_ device* dev) {
    NTSTATUS Status;
    superblock* sb;
    ULONG to_read, cc;

    if (!dev->devobj)
        return STATUS_WRONG_VOLUME;

    if (dev->removable) {
        IO_STATUS_BLOCK iosb;

        Status = dev_ioctl(dev->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, &cc, sizeof(ULONG), true, &iosb);

        if (IoIsErrorUserInduced(Status)) {
            ERR("IOCTL_STORAGE_CHECK_VERIFY returned %08lx (user-induced)\n", Status);

            if (Vcb->vde) {
                pdo_device_extension* pdode = Vcb->vde->pdode;
                LIST_ENTRY* le2;
                bool changed = false;

                ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

                le2 = pdode->children.Flink;
                while (le2 != &pdode->children) {
                    volume_child* vc = CONTAINING_RECORD(le2, volume_child, list_entry);

                    if (vc->devobj == dev->devobj) {
                        TRACE("removing device\n");

                        remove_volume_child(Vcb->vde, vc, true);
                        changed = true;

                        break;
                    }

                    le2 = le2->Flink;
                }

                if (!changed)
                    ExReleaseResourceLite(&pdode->child_lock);
            }
        } else if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_STORAGE_CHECK_VERIFY returned %08lx\n", Status);
            return Status;
        } else if (iosb.Information < sizeof(ULONG)) {
            ERR("iosb.Information was too short\n");
            return STATUS_INTERNAL_ERROR;
        }

        dev->change_count = cc;
    }

    to_read = dev->devobj->SectorSize == 0 ? sizeof(superblock) : (ULONG)sector_align(sizeof(superblock), dev->devobj->SectorSize);

    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = sync_read_phys(dev->devobj, dev->fileobj, superblock_addrs[0], to_read, (PUCHAR)sb, true);
    if (!NT_SUCCESS(Status)) {
        ERR("Failed to read superblock: %08lx\n", Status);
        ExFreePool(sb);
        return Status;
    }

    if (sb->magic != BTRFS_MAGIC) {
        ERR("not a BTRFS volume\n");
        ExFreePool(sb);
        return STATUS_WRONG_VOLUME;
    }

    if (!check_superblock_checksum(sb)) {
        ExFreePool(sb);
        return STATUS_WRONG_VOLUME;
    }

    if (RtlCompareMemory(&sb->uuid, &Vcb->superblock.uuid, sizeof(BTRFS_UUID)) != sizeof(BTRFS_UUID)) {
        ERR("different UUIDs\n");
        ExFreePool(sb);
        return STATUS_WRONG_VOLUME;
    }

    ExFreePool(sb);

    dev->devobj->Flags &= ~DO_VERIFY_VOLUME;

    return STATUS_SUCCESS;
}

static NTSTATUS verify_volume(_In_ PDEVICE_OBJECT devobj) {
    device_extension* Vcb = devobj->DeviceExtension;
    NTSTATUS Status;
    LIST_ENTRY* le;
    uint64_t failed_devices = 0;
    bool locked = false, remove = false;

    if (!(Vcb->Vpb->Flags & VPB_MOUNTED))
        return STATUS_WRONG_VOLUME;

    if (!ExIsResourceAcquiredExclusive(&Vcb->tree_lock)) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);
        locked = true;
    }

    if (Vcb->removing) {
        if (locked) ExReleaseResourceLite(&Vcb->tree_lock);
        return STATUS_WRONG_VOLUME;
    }

    Status = STATUS_SUCCESS;

    InterlockedIncrement(&Vcb->open_files); // so pnp_surprise_removal doesn't uninit the device while we're still using it

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        Status = verify_device(Vcb, dev);
        if (!NT_SUCCESS(Status)) {
            failed_devices++;

            if (dev->devobj && Vcb->options.allow_degraded)
                dev->devobj = NULL;
        }

        le = le->Flink;
    }

    InterlockedDecrement(&Vcb->open_files);

    if (Vcb->removing && Vcb->open_files == 0)
        remove = true;

    if (locked)
        ExReleaseResourceLite(&Vcb->tree_lock);

    if (remove) {
        uninit(Vcb);
        return Status;
    }

    if (failed_devices == 0 || (Vcb->options.allow_degraded && failed_devices < Vcb->superblock.num_devices)) {
        Vcb->Vpb->RealDevice->Flags &= ~DO_VERIFY_VOLUME;

        return STATUS_SUCCESS;
    }

    return Status;
}

_Dispatch_type_(IRP_MJ_FILE_SYSTEM_CONTROL)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_file_system_control(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("file system control\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    } else if (!Vcb || (Vcb->type != VCB_TYPE_FS && Vcb->type != VCB_TYPE_CONTROL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_NOT_IMPLEMENTED;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Irp->IoStatus.Information = 0;

    switch (IrpSp->MinorFunction) {
        case IRP_MN_MOUNT_VOLUME:
            TRACE("IRP_MN_MOUNT_VOLUME\n");

            Status = mount_vol(DeviceObject, Irp);
            break;

        case IRP_MN_KERNEL_CALL:
            TRACE("IRP_MN_KERNEL_CALL\n");

            Status = fsctl_request(DeviceObject, &Irp, IrpSp->Parameters.FileSystemControl.FsControlCode);
            break;

        case IRP_MN_USER_FS_REQUEST:
            TRACE("IRP_MN_USER_FS_REQUEST\n");

            Status = fsctl_request(DeviceObject, &Irp, IrpSp->Parameters.FileSystemControl.FsControlCode);
            break;

        case IRP_MN_VERIFY_VOLUME:
            TRACE("IRP_MN_VERIFY_VOLUME\n");

            Status = verify_volume(DeviceObject);

            if (!NT_SUCCESS(Status) && Vcb->Vpb->Flags & VPB_MOUNTED) {
                ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);
                Vcb->removing = true;
                ExReleaseResourceLite(&Vcb->tree_lock);
            }

            break;

        default:
            break;
    }

end:
    TRACE("returning %08lx\n", Status);

    if (Irp) {
        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

_Dispatch_type_(IRP_MJ_LOCK_CONTROL)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_lock_control(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    fcb* fcb = IrpSp->FileObject ? IrpSp->FileObject->FsContext : NULL;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        goto exit;
    }

    TRACE("lock control\n");

    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

    Status = FsRtlProcessFileLock(&fcb->lock, Irp, NULL);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

exit:
    TRACE("returning %08lx\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

void do_shutdown(PIRP Irp) {
    LIST_ENTRY* le;
    bus_device_extension* bde;

    shutting_down = true;
    KeSetEvent(&mountmgr_thread_event, 0, false);

    le = VcbList.Flink;
    while (le != &VcbList) {
        LIST_ENTRY* le2 = le->Flink;

        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);
        volume_device_extension* vde = Vcb->vde;
        PDEVICE_OBJECT devobj = vde ? vde->device : NULL;

        TRACE("shutting down Vcb %p\n", Vcb);

        if (vde)
            InterlockedIncrement(&vde->open_count);

        if (devobj)
            ObReferenceObject(devobj);

        dismount_volume(Vcb, true, Irp);

        if (vde) {
            NTSTATUS Status;
            UNICODE_STRING mmdevpath;
            PDEVICE_OBJECT mountmgr;
            PFILE_OBJECT mountmgrfo;
            KIRQL irql;
            PVPB newvpb;

            RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
            Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
            if (!NT_SUCCESS(Status))
                ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
            else {
                remove_drive_letter(mountmgr, &vde->name);

                ObDereferenceObject(mountmgrfo);
            }

            vde->removing = true;

            newvpb = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), ALLOC_TAG);
            if (!newvpb) {
                ERR("out of memory\n");
                return;
            }

            RtlZeroMemory(newvpb, sizeof(VPB));

            newvpb->Type = IO_TYPE_VPB;
            newvpb->Size = sizeof(VPB);
            newvpb->RealDevice = newvpb->DeviceObject = vde->device;
            newvpb->Flags = VPB_DIRECT_WRITES_ALLOWED;

            IoAcquireVpbSpinLock(&irql);
            vde->device->Vpb = newvpb;
            IoReleaseVpbSpinLock(irql);

            if (InterlockedDecrement(&vde->open_count) == 0)
                free_vol(vde);
        }

        if (devobj)
            ObDereferenceObject(devobj);

        le = le2;
    }

#ifdef _DEBUG
    if (comfo) {
        ObDereferenceObject(comfo);
        comdo = NULL;
        comfo = NULL;
    }
#endif

    IoUnregisterFileSystem(master_devobj);

    if (notification_entry2) {
        if (fIoUnregisterPlugPlayNotificationEx)
            fIoUnregisterPlugPlayNotificationEx(notification_entry2);
        else
            IoUnregisterPlugPlayNotification(notification_entry2);

        notification_entry2 = NULL;
    }

    if (notification_entry3) {
        if (fIoUnregisterPlugPlayNotificationEx)
            fIoUnregisterPlugPlayNotificationEx(notification_entry3);
        else
            IoUnregisterPlugPlayNotification(notification_entry3);

        notification_entry3 = NULL;
    }

    if (notification_entry) {
        if (fIoUnregisterPlugPlayNotificationEx)
            fIoUnregisterPlugPlayNotificationEx(notification_entry);
        else
            IoUnregisterPlugPlayNotification(notification_entry);

        notification_entry = NULL;
    }

    bde = busobj->DeviceExtension;

    if (bde->attached_device)
        IoDetachDevice(bde->attached_device);

    IoDeleteDevice(busobj);
    IoDeleteDevice(master_devobj);
}

_Dispatch_type_(IRP_MJ_SHUTDOWN)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_shutdown(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    bool top_level;
    device_extension* Vcb = DeviceObject->DeviceExtension;

    FsRtlEnterFileSystem();

    TRACE("shutdown\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    Status = STATUS_SUCCESS;

    do_shutdown(Irp);

end:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

static bool device_still_valid(device* dev, uint64_t expected_generation) {
    NTSTATUS Status;
    unsigned int to_read;
    superblock* sb;

    to_read = (unsigned int)(dev->devobj->SectorSize == 0 ? sizeof(superblock) : sector_align(sizeof(superblock), dev->devobj->SectorSize));

    sb = ExAllocatePoolWithTag(NonPagedPool, to_read, ALLOC_TAG);
    if (!sb) {
        ERR("out of memory\n");
        return false;
    }

    Status = sync_read_phys(dev->devobj, dev->fileobj, superblock_addrs[0], to_read, (PUCHAR)sb, false);
    if (!NT_SUCCESS(Status)) {
        ERR("sync_read_phys returned %08lx\n", Status);
        ExFreePool(sb);
        return false;
    }

    if (sb->magic != BTRFS_MAGIC) {
        ERR("magic not found\n");
        ExFreePool(sb);
        return false;
    }

    if (!check_superblock_checksum(sb)) {
        ExFreePool(sb);
        return false;
    }

    if (sb->generation > expected_generation) {
        ERR("generation was %I64x, expected %I64x\n", sb->generation, expected_generation);
        ExFreePool(sb);
        return false;
    }

    ExFreePool(sb);

    return true;
}

_Function_class_(IO_WORKITEM_ROUTINE)
static void __stdcall check_after_wakeup(PDEVICE_OBJECT DeviceObject, PVOID con) {
    device_extension* Vcb = (device_extension*)con;
    LIST_ENTRY* le;

    UNUSED(DeviceObject);

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

    le = Vcb->devices.Flink;

    // FIXME - do reads in parallel?

    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj) {
            if (!device_still_valid(dev, Vcb->superblock.generation - 1)) {
                PDEVICE_OBJECT voldev = Vcb->Vpb->RealDevice;
                KIRQL irql;
                PVPB newvpb;

                WARN("forcing remount\n");

                newvpb = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), ALLOC_TAG);
                if (!newvpb) {
                    ERR("out of memory\n");
                    return;
                }

                RtlZeroMemory(newvpb, sizeof(VPB));

                newvpb->Type = IO_TYPE_VPB;
                newvpb->Size = sizeof(VPB);
                newvpb->RealDevice = voldev;
                newvpb->Flags = VPB_DIRECT_WRITES_ALLOWED;

                Vcb->removing = true;

                IoAcquireVpbSpinLock(&irql);
                voldev->Vpb = newvpb;
                IoReleaseVpbSpinLock(irql);

                Vcb->vde = NULL;

                ExReleaseResourceLite(&Vcb->tree_lock);

                if (Vcb->open_files == 0)
                    uninit(Vcb);
                else { // remove from VcbList
                    ExAcquireResourceExclusiveLite(&global_loading_lock, true);
                    RemoveEntryList(&Vcb->list_entry);
                    Vcb->list_entry.Flink = NULL;
                    ExReleaseResourceLite(&global_loading_lock);
                }

                return;
            }
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->tree_lock);
}

_Dispatch_type_(IRP_MJ_POWER)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_power(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    // no need for FsRtlEnterFileSystem, as this only ever gets called in a system thread

    top_level = is_top_level(Irp);

    Irp->IoStatus.Information = 0;

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        volume_device_extension* vde = DeviceObject->DeviceExtension;

        if (IrpSp->MinorFunction == IRP_MN_QUERY_POWER && IrpSp->Parameters.Power.Type == SystemPowerState &&
            IrpSp->Parameters.Power.State.SystemState != PowerSystemWorking && vde->mounted_device) {
            device_extension* Vcb2 = vde->mounted_device->DeviceExtension;

            /* If power state is about to go to sleep or hibernate, do a flush. We do this on IRP_MJ_QUERY_POWER
            * rather than IRP_MJ_SET_POWER because we know that the hard disks are still awake. */

            if (Vcb2) {
                ExAcquireResourceExclusiveLite(&Vcb2->tree_lock, true);

                if (Vcb2->need_write && !Vcb2->readonly) {
                    TRACE("doing protective flush on power state change\n");
                    Status = do_write(Vcb2, NULL);
                } else
                    Status = STATUS_SUCCESS;

                free_trees(Vcb2);

                if (!NT_SUCCESS(Status))
                    ERR("do_write returned %08lx\n", Status);

                ExReleaseResourceLite(&Vcb2->tree_lock);
            }
        } else if (IrpSp->MinorFunction == IRP_MN_SET_POWER && IrpSp->Parameters.Power.Type == SystemPowerState &&
            IrpSp->Parameters.Power.State.SystemState == PowerSystemWorking && vde->mounted_device) {
            device_extension* Vcb2 = vde->mounted_device->DeviceExtension;

            /* If waking up, make sure that the FS hasn't been changed while we've been out (e.g., by dual-boot Linux) */

            if (Vcb2) {
                PIO_WORKITEM work_item;

                work_item = IoAllocateWorkItem(DeviceObject);
                if (!work_item) {
                    ERR("out of memory\n");
                } else
                    IoQueueWorkItem(work_item, check_after_wakeup, DelayedWorkQueue, Vcb2);
            }
        }

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        Status = PoCallDriver(vde->attached_device, Irp);

        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_FS) {
        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);

        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_BUS) {
        bus_device_extension* bde = DeviceObject->DeviceExtension;

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        Status = PoCallDriver(bde->attached_device, Irp);

        goto exit;
    }

    if (IrpSp->MinorFunction == IRP_MN_SET_POWER || IrpSp->MinorFunction == IRP_MN_QUERY_POWER)
        Irp->IoStatus.Status = STATUS_SUCCESS;

    Status = Irp->IoStatus.Status;

    PoStartNextPowerIrp(Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

exit:
    if (top_level)
        IoSetTopLevelIrp(NULL);

    return Status;
}

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
_Function_class_(DRIVER_DISPATCH)
static NTSTATUS __stdcall drv_system_control(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
    NTSTATUS Status;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    Irp->IoStatus.Information = 0;

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        volume_device_extension* vde = DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(vde->attached_device, Irp);

        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_FS) {
        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);

        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_BUS) {
        bus_device_extension* bde = DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(bde->attached_device, Irp);

        goto exit;
    }

    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

exit:
    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS check_file_name_valid(_In_ PUNICODE_STRING us, _In_ bool posix, _In_ bool stream) {
    ULONG i;

    if (us->Length < sizeof(WCHAR))
        return STATUS_OBJECT_NAME_INVALID;

    if (us->Length > 255 * sizeof(WCHAR))
        return STATUS_OBJECT_NAME_INVALID;

    for (i = 0; i < us->Length / sizeof(WCHAR); i++) {
        if (us->Buffer[i] == '/' || us->Buffer[i] == 0 ||
            (!posix && (us->Buffer[i] == '/' || us->Buffer[i] == ':')) ||
            (!posix && !stream && (us->Buffer[i] == '<' || us->Buffer[i] == '>' || us->Buffer[i] == '"' ||
            us->Buffer[i] == '|' || us->Buffer[i] == '?' || us->Buffer[i] == '*' || (us->Buffer[i] >= 1 && us->Buffer[i] <= 31))))
            return STATUS_OBJECT_NAME_INVALID;

        /* Don't allow unpaired surrogates ("WTF-16") */

        if ((us->Buffer[i] & 0xfc00) == 0xdc00 && (i == 0 || ((us->Buffer[i-1] & 0xfc00) != 0xd800)))
            return STATUS_OBJECT_NAME_INVALID;

        if ((us->Buffer[i] & 0xfc00) == 0xd800 && (i == (us->Length / sizeof(WCHAR)) - 1 || ((us->Buffer[i+1] & 0xfc00) != 0xdc00)))
            return STATUS_OBJECT_NAME_INVALID;
    }

    if (us->Buffer[0] == '.' && (us->Length == sizeof(WCHAR) || (us->Length == 2 * sizeof(WCHAR) && us->Buffer[1] == '.')))
        return STATUS_OBJECT_NAME_INVALID;

    /* The Linux driver expects filenames with a maximum length of 255 bytes - make sure
     * that our UTF-8 length won't be longer than that. */
    if (us->Length >= 85 * sizeof(WCHAR)) {
        NTSTATUS Status;
        ULONG utf8len;

        Status = utf16_to_utf8(NULL, 0, &utf8len, us->Buffer, us->Length);
        if (!NT_SUCCESS(Status))
            return Status;

        if (utf8len > 255)
            return STATUS_OBJECT_NAME_INVALID;
        else if (stream && utf8len > 250) // minus five bytes for "user."
            return STATUS_OBJECT_NAME_INVALID;
    }

    return STATUS_SUCCESS;
}

void chunk_lock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ uint64_t start, _In_ uint64_t length) {
    LIST_ENTRY* le;
    bool locked;
    range_lock* rl;

    rl = ExAllocateFromNPagedLookasideList(&Vcb->range_lock_lookaside);
    if (!rl) {
        ERR("out of memory\n");
        return;
    }

    rl->start = start;
    rl->length = length;
    rl->thread = PsGetCurrentThread();

    while (true) {
        locked = false;

        ExAcquireResourceExclusiveLite(&c->range_locks_lock, true);

        le = c->range_locks.Flink;
        while (le != &c->range_locks) {
            range_lock* rl2 = CONTAINING_RECORD(le, range_lock, list_entry);

            if (rl2->start < start + length && rl2->start + rl2->length > start && rl2->thread != PsGetCurrentThread()) {
                locked = true;
                break;
            }

            le = le->Flink;
        }

        if (!locked) {
            InsertTailList(&c->range_locks, &rl->list_entry);

            ExReleaseResourceLite(&c->range_locks_lock);
            return;
        }

        KeClearEvent(&c->range_locks_event);

        ExReleaseResourceLite(&c->range_locks_lock);

        KeWaitForSingleObject(&c->range_locks_event, UserRequest, KernelMode, false, NULL);
    }
}

void chunk_unlock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ uint64_t start, _In_ uint64_t length) {
    LIST_ENTRY* le;

    ExAcquireResourceExclusiveLite(&c->range_locks_lock, true);

    le = c->range_locks.Flink;
    while (le != &c->range_locks) {
        range_lock* rl = CONTAINING_RECORD(le, range_lock, list_entry);

        if (rl->start == start && rl->length == length) {
            RemoveEntryList(&rl->list_entry);
            ExFreeToNPagedLookasideList(&Vcb->range_lock_lookaside, rl);
            break;
        }

        le = le->Flink;
    }

    KeSetEvent(&c->range_locks_event, 0, false);

    ExReleaseResourceLite(&c->range_locks_lock);
}

void log_device_error(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ int error) {
    dev->stats[error]++;
    dev->stats_changed = true;
    Vcb->stats_changed = true;
}

#ifdef _DEBUG
_Function_class_(KSTART_ROUTINE)
static void __stdcall serial_thread(void* context) {
    LARGE_INTEGER due_time;
    KTIMER timer;

    UNUSED(context);

    KeInitializeTimer(&timer);

    due_time.QuadPart = (uint64_t)-10000000;

    KeSetTimer(&timer, due_time, NULL);

    while (true) {
        KeWaitForSingleObject(&timer, Executive, KernelMode, false, NULL);

        init_serial(false);

        if (comdo)
            break;

        KeSetTimer(&timer, due_time, NULL);
    }

    KeCancelTimer(&timer);

    PsTerminateSystemThread(STATUS_SUCCESS);

    serial_thread_handle = NULL;
}

static void init_serial(bool first_time) {
    NTSTATUS Status;

    Status = IoGetDeviceObjectPointer(&log_device, FILE_WRITE_DATA, &comfo, &comdo);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);

        if (first_time) {
            OBJECT_ATTRIBUTES oa;

            InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

            Status = PsCreateSystemThread(&serial_thread_handle, 0, &oa, NULL, NULL, serial_thread, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("PsCreateSystemThread returned %08lx\n", Status);
                return;
            }
        }
    }
}
#endif

#if !defined(__REACTOS__) && (defined(_X86_) || defined(_AMD64_))
static void check_cpu() {
    bool have_sse2 = false, have_sse42 = false, have_avx2 = false;

#ifndef _MSC_VER
    {
        uint32_t eax, ebx, ecx, edx;

        __cpuid(1, eax, ebx, ecx, edx);

        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            have_sse42 = ecx & bit_SSE4_2;
            have_sse2 = edx & bit_SSE2;
        }

        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
            have_avx2 = ebx & bit_AVX2;

        if (have_avx2) {
            // check Windows has enabled AVX2 - Windows 10 doesn't immediately

            if (__readcr4() & (1 << 18)) {
                uint32_t xcr0;

                __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "edx" );

                if ((xcr0 & 6) != 6)
                    have_avx2 = false;
            } else
                have_avx2 = false;
        }
    }
#else
    {
        unsigned int cpu_info[4];

        __cpuid(cpu_info, 1);
        have_sse42 = cpu_info[2] & (1 << 20);
        have_sse2 = cpu_info[3] & (1 << 26);

        __cpuidex(cpu_info, 7, 0);
        have_avx2 = cpu_info[1] & (1 << 5);

        if (have_avx2) {
            // check Windows has enabled AVX2 - Windows 10 doesn't immediately

            if (__readcr4() & (1 << 18)) {
                uint32_t xcr0 = (uint32_t)_xgetbv(0);

                if ((xcr0 & 6) != 6)
                    have_avx2 = false;
            } else
                have_avx2 = false;
        }
    }
#endif

    if (have_sse42) {
        TRACE("SSE4.2 is supported\n");
        calc_crc32c = calc_crc32c_hw;
    } else
        TRACE("SSE4.2 not supported\n");

    if (have_sse2) {
        TRACE("SSE2 is supported\n");

        if (!have_avx2)
            do_xor = do_xor_sse2;
    } else
        TRACE("SSE2 is not supported\n");

    if (have_avx2) {
        TRACE("AVX2 is supported\n");
        do_xor = do_xor_avx2;
    } else
        TRACE("AVX2 is not supported\n");
}
#endif

#ifdef _DEBUG
static void init_logging() {
    ExAcquireResourceExclusiveLite(&log_lock, true);

    if (log_device.Length > 0)
        init_serial(true);
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
            ERR("ZwCreateFile returned %08lx\n", Status);
            goto end;
        }

        if (iosb.Information == FILE_OPENED) { // already exists
            FILE_STANDARD_INFORMATION fsi;
            FILE_POSITION_INFORMATION fpi;

            static const char delim[] = "\n---\n";

            // move to end of file

            Status = ZwQueryInformationFile(log_handle, &iosb, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);

            if (!NT_SUCCESS(Status)) {
                ERR("ZwQueryInformationFile returned %08lx\n", Status);
                goto end;
            }

            fpi.CurrentByteOffset = fsi.EndOfFile;

            Status = ZwSetInformationFile(log_handle, &iosb, &fpi, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation);

            if (!NT_SUCCESS(Status)) {
                ERR("ZwSetInformationFile returned %08lx\n", Status);
                goto end;
            }

            Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, (void*)delim, sizeof(delim) - 1, NULL, NULL);

            if (!NT_SUCCESS(Status)) {
                ERR("ZwWriteFile returned %08lx\n", Status);
                goto end;
            }
        }

        dateline = ExAllocatePoolWithTag(PagedPool, 256, ALLOC_TAG);

        if (!dateline) {
            ERR("out of memory\n");
            goto end;
        }

        KeQuerySystemTime(&time);

        RtlTimeToTimeFields(&time, &tf);

        sprintf(dateline, "Starting logging at %04i-%02i-%02i %02i:%02i:%02i\n", tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second);

        Status = ZwWriteFile(log_handle, NULL, NULL, NULL, &iosb, dateline, (ULONG)strlen(dateline), NULL, NULL);

        ExFreePool(dateline);

        if (!NT_SUCCESS(Status)) {
            ERR("ZwWriteFile returned %08lx\n", Status);
            goto end;
        }
    }

end:
    ExReleaseResourceLite(&log_lock);
}
#endif

_Function_class_(KSTART_ROUTINE)
static void __stdcall degraded_wait_thread(_In_ void* context) {
    KTIMER timer;
    LARGE_INTEGER delay;

    UNUSED(context);

    KeInitializeTimer(&timer);

    delay.QuadPart = -30000000; // wait three seconds
    KeSetTimer(&timer, delay, NULL);
    KeWaitForSingleObject(&timer, Executive, KernelMode, false, NULL);

    TRACE("timer expired\n");

    degraded_wait = false;

    ZwClose(degraded_wait_handle);
    degraded_wait_handle = NULL;

    PsTerminateSystemThread(STATUS_SUCCESS);
}

_Function_class_(DRIVER_ADD_DEVICE)
NTSTATUS __stdcall AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    UNICODE_STRING volname;
    ULONG i;
    WCHAR* s;
    pdo_device_extension* pdode = NULL;
    PDEVICE_OBJECT voldev;
    volume_device_extension* vde;
    UNICODE_STRING arc_name_us;
    WCHAR* anp;

    static const WCHAR arc_name_prefix[] = L"\\ArcName\\btrfs(";

    WCHAR arc_name[(sizeof(arc_name_prefix) / sizeof(WCHAR)) - 1 + 37];

    TRACE("(%p, %p)\n", DriverObject, PhysicalDeviceObject);

    UNUSED(DriverObject);

    ExAcquireResourceSharedLite(&pdo_list_lock, true);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode2 = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

        if (pdode2->pdo == PhysicalDeviceObject) {
            pdode = pdode2;
            break;
        }

        le = le->Flink;
    }

    if (!pdode) {
        WARN("unrecognized PDO %p\n", PhysicalDeviceObject);
        Status = STATUS_NOT_SUPPORTED;
        goto end;
    }

    ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

    if (pdode->vde) { // if already done, return success
        Status = STATUS_SUCCESS;
        goto end2;
    }

    volname.Length = volname.MaximumLength = (sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR)) + ((36 + 1) * sizeof(WCHAR));
    volname.Buffer = ExAllocatePoolWithTag(PagedPool, volname.MaximumLength, ALLOC_TAG); // FIXME - when do we free this?

    if (!volname.Buffer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end2;
    }

    RtlCopyMemory(volname.Buffer, BTRFS_VOLUME_PREFIX, sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR));
    RtlCopyMemory(arc_name, arc_name_prefix, sizeof(arc_name_prefix) - sizeof(WCHAR));

    anp = &arc_name[(sizeof(arc_name_prefix) / sizeof(WCHAR)) - 1];
    s = &volname.Buffer[(sizeof(BTRFS_VOLUME_PREFIX) / sizeof(WCHAR)) - 1];

    for (i = 0; i < 16; i++) {
        *s = *anp = hex_digit(pdode->uuid.uuid[i] >> 4);
        s++;
        anp++;

        *s = *anp = hex_digit(pdode->uuid.uuid[i] & 0xf);
        s++;
        anp++;

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *s = *anp = '-';
            s++;
            anp++;
        }
    }

    *s = '}';
    *anp = ')';

    Status = IoCreateDevice(drvobj, sizeof(volume_device_extension), &volname, FILE_DEVICE_DISK,
                            is_windows_8 ? FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL : 0, false, &voldev);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08lx\n", Status);
        goto end2;
    }

    arc_name_us.Buffer = arc_name;
    arc_name_us.Length = arc_name_us.MaximumLength = sizeof(arc_name);

    Status = IoCreateSymbolicLink(&arc_name_us, &volname);
    if (!NT_SUCCESS(Status))
        WARN("IoCreateSymbolicLink returned %08lx\n", Status);

    voldev->SectorSize = PhysicalDeviceObject->SectorSize;
    voldev->Flags |= DO_DIRECT_IO;

    vde = voldev->DeviceExtension;
    vde->type = VCB_TYPE_VOLUME;
    vde->name = volname;
    vde->device = voldev;
    vde->mounted_device = NULL;
    vde->pdo = PhysicalDeviceObject;
    vde->pdode = pdode;
    vde->removing = false;
    vde->dead = false;
    vde->open_count = 0;

    Status = IoRegisterDeviceInterface(PhysicalDeviceObject, &GUID_DEVINTERFACE_VOLUME, NULL, &vde->bus_name);
    if (!NT_SUCCESS(Status))
        WARN("IoRegisterDeviceInterface returned %08lx\n", Status);

    vde->attached_device = IoAttachDeviceToDeviceStack(voldev, PhysicalDeviceObject);

    pdode->vde = vde;

    if (pdode->removable)
        voldev->Characteristics |= FILE_REMOVABLE_MEDIA;

    if (RtlCompareMemory(&boot_uuid, &pdode->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
        voldev->Flags |= DO_SYSTEM_BOOT_PARTITION;
        PhysicalDeviceObject->Flags |= DO_SYSTEM_BOOT_PARTITION;
    }

    voldev->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoSetDeviceInterfaceState(&vde->bus_name, true);
    if (!NT_SUCCESS(Status))
        WARN("IoSetDeviceInterfaceState returned %08lx\n", Status);

    Status = STATUS_SUCCESS;

end2:
    ExReleaseResourceLite(&pdode->child_lock);

end:
    ExReleaseResourceLite(&pdo_list_lock);

    return Status;
}

_Function_class_(DRIVER_INITIALIZE)
NTSTATUS __stdcall DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING device_nameW;
    UNICODE_STRING dosdevice_nameW;
    control_device_extension* cde;
    bus_device_extension* bde;
    HANDLE regh;
    OBJECT_ATTRIBUTES oa, system_thread_attributes;
    ULONG dispos;
    RTL_OSVERSIONINFOW ver;

    ver.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

    Status = RtlGetVersion(&ver);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetVersion returned %08lx\n", Status);
        return Status;
    }

    is_windows_8 = ver.dwMajorVersion > 6 || (ver.dwMajorVersion == 6 && ver.dwMinorVersion >= 2);

    KeInitializeSpinLock(&fve_data_lock);

    InitializeListHead(&uid_map_list);
    InitializeListHead(&gid_map_list);

#ifdef _DEBUG
    ExInitializeResourceLite(&log_lock);
#endif
    ExInitializeResourceLite(&mapping_lock);

    log_device.Buffer = NULL;
    log_device.Length = log_device.MaximumLength = 0;
    log_file.Buffer = NULL;
    log_file.Length = log_file.MaximumLength = 0;

    registry_path.Length = registry_path.MaximumLength = RegistryPath->Length;
    registry_path.Buffer = ExAllocatePoolWithTag(PagedPool, registry_path.Length, ALLOC_TAG);

    if (!registry_path.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(registry_path.Buffer, RegistryPath->Buffer, registry_path.Length);

    read_registry(&registry_path, false);

#ifdef _DEBUG
    if (debug_log_level > 0)
        init_logging();

    log_started = true;
#endif

    TRACE("DriverEntry\n");

#if !defined(__REACTOS__) && (defined(_X86_) || defined(_AMD64_))
    check_cpu();
#endif

    if (ver.dwMajorVersion > 6 || (ver.dwMajorVersion == 6 && ver.dwMinorVersion >= 2)) { // Windows 8 or above
        UNICODE_STRING name;
        tPsIsDiskCountersEnabled fPsIsDiskCountersEnabled;

        RtlInitUnicodeString(&name, L"PsIsDiskCountersEnabled");
        fPsIsDiskCountersEnabled = (tPsIsDiskCountersEnabled)MmGetSystemRoutineAddress(&name);

        if (fPsIsDiskCountersEnabled) {
            diskacc = fPsIsDiskCountersEnabled();

            RtlInitUnicodeString(&name, L"PsUpdateDiskCounters");
            fPsUpdateDiskCounters = (tPsUpdateDiskCounters)MmGetSystemRoutineAddress(&name);

            if (!fPsUpdateDiskCounters)
                diskacc = false;

            RtlInitUnicodeString(&name, L"FsRtlUpdateDiskCounters");
            fFsRtlUpdateDiskCounters = (tFsRtlUpdateDiskCounters)MmGetSystemRoutineAddress(&name);
        }

        RtlInitUnicodeString(&name, L"CcCopyReadEx");
        fCcCopyReadEx = (tCcCopyReadEx)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"CcCopyWriteEx");
        fCcCopyWriteEx = (tCcCopyWriteEx)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"CcSetAdditionalCacheAttributesEx");
        fCcSetAdditionalCacheAttributesEx = (tCcSetAdditionalCacheAttributesEx)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"FsRtlCheckLockForOplockRequest");
        fFsRtlCheckLockForOplockRequest = (tFsRtlCheckLockForOplockRequest)MmGetSystemRoutineAddress(&name);
    } else {
        fPsUpdateDiskCounters = NULL;
        fCcCopyReadEx = NULL;
        fCcCopyWriteEx = NULL;
        fCcSetAdditionalCacheAttributesEx = NULL;
        fFsRtlUpdateDiskCounters = NULL;
        fFsRtlCheckLockForOplockRequest = NULL;
    }

    if (ver.dwMajorVersion > 6 || (ver.dwMajorVersion == 6 && ver.dwMinorVersion >= 1)) { // Windows 7 or above
        UNICODE_STRING name;

        RtlInitUnicodeString(&name, L"IoUnregisterPlugPlayNotificationEx");
        fIoUnregisterPlugPlayNotificationEx = (tIoUnregisterPlugPlayNotificationEx)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"FsRtlAreThereCurrentOrInProgressFileLocks");
        fFsRtlAreThereCurrentOrInProgressFileLocks = (tFsRtlAreThereCurrentOrInProgressFileLocks)MmGetSystemRoutineAddress(&name);
    } else {
        fIoUnregisterPlugPlayNotificationEx = NULL;
        fFsRtlAreThereCurrentOrInProgressFileLocks = NULL;
    }

    if (ver.dwMajorVersion >= 6) { // Windows Vista or above
        UNICODE_STRING name;

        RtlInitUnicodeString(&name, L"FsRtlGetEcpListFromIrp");
        fFsRtlGetEcpListFromIrp = (tFsRtlGetEcpListFromIrp)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"FsRtlGetNextExtraCreateParameter");
        fFsRtlGetNextExtraCreateParameter = (tFsRtlGetNextExtraCreateParameter)MmGetSystemRoutineAddress(&name);

        RtlInitUnicodeString(&name, L"FsRtlValidateReparsePointBuffer");
        fFsRtlValidateReparsePointBuffer = (tFsRtlValidateReparsePointBuffer)MmGetSystemRoutineAddress(&name);
    } else {
        fFsRtlGetEcpListFromIrp = NULL;
        fFsRtlGetNextExtraCreateParameter = NULL;
        fFsRtlValidateReparsePointBuffer = compat_FsRtlValidateReparsePointBuffer;
    }

    drvobj = DriverObject;

    DriverObject->DriverUnload = DriverUnload;

    DriverObject->DriverExtension->AddDevice = AddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = drv_create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = drv_close;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = drv_read;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = drv_write;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = drv_query_information;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = drv_set_information;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = drv_query_ea;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = drv_set_ea;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = drv_flush_buffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = drv_query_volume_information;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = drv_set_volume_information;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = drv_directory_control;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = drv_file_system_control;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = drv_device_control;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = drv_shutdown;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = drv_lock_control;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = drv_cleanup;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]           = drv_query_security;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]             = drv_set_security;
    DriverObject->MajorFunction[IRP_MJ_POWER]                    = drv_power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]           = drv_system_control;
    DriverObject->MajorFunction[IRP_MJ_PNP]                      = drv_pnp;

    init_fast_io_dispatch(&DriverObject->FastIoDispatch);

    device_nameW.Buffer = (WCHAR*)device_name;
    device_nameW.Length = device_nameW.MaximumLength = sizeof(device_name) - sizeof(WCHAR);
    dosdevice_nameW.Buffer = (WCHAR*)dosdevice_name;
    dosdevice_nameW.Length = dosdevice_nameW.MaximumLength = sizeof(dosdevice_name) - sizeof(WCHAR);

    Status = IoCreateDevice(DriverObject, sizeof(control_device_extension), &device_nameW, FILE_DEVICE_DISK_FILE_SYSTEM,
                            FILE_DEVICE_SECURE_OPEN, false, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08lx\n", Status);
        return Status;
    }

    master_devobj = DeviceObject;
    cde = (control_device_extension*)master_devobj->DeviceExtension;

    RtlZeroMemory(cde, sizeof(control_device_extension));

    cde->type = VCB_TYPE_CONTROL;

    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoCreateSymbolicLink(&dosdevice_nameW, &device_nameW);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateSymbolicLink returned %08lx\n", Status);
        return Status;
    }

    init_cache();

    InitializeListHead(&VcbList);
    ExInitializeResourceLite(&global_loading_lock);
    ExInitializeResourceLite(&pdo_list_lock);

    InitializeListHead(&pdo_list);

    InitializeObjectAttributes(&oa, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwCreateKey(&regh, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08lx\n", Status);
        return Status;
    }

    watch_registry(regh);

    Status = IoCreateDevice(DriverObject, sizeof(bus_device_extension), NULL, FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN, false, &busobj);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08lx\n", Status);
        return Status;
    }

    bde = (bus_device_extension*)busobj->DeviceExtension;

    RtlZeroMemory(bde, sizeof(bus_device_extension));

    bde->type = VCB_TYPE_BUS;

    Status = IoReportDetectedDevice(drvobj, InterfaceTypeUndefined, 0xFFFFFFFF, 0xFFFFFFFF,
                                    NULL, NULL, 0, &bde->buspdo);
    if (!NT_SUCCESS(Status)) {
        ERR("IoReportDetectedDevice returned %08lx\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(bde->buspdo, &BtrfsBusInterface, NULL, &bde->bus_name);
    if (!NT_SUCCESS(Status))
        WARN("IoRegisterDeviceInterface returned %08lx\n", Status);

    bde->attached_device = IoAttachDeviceToDeviceStack(busobj, bde->buspdo);

    busobj->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoSetDeviceInterfaceState(&bde->bus_name, true);
    if (!NT_SUCCESS(Status))
        WARN("IoSetDeviceInterfaceState returned %08lx\n", Status);

    IoInvalidateDeviceRelations(bde->buspdo, BusRelations);

    InitializeObjectAttributes(&system_thread_attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread(&degraded_wait_handle, 0, &system_thread_attributes, NULL, NULL, degraded_wait_thread, NULL);
    if (!NT_SUCCESS(Status))
        WARN("PsCreateSystemThread returned %08lx\n", Status);

    ExInitializeResourceLite(&boot_lock);

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange, PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVINTERFACE_VOLUME, DriverObject, volume_notification, NULL, &notification_entry2);
    if (!NT_SUCCESS(Status))
        ERR("IoRegisterPlugPlayNotification returned %08lx\n", Status);

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange, PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVINTERFACE_HIDDEN_VOLUME, DriverObject, volume_notification, NULL, &notification_entry3);
    if (!NT_SUCCESS(Status))
        ERR("IoRegisterPlugPlayNotification returned %08lx\n", Status);

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange, PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVINTERFACE_DISK, DriverObject, pnp_notification, DriverObject, &notification_entry);
    if (!NT_SUCCESS(Status))
        ERR("IoRegisterPlugPlayNotification returned %08lx\n", Status);

    finished_probing = true;

    KeInitializeEvent(&mountmgr_thread_event, NotificationEvent, false);

    // Status = PsCreateSystemThread(&mountmgr_thread_handle, 0, &system_thread_attributes, NULL, NULL, mountmgr_thread, NULL);
    // if (!NT_SUCCESS(Status))
    //     WARN("PsCreateSystemThread returned %08lx\n", Status);

    IoRegisterFileSystem(DeviceObject);

    check_system_root();

    return STATUS_SUCCESS;
}
