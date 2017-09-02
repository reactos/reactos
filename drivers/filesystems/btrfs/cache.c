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
#include <wdm.h>

CACHE_MANAGER_CALLBACKS* cache_callbacks;

static BOOLEAN STDCALL acquire_for_lazy_write(PVOID Context, BOOLEAN Wait) {
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;
    
    TRACE("(%p, %u)\n", Context, Wait);
    
//     if (!fcb || FileObject->Flags & FO_CLEANUP_COMPLETE)
//         return FALSE;
    
    if (!ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, Wait))
        return FALSE;

    if (!ExAcquireResourceExclusiveLite(fcb->Header.Resource, Wait)) {
        ExReleaseResourceLite(&fcb->Vcb->tree_lock);
        return FALSE;
    }
    
    fcb->lazy_writer_thread = KeGetCurrentThread();
    
    return TRUE;
}

static void STDCALL release_from_lazy_write(PVOID Context) {
    PFILE_OBJECT FileObject = Context;
    fcb* fcb = FileObject->FsContext;
    
    TRACE("(%p)\n", Context);
    
//     if (!fcb || FileObject->Flags & FO_CLEANUP_COMPLETE)
//         return;
    
    fcb->lazy_writer_thread = NULL;
    
    ExReleaseResourceLite(fcb->Header.Resource);
    
    ExReleaseResourceLite(&fcb->Vcb->tree_lock);
}

static BOOLEAN STDCALL acquire_for_read_ahead(PVOID Context, BOOLEAN Wait) {
    TRACE("(%p, %u)\n", Context, Wait);
    
    return TRUE;
}

static void STDCALL release_from_read_ahead(PVOID Context) {
    TRACE("(%p)\n", Context);
}

NTSTATUS STDCALL init_cache() {
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

void STDCALL free_cache() {
    ExFreePool(cache_callbacks);
}
