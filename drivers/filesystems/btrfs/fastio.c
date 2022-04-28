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

FAST_IO_DISPATCH FastIoDispatch;

_Function_class_(FAST_IO_QUERY_BASIC_INFO)
static BOOLEAN __stdcall fast_query_basic_info(PFILE_OBJECT FileObject, BOOLEAN wait, PFILE_BASIC_INFORMATION fbi,
                                               PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb;
    ccb* ccb;

    UNUSED(DeviceObject);

    FsRtlEnterFileSystem();

    TRACE("(%p, %u, %p, %p, %p)\n", FileObject, wait, fbi, IoStatus, DeviceObject);

    if (!FileObject) {
        FsRtlExitFileSystem();
        return false;
    }

    fcb = FileObject->FsContext;

    if (!fcb) {
        FsRtlExitFileSystem();
        return false;
    }

    ccb = FileObject->FsContext2;

    if (!ccb) {
        FsRtlExitFileSystem();
        return false;
    }

    if (!(ccb->access & (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES))) {
        FsRtlExitFileSystem();
        return false;
    }

    if (fcb->ads) {
        if (!ccb->fileref || !ccb->fileref->parent || !ccb->fileref->parent->fcb) {
            FsRtlExitFileSystem();
            return false;
        }

        fcb = ccb->fileref->parent->fcb;
    }

    if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
        FsRtlExitFileSystem();
        return false;
    }

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fbi->CreationTime = fbi->LastAccessTime = fbi->LastWriteTime = fbi->ChangeTime = time;
    } else {
        fbi->CreationTime.QuadPart = unix_time_to_win(&fcb->inode_item.otime);
        fbi->LastAccessTime.QuadPart = unix_time_to_win(&fcb->inode_item.st_atime);
        fbi->LastWriteTime.QuadPart = unix_time_to_win(&fcb->inode_item.st_mtime);
        fbi->ChangeTime.QuadPart = unix_time_to_win(&fcb->inode_item.st_ctime);
    }

    fbi->FileAttributes = fcb->atts == 0 ? FILE_ATTRIBUTE_NORMAL : fcb->atts;

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

    ExReleaseResourceLite(fcb->Header.Resource);

    FsRtlExitFileSystem();

    return true;
}

_Function_class_(FAST_IO_QUERY_STANDARD_INFO)
static BOOLEAN __stdcall fast_query_standard_info(PFILE_OBJECT FileObject, BOOLEAN wait, PFILE_STANDARD_INFORMATION fsi,
                                                  PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb;
    ccb* ccb;
    bool ads;
    ULONG adssize;

    UNUSED(DeviceObject);

    FsRtlEnterFileSystem();

    TRACE("(%p, %u, %p, %p, %p)\n", FileObject, wait, fsi, IoStatus, DeviceObject);

    if (!FileObject) {
        FsRtlExitFileSystem();
        return false;
    }

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (!fcb) {
        FsRtlExitFileSystem();
        return false;
    }

    if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
        FsRtlExitFileSystem();
        return false;
    }

    ads = fcb->ads;

    if (ads) {
        struct _fcb* fcb2;

        if (!ccb || !ccb->fileref || !ccb->fileref->parent || !ccb->fileref->parent->fcb) {
            ExReleaseResourceLite(fcb->Header.Resource);
            FsRtlExitFileSystem();
            return false;
        }

        adssize = fcb->adsdata.Length;

        fcb2 = ccb->fileref->parent->fcb;

        ExReleaseResourceLite(fcb->Header.Resource);

        fcb = fcb2;

        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
            FsRtlExitFileSystem();
            return false;
        }

        fsi->AllocationSize.QuadPart = fsi->EndOfFile.QuadPart = adssize;
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;
        fsi->Directory = false;
    } else {
        fsi->AllocationSize.QuadPart = fcb_alloc_size(fcb);
        fsi->EndOfFile.QuadPart = S_ISDIR(fcb->inode_item.st_mode) ? 0 : fcb->inode_item.st_size;
        fsi->NumberOfLinks = fcb->inode_item.st_nlink;
        fsi->Directory = S_ISDIR(fcb->inode_item.st_mode);
    }

    fsi->DeletePending = ccb->fileref ? ccb->fileref->delete_on_close : false;

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

    ExReleaseResourceLite(fcb->Header.Resource);

    FsRtlExitFileSystem();

    return true;
}

