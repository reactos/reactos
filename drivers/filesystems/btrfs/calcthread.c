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

#define SECTOR_BLOCK 16

NTSTATUS add_calc_job(device_extension* Vcb, uint8_t* data, uint32_t sectors, uint32_t* csum, calc_job** pcj) {
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
    KeInitializeEvent(&cj->event, NotificationEvent, false);

    ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, true);

    InsertTailList(&Vcb->calcthreads.job_list, &cj->list_entry);

    KeSetEvent(&Vcb->calcthreads.event, 0, false);
    KeClearEvent(&Vcb->calcthreads.event);

    ExReleaseResourceLite(&Vcb->calcthreads.lock);

    *pcj = cj;

    return STATUS_SUCCESS;
}

void free_calc_job(calc_job* cj) {
    LONG rc = InterlockedDecrement(&cj->refcount);

    if (rc == 0)
        ExFreePool(cj);
}

static bool do_calc(device_extension* Vcb, calc_job* cj) {
    LONG pos, done;
    uint32_t* csum;
    uint8_t* data;
    ULONG blocksize, i;

    pos = InterlockedIncrement(&cj->pos) - 1;

    if ((uint32_t)pos * SECTOR_BLOCK >= cj->sectors)
        return false;

    csum = &cj->csum[pos * SECTOR_BLOCK];
    data = cj->data + (pos * SECTOR_BLOCK * Vcb->superblock.sector_size);

    blocksize = min(SECTOR_BLOCK, cj->sectors - (pos * SECTOR_BLOCK));
    for (i = 0; i < blocksize; i++) {
        *csum = ~calc_crc32c(0xffffffff, data, Vcb->superblock.sector_size);
        csum++;
        data += Vcb->superblock.sector_size;
    }

    done = InterlockedIncrement(&cj->done);

    if ((uint32_t)done * SECTOR_BLOCK >= cj->sectors) {
        ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, true);
        RemoveEntryList(&cj->list_entry);
        ExReleaseResourceLite(&Vcb->calcthreads.lock);

        KeSetEvent(&cj->event, 0, false);
    }

    return true;
}

_Function_class_(KSTART_ROUTINE)
void __stdcall calc_thread(void* context) {
    drv_calc_thread* thread = context;
    device_extension* Vcb = thread->DeviceObject->DeviceExtension;

    ObReferenceObject(thread->DeviceObject);

    while (true) {
        KeWaitForSingleObject(&Vcb->calcthreads.event, Executive, KernelMode, false, NULL);

        while (true) {
            calc_job* cj;
            bool b;

            ExAcquireResourceExclusiveLite(&Vcb->calcthreads.lock, true);

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

        if (thread->quit)
            break;
    }

    ObDereferenceObject(thread->DeviceObject);

    KeSetEvent(&thread->finished, 0, false);

    PsTerminateSystemThread(STATUS_SUCCESS);
}
