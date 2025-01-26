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
    uint16_t stripenum;
    bool rewrite;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_data_status status;
    PMDL mdl;
    uint64_t stripestart;
    uint64_t stripeend;
} read_data_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
    uint64_t address;
    uint32_t buflen;
    LONG num_stripes, stripes_left;
    uint64_t type;
    uint32_t sector_size;
    uint16_t firstoff, startoffstripe, sectors_per_stripe;
    void* csum;
    bool tree;
    read_data_stripe* stripes;
    uint8_t* va;
} read_data_context;

extern bool diskacc;
extern tPsUpdateDiskCounters fPsUpdateDiskCounters;
extern tCcCopyReadEx fCcCopyReadEx;
extern tFsRtlUpdateDiskCounters fFsRtlUpdateDiskCounters;

#define LZO_PAGE_SIZE 4096

_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_data_stripe* stripe = conptr;
    read_data_context* context = (read_data_context*)stripe->context;

    UNUSED(DeviceObject);

    stripe->iosb = Irp->IoStatus;

    if (NT_SUCCESS(Irp->IoStatus.Status))
        stripe->status = ReadDataStatus_Success;
    else
        stripe->status = ReadDataStatus_Error;

    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS check_csum(device_extension* Vcb, uint8_t* data, uint32_t sectors, void* csum) {
    void* csum2;

    csum2 = ExAllocatePoolWithTag(PagedPool, Vcb->csum_size * sectors, ALLOC_TAG);
    if (!csum2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    do_calc_job(Vcb, data, sectors, csum2);

    if (RtlCompareMemory(csum2, csum, sectors * Vcb->csum_size) != sectors * Vcb->csum_size) {
        ExFreePool(csum2);
        return STATUS_CRC_ERROR;
    }

    ExFreePool(csum2);

    return STATUS_SUCCESS;
}

void get_tree_checksum(device_extension* Vcb, tree_header* th, void* csum) {
    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C:
            *(uint32_t*)csum = ~calc_crc32c(0xffffffff, (uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        break;

        case CSUM_TYPE_XXHASH:
            *(uint64_t*)csum = XXH64((uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum), 0);
        break;

        case CSUM_TYPE_SHA256:
            calc_sha256(csum, &th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        break;

        case CSUM_TYPE_BLAKE2:
            blake2b(csum, BLAKE2_HASH_SIZE, (uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        break;
    }
}

bool check_tree_checksum(device_extension* Vcb, tree_header* th) {
    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C: {
            uint32_t crc32 = ~calc_crc32c(0xffffffff, (uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

            if (crc32 == *((uint32_t*)th->csum))
                return true;

            WARN("hash was %08x, expected %08x\n", crc32, *((uint32_t*)th->csum));

            break;
        }

        case CSUM_TYPE_XXHASH: {
            uint64_t hash = XXH64((uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum), 0);

            if (hash == *((uint64_t*)th->csum))
                return true;

            WARN("hash was %I64x, expected %I64x\n", hash, *((uint64_t*)th->csum));

            break;
        }

        case CSUM_TYPE_SHA256: {
            uint8_t hash[SHA256_HASH_SIZE];

            calc_sha256(hash, (uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

            if (RtlCompareMemory(hash, th, SHA256_HASH_SIZE) == SHA256_HASH_SIZE)
                return true;

            WARN("hash was invalid\n");

            break;
        }

        case CSUM_TYPE_BLAKE2: {
            uint8_t hash[BLAKE2_HASH_SIZE];

            blake2b(hash, sizeof(hash), (uint8_t*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));

            if (RtlCompareMemory(hash, th, BLAKE2_HASH_SIZE) == BLAKE2_HASH_SIZE)
                return true;

            WARN("hash was invalid\n");

            break;
        }
    }

    return false;
}

void get_sector_csum(device_extension* Vcb, void* buf, void* csum) {
    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C:
            *(uint32_t*)csum = ~calc_crc32c(0xffffffff, buf, Vcb->superblock.sector_size);
        break;

        case CSUM_TYPE_XXHASH:
            *(uint64_t*)csum = XXH64(buf, Vcb->superblock.sector_size, 0);
        break;

        case CSUM_TYPE_SHA256:
            calc_sha256(csum, buf, Vcb->superblock.sector_size);
        break;

        case CSUM_TYPE_BLAKE2:
            blake2b(csum, BLAKE2_HASH_SIZE, buf, Vcb->superblock.sector_size);
        break;
    }
}

bool check_sector_csum(device_extension* Vcb, void* buf, void* csum) {
    switch (Vcb->superblock.csum_type) {
        case CSUM_TYPE_CRC32C: {
            uint32_t crc32 = ~calc_crc32c(0xffffffff, buf, Vcb->superblock.sector_size);

            return *(uint32_t*)csum == crc32;
        }

        case CSUM_TYPE_XXHASH: {
            uint64_t hash = XXH64(buf, Vcb->superblock.sector_size, 0);

            return *(uint64_t*)csum == hash;
        }

        case CSUM_TYPE_SHA256: {
            uint8_t hash[SHA256_HASH_SIZE];

            calc_sha256(hash, buf, Vcb->superblock.sector_size);

            return RtlCompareMemory(hash, csum, SHA256_HASH_SIZE) == SHA256_HASH_SIZE;
        }

        case CSUM_TYPE_BLAKE2: {
            uint8_t hash[BLAKE2_HASH_SIZE];

            blake2b(hash, sizeof(hash), buf, Vcb->superblock.sector_size);

            return RtlCompareMemory(hash, csum, BLAKE2_HASH_SIZE) == BLAKE2_HASH_SIZE;
        }
    }

    return false;
}

static NTSTATUS read_data_dup(device_extension* Vcb, uint8_t* buf, uint64_t addr, read_data_context* context, CHUNK_ITEM* ci,
                              device** devices, uint64_t generation) {
    bool checksum_error = false;
    uint16_t j, stripe = 0;
    NTSTATUS Status;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08lx\n", j, context->stripes[j].iosb.Status);
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

        if (th->address != context->address || !check_tree_checksum(Vcb, th)) {
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && th->generation != generation) {
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
        Status = check_csum(Vcb, buf, (ULONG)context->stripes[stripe].Irp->IoStatus.Information / context->sector_size, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08lx\n", Status);
            return Status;
        }
    }

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (ci->num_stripes == 1)
        return STATUS_CRC_ERROR;

    if (context->tree) {
        tree_header* t2;
        bool recovered = false;

        t2 = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!t2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (j = 0; j < ci->num_stripes; j++) {
            if (j != stripe && devices[j] && devices[j]->devobj) {
                Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + context->stripes[stripe].stripestart,
                                        Vcb->superblock.node_size, (uint8_t*)t2, false);
                if (!NT_SUCCESS(Status)) {
                    WARN("sync_read_phys returned %08lx\n", Status);
                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                } else {
                    bool checksum_error = !check_tree_checksum(Vcb, t2);

                    if (t2->address == addr && !checksum_error && (generation == 0 || t2->generation == generation)) {
                        RtlCopyMemory(buf, t2, Vcb->superblock.node_size);
                        ERR("recovering from checksum error at %I64x, device %I64x\n", addr, devices[stripe]->devitem.dev_id);
                        recovered = true;

                        if (!Vcb->readonly && !devices[stripe]->readonly) { // write good data over bad
                            Status = write_data_phys(devices[stripe]->devobj, devices[stripe]->fileobj, cis[stripe].offset + context->stripes[stripe].stripestart,
                                                     t2, Vcb->superblock.node_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08lx\n", Status);
                                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }

                        break;
                    } else if (t2->address != addr || checksum_error)
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                    else
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_GENERATION_ERRORS);
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %I64x\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = (ULONG)context->stripes[stripe].Irp->IoStatus.Information >> Vcb->sector_shift;
        uint8_t* sector;
        void* ptr = context->csum;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (ULONG i = 0; i < sectors; i++) {
            if (!check_sector_csum(Vcb, buf + (i << Vcb->sector_shift), ptr)) {
                bool recovered = false;

                for (j = 0; j < ci->num_stripes; j++) {
                    if (j != stripe && devices[j] && devices[j]->devobj) {
                        Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj,
                                                cis[j].offset + context->stripes[stripe].stripestart + ((uint64_t)i << Vcb->sector_shift),
                                                Vcb->superblock.sector_size, sector, false);
                        if (!NT_SUCCESS(Status)) {
                            WARN("sync_read_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                        } else {
                            if (check_sector_csum(Vcb, sector, ptr)) {
                                RtlCopyMemory(buf + (i << Vcb->sector_shift), sector, Vcb->superblock.sector_size);
                                ERR("recovering from checksum error at %I64x, device %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift), devices[stripe]->devitem.dev_id);
                                recovered = true;

                                if (!Vcb->readonly && !devices[stripe]->readonly) { // write good data over bad
                                    Status = write_data_phys(devices[stripe]->devobj, devices[stripe]->fileobj,
                                                             cis[stripe].offset + context->stripes[stripe].stripestart + ((uint64_t)i << Vcb->sector_shift),
                                                             sector, Vcb->superblock.sector_size);
                                    if (!NT_SUCCESS(Status)) {
                                        WARN("write_data_phys returned %08lx\n", Status);
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
                    ERR("unrecoverable checksum error at %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }

            ptr = (uint8_t*)ptr + Vcb->csum_size;
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid0(device_extension* Vcb, uint8_t* buf, uint64_t addr, uint32_t length, read_data_context* context,
                                CHUNK_ITEM* ci, device** devices, uint64_t generation, uint64_t offset) {
    for (uint16_t i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08lx\n", i, context->stripes[i].iosb.Status);
            log_device_error(Vcb, devices[i], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[i].iosb.Status;
        }
    }

    if (context->tree) { // shouldn't happen, as trees shouldn't cross stripe boundaries
        tree_header* th = (tree_header*)buf;
        bool checksum_error = !check_tree_checksum(Vcb, th);

        if (checksum_error || addr != th->address || (generation != 0 && generation != th->generation)) {
            uint64_t off;
            uint16_t stripe;

            get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes, &off, &stripe);

            ERR("unrecoverable checksum error at %I64x, device %I64x\n", addr, devices[stripe]->devitem.dev_id);

            if (checksum_error) {
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                return STATUS_CRC_ERROR;
            } else if (addr != th->address) {
                WARN("address of tree was %I64x, not %I64x as expected\n", th->address, addr);
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                return STATUS_CRC_ERROR;
            } else if (generation != 0 && generation != th->generation) {
                WARN("generation of tree was %I64x, not %I64x as expected\n", th->generation, generation);
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
                return STATUS_CRC_ERROR;
            }
        }
    } else if (context->csum) {
        NTSTATUS Status;

        Status = check_csum(Vcb, buf, length >> Vcb->sector_shift, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            void* ptr = context->csum;

            for (uint32_t i = 0; i < length >> Vcb->sector_shift; i++) {
                if (!check_sector_csum(Vcb, buf + (i << Vcb->sector_shift), ptr)) {
                    uint64_t off;
                    uint16_t stripe;

                    get_raid0_offset(addr - offset + ((uint64_t)i << Vcb->sector_shift), ci->stripe_length, ci->num_stripes, &off, &stripe);

                    ERR("unrecoverable checksum error at %I64x, device %I64x\n", addr, devices[stripe]->devitem.dev_id);

                    log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                    return Status;
                }

                ptr = (uint8_t*)ptr + Vcb->csum_size;
            }

            return Status;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08lx\n", Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid10(device_extension* Vcb, uint8_t* buf, uint64_t addr, uint32_t length, read_data_context* context,
                                 CHUNK_ITEM* ci, device** devices, uint64_t generation, uint64_t offset) {
    uint16_t stripe = 0;
    NTSTATUS Status;
    bool checksum_error = false;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    for (uint16_t j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08lx\n", j, context->stripes[j].iosb.Status);
            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success)
            stripe = j;
    }

    if (context->tree) {
        tree_header* th = (tree_header*)buf;

        if (!check_tree_checksum(Vcb, th)) {
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (addr != th->address) {
            WARN("address of tree was %I64x, not %I64x as expected\n", th->address, addr);
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            WARN("generation of tree was %I64x, not %I64x as expected\n", th->generation, generation);
            checksum_error = true;
            log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
        Status = check_csum(Vcb, buf, length >> Vcb->sector_shift, context->csum);

        if (Status == STATUS_CRC_ERROR)
            checksum_error = true;
        else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08lx\n", Status);
            return Status;
        }
    }

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        tree_header* t2;
        uint64_t off;
        uint16_t badsubstripe = 0;
        bool recovered = false;

        t2 = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!t2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &off, &stripe);

        stripe *= ci->sub_stripes;

        for (uint16_t j = 0; j < ci->sub_stripes; j++) {
            if (context->stripes[stripe + j].status == ReadDataStatus_Success) {
                badsubstripe = j;
                break;
            }
        }

        for (uint16_t j = 0; j < ci->sub_stripes; j++) {
            if (context->stripes[stripe + j].status != ReadDataStatus_Success && devices[stripe + j] && devices[stripe + j]->devobj) {
                Status = sync_read_phys(devices[stripe + j]->devobj, devices[stripe + j]->fileobj, cis[stripe + j].offset + off,
                                        Vcb->superblock.node_size, (uint8_t*)t2, false);
                if (!NT_SUCCESS(Status)) {
                    WARN("sync_read_phys returned %08lx\n", Status);
                    log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_READ_ERRORS);
                } else {
                    bool checksum_error = !check_tree_checksum(Vcb, t2);

                    if (t2->address == addr && !checksum_error && (generation == 0 || t2->generation == generation)) {
                        RtlCopyMemory(buf, t2, Vcb->superblock.node_size);
                        ERR("recovering from checksum error at %I64x, device %I64x\n", addr, devices[stripe + j]->devitem.dev_id);
                        recovered = true;

                        if (!Vcb->readonly && !devices[stripe + badsubstripe]->readonly && devices[stripe + badsubstripe]->devobj) { // write good data over bad
                            Status = write_data_phys(devices[stripe + badsubstripe]->devobj, devices[stripe + badsubstripe]->fileobj,
                                                     cis[stripe + badsubstripe].offset + off, t2, Vcb->superblock.node_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08lx\n", Status);
                                log_device_error(Vcb, devices[stripe + badsubstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }

                        break;
                    } else if (t2->address != addr || checksum_error)
                        log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
                    else
                        log_device_error(Vcb, devices[stripe + j], BTRFS_DEV_STAT_GENERATION_ERRORS);
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %I64x\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = length >> Vcb->sector_shift;
        uint8_t* sector;
        void* ptr = context->csum;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (ULONG i = 0; i < sectors; i++) {
            if (!check_sector_csum(Vcb, buf + (i << Vcb->sector_shift), ptr)) {
                uint64_t off;
                uint16_t stripe2, badsubstripe = 0;
                bool recovered = false;

                get_raid0_offset(addr - offset + ((uint64_t)i << Vcb->sector_shift), ci->stripe_length,
                                 ci->num_stripes / ci->sub_stripes, &off, &stripe2);

                stripe2 *= ci->sub_stripes;

                for (uint16_t j = 0; j < ci->sub_stripes; j++) {
                    if (context->stripes[stripe2 + j].status == ReadDataStatus_Success) {
                        badsubstripe = j;
                        break;
                    }
                }

                log_device_error(Vcb, devices[stripe2 + badsubstripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                for (uint16_t j = 0; j < ci->sub_stripes; j++) {
                    if (context->stripes[stripe2 + j].status != ReadDataStatus_Success && devices[stripe2 + j] && devices[stripe2 + j]->devobj) {
                        Status = sync_read_phys(devices[stripe2 + j]->devobj, devices[stripe2 + j]->fileobj, cis[stripe2 + j].offset + off,
                                                Vcb->superblock.sector_size, sector, false);
                        if (!NT_SUCCESS(Status)) {
                            WARN("sync_read_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[stripe2 + j], BTRFS_DEV_STAT_READ_ERRORS);
                        } else {
                            if (check_sector_csum(Vcb, sector, ptr)) {
                                RtlCopyMemory(buf + (i << Vcb->sector_shift), sector, Vcb->superblock.sector_size);
                                ERR("recovering from checksum error at %I64x, device %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift), devices[stripe2 + j]->devitem.dev_id);
                                recovered = true;

                                if (!Vcb->readonly && !devices[stripe2 + badsubstripe]->readonly && devices[stripe2 + badsubstripe]->devobj) { // write good data over bad
                                    Status = write_data_phys(devices[stripe2 + badsubstripe]->devobj, devices[stripe2 + badsubstripe]->fileobj,
                                                             cis[stripe2 + badsubstripe].offset + off, sector, Vcb->superblock.sector_size);
                                    if (!NT_SUCCESS(Status)) {
                                        WARN("write_data_phys returned %08lx\n", Status);
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
                    ERR("unrecoverable checksum error at %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }

            ptr = (uint8_t*)ptr + Vcb->csum_size;
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid5(device_extension* Vcb, uint8_t* buf, uint64_t addr, uint32_t length, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, uint64_t offset, uint64_t generation, chunk* c, bool degraded) {
    NTSTATUS Status;
    bool checksum_error = false;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    uint16_t j, stripe = 0;
    bool no_success = true;

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08lx\n", j, context->stripes[j].iosb.Status);
            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success) {
            stripe = j;
            no_success = false;
        }
    }

    if (c) {    // check partial stripes
        LIST_ENTRY* le;
        uint64_t ps_length = (ci->num_stripes - 1) * ci->stripe_length;

        ExAcquireResourceSharedLite(&c->partial_stripes_lock, true);

        le = c->partial_stripes.Flink;
        while (le != &c->partial_stripes) {
            partial_stripe* ps = CONTAINING_RECORD(le, partial_stripe, list_entry);

            if (ps->address + ps_length > addr && ps->address < addr + length) {
                ULONG runlength, index;

                runlength = RtlFindFirstRunClear(&ps->bmp, &index);

                while (runlength != 0) {
                    if (index >= ps->bmplen)
                        break;

                    if (index + runlength >= ps->bmplen) {
                        runlength = ps->bmplen - index;

                        if (runlength == 0)
                            break;
                    }

                    uint64_t runstart = ps->address + (index << Vcb->sector_shift);
                    uint64_t runend = runstart + (runlength << Vcb->sector_shift);
                    uint64_t start = max(runstart, addr);
                    uint64_t end = min(runend, addr + length);

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

        if (addr != th->address || !check_tree_checksum(Vcb, th)) {
            checksum_error = true;
            if (!no_success && !degraded)
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            checksum_error = true;
            if (!no_success && !degraded)
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
        Status = check_csum(Vcb, buf, length >> Vcb->sector_shift, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            if (!degraded)
                WARN("checksum error\n");
            checksum_error = true;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08lx\n", Status);
            return Status;
        }
    } else if (degraded)
        checksum_error = true;

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        uint16_t parity;
        uint64_t off;
        bool recovered = false, first = true, failed = false;
        uint8_t* t2;

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
                        Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.node_size, t2, false);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                            failed = true;
                            break;
                        }

                        first = false;
                    } else {
                        Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.node_size, t2 + Vcb->superblock.node_size, false);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                            failed = true;
                            break;
                        }

                        do_xor(t2, t2 + Vcb->superblock.node_size, Vcb->superblock.node_size);
                    }
                } else {
                    failed = true;
                    break;
                }
            }
        }

        if (!failed) {
            tree_header* t3 = (tree_header*)t2;

            if (t3->address == addr && check_tree_checksum(Vcb, t3) && (generation == 0 || t3->generation == generation)) {
                RtlCopyMemory(buf, t2, Vcb->superblock.node_size);

                if (!degraded)
                    ERR("recovering from checksum error at %I64x, device %I64x\n", addr, devices[stripe]->devitem.dev_id);

                recovered = true;

                if (!Vcb->readonly && devices[stripe] && !devices[stripe]->readonly && devices[stripe]->devobj) { // write good data over bad
                    Status = write_data_phys(devices[stripe]->devobj, devices[stripe]->fileobj, cis[stripe].offset + off, t2, Vcb->superblock.node_size);
                    if (!NT_SUCCESS(Status)) {
                        WARN("write_data_phys returned %08lx\n", Status);
                        log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                    }
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %I64x\n", addr);
            ExFreePool(t2);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(t2);
    } else {
        ULONG sectors = length >> Vcb->sector_shift;
        uint8_t* sector;
        void* ptr = context->csum;

        sector = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.sector_size * 2, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (ULONG i = 0; i < sectors; i++) {
            uint16_t parity;
            uint64_t off;

            get_raid0_offset(addr - offset + ((uint64_t)i << Vcb->sector_shift), ci->stripe_length,
                             ci->num_stripes - 1, &off, &stripe);

            parity = (((addr - offset + ((uint64_t)i << Vcb->sector_shift)) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            stripe = (parity + stripe + 1) % ci->num_stripes;

            if (!devices[stripe] || !devices[stripe]->devobj || (ptr && !check_sector_csum(Vcb, buf + (i << Vcb->sector_shift), ptr))) {
                bool recovered = false, first = true, failed = false;

                if (devices[stripe] && devices[stripe]->devobj)
                    log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_READ_ERRORS);

                for (j = 0; j < ci->num_stripes; j++) {
                    if (j != stripe) {
                        if (devices[j] && devices[j]->devobj) {
                            if (first) {
                                Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.sector_size, sector, false);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("sync_read_phys returned %08lx\n", Status);
                                    failed = true;
                                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                    break;
                                }

                                first = false;
                            } else {
                                Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.sector_size,
                                                        sector + Vcb->superblock.sector_size, false);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("sync_read_phys returned %08lx\n", Status);
                                    failed = true;
                                    log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                    break;
                                }

                                do_xor(sector, sector + Vcb->superblock.sector_size, Vcb->superblock.sector_size);
                            }
                        } else {
                            failed = true;
                            break;
                        }
                    }
                }

                if (!failed) {
                    if (!ptr || check_sector_csum(Vcb, sector, ptr)) {
                        RtlCopyMemory(buf + (i << Vcb->sector_shift), sector, Vcb->superblock.sector_size);

                        if (!degraded)
                            ERR("recovering from checksum error at %I64x, device %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift), devices[stripe]->devitem.dev_id);

                        recovered = true;

                        if (!Vcb->readonly && devices[stripe] && !devices[stripe]->readonly && devices[stripe]->devobj) { // write good data over bad
                            Status = write_data_phys(devices[stripe]->devobj, devices[stripe]->fileobj, cis[stripe].offset + off,
                                                     sector, Vcb->superblock.sector_size);
                            if (!NT_SUCCESS(Status)) {
                                WARN("write_data_phys returned %08lx\n", Status);
                                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                            }
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }

            if (ptr)
                ptr = (uint8_t*)ptr + Vcb->csum_size;
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

void raid6_recover2(uint8_t* sectors, uint16_t num_stripes, ULONG sector_size, uint16_t missing1, uint16_t missing2, uint8_t* out) {
    if (missing1 == num_stripes - 2 || missing2 == num_stripes - 2) { // reconstruct from q and data
        uint16_t missing = missing1 == (num_stripes - 2) ? missing2 : missing1;
        uint16_t stripe;

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
            galois_divpower(out, (uint8_t)missing, sector_size);
    } else { // reconstruct from p and q
        uint16_t x = missing1, y = missing2, stripe;
        uint8_t gyx, gx, denom, a, b, *p, *q, *pxy, *qxy;
        uint32_t j;

        stripe = num_stripes - 3;

        pxy = out + sector_size;
        qxy = out;

        if (stripe == missing1 || stripe == missing2) {
            RtlZeroMemory(qxy, sector_size);
            RtlZeroMemory(pxy, sector_size);
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
            }
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

static NTSTATUS read_data_raid6(device_extension* Vcb, uint8_t* buf, uint64_t addr, uint32_t length, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, uint64_t offset, uint64_t generation, chunk* c, bool degraded) {
    NTSTATUS Status;
    bool checksum_error = false;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
    uint16_t stripe = 0, j;
    bool no_success = true;

    for (j = 0; j < ci->num_stripes; j++) {
        if (context->stripes[j].status == ReadDataStatus_Error) {
            WARN("stripe %u returned error %08lx\n", j, context->stripes[j].iosb.Status);

            if (devices[j])
                log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
            return context->stripes[j].iosb.Status;
        } else if (context->stripes[j].status == ReadDataStatus_Success) {
            stripe = j;
            no_success = false;
        }
    }

    if (c) {    // check partial stripes
        LIST_ENTRY* le;
        uint64_t ps_length = (ci->num_stripes - 2) * ci->stripe_length;

        ExAcquireResourceSharedLite(&c->partial_stripes_lock, true);

        le = c->partial_stripes.Flink;
        while (le != &c->partial_stripes) {
            partial_stripe* ps = CONTAINING_RECORD(le, partial_stripe, list_entry);

            if (ps->address + ps_length > addr && ps->address < addr + length) {
                ULONG runlength, index;

                runlength = RtlFindFirstRunClear(&ps->bmp, &index);

                while (runlength != 0) {
                    if (index >= ps->bmplen)
                        break;

                    if (index + runlength >= ps->bmplen) {
                        runlength = ps->bmplen - index;

                        if (runlength == 0)
                            break;
                    }

                    uint64_t runstart = ps->address + (index << Vcb->sector_shift);
                    uint64_t runend = runstart + (runlength << Vcb->sector_shift);
                    uint64_t start = max(runstart, addr);
                    uint64_t end = min(runend, addr + length);

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

        if (addr != th->address || !check_tree_checksum(Vcb, th)) {
            checksum_error = true;
            if (!no_success && !degraded && devices[stripe])
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_CORRUPTION_ERRORS);
        } else if (generation != 0 && generation != th->generation) {
            checksum_error = true;
            if (!no_success && !degraded && devices[stripe])
                log_device_error(Vcb, devices[stripe], BTRFS_DEV_STAT_GENERATION_ERRORS);
        }
    } else if (context->csum) {
        Status = check_csum(Vcb, buf, length >> Vcb->sector_shift, context->csum);

        if (Status == STATUS_CRC_ERROR) {
            if (!degraded)
                WARN("checksum error\n");
            checksum_error = true;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08lx\n", Status);
            return Status;
        }
    } else if (degraded)
        checksum_error = true;

    if (!checksum_error)
        return STATUS_SUCCESS;

    if (context->tree) {
        uint8_t* sector;
        uint16_t k, physstripe, parity1, parity2, error_stripe = 0;
        uint64_t off;
        bool recovered = false, failed = false;
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
                    Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.node_size,
                                            sector + (k * Vcb->superblock.node_size), false);
                    if (!NT_SUCCESS(Status)) {
                        ERR("sync_read_phys returned %08lx\n", Status);
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                        num_errors++;
                        error_stripe = k;

                        if (num_errors > 1) {
                            failed = true;
                            break;
                        }
                    }
                } else {
                    num_errors++;
                    error_stripe = k;

                    if (num_errors > 1) {
                        failed = true;
                        break;
                    }
                }
            }

            j = (j + 1) % ci->num_stripes;
        }

        if (!failed) {
            if (num_errors == 0) {
                tree_header* th = (tree_header*)(sector + (stripe * Vcb->superblock.node_size));

                RtlCopyMemory(sector + (stripe * Vcb->superblock.node_size), sector + ((ci->num_stripes - 2) * Vcb->superblock.node_size),
                              Vcb->superblock.node_size);

                for (j = 0; j < ci->num_stripes - 2; j++) {
                    if (j != stripe)
                        do_xor(sector + (stripe * Vcb->superblock.node_size), sector + (j * Vcb->superblock.node_size), Vcb->superblock.node_size);
                }

                if (th->address == addr && check_tree_checksum(Vcb, th) && (generation == 0 || th->generation == generation)) {
                    RtlCopyMemory(buf, sector + (stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);

                    if (devices[physstripe] && devices[physstripe]->devobj)
                        ERR("recovering from checksum error at %I64x, device %I64x\n", addr, devices[physstripe]->devitem.dev_id);

                    recovered = true;

                    if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[physstripe]->devobj, devices[physstripe]->fileobj, cis[physstripe].offset + off,
                                                 sector + (stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }
                }
            }

            if (!recovered) {
                tree_header* th = (tree_header*)(sector + (ci->num_stripes * Vcb->superblock.node_size));
                bool read_q = false;

                if (devices[parity2] && devices[parity2]->devobj) {
                    Status = sync_read_phys(devices[parity2]->devobj, devices[parity2]->fileobj, cis[parity2].offset + off,
                                            Vcb->superblock.node_size, sector + ((ci->num_stripes - 1) * Vcb->superblock.node_size), false);
                    if (!NT_SUCCESS(Status)) {
                        ERR("sync_read_phys returned %08lx\n", Status);
                        log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                    } else
                        read_q = true;
                }

                if (read_q) {
                    if (num_errors == 1) {
                        raid6_recover2(sector, ci->num_stripes, Vcb->superblock.node_size, stripe, error_stripe, sector + (ci->num_stripes * Vcb->superblock.node_size));

                        if (th->address == addr && check_tree_checksum(Vcb, th) && (generation == 0 || th->generation == generation))
                            recovered = true;
                    } else {
                        for (j = 0; j < ci->num_stripes - 1; j++) {
                            if (j != stripe) {
                                raid6_recover2(sector, ci->num_stripes, Vcb->superblock.node_size, stripe, j, sector + (ci->num_stripes * Vcb->superblock.node_size));

                                if (th->address == addr && check_tree_checksum(Vcb, th) && (generation == 0 || th->generation == generation)) {
                                    recovered = true;
                                    error_stripe = j;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (recovered) {
                    uint16_t error_stripe_phys = (parity2 + error_stripe + 1) % ci->num_stripes;

                    if (devices[physstripe] && devices[physstripe]->devobj)
                        ERR("recovering from checksum error at %I64x, device %I64x\n", addr, devices[physstripe]->devitem.dev_id);

                    RtlCopyMemory(buf, sector + (ci->num_stripes * Vcb->superblock.node_size), Vcb->superblock.node_size);

                    if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[physstripe]->devobj, devices[physstripe]->fileobj, cis[physstripe].offset + off,
                                                 sector + (ci->num_stripes * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }

                    if (devices[error_stripe_phys] && devices[error_stripe_phys]->devobj) {
                        if (error_stripe == ci->num_stripes - 2) {
                            ERR("recovering from parity error at %I64x, device %I64x\n", addr, devices[error_stripe_phys]->devitem.dev_id);

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
                            ERR("recovering from checksum error at %I64x, device %I64x\n", addr + ((error_stripe - stripe) * ci->stripe_length),
                                devices[error_stripe_phys]->devitem.dev_id);

                            log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                            RtlCopyMemory(sector + (error_stripe * Vcb->superblock.node_size),
                                          sector + ((ci->num_stripes + 1) * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        }
                    }

                    if (!Vcb->readonly && devices[error_stripe_phys] && devices[error_stripe_phys]->devobj && !devices[error_stripe_phys]->readonly) { // write good data over bad
                        Status = write_data_phys(devices[error_stripe_phys]->devobj, devices[error_stripe_phys]->fileobj, cis[error_stripe_phys].offset + off,
                                                 sector + (error_stripe * Vcb->superblock.node_size), Vcb->superblock.node_size);
                        if (!NT_SUCCESS(Status)) {
                            WARN("write_data_phys returned %08lx\n", Status);
                            log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_WRITE_ERRORS);
                        }
                    }
                }
            }
        }

        if (!recovered) {
            ERR("unrecoverable checksum error at %I64x\n", addr);
            ExFreePool(sector);
            return STATUS_CRC_ERROR;
        }

        ExFreePool(sector);
    } else {
        ULONG sectors = length >> Vcb->sector_shift;
        uint8_t* sector;
        void* ptr = context->csum;

        sector = ExAllocatePoolWithTag(NonPagedPool, (ci->num_stripes + 2) << Vcb->sector_shift, ALLOC_TAG);
        if (!sector) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (ULONG i = 0; i < sectors; i++) {
            uint64_t off;
            uint16_t physstripe, parity1, parity2;

            get_raid0_offset(addr - offset + ((uint64_t)i << Vcb->sector_shift), ci->stripe_length,
                             ci->num_stripes - 2, &off, &stripe);

            parity1 = (((addr - offset + ((uint64_t)i << Vcb->sector_shift)) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
            parity2 = (parity1 + 1) % ci->num_stripes;

            physstripe = (parity2 + stripe + 1) % ci->num_stripes;

            if (!devices[physstripe] || !devices[physstripe]->devobj || (context->csum && !check_sector_csum(Vcb, buf + (i << Vcb->sector_shift), ptr))) {
                uint16_t error_stripe = 0;
                bool recovered = false, failed = false;
                ULONG num_errors = 0;

                if (devices[physstripe] && devices[physstripe]->devobj)
                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_READ_ERRORS);

                j = (parity2 + 1) % ci->num_stripes;

                for (uint16_t k = 0; k < ci->num_stripes - 1; k++) {
                    if (j != physstripe) {
                        if (devices[j] && devices[j]->devobj) {
                            Status = sync_read_phys(devices[j]->devobj, devices[j]->fileobj, cis[j].offset + off, Vcb->superblock.sector_size,
                                                    sector + ((ULONG)k << Vcb->sector_shift), false);
                            if (!NT_SUCCESS(Status)) {
                                ERR("sync_read_phys returned %08lx\n", Status);
                                log_device_error(Vcb, devices[j], BTRFS_DEV_STAT_READ_ERRORS);
                                num_errors++;
                                error_stripe = k;

                                if (num_errors > 1) {
                                    failed = true;
                                    break;
                                }
                            }
                        } else {
                            num_errors++;
                            error_stripe = k;

                            if (num_errors > 1) {
                                failed = true;
                                break;
                            }
                        }
                    }

                    j = (j + 1) % ci->num_stripes;
                }

                if (!failed) {
                    if (num_errors == 0) {
                        RtlCopyMemory(sector + ((unsigned int)stripe << Vcb->sector_shift), sector + ((unsigned int)(ci->num_stripes - 2) << Vcb->sector_shift), Vcb->superblock.sector_size);

                        for (j = 0; j < ci->num_stripes - 2; j++) {
                            if (j != stripe)
                                do_xor(sector + ((unsigned int)stripe << Vcb->sector_shift), sector + ((unsigned int)j << Vcb->sector_shift), Vcb->superblock.sector_size);
                        }

                        if (!ptr || check_sector_csum(Vcb, sector + ((unsigned int)stripe << Vcb->sector_shift), ptr)) {
                            RtlCopyMemory(buf + (i << Vcb->sector_shift), sector + ((unsigned int)stripe << Vcb->sector_shift), Vcb->superblock.sector_size);

                            if (devices[physstripe] && devices[physstripe]->devobj)
                                ERR("recovering from checksum error at %I64x, device %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift),
                                    devices[physstripe]->devitem.dev_id);

                            recovered = true;

                            if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[physstripe]->devobj, devices[physstripe]->fileobj, cis[physstripe].offset + off,
                                                         sector + ((unsigned int)stripe << Vcb->sector_shift), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08lx\n", Status);
                                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }
                        }
                    }

                    if (!recovered) {
                        bool read_q = false;

                        if (devices[parity2] && devices[parity2]->devobj) {
                            Status = sync_read_phys(devices[parity2]->devobj, devices[parity2]->fileobj, cis[parity2].offset + off,
                                                    Vcb->superblock.sector_size, sector + ((unsigned int)(ci->num_stripes - 1) << Vcb->sector_shift), false);
                            if (!NT_SUCCESS(Status)) {
                                ERR("sync_read_phys returned %08lx\n", Status);
                                log_device_error(Vcb, devices[parity2], BTRFS_DEV_STAT_READ_ERRORS);
                            } else
                                read_q = true;
                        }

                        if (read_q) {
                            if (num_errors == 1) {
                                raid6_recover2(sector, ci->num_stripes, Vcb->superblock.sector_size, stripe, error_stripe, sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift));

                                if (!devices[physstripe] || !devices[physstripe]->devobj)
                                    recovered = true;
                                else
                                    recovered = check_sector_csum(Vcb, sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift), ptr);
                            } else {
                                for (j = 0; j < ci->num_stripes - 1; j++) {
                                    if (j != stripe) {
                                        raid6_recover2(sector, ci->num_stripes, Vcb->superblock.sector_size, stripe, j, sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift));

                                        if (check_sector_csum(Vcb, sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift), ptr)) {
                                            recovered = true;
                                            error_stripe = j;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (recovered) {
                            uint16_t error_stripe_phys = (parity2 + error_stripe + 1) % ci->num_stripes;

                            if (devices[physstripe] && devices[physstripe]->devobj)
                                ERR("recovering from checksum error at %I64x, device %I64x\n",
                                    addr + ((uint64_t)i << Vcb->sector_shift), devices[physstripe]->devitem.dev_id);

                            RtlCopyMemory(buf + (i << Vcb->sector_shift), sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift), Vcb->superblock.sector_size);

                            if (!Vcb->readonly && devices[physstripe] && devices[physstripe]->devobj && !devices[physstripe]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[physstripe]->devobj, devices[physstripe]->fileobj, cis[physstripe].offset + off,
                                                         sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08lx\n", Status);
                                    log_device_error(Vcb, devices[physstripe], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }

                            if (devices[error_stripe_phys] && devices[error_stripe_phys]->devobj) {
                                if (error_stripe == ci->num_stripes - 2) {
                                    ERR("recovering from parity error at %I64x, device %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift),
                                        devices[error_stripe_phys]->devitem.dev_id);

                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                                    RtlZeroMemory(sector + ((unsigned int)(ci->num_stripes - 2) << Vcb->sector_shift), Vcb->superblock.sector_size);

                                    for (j = 0; j < ci->num_stripes - 2; j++) {
                                        if (j == stripe) {
                                            do_xor(sector + ((unsigned int)(ci->num_stripes - 2) << Vcb->sector_shift), sector + ((unsigned int)ci->num_stripes << Vcb->sector_shift),
                                                   Vcb->superblock.sector_size);
                                        } else {
                                            do_xor(sector + ((unsigned int)(ci->num_stripes - 2) << Vcb->sector_shift), sector + ((unsigned int)j << Vcb->sector_shift),
                                                   Vcb->superblock.sector_size);
                                        }
                                    }
                                } else {
                                    ERR("recovering from checksum error at %I64x, device %I64x\n",
                                        addr + ((uint64_t)i << Vcb->sector_shift) + ((error_stripe - stripe) * ci->stripe_length),
                                        devices[error_stripe_phys]->devitem.dev_id);

                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_CORRUPTION_ERRORS);

                                    RtlCopyMemory(sector + ((unsigned int)error_stripe << Vcb->sector_shift),
                                                  sector + ((unsigned int)(ci->num_stripes + 1) << Vcb->sector_shift), Vcb->superblock.sector_size);
                                }
                            }

                            if (!Vcb->readonly && devices[error_stripe_phys] && devices[error_stripe_phys]->devobj && !devices[error_stripe_phys]->readonly) { // write good data over bad
                                Status = write_data_phys(devices[error_stripe_phys]->devobj, devices[error_stripe_phys]->fileobj, cis[error_stripe_phys].offset + off,
                                                         sector + ((unsigned int)error_stripe << Vcb->sector_shift), Vcb->superblock.sector_size);
                                if (!NT_SUCCESS(Status)) {
                                    WARN("write_data_phys returned %08lx\n", Status);
                                    log_device_error(Vcb, devices[error_stripe_phys], BTRFS_DEV_STAT_WRITE_ERRORS);
                                }
                            }
                        }
                    }
                }

                if (!recovered) {
                    ERR("unrecoverable checksum error at %I64x\n", addr + ((uint64_t)i << Vcb->sector_shift));
                    ExFreePool(sector);
                    return STATUS_CRC_ERROR;
                }
            }

            if (ptr)
                ptr = (uint8_t*)ptr + Vcb->csum_size;
        }

        ExFreePool(sector);
    }

    return STATUS_SUCCESS;
}

NTSTATUS read_data(_In_ device_extension* Vcb, _In_ uint64_t addr, _In_ uint32_t length, _In_reads_bytes_opt_(length*sizeof(uint32_t)/Vcb->superblock.sector_size) void* csum,
                   _In_ bool is_tree, _Out_writes_bytes_(length) uint8_t* buf, _In_opt_ chunk* c, _Out_opt_ chunk** pc, _In_opt_ PIRP Irp, _In_ uint64_t generation, _In_ bool file_read,
                   _In_ ULONG priority) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_data_context context;
    uint64_t type, offset, total_reading = 0;
    NTSTATUS Status;
    device** devices = NULL;
    uint16_t i, startoffstripe, allowed_missing, missing_devices = 0;
    uint8_t* dummypage = NULL;
    PMDL dummy_mdl = NULL;
    bool need_to_wait;
    uint64_t lockaddr, locklen;

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

                    devices = ExAllocatePoolWithTag(NonPagedPool, sizeof(device*) * ci->num_stripes, ALLOC_TAG);
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
            ERR("could not find chunk for %I64x in bootstrap\n", addr);
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
    } else if (ci->type & BLOCK_FLAG_RAID1C3) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 2;
    } else if (ci->type & BLOCK_FLAG_RAID1C4) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 3;
    } else { // SINGLE
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 0;
    }

    cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    RtlZeroMemory(&context, sizeof(read_data_context));
    KeInitializeEvent(&context.Event, NotificationEvent, false);

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
        uint64_t startoff, endoff;
        uint16_t endoffstripe, stripe;
        uint32_t *stripeoff, pos;
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

        master_mdl = IoAllocateMdl(context.va, length, false, false, NULL);
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
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
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
                context.stripes[i].mdl = IoAllocateMdl(context.va, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart), false, false, NULL);

                if (!context.stripes[i].mdl) {
                    ERR("IoAllocateMdl failed\n");
                    MmUnlockPages(master_mdl);
                    IoFreeMdl(master_mdl);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
        }

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(uint32_t) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(uint32_t) * ci->num_stripes);

        pos = 0;
        stripe = startoffstripe;
        while (pos < length) {
            PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

            if (pos == 0) {
                uint32_t readlen = (uint32_t)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length));

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] += readlen;
                pos += readlen;
            } else if (length - pos < ci->stripe_length) {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (length - pos) * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                pos = length;
            } else {
                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                stripeoff[stripe] += (uint32_t)ci->stripe_length;
                pos += (uint32_t)ci->stripe_length;
            }

            stripe = (stripe + 1) % ci->num_stripes;
        }

        MmUnlockPages(master_mdl);
        IoFreeMdl(master_mdl);

        ExFreePool(stripeoff);
    } else if (type == BLOCK_FLAG_RAID10) {
        uint64_t startoff, endoff;
        uint16_t endoffstripe, j, stripe;
        ULONG orig_ls;
        PMDL master_mdl;
        PFN_NUMBER* pfns;
        uint32_t* stripeoff, pos;
        read_data_stripe** stripes;

        if (c)
            orig_ls = c->last_stripe;
        else
            orig_ls = 0;

        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &endoff, &endoffstripe);

        if ((ci->num_stripes % ci->sub_stripes) != 0) {
            ERR("chunk %I64x: num_stripes %x was not a multiple of sub_stripes %x!\n", offset, ci->num_stripes, ci->sub_stripes);
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

        context.firstoff = (uint16_t)((startoff % ci->stripe_length) >> Vcb->sector_shift);
        context.startoffstripe = startoffstripe;
        context.sectors_per_stripe = (uint16_t)(ci->stripe_length >> Vcb->sector_shift);

        startoffstripe *= ci->sub_stripes;
        endoffstripe *= ci->sub_stripes;

        if (c)
            c->last_stripe = (orig_ls + 1) % ci->sub_stripes;

        master_mdl = IoAllocateMdl(context.va, length, false, false, NULL);
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
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
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
            uint64_t sstart, send;
            bool stripeset = false;

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
                        context.stripes[i+j].mdl = IoAllocateMdl(context.va, (ULONG)(send - sstart), false, false, NULL);

                        if (!context.stripes[i+j].mdl) {
                            ERR("IoAllocateMdl failed\n");
                            MmUnlockPages(master_mdl);
                            IoFreeMdl(master_mdl);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                    }

                    stripeset = true;
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
                            context.stripes[i+j].mdl = IoAllocateMdl(context.va, (ULONG)(send - sstart), false, false, NULL);

                            if (!context.stripes[i+j].mdl) {
                                ERR("IoAllocateMdl failed\n");
                                MmUnlockPages(master_mdl);
                                IoFreeMdl(master_mdl);
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                goto exit;
                            }
                        }

                        stripeset = true;
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

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(uint32_t) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(uint32_t) * ci->num_stripes / ci->sub_stripes);

        pos = 0;
        stripe = startoffstripe / ci->sub_stripes;
        while (pos < length) {
            PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(stripes[stripe]->mdl + 1);

            if (pos == 0) {
                uint32_t readlen = (uint32_t)min(stripes[stripe]->stripeend - stripes[stripe]->stripestart,
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
        uint64_t orig_ls;

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

            context.stripes[i].mdl = IoAllocateMdl(context.va, length, false, false, NULL);
            if (!context.stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            MmBuildMdlForNonPagedPool(context.stripes[i].mdl);
        } else {
            context.stripes[i].mdl = IoAllocateMdl(buf, length, false, false, NULL);

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
                ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
                goto exit;
            }
        }
    } else if (type == BLOCK_FLAG_RAID5) {
        uint64_t startoff, endoff;
        uint16_t endoffstripe, parity;
        uint32_t *stripeoff, pos;
        PMDL master_mdl;
        PFN_NUMBER *pfns, dummy = 0;
        bool need_dummy = false;

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

        master_mdl = IoAllocateMdl(context.va, length, false, false, NULL);
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
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        pos = 0;
        while (pos < length) {
            parity = (((addr - offset + pos) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            if (pos == 0) {
                uint16_t stripe = (parity + startoffstripe + 1) % ci->num_stripes;
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
                    uint16_t stripe2 = (parity + i + 1) % ci->num_stripes;

                    context.stripes[stripe2].stripestart = context.stripes[stripe2].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
                }

                context.stripes[parity].stripestart = context.stripes[parity].stripeend = startoff - (startoff % ci->stripe_length) + ci->stripe_length;

                if (length - pos > ci->num_stripes * (ci->num_stripes - 1) * ci->stripe_length) {
                    skip = (ULONG)(((length - pos) / (ci->num_stripes * (ci->num_stripes - 1) * ci->stripe_length)) - 1);

                    for (i = 0; i < ci->num_stripes; i++) {
                        context.stripes[i].stripeend += skip * ci->num_stripes * ci->stripe_length;
                    }

                    pos += (uint32_t)(skip * (ci->num_stripes - 1) * ci->num_stripes * ci->stripe_length);
                    need_dummy = true;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 1)) {
                for (i = 0; i < ci->num_stripes; i++) {
                    context.stripes[i].stripeend += ci->stripe_length;
                }

                pos += (uint32_t)(ci->stripe_length * (ci->num_stripes - 1));
                need_dummy = true;
            } else {
                uint16_t stripe = (parity + 1) % ci->num_stripes;

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
                                                       false, false, NULL);

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

            dummy_mdl = IoAllocateMdl(dummypage, PAGE_SIZE, false, false, NULL);
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

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(uint32_t) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(uint32_t) * ci->num_stripes);

        pos = 0;

        while (pos < length) {
            PFN_NUMBER* stripe_pfns;

            parity = (((addr - offset + pos) / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;

            if (pos == 0) {
                uint16_t stripe = (parity + startoffstripe + 1) % ci->num_stripes;
                uint32_t readlen = min(length - pos, (uint32_t)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart,
                                                       ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length)));

                stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] = readlen;
                pos += readlen;

                stripe = (stripe + 1) % ci->num_stripes;

                while (stripe != parity) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = min(length - pos, (uint32_t)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] = readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 1)) {
                uint16_t stripe = (parity + 1) % ci->num_stripes;
                ULONG k;

                while (stripe != parity) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                    stripeoff[stripe] += (uint32_t)ci->stripe_length;
                    pos += (uint32_t)ci->stripe_length;

                    stripe = (stripe + 1) % ci->num_stripes;
                }

                stripe_pfns = (PFN_NUMBER*)(context.stripes[parity].mdl + 1);

                for (k = 0; k < ci->stripe_length >> PAGE_SHIFT; k++) {
                    stripe_pfns[stripeoff[parity] >> PAGE_SHIFT] = dummy;
                    stripeoff[parity] += PAGE_SIZE;
                }
            } else {
                uint16_t stripe = (parity + 1) % ci->num_stripes;
                uint32_t readlen;

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
        uint64_t startoff, endoff;
        uint16_t endoffstripe, parity1;
        uint32_t *stripeoff, pos;
        PMDL master_mdl;
        PFN_NUMBER *pfns, dummy = 0;
        bool need_dummy = false;

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

        master_mdl = IoAllocateMdl(context.va, length, false, false, NULL);
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
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        pfns = (PFN_NUMBER*)(master_mdl + 1);

        pos = 0;
        while (pos < length) {
            parity1 = (((addr - offset + pos) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;

            if (pos == 0) {
                uint16_t stripe = (parity1 + startoffstripe + 2) % ci->num_stripes, parity2;
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
                    uint16_t stripe2 = (parity1 + i + 2) % ci->num_stripes;

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

                    pos += (uint32_t)(skip * (ci->num_stripes - 2) * ci->num_stripes * ci->stripe_length);
                    need_dummy = true;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 2)) {
                for (i = 0; i < ci->num_stripes; i++) {
                    context.stripes[i].stripeend += ci->stripe_length;
                }

                pos += (uint32_t)(ci->stripe_length * (ci->num_stripes - 2));
                need_dummy = true;
            } else {
                uint16_t stripe = (parity1 + 2) % ci->num_stripes;

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
                context.stripes[i].mdl = IoAllocateMdl(context.va, (ULONG)(context.stripes[i].stripeend - context.stripes[i].stripestart), false, false, NULL);

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

            dummy_mdl = IoAllocateMdl(dummypage, PAGE_SIZE, false, false, NULL);
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

        stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(uint32_t) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            MmUnlockPages(master_mdl);
            IoFreeMdl(master_mdl);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory(stripeoff, sizeof(uint32_t) * ci->num_stripes);

        pos = 0;

        while (pos < length) {
            PFN_NUMBER* stripe_pfns;

            parity1 = (((addr - offset + pos) / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;

            if (pos == 0) {
                uint16_t stripe = (parity1 + startoffstripe + 2) % ci->num_stripes;
                uint32_t readlen = min(length - pos, (uint32_t)min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart,
                                                       ci->stripe_length - (context.stripes[stripe].stripestart % ci->stripe_length)));

                stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                RtlCopyMemory(stripe_pfns, pfns, readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                stripeoff[stripe] = readlen;
                pos += readlen;

                stripe = (stripe + 1) % ci->num_stripes;

                while (stripe != parity1) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = (uint32_t)min(length - pos, min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

                    if (readlen == 0)
                        break;

                    RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], readlen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                    stripeoff[stripe] = readlen;
                    pos += readlen;

                    stripe = (stripe + 1) % ci->num_stripes;
                }
            } else if (length - pos >= ci->stripe_length * (ci->num_stripes - 2)) {
                uint16_t stripe = (parity1 + 2) % ci->num_stripes;
                uint16_t parity2 = (parity1 + 1) % ci->num_stripes;
                ULONG k;

                while (stripe != parity1) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);

                    RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(ci->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                    stripeoff[stripe] += (uint32_t)ci->stripe_length;
                    pos += (uint32_t)ci->stripe_length;

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
                uint16_t stripe = (parity1 + 2) % ci->num_stripes;
                uint32_t readlen;

                while (pos < length) {
                    stripe_pfns = (PFN_NUMBER*)(context.stripes[stripe].mdl + 1);
                    readlen = (uint32_t)min(length - pos, min(context.stripes[stripe].stripeend - context.stripes[stripe].stripestart, ci->stripe_length));

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
                context.stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, false);

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
            IrpSp->MinorFunction = IRP_MN_NORMAL;
            IrpSp->FileObject = devices[i]->fileobj;

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

            IoSetCompletionRoutine(context.stripes[i].Irp, read_data_completion, &context.stripes[i], true, true, true);

            context.stripes[i].status = ReadDataStatus_Pending;
        }
    }

    need_to_wait = false;
    for (i = 0; i < ci->num_stripes; i++) {
        if (context.stripes[i].status != ReadDataStatus_MissingDevice && context.stripes[i].status != ReadDataStatus_Skip) {
            IoCallDriver(devices[i]->devobj, context.stripes[i].Irp);
            need_to_wait = true;
        }
    }

    if (need_to_wait)
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, false, NULL);

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
            ERR("read_data_raid0 returned %08lx\n", Status);

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
            ERR("read_data_raid10 returned %08lx\n", Status);

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
            ERR("read_data_dup returned %08lx\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_RAID5) {
        Status = read_data_raid5(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, offset, generation, c, missing_devices > 0 ? true : false);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid5 returned %08lx\n", Status);

            if (file_read)
                ExFreePool(context.va);

            goto exit;
        }

        if (file_read) {
            RtlCopyMemory(buf, context.va, length);
            ExFreePool(context.va);
        }
    } else if (type == BLOCK_FLAG_RAID6) {
        Status = read_data_raid6(Vcb, file_read ? context.va : buf, addr, length, &context, ci, devices, offset, generation, c, missing_devices > 0 ? true : false);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid6 returned %08lx\n", Status);

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

__attribute__((nonnull(1, 2)))
NTSTATUS read_stream(fcb* fcb, uint8_t* data, uint64_t start, ULONG length, ULONG* pbr) {
    ULONG readlen;

    TRACE("(%p, %p, %I64x, %lx, %p)\n", fcb, data, start, length, pbr);

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
        RtlCopyMemory(data, fcb->adsdata.Buffer + start, readlen);

    if (pbr) *pbr = readlen;

    return STATUS_SUCCESS;
}

typedef struct {
    uint64_t off;
    uint64_t ed_size;
    uint64_t ed_offset;
    uint64_t ed_num_bytes;
} read_part_extent;

typedef struct {
    LIST_ENTRY list_entry;
    uint64_t addr;
    chunk* c;
    uint32_t read;
    uint32_t to_read;
    void* csum;
    bool csum_free;
    uint8_t* buf;
    bool buf_free;
    uint32_t bumpoff;
    bool mdl;
    void* data;
    uint8_t compression;
    unsigned int num_extents;
    read_part_extent extents[1];
} read_part;

typedef struct {
    LIST_ENTRY list_entry;
    calc_job* cj;
    void* decomp;
    void* data;
    unsigned int offset;
    size_t length;
} comp_calc_job;

__attribute__((nonnull(1, 2)))
NTSTATUS read_file(fcb* fcb, uint8_t* data, uint64_t start, uint64_t length, ULONG* pbr, PIRP Irp) {
    NTSTATUS Status;
    uint32_t bytes_read = 0;
    uint64_t last_end;
    LIST_ENTRY* le;
    POOL_TYPE pool_type;
    LIST_ENTRY read_parts, calc_jobs;

    TRACE("(%p, %p, %I64x, %I64x, %p)\n", fcb, data, start, length, pbr);

    if (pbr)
        *pbr = 0;

    if (start >= fcb->inode_item.st_size) {
        WARN("Tried to read beyond end of file\n");
        return STATUS_END_OF_FILE;
    }

    InitializeListHead(&read_parts);
    InitializeListHead(&calc_jobs);

    pool_type = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? NonPagedPool : PagedPool;

    le = fcb->extents.Flink;

    last_end = start;

    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext->ignore) {
            EXTENT_DATA* ed = &ext->extent_data;
            uint64_t len;

            if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC)
                len = ((EXTENT_DATA2*)ed->data)->num_bytes;
            else
                len = ed->decoded_size;

            if (ext->offset + len <= start) {
                last_end = ext->offset + len;
                goto nextitem;
            }

            if (ext->offset > last_end && ext->offset > start + bytes_read) {
                uint32_t read = (uint32_t)min(length, ext->offset - max(start, last_end));

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
                    uint64_t off = start + bytes_read - ext->offset;
                    uint32_t read;

                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        read = (uint32_t)min(min(len, ext->datalen) - off, length);

                        RtlCopyMemory(data + bytes_read, &ed->data[off], read);
                    } else if (ed->compression == BTRFS_COMPRESSION_ZLIB || ed->compression == BTRFS_COMPRESSION_LZO || ed->compression == BTRFS_COMPRESSION_ZSTD) {
                        uint8_t* decomp;
                        bool decomp_alloc;
                        uint16_t inlen = ext->datalen - (uint16_t)offsetof(EXTENT_DATA, data[0]);

                        if (ed->decoded_size == 0 || ed->decoded_size > 0xffffffff) {
                            ERR("ed->decoded_size was invalid (%I64x)\n", ed->decoded_size);
                            Status = STATUS_INTERNAL_ERROR;
                            goto exit;
                        }

                        read = (uint32_t)min(ed->decoded_size - off, length);

                        if (off > 0) {
                            decomp = ExAllocatePoolWithTag(NonPagedPool, (uint32_t)ed->decoded_size, ALLOC_TAG);
                            if (!decomp) {
                                ERR("out of memory\n");
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                goto exit;
                            }

                            decomp_alloc = true;
                        } else {
                            decomp = data + bytes_read;
                            decomp_alloc = false;
                        }

                        if (ed->compression == BTRFS_COMPRESSION_ZLIB) {
                            Status = zlib_decompress(ed->data, inlen, decomp, (uint32_t)(read + off));
                            if (!NT_SUCCESS(Status)) {
                                ERR("zlib_decompress returned %08lx\n", Status);
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_LZO) {
                            if (inlen < sizeof(uint32_t)) {
                                ERR("extent data was truncated\n");
                                Status = STATUS_INTERNAL_ERROR;
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            } else
                                inlen -= sizeof(uint32_t);

                            Status = lzo_decompress(ed->data + sizeof(uint32_t), inlen, decomp, (uint32_t)(read + off), sizeof(uint32_t));
                            if (!NT_SUCCESS(Status)) {
                                ERR("lzo_decompress returned %08lx\n", Status);
                                if (decomp_alloc) ExFreePool(decomp);
                                goto exit;
                            }
                        } else if (ed->compression == BTRFS_COMPRESSION_ZSTD) {
                            Status = zstd_decompress(ed->data, inlen, decomp, (uint32_t)(read + off));
                            if (!NT_SUCCESS(Status)) {
                                ERR("zstd_decompress returned %08lx\n", Status);
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
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                    read_part* rp;

                    rp = ExAllocatePoolWithTag(pool_type, sizeof(read_part), ALLOC_TAG);
                    if (!rp) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto exit;
                    }

                    rp->mdl = (Irp && Irp->MdlAddress) ? true : false;
                    rp->extents[0].off = start + bytes_read - ext->offset;
                    rp->bumpoff = 0;
                    rp->num_extents = 1;
                    rp->csum_free = false;

                    rp->read = (uint32_t)(len - rp->extents[0].off);
                    if (rp->read > length) rp->read = (uint32_t)length;

                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        rp->addr = ed2->address + ed2->offset + rp->extents[0].off;
                        rp->to_read = (uint32_t)sector_align(rp->read, fcb->Vcb->superblock.sector_size);

                        if (rp->addr & (fcb->Vcb->superblock.sector_size - 1)) {
                            rp->bumpoff = rp->addr & (fcb->Vcb->superblock.sector_size - 1);
                            rp->addr -= rp->bumpoff;
                            rp->to_read = (uint32_t)sector_align(rp->read + rp->bumpoff, fcb->Vcb->superblock.sector_size);
                        }
                    } else {
                        rp->addr = ed2->address;
                        rp->to_read = (uint32_t)sector_align(ed2->size, fcb->Vcb->superblock.sector_size);
                    }

                    if (ed->compression == BTRFS_COMPRESSION_NONE && (start & (fcb->Vcb->superblock.sector_size - 1)) == 0 &&
                        (length & (fcb->Vcb->superblock.sector_size - 1)) == 0) {
                        rp->buf = data + bytes_read;
                        rp->buf_free = false;
                    } else {
                        rp->buf = ExAllocatePoolWithTag(pool_type, rp->to_read, ALLOC_TAG);
                        rp->buf_free = true;

                        if (!rp->buf) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(rp);
                            goto exit;
                        }

                        rp->mdl = false;
                    }

                    rp->c = get_chunk_from_address(fcb->Vcb, rp->addr);

                    if (!rp->c) {
                        ERR("get_chunk_from_address(%I64x) failed\n", rp->addr);

                        if (rp->buf_free)
                            ExFreePool(rp->buf);

                        ExFreePool(rp);

                        Status = STATUS_INTERNAL_ERROR;
                        goto exit;
                    }

                    if (ext->csum) {
                        if (ed->compression == BTRFS_COMPRESSION_NONE) {
                            rp->csum = (uint8_t*)ext->csum + (fcb->Vcb->csum_size * (rp->extents[0].off >> fcb->Vcb->sector_shift));
                        } else
                            rp->csum = ext->csum;
                    } else
                        rp->csum = NULL;

                    rp->data = data + bytes_read;
                    rp->compression = ed->compression;
                    rp->extents[0].ed_offset = ed2->offset;
                    rp->extents[0].ed_size = ed2->size;
                    rp->extents[0].ed_num_bytes = ed2->num_bytes;

                    InsertTailList(&read_parts, &rp->list_entry);

                    bytes_read += rp->read;
                    length -= rp->read;

                    break;
                }

                case EXTENT_TYPE_PREALLOC:
                {
                    uint64_t off = start + bytes_read - ext->offset;
                    uint32_t read = (uint32_t)(len - off);

                    if (read > length) read = (uint32_t)length;

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

    if (!IsListEmpty(&read_parts) && read_parts.Flink->Flink != &read_parts) { // at least two entries in list
        read_part* last_rp = CONTAINING_RECORD(read_parts.Flink, read_part, list_entry);

        le = read_parts.Flink->Flink;
        while (le != &read_parts) {
            LIST_ENTRY* le2 = le->Flink;
            read_part* rp = CONTAINING_RECORD(le, read_part, list_entry);

            // merge together runs
            if (rp->compression != BTRFS_COMPRESSION_NONE && rp->compression == last_rp->compression && rp->addr == last_rp->addr + last_rp->to_read &&
                rp->data == (uint8_t*)last_rp->data + last_rp->read && rp->c == last_rp->c && ((rp->csum && last_rp->csum) || (!rp->csum && !last_rp->csum))) {
                read_part* rp2;

                rp2 = ExAllocatePoolWithTag(pool_type, offsetof(read_part, extents) + (sizeof(read_part_extent) * (last_rp->num_extents + 1)), ALLOC_TAG);

                rp2->addr = last_rp->addr;
                rp2->c = last_rp->c;
                rp2->read = last_rp->read + rp->read;
                rp2->to_read = last_rp->to_read + rp->to_read;
                rp2->csum_free = false;

                if (last_rp->csum) {
                    uint32_t sectors = (last_rp->to_read + rp->to_read) >> fcb->Vcb->sector_shift;

                    rp2->csum = ExAllocatePoolWithTag(pool_type, sectors * fcb->Vcb->csum_size, ALLOC_TAG);
                    if (!rp2->csum) {
                        ERR("out of memory\n");
                        ExFreePool(rp2);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto exit;
                    }

                    RtlCopyMemory(rp2->csum, last_rp->csum, (last_rp->to_read * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift);
                    RtlCopyMemory((uint8_t*)rp2->csum + ((last_rp->to_read * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift), rp->csum,
                                  (rp->to_read * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift);

                    rp2->csum_free = true;
                } else
                    rp2->csum = NULL;

                rp2->buf = ExAllocatePoolWithTag(pool_type, rp2->to_read, ALLOC_TAG);
                if (!rp2->buf) {
                    ERR("out of memory\n");

                    if (rp2->csum)
                        ExFreePool(rp2->csum);

                    ExFreePool(rp2);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                rp2->buf_free = true;
                rp2->bumpoff = 0;
                rp2->mdl = false;
                rp2->data = last_rp->data;
                rp2->compression = last_rp->compression;
                rp2->num_extents = last_rp->num_extents + 1;

                RtlCopyMemory(rp2->extents, last_rp->extents, last_rp->num_extents * sizeof(read_part_extent));
                RtlCopyMemory(&rp2->extents[last_rp->num_extents], rp->extents, sizeof(read_part_extent));

                InsertHeadList(le->Blink, &rp2->list_entry);

                if (rp->buf_free)
                    ExFreePool(rp->buf);

                if (rp->csum_free)
                    ExFreePool(rp->csum);

                RemoveEntryList(&rp->list_entry);

                ExFreePool(rp);

                if (last_rp->buf_free)
                    ExFreePool(last_rp->buf);

                if (last_rp->csum_free)
                    ExFreePool(last_rp->csum);

                RemoveEntryList(&last_rp->list_entry);

                ExFreePool(last_rp);

                last_rp = rp2;
            } else
                last_rp = rp;

            le = le2;
        }
    }

    le = read_parts.Flink;
    while (le != &read_parts) {
        read_part* rp = CONTAINING_RECORD(le, read_part, list_entry);

        Status = read_data(fcb->Vcb, rp->addr, rp->to_read, rp->csum, false, rp->buf, rp->c, NULL, Irp, 0, rp->mdl,
                           fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data returned %08lx\n", Status);
            goto exit;
        }

        if (rp->compression == BTRFS_COMPRESSION_NONE) {
            if (rp->buf_free)
                RtlCopyMemory(rp->data, rp->buf + rp->bumpoff, rp->read);
        } else {
            uint8_t* buf = rp->buf;

            for (unsigned int i = 0; i < rp->num_extents; i++) {
                uint8_t *decomp = NULL, *buf2;
                ULONG outlen, inlen, off2;
                uint32_t inpageoff = 0;
                comp_calc_job* ccj;

                off2 = (ULONG)(rp->extents[i].ed_offset + rp->extents[i].off);
                buf2 = buf;
                inlen = (ULONG)rp->extents[i].ed_size;

                if (rp->compression == BTRFS_COMPRESSION_LZO) {
                    ULONG inoff = sizeof(uint32_t);

                    inlen -= sizeof(uint32_t);

                    // If reading a few sectors in, skip to the interesting bit
                    while (off2 > LZO_PAGE_SIZE) {
                        uint32_t partlen;

                        if (inlen < sizeof(uint32_t))
                            break;

                        partlen = *(uint32_t*)(buf2 + inoff);

                        if (partlen < inlen) {
                            off2 -= LZO_PAGE_SIZE;
                            inoff += partlen + sizeof(uint32_t);
                            inlen -= partlen + sizeof(uint32_t);

                            if (LZO_PAGE_SIZE - (inoff % LZO_PAGE_SIZE) < sizeof(uint32_t))
                                inoff = ((inoff / LZO_PAGE_SIZE) + 1) * LZO_PAGE_SIZE;
                        } else
                            break;
                    }

                    buf2 = &buf2[inoff];
                    inpageoff = inoff % LZO_PAGE_SIZE;
                }

                /* Previous versions of this code decompressed directly into the destination buffer,
                 * but unfortunately that can't be relied on - Windows likes to use dummy pages sometimes
                 * when mmap-ing, which breaks the backtracking used by e.g. zstd. */

                if (off2 != 0)
                    outlen = off2 + min(rp->read, (uint32_t)(rp->extents[i].ed_num_bytes - rp->extents[i].off));
                else
                    outlen = min(rp->read, (uint32_t)(rp->extents[i].ed_num_bytes - rp->extents[i].off));

                decomp = ExAllocatePoolWithTag(pool_type, outlen, ALLOC_TAG);
                if (!decomp) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                ccj = (comp_calc_job*)ExAllocatePoolWithTag(pool_type, sizeof(comp_calc_job), ALLOC_TAG);
                if (!ccj) {
                    ERR("out of memory\n");

                    ExFreePool(decomp);

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                ccj->data = rp->data;
                ccj->decomp = decomp;

                ccj->offset = off2;
                ccj->length = (size_t)min(rp->read, rp->extents[i].ed_num_bytes - rp->extents[i].off);

                Status = add_calc_job_decomp(fcb->Vcb, rp->compression, buf2, inlen, decomp, outlen,
                                             inpageoff, &ccj->cj);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_calc_job_decomp returned %08lx\n", Status);

                    ExFreePool(decomp);
                    ExFreePool(ccj);

                    goto exit;
                }

                InsertTailList(&calc_jobs, &ccj->list_entry);

                buf += rp->extents[i].ed_size;
                rp->data = (uint8_t*)rp->data + rp->extents[i].ed_num_bytes - rp->extents[i].off;
                rp->read -= (uint32_t)(rp->extents[i].ed_num_bytes - rp->extents[i].off);
            }
        }

        le = le->Flink;
    }

    if (length > 0 && start + bytes_read < fcb->inode_item.st_size) {
        uint32_t read = (uint32_t)min(fcb->inode_item.st_size - start - bytes_read, length);

        RtlZeroMemory(data + bytes_read, read);

        bytes_read += read;
        length -= read;
    }

    Status = STATUS_SUCCESS;

    while (!IsListEmpty(&calc_jobs)) {
        comp_calc_job* ccj = CONTAINING_RECORD(RemoveTailList(&calc_jobs), comp_calc_job, list_entry);

        calc_thread_main(fcb->Vcb, ccj->cj);

        KeWaitForSingleObject(&ccj->cj->event, Executive, KernelMode, false, NULL);

        if (!NT_SUCCESS(ccj->cj->Status))
            Status = ccj->cj->Status;

        RtlCopyMemory(ccj->data, (uint8_t*)ccj->decomp + ccj->offset, ccj->length);
        ExFreePool(ccj->decomp);

        ExFreePool(ccj);
    }

    if (pbr)
        *pbr = bytes_read;

exit:
    while (!IsListEmpty(&read_parts)) {
        read_part* rp = CONTAINING_RECORD(RemoveHeadList(&read_parts), read_part, list_entry);

        if (rp->buf_free)
            ExFreePool(rp->buf);

        if (rp->csum_free)
            ExFreePool(rp->csum);

        ExFreePool(rp);
    }

    while (!IsListEmpty(&calc_jobs)) {
        comp_calc_job* ccj = CONTAINING_RECORD(RemoveHeadList(&calc_jobs), comp_calc_job, list_entry);

        KeWaitForSingleObject(&ccj->cj->event, Executive, KernelMode, false, NULL);

        if (ccj->decomp)
            ExFreePool(ccj->decomp);

        ExFreePool(ccj->cj);

        ExFreePool(ccj);
    }

    return Status;
}

NTSTATUS do_read(PIRP Irp, bool wait, ULONG* bytes_read) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    uint8_t* data = NULL;
    ULONG length = IrpSp->Parameters.Read.Length, addon = 0;
    uint64_t start = IrpSp->Parameters.Read.ByteOffset.QuadPart;

    *bytes_read = 0;

    if (!fcb || !fcb->Vcb || !fcb->subvol)
        return STATUS_INTERNAL_ERROR;

    TRACE("fcb = %p\n", fcb);
    TRACE("offset = %I64x, length = %lx\n", start, length);
    TRACE("paging_io = %s, no cache = %s\n", Irp->Flags & IRP_PAGING_IO ? "true" : "false", Irp->Flags & IRP_NOCACHE ? "true" : "false");

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

    if (start >= (uint64_t)fcb->Header.FileSize.QuadPart) {
        TRACE("tried to read with offset after file end (%I64x >= %I64x)\n", start, fcb->Header.FileSize.QuadPart);
        return STATUS_END_OF_FILE;
    }

    TRACE("FileObject %p fcb %p FileSize = %I64x st_size = %I64x (%p)\n", FileObject, fcb, fcb->Header.FileSize.QuadPart, fcb->inode_item.st_size, &fcb->inode_item.st_size);

    if (!(Irp->Flags & IRP_NOCACHE) && IrpSp->MinorFunction & IRP_MN_MDL) {
        NTSTATUS Status = STATUS_SUCCESS;

        _SEH2_TRY {
            if (!FileObject->PrivateCacheMap) {
                CC_FILE_SIZES ccfs;

                ccfs.AllocationSize = fcb->Header.AllocationSize;
                ccfs.FileSize = fcb->Header.FileSize;
                ccfs.ValidDataLength = fcb->Header.ValidDataLength;

                init_file_cache(FileObject, &ccfs);
            }

            CcMdlRead(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, &Irp->MdlAddress, &Irp->IoStatus);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

        if (NT_SUCCESS(Status)) {
            Status = Irp->IoStatus.Status;
            Irp->IoStatus.Information += addon;
            *bytes_read = (ULONG)Irp->IoStatus.Information;
        } else
            ERR("EXCEPTION - %08lx\n", Status);

        return Status;
    }

    data = map_user_buffer(Irp, fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);

    if (Irp->MdlAddress && !data) {
        ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (start >= (uint64_t)fcb->Header.ValidDataLength.QuadPart) {
        length = (ULONG)min(length, min(start + length, (uint64_t)fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart);
        RtlZeroMemory(data, length);
        Irp->IoStatus.Information = *bytes_read = length;
        return STATUS_SUCCESS;
    }

    if (length + start > (uint64_t)fcb->Header.ValidDataLength.QuadPart) {
        addon = (ULONG)(min(start + length, (uint64_t)fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart);
        RtlZeroMemory(data + (fcb->Header.ValidDataLength.QuadPart - start), addon);
        length = (ULONG)(fcb->Header.ValidDataLength.QuadPart - start);
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

            if (fCcCopyReadEx) {
                TRACE("CcCopyReadEx(%p, %I64x, %lx, %u, %p, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart,
                        length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread);
                TRACE("sizes = %I64x, %I64x, %I64x\n", fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
                if (!fCcCopyReadEx(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread)) {
                    TRACE("CcCopyReadEx could not wait\n");

                    IoMarkIrpPending(Irp);
                    return STATUS_PENDING;
                }
                TRACE("CcCopyReadEx finished\n");
            } else {
                TRACE("CcCopyRead(%p, %I64x, %lx, %u, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart, length, wait, data, &Irp->IoStatus);
                TRACE("sizes = %I64x, %I64x, %I64x\n", fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
                if (!CcCopyRead(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus)) {
                    TRACE("CcCopyRead could not wait\n");

                    IoMarkIrpPending(Irp);
                    return STATUS_PENDING;
                }
                TRACE("CcCopyRead finished\n");
            }
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (NT_SUCCESS(Status)) {
            Status = Irp->IoStatus.Status;
            Irp->IoStatus.Information += addon;
            *bytes_read = (ULONG)Irp->IoStatus.Information;
        } else
            ERR("EXCEPTION - %08lx\n", Status);

        return Status;
    } else {
        NTSTATUS Status;

        if (!wait) {
            IoMarkIrpPending(Irp);
            return STATUS_PENDING;
        }

        if (fcb->ads) {
            Status = read_stream(fcb, data, start, length, bytes_read);

            if (!NT_SUCCESS(Status))
                ERR("read_stream returned %08lx\n", Status);
        } else {
            Status = read_file(fcb, data, start, length, bytes_read, Irp);

            if (!NT_SUCCESS(Status))
                ERR("read_file returned %08lx\n", Status);
        }

        *bytes_read += addon;
        TRACE("read %lu bytes\n", *bytes_read);

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
NTSTATUS __stdcall drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    ULONG bytes_read = 0;
    NTSTATUS Status;
    bool top_level;
    fcb* fcb;
    ccb* ccb;
    bool acquired_fcb_lock = false, wait;

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

    if (!(Irp->Flags & IRP_PAGING_IO))
        FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

    wait = IoIsOperationSynchronous(Irp);

    // Don't offload jobs when doing paging IO - otherwise this can lead to
    // deadlocks in CcCopyRead.
    if (Irp->Flags & IRP_PAGING_IO)
        wait = true;

    if (!(Irp->Flags & IRP_PAGING_IO) && FileObject->SectionObjectPointer && FileObject->SectionObjectPointer->DataSectionObject) {
        IO_STATUS_BLOCK iosb;

        CcFlushCache(FileObject->SectionObjectPointer, &IrpSp->Parameters.Read.ByteOffset, IrpSp->Parameters.Read.Length, &iosb);
        if (!NT_SUCCESS(iosb.Status)) {
            ERR("CcFlushCache returned %08lx\n", iosb.Status);
            return iosb.Status;
        }
    }

    if (!ExIsResourceAcquiredSharedLite(fcb->Header.Resource)) {
        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            goto exit;
        }

        acquired_fcb_lock = true;
    }

    Status = do_read(Irp, wait, &bytes_read);

    if (acquired_fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);

exit:
    if (FileObject->Flags & FO_SYNCHRONOUS_IO && !(Irp->Flags & IRP_PAGING_IO))
        FileObject->CurrentByteOffset.QuadPart = IrpSp->Parameters.Read.ByteOffset.QuadPart + (NT_SUCCESS(Status) ? bytes_read : 0);

end:
    Irp->IoStatus.Status = Status;

    TRACE("Irp->IoStatus.Status = %08lx\n", Irp->IoStatus.Status);
    TRACE("Irp->IoStatus.Information = %Iu\n", Irp->IoStatus.Information);
    TRACE("returning %08lx\n", Status);

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    else {
        if (!add_thread_job(Vcb, Irp))
            Status = do_read_job(Irp);
    }

exit2:
    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