_Function_class_(FAST_IO_CHECK_IF_POSSIBLE)
static BOOLEAN __stdcall fast_io_check_if_possible(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait,
                                                   ULONG LockKey, BOOLEAN CheckForReadOperation, PIO_STATUS_BLOCK IoStatus,
                                                   PDEVICE_OBJECT DeviceObject) {
    fcb* fcb = FileObject->FsContext;
    LARGE_INTEGER len2;

    UNUSED(Wait);
    UNUSED(IoStatus);
    UNUSED(DeviceObject);

    len2.QuadPart = Length;

    if (CheckForReadOperation) {
        if (FsRtlFastCheckLockForRead(&fcb->lock, FileOffset, &len2, LockKey, FileObject, PsGetCurrentProcess()))
            return true;
    } else {
        if (!fcb->Vcb->readonly && !is_subvol_readonly(fcb->subvol, NULL) && FsRtlFastCheckLockForWrite(&fcb->lock, FileOffset, &len2, LockKey, FileObject, PsGetCurrentProcess()))
            return true;
    }

    return false;
}

_Function_class_(FAST_IO_QUERY_NETWORK_OPEN_INFO)
static BOOLEAN __stdcall fast_io_query_network_open_info(PFILE_OBJECT FileObject, BOOLEAN Wait, FILE_NETWORK_OPEN_INFORMATION* fnoi,
                                                         PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;

    UNUSED(Wait);
    UNUSED(IoStatus); // FIXME - really? What about IoStatus->Information?
    UNUSED(DeviceObject);

    FsRtlEnterFileSystem();

    TRACE("(%p, %u, %p, %p, %p)\n", FileObject, Wait, fnoi, IoStatus, DeviceObject);

    RtlZeroMemory(fnoi, sizeof(FILE_NETWORK_OPEN_INFORMATION));

    fcb = FileObject->FsContext;

    if (!fcb || fcb == fcb->Vcb->volume_fcb) {
        FsRtlExitFileSystem();
        return false;
    }

    ccb = FileObject->FsContext2;

    if (!ccb) {
        FsRtlExitFileSystem();
        return false;
    }

    fileref = ccb->fileref;

    if (fcb == fcb->Vcb->dummy_fcb) {
        LARGE_INTEGER time;

        KeQuerySystemTime(&time);
        fnoi->CreationTime = fnoi->LastAccessTime = fnoi->LastWriteTime = fnoi->ChangeTime = time;
    } else {
        INODE_ITEM* ii;

        if (fcb->ads) {
            if (!fileref || !fileref->parent) {
                ERR("no fileref for stream\n");
                FsRtlExitFileSystem();
                return false;
            }

            ii = &fileref->parent->fcb->inode_item;
        } else
            ii = &fcb->inode_item;

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

    FsRtlExitFileSystem();

    return true;
}

_Function_class_(FAST_IO_ACQUIRE_FOR_MOD_WRITE)
static NTSTATUS __stdcall fast_io_acquire_for_mod_write(PFILE_OBJECT FileObject, PLARGE_INTEGER EndingOffset,
                                                        struct _ERESOURCE **ResourceToRelease, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb;

    TRACE("(%p, %I64x, %p, %p)\n", FileObject, EndingOffset ? EndingOffset->QuadPart : 0, ResourceToRelease, DeviceObject);

    UNUSED(EndingOffset);
    UNUSED(DeviceObject);

    fcb = FileObject->FsContext;

    if (!fcb)
        return STATUS_INVALID_PARAMETER;

    // Make sure we don't get interrupted by the flush thread, which can cause a deadlock

    if (!ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, false))
        return STATUS_CANT_WAIT;

    if (!ExAcquireResourceExclusiveLite(fcb->Header.Resource, false)) {
        ExReleaseResourceLite(&fcb->Vcb->tree_lock);
        TRACE("returning STATUS_CANT_WAIT\n");
        return STATUS_CANT_WAIT;
    }

    // Ideally this would be PagingIoResource, but that doesn't play well with copy-on-write,
    // as we can't guarantee that we won't need to do any reallocations.

    *ResourceToRelease = fcb->Header.Resource;

    TRACE("returning STATUS_SUCCESS\n");

    return STATUS_SUCCESS;
}

