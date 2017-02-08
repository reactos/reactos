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

#define SECTOR_BLOCK 16

NTSTATUS add_calc_job(device_extension* Vcb, UINT8* data, UINT32 sectors, UINT32* csum, calc_job** pcj) {
    calc_job* cj;
    
    cj = ExAllocatePoolWithTag(NonPagedPool, sizeof(calc_job), ALLOC_TAG);
    if (!cj) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    cj->data = data;
    cj->sectors = sectors;
    cj->csum = csum;
    cj->pos = 0;
    cj->done = 0;
    cj->refcount = 1;
    KeInitializeEvent(&cj->event, NotificationEvent, FALSE);

    ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, TRUE);
    InsertTailList(&Vcb->calcthreads.job_list, &cj->list_entry);
    ExReleaseResourceLite(&Vcb->calcthreads.lock);
    
    KeSetEvent(&Vcb->calcthreads.event, 0, FALSE);
    KeClearEvent(&Vcb->calcthreads.event);
    
    *pcj = cj;
    
    return STATUS_SUCCESS;
}

void free_calc_job(calc_job* cj) {
    LONG rc = InterlockedDecrement(&cj->refcount);
    
    if (rc == 0)
        ExFreePool(cj);
}

static BOOL do_calc(device_extension* Vcb, calc_job* cj) {
    LONG pos, done;
    UINT32* csum;
    UINT8* data;
    ULONG blocksize, i;
    
    pos = InterlockedIncrement(&cj->pos) - 1;
    
    if (pos * SECTOR_BLOCK >= cj->sectors)
        return FALSE;

    csum = &cj->csum[pos * SECTOR_BLOCK];
    data = cj->data + (pos * SECTOR_BLOCK * Vcb->superblock.sector_size);
    
    blocksize = min(SECTOR_BLOCK, cj->sectors - (pos * SECTOR_BLOCK));
    for (i = 0; i < blocksize; i++) {
        *csum = ~calc_crc32c(0xffffffff, data, Vcb->superblock.sector_size);
        csum++;
        data += Vcb->superblock.sector_size;
    }
    
    done = InterlockedIncrement(&cj->done);
    
    if (done * SECTOR_BLOCK >= cj->sectors) {
        ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, TRUE);
        RemoveEntryList(&cj->list_entry);
        ExReleaseResourceLite(&Vcb->calcthreads.lock);
        
        KeSetEvent(&cj->event, 0, FALSE);
    }
    
    return TRUE;
}

#ifdef __REACTOS__
void NTAPI calc_thread(void* context) {
#else
void calc_thread(void* context) {
#endif
    drv_calc_thread* thread = context;
    device_extension* Vcb = thread->DeviceObject->DeviceExtension;
    
    ObReferenceObject(thread->DeviceObject);
    
    while (TRUE) {
        KeWaitForSingleObject(&Vcb->calcthreads.event, Executive, KernelMode, FALSE, NULL);
        
        FsRtlEnterFileSystem();
        
        while (TRUE) {
            calc_job* cj;
            BOOL b;
            
            ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, TRUE);
            
            if (IsListEmpty(&Vcb->calcthreads.job_list)) {
                ExReleaseResourceLite(&Vcb->calcthreads.lock);
                break;
            }
            
            cj = CONTAINING_RECORD(Vcb->calcthreads.job_list.Flink, calc_job, list_entry);
            cj->refcount++;
            
            ExReleaseResourceLite(&Vcb->calcthreads.lock);
            
            b = do_calc(Vcb, cj);
            
            free_calc_job(cj);
            
            if (!b)
                break;
        }
        
        FsRtlExitFileSystem();
        
        if (thread->quit)
            break;
    }

    ObDereferenceObject(thread->DeviceObject);
     
    KeSetEvent(&thread->finished, 0, FALSE);
     
    PsTerminateSystemThread(STATUS_SUCCESS);
}
