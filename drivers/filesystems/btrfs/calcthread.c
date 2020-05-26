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
#include "xxhash.h"
#include "crc32c.h"

void calc_thread_main(device_extension* Vcb, calc_job* cj) {
    while (true) {
        KIRQL irql;
        calc_job* cj2;
        uint8_t* src;
        void* dest;
        bool last_one = false;

        KeAcquireSpinLock(&Vcb->calcthreads.spinlock, &irql);

        if (cj && cj->not_started == 0) {
            KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);
            break;
        }

        if (cj)
            cj2 = cj;
        else {
            if (IsListEmpty(&Vcb->calcthreads.job_list)) {
                KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);
                break;
            }

            cj2 = CONTAINING_RECORD(Vcb->calcthreads.job_list.Flink, calc_job, list_entry);
        }

        src = cj2->in;
        dest = cj2->out;

        switch (cj2->type) {
            case calc_thread_crc32c:
            case calc_thread_xxhash:
            case calc_thread_sha256:
            case calc_thread_blake2:
                cj2->in = (uint8_t*)cj2->in + Vcb->superblock.sector_size;
                cj2->out = (uint8_t*)cj2->out + Vcb->csum_size;
            break;

            default:
                break;
        }

        cj2->not_started--;

        if (cj2->not_started == 0) {
            RemoveEntryList(&cj2->list_entry);
            last_one = true;
        }

        KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);

        switch (cj2->type) {
            case calc_thread_crc32c:
                *(uint32_t*)dest = ~calc_crc32c(0xffffffff, src, Vcb->superblock.sector_size);
            break;

            case calc_thread_xxhash:
                *(uint64_t*)dest = XXH64(src, Vcb->superblock.sector_size, 0);
            break;

            case calc_thread_sha256:
                calc_sha256(dest, src, Vcb->superblock.sector_size);
            break;

            case calc_thread_blake2:
                blake2b(dest, BLAKE2_HASH_SIZE, src, Vcb->superblock.sector_size);
            break;

            case calc_thread_decomp_zlib:
                cj2->Status = zlib_decompress(src, cj2->inlen, dest, cj2->outlen);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("zlib_decompress returned %08lx\n", cj2->Status);
            break;

            case calc_thread_decomp_lzo:
                cj2->Status = lzo_decompress(src, cj2->inlen, dest, cj2->outlen, cj2->off);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("lzo_decompress returned %08lx\n", cj2->Status);
            break;

            case calc_thread_decomp_zstd:
                cj2->Status = zstd_decompress(src, cj2->inlen, dest, cj2->outlen);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("zstd_decompress returned %08lx\n", cj2->Status);
            break;

            case calc_thread_comp_zlib:
                cj2->Status = zlib_compress(src, cj2->inlen, dest, cj2->outlen, Vcb->options.zlib_level, &cj2->space_left);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("zlib_compress returned %08lx\n", cj2->Status);
            break;

            case calc_thread_comp_lzo:
                cj2->Status = lzo_compress(src, cj2->inlen, dest, cj2->outlen, &cj2->space_left);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("lzo_compress returned %08lx\n", cj2->Status);
            break;

            case calc_thread_comp_zstd:
                cj2->Status = zstd_compress(src, cj2->inlen, dest, cj2->outlen, Vcb->options.zstd_level, &cj2->space_left);

                if (!NT_SUCCESS(cj2->Status))
                    ERR("zstd_compress returned %08lx\n", cj2->Status);
            break;
        }

        if (InterlockedDecrement(&cj2->left) == 0)
            KeSetEvent(&cj2->event, 0, false);

        if (last_one)
            break;
    }
}

void do_calc_job(device_extension* Vcb, uint8_t* data, uint32_t sectors, void* csum) {
    KIRQL irql;
    calc_job cj;

    cj.in = data;
    cj.out = csum;
    cj.left = cj.not_started = sectors;

    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C:
            cj.type = calc_thread_crc32c;
        break;

        case CSUM_TYPE_XXHASH:
            cj.type = calc_thread_xxhash;
        break;

        case CSUM_TYPE_SHA256:
            cj.type = calc_thread_sha256;
        break;

        case CSUM_TYPE_BLAKE2:
            cj.type = calc_thread_blake2;
        break;
    }

    KeInitializeEvent(&cj.event, NotificationEvent, false);

    KeAcquireSpinLock(&Vcb->calcthreads.spinlock, &irql);

    InsertTailList(&Vcb->calcthreads.job_list, &cj.list_entry);

    KeSetEvent(&Vcb->calcthreads.event, 0, false);
    KeClearEvent(&Vcb->calcthreads.event);

    KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);

    calc_thread_main(Vcb, &cj);

    KeWaitForSingleObject(&cj.event, Executive, KernelMode, false, NULL);
}

