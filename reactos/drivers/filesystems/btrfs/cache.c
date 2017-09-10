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

CACHE_MANAGER_CALLBACKS* cache_callbacks;

#ifdef __REACTOS__
static BOOLEAN NTAPI acquire_for_lazy_write(PVOID Context, BOOLEAN Wait) {
#else
static BOOLEAN acquire_for_lazy_write(PVOID Context, BOOLEAN Wait) {
#endif
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;

    TRACE("(%p, %u)\n", Context, Wait);

    if (!ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, Wait))
        return FALSE;

    if (!ExAcquireResourceExclusiveLite(fcb->Header.Resource, Wait)) {
        ExReleaseResourceLite(&fcb->Vcb->tree_lock);
        return FALSE;
    }

    fcb->lazy_writer_thread = KeGetCurrentThread();

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

#ifdef __REACTOS__
static void NTAPI release_from_lazy_write(PVOID Context) {
#else
static void release_from_lazy_write(PVOID Context) {
#endif
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;

    TRACE("(%p)\n", Context);

    fcb->lazy_writer_thread = NULL;

    ExReleaseResourceLite(fcb->Header.Resource);

    ExReleaseResourceLite(&fcb->Vcb->tree_lock);

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
        IoSetTopLevelIrp(NULL);
}

#ifdef __REACTOS__
static BOOLEAN NTAPI acquire_for_read_ahead(PVOID Context, BOOLEAN Wait) {
#else
static BOOLEAN acquire_for_read_ahead(PVOID Context, BOOLEAN Wait) {
#endif
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;

    TRACE("(%p, %u)\n", Context, Wait);

    if (!ExAcquireResourceSharedLite(fcb->Header.Resource, Wait))
        return FALSE;

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

#ifdef __REACTOS__
static void NTAPI release_from_read_ahead(PVOID Context) {
#else
static void release_from_read_ahead(PVOID Context) {
#endif
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;

    TRACE("(%p)\n", Context);

    ExReleaseResourceLite(fcb->Header.Resource);

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
        IoSetTopLevelIrp(NULL);
}

NTSTATUS init_cache() {
    cache_callbacks = ExAllocatePoolWithTag(NonPagedPool, sizeof(CACHE_MANAGER_CALLBACKS), ALLOC_TAG);
    if (!cache_callbacks) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cache_callbacks->AcquireForLazyWrite = acquire_for_lazy_write;
    cache_callbacks->ReleaseFromLazyWrite = release_from_lazy_write;
    cache_callbacks->AcquireForReadAhead = acquire_for_read_ahead;
    cache_callbacks->ReleaseFromReadAhead = release_from_read_ahead;

    return STATUS_SUCCESS;
}

void free_cache() {
    ExFreePool(cache_callbacks);
}
