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

enum read_data_status {
    ReadDataStatus_Pending,
    ReadDataStatus_Success,
    ReadDataStatus_Error,
    ReadDataStatus_MissingDevice,
    ReadDataStatus_Skip
};

struct read_data_context;

typedef struct {
    struct read_data_context* context;
    UINT16 stripenum;
    BOOL rewrite;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_data_status status;
    PMDL mdl;
    UINT64 stripestart;
    UINT64 stripeend;
} read_data_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
    UINT64 address;
    UINT32 buflen;
    LONG num_stripes, stripes_left;
    UINT64 type;
    UINT32 sector_size;
    UINT16 firstoff, startoffstripe, sectors_per_stripe;
    UINT32* csum;
    BOOL tree;
    read_data_stripe* stripes;
    UINT8* va;
} read_data_context;

extern BOOL diskacc;
extern tPsUpdateDiskCounters fPsUpdateDiskCounters;
extern tCcCopyReadEx fCcCopyReadEx;
extern tFsRtlUpdateDiskCounters fFsRtlUpdateDiskCounters;

#define LINUX_PAGE_SIZE 4096

_Function_class_(IO_COMPLETION_ROUTINE)
#ifdef __REACTOS__
static NTSTATUS NTAPI read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    read_data_stripe* stripe = conptr;
    read_data_context* context = (read_data_context*)stripe->context;

    UNUSED(DeviceObject);

    stripe->iosb = Irp->IoStatus;

    if (NT_SUCCESS(Irp->IoStatus.Status))
        stripe->status = ReadDataStatus_Success;
    else
        stripe->status = ReadDataStatus_Error;

    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS check_csum(device_extension* Vcb, UINT8* data, UINT32 sectors, UINT32* csum) {
    NTSTATUS Status;
    calc_job* cj;
    UINT32* csum2;

    // From experimenting, it seems that 40 sectors is roughly the crossover
    // point where offloading the crc32 calculation becomes worth it.

    if (sectors < 40 || KeQueryActiveProcessorCount(NULL) < 2) {
        ULONG j;

        for (j = 0; j < sectors; j++) {
            UINT32 crc32 = ~calc_crc32c(0xffffffff, data + (j * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

            if (crc32 != csum[j]) {
                return STATUS_CRC_ERROR;
            }
        }

        return STATUS_SUCCESS;
    }

    csum2 = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sectors, ALLOC_TAG);
    if (!csum2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = add_calc_job(Vcb, data, sectors, csum2, &cj);
    if (!NT_SUCCESS(Status)) {
        ERR("add_calc_job returned %08x\n", Status);
        ExFreePool(csum2);
        return Status;
    }

    KeWaitForSingleObject(&cj->event, Executive, KernelMode, FALSE, NULL);

    if (RtlCompareMemory(csum2, csum, sectors * sizeof(UINT32)) != sectors * sizeof(UINT32)) {
        free_calc_job(cj);
        ExFreePool(csum2);
        return STATUS_CRC_ERROR;
    }

    free_calc_job(cj);
    ExFreePool(csum2);

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_dup(device_extension* Vcb, UINT8* buf, UINT64 addr, read_data_context* context, CHUNK_ITEM* ci,
                              device** devices, UINT64 generation) {
    ULONG i;
    BOOL checksum_error = FALSE;
    UINT16 j, stripe = 0;
    NTSTATUS Status;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08x\n", j, context->stripes[j].iosb.Status);
            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success) {
            stripe = j;
            break;
        }
    }

    if (context->stripes[stripe].status != ReadDataStatus_Success)
        return STATUS_INTERNAL_ERROR;

    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32;

        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));

        if (th->address != context->address || crc32 != *((UINT32*)th->csum)) {
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && th->generation != generation) {
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;

        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, (ULONG)context->stripes[stripe].Irp->IoStatus.Information / context->sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (ci->num_stripes == 1)
        return STATUS_CRC_ERROR;

    if (context->tree) {
        tree_header* t2;
        BOOL recovered = FALSE;

        t2 = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!t2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (j = 0; j < ci->num_stripes; j++) {
            if (j != stripe && devices[j] && devices[j]->devobj) {
                Status = sync_read_phys(devices[j]->devobj, cis[j].offset + context->stripes[stripe].stripestart, Vcb->superblock.node_size, (UINT8*)t2, FALSE);
                if (!NT_SUCCESS(Status)) {
                    WARN("sync_read_phys returned %08x\n", Status);
                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                } else {
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&t2->fs_uuid, Vcb->superblock.node_size - sizeof(t2->csum));

                    if (t2->address == addr && crc32 == *((UINT32*)t2->csum) && (generation == 0 || t2->generation == generation)) {
                        RtlCopyMemory(buf, t2, Vcb->superblock.node_size);
                        ERR("recovering from checksum error at %llx, device %llx\n", addr, devices[stripe]->devitem.dev_id);
                        recovered = TRUE;

                        if (!Vcb->readonly && !devices[stripe]->readonly) { // write good data over bad
                            Status = write_data_phys(devices[stripe]->devobj, cis[stripe].offset + context->stripes[stripe].stripestart,
                                                     t2, Vcb->superblock.node_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08x\n", Status);
                                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }

                        break;
                    } else if (t2->address != addr || crc32 != *((UINT32*)t2->csum))
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                    else
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_GENERATION_ERRORS);
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %llx\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = (ULONG)context->stripes[stripe].Irp->IoStatus.Information / Vcb->superblock.sector_size;
        UINT8* sector;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < sectors; i++) {
            UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

            if (context->csum[i] != crc32) {
                BOOL recovered = FALSE;

                for (j = 0; j < ci->num_stripes; j++) {
                    if (j != stripe && devices[j] && devices[j]->devobj) {
                        Status = sync_read_phys(devices[j]->devobj, cis[j].offset + context->stripes[stripe].stripestart + UInt32x32To64(i, Vcb->superblock.sector_size),
                                                Vcb->superblock.sector_size, sector, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            WARN("sync_read_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                        } else {
                            UINT32 crc32b = ~calc_crc32c(0xffffffff, sector, Vcb->superblock.sector_size);

                            if (crc32b == context->csum[i]) {
                                RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), sector, Vcb->superblock.sector_size);
                                ERR("recovering from checksum error at %llx, device %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size), devices[stripe]->devitem.dev_id);
                                recovered = TRUE;

                                if (!Vcb->readonly && !devices[stripe]->readonly) { // write good data over bad
                                    Status = write_data_phys(devices[stripe]->devobj, cis[stripe].offset + context->stripes[stripe].stripestart + UInt32x32To64(i, Vcb->superblock.sector_size),
                                                             sector, Vcb->superblock.sector_size);
                                    if (!NT_SUCCESS(Status)) {
                                        WARN("write_data_phys returned %08x\n", Status);
                                        log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                                    }
                                }

                                break;
                            } else
                                log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid0(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, read_data_context* context,
                                CHUNK_ITEM* ci, device** devices, UINT64 generation, UINT64 offset) {
    UINT64 i;

    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            WARN("stripe %llu returned error %08x\n", i, context->stripes[i].iosb.Status);
            log_device_error(Vcb, devices[i], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[i].iosb.Status;
        }
    }

    if (context->tree) { // shouldn't happen, as trees shouldn't cross stripe boundaries
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

        if (crc32 != *((UINT32*)th->csum) || addr != th->address || (generation != 0 && generation != th->generation)) {
            UINT64 off;
            UINT16 stripe;

            get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes, &off, &stripe);

            ERR("unrecoverable checksum error at %llx, device %llx\n", addr, devices[stripe]->devitem.dev_id);

            if (crc32 != *((UINT32*)th->csum)) {
                WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                return STATUS_CRC_ERROR;
            } else if (addr != th->address) {
                WARN("address of tree was %llx, not %llx as expected\n", th->address, addr);
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                return STATUS_CRC_ERROR;
            } else if (generation != 0 && generation != th->generation) {
                WARN("generation of tree was %llx, not %llx as expected\n", th->generation, generation);
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
                return STATUS_CRC_ERROR;
            }
        }
    } else if (context->csum) {
        NTSTATUS Status;
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;

        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            for (i = 0; i < length / Vcb->superblock.sector_size; i++) {
                UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                if (context->csum[i] != crc32) {
                    UINT64 off;
                    UINT16 stripe;

                    get_raid0_offset(addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size), ci->stripe_length, ci->num_stripes, &off, &stripe);

                    ERR("unrecoverable checksum error at %llx, device %llx\n", addr, devices[stripe]->devitem.dev_id);

                    log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                    return Status;
                }
            }

            return Status;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid10(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, read_data_context* context,
                                 CHUNK_ITEM* ci, device** devices, UINT64 generation, UINT64 offset) {
    UINT64 i;
    UINT16 j, stripe;
    NTSTATUS Status;
    BOOL checksum_error = FALSE;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %llu returned error %08x\n", j, context->stripes[j].iosb.Status);
            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success)
            stripe = j;
    }

    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

        if (crc32 != *((UINT32*)th->csum)) {
            WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (addr != th->address) {
            WARN("address of tree was %llx, not %llx as expected\n", th->address, addr);
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            WARN("generation of tree was %llx, not %llx as expected\n", th->generation, generation);
            checksum_error = TRUE;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;

        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR)
            checksum_error = TRUE;
        else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        tree_header* t2;
        UINT64 off;
        UINT16 badsubstripe = 0;
        BOOL recovered = FALSE;

        t2 = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!t2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &off, &stripe);

        stripe *= ci->sub_stripes;

        for (j = 0; j < ci->sub_stripes; j++) {
            if (context->stripes[stripe + j].status == ReadDataStatus_Success) {
                badsubstripe = j;
                break;
            }
        }

        for (j = 0; j < ci->sub_stripes; j++) {
            if (context->stripes[stripe + j].status != ReadDataStatus_Success && devices[stripe + j] && devices[stripe + j]->devobj) {
                Status = sync_read_phys(devices[stripe + j]->devobj, cis[stripe + j].offset + off,
                                        Vcb->superblock.node_size, (UINT8*)t2, FALSE);
                if (!NT_SUCCESS(Status)) {
                    WARN("sync_read_phys returned %08x\n", Status);
                    log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_READ_ERRORS);
                } else {
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&t2->fs_uuid, Vcb->superblock.node_size - sizeof(t2->csum));

                    if (t2->address == addr && crc32 == *((UINT32*)t2->csum) && (generation == 0 || t2->generation == generation)) {
                        RtlCopyMemory(buf, t2, Vcb->superblock.node_size);
                        ERR("recovering from checksum error at %llx, device %llx\n", addr, devices[stripe + j]->devitem.dev_id);
                        recovered = TRUE;

                        if (!Vcb->readonly && !devices[stripe + badsubstripe]->readonly && devices[stripe + badsubstripe]->devobj) { // write good data over bad
                            Status = write_data_phys(devices[stripe + badsubstripe]->devobj, cis[stripe + badsubstripe].offset + off,
                                                     t2, Vcb->superblock.node_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08x\n", Status);
                                log_device_error(Vcb, devices[stripe + badsubstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }

                        break;
                    } else if (t2->address != addr || crc32 != *((UINT32*)t2->csum))
                        log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                    else
                        log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_GENERATION_ERRORS);
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %llx\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = length / Vcb->superblock.sector_size;
        UINT8* sector;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < sectors; i++) {
            UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

            if (context->csum[i] != crc32) {
                UINT64 off;
                UINT16 stripe2, badsubstripe = 0;
                BOOL recovered = FALSE;

                get_raid0_offset(addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size), ci->stripe_length,
                                 ci->num_stripes / ci->sub_stripes, &off, &stripe2);

                stripe2 *= ci->sub_stripes;

                for (j = 0; j < ci->sub_stripes; j++) {
                    if (context->stripes[stripe2 + j].status == ReadDataStatus_Success) {
                        badsubstripe = j;
                        break;
                    }
                }

                log_device_error(Vcb, devices[stripe2 + badsubstripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                for (j = 0; j < ci->sub_stripes; j++) {
                    if (context->stripes[stripe2 + j].status != ReadDataStatus_Success && devices[stripe2 + j] && devices[stripe2 + j]->devobj) {
                        Status = sync_read_phys(devices[stripe2 + j]->devobj, cis[stripe2 + j].offset + off,
                                                Vcb->superblock.sector_size, sector, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            WARN("sync_read_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[stripe2 + j], BTRFS_DEV_STAT_READ_ERRORS);
                        } else {
                            UINT32 crc32b = ~calc_crc32c(0xffffffff, sector, Vcb->superblock.sector_size);

                            if (crc32b == context->csum[i]) {
                                RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), sector, Vcb->superblock.sector_size);
                                ERR("recovering from checksum error at %llx, device %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size), devices[stripe2 + j]->devitem.dev_id);
                                recovered = TRUE;

                                if (!Vcb->readonly && !devices[stripe2 + badsubstripe]->readonly && devices[stripe2 + badsubstripe]->devobj) { // write good data over bad
                                    Status = write_data_phys(devices[stripe2 + badsubstripe]->devobj, cis[stripe2 + badsubstripe].offset + off,
                                                             sector, Vcb->superblock.sector_size);
                                    if (!NT_SUCCESS(Status)) {
                                        WARN("write_data_phys returned %08x\n", Status);
                                        log_device_error(Vcb, devices[stripe2 + badsubstripe], BTRFS_DEV_STAT_READ_ERRORS);
                                    }
                                }

                                break;
                            } else
                                log_device_error(Vcb, devices[stripe2 + j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid5(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, UINT64 offset, UINT64 generation, chunk* c, BOOL degraded) {
    ULONG i;
    NTSTATUS Status;
    BOOL checksum_error = FALSE;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    UINT16 j, stripe;
    BOOL no_success = TRUE;

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08x\n", j, context->stripes[j].iosb.Status);
            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success) {
            stripe = j;
            no_success = FALSE;
        }
    }

    if (c) {    // check partial stripes
        LIST_ENTRY* le;
        UINT64 ps_length = (ci->num_stripes - 1) * ci->stripe_length;

        ExAcquireResourceSharedLite(&c->partial_stripes_lock, TRUE);

        le = c->partial_stripes.Flink;
        while (le != &c->partial_stripes) {
            partial_stripe* ps = CONTAINING_RECORD(le, partial_stripe, list_entry);

            if (ps->address + ps_length > addr && ps->address < addr + length) {
                ULONG runlength, index;

                runlength = RtlFindFirstRunClear(&ps->bmp, &index);

                while (runlength != 0) {
                    UINT64 runstart = ps->address + (index * Vcb->superblock.sector_size);
                    UINT64 runend = runstart + (runlength * Vcb->superblock.sector_size);
                    UINT64 start = max(runstart, addr);
                    UINT64 end = min(runend, addr + length);

                    if (end > start)
                        RtlCopyMemory(buf + start - addr, &ps->data[start - ps->address], (ULONG)(end - start));

                    runlength = RtlFindNextForwardRunClear(&ps->bmp, index + runlength, &index);
                }
            } else if (ps->address >= addr + length)
                break;

            le = le->Flink;
        }

        ExReleaseResourceLite(&c->partial_stripes_lock);
    }

    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

        if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
            checksum_error = TRUE;
            if (!no_success && !degraded)
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            checksum_error = TRUE;
            if (!no_success && !degraded)
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;

        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            if (!degraded)
                WARN("checksum error\n");
            checksum_error = TRUE;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }

#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    } else if (degraded)
        checksum_error = TRUE;

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        UINT16 parity;
        UINT64 off;
        BOOL recovered = FALSE, first = TRUE, failed = FALSE;
        UINT8* t2;

        t2 = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size * 2, ALLOC_TAG);
        if (!t2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 1, &off, &stripe);

        parity = (((addr - offset) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

        stripe = (parity + stripe + 1) % ci->num_stripes;

        for (j = 0; j < ci->num_stripes; j++) {
            if (j != stripe) {
                if (devices[j] && devices[j]->devobj) {
                    if (first) {
                        Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.node_size, t2, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                            failed = TRUE;
                            break;
                        }

                        first = FALSE;
                    } else {
                        Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.node_size, t2 + Vcb->superblock.node_size, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                            failed = TRUE;
                            break;
                        }

                        do_xor(t2, t2 + Vcb->superblock.node_size, Vcb->superblock.node_size);
                    }
                } else {
                    failed = TRUE;
                    break;
                }
            }
        }

        if (!failed) {
            tree_header* t3 = (tree_header*)t2;
            UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&t3->fs_uuid, Vcb->superblock.node_size - sizeof(t3->csum));

            if (t3->address == addr && crc32 == *((UINT32*)t3->csum) && (generation == 0 || t3->generation == generation)) {
                RtlCopyMemory(buf, t2, Vcb->superblock.node_size);

                if (!degraded)
                    ERR("recovering from checksum error at %llx, device %llx\n", addr, devices[stripe]->devitem.dev_id);

                recovered = TRUE;

                if (!Vcb->readonly && devices[stripe] && !devices[stripe]->readonly && devices[stripe]->devobj) { // write good data over bad
                    Status = write_data_phys(devices[stripe]->devobj, cis[stripe].offset + off, t2, Vcb->superblock.node_size);
                    if (!NT_SUCCESS(Status)) {
                        WARN("write_data_phys returned %08x\n", Status);
                        log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                    }
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %llx\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = length / Vcb->superblock.sector_size;
        UINT8* sector;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size * 2, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < sectors; i++) {
            UINT16 parity;
            UINT64 off;
            UINT32 crc32;

            if (context->csum)
                crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

            get_raid0_offset(addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size), ci->stripe_length,
                             ci->num_stripes - 1, &off, &stripe);

            parity = (((addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size)) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            stripe = (parity + stripe + 1) % ci->num_stripes;

            if (!devices[stripe] || !devices[stripe]->devobj || (context->csum && context->csum[i] != crc32)) {
                BOOL recovered = FALSE, first = TRUE, failed = FALSE;

                if (devices[stripe] && devices[stripe]->devobj)
                    log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_READ_ERRORS);

                for (j = 0; j < ci->num_stripes; j++) {
                    if (j != stripe) {
                        if (devices[j] && devices[j]->devobj) {
                            if (first) {
                                Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.sector_size, sector, FALSE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("sync_read_phys returned %08x\n", Status);
                                    failed = TRUE;
                                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                    break;
                                }

                                first = FALSE;
                            } else {
                                Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.sector_size, sector + Vcb->superblock.sector_size, FALSE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("sync_read_phys returned %08x\n", Status);
                                    failed = TRUE;
                                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                    break;
                                }

                                do_xor(sector, sector + Vcb->superblock.sector_size, Vcb->superblock.sector_size);
                            }
                        } else {
                            failed = TRUE;
                            break;
                        }
                    }
                }

                if (!failed) {
                    if (context->csum)
                        crc32 = ~calc_crc32c(0xffffffff, sector, Vcb->superblock.sector_size);

                    if (!context->csum || crc32 == context->csum[i]) {
                        RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), sector, Vcb->superblock.sector_size);

                        if (!degraded)
                            ERR("recovering from checksum error at %llx, device %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size), devices[stripe]->devitem.dev_id);

                        recovered = TRUE;

                        if (!Vcb->readonly && devices[stripe] && !devices[stripe]->readonly && devices[stripe]->devobj) { // write good data over bad
                            Status = write_data_phys(devices[stripe]->devobj, cis[stripe].offset + off,
                                                     sector, Vcb->superblock.sector_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08x\n", Status);
                                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

void raid6_recover2(UINT8* sectors, UINT16 num_stripes, ULONG sector_size, UINT16 missing1, UINT16 missing2, UINT8* out) {
    if (missing1 == num_stripes - 2 || missing2 == num_stripes - 2) { // reconstruct from q and data
        UINT16 missing = missing1 == (num_stripes - 2) ? missing2 : missing1;
        UINT16 stripe;

        stripe = num_stripes - 3;

        if (stripe == missing)
            RtlZeroMemory(out, sector_size);
        else
            RtlCopyMemory(out, sectors + (stripe * sector_size), sector_size);

        do {
            stripe--;

            galois_double(out, sector_size);

            if (stripe != missing)
                do_xor(out, sectors + (stripe * sector_size), sector_size);
        } while (stripe > 0);

        do_xor(out, sectors + ((num_stripes - 1) * sector_size), sector_size);

        if (missing != 0)
            galois_divpower(out, (UINT8)missing, sector_size);
    } else { // reconstruct from p and q
        UINT16 x, y, stripe;
        UINT8 gyx, gx, denom, a, b, *p, *q, *pxy, *qxy;
        UINT32 j;

        stripe = num_stripes - 3;

        pxy = out + sector_size;
        qxy = out;

        if (stripe == missing1 || stripe == missing2) {
            RtlZeroMemory(qxy, sector_size);
            RtlZeroMemory(pxy, sector_size);

            if (stripe == missing1)
                x = stripe;
            else
                y = stripe;
        } else {
            RtlCopyMemory(qxy, sectors + (stripe * sector_size), sector_size);
            RtlCopyMemory(pxy, sectors + (stripe * sector_size), sector_size);
        }

        do {
            stripe--;

            galois_double(qxy, sector_size);

            if (stripe != missing1 && stripe != missing2) {
                do_xor(qxy, sectors + (stripe * sector_size), sector_size);
                do_xor(pxy, sectors + (stripe * sector_size), sector_size);
            } else if (stripe == missing1)
                x = stripe;
            else if (stripe == missing2)
                y = stripe;
        } while (stripe > 0);

        gyx = gpow2(y > x ? (y-x) : (255-x+y));
        gx = gpow2(255-x);

        denom = gdiv(1, gyx ^ 1);
        a = gmul(gyx, denom);
        b = gmul(gx, denom);

        p = sectors + ((num_stripes - 2) * sector_size);
        q = sectors + ((num_stripes - 1) * sector_size);

        for (j = 0; j < sector_size; j++) {
            *qxy = gmul(a, *p ^ *pxy) ^ gmul(b, *q ^ *qxy);

            p++;
            q++;
            pxy++;
            qxy++;
        }

        do_xor(out + sector_size, out, sector_size);
        do_xor(out + sector_size, sectors + ((num_stripes - 2) * sector_size), sector_size);
    }
}

static NTSTATUS read_data_raid6(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, UINT64 offset, UINT64 generation, chunk* c, BOOL degraded) {
    NTSTATUS Status;
    ULONG i;
    BOOL checksum_error = FALSE;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    UINT16 stripe, j;
    BOOL no_success = TRUE;

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08x\n", j, context->stripes[j].iosb.Status);

            if (devices[j])
                log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success) {
            stripe = j;
            no_success = FALSE;
        }
    }

    if (c) {    // check partial stripes
        LIST_ENTRY* le;
        UINT64 ps_length = (ci->num_stripes - 2) * ci->stripe_length;

        ExAcquireResourceSharedLite(&c->partial_stripes_lock, TRUE);

        le = c->partial_stripes.Flink;
        while (le != &c->partial_stripes) {
            partial_stripe* ps = CONTAINING_RECORD(le, partial_stripe, list_entry);

            if (ps->address + ps_length > addr && ps->address < addr + length) {
                ULONG runlength, index;

                runlength = RtlFindFirstRunClear(&ps->bmp, &index);

                while (runlength != 0) {
                    UINT64 runstart = ps->address + (index * Vcb->superblock.sector_size);
                    UINT64 runend = runstart + (runlength * Vcb->superblock.sector_size);
                    UINT64 start = max(runstart, addr);
                    UINT64 end = min(runend, addr + length);

                    if (end > start)
                        RtlCopyMemory(buf + start - addr, &ps->data[start - ps->address], (ULONG)(end - start));

                    runlength = RtlFindNextForwardRunClear(&ps->bmp, index + runlength, &index);
                }
            } else if (ps->address >= addr + length)
                break;

            le = le->Flink;
        }

        ExReleaseResourceLite(&c->partial_stripes_lock);
    }

    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

        if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
            checksum_error = TRUE;
            if (!no_success && !degraded && devices[stripe])
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            checksum_error = TRUE;
            if (!no_success && !degraded && devices[stripe])
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;

        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            if (!degraded)
                WARN("checksum error\n");
            checksum_error = TRUE;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    } else if (degraded)
        checksum_error = TRUE;

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        UINT8* sector;
        UINT16 k, physstripe, parity1, parity2, error_stripe;
        UINT64 off;
        BOOL recovered = FALSE, failed = FALSE;
        ULONG num_errors = 0;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size * (ci->num_stripes + 2), ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 2, &off, &stripe);

        parity1 = (((addr - offset) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
        parity2 = (parity1 + 1) % ci->num_stripes;

        physstripe = (parity2 + stripe + 1) % ci->num_stripes;

        j = (parity2 + 1) % ci->num_stripes;

        for (k = 0; k < ci->num_stripes - 1; k++) {
            if (j != physstripe) {
                if (devices[j] && devices[j]->devobj) {
                    Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.node_size, sector + (k * Vcb->superblock.node_size), FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("sync_read_phys returned %08x\n", Status);
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                        num_errors++;
                        error_stripe = k;

                        if (num_errors > 1) {
                            failed = TRUE;
                            break;
                        }
                    }
                } else {
                    num_errors++;
                    error_stripe = k;

                    if (num_errors > 1) {
                        failed = TRUE;
                        break;
                    }
                }
            }

            j = (j + 1) % ci->num_stripes;
        }

        if (!failed) {
            if (num_errors == 0) {
                tree_header* th = (tree_header*)(sector + (stripe * Vcb->superblock.node_size));
                UINT32 crc32;

                RtlCopyMemory(sector + (stripe * Vcb->superblock.node_size), sector + ((ci->num_stripes - 2) * Vcb->superblock.node_size),
                              Vcb->superblock.node_size);

                for (j = 0; j < ci->num_stripes - 2; j++) {
                    if (j != stripe)
                        do_xor(sector + (stripe * Vcb->superblock.node_size), sector + (j * Vcb->superblock.node_size), Vcb->superblock.node_size);
                }

                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

                if (th->address == addr && crc32 == *((UINT32*)th->csum) && (generation == 0 || th->generation == generation)) {
                    RtlCopyMemory(buf, sector + (stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);

                    if (devices[physstripe] && devices[physstripe]->devobj)
                        ERR("recovering from checksum error at %llx, device %llx\n", addr, devices[physstripe]->devitem.dev_id);

                    recovered = TRUE;

                    if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[physstripe]->devobj, cis[physstripe].offset + off,
                                                 sector + (stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }
                }
            }

            if (!recovered) {
                UINT32 crc32;
                tree_header* th = (tree_header*)(sector + (ci->num_stripes * Vcb->superblock.node_size));
                BOOL read_q = FALSE;

                if (devices[parity2] && devices[parity2]->devobj) {
                    Status = sync_read_phys(devices[parity2]->devobj, cis[parity2].offset + off,
                                            Vcb->superblock.node_size, sector + ((ci->num_stripes - 1) * Vcb->superblock.node_size), FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("sync_read_phys returned %08x\n", Status);
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                    } else
                        read_q = TRUE;
                }

                if (read_q) {
                    if (num_errors == 1) {
                        raid6_recover2(sector, ci->num_stripes, Vcb->superblock.node_size, stripe, error_stripe, sector + (ci->num_stripes * Vcb->superblock.node_size));

                        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

                        if (th->address == addr && crc32 == *((UINT32*)th->csum) && (generation == 0 || th->generation == generation))
                            recovered = TRUE;
                    } else {
                        for (j = 0; j < ci->num_stripes - 1; j++) {
                            if (j != stripe) {
                                raid6_recover2(sector, ci->num_stripes, Vcb->superblock.node_size, stripe, j, sector + (ci->num_stripes * Vcb->superblock.node_size));

                                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

                                if (th->address == addr && crc32 == *((UINT32*)th->csum) && (generation == 0 || th->generation == generation)) {
                                    recovered = TRUE;
                                    error_stripe = j;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (recovered) {
                    UINT16 error_stripe_phys = (parity2 + error_stripe + 1) % ci->num_stripes;

                    if (devices[physstripe] && devices[physstripe]->devobj)
                        ERR("recovering from checksum error at %llx, device %llx\n", addr, devices[physstripe]->devitem.dev_id);

                    RtlCopyMemory(buf, sector + (ci->num_stripes * Vcb->superblock.node_size), Vcb->superblock.node_size);

                    if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[physstripe]->devobj, cis[physstripe].offset + off,
                                                 sector + (ci->num_stripes * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }

                    if (devices[error_stripe_phys] && devices[error_stripe_phys]->devobj) {
                        if (error_stripe == ci->num_stripes - 2) {
                            ERR("recovering from parity error at %llx, device %llx\n", addr, devices[error_stripe_phys]->devitem.dev_id);

                            log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                            RtlZeroMemory(sector + ((ci->num_stripes - 2) * Vcb->superblock.node_size), Vcb->superblock.node_size);

                            for (j = 0; j < ci->num_stripes - 2; j++) {
                                if (j == stripe) {
                                    do_xor(sector + ((ci->num_stripes - 2) * Vcb->superblock.node_size), sector + (ci->num_stripes * Vcb->superblock.node_size),
                                           Vcb->superblock.node_size);
                                } else {
                                    do_xor(sector + ((ci->num_stripes - 2) * Vcb->superblock.node_size), sector + (j * Vcb->superblock.node_size),
                                            Vcb->superblock.node_size);
                                }
                            }
                        } else {
                            ERR("recovering from checksum error at %llx, device %llx\n", addr + ((error_stripe - stripe) * ci->stripe_length),
                                devices[error_stripe_phys]->devitem.dev_id);

                            log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                            RtlCopyMemory(sector + (error_stripe * Vcb->superblock.node_size),
                                          sector + ((ci->num_stripes + 1) * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        }
                    }

                    if (!Vcb->readonly && devices[error_stripe_phys] && devices[error_stripe_phys]->devobj && !devices[error_stripe_phys]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[error_stripe_phys]->devobj, cis[error_stripe_phys].offset + off,
                                                 sector + (error_stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08x\n", Status);
                            log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %llx\n", addr);
            ExFreePool(sector);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(sector);
    } else {
        ULONG sectors = length / Vcb->superblock.sector_size;
        UINT8* sector;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size * (ci->num_stripes + 2), ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (i = 0; i < sectors; i++) {
            UINT64 off;
            UINT16 physstripe, parity1, parity2;
            UINT32 crc32;

            if (context->csum)
                crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

            get_raid0_offset(addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size), ci->stripe_length,
                             ci->num_stripes - 2, &off, &stripe);

            parity1 = (((addr - offset + UInt32x32To64(i, Vcb->superblock.sector_size)) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
            parity2 = (parity1 + 1) % ci->num_stripes;

            physstripe = (parity2 + stripe + 1) % ci->num_stripes;

            if (!devices[physstripe] || !devices[physstripe]->devobj || (context->csum && context->csum[i] != crc32)) {
                UINT16 k, error_stripe;
                BOOL recovered = FALSE, failed = FALSE;
                ULONG num_errors = 0;

                if (devices[physstripe] && devices[physstripe]->devobj)
                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_READ_ERRORS);

                j = (parity2 + 1) % ci->num_stripes;

                for (k = 0; k < ci->num_stripes - 1; k++) {
                    if (j != physstripe) {
                        if (devices[j] && devices[j]->devobj) {
                            Status = sync_read_phys(devices[j]->devobj, cis[j].offset + off, Vcb->superblock.sector_size, sector + (k * Vcb->superblock.sector_size), FALSE);
                            if (!NT_SUCCESS(Status)) {
                                ERR("sync_read_phys returned %08x\n", Status);
                                log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                num_errors++;
                                error_stripe = k;

                                if (num_errors > 1) {
                                    failed = TRUE;
                                    break;
                                }
                            }
                        } else {
                            num_errors++;
                            error_stripe = k;

                            if (num_errors > 1) {
                                failed = TRUE;
                                break;
                            }
                        }
                    }

                    j = (j + 1) % ci->num_stripes;
                }

                if (!failed) {
                    if (num_errors == 0) {
                        RtlCopyMemory(sector + (stripe * Vcb->superblock.sector_size), sector + ((ci->num_stripes - 2) * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                        for (j = 0; j < ci->num_stripes - 2; j++) {
                            if (j != stripe)
                                do_xor(sector + (stripe * Vcb->superblock.sector_size), sector + (j * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                        }

                        if (context->csum)
                            crc32 = ~calc_crc32c(0xffffffff, sector + (stripe * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                        if (!context->csum || crc32 == context->csum[i]) {
                            RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), sector + (stripe * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                            if (devices[physstripe] && devices[physstripe]->devobj)
                                ERR("recovering from checksum error at %llx, device %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size),
                                    devices[physstripe]->devitem.dev_id);

                            recovered = TRUE;

                            if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[physstripe]->devobj, cis[physstripe].offset + off,
                                                         sector + (stripe * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08x\n", Status);
                                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }
                        }
                    }

                    if (!recovered) {
                        BOOL read_q = FALSE;

                        if (devices[parity2] && devices[parity2]->devobj) {
                            Status = sync_read_phys(devices[parity2]->devobj, cis[parity2].offset + off,
                                                    Vcb->superblock.sector_size, sector + ((ci->num_stripes - 1) * Vcb->superblock.sector_size), FALSE);
                            if (!NT_SUCCESS(Status)) {
                                ERR("sync_read_phys returned %08x\n", Status);
                                log_device_error(Vcb, devices[parity2], BTRFS_DEV_STAT_READ_ERRORS);
                            } else
                                read_q = TRUE;
                        }

                        if (read_q) {
                            if (num_errors == 1) {
                                raid6_recover2(sector, ci->num_stripes, Vcb->superblock.sector_size, stripe, error_stripe, sector + (ci->num_stripes * Vcb->superblock.sector_size));

                                if (!devices[physstripe] || !devices[physstripe]->devobj)
                                    recovered = TRUE;
                                else {
                                    crc32 = ~calc_crc32c(0xffffffff, sector + (ci->num_stripes * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                                    if (crc32 == context->csum[i])
                                        recovered = TRUE;
                                }
                            } else {
                                for (j = 0; j < ci->num_stripes - 1; j++) {
                                    if (j != stripe) {
                                        raid6_recover2(sector, ci->num_stripes, Vcb->superblock.sector_size, stripe, j, sector + (ci->num_stripes * Vcb->superblock.sector_size));

                                        crc32 = ~calc_crc32c(0xffffffff, sector + (ci->num_stripes * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                                        if (crc32 == context->csum[i]) {
                                            recovered = TRUE;
                                            error_stripe = j;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (recovered) {
                            UINT16 error_stripe_phys = (parity2 + error_stripe + 1) % ci->num_stripes;

                            if (devices[physstripe] && devices[physstripe]->devobj)
                                ERR("recovering from checksum error at %llx, device %llx\n",
                                    addr + UInt32x32To64(i, Vcb->superblock.sector_size), devices[physstripe]->devitem.dev_id);

                            RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), sector + (ci->num_stripes * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                            if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[physstripe]->devobj, cis[physstripe].offset + off,
                                                         sector + (ci->num_stripes * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08x\n", Status);
                                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }

                            if (devices[error_stripe_phys] && devices[error_stripe_phys]->devobj) {
                                if (error_stripe == ci->num_stripes - 2) {
                                    ERR("recovering from parity error at %llx, device %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size),
                                        devices[error_stripe_phys]->devitem.dev_id);

                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                                    RtlZeroMemory(sector + ((ci->num_stripes - 2) * Vcb->superblock.sector_size), Vcb->superblock.sector_size);

                                    for (j = 0; j < ci->num_stripes - 2; j++) {
                                        if (j == stripe) {
                                            do_xor(sector + ((ci->num_stripes - 2) * Vcb->superblock.sector_size), sector + (ci->num_stripes * Vcb->superblock.sector_size),
                                                   Vcb->superblock.sector_size);
                                        } else {
                                            do_xor(sector + ((ci->num_stripes - 2) * Vcb->superblock.sector_size), sector + (j * Vcb->superblock.sector_size),
                                                   Vcb->superblock.sector_size);
                                        }
                                    }
                                } else {
                                    ERR("recovering from checksum error at %llx, device %llx\n",
                                        addr + UInt32x32To64(i, Vcb->superblock.sector_size) + ((error_stripe - stripe) * ci->stripe_length),
                                        devices[error_stripe_phys]->devitem.dev_id);

                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                                    RtlCopyMemory(sector + (error_stripe * Vcb->superblock.sector_size),
                                                  sector + ((ci->num_stripes + 1) * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                                }
                            }

                            if (!Vcb->readonly && devices[error_stripe_phys] && devices[error_stripe_phys]->devobj && !devices[error_stripe_phys]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[error_stripe_phys]->devobj, cis[error_stripe_phys].offset + off,
                                                         sector + (error_stripe * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08x\n", Status);
                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %llx\n", addr + UInt32x32To64(i, Vcb->superblock.sector_size));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

NTSTATUS read_data(_In_ device_extension* Vcb, _In_ UINT64 addr, _In_ UINT32 length, _In_reads_bytes_opt_(length*sizeof(UINT32)/Vcb->superblock.sector_size) UINT32* csum,
                   _In_ BOOL is_tree, _Out_writes_bytes_(length) UINT8* buf, _In_opt_ chunk* c, _Out_opt_ chunk** pc, _In_opt_ PIRP Irp, _In_ UINT64 generation, _In_ BOOL file_read,
                   _In_ ULONG priority) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_data_context context;
    UINT64 type, offset, total_reading = 0;
    NTSTATUS Status;
    device** devices = NULL;
    UINT16 i, startoffstripe, allowed_missing, missing_devices = 0;
    UINT8* dummypage = NULL;
    PMDL dummy_mdl = NULL;
    BOOL need_to_wait;
    UINT64 lockaddr, locklen;
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
#endif

    if (Vcb->log_to_phys_loaded) {
        if (!c) {
            c = get_chunk_from_address(Vcb, addr);

            if (!c) {
                ERR("get_chunk_from_address failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        }

        ci = c->chunk_item;
        offset = c->offset;
        devices = c->devices;

        if (pc)
            *pc = c;
    } else {
        LIST_ENTRY* le = Vcb->sys_chunks.Flink;

        ci = NULL;

        c = NULL;
        while (le != &Vcb->sys_chunks) {
            sys_chunk* sc = CONTAINING_RECORD(le, sys_chunk, list_entry);

            if (sc->key.obj_id == 0x100 && sc->key.obj_type == TYPE_CHUNK_ITEM && sc->key.offset <= addr) {
                CHUNK_ITEM* chunk_item = sc->data;

                if ((addr - sc->key.offset) < chunk_item->size && chunk_item->num_stripes > 0) {
                    ci = chunk_item;
                    offset = sc->key.offset;
                    cis = (CHUNK_ITEM_STRIPE*)&chunk_item[1];

                    devices = ExAllocatePoolWithTag(PagedPool, sizeof(device*) * ci->num_stripes, ALLOC_TAG);
                    if (!devices) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    for (i = 0; i < ci->num_stripes; i++) {
                        devices[i] = find_device_from_uuid(Vcb, &cis[i].dev_uuid);
                    }

                    break;
                }
            }

            le = le->Flink;
        }

        if (!ci) {
            ERR("could not find chunk for %llx in bootstrap\n", addr);
            return STATUS_INTERNAL_ERROR;
        }

        if (pc)
            *pc = NULL;
    }

    if (ci->type & BLOCK_FLAG_DUPLICATE) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = ci->num_stripes - 1;
    } else if (ci->type & BLOCK_FLAG_RAID0) {
        type = BLOCK_FLAG_RAID0;
        allowed_missing = 0;
    } else if (ci->type & BLOCK_FLAG_RAID1) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID10) {
        type = BLOCK_FLAG_RAID10;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID5) {
        type = BLOCK_FLAG_RAID5;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID6) {
        type = BLOCK_FLAG_RAID6;
        allowed_missing = 2;
    } else { // SINGLE
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 0;
    }

    cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    RtlZeroMemory(&context, sizeof(read_data_context));
    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);

    context.stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe) * ci->num_stripes, ALLOC_TAG);
    if (!context.stripes) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (c && (type == BLOCK_FLAG_RAID5 || type == BLOCK_FLAG_RAID6)) {
        get_raid56_lock_range(c, addr, length, &lockaddr, &locklen);
        chunk_lock_range(Vcb, c, lockaddr, locklen);
    }

    RtlZeroMemory(context.stripes, sizeof(read_data_stripe) * ci->num_stripes);

    context.buflen = length;
    context.num_stripes = ci->num_stripes;
    context.stripes_left = context.num_stripes;
    context.sector_size = Vcb->superblock.sector_size;
    context.csum = csum;
    context.tree = is_tree;
    context.type = type;

    if (type == BLOCK_FLAG_RAID0) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe, stripe;
        UINT32 *stripeoff, pos;
        PMDL master_mdl;
        PFN_NUMBER* pfns;

        // FIXME - test this still works if page size isn't the same as sector size

        // This relies on the fact that MDLs are followed in memory by the page file numbers,
        // so with a bit of jiggery-pokery you can trick your disks into deinterlacing your RAID0
        // data for you without doing a memcpy yourself.
        // MDLs are officially opaque, so this might very well break in future versions of Windows.

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes, &endoff, &endoffstripe);

        if (file_read) {
            // Unfortunately we can't avoid doing at least one memcpy, as Windows can give us an MDL
            // with duplicated dummy PFNs, which confuse check_csum. Ah well.
            // See https://msdn.microsoft.com/en-us/library/windows/hardware/Dn614012.aspx if you're interested.

            context.va = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);

            if (!context.va) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        } else
            context.va = buf;

        master_mdl = IoAllocateMdl(context.va, length, FALSE, FALSE, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoWriteAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08x\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        for (i = 0; i < ci->num_stripes; i++) {
            if (startoffstripe > i)
                context.stripes[i].stripestart = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            else if (startoffstripe == i)
                context.stripes[i].stripestart = startoff;
            else
                context.stripes[i].stripestart = startoff - (startoff % ci->stripe_length);

            if (endoffstripe > i)
                context.stripes[i].stripeend = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            else if (endoffstripe == i)
                context.stripes[i].stripeend = endoff + 1;
            else
                context.stripes[i].stripeend = endoff - (endoff % ci->stripe_length);

            if (context.stripes[i].stripestart != context.stripes[i].stripeend) {
                context.stripes[i].mdl = IoAllocateMdl(context.va, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart), FALSE, FALSE, NULL);

                if (!context.stripes[i].mdl) {
                    ERR("IoAllocateMdl failed\n");
                    MmUnlockPages(master_mdl);
                    IoFreeMdl(master_mdl);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
        }

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes);

        pos = 0;
        stripe = startoffstripe;
        while (pos < length) {
            PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

            if (pos == 0) {
                UINT32 readlen = (UINT32)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length));

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] += readlen;
                pos += readlen;
            } else if (length - pos < ci->stripe_length) {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (length - pos) * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                pos = length;
            } else {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                stripeoff[stripe] += (UINT32)ci->stripe_length;
                pos += (UINT32)ci->stripe_length;
            }

            stripe = (stripe + 1) % ci->num_stripes;
        }

        MmUnlockPages(master_mdl);
        IoFreeMdl(master_mdl);

        ExFreePool(stripeoff);
    } else if (type == BLOCK_FLAG_RAID10) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe, j, stripe;
        ULONG orig_ls;
        PMDL master_mdl;
        PFN_NUMBER* pfns;
        UINT32* stripeoff, pos;
        read_data_stripe** stripes;

        if (c)
            orig_ls = c->last_stripe;
        else
            orig_ls = 0;

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &endoff, &endoffstripe);

        if ((ci->num_stripes % ci->sub_stripes) != 0) {
            ERR("chunk %llx: num_stripes %x was not a multiple of sub_stripes %x!\n", offset, ci->num_stripes, ci->sub_stripes);
            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }

        if (file_read) {
            context.va = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);

            if (!context.va) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        } else
            context.va = buf;

        context.firstoff = (UINT16)((startoff % ci->stripe_length) / Vcb->superblock.sector_size);
        context.startoffstripe = startoffstripe;
        context.sectors_per_stripe = (UINT16)(ci->stripe_length / Vcb->superblock.sector_size);

        startoffstripe *= ci->sub_stripes;
        endoffstripe *= ci->sub_stripes;

        if (c)
            c->last_stripe = (orig_ls + 1) % ci->sub_stripes;

        master_mdl = IoAllocateMdl(context.va, length, FALSE, FALSE, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoWriteAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08x\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe*) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
        if (!stripes) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripes, sizeof(read_data_stripe*) * ci->num_stripes / ci->sub_stripes);

        for (i = 0; i < ci->num_stripes; i += ci->sub_stripes) {
            UINT64 sstart, send;
            BOOL stripeset = FALSE;

            if (startoffstripe > i)
                sstart = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            else if (startoffstripe == i)
                sstart = startoff;
            else
                sstart = startoff - (startoff % ci->stripe_length);

            if (endoffstripe > i)
                send = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            else if (endoffstripe == i)
                send = endoff + 1;
            else
                send = endoff - (endoff % ci->stripe_length);

            for (j = 0; j < ci->sub_stripes; j++) {
                if (j == orig_ls && devices[i+j] && devices[i+j]->devobj) {
                    context.stripes[i+j].stripestart = sstart;
                    context.stripes[i+j].stripeend = send;
                    stripes[i / ci->sub_stripes] = &context.stripes[i+j];

                    if (sstart != send) {
                        context.stripes[i+j].mdl = IoAllocateMdl(context.va, (ULONG)(send - sstart), FALSE, FALSE, NULL);

                        if (!context.stripes[i+j].mdl) {
                            ERR("IoAllocateMdl failed\n");
                            MmUnlockPages(master_mdl);
                            IoFreeMdl(master_mdl);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                    }

                    stripeset = TRUE;
                } else
                    context.stripes[i+j].status = ReadDataStatus_Skip;
            }

            if (!stripeset) {
                for (j = 0; j < ci->sub_stripes; j++) {
                    if (devices[i+j] && devices[i+j]->devobj) {
                        context.stripes[i+j].stripestart = sstart;
                        context.stripes[i+j].stripeend = send;
                        context.stripes[i+j].status = ReadDataStatus_Pending;
                        stripes[i / ci->sub_stripes] = &context.stripes[i+j];

                        if (sstart != send) {
                            context.stripes[i+j].mdl = IoAllocateMdl(context.va, (ULONG)(send - sstart), FALSE, FALSE, NULL);

                            if (!context.stripes[i+j].mdl) {
                                ERR("IoAllocateMdl failed\n");
                                MmUnlockPages(master_mdl);
                                IoFreeMdl(master_mdl);
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                goto exit;
                            }
                        }

                        stripeset = TRUE;
                        break;
                    }
                }

                if (!stripeset) {
                    ERR("could not find stripe to read\n");
                    Status = STATUS_DEVICE_NOT_READY;
                    goto exit;
                }
            }
        }

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes);

        pos = 0;
        stripe = startoffstripe / ci->sub_stripes;
        while (pos < length) {
            PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(stripes[stripe]->mdl + 1);

            if (pos == 0) {
                UINT32 readlen = (UINT32)min(stripes[stripe]->stripeend - stripes[stripe]->stripestart,
                                             ci->stripe_length - (stripes[stripe]->stripestart % ci->stripe_length));

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] += readlen;
                pos += readlen;
            } else if (length - pos < ci->stripe_length) {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (length - pos) * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                pos = length;
            } else {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                stripeoff[stripe] += (ULONG)ci->stripe_length;
                pos += (ULONG)ci->stripe_length;
            }

            stripe = (stripe + 1) % (ci->num_stripes / ci->sub_stripes);
        }

        MmUnlockPages(master_mdl);
        IoFreeMdl(master_mdl);

        ExFreePool(stripeoff);
        ExFreePool(stripes);
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        UINT64 orig_ls;

        if (c)
            orig_ls = i = c->last_stripe;
        else
            orig_ls = i = 0;

        while (!devices[i] || !devices[i]->devobj) {
            i = (i + 1) % ci->num_stripes;

            if (i == orig_ls) {
                ERR("no devices available to service request\n");
                Status = STATUS_DEVICE_NOT_READY;
                goto exit;
            }
        }

        if (c)
            c->last_stripe = (i + 1) % ci->num_stripes;

        context.stripes[i].stripestart = addr - offset;
        context.stripes[i].stripeend = context.stripes[i].stripestart + length;

        if (file_read) {
            context.va = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);

            if (!context.va) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            context.stripes[i].mdl = IoAllocateMdl(context.va, length, FALSE, FALSE, NULL);
            if (!context.stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            MmBuildMdlForNonPagedPool(context.stripes[i].mdl);
        } else {
            context.stripes[i].mdl = IoAllocateMdl(buf, length, FALSE, FALSE, NULL);

            if (!context.stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            Status = STATUS_SUCCESS;

            _SEH2_TRY {
                MmProbeAndLockPages(context.stripes[i].mdl, KernelMode, IoWriteAccess);
            } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                Status = _SEH2_GetExceptionCode();
            } _SEH2_END;

            if (!NT_SUCCESS(Status)) {
                ERR("MmProbeAndLockPages threw exception %08x\n", Status);
                goto exit;
            }
        }
    } else if (type == BLOCK_FLAG_RAID5) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe, parity;
        UINT32 *stripeoff, pos;
        PMDL master_mdl;
        PFN_NUMBER *pfns, dummy;
        BOOL need_dummy = FALSE;

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 1, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes - 1, &endoff, &endoffstripe);

        if (file_read) {
            context.va = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);

            if (!context.va) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        } else
            context.va = buf;

        master_mdl = IoAllocateMdl(context.va, length, FALSE, FALSE, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoWriteAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08x\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        pos = 0;
        while (pos < length) {
            parity = (((addr - offset + pos) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            if (pos == 0) {
                UINT16 stripe = (parity + startoffstripe + 1) % ci->num_stripes;
                ULONG skip, readlen;

                i = startoffstripe;
                while (stripe != parity) {
                    if (i == startoffstripe) {
                        readlen = min(length, (ULONG)(ci->stripe_length - (startoff % ci->stripe_length)));

                        context.stripes[stripe].stripestart = startoff;
                        context.stripes[stripe].stripeend = startoff + readlen;

                        pos += readlen;

                        if (pos == length)
                            break;
                    } else {
                        readlen = min(length - pos, (ULONG)ci->stripe_length);

                        context.stripes[stripe].stripestart = startoff - (startoff % ci->stripe_length);
                        context.stripes[stripe].stripeend = context.stripes[stripe].stripestart + readlen;

                        pos += readlen;

                        if (pos == length)
                            break;
                    }

                    i++;
                    stripe = (stripe + 1) % ci->num_stripes;
                }

                if (pos == length)
                    break;

                for (i = 0; i < startoffstripe; i++) {
                    UINT16 stripe2 = (parity + i + 1) % ci->num_stripes;

                    context.stripes[stripe2].stripestart = context.stripes[stripe2].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
                }

                context.stripes[parity].stripestart = context.stripes[parity].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;

                if (length - pos > ci->num_stripes * (ci->num_stripes - 1) * ci->stripe_length) {
                    skip = (ULONG)(((length - pos) / (ci->num_stripes * (ci->num_stripes - 1) * ci->stripe_length)) - 1);

                    for (i = 0; i < ci->num_stripes; i++) {
                        context.stripes[i].stripeend += skip * ci->num_stripes * ci->stripe_length;
                    }

                    pos += (UINT32)(skip * (ci->num_stripes - 1) * ci->num_stripes * ci->stripe_length);
                    need_dummy = TRUE;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 1)) {
                for (i = 0; i < ci->num_stripes; i++) {
                    context.stripes[i].stripeend += ci->stripe_length;
                }

                pos += (UINT32)(ci->stripe_length * (ci->num_stripes - 1));
                need_dummy = TRUE;
            } else {
                UINT16 stripe = (parity + 1) % ci->num_stripes;

                i = 0;
                while (stripe != parity) {
                    if (endoffstripe == i) {
                        context.stripes[stripe].stripeend = endoff + 1;
                        break;
                    } else if (endoffstripe > i)
                        context.stripes[stripe].stripeend = endoff - (endoff % ci->stripe_length) + ci->stripe_length;

                    i++;
                    stripe = (stripe + 1) % ci->num_stripes;
                }

                break;
            }
        }

        for (i = 0; i < ci->num_stripes; i++) {
            if (context.stripes[i].stripestart != context.stripes[i].stripeend) {
                context.stripes[i].mdl = IoAllocateMdl(context.va, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart),
                                                       FALSE, FALSE, NULL);

                if (!context.stripes[i].mdl) {
                    ERR("IoAllocateMdl failed\n");
                    MmUnlockPages(master_mdl);
                    IoFreeMdl(master_mdl);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
        }

        if (need_dummy) {
            dummypage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, ALLOC_TAG);
            if (!dummypage) {
                ERR("out of memory\n");
                MmUnlockPages(master_mdl);
                IoFreeMdl(master_mdl);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            dummy_mdl = IoAllocateMdl(dummypage, PAGE_SIZE, FALSE, FALSE, NULL);
            if (!dummy_mdl) {
                ERR("IoAllocateMdl failed\n");
                MmUnlockPages(master_mdl);
                IoFreeMdl(master_mdl);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            MmBuildMdlForNonPagedPool(dummy_mdl);

            dummy = *(PFN_NUMBER*)(dummy_mdl + 1);
        }

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes);

        pos = 0;

        while (pos < length) {
            PFN_NUMBER* stripe_pfns;

            parity = (((addr - offset + pos) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            if (pos == 0) {
                UINT16 stripe = (parity + startoffstripe + 1) % ci->num_stripes;
                UINT32 readlen = min(length - pos, (UINT32)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart,
                                                       ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length)));

                stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] = readlen;
                pos += readlen;

                stripe = (stripe + 1) % ci->num_stripes;

                while (stripe != parity) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = min(length - pos, (UINT32)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] = readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 1)) {
                UINT16 stripe = (parity + 1) % ci->num_stripes;
                ULONG k;

                while (stripe != parity) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                    stripeoff[stripe] += (UINT32)ci->stripe_length;
                    pos += (UINT32)ci->stripe_length;

                    stripe = (stripe + 1) % ci->num_stripes;
                }

                stripe_pfns = (PFN_NUMBER*)(context.stripes[parity].mdl + 1);

                for (k = 0; k < ci->stripe_length >> PAGE_SHIFT; k++) {
                    stripe_pfns[stripeoff[parity] >> PAGE_SHIFT] = dummy;
                    stripeoff[parity] += PAGE_SIZE;
                }
            } else {
                UINT16 stripe = (parity + 1) % ci->num_stripes;
                UINT32 readlen;

                while (pos < length) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = min(length - pos, (ULONG)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] += readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            }
        }

        MmUnlockPages(master_mdl);
        IoFreeMdl(master_mdl);

        ExFreePool(stripeoff);
    } else if (type == BLOCK_FLAG_RAID6) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe, parity1;
        UINT32 *stripeoff, pos;
        PMDL master_mdl;
        PFN_NUMBER *pfns, dummy;
        BOOL need_dummy = FALSE;

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 2, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes - 2, &endoff, &endoffstripe);

        if (file_read) {
            context.va = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);

            if (!context.va) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        } else
            context.va = buf;

        master_mdl = IoAllocateMdl(context.va, length, FALSE, FALSE, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoWriteAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08x\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        pos = 0;
        while (pos < length) {
            parity1 = (((addr - offset + pos) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;

            if (pos == 0) {
                UINT16 stripe = (parity1 + startoffstripe + 2) % ci->num_stripes, parity2;
                ULONG skip, readlen;

                i = startoffstripe;
                while (stripe != parity1) {
                    if (i == startoffstripe) {
                        readlen = (ULONG)min(length, ci->stripe_length - (startoff % ci->stripe_length));

                        context.stripes[stripe].stripestart = startoff;
                        context.stripes[stripe].stripeend = startoff + readlen;

                        pos += readlen;

                        if (pos == length)
                            break;
                    } else {
                        readlen = min(length - pos, (ULONG)ci->stripe_length);

                        context.stripes[stripe].stripestart = startoff - (startoff % ci->stripe_length);
                        context.stripes[stripe].stripeend = context.stripes[stripe].stripestart + readlen;

                        pos += readlen;

                        if (pos == length)
                            break;
                    }

                    i++;
                    stripe = (stripe + 1) % ci->num_stripes;
                }

                if (pos == length)
                    break;

                for (i = 0; i < startoffstripe; i++) {
                    UINT16 stripe2 = (parity1 + i + 2) % ci->num_stripes;

                    context.stripes[stripe2].stripestart = context.stripes[stripe2].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
                }

                context.stripes[parity1].stripestart = context.stripes[parity1].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;

                parity2 = (parity1 + 1) % ci->num_stripes;
                context.stripes[parity2].stripestart = context.stripes[parity2].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;

                if (length - pos > ci->num_stripes * (ci->num_stripes - 2) * ci->stripe_length) {
                    skip = (ULONG)(((length - pos) / (ci->num_stripes * (ci->num_stripes - 2) * ci->stripe_length)) - 1);

                    for (i = 0; i < ci->num_stripes; i++) {
                        context.stripes[i].stripeend += skip * ci->num_stripes * ci->stripe_length;
                    }

                    pos += (UINT32)(skip * (ci->num_stripes - 2) * ci->num_stripes * ci->stripe_length);
                    need_dummy = TRUE;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 2)) {
                for (i = 0; i < ci->num_stripes; i++) {
                    context.stripes[i].stripeend += ci->stripe_length;
                }

                pos += (UINT32)(ci->stripe_length * (ci->num_stripes - 2));
                need_dummy = TRUE;
            } else {
                UINT16 stripe = (parity1 + 2) % ci->num_stripes;

                i = 0;
                while (stripe != parity1) {
                    if (endoffstripe == i) {
                        context.stripes[stripe].stripeend = endoff + 1;
                        break;
                    } else if (endoffstripe > i)
                        context.stripes[stripe].stripeend = endoff - (endoff % ci->stripe_length) + ci->stripe_length;

                    i++;
                    stripe = (stripe + 1) % ci->num_stripes;
                }

                break;
            }
        }

        for (i = 0; i < ci->num_stripes; i++) {
            if (context.stripes[i].stripestart != context.stripes[i].stripeend) {
                context.stripes[i].mdl = IoAllocateMdl(context.va, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart), FALSE, FALSE, NULL);

                if (!context.stripes[i].mdl) {
                    ERR("IoAllocateMdl failed\n");
                    MmUnlockPages(master_mdl);
                    IoFreeMdl(master_mdl);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
        }

        if (need_dummy) {
            dummypage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, ALLOC_TAG);
            if (!dummypage) {
                ERR("out of memory\n");
                MmUnlockPages(master_mdl);
                IoFreeMdl(master_mdl);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            dummy_mdl = IoAllocateMdl(dummypage, PAGE_SIZE, FALSE, FALSE, NULL);
            if (!dummy_mdl) {
                ERR("IoAllocateMdl failed\n");
                MmUnlockPages(master_mdl);
                IoFreeMdl(master_mdl);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            MmBuildMdlForNonPagedPool(dummy_mdl);

            dummy = *(PFN_NUMBER*)(dummy_mdl + 1);
        }

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes);

        pos = 0;

        while (pos < length) {
            PFN_NUMBER* stripe_pfns;

            parity1 = (((addr - offset + pos) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;

            if (pos == 0) {
                UINT16 stripe = (parity1 + startoffstripe + 2) % ci->num_stripes;
                UINT32 readlen = min(length - pos, (UINT32)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart,
                                                       ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length)));

                stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] = readlen;
                pos += readlen;

                stripe = (stripe + 1) % ci->num_stripes;

                while (stripe != parity1) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = (UINT32)min(length - pos, min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] = readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 2)) {
                UINT16 stripe = (parity1 + 2) % ci->num_stripes;
                UINT16 parity2 = (parity1 + 1) % ci->num_stripes;
                ULONG k;

                while (stripe != parity1) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                    stripeoff[stripe] += (UINT32)ci->stripe_length;
                    pos += (UINT32)ci->stripe_length;

                    stripe = (stripe + 1) % ci->num_stripes;
                }

                stripe_pfns = (PFN_NUMBER*)(context.stripes[parity1].mdl + 1);

                for (k = 0; k < ci->stripe_length >> PAGE_SHIFT; k++) {
                    stripe_pfns[stripeoff[parity1] >> PAGE_SHIFT] = dummy;
                    stripeoff[parity1] += PAGE_SIZE;
                }

                stripe_pfns = (PFN_NUMBER*)(context.stripes[parity2].mdl + 1);

                for (k = 0; k < ci->stripe_length >> PAGE_SHIFT; k++) {
                    stripe_pfns[stripeoff[parity2] >> PAGE_SHIFT] = dummy;
                    stripeoff[parity2] += PAGE_SIZE;
                }
            } else {
                UINT16 stripe = (parity1 + 2) % ci->num_stripes;
                UINT32 readlen;

                while (pos < length) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = (UINT32)min(length - pos, min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] += readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            }
        }

        MmUnlockPages(master_mdl);
        IoFreeMdl(master_mdl);

        ExFreePool(stripeoff);
    }

    context.address = addr;

    for (i = 0; i < ci->num_stripes; i++) {
        if (!devices[i] || !devices[i]->devobj || context.stripes[i].stripestart == context.stripes[i].stripeend) {
            context.stripes[i].status = ReadDataStatus_MissingDevice;
            context.stripes_left--;

            if (!devices[i] || !devices[i]->devobj)
                missing_devices++;
        }
    }

    if (missing_devices > allowed_missing) {
        ERR("not enough devices to service request (%u missing)\n", missing_devices);
        Status = STATUS_UNEXPECTED_IO_ERROR;
        goto exit;
    }

    for (i = 0; i < ci->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;

        if (devices[i] && devices[i]->devobj && context.stripes[i].stripestart != context.stripes[i].stripeend && context.stripes[i].status != ReadDataStatus_Skip) {
            context.stripes[i].context = (struct read_data_context*)&context;

            if (type == BLOCK_FLAG_RAID10) {
                context.stripes[i].stripenum = i / ci->sub_stripes;
            }

            if (!Irp) {
                context.stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, FALSE);

                if (!context.stripes[i].Irp) {
                    ERR("IoAllocateIrp failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            } else {
                context.stripes[i].Irp = IoMakeAssociatedIrp(Irp, devices[i]->devobj->StackSize);

                if (!context.stripes[i].Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }

            IrpSp = IoGetNextIrpStackLocation(context.stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;

            if (devices[i]->devobj->Flags & DO_BUFFERED_IO) {
                context.stripes[i].Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart), ALLOC_TAG);
                if (!context.stripes[i].Irp->AssociatedIrp.SystemBuffer) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                context.stripes[i].Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;

                context.stripes[i].Irp->UserBuffer = MmGetSystemAddressForMdlSafe(context.stripes[i].mdl, priority);
            } else if (devices[i]->devobj->Flags & DO_DIRECT_IO)
                context.stripes[i].Irp->MdlAddress = context.stripes[i].mdl;
            else
                context.stripes[i].Irp->UserBuffer = MmGetSystemAddressForMdlSafe(context.stripes[i].mdl, priority);

            IrpSp->Parameters.Read.Length = (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart);
            IrpSp->Parameters.Read.ByteOffset.QuadPart = context.stripes[i].stripestart + cis[i].offset;

            total_reading += IrpSp->Parameters.Read.Length;

            context.stripes[i].Irp->UserIosb = &context.stripes[i].iosb;

            IoSetCompletionRoutine(context.stripes[i].Irp, read_data_completion, &context.stripes[i], TRUE, TRUE, TRUE);

            context.stripes[i].status = ReadDataStatus_Pending;
        }
    }

#ifdef DEBUG_STATS
    if (!is_tree)
        time1 = KeQueryPerformanceCounter(NULL);
#endif

    need_to_wait = FALSE;
    for (i = 0; i < ci->num_stripes; i++) {
        if (context.stripes[i].status != ReadDataStatus_MissingDevice && context.stripes[i].status != ReadDataStatus_Skip) {
            IoCallDriver(devices[i]->devobj, context.stripes[i].Irp);
            need_to_wait = TRUE;
        }
    }

    if (need_to_wait)
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

#ifdef DEBUG_STATS
    if (!is_tree) {
        time2 = KeQueryPerformanceCounter(NULL);

        Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
    }
#endif

    if (diskacc)
        fFsRtlUpdateDiskCounters(total_reading, 0);

    // check if any of the devices return a "user-induced" error

    for (i = 0; i < ci->num_stripes; i++) {
        if (context.stripes[i].status == ReadDataStatus_Error && IoIsErrorUserInduced(context.stripes[i].iosb.Status)) {
            Status = context.stripes[i].iosb.Status;
            goto exit;
        }
    }

    if (type == BLOCK_FLAG_RAID0) {
        Status = read_data_raid0(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, generation, offset);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid0 returned %08x\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_RAID10) {
        Status = read_data_raid10(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, generation, offset);

        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid10 returned %08x\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        Status = read_data_dup(Vcb, file_read ? context.va : buf, addr, &context, ci, devices, generation);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_dup returned %08x\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_RAID5) {
        Status = read_data_raid5(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, offset, generation, c, missing_devices > 0 ? TRUE : FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid5 returned %08x\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_RAID6) {
        Status = read_data_raid6(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, offset, generation, c, missing_devices > 0 ? TRUE : FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid6 returned %08x\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    }

exit:
    if (c && (type == BLOCK_FLAG_RAID5 || type == BLOCK_FLAG_RAID6))
        chunk_unlock_range(Vcb, c, lockaddr, locklen);

    if (dummy_mdl)
        IoFreeMdl(dummy_mdl);

    if (dummypage)
        ExFreePool(dummypage);

    for (i = 0; i < ci->num_stripes; i++) {
        if (context.stripes[i].mdl) {
            if (context.stripes[i].mdl->MdlFlags & MDL_PAGES_LOCKED)
                MmUnlockPages(context.stripes[i].mdl);

            IoFreeMdl(context.stripes[i].mdl);
        }

        if (context.stripes[i].Irp)
            IoFreeIrp(context.stripes[i].Irp);
    }

    ExFreePool(context.stripes);

    if (!Vcb->log_to_phys_loaded)
        ExFreePool(devices);

    return Status;
}

NTSTATUS read_stream(fcb* fcb, UINT8* data, UINT64 start, ULONG length, ULONG* pbr) {
    ULONG readlen;

    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);

    if (pbr) *pbr = 0;

    if (start >= fcb->adsdata.Length) {
        TRACE("tried to read beyond end of stream\n");
        return STATUS_END_OF_FILE;
    }

    if (length == 0) {
        WARN("tried to read zero bytes\n");
        return STATUS_SUCCESS;
    }

    if (start + length < fcb->adsdata.Length)
        readlen = length;
    else
        readlen = fcb->adsdata.Length - (ULONG)start;

    if (readlen > 0)
        RtlCopyMemory(data + start, fcb->adsdata.Buffer, readlen);

    if (pbr) *pbr = readlen;

    return STATUS_SUCCESS;
}

NTSTATUS read_file(fcb* fcb, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr, PIRP Irp) {
    NTSTATUS Status;
    EXTENT_DATA* ed;
    UINT32 bytes_read = 0;
    UINT64 last_end;
    LIST_ENTRY* le;
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
#endif

    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);

    if (pbr)
        *pbr = 0;

    if (start >= fcb->inode_item.st_size) {
        WARN("Tried to read beyond end of file\n");
        Status = STATUS_END_OF_FILE;
        goto exit;
    }

#ifdef DEBUG_STATS
    time1 = KeQueryPerformanceCounter(NULL);
#endif

    le = fcb->extents.Flink;

    last_end = start;

    while (le != &fcb->extents) {
        UINT64 len;
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        EXTENT_DATA2* ed2;

        if (!ext->ignore) {
            ed = &ext->extent_data;

            ed2 = (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) ? (EXTENT_DATA2*)ed->data : NULL;

            len = ed2 ? ed2->num_bytes : ed->decoded_size;

            if (ext->offset + len <= start) {
                last_end = ext->offset + len;
                goto nextitem;
            }

            if (ext->offset > last_end && ext->offset > start + bytes_read) {
                UINT32 read = (UINT32)min(length, ext->offset - max(start, last_end));

                RtlZeroMemory(data + bytes_read, read);
                bytes_read += read;
                length -= read;
            }

            if (length == 0 || ext->offset > start + bytes_read + length)
                break;

            if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
                WARN("Encryption not supported\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto exit;
            }

            if (ed->encoding != BTRFS_ENCODING_NONE) {
                WARN("Other encodings not supported\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto exit;
            }

            switch (ed->type) {
                case EXTENT_TYPE_INLINE:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT32 read;

                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        read = (UINT32)min(min(len, ext->datalen) - off, length);

                        RtlCopyMemory(data + bytes_read, &ed->data[off], read);
                    } else if (ed->compression == BTRFS_COMPRESSION_ZLIB || ed->compression == BTRFS_COMPRESSION_LZO || ed->compression == BTRFS_COMPRESSION_ZSTD) {
                        UINT8* decomp;
                        BOOL decomp_alloc;
                        UINT16 inlen = ext->datalen - (UINT16)offsetof(EXTENT_DATA, data[0]);

                        if (ed->decoded_size == 0 || ed->decoded_size > 0xffffffff) {
                            ERR("ed->decoded_size was invalid (%llx)\n", ed->decoded_size);
                            Status = STATUS_INTERNAL_ERROR;
                            goto exit;
                        }

                        read = (UINT32)min(ed->decoded_size - off, length);

                        if (off > 0) {
                            decomp = ExAllocatePoolWithTag(NonPagedPool, (UINT32)ed->decoded_size, ALLOC_TAG);
                            if (!decomp) {
                                ERR("out of memory\n");
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                goto exit;
                            }

                            decomp_alloc = TRUE;
                        } else {
                            decomp = data + bytes_read;
                            decomp_alloc = FALSE;
                        }

                        if (ed->compression == BTRFS_COMPRESSION_ZLIB) {
                            Status = zlib_decompress(ed->data, inlen, decomp, (UINT32)(read + off));
                            if (!NT_SUCCESS(Status)) {
                                ERR("zlib_decompress returned %08x\n", Status);
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_LZO) {
                            if (inlen < sizeof(UINT32)) {
                                ERR("extent data was truncated\n");
                                Status = STATUS_INTERNAL_ERROR;
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            } else
                                inlen -= sizeof(UINT32);

                            Status = lzo_decompress(ed->data + sizeof(UINT32), inlen, decomp, (UINT32)(read + off), sizeof(UINT32));
                            if (!NT_SUCCESS(Status)) {
                                ERR("lzo_decompress returned %08x\n", Status);
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_ZSTD) {
                            Status = zstd_decompress(ed->data, inlen, decomp, (UINT32)(read + off));
                            if (!NT_SUCCESS(Status)) {
                                ERR("zstd_decompress returned %08x\n", Status);
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            }
                        }

                        if (decomp_alloc) {
                            RtlCopyMemory(data + bytes_read, decomp + off, read);
                            ExFreePool(decomp);
                        }
                    } else {
                        ERR("unhandled compression type %x\n", ed->compression);
                        Status = STATUS_NOT_IMPLEMENTED;
                        goto exit;
                    }

                    bytes_read += read;
                    length -= read;

                    break;
                }

                case EXTENT_TYPE_REGULAR:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT32 to_read, read;
                    UINT8* buf;
                    BOOL mdl = (Irp && Irp->MdlAddress) ? TRUE : FALSE;
                    BOOL buf_free;
                    UINT32 bumpoff = 0, *csum;
                    UINT64 addr;
                    chunk* c;

                    read = (UINT32)(len - off);
                    if (read > length) read = (UINT32)length;

                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        addr = ed2->address + ed2->offset + off;
                        to_read = (UINT32)sector_align(read, fcb->Vcb->superblock.sector_size);

                        if (addr % fcb->Vcb->superblock.sector_size > 0) {
                            bumpoff = addr % fcb->Vcb->superblock.sector_size;
                            addr -= bumpoff;
                            to_read = (UINT32)sector_align(read + bumpoff, fcb->Vcb->superblock.sector_size);
                        }
                    } else {
                        addr = ed2->address;
                        to_read = (UINT32)sector_align(ed2->size, fcb->Vcb->superblock.sector_size);
                    }

                    if (ed->compression == BTRFS_COMPRESSION_NONE && start % fcb->Vcb->superblock.sector_size == 0 &&
                        length % fcb->Vcb->superblock.sector_size == 0) {
                        buf = data + bytes_read;
                        buf_free = FALSE;
                    } else {
                        buf = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
                        buf_free = TRUE;

                        if (!buf) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }

                        mdl = FALSE;
                    }

                    c = get_chunk_from_address(fcb->Vcb, addr);

                    if (!c) {
                        ERR("get_chunk_from_address(%llx) failed\n", addr);

                        if (buf_free)
                            ExFreePool(buf);

                        goto exit;
                    }

                    if (ext->csum) {
                        if (ed->compression == BTRFS_COMPRESSION_NONE)
                            csum = &ext->csum[off / fcb->Vcb->superblock.sector_size];
                        else
                            csum = ext->csum;
                    } else
                        csum = NULL;

                    Status = read_data(fcb->Vcb, addr, to_read, csum, FALSE, buf, c, NULL, Irp, 0, mdl,
                                       fcb && fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_data returned %08x\n", Status);

                        if (buf_free)
                            ExFreePool(buf);

                        goto exit;
                    }

                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        if (buf_free)
                            RtlCopyMemory(data + bytes_read, buf + bumpoff, read);
                    } else {
                        UINT8 *decomp = NULL, *buf2;
                        ULONG outlen, inlen, off2;
                        UINT32 inpageoff = 0;

                        off2 = (ULONG)(ed2->offset + off);
                        buf2 = buf;
                        inlen = (ULONG)ed2->size;

                        if (ed->compression == BTRFS_COMPRESSION_LZO) {
                            ULONG inoff = sizeof(UINT32);

                            inlen -= sizeof(UINT32);

                            // If reading a few sectors in, skip to the interesting bit
                            while (off2 > LINUX_PAGE_SIZE) {
                                UINT32 partlen;

                                if (inlen < sizeof(UINT32))
                                    break;

                                partlen = *(UINT32*)(buf2 + inoff);

                                if (partlen < inlen) {
                                    off2 -= LINUX_PAGE_SIZE;
                                    inoff += partlen + sizeof(UINT32);
                                    inlen -= partlen + sizeof(UINT32);

                                    if (LINUX_PAGE_SIZE - (inoff % LINUX_PAGE_SIZE) < sizeof(UINT32))
                                        inoff = ((inoff / LINUX_PAGE_SIZE) + 1) * LINUX_PAGE_SIZE;
                                } else
                                    break;
                            }

                            buf2 = &buf2[inoff];
                            inpageoff = inoff % LINUX_PAGE_SIZE;
                        }

                        if (off2 != 0) {
                            outlen = off2 + min(read, (UINT32)(ed2->num_bytes - off));

                            decomp = ExAllocatePoolWithTag(PagedPool, outlen, ALLOC_TAG);
                            if (!decomp) {
                                ERR("out of memory\n");
                                ExFreePool(buf);
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                goto exit;
                            }
                        } else
                            outlen = min(read, (UINT32)(ed2->num_bytes - off));

                        if (ed->compression == BTRFS_COMPRESSION_ZLIB) {
                            Status = zlib_decompress(buf2, inlen, decomp ? decomp : (data + bytes_read), outlen);

                            if (!NT_SUCCESS(Status)) {
                                ERR("zlib_decompress returned %08x\n", Status);
                                ExFreePool(buf);

                                if (decomp)
                                    ExFreePool(decomp);

                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_LZO) {
                            Status = lzo_decompress(buf2, inlen, decomp ? decomp : (data + bytes_read), outlen, inpageoff);

                            if (!NT_SUCCESS(Status)) {
                                ERR("lzo_decompress returned %08x\n", Status);
                                ExFreePool(buf);

                                if (decomp)
                                    ExFreePool(decomp);

                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_ZSTD) {
                            Status = zstd_decompress(buf2, inlen, decomp ? decomp : (data + bytes_read), outlen);

                            if (!NT_SUCCESS(Status)) {
                                ERR("zstd_decompress returned %08x\n", Status);
                                ExFreePool(buf);

                                if (decomp)
                                    ExFreePool(decomp);

                                goto exit;
                            }
                        } else {
                            ERR("unsupported compression type %x\n", ed->compression);
                            Status = STATUS_NOT_SUPPORTED;

                            ExFreePool(buf);

                            if (decomp)
                                ExFreePool(decomp);

                            goto exit;
                        }

                        if (decomp) {
                            RtlCopyMemory(data + bytes_read, decomp + off2, (size_t)min(read, ed2->num_bytes - off));
                            ExFreePool(decomp);
                        }
                    }

                    if (buf_free)
                        ExFreePool(buf);

                    bytes_read += read;
                    length -= read;

                    break;
                }

                case EXTENT_TYPE_PREALLOC:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT32 read = (UINT32)(len - off);

                    if (read > length) read = (UINT32)length;

                    RtlZeroMemory(data + bytes_read, read);

                    bytes_read += read;
                    length -= read;

                    break;
                }

                default:
                    WARN("Unsupported extent data type %u\n", ed->type);
                    Status = STATUS_NOT_IMPLEMENTED;
                    goto exit;
            }

            last_end = ext->offset + len;

            if (length == 0)
                break;
        }

nextitem:
        le = le->Flink;
    }

    if (length > 0 && start + bytes_read < fcb->inode_item.st_size) {
        UINT32 read = (UINT32)min(fcb->inode_item.st_size - start - bytes_read, length);

        RtlZeroMemory(data + bytes_read, read);

        bytes_read += read;
        length -= read;
    }

    Status = STATUS_SUCCESS;
    if (pbr)
        *pbr = bytes_read;

#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);

    fcb->Vcb->stats.num_reads++;
    fcb->Vcb->stats.data_read += bytes_read;
    fcb->Vcb->stats.read_total_time += time2.QuadPart - time1.QuadPart;
#endif

exit:
    return Status;
}

NTSTATUS do_read(PIRP Irp, BOOLEAN wait, ULONG* bytes_read) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    UINT8* data = NULL;
    ULONG length = IrpSp->Parameters.Read.Length, addon = 0;
    UINT64 start = IrpSp->Parameters.Read.ByteOffset.QuadPart;

    *bytes_read = 0;

    if (!fcb || !fcb->Vcb || !fcb->subvol)
        return STATUS_INTERNAL_ERROR;

    TRACE("file = %S (fcb = %p)\n", file_desc(FileObject), fcb);
    TRACE("offset = %llx, length = %x\n", start, length);
    TRACE("paging_io = %s, no cache = %s\n", Irp->Flags & IRP_PAGING_IO ? "TRUE" : "FALSE", Irp->Flags & IRP_NOCACHE ? "TRUE" : "FALSE");

    if (!fcb->ads && fcb->type == BTRFS_TYPE_DIRECTORY)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (!(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForReadAccess(&fcb->lock, Irp)) {
        WARN("tried to read locked region\n");
        return STATUS_FILE_LOCK_CONFLICT;
    }

    if (length == 0) {
        TRACE("tried to read zero bytes\n");
        return STATUS_SUCCESS;
    }

    if (start >= (UINT64)fcb->Header.FileSize.QuadPart) {
        TRACE("tried to read with offset after file end (%llx >= %llx)\n", start, fcb->Header.FileSize.QuadPart);
        return STATUS_END_OF_FILE;
    }

    TRACE("FileObject %p fcb %p FileSize = %llx st_size = %llx (%p)\n", FileObject, fcb, fcb->Header.FileSize.QuadPart, fcb->inode_item.st_size, &fcb->inode_item.st_size);

    if (Irp->Flags & IRP_NOCACHE || !(IrpSp->MinorFunction & IRP_MN_MDL)) {
        data = map_user_buffer(Irp, fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);

        if (Irp->MdlAddress && !data) {
            ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (start >= (UINT64)fcb->Header.ValidDataLength.QuadPart) {
            length = (ULONG)min(length, min(start + length, (UINT64)fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart);
            RtlZeroMemory(data, length);
            Irp->IoStatus.Information = *bytes_read = length;
            return STATUS_SUCCESS;
        }

        if (length + start > (UINT64)fcb->Header.ValidDataLength.QuadPart) {
            addon = (ULONG)(min(start + length, (UINT64)fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart);
            RtlZeroMemory(data + (fcb->Header.ValidDataLength.QuadPart - start), addon);
            length = (ULONG)(fcb->Header.ValidDataLength.QuadPart - start);
        }
    }

    if (!(Irp->Flags & IRP_NOCACHE)) {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY {
            if (!FileObject->PrivateCacheMap) {
                CC_FILE_SIZES ccfs;

                ccfs.AllocationSize = fcb->Header.AllocationSize;
                ccfs.FileSize = fcb->Header.FileSize;
                ccfs.ValidDataLength = fcb->Header.ValidDataLength;

                init_file_cache(FileObject, &ccfs);
            }

            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                CcMdlRead(FileObject,&IrpSp->Parameters.Read.ByteOffset, length, &Irp->MdlAddress, &Irp->IoStatus);
            } else {
                if (fCcCopyReadEx) {
                    TRACE("CcCopyReadEx(%p, %llx, %x, %u, %p, %p, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart,
                          length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread);
                    TRACE("sizes = %llx, %llx, %llx\n", fcb->Header.AllocationSize, fcb->Header.FileSize, fcb->Header.ValidDataLength);
                    if (!fCcCopyReadEx(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread)) {
                        TRACE("CcCopyReadEx could not wait\n");

                        IoMarkIrpPending(Irp);
                        return STATUS_PENDING;
                    }
                    TRACE("CcCopyReadEx finished\n");
                } else {
                    TRACE("CcCopyRead(%p, %llx, %x, %u, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart, length, wait, data, &Irp->IoStatus);
                    TRACE("sizes = %llx, %llx, %llx\n", fcb->Header.AllocationSize, fcb->Header.FileSize, fcb->Header.ValidDataLength);
                    if (!CcCopyRead(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus)) {
                        TRACE("CcCopyRead could not wait\n");

                        IoMarkIrpPending(Irp);
                        return STATUS_PENDING;
                    }
                    TRACE("CcCopyRead finished\n");
                }
            }
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (NT_SUCCESS(Status)) {
            Status = Irp->IoStatus.Status;
            Irp->IoStatus.Information += addon;
            *bytes_read = (ULONG)Irp->IoStatus.Information;
        } else
            ERR("EXCEPTION - %08x\n", Status);

        return Status;
    } else {
        NTSTATUS Status;

        if (!wait) {
            IoMarkIrpPending(Irp);
            return STATUS_PENDING;
        }

        if (fcb->ads)
            Status = read_stream(fcb, data, start, length, bytes_read);
        else
            Status = read_file(fcb, data, start, length, bytes_read, Irp);

        *bytes_read += addon;
        TRACE("read %u bytes\n", *bytes_read);

        Irp->IoStatus.Information = *bytes_read;

        if (diskacc && Status != STATUS_PENDING) {
            PETHREAD thread = NULL;

            if (Irp->Tail.Overlay.Thread && !IoIsSystemThread(Irp->Tail.Overlay.Thread))
                thread = Irp->Tail.Overlay.Thread;
            else if (!IoIsSystemThread(PsGetCurrentThread()))
                thread = PsGetCurrentThread();
            else if (IoIsSystemThread(PsGetCurrentThread()) && IoGetTopLevelIrp() == Irp)
                thread = PsGetCurrentThread();

            if (thread)
                fPsUpdateDiskCounters(PsGetThreadProcess(thread), *bytes_read, 0, 1, 0, 0);
        }

        return Status;
    }
}

_Dispatch_type_(IRP_MJ_READ)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    ULONG bytes_read = 0;
    NTSTATUS Status;
    BOOL top_level;
    fcb* fcb;
    ccb* ccb;
    BOOLEAN fcb_lock = FALSE, wait;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    TRACE("read\n");

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_read(DeviceObject, Irp);
        goto exit2;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Irp->IoStatus.Information = 0;

    if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
        CcMdlReadComplete(IrpSp->FileObject, Irp->MdlAddress);

        Irp->MdlAddress = NULL;
        Status = STATUS_SUCCESS;

        goto exit;
    }

    fcb = FileObject->FsContext;

    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    ccb = FileObject->FsContext2;

    if (!ccb) {
        ERR("ccb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_READ_DATA)) {
        WARN("insufficient privileges\n");
        Status = STATUS_ACCESS_DENIED;
        goto exit;
    }

    if (fcb == Vcb->volume_fcb) {
        TRACE("reading volume FCB\n");

        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);

        goto exit2;
    }

    wait = IoIsOperationSynchronous(Irp);

    // Don't offload jobs when doing paging IO - otherwise this can lead to
    // deadlocks in CcCopyRead.
    if (Irp->Flags & IRP_PAGING_IO)
        wait = TRUE;

    if (!(Irp->Flags & IRP_PAGING_IO) && FileObject->SectionObjectPointer && FileObject->SectionObjectPointer->DataSectionObject) {
        IO_STATUS_BLOCK iosb;

        CcFlushCache(FileObject->SectionObjectPointer, &IrpSp->Parameters.Read.ByteOffset, IrpSp->Parameters.Read.Length, &iosb);
        if (!NT_SUCCESS(iosb.Status)) {
            ERR("CcFlushCache returned %08x\n", iosb.Status);
            return iosb.Status;
        }
    }

    if (!ExIsResourceAcquiredSharedLite(fcb->Header.Resource)) {
        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            goto exit;
        }

        fcb_lock = TRUE;
    }

    Status = do_read(Irp, wait, &bytes_read);

    if (fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);

exit:
    if (FileObject->Flags & FO_SYNCHRONOUS_IO && !(Irp->Flags & IRP_PAGING_IO))
        FileObject->CurrentByteOffset.QuadPart = IrpSp->Parameters.Read.ByteOffset.QuadPart + (NT_SUCCESS(Status) ? bytes_read : 0);

end:
    Irp->IoStatus.Status = Status;

    TRACE("Irp->IoStatus.Status = %08x\n", Irp->IoStatus.Status);
    TRACE("Irp->IoStatus.Information = %lu\n", Irp->IoStatus.Information);
    TRACE("returning %08x\n", Status);

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    else {
        if (!add_thread_job(Vcb, Irp))
            do_read_job(Irp);
    }

exit2:
    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