_Function_class_(FAST_IO_RELEASE_FOR_MOD_WRITE)
static NTSTATUS __stdcall fast_io_release_for_mod_write(PFILE_OBJECT FileObject, struct _ERESOURCE *ResourceToRelease,
                                                        PDEVICE_OBJECT DeviceObject) {
    fcb* fcb;

    TRACE("(%p, %p, %p)\n", FileObject, ResourceToRelease, DeviceObject);

    UNUSED(DeviceObject);

    fcb = FileObject->FsContext;

    ExReleaseResourceLite(ResourceToRelease);

    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    return STATUS_SUCCESS;
}

_Function_class_(FAST_IO_ACQUIRE_FOR_CCFLUSH)
static NTSTATUS __stdcall fast_io_acquire_for_ccflush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject) {
    UNUSED(FileObject);
    UNUSED(DeviceObject);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return STATUS_SUCCESS;
}

_Function_class_(FAST_IO_RELEASE_FOR_CCFLUSH)
static NTSTATUS __stdcall fast_io_release_for_ccflush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject) {
    UNUSED(FileObject);
    UNUSED(DeviceObject);

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
        IoSetTopLevelIrp(NULL);

    return STATUS_SUCCESS;
}

_Function_class_(FAST_IO_WRITE)
static BOOLEAN __stdcall fast_io_write(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, PVOID Buffer, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb = FileObject->FsContext;
    bool ret;

    FsRtlEnterFileSystem();

    if (!ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, Wait)) {
        FsRtlExitFileSystem();
        return false;
    }

    ret = FsRtlCopyWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);

    if (ret)
        fcb->inode_item.st_size = fcb->Header.FileSize.QuadPart;

    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    FsRtlExitFileSystem();

    return ret;
}

_Function_class_(FAST_IO_LOCK)
static BOOLEAN __stdcall fast_io_lock(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId,
                                      ULONG Key, BOOLEAN FailImmediately, BOOLEAN ExclusiveLock, PIO_STATUS_BLOCK IoStatus,
                                      PDEVICE_OBJECT DeviceObject) {
    BOOLEAN ret;
    fcb* fcb = FileObject->FsContext;

    UNUSED(DeviceObject);

    TRACE("(%p, %I64x, %I64x, %p, %lx, %u, %u, %p, %p)\n", FileObject, FileOffset ? FileOffset->QuadPart : 0, Length ? Length->QuadPart : 0,
          ProcessId, Key, FailImmediately, ExclusiveLock, IoStatus, DeviceObject);

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("can only lock files\n");
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;
        return true;
    }

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    ret = FsRtlFastLock(&fcb->lock, FileObject, FileOffset, Length, ProcessId, Key, FailImmediately,
                        ExclusiveLock, IoStatus, NULL, false);

    if (ret)
        fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    ExReleaseResourceLite(fcb->Header.Resource);
    FsRtlExitFileSystem();

    return ret;
}

_Function_class_(FAST_IO_UNLOCK_SINGLE)
static BOOLEAN __stdcall fast_io_unlock_single(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId,
                                               ULONG Key, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb = FileObject->FsContext;

    UNUSED(DeviceObject);

    TRACE("(%p, %I64x, %I64x, %p, %lx, %p, %p)\n", FileObject, FileOffset ? FileOffset->QuadPart : 0, Length ? Length->QuadPart : 0,
          ProcessId, Key, IoStatus, DeviceObject);

    IoStatus->Information = 0;

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("can only lock files\n");
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return true;
    }

    FsRtlEnterFileSystem();

    IoStatus->Status = FsRtlFastUnlockSingle(&fcb->lock, FileObject, FileOffset, Length, ProcessId, Key, NULL, false);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    FsRtlExitFileSystem();

    return true;
}

_Function_class_(FAST_IO_UNLOCK_ALL)
static BOOLEAN __stdcall fast_io_unlock_all(PFILE_OBJECT FileObject, PEPROCESS ProcessId, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb = FileObject->FsContext;

    UNUSED(DeviceObject);

    TRACE("(%p, %p, %p, %p)\n", FileObject, ProcessId, IoStatus, DeviceObject);

    IoStatus->Information = 0;

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("can only lock files\n");
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return true;
    }

    FsRtlEnterFileSystem();

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    IoStatus->Status = FsRtlFastUnlockAll(&fcb->lock, FileObject, ProcessId, NULL);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    ExReleaseResourceLite(fcb->Header.Resource);

    FsRtlExitFileSystem();

    return true;
}