NTSTATUS add_calc_job_decomp(device_extension* Vcb, uint8_t compression, void* in, unsigned int inlen,
                             void* out, unsigned int outlen, unsigned int off, calc_job** pcj) {
    calc_job* cj;
    KIRQL irql;

    cj = ExAllocatePoolWithTag(NonPagedPool, sizeof(calc_job), ALLOC_TAG);
    if (!cj) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cj->in = in;
    cj->inlen = inlen;
    cj->out = out;
    cj->outlen = outlen;
    cj->off = off;
    cj->left = cj->not_started = 1;
    cj->Status = STATUS_SUCCESS;

    switch (compression) {
        case BTRFS_COMPRESSION_ZLIB:
            cj->type = calc_thread_decomp_zlib;
        break;

        case BTRFS_COMPRESSION_LZO:
            cj->type = calc_thread_decomp_lzo;
        break;

        case BTRFS_COMPRESSION_ZSTD:
            cj->type = calc_thread_decomp_zstd;
        break;

        default:
            ERR("unexpected compression type %x\n", compression);
            ExFreePool(cj);
        return STATUS_NOT_SUPPORTED;
    }

    KeInitializeEvent(&cj->event, NotificationEvent, false);

    KeAcquireSpinLock(&Vcb->calcthreads.spinlock, &irql);

    InsertTailList(&Vcb->calcthreads.job_list, &cj->list_entry);

    KeSetEvent(&Vcb->calcthreads.event, 0, false);
    KeClearEvent(&Vcb->calcthreads.event);

    KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);

    *pcj = cj;

    return STATUS_SUCCESS;
}

NTSTATUS add_calc_job_comp(device_extension* Vcb, uint8_t compression, void* in, unsigned int inlen,
                           void* out, unsigned int outlen, calc_job** pcj) {
    calc_job* cj;
    KIRQL irql;

    cj = ExAllocatePoolWithTag(NonPagedPool, sizeof(calc_job), ALLOC_TAG);
    if (!cj) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cj->in = in;
    cj->inlen = inlen;
    cj->out = out;
    cj->outlen = outlen;
    cj->left = cj->not_started = 1;
    cj->Status = STATUS_SUCCESS;

    switch (compression) {
        case BTRFS_COMPRESSION_ZLIB:
            cj->type = calc_thread_comp_zlib;
        break;

        case BTRFS_COMPRESSION_LZO:
            cj->type = calc_thread_comp_lzo;
        break;

        case BTRFS_COMPRESSION_ZSTD:
            cj->type = calc_thread_comp_zstd;
        break;

        default:
            ERR("unexpected compression type %x\n", compression);
            ExFreePool(cj);
        return STATUS_NOT_SUPPORTED;
    }

    KeInitializeEvent(&cj->event, NotificationEvent, false);

    KeAcquireSpinLock(&Vcb->calcthreads.spinlock, &irql);

    InsertTailList(&Vcb->calcthreads.job_list, &cj->list_entry);

    KeSetEvent(&Vcb->calcthreads.event, 0, false);
    KeClearEvent(&Vcb->calcthreads.event);

    KeReleaseSpinLock(&Vcb->calcthreads.spinlock, irql);

    *pcj = cj;

    return STATUS_SUCCESS;
}

_Function_class_(KSTART_ROUTINE)
void __stdcall calc_thread(void* context) {
    drv_calc_thread* thread = context;
    device_extension* Vcb = thread->DeviceObject->DeviceExtension;

    ObReferenceObject(thread->DeviceObject);

    KeSetSystemAffinityThread((KAFFINITY)(1 << thread->number));

    while (true) {
        KeWaitForSingleObject(&Vcb->calcthreads.event, Executive, KernelMode, false, NULL);

        calc_thread_main(Vcb, NULL);

        if (thread->quit)
            break;
    }

    ObDereferenceObject(thread->DeviceObject);

    KeSetEvent(&thread->finished, 0, false);

    PsTerminateSystemThread(STATUS_SUCCESS);
}