_Function_class_(FAST_IO_UNLOCK_ALL_BY_KEY)
static BOOLEAN __stdcall fast_io_unlock_all_by_key(PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key,
                                                   PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject) {
    fcb* fcb = FileObject->FsContext;

    UNUSED(DeviceObject);

    TRACE("(%p, %p, %lx, %p, %p)\n", FileObject, ProcessId, Key, IoStatus, DeviceObject);

    IoStatus->Information = 0;

    if (fcb->type != BTRFS_TYPE_FILE) {
        WARN("can only lock files\n");
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return true;
    }

    FsRtlEnterFileSystem();

    ExAcquireResourceSharedLite(fcb->Header.Resource, true);

    IoStatus->Status = FsRtlFastUnlockAllByKey(&fcb->lock, FileObject, ProcessId, Key, NULL);

    fcb->Header.IsFastIoPossible = fast_io_possible(fcb);

    ExReleaseResourceLite(fcb->Header.Resource);

    FsRtlExitFileSystem();

    return true;
}

#ifdef __REACTOS__
_Function_class_(FAST_IO_ACQUIRE_FILE)
#endif /* __REACTOS__ */
static void __stdcall fast_io_acquire_for_create_section(_In_ PFILE_OBJECT FileObject) {
    fcb* fcb;

    TRACE("(%p)\n", FileObject);

    if (!FileObject)
        return;

    fcb = FileObject->FsContext;

    if (!fcb)
        return;

    ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, true);
    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);
}

#ifdef __REACTOS__
_Function_class_(FAST_IO_RELEASE_FILE)
#endif /* __REACTOS__ */
static void __stdcall fast_io_release_for_create_section(_In_ PFILE_OBJECT FileObject) {
    fcb* fcb;

    TRACE("(%p)\n", FileObject);

    if (!FileObject)
        return;

    fcb = FileObject->FsContext;

    if (!fcb)
        return;

    ExReleaseResourceLite(fcb->Header.Resource);
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
}

void init_fast_io_dispatch(FAST_IO_DISPATCH** fiod) {
    RtlZeroMemory(&FastIoDispatch, sizeof(FastIoDispatch));

    FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);

    FastIoDispatch.FastIoCheckIfPossible = fast_io_check_if_possible;
    FastIoDispatch.FastIoRead = FsRtlCopyRead;
    FastIoDispatch.FastIoWrite = fast_io_write;
    FastIoDispatch.FastIoQueryBasicInfo = fast_query_basic_info;
    FastIoDispatch.FastIoQueryStandardInfo = fast_query_standard_info;
    FastIoDispatch.FastIoLock = fast_io_lock;
    FastIoDispatch.FastIoUnlockSingle = fast_io_unlock_single;
    FastIoDispatch.FastIoUnlockAll = fast_io_unlock_all;
    FastIoDispatch.FastIoUnlockAllByKey = fast_io_unlock_all_by_key;
    FastIoDispatch.AcquireFileForNtCreateSection = fast_io_acquire_for_create_section;
    FastIoDispatch.ReleaseFileForNtCreateSection = fast_io_release_for_create_section;
    FastIoDispatch.FastIoQueryNetworkOpenInfo = fast_io_query_network_open_info;
    FastIoDispatch.AcquireForModWrite = fast_io_acquire_for_mod_write;
    FastIoDispatch.MdlRead = FsRtlMdlReadDev;
    FastIoDispatch.MdlReadComplete = FsRtlMdlReadCompleteDev;
    FastIoDispatch.PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
    FastIoDispatch.MdlWriteComplete = FsRtlMdlWriteCompleteDev;
    FastIoDispatch.ReleaseForModWrite = fast_io_release_for_mod_write;
    FastIoDispatch.AcquireForCcFlush = fast_io_acquire_for_ccflush;
    FastIoDispatch.ReleaseForCcFlush = fast_io_release_for_ccflush;

    *fiod = &FastIoDispatch;
}
