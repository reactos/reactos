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

typedef struct {
    uint64_t start;
    uint64_t end;
    uint8_t* data;
    PMDL mdl;
    uint64_t irp_offset;
} write_stripe;

_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall write_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr);

static void remove_fcb_extent(fcb* fcb, extent* ext, LIST_ENTRY* rollback) __attribute__((nonnull(1, 2, 3)));

extern tPsUpdateDiskCounters fPsUpdateDiskCounters;
extern tCcCopyWriteEx fCcCopyWriteEx;
extern tFsRtlUpdateDiskCounters fFsRtlUpdateDiskCounters;
extern bool diskacc;

__attribute__((nonnull(1, 2, 4)))
bool find_data_address_in_chunk(device_extension* Vcb, chunk* c, uint64_t length, uint64_t* address) {
    LIST_ENTRY* le;
    space* s;

    TRACE("(%p, %I64x, %I64x, %p)\n", Vcb, c->offset, length, address);

    if (length > c->chunk_item->size - c->used)
        return false;

    if (!c->cache_loaded) {
        NTSTATUS Status = load_cache_chunk(Vcb, c, NULL);

        if (!NT_SUCCESS(Status)) {
            ERR("load_cache_chunk returned %08lx\n", Status);
            return false;
        }
    }

    if (IsListEmpty(&c->space_size))
        return false;

    le = c->space_size.Flink;
    while (le != &c->space_size) {
        s = CONTAINING_RECORD(le, space, list_entry_size);

        if (s->size == length) {
            *address = s->address;
            return true;
        } else if (s->size < length) {
            if (le == c->space_size.Flink)
                return false;

            s = CONTAINING_RECORD(le->Blink, space, list_entry_size);

            *address = s->address;
            return true;
        }

        le = le->Flink;
    }

    s = CONTAINING_RECORD(c->space_size.Blink, space, list_entry_size);

    if (s->size > length) {
        *address = s->address;
        return true;
    }

    return false;
}

__attribute__((nonnull(1)))
chunk* get_chunk_from_address(device_extension* Vcb, uint64_t address) {
    LIST_ENTRY* le2;

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, true);

    le2 = Vcb->chunks.Flink;
    while (le2 != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le2, chunk, list_entry);

        if (address >= c->offset && address < c->offset + c->chunk_item->size) {
            ExReleaseResourceLite(&Vcb->chunk_lock);
            return c;
        }

        le2 = le2->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    return NULL;
}

typedef struct {
    space* dh;
    device* device;
} stripe;

__attribute__((nonnull(1)))
static uint64_t find_new_chunk_address(device_extension* Vcb, uint64_t size) {
    uint64_t lastaddr;
    LIST_ENTRY* le;

    lastaddr = 0xc00000;

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->offset >= lastaddr + size)
            return lastaddr;

        lastaddr = c->offset + c->chunk_item->size;

        le = le->Flink;
    }

    return lastaddr;
}

__attribute__((nonnull(1,2)))
static bool find_new_dup_stripes(device_extension* Vcb, stripe* stripes, uint64_t max_stripe_size, bool full_size) {
    uint64_t devusage = 0xffffffffffffffff;
    space *devdh1 = NULL, *devdh2 = NULL;
    LIST_ENTRY* le;
    device* dev2 = NULL;

    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (!dev->readonly && !dev->reloc && dev->devobj) {
            uint64_t usage = (dev->devitem.bytes_used * 4096) / dev->devitem.num_bytes;

            // favour devices which have been used the least
            if (usage < devusage) {
                if (!IsListEmpty(&dev->space)) {
                    LIST_ENTRY* le2;
                    space *dh1 = NULL, *dh2 = NULL;

                    le2 = dev->space.Flink;
                    while (le2 != &dev->space) {
                        space* dh = CONTAINING_RECORD(le2, space, list_entry);

                        if (dh->size >= max_stripe_size && (!dh1 || !dh2 || dh->size < dh1->size)) {
                            dh2 = dh1;
                            dh1 = dh;
                        }

                        le2 = le2->Flink;
                    }

                    if (dh1 && (dh2 || dh1->size >= 2 * max_stripe_size)) {
                        dev2 = dev;
                        devusage = usage;
                        devdh1 = dh1;
                        devdh2 = dh2 ? dh2 : dh1;
                    }
                }
            }
        }

        le = le->Flink;
    }

    if (!devdh1) {
        uint64_t size = 0;

        // Can't find hole of at least max_stripe_size; look for the largest one we can find

        if (full_size)
            return false;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly && !dev->reloc) {
                if (!IsListEmpty(&dev->space)) {
                    LIST_ENTRY* le2;
                    space *dh1 = NULL, *dh2 = NULL;

                    le2 = dev->space.Flink;
                    while (le2 != &dev->space) {
                        space* dh = CONTAINING_RECORD(le2, space, list_entry);

                        if (!dh1 || !dh2 || dh->size < dh1->size) {
                            dh2 = dh1;
                            dh1 = dh;
                        }

                        le2 = le2->Flink;
                    }

                    if (dh1) {
                        uint64_t devsize;

                        if (dh2)
                            devsize = max(dh1->size / 2, min(dh1->size, dh2->size));
                        else
                            devsize = dh1->size / 2;

                        if (devsize > size) {
                            dev2 = dev;
                            devdh1 = dh1;

                            if (dh2 && min(dh1->size, dh2->size) > dh1->size / 2)
                                devdh2 = dh2;
                            else
                                devdh2 = dh1;

                            size = devsize;
                        }
                    }
                }
            }

            le = le->Flink;
        }

        if (!devdh1)
            return false;
    }

    stripes[0].device = stripes[1].device = dev2;
    stripes[0].dh = devdh1;
    stripes[1].dh = devdh2;

    return true;
}

__attribute__((nonnull(1,2)))
static bool find_new_stripe(device_extension* Vcb, stripe* stripes, uint16_t i, uint64_t max_stripe_size, bool allow_missing, bool full_size) {
    uint64_t k, devusage = 0xffffffffffffffff;
    space* devdh = NULL;
    LIST_ENTRY* le;
    device* dev2 = NULL;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        uint64_t usage;
        bool skip = false;

        if (dev->readonly || dev->reloc || (!dev->devobj && !allow_missing)) {
            le = le->Flink;
            continue;
        }

        // skip this device if it already has a stripe
        if (i > 0) {
            for (k = 0; k < i; k++) {
                if (stripes[k].device == dev) {
                    skip = true;
                    break;
                }
            }
        }

        if (!skip) {
            usage = (dev->devitem.bytes_used * 4096) / dev->devitem.num_bytes;

            // favour devices which have been used the least
            if (usage < devusage) {
                if (!IsListEmpty(&dev->space)) {
                    LIST_ENTRY* le2;

                    le2 = dev->space.Flink;
                    while (le2 != &dev->space) {
                        space* dh = CONTAINING_RECORD(le2, space, list_entry);

                        if ((dev2 != dev && dh->size >= max_stripe_size) ||
                            (dev2 == dev && dh->size >= max_stripe_size && dh->size < devdh->size)
                        ) {
                            devdh = dh;
                            dev2 = dev;
                            devusage = usage;
                        }

                        le2 = le2->Flink;
                    }
                }
            }
        }

        le = le->Flink;
    }

    if (!devdh) {
        // Can't find hole of at least max_stripe_size; look for the largest one we can find

        if (full_size)
            return false;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);
            bool skip = false;

            if (dev->readonly || dev->reloc || (!dev->devobj && !allow_missing)) {
                le = le->Flink;
                continue;
            }

            // skip this device if it already has a stripe
            if (i > 0) {
                for (k = 0; k < i; k++) {
                    if (stripes[k].device == dev) {
                        skip = true;
                        break;
                    }
                }
            }

            if (!skip) {
                if (!IsListEmpty(&dev->space)) {
                    LIST_ENTRY* le2;

                    le2 = dev->space.Flink;
                    while (le2 != &dev->space) {
                        space* dh = CONTAINING_RECORD(le2, space, list_entry);

                        if (!devdh || devdh->size < dh->size) {
                            devdh = dh;
                            dev2 = dev;
                        }

                        le2 = le2->Flink;
                    }
                }
            }

            le = le->Flink;
        }

        if (!devdh)
            return false;
    }

    stripes[i].dh = devdh;
    stripes[i].device = dev2;

    return true;
}

__attribute__((nonnull(1,3)))
NTSTATUS alloc_chunk(device_extension* Vcb, uint64_t flags, chunk** pc, bool full_size) {
    NTSTATUS Status;
    uint64_t max_stripe_size, max_chunk_size, stripe_size, stripe_length, factor;
    uint64_t total_size = 0, logaddr;
    uint16_t i, type, num_stripes, sub_stripes, max_stripes, min_stripes, allowed_missing;
    stripe* stripes = NULL;
    uint16_t cisize;
    CHUNK_ITEM_STRIPE* cis;
    chunk* c = NULL;
    space* s = NULL;
    LIST_ENTRY* le;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        total_size += dev->devitem.num_bytes;

        le = le->Flink;
    }

    TRACE("total_size = %I64x\n", total_size);

    // We purposely check for DATA first - mixed blocks have the same size
    // as DATA ones.
    if (flags & BLOCK_FLAG_DATA) {
        max_stripe_size = 0x40000000; // 1 GB
        max_chunk_size = 10 * max_stripe_size;
    } else if (flags & BLOCK_FLAG_METADATA) {
        if (total_size > 0xC80000000) // 50 GB
            max_stripe_size = 0x40000000; // 1 GB
        else
            max_stripe_size = 0x10000000; // 256 MB

        max_chunk_size = max_stripe_size;
    } else if (flags & BLOCK_FLAG_SYSTEM) {
        max_stripe_size = 0x2000000; // 32 MB
        max_chunk_size = 2 * max_stripe_size;
    } else {
        ERR("unknown chunk type\n");
        return STATUS_INTERNAL_ERROR;
    }

    if (flags & BLOCK_FLAG_DUPLICATE) {
        min_stripes = 2;
        max_stripes = 2;
        sub_stripes = 0;
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 0;
    } else if (flags & BLOCK_FLAG_RAID0) {
        min_stripes = 2;
        max_stripes = (uint16_t)min(0xffff, Vcb->superblock.num_devices);
        sub_stripes = 0;
        type = BLOCK_FLAG_RAID0;
        allowed_missing = 0;
    } else if (flags & BLOCK_FLAG_RAID1) {
        min_stripes = 2;
        max_stripes = 2;
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID1;
        allowed_missing = 1;
    } else if (flags & BLOCK_FLAG_RAID10) {
        min_stripes = 4;
        max_stripes = (uint16_t)min(0xffff, Vcb->superblock.num_devices);
        sub_stripes = 2;
        type = BLOCK_FLAG_RAID10;
        allowed_missing = 1;
    } else if (flags & BLOCK_FLAG_RAID5) {
        min_stripes = 3;
        max_stripes = (uint16_t)min(0xffff, Vcb->superblock.num_devices);
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID5;
        allowed_missing = 1;
    } else if (flags & BLOCK_FLAG_RAID6) {
        min_stripes = 4;
        max_stripes = 257;
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID6;
        allowed_missing = 2;
    } else if (flags & BLOCK_FLAG_RAID1C3) {
        min_stripes = 3;
        max_stripes = 3;
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID1C3;
        allowed_missing = 2;
    } else if (flags & BLOCK_FLAG_RAID1C4) {
        min_stripes = 4;
        max_stripes = 4;
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID1C4;
        allowed_missing = 3;
    } else { // SINGLE
        min_stripes = 1;
        max_stripes = 1;
        sub_stripes = 1;
        type = 0;
        allowed_missing = 0;
    }

    if (max_chunk_size > total_size / 10) {  // cap at 10%
        max_chunk_size = total_size / 10;
        max_stripe_size = max_chunk_size / min_stripes;
    }

    if (max_stripe_size > total_size / (10 * min_stripes))
        max_stripe_size = total_size / (10 * min_stripes);

    TRACE("would allocate a new chunk of %I64x bytes and stripe %I64x\n", max_chunk_size, max_stripe_size);

    stripes = ExAllocatePoolWithTag(PagedPool, sizeof(stripe) * max_stripes, ALLOC_TAG);
    if (!stripes) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    num_stripes = 0;

    if (type == BLOCK_FLAG_DUPLICATE) {
        if (!find_new_dup_stripes(Vcb, stripes, max_stripe_size, full_size)) {
            Status = STATUS_DISK_FULL;
            goto end;
        } else
            num_stripes = max_stripes;
    } else {
        for (i = 0; i < max_stripes; i++) {
            if (!find_new_stripe(Vcb, stripes, i, max_stripe_size, false, full_size))
                break;
            else
                num_stripes++;
        }
    }

    if (num_stripes < min_stripes && Vcb->options.allow_degraded && allowed_missing > 0) {
        uint16_t added_missing = 0;

        for (i = num_stripes; i < max_stripes; i++) {
            if (!find_new_stripe(Vcb, stripes, i, max_stripe_size, true, full_size))
                break;
            else {
                added_missing++;
                if (added_missing >= allowed_missing)
                    break;
            }
        }

        num_stripes += added_missing;
    }

    // for RAID10, round down to an even number of stripes
    if (type == BLOCK_FLAG_RAID10 && (num_stripes % sub_stripes) != 0) {
        num_stripes -= num_stripes % sub_stripes;
    }

    if (num_stripes < min_stripes) {
        WARN("found %u stripes, needed at least %u\n", num_stripes, min_stripes);
        Status = STATUS_DISK_FULL;
        goto end;
    }

    c = ExAllocatePoolWithTag(NonPagedPool, sizeof(chunk), ALLOC_TAG);
    if (!c) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    c->devices = NULL;

    cisize = sizeof(CHUNK_ITEM) + (num_stripes * sizeof(CHUNK_ITEM_STRIPE));
    c->chunk_item = ExAllocatePoolWithTag(NonPagedPool, cisize, ALLOC_TAG);
    if (!c->chunk_item) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    stripe_length = 0x10000; // FIXME? BTRFS_STRIPE_LEN in kernel

    if (type == BLOCK_FLAG_DUPLICATE && stripes[1].dh == stripes[0].dh)
        stripe_size = min(stripes[0].dh->size / 2, max_stripe_size);
    else {
        stripe_size = max_stripe_size;
        for (i = 0; i < num_stripes; i++) {
            if (stripes[i].dh->size < stripe_size)
                stripe_size = stripes[i].dh->size;
        }
    }

    if (type == BLOCK_FLAG_RAID0)
        factor = num_stripes;
    else if (type == BLOCK_FLAG_RAID10)
        factor = num_stripes / sub_stripes;
    else if (type == BLOCK_FLAG_RAID5)
        factor = num_stripes - 1;
    else if (type == BLOCK_FLAG_RAID6)
        factor = num_stripes - 2;
    else
        factor = 1; // SINGLE, DUPLICATE, RAID1, RAID1C3, RAID1C4

    if (stripe_size * factor > max_chunk_size)
        stripe_size = max_chunk_size / factor;

    if (stripe_size % stripe_length > 0)
        stripe_size -= stripe_size % stripe_length;

    if (stripe_size == 0) {
        ERR("not enough free space found (stripe_size == 0)\n");
        Status = STATUS_DISK_FULL;
        goto end;
    }

    c->chunk_item->size = stripe_size * factor;
    c->chunk_item->root_id = Vcb->extent_root->id;
    c->chunk_item->stripe_length = stripe_length;
    c->chunk_item->type = flags;
    c->chunk_item->opt_io_alignment = (uint32_t)c->chunk_item->stripe_length;
    c->chunk_item->opt_io_width = (uint32_t)c->chunk_item->stripe_length;
    c->chunk_item->sector_size = stripes[0].device->devitem.minimal_io_size;
    c->chunk_item->num_stripes = num_stripes;
    c->chunk_item->sub_stripes = sub_stripes;

    c->devices = ExAllocatePoolWithTag(NonPagedPool, sizeof(device*) * num_stripes, ALLOC_TAG);
    if (!c->devices) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    for (i = 0; i < num_stripes; i++) {
        cis[i].dev_id = stripes[i].device->devitem.dev_id;

        if (type == BLOCK_FLAG_DUPLICATE && i == 1 && stripes[i].dh == stripes[0].dh)
            cis[i].offset = stripes[0].dh->address + stripe_size;
        else
            cis[i].offset = stripes[i].dh->address;

        cis[i].dev_uuid = stripes[i].device->devitem.device_uuid;

        c->devices[i] = stripes[i].device;
    }

    logaddr = find_new_chunk_address(Vcb, c->chunk_item->size);

    Vcb->superblock.chunk_root_generation = Vcb->superblock.generation;

    c->size = cisize;
    c->offset = logaddr;
    c->used = c->oldused = 0;
    c->cache = c->old_cache = NULL;
    c->readonly = false;
    c->reloc = false;
    c->last_alloc_set = false;
    c->last_stripe = 0;
    c->cache_loaded = true;
    c->changed = false;
    c->space_changed = false;
    c->balance_num = 0;

    InitializeListHead(&c->space);
    InitializeListHead(&c->space_size);
    InitializeListHead(&c->deleting);
    InitializeListHead(&c->changed_extents);

    InitializeListHead(&c->range_locks);
    ExInitializeResourceLite(&c->range_locks_lock);
    KeInitializeEvent(&c->range_locks_event, NotificationEvent, false);

    InitializeListHead(&c->partial_stripes);
    ExInitializeResourceLite(&c->partial_stripes_lock);

    ExInitializeResourceLite(&c->lock);
    ExInitializeResourceLite(&c->changed_extents_lock);

    s = ExAllocatePoolWithTag(NonPagedPool, sizeof(space), ALLOC_TAG);
    if (!s) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    s->address = c->offset;
    s->size = c->chunk_item->size;
    InsertTailList(&c->space, &s->list_entry);
    InsertTailList(&c->space_size, &s->list_entry_size);

    protect_superblocks(c);

    for (i = 0; i < num_stripes; i++) {
        stripes[i].device->devitem.bytes_used += stripe_size;

        space_list_subtract2(&stripes[i].device->space, NULL, cis[i].offset, stripe_size, NULL, NULL);
    }

    Status = STATUS_SUCCESS;

    if (flags & BLOCK_FLAG_RAID5 || flags & BLOCK_FLAG_RAID6)
        Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_RAID56;

end:
    if (stripes)
        ExFreePool(stripes);

    if (!NT_SUCCESS(Status)) {
        if (c) {
            if (c->devices)
                ExFreePool(c->devices);

            if (c->chunk_item)
                ExFreePool(c->chunk_item);

            ExFreePool(c);
        }

        if (s) ExFreePool(s);
    } else {
        bool done = false;

        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);

            if (c2->offset > c->offset) {
                InsertHeadList(le->Blink, &c->list_entry);
                done = true;
                break;
            }

            le = le->Flink;
        }

        if (!done)
            InsertTailList(&Vcb->chunks, &c->list_entry);

        c->created = true;
        c->changed = true;
        c->space_changed = true;
        c->list_entry_balance.Flink = NULL;

        *pc = c;
    }

    return Status;
}

__attribute__((nonnull(1,3,5,8)))
static NTSTATUS prepare_raid0_write(_Pre_satisfies_(_Curr_->chunk_item->num_stripes>0) _In_ chunk* c, _In_ uint64_t address, _In_reads_bytes_(length) void* data,
                                    _In_ uint32_t length, _In_ write_stripe* stripes, _In_ PIRP Irp, _In_ uint64_t irp_offset, _In_ write_data_context* wtc) {
    uint64_t startoff, endoff;
    uint16_t startoffstripe, endoffstripe, stripenum;
    uint64_t pos, *stripeoff;
    uint32_t i;
    bool file_write = Irp && Irp->MdlAddress && (Irp->MdlAddress->ByteOffset == 0);
    PMDL master_mdl;
    PFN_NUMBER* pfns;

    stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &startoff, &startoffstripe);
    get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &endoff, &endoffstripe);

    if (file_write) {
        master_mdl = Irp->MdlAddress;

        pfns = (PFN_NUMBER*)(Irp->MdlAddress + 1);
        pfns = &pfns[irp_offset >> PAGE_SHIFT];
    } else if (((ULONG_PTR)data % PAGE_SIZE) != 0) {
        wtc->scratch = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);
        if (!wtc->scratch) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(wtc->scratch, data, length);

        master_mdl = IoAllocateMdl(wtc->scratch, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(master_mdl);

        wtc->mdl = master_mdl;

        pfns = (PFN_NUMBER*)(master_mdl + 1);
    } else {
        NTSTATUS Status = STATUS_SUCCESS;

        master_mdl = IoAllocateMdl(data, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoReadAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            return Status;
        }

        wtc->mdl = master_mdl;

        pfns = (PFN_NUMBER*)(master_mdl + 1);
    }

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (startoffstripe > i)
            stripes[i].start = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
        else if (startoffstripe == i)
            stripes[i].start = startoff;
        else
            stripes[i].start = startoff - (startoff % c->chunk_item->stripe_length);

        if (endoffstripe > i)
            stripes[i].end = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
        else if (endoffstripe == i)
            stripes[i].end = endoff + 1;
        else
            stripes[i].end = endoff - (endoff % c->chunk_item->stripe_length);

        if (stripes[i].start != stripes[i].end) {
            stripes[i].mdl = IoAllocateMdl(NULL, (ULONG)(stripes[i].end - stripes[i].start), false, false, NULL);
            if (!stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                ExFreePool(stripeoff);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    pos = 0;
    RtlZeroMemory(stripeoff, sizeof(uint64_t) * c->chunk_item->num_stripes);

    stripenum = startoffstripe;

    while (pos < length) {
        PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(stripes[stripenum].mdl + 1);

        if (pos == 0) {
            uint32_t writelen = (uint32_t)min(stripes[stripenum].end - stripes[stripenum].start,
                                          c->chunk_item->stripe_length - (stripes[stripenum].start % c->chunk_item->stripe_length));

            RtlCopyMemory(stripe_pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

            stripeoff[stripenum] += writelen;
            pos += writelen;
        } else if (length - pos < c->chunk_item->stripe_length) {
            RtlCopyMemory(&stripe_pfns[stripeoff[stripenum] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)((length - pos) * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
            break;
        } else {
            RtlCopyMemory(&stripe_pfns[stripeoff[stripenum] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

            stripeoff[stripenum] += c->chunk_item->stripe_length;
            pos += c->chunk_item->stripe_length;
        }

        stripenum = (stripenum + 1) % c->chunk_item->num_stripes;
    }

    ExFreePool(stripeoff);

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,3,5,8)))
static NTSTATUS prepare_raid10_write(_Pre_satisfies_(_Curr_->chunk_item->sub_stripes>0&&_Curr_->chunk_item->num_stripes>=_Curr_->chunk_item->sub_stripes) _In_ chunk* c,
                                     _In_ uint64_t address, _In_reads_bytes_(length) void* data, _In_ uint32_t length, _In_ write_stripe* stripes,
                                     _In_ PIRP Irp, _In_ uint64_t irp_offset, _In_ write_data_context* wtc) {
    uint64_t startoff, endoff;
    uint16_t startoffstripe, endoffstripe, stripenum;
    uint64_t pos, *stripeoff;
    uint32_t i;
    bool file_write = Irp && Irp->MdlAddress && (Irp->MdlAddress->ByteOffset == 0);
    PMDL master_mdl;
    PFN_NUMBER* pfns;

    get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes / c->chunk_item->sub_stripes, &startoff, &startoffstripe);
    get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes / c->chunk_item->sub_stripes, &endoff, &endoffstripe);

    stripenum = startoffstripe;
    startoffstripe *= c->chunk_item->sub_stripes;
    endoffstripe *= c->chunk_item->sub_stripes;

    if (file_write) {
        master_mdl = Irp->MdlAddress;

        pfns = (PFN_NUMBER*)(Irp->MdlAddress + 1);
        pfns = &pfns[irp_offset >> PAGE_SHIFT];
    } else if (((ULONG_PTR)data % PAGE_SIZE) != 0) {
        wtc->scratch = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);
        if (!wtc->scratch) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(wtc->scratch, data, length);

        master_mdl = IoAllocateMdl(wtc->scratch, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(master_mdl);

        wtc->mdl = master_mdl;

        pfns = (PFN_NUMBER*)(master_mdl + 1);
    } else {
        NTSTATUS Status = STATUS_SUCCESS;

        master_mdl = IoAllocateMdl(data, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoReadAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            return Status;
        }

        wtc->mdl = master_mdl;

        pfns = (PFN_NUMBER*)(master_mdl + 1);
    }

    for (i = 0; i < c->chunk_item->num_stripes; i += c->chunk_item->sub_stripes) {
        uint16_t j;

        if (startoffstripe > i)
            stripes[i].start = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
        else if (startoffstripe == i)
            stripes[i].start = startoff;
        else
            stripes[i].start = startoff - (startoff % c->chunk_item->stripe_length);

        if (endoffstripe > i)
            stripes[i].end = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
        else if (endoffstripe == i)
            stripes[i].end = endoff + 1;
        else
            stripes[i].end = endoff - (endoff % c->chunk_item->stripe_length);

        stripes[i].mdl = IoAllocateMdl(NULL, (ULONG)(stripes[i].end - stripes[i].start), false, false, NULL);
        if (!stripes[i].mdl) {
            ERR("IoAllocateMdl failed\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (j = 1; j < c->chunk_item->sub_stripes; j++) {
            stripes[i+j].start = stripes[i].start;
            stripes[i+j].end = stripes[i].end;
            stripes[i+j].data = stripes[i].data;
            stripes[i+j].mdl = stripes[i].mdl;
        }
    }

    pos = 0;

    stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t) * c->chunk_item->num_stripes / c->chunk_item->sub_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(stripeoff, sizeof(uint64_t) * c->chunk_item->num_stripes / c->chunk_item->sub_stripes);

    while (pos < length) {
        PFN_NUMBER* stripe_pfns = (PFN_NUMBER*)(stripes[stripenum * c->chunk_item->sub_stripes].mdl + 1);

        if (pos == 0) {
            uint32_t writelen = (uint32_t)min(stripes[stripenum * c->chunk_item->sub_stripes].end - stripes[stripenum * c->chunk_item->sub_stripes].start,
                                          c->chunk_item->stripe_length - (stripes[stripenum * c->chunk_item->sub_stripes].start % c->chunk_item->stripe_length));

            RtlCopyMemory(stripe_pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

            stripeoff[stripenum] += writelen;
            pos += writelen;
        } else if (length - pos < c->chunk_item->stripe_length) {
            RtlCopyMemory(&stripe_pfns[stripeoff[stripenum] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)((length - pos) * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
            break;
        } else {
            RtlCopyMemory(&stripe_pfns[stripeoff[stripenum] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

            stripeoff[stripenum] += c->chunk_item->stripe_length;
            pos += c->chunk_item->stripe_length;
        }

        stripenum = (stripenum + 1) % (c->chunk_item->num_stripes / c->chunk_item->sub_stripes);
    }

    ExFreePool(stripeoff);

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2,5)))
static NTSTATUS add_partial_stripe(device_extension* Vcb, chunk* c, uint64_t address, uint32_t length, void* data) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    partial_stripe* ps;
    uint64_t stripe_addr;
    uint16_t num_data_stripes;

    num_data_stripes = c->chunk_item->num_stripes - (c->chunk_item->type & BLOCK_FLAG_RAID5 ? 1 : 2);
    stripe_addr = address - ((address - c->offset) % (num_data_stripes * c->chunk_item->stripe_length));

    ExAcquireResourceExclusiveLite(&c->partial_stripes_lock, true);

    le = c->partial_stripes.Flink;
    while (le != &c->partial_stripes) {
        ps = CONTAINING_RECORD(le, partial_stripe, list_entry);

        if (ps->address == stripe_addr) {
            // update existing entry

            RtlCopyMemory(ps->data + address - stripe_addr, data, length);
            RtlClearBits(&ps->bmp, (ULONG)((address - stripe_addr) >> Vcb->sector_shift), length >> Vcb->sector_shift);

            // if now filled, flush
            if (RtlAreBitsClear(&ps->bmp, 0, (ULONG)((num_data_stripes * c->chunk_item->stripe_length) >> Vcb->sector_shift))) {
                Status = flush_partial_stripe(Vcb, c, ps);
                if (!NT_SUCCESS(Status)) {
                    ERR("flush_partial_stripe returned %08lx\n", Status);
                    goto end;
                }

                RemoveEntryList(&ps->list_entry);

                if (ps->bmparr)
                    ExFreePool(ps->bmparr);

                ExFreePool(ps);
            }

            Status = STATUS_SUCCESS;
            goto end;
        } else if (ps->address > stripe_addr)
            break;

        le = le->Flink;
    }

    // add new entry

    ps = ExAllocatePoolWithTag(NonPagedPool, offsetof(partial_stripe, data[0]) + (ULONG)(num_data_stripes * c->chunk_item->stripe_length), ALLOC_TAG);
    if (!ps) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    ps->bmplen = (ULONG)(num_data_stripes * c->chunk_item->stripe_length) >> Vcb->sector_shift;

    ps->address = stripe_addr;
    ps->bmparr = ExAllocatePoolWithTag(NonPagedPool, (size_t)sector_align(((ps->bmplen / 8) + 1), sizeof(ULONG)), ALLOC_TAG);
    if (!ps->bmparr) {
        ERR("out of memory\n");
        ExFreePool(ps);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlInitializeBitMap(&ps->bmp, ps->bmparr, ps->bmplen);
    RtlSetAllBits(&ps->bmp);

    RtlCopyMemory(ps->data + address - stripe_addr, data, length);
    RtlClearBits(&ps->bmp, (ULONG)((address - stripe_addr) >> Vcb->sector_shift), length >> Vcb->sector_shift);

    InsertHeadList(le->Blink, &ps->list_entry);

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&c->partial_stripes_lock);

    return Status;
}

typedef struct {
    PMDL mdl;
    PFN_NUMBER* pfns;
} log_stripe;

__attribute__((nonnull(1,2,4,6,10)))
static NTSTATUS prepare_raid5_write(device_extension* Vcb, chunk* c, uint64_t address, void* data, uint32_t length, write_stripe* stripes, PIRP Irp,
                                    uint64_t irp_offset, ULONG priority, write_data_context* wtc) {
    uint64_t startoff, endoff, parity_start, parity_end;
    uint16_t startoffstripe, endoffstripe, parity, num_data_stripes = c->chunk_item->num_stripes - 1;
    uint64_t pos, parity_pos, *stripeoff = NULL;
    uint32_t i;
    bool file_write = Irp && Irp->MdlAddress && (Irp->MdlAddress->ByteOffset == 0);
    PMDL master_mdl;
    NTSTATUS Status;
    PFN_NUMBER *pfns, *parity_pfns;
    log_stripe* log_stripes = NULL;

    if ((address + length - c->offset) % (num_data_stripes * c->chunk_item->stripe_length) > 0) {
        uint64_t delta = (address + length - c->offset) % (num_data_stripes * c->chunk_item->stripe_length);

        delta = min(length, delta);
        Status = add_partial_stripe(Vcb, c, address + length - delta, (uint32_t)delta, (uint8_t*)data + length - delta);
        if (!NT_SUCCESS(Status)) {
            ERR("add_partial_stripe returned %08lx\n", Status);
            goto exit;
        }

        length -= (uint32_t)delta;
    }

    if (length > 0 && (address - c->offset) % (num_data_stripes * c->chunk_item->stripe_length) > 0) {
        uint64_t delta = (num_data_stripes * c->chunk_item->stripe_length) - ((address - c->offset) % (num_data_stripes * c->chunk_item->stripe_length));

        Status = add_partial_stripe(Vcb, c, address, (uint32_t)delta, data);
        if (!NT_SUCCESS(Status)) {
            ERR("add_partial_stripe returned %08lx\n", Status);
            goto exit;
        }

        address += delta;
        length -= (uint32_t)delta;
        irp_offset += delta;
        data = (uint8_t*)data + delta;
    }

    if (length == 0) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, num_data_stripes, &startoff, &startoffstripe);
    get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, num_data_stripes, &endoff, &endoffstripe);

    pos = 0;
    while (pos < length) {
        parity = (((address - c->offset + pos) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;

        if (pos == 0) {
            uint16_t stripe = (parity + startoffstripe + 1) % c->chunk_item->num_stripes;
            ULONG skip, writelen;

            i = startoffstripe;
            while (stripe != parity) {
                if (i == startoffstripe) {
                    writelen = (ULONG)min(length, c->chunk_item->stripe_length - (startoff % c->chunk_item->stripe_length));

                    stripes[stripe].start = startoff;
                    stripes[stripe].end = startoff + writelen;

                    pos += writelen;

                    if (pos == length)
                        break;
                } else {
                    writelen = (ULONG)min(length - pos, c->chunk_item->stripe_length);

                    stripes[stripe].start = startoff - (startoff % c->chunk_item->stripe_length);
                    stripes[stripe].end = stripes[stripe].start + writelen;

                    pos += writelen;

                    if (pos == length)
                        break;
                }

                i++;
                stripe = (stripe + 1) % c->chunk_item->num_stripes;
            }

            if (pos == length)
                break;

            for (i = 0; i < startoffstripe; i++) {
                stripe = (parity + i + 1) % c->chunk_item->num_stripes;

                stripes[stripe].start = stripes[stripe].end = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            }

            stripes[parity].start = stripes[parity].end = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;

            if (length - pos > c->chunk_item->num_stripes * num_data_stripes * c->chunk_item->stripe_length) {
                skip = (ULONG)(((length - pos) / (c->chunk_item->num_stripes * num_data_stripes * c->chunk_item->stripe_length)) - 1);

                for (i = 0; i < c->chunk_item->num_stripes; i++) {
                    stripes[i].end += skip * c->chunk_item->num_stripes * c->chunk_item->stripe_length;
                }

                pos += skip * num_data_stripes * c->chunk_item->num_stripes * c->chunk_item->stripe_length;
            }
        } else if (length - pos >= c->chunk_item->stripe_length * num_data_stripes) {
            for (i = 0; i < c->chunk_item->num_stripes; i++) {
                stripes[i].end += c->chunk_item->stripe_length;
            }

            pos += c->chunk_item->stripe_length * num_data_stripes;
        } else {
            uint16_t stripe = (parity + 1) % c->chunk_item->num_stripes;

            i = 0;
            while (stripe != parity) {
                if (endoffstripe == i) {
                    stripes[stripe].end = endoff + 1;
                    break;
                } else if (endoffstripe > i)
                    stripes[stripe].end = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;

                i++;
                stripe = (stripe + 1) % c->chunk_item->num_stripes;
            }

            break;
        }
    }

    parity_start = 0xffffffffffffffff;
    parity_end = 0;

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (stripes[i].start != 0 || stripes[i].end != 0) {
            parity_start = min(stripes[i].start, parity_start);
            parity_end = max(stripes[i].end, parity_end);
        }
    }

    if (parity_end == parity_start) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    parity = (((address - c->offset) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;
    stripes[parity].start = parity_start;

    parity = (((address - c->offset + length - 1) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;
    stripes[parity].end = parity_end;

    log_stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(log_stripe) * num_data_stripes, ALLOC_TAG);
    if (!log_stripes) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(log_stripes, sizeof(log_stripe) * num_data_stripes);

    for (i = 0; i < num_data_stripes; i++) {
        log_stripes[i].mdl = IoAllocateMdl(NULL, (ULONG)(parity_end - parity_start), false, false, NULL);
        if (!log_stripes[i].mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        log_stripes[i].mdl->MdlFlags |= MDL_PARTIAL;
        log_stripes[i].pfns = (PFN_NUMBER*)(log_stripes[i].mdl + 1);
    }

    wtc->parity1 = ExAllocatePoolWithTag(NonPagedPool, (ULONG)(parity_end - parity_start), ALLOC_TAG);
    if (!wtc->parity1) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    wtc->parity1_mdl = IoAllocateMdl(wtc->parity1, (ULONG)(parity_end - parity_start), false, false, NULL);
    if (!wtc->parity1_mdl) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    MmBuildMdlForNonPagedPool(wtc->parity1_mdl);

    if (file_write)
        master_mdl = Irp->MdlAddress;
    else if (((ULONG_PTR)data % PAGE_SIZE) != 0) {
        wtc->scratch = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);
        if (!wtc->scratch) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlCopyMemory(wtc->scratch, data, length);

        master_mdl = IoAllocateMdl(wtc->scratch, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        MmBuildMdlForNonPagedPool(master_mdl);

        wtc->mdl = master_mdl;
    } else {
        master_mdl = IoAllocateMdl(data, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoReadAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            return Status;
        }

        wtc->mdl = master_mdl;
    }

    pfns = (PFN_NUMBER*)(master_mdl + 1);
    parity_pfns = (PFN_NUMBER*)(wtc->parity1_mdl + 1);

    if (file_write)
        pfns = &pfns[irp_offset >> PAGE_SHIFT];

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (stripes[i].start != stripes[i].end) {
            stripes[i].mdl = IoAllocateMdl((uint8_t*)MmGetMdlVirtualAddress(master_mdl) + irp_offset, (ULONG)(stripes[i].end - stripes[i].start), false, false, NULL);
            if (!stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        }
    }

    stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(stripeoff, sizeof(uint64_t) * c->chunk_item->num_stripes);

    pos = 0;
    parity_pos = 0;

    while (pos < length) {
        PFN_NUMBER* stripe_pfns;

        parity = (((address - c->offset + pos) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;

        if (pos == 0) {
            uint16_t stripe = (parity + startoffstripe + 1) % c->chunk_item->num_stripes;
            uint32_t writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start,
                                                            c->chunk_item->stripe_length - (stripes[stripe].start % c->chunk_item->stripe_length)));
            uint32_t maxwritelen = writelen;

            stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);

            RtlCopyMemory(stripe_pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

            RtlCopyMemory(log_stripes[startoffstripe].pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
            log_stripes[startoffstripe].pfns += writelen >> PAGE_SHIFT;

            stripeoff[stripe] = writelen;
            pos += writelen;

            stripe = (stripe + 1) % c->chunk_item->num_stripes;
            i = startoffstripe + 1;

            while (stripe != parity) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);
                writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start, c->chunk_item->stripe_length));

                if (writelen == 0)
                    break;

                if (writelen > maxwritelen)
                    maxwritelen = writelen;

                RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
                log_stripes[i].pfns += writelen >> PAGE_SHIFT;

                stripeoff[stripe] = writelen;
                pos += writelen;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity].mdl + 1);

            RtlCopyMemory(stripe_pfns, parity_pfns, maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
            stripeoff[parity] = maxwritelen;
            parity_pos = maxwritelen;
        } else if (length - pos >= c->chunk_item->stripe_length * num_data_stripes) {
            uint16_t stripe = (parity + 1) % c->chunk_item->num_stripes;

            i = 0;
            while (stripe != parity) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);

                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
                log_stripes[i].pfns += c->chunk_item->stripe_length >> PAGE_SHIFT;

                stripeoff[stripe] += c->chunk_item->stripe_length;
                pos += c->chunk_item->stripe_length;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity].mdl + 1);

            RtlCopyMemory(&stripe_pfns[stripeoff[parity] >> PAGE_SHIFT], &parity_pfns[parity_pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
            stripeoff[parity] += c->chunk_item->stripe_length;
            parity_pos += c->chunk_item->stripe_length;
        } else {
            uint16_t stripe = (parity + 1) % c->chunk_item->num_stripes;
            uint32_t writelen, maxwritelen = 0;

            i = 0;
            while (pos < length) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);
                writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start, c->chunk_item->stripe_length));

                if (writelen == 0)
                    break;

                if (writelen > maxwritelen)
                    maxwritelen = writelen;

                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
                log_stripes[i].pfns += writelen >> PAGE_SHIFT;

                stripeoff[stripe] += writelen;
                pos += writelen;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity].mdl + 1);

            RtlCopyMemory(&stripe_pfns[stripeoff[parity] >> PAGE_SHIFT], &parity_pfns[parity_pos >> PAGE_SHIFT], maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
        }
    }

    for (i = 0; i < num_data_stripes; i++) {
        uint8_t* ss = MmGetSystemAddressForMdlSafe(log_stripes[i].mdl, priority);

        if (i == 0)
            RtlCopyMemory(wtc->parity1, ss, (uint32_t)(parity_end - parity_start));
        else
            do_xor(wtc->parity1, ss, (uint32_t)(parity_end - parity_start));
    }

    Status = STATUS_SUCCESS;

exit:
    if (log_stripes) {
        for (i = 0; i < num_data_stripes; i++) {
            if (log_stripes[i].mdl)
                IoFreeMdl(log_stripes[i].mdl);
        }

        ExFreePool(log_stripes);
    }

    if (stripeoff)
        ExFreePool(stripeoff);

    return Status;
}

__attribute__((nonnull(1,2,4,6,10)))
static NTSTATUS prepare_raid6_write(device_extension* Vcb, chunk* c, uint64_t address, void* data, uint32_t length, write_stripe* stripes, PIRP Irp,
                                    uint64_t irp_offset, ULONG priority, write_data_context* wtc) {
    uint64_t startoff, endoff, parity_start, parity_end;
    uint16_t startoffstripe, endoffstripe, parity1, num_data_stripes = c->chunk_item->num_stripes - 2;
    uint64_t pos, parity_pos, *stripeoff = NULL;
    uint32_t i;
    bool file_write = Irp && Irp->MdlAddress && (Irp->MdlAddress->ByteOffset == 0);
    PMDL master_mdl;
    NTSTATUS Status;
    PFN_NUMBER *pfns, *parity1_pfns, *parity2_pfns;
    log_stripe* log_stripes = NULL;

    if ((address + length - c->offset) % (num_data_stripes * c->chunk_item->stripe_length) > 0) {
        uint64_t delta = (address + length - c->offset) % (num_data_stripes * c->chunk_item->stripe_length);

        delta = min(length, delta);
        Status = add_partial_stripe(Vcb, c, address + length - delta, (uint32_t)delta, (uint8_t*)data + length - delta);
        if (!NT_SUCCESS(Status)) {
            ERR("add_partial_stripe returned %08lx\n", Status);
            goto exit;
        }

        length -= (uint32_t)delta;
    }

    if (length > 0 && (address - c->offset) % (num_data_stripes * c->chunk_item->stripe_length) > 0) {
        uint64_t delta = (num_data_stripes * c->chunk_item->stripe_length) - ((address - c->offset) % (num_data_stripes * c->chunk_item->stripe_length));

        Status = add_partial_stripe(Vcb, c, address, (uint32_t)delta, data);
        if (!NT_SUCCESS(Status)) {
            ERR("add_partial_stripe returned %08lx\n", Status);
            goto exit;
        }

        address += delta;
        length -= (uint32_t)delta;
        irp_offset += delta;
        data = (uint8_t*)data + delta;
    }

    if (length == 0) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, num_data_stripes, &startoff, &startoffstripe);
    get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, num_data_stripes, &endoff, &endoffstripe);

    pos = 0;
    while (pos < length) {
        parity1 = (((address - c->offset + pos) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;

        if (pos == 0) {
            uint16_t stripe = (parity1 + startoffstripe + 2) % c->chunk_item->num_stripes;
            uint16_t parity2 = (parity1 + 1) % c->chunk_item->num_stripes;
            ULONG skip, writelen;

            i = startoffstripe;
            while (stripe != parity1) {
                if (i == startoffstripe) {
                    writelen = (ULONG)min(length, c->chunk_item->stripe_length - (startoff % c->chunk_item->stripe_length));

                    stripes[stripe].start = startoff;
                    stripes[stripe].end = startoff + writelen;

                    pos += writelen;

                    if (pos == length)
                        break;
                } else {
                    writelen = (ULONG)min(length - pos, c->chunk_item->stripe_length);

                    stripes[stripe].start = startoff - (startoff % c->chunk_item->stripe_length);
                    stripes[stripe].end = stripes[stripe].start + writelen;

                    pos += writelen;

                    if (pos == length)
                        break;
                }

                i++;
                stripe = (stripe + 1) % c->chunk_item->num_stripes;
            }

            if (pos == length)
                break;

            for (i = 0; i < startoffstripe; i++) {
                stripe = (parity1 + i + 2) % c->chunk_item->num_stripes;

                stripes[stripe].start = stripes[stripe].end = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            }

            stripes[parity1].start = stripes[parity1].end = stripes[parity2].start = stripes[parity2].end =
                startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;

            if (length - pos > c->chunk_item->num_stripes * num_data_stripes * c->chunk_item->stripe_length) {
                skip = (ULONG)(((length - pos) / (c->chunk_item->num_stripes * num_data_stripes * c->chunk_item->stripe_length)) - 1);

                for (i = 0; i < c->chunk_item->num_stripes; i++) {
                    stripes[i].end += skip * c->chunk_item->num_stripes * c->chunk_item->stripe_length;
                }

                pos += skip * num_data_stripes * c->chunk_item->num_stripes * c->chunk_item->stripe_length;
            }
        } else if (length - pos >= c->chunk_item->stripe_length * num_data_stripes) {
            for (i = 0; i < c->chunk_item->num_stripes; i++) {
                stripes[i].end += c->chunk_item->stripe_length;
            }

            pos += c->chunk_item->stripe_length * num_data_stripes;
        } else {
            uint16_t stripe = (parity1 + 2) % c->chunk_item->num_stripes;

            i = 0;
            while (stripe != parity1) {
                if (endoffstripe == i) {
                    stripes[stripe].end = endoff + 1;
                    break;
                } else if (endoffstripe > i)
                    stripes[stripe].end = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;

                i++;
                stripe = (stripe + 1) % c->chunk_item->num_stripes;
            }

            break;
        }
    }

    parity_start = 0xffffffffffffffff;
    parity_end = 0;

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (stripes[i].start != 0 || stripes[i].end != 0) {
            parity_start = min(stripes[i].start, parity_start);
            parity_end = max(stripes[i].end, parity_end);
        }
    }

    if (parity_end == parity_start) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    parity1 = (((address - c->offset) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;
    stripes[parity1].start = stripes[(parity1 + 1) % c->chunk_item->num_stripes].start = parity_start;

    parity1 = (((address - c->offset + length - 1) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;
    stripes[parity1].end = stripes[(parity1 + 1) % c->chunk_item->num_stripes].end = parity_end;

    log_stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(log_stripe) * num_data_stripes, ALLOC_TAG);
    if (!log_stripes) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(log_stripes, sizeof(log_stripe) * num_data_stripes);

    for (i = 0; i < num_data_stripes; i++) {
        log_stripes[i].mdl = IoAllocateMdl(NULL, (ULONG)(parity_end - parity_start), false, false, NULL);
        if (!log_stripes[i].mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        log_stripes[i].mdl->MdlFlags |= MDL_PARTIAL;
        log_stripes[i].pfns = (PFN_NUMBER*)(log_stripes[i].mdl + 1);
    }

    wtc->parity1 = ExAllocatePoolWithTag(NonPagedPool, (ULONG)(parity_end - parity_start), ALLOC_TAG);
    if (!wtc->parity1) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    wtc->parity2 = ExAllocatePoolWithTag(NonPagedPool, (ULONG)(parity_end - parity_start), ALLOC_TAG);
    if (!wtc->parity2) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    wtc->parity1_mdl = IoAllocateMdl(wtc->parity1, (ULONG)(parity_end - parity_start), false, false, NULL);
    if (!wtc->parity1_mdl) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    MmBuildMdlForNonPagedPool(wtc->parity1_mdl);

    wtc->parity2_mdl = IoAllocateMdl(wtc->parity2, (ULONG)(parity_end - parity_start), false, false, NULL);
    if (!wtc->parity2_mdl) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    MmBuildMdlForNonPagedPool(wtc->parity2_mdl);

    if (file_write)
        master_mdl = Irp->MdlAddress;
    else if (((ULONG_PTR)data % PAGE_SIZE) != 0) {
        wtc->scratch = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);
        if (!wtc->scratch) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlCopyMemory(wtc->scratch, data, length);

        master_mdl = IoAllocateMdl(wtc->scratch, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        MmBuildMdlForNonPagedPool(master_mdl);

        wtc->mdl = master_mdl;
    } else {
        master_mdl = IoAllocateMdl(data, length, false, false, NULL);
        if (!master_mdl) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(master_mdl, KernelMode, IoReadAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
            IoFreeMdl(master_mdl);
            goto exit;
        }

        wtc->mdl = master_mdl;
    }

    pfns = (PFN_NUMBER*)(master_mdl + 1);
    parity1_pfns = (PFN_NUMBER*)(wtc->parity1_mdl + 1);
    parity2_pfns = (PFN_NUMBER*)(wtc->parity2_mdl + 1);

    if (file_write)
        pfns = &pfns[irp_offset >> PAGE_SHIFT];

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (stripes[i].start != stripes[i].end) {
            stripes[i].mdl = IoAllocateMdl((uint8_t*)MmGetMdlVirtualAddress(master_mdl) + irp_offset, (ULONG)(stripes[i].end - stripes[i].start), false, false, NULL);
            if (!stripes[i].mdl) {
                ERR("IoAllocateMdl failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        }
    }

    stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(uint64_t) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(stripeoff, sizeof(uint64_t) * c->chunk_item->num_stripes);

    pos = 0;
    parity_pos = 0;

    while (pos < length) {
        PFN_NUMBER* stripe_pfns;

        parity1 = (((address - c->offset + pos) / (num_data_stripes * c->chunk_item->stripe_length)) + num_data_stripes) % c->chunk_item->num_stripes;

        if (pos == 0) {
            uint16_t stripe = (parity1 + startoffstripe + 2) % c->chunk_item->num_stripes, parity2;
            uint32_t writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start,
                                                            c->chunk_item->stripe_length - (stripes[stripe].start % c->chunk_item->stripe_length)));
            uint32_t maxwritelen = writelen;

            stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);

            RtlCopyMemory(stripe_pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

            RtlCopyMemory(log_stripes[startoffstripe].pfns, pfns, writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
            log_stripes[startoffstripe].pfns += writelen >> PAGE_SHIFT;

            stripeoff[stripe] = writelen;
            pos += writelen;

            stripe = (stripe + 1) % c->chunk_item->num_stripes;
            i = startoffstripe + 1;

            while (stripe != parity1) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);
                writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start, c->chunk_item->stripe_length));

                if (writelen == 0)
                    break;

                if (writelen > maxwritelen)
                    maxwritelen = writelen;

                RtlCopyMemory(stripe_pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
                log_stripes[i].pfns += writelen >> PAGE_SHIFT;

                stripeoff[stripe] = writelen;
                pos += writelen;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity1].mdl + 1);
            RtlCopyMemory(stripe_pfns, parity1_pfns, maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
            stripeoff[parity1] = maxwritelen;

            parity2 = (parity1 + 1) % c->chunk_item->num_stripes;

            stripe_pfns = (PFN_NUMBER*)(stripes[parity2].mdl + 1);
            RtlCopyMemory(stripe_pfns, parity2_pfns, maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
            stripeoff[parity2] = maxwritelen;

            parity_pos = maxwritelen;
        } else if (length - pos >= c->chunk_item->stripe_length * num_data_stripes) {
            uint16_t stripe = (parity1 + 2) % c->chunk_item->num_stripes, parity2;

            i = 0;
            while (stripe != parity1) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);

                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
                log_stripes[i].pfns += c->chunk_item->stripe_length >> PAGE_SHIFT;

                stripeoff[stripe] += c->chunk_item->stripe_length;
                pos += c->chunk_item->stripe_length;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity1].mdl + 1);
            RtlCopyMemory(&stripe_pfns[stripeoff[parity1] >> PAGE_SHIFT], &parity1_pfns[parity_pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
            stripeoff[parity1] += c->chunk_item->stripe_length;

            parity2 = (parity1 + 1) % c->chunk_item->num_stripes;

            stripe_pfns = (PFN_NUMBER*)(stripes[parity2].mdl + 1);
            RtlCopyMemory(&stripe_pfns[stripeoff[parity2] >> PAGE_SHIFT], &parity2_pfns[parity_pos >> PAGE_SHIFT], (ULONG)(c->chunk_item->stripe_length * sizeof(PFN_NUMBER) >> PAGE_SHIFT));
            stripeoff[parity2] += c->chunk_item->stripe_length;

            parity_pos += c->chunk_item->stripe_length;
        } else {
            uint16_t stripe = (parity1 + 2) % c->chunk_item->num_stripes, parity2;
            uint32_t writelen, maxwritelen = 0;

            i = 0;
            while (pos < length) {
                stripe_pfns = (PFN_NUMBER*)(stripes[stripe].mdl + 1);
                writelen = (uint32_t)min(length - pos, min(stripes[stripe].end - stripes[stripe].start, c->chunk_item->stripe_length));

                if (writelen == 0)
                    break;

                if (writelen > maxwritelen)
                    maxwritelen = writelen;

                RtlCopyMemory(&stripe_pfns[stripeoff[stripe] >> PAGE_SHIFT], &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

                RtlCopyMemory(log_stripes[i].pfns, &pfns[pos >> PAGE_SHIFT], writelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
                log_stripes[i].pfns += writelen >> PAGE_SHIFT;

                stripeoff[stripe] += writelen;
                pos += writelen;

                stripe = (stripe + 1) % c->chunk_item->num_stripes;
                i++;
            }

            stripe_pfns = (PFN_NUMBER*)(stripes[parity1].mdl + 1);
            RtlCopyMemory(&stripe_pfns[stripeoff[parity1] >> PAGE_SHIFT], &parity1_pfns[parity_pos >> PAGE_SHIFT], maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);

            parity2 = (parity1 + 1) % c->chunk_item->num_stripes;

            stripe_pfns = (PFN_NUMBER*)(stripes[parity2].mdl + 1);
            RtlCopyMemory(&stripe_pfns[stripeoff[parity2] >> PAGE_SHIFT], &parity2_pfns[parity_pos >> PAGE_SHIFT], maxwritelen * sizeof(PFN_NUMBER) >> PAGE_SHIFT);
        }
    }

    for (i = 0; i < num_data_stripes; i++) {
        uint8_t* ss = MmGetSystemAddressForMdlSafe(log_stripes[c->chunk_item->num_stripes - 3 - i].mdl, priority);

        if (i == 0) {
            RtlCopyMemory(wtc->parity1, ss, (ULONG)(parity_end - parity_start));
            RtlCopyMemory(wtc->parity2, ss, (ULONG)(parity_end - parity_start));
        } else {
            do_xor(wtc->parity1, ss, (uint32_t)(parity_end - parity_start));

            galois_double(wtc->parity2, (uint32_t)(parity_end - parity_start));
            do_xor(wtc->parity2, ss, (uint32_t)(parity_end - parity_start));
        }
    }

    Status = STATUS_SUCCESS;

exit:
    if (log_stripes) {
        for (i = 0; i < num_data_stripes; i++) {
            if (log_stripes[i].mdl)
                IoFreeMdl(log_stripes[i].mdl);
        }

        ExFreePool(log_stripes);
    }

    if (stripeoff)
        ExFreePool(stripeoff);

    return Status;
}

__attribute__((nonnull(1,3,5)))
NTSTATUS write_data(_In_ device_extension* Vcb, _In_ uint64_t address, _In_reads_bytes_(length) void* data, _In_ uint32_t length, _In_ write_data_context* wtc,
                    _In_opt_ PIRP Irp, _In_opt_ chunk* c, _In_ bool file_write, _In_ uint64_t irp_offset, _In_ ULONG priority) {
    NTSTATUS Status;
    uint32_t i;
    CHUNK_ITEM_STRIPE* cis;
    write_stripe* stripes = NULL;
    uint64_t total_writing = 0;
    ULONG allowed_missing, missing;

    TRACE("(%p, %I64x, %p, %x)\n", Vcb, address, data, length);

    if (!c) {
        c = get_chunk_from_address(Vcb, address);
        if (!c) {
            ERR("could not get chunk for address %I64x\n", address);
            return STATUS_INTERNAL_ERROR;
        }
    }

    stripes = ExAllocatePoolWithTag(PagedPool, sizeof(write_stripe) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripes) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(stripes, sizeof(write_stripe) * c->chunk_item->num_stripes);

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

    if (c->chunk_item->type & BLOCK_FLAG_RAID0) {
        Status = prepare_raid0_write(c, address, data, length, stripes, file_write ? Irp : NULL, irp_offset, wtc);
        if (!NT_SUCCESS(Status)) {
            ERR("prepare_raid0_write returned %08lx\n", Status);
            goto prepare_failed;
        }

        allowed_missing = 0;
    } else if (c->chunk_item->type & BLOCK_FLAG_RAID10) {
        Status = prepare_raid10_write(c, address, data, length, stripes, file_write ? Irp : NULL, irp_offset, wtc);
        if (!NT_SUCCESS(Status)) {
            ERR("prepare_raid10_write returned %08lx\n", Status);
            goto prepare_failed;
        }

        allowed_missing = 1;
    } else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
        Status = prepare_raid5_write(Vcb, c, address, data, length, stripes, file_write ? Irp : NULL, irp_offset, priority, wtc);
        if (!NT_SUCCESS(Status)) {
            ERR("prepare_raid5_write returned %08lx\n", Status);
            goto prepare_failed;
        }

        allowed_missing = 1;
    } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
        Status = prepare_raid6_write(Vcb, c, address, data, length, stripes, file_write ? Irp : NULL, irp_offset, priority, wtc);
        if (!NT_SUCCESS(Status)) {
            ERR("prepare_raid6_write returned %08lx\n", Status);
            goto prepare_failed;
        }

        allowed_missing = 2;
    } else {  // write same data to every location - SINGLE, DUP, RAID1, RAID1C3, RAID1C4
        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            stripes[i].start = address - c->offset;
            stripes[i].end = stripes[i].start + length;
            stripes[i].data = data;
            stripes[i].irp_offset = irp_offset;

            if (c->devices[i]->devobj) {
                if (file_write) {
                    uint8_t* va;
                    ULONG writelen = (ULONG)(stripes[i].end - stripes[i].start);

                    va = (uint8_t*)MmGetMdlVirtualAddress(Irp->MdlAddress) + stripes[i].irp_offset;

                    stripes[i].mdl = IoAllocateMdl(va, writelen, false, false, NULL);
                    if (!stripes[i].mdl) {
                        ERR("IoAllocateMdl failed\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto prepare_failed;
                    }

                    IoBuildPartialMdl(Irp->MdlAddress, stripes[i].mdl, va, writelen);
                } else {
                    stripes[i].mdl = IoAllocateMdl(stripes[i].data, (ULONG)(stripes[i].end - stripes[i].start), false, false, NULL);
                    if (!stripes[i].mdl) {
                        ERR("IoAllocateMdl failed\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto prepare_failed;
                    }

                    Status = STATUS_SUCCESS;

                    _SEH2_TRY {
                        MmProbeAndLockPages(stripes[i].mdl, KernelMode, IoReadAccess);
                    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                        Status = _SEH2_GetExceptionCode();
                    } _SEH2_END;

                    if (!NT_SUCCESS(Status)) {
                        ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
                        IoFreeMdl(stripes[i].mdl);
                        stripes[i].mdl = NULL;
                        goto prepare_failed;
                    }
                }
            }
        }

        allowed_missing = c->chunk_item->num_stripes - 1;
    }

    missing = 0;
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (!c->devices[i]->devobj)
            missing++;
    }

    if (missing > allowed_missing) {
        ERR("cannot write as %lu missing devices (maximum %lu)\n", missing, allowed_missing);
        Status = STATUS_DEVICE_NOT_READY;
        goto prepare_failed;
    }

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        write_data_stripe* stripe;
        PIO_STACK_LOCATION IrpSp;

        stripe = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_stripe), ALLOC_TAG);
        if (!stripe) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        if (stripes[i].start == stripes[i].end || !c->devices[i]->devobj) {
            stripe->status = WriteDataStatus_Ignore;
            stripe->Irp = NULL;
            stripe->buf = stripes[i].data;
            stripe->mdl = NULL;
        } else {
            stripe->context = (struct _write_data_context*)wtc;
            stripe->buf = stripes[i].data;
            stripe->device = c->devices[i];
            RtlZeroMemory(&stripe->iosb, sizeof(IO_STATUS_BLOCK));
            stripe->status = WriteDataStatus_Pending;
            stripe->mdl = stripes[i].mdl;

            if (!Irp) {
                stripe->Irp = IoAllocateIrp(stripe->device->devobj->StackSize, false);

                if (!stripe->Irp) {
                    ERR("IoAllocateIrp failed\n");
                    ExFreePool(stripe);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
            } else {
                stripe->Irp = IoMakeAssociatedIrp(Irp, stripe->device->devobj->StackSize);

                if (!stripe->Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    ExFreePool(stripe);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
            }

            IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
            IrpSp->MajorFunction = IRP_MJ_WRITE;
            IrpSp->FileObject = stripe->device->fileobj;

            if (stripe->device->devobj->Flags & DO_BUFFERED_IO) {
                stripe->Irp->AssociatedIrp.SystemBuffer = MmGetSystemAddressForMdlSafe(stripes[i].mdl, priority);

                stripe->Irp->Flags = IRP_BUFFERED_IO;
            } else if (stripe->device->devobj->Flags & DO_DIRECT_IO)
                stripe->Irp->MdlAddress = stripe->mdl;
            else
                stripe->Irp->UserBuffer = MmGetSystemAddressForMdlSafe(stripes[i].mdl, priority);

#ifdef DEBUG_PARANOID
            if (stripes[i].end < stripes[i].start) {
                ERR("trying to write stripe with negative length (%I64x < %I64x)\n", stripes[i].end, stripes[i].start);
                int3;
            }
#endif

            IrpSp->Parameters.Write.Length = (ULONG)(stripes[i].end - stripes[i].start);
            IrpSp->Parameters.Write.ByteOffset.QuadPart = stripes[i].start + cis[i].offset;

            total_writing += IrpSp->Parameters.Write.Length;

            stripe->Irp->UserIosb = &stripe->iosb;
            wtc->stripes_left++;

            IoSetCompletionRoutine(stripe->Irp, write_data_completion, stripe, true, true, true);
        }

        InsertTailList(&wtc->stripes, &stripe->list_entry);
    }

    if (diskacc)
        fFsRtlUpdateDiskCounters(0, total_writing);

    Status = STATUS_SUCCESS;

end:

    if (stripes) ExFreePool(stripes);

    if (!NT_SUCCESS(Status))
        free_write_data_stripes(wtc);

    return Status;

prepare_failed:
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (stripes[i].mdl && (i == 0 || stripes[i].mdl != stripes[i-1].mdl)) {
            if (stripes[i].mdl->MdlFlags & MDL_PAGES_LOCKED)
                MmUnlockPages(stripes[i].mdl);

            IoFreeMdl(stripes[i].mdl);
        }
    }

    if (wtc->parity1_mdl) {
        if (wtc->parity1_mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->parity1_mdl);

        IoFreeMdl(wtc->parity1_mdl);
        wtc->parity1_mdl = NULL;
    }

    if (wtc->parity2_mdl) {
        if (wtc->parity2_mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->parity2_mdl);

        IoFreeMdl(wtc->parity2_mdl);
        wtc->parity2_mdl = NULL;
    }

    if (wtc->mdl) {
        if (wtc->mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->mdl);

        IoFreeMdl(wtc->mdl);
        wtc->mdl = NULL;
    }

    if (wtc->parity1) {
        ExFreePool(wtc->parity1);
        wtc->parity1 = NULL;
    }

    if (wtc->parity2) {
        ExFreePool(wtc->parity2);
        wtc->parity2 = NULL;
    }

    if (wtc->scratch) {
        ExFreePool(wtc->scratch);
        wtc->scratch = NULL;
    }

    ExFreePool(stripes);
    return Status;
}

__attribute__((nonnull(1,4,5)))
void get_raid56_lock_range(chunk* c, uint64_t address, uint64_t length, uint64_t* lockaddr, uint64_t* locklen) {
    uint64_t startoff, endoff;
    uint16_t startoffstripe, endoffstripe, datastripes;

    datastripes = c->chunk_item->num_stripes - (c->chunk_item->type & BLOCK_FLAG_RAID5 ? 1 : 2);

    get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, datastripes, &startoff, &startoffstripe);
    get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, datastripes, &endoff, &endoffstripe);

    startoff -= startoff % c->chunk_item->stripe_length;
    endoff = sector_align(endoff, c->chunk_item->stripe_length);

    *lockaddr = c->offset + (startoff * datastripes);
    *locklen = (endoff - startoff) * datastripes;
}

__attribute__((nonnull(1,3)))
NTSTATUS write_data_complete(device_extension* Vcb, uint64_t address, void* data, uint32_t length, PIRP Irp, chunk* c, bool file_write, uint64_t irp_offset, ULONG priority) {
    write_data_context wtc;
    NTSTATUS Status;
    uint64_t lockaddr, locklen;

    KeInitializeEvent(&wtc.Event, NotificationEvent, false);
    InitializeListHead(&wtc.stripes);
    wtc.stripes_left = 0;
    wtc.parity1 = wtc.parity2 = wtc.scratch = NULL;
    wtc.mdl = wtc.parity1_mdl = wtc.parity2_mdl = NULL;

    if (!c) {
        c = get_chunk_from_address(Vcb, address);
        if (!c) {
            ERR("could not get chunk for address %I64x\n", address);
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6) {
        get_raid56_lock_range(c, address, length, &lockaddr, &locklen);
        chunk_lock_range(Vcb, c, lockaddr, locklen);
    }

    _SEH2_TRY {
        Status = write_data(Vcb, address, data, length, &wtc, Irp, c, file_write, irp_offset, priority);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08lx\n", Status);

        if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)
            chunk_unlock_range(Vcb, c, lockaddr, locklen);

        free_write_data_stripes(&wtc);
        return Status;
    }

    if (wtc.stripes.Flink != &wtc.stripes) {
        // launch writes and wait
        LIST_ENTRY* le = wtc.stripes.Flink;
        bool no_wait = true;

        while (le != &wtc.stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (stripe->status != WriteDataStatus_Ignore) {
                IoCallDriver(stripe->device->devobj, stripe->Irp);
                no_wait = false;
            }

            le = le->Flink;
        }

        if (!no_wait)
            KeWaitForSingleObject(&wtc.Event, Executive, KernelMode, false, NULL);

        le = wtc.stripes.Flink;
        while (le != &wtc.stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;

                log_device_error(Vcb, stripe->device, BTRFS_DEV_STAT_WRITE_ERRORS);
                break;
            }

            le = le->Flink;
        }

        free_write_data_stripes(&wtc);
    }

    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)
        chunk_unlock_range(Vcb, c, lockaddr, locklen);

    return Status;
}

__attribute__((nonnull(2,3)))
_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall write_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    write_data_stripe* stripe = conptr;
    write_data_context* context = (write_data_context*)stripe->context;
    LIST_ENTRY* le;

    UNUSED(DeviceObject);

    // FIXME - we need a lock here

    if (stripe->status == WriteDataStatus_Cancelling) {
        stripe->status = WriteDataStatus_Cancelled;
        goto end;
    }

    stripe->iosb = Irp->IoStatus;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        stripe->status = WriteDataStatus_Success;
    } else {
        le = context->stripes.Flink;

        stripe->status = WriteDataStatus_Error;

        while (le != &context->stripes) {
            write_data_stripe* s2 = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (s2->status == WriteDataStatus_Pending) {
                s2->status = WriteDataStatus_Cancelling;
                IoCancelIrp(s2->Irp);
            }

            le = le->Flink;
        }
    }

end:
    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

__attribute__((nonnull(1)))
void free_write_data_stripes(write_data_context* wtc) {
    LIST_ENTRY* le;
    PMDL last_mdl = NULL;

    if (wtc->parity1_mdl) {
        if (wtc->parity1_mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->parity1_mdl);

        IoFreeMdl(wtc->parity1_mdl);
    }

    if (wtc->parity2_mdl) {
        if (wtc->parity2_mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->parity2_mdl);

        IoFreeMdl(wtc->parity2_mdl);
    }

    if (wtc->mdl) {
        if (wtc->mdl->MdlFlags & MDL_PAGES_LOCKED)
            MmUnlockPages(wtc->mdl);

        IoFreeMdl(wtc->mdl);
    }

    if (wtc->parity1)
        ExFreePool(wtc->parity1);

    if (wtc->parity2)
        ExFreePool(wtc->parity2);

    if (wtc->scratch)
        ExFreePool(wtc->scratch);

    le = wtc->stripes.Flink;
    while (le != &wtc->stripes) {
        write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

        if (stripe->mdl && stripe->mdl != last_mdl) {
            if (stripe->mdl->MdlFlags & MDL_PAGES_LOCKED)
                MmUnlockPages(stripe->mdl);

            IoFreeMdl(stripe->mdl);
        }

        last_mdl = stripe->mdl;

        if (stripe->Irp)
            IoFreeIrp(stripe->Irp);

        le = le->Flink;
    }

    while (!IsListEmpty(&wtc->stripes)) {
        write_data_stripe* stripe = CONTAINING_RECORD(RemoveHeadList(&wtc->stripes), write_data_stripe, list_entry);

        ExFreePool(stripe);
    }
}

__attribute__((nonnull(1,2,3)))
void add_extent(_In_ fcb* fcb, _In_ LIST_ENTRY* prevextle, _In_ __drv_aliasesMem extent* newext) {
    LIST_ENTRY* le = prevextle->Flink;

    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (ext->offset >= newext->offset) {
            InsertHeadList(ext->list_entry.Blink, &newext->list_entry);
            return;
        }

        le = le->Flink;
    }

    InsertTailList(&fcb->extents, &newext->list_entry);
}

__attribute__((nonnull(1,2,6)))
NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, uint64_t start_data, uint64_t end_data, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;

    le = fcb->extents.Flink;

    while (le != &fcb->extents) {
        LIST_ENTRY* le2 = le->Flink;
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext->ignore) {
            EXTENT_DATA* ed = &ext->extent_data;
            uint64_t len;

            if (ed->type == EXTENT_TYPE_INLINE)
                len = ed->decoded_size;
            else
                len = ((EXTENT_DATA2*)ed->data)->num_bytes;

            if (ext->offset < end_data && ext->offset + len > start_data) {
                if (ed->type == EXTENT_TYPE_INLINE) {
                    if (start_data <= ext->offset && end_data >= ext->offset + len) { // remove all
                        remove_fcb_extent(fcb, ext, rollback);

                        fcb->inode_item.st_blocks -= len;
                        fcb->inode_item_changed = true;
                    } else {
                        ERR("trying to split inline extent\n");
#ifdef DEBUG_PARANOID
                        int3;
#endif
                        return STATUS_INTERNAL_ERROR;
                    }
                } else {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                    if (start_data <= ext->offset && end_data >= ext->offset + len) { // remove all
                        if (ed2->size != 0) {
                            chunk* c;

                            fcb->inode_item.st_blocks -= len;
                            fcb->inode_item_changed = true;

                            c = get_chunk_from_address(Vcb, ed2->address);

                            if (!c) {
                                ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, -1,
                                                                   fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("update_changed_extent_ref returned %08lx\n", Status);
                                    goto end;
                                }
                            }
                        }

                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data <= ext->offset && end_data < ext->offset + len) { // remove beginning
                        EXTENT_DATA2* ned2;
                        extent* newext;

                        if (ed2->size != 0) {
                            fcb->inode_item.st_blocks -= end_data - ext->offset;
                            fcb->inode_item_changed = true;
                        }

                        newext = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }

                        ned2 = (EXTENT_DATA2*)newext->extent_data.data;

                        newext->extent_data.generation = Vcb->superblock.generation;
                        newext->extent_data.decoded_size = ed->decoded_size;
                        newext->extent_data.compression = ed->compression;
                        newext->extent_data.encryption = ed->encryption;
                        newext->extent_data.encoding = ed->encoding;
                        newext->extent_data.type = ed->type;
                        ned2->address = ed2->address;
                        ned2->size = ed2->size;
                        ned2->offset = ed2->offset + (end_data - ext->offset);
                        ned2->num_bytes = ed2->num_bytes - (end_data - ext->offset);

                        newext->offset = end_data;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext->unique = ext->unique;
                        newext->ignore = false;
                        newext->inserted = true;

                        if (ext->csum) {
                            if (ed->compression == BTRFS_COMPRESSION_NONE) {
                                newext->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ned2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext);
                                    goto end;
                                }

                                RtlCopyMemory(newext->csum, (uint8_t*)ext->csum + (((end_data - ext->offset) * Vcb->csum_size) >> Vcb->sector_shift),
                                              (ULONG)((ned2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift));
                            } else {
                                newext->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext);
                                    goto end;
                                }

                                RtlCopyMemory(newext->csum, ext->csum, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift));
                            }
                        } else
                            newext->csum = NULL;

                        add_extent(fcb, &ext->list_entry, newext);

                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data > ext->offset && end_data >= ext->offset + len) { // remove end
                        EXTENT_DATA2* ned2;
                        extent* newext;

                        if (ed2->size != 0) {
                            fcb->inode_item.st_blocks -= ext->offset + len - start_data;
                            fcb->inode_item_changed = true;
                        }

                        newext = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }

                        ned2 = (EXTENT_DATA2*)newext->extent_data.data;

                        newext->extent_data.generation = Vcb->superblock.generation;
                        newext->extent_data.decoded_size = ed->decoded_size;
                        newext->extent_data.compression = ed->compression;
                        newext->extent_data.encryption = ed->encryption;
                        newext->extent_data.encoding = ed->encoding;
                        newext->extent_data.type = ed->type;
                        ned2->address = ed2->address;
                        ned2->size = ed2->size;
                        ned2->offset = ed2->offset;
                        ned2->num_bytes = start_data - ext->offset;

                        newext->offset = ext->offset;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext->unique = ext->unique;
                        newext->ignore = false;
                        newext->inserted = true;

                        if (ext->csum) {
                            if (ed->compression == BTRFS_COMPRESSION_NONE) {
                                newext->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ned2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext);
                                    goto end;
                                }

                                RtlCopyMemory(newext->csum, ext->csum, (ULONG)((ned2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift));
                            } else {
                                newext->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext);
                                    goto end;
                                }

                                RtlCopyMemory(newext->csum, ext->csum, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift));
                            }
                        } else
                            newext->csum = NULL;

                        InsertHeadList(&ext->list_entry, &newext->list_entry);

                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data > ext->offset && end_data < ext->offset + len) { // remove middle
                        EXTENT_DATA2 *neda2, *nedb2;
                        extent *newext1, *newext2;

                        if (ed2->size != 0) {
                            chunk* c;

                            fcb->inode_item.st_blocks -= end_data - start_data;
                            fcb->inode_item_changed = true;

                            c = get_chunk_from_address(Vcb, ed2->address);

                            if (!c) {
                                ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                                                   fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("update_changed_extent_ref returned %08lx\n", Status);
                                    goto end;
                                }
                            }
                        }

                        newext1 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!newext1) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }

                        newext2 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!newext2) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(newext1);
                            goto end;
                        }

                        neda2 = (EXTENT_DATA2*)newext1->extent_data.data;

                        newext1->extent_data.generation = Vcb->superblock.generation;
                        newext1->extent_data.decoded_size = ed->decoded_size;
                        newext1->extent_data.compression = ed->compression;
                        newext1->extent_data.encryption = ed->encryption;
                        newext1->extent_data.encoding = ed->encoding;
                        newext1->extent_data.type = ed->type;
                        neda2->address = ed2->address;
                        neda2->size = ed2->size;
                        neda2->offset = ed2->offset;
                        neda2->num_bytes = start_data - ext->offset;

                        nedb2 = (EXTENT_DATA2*)newext2->extent_data.data;

                        newext2->extent_data.generation = Vcb->superblock.generation;
                        newext2->extent_data.decoded_size = ed->decoded_size;
                        newext2->extent_data.compression = ed->compression;
                        newext2->extent_data.encryption = ed->encryption;
                        newext2->extent_data.encoding = ed->encoding;
                        newext2->extent_data.type = ed->type;
                        nedb2->address = ed2->address;
                        nedb2->size = ed2->size;
                        nedb2->offset = ed2->offset + (end_data - ext->offset);
                        nedb2->num_bytes = ext->offset + len - end_data;

                        newext1->offset = ext->offset;
                        newext1->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext1->unique = ext->unique;
                        newext1->ignore = false;
                        newext1->inserted = true;

                        newext2->offset = end_data;
                        newext2->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext2->unique = ext->unique;
                        newext2->ignore = false;
                        newext2->inserted = true;

                        if (ext->csum) {
                            if (ed->compression == BTRFS_COMPRESSION_NONE) {
                                newext1->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((neda2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext1->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext1);
                                    ExFreePool(newext2);
                                    goto end;
                                }

                                newext2->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((nedb2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext2->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext1->csum);
                                    ExFreePool(newext1);
                                    ExFreePool(newext2);
                                    goto end;
                                }

                                RtlCopyMemory(newext1->csum, ext->csum, (ULONG)((neda2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift));
                                RtlCopyMemory(newext2->csum, (uint8_t*)ext->csum + (((end_data - ext->offset) * Vcb->csum_size) >> Vcb->sector_shift),
                                              (ULONG)((nedb2->num_bytes * Vcb->csum_size) >> Vcb->sector_shift));
                            } else {
                                newext1->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext1->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext1);
                                    ExFreePool(newext2);
                                    goto end;
                                }

                                newext2->csum = ExAllocatePoolWithTag(PagedPool, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift), ALLOC_TAG);
                                if (!newext2->csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ExFreePool(newext1->csum);
                                    ExFreePool(newext1);
                                    ExFreePool(newext2);
                                    goto end;
                                }

                                RtlCopyMemory(newext1->csum, ext->csum, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift));
                                RtlCopyMemory(newext2->csum, ext->csum, (ULONG)((ed2->size * Vcb->csum_size) >> Vcb->sector_shift));
                            }
                        } else {
                            newext1->csum = NULL;
                            newext2->csum = NULL;
                        }

                        InsertHeadList(&ext->list_entry, &newext1->list_entry);
                        add_extent(fcb, &newext1->list_entry, newext2);

                        remove_fcb_extent(fcb, ext, rollback);
                    }
                }
            }
        }

        le = le2;
    }

    Status = STATUS_SUCCESS;

end:
    fcb->extents_changed = true;
    mark_fcb_dirty(fcb);

    return Status;
}

__attribute__((nonnull(1,2,3)))
static void add_insert_extent_rollback(LIST_ENTRY* rollback, fcb* fcb, extent* ext) {
    rollback_extent* re;

    re = ExAllocatePoolWithTag(NonPagedPool, sizeof(rollback_extent), ALLOC_TAG);
    if (!re) {
        ERR("out of memory\n");
        return;
    }

    re->fcb = fcb;
    re->ext = ext;

    add_rollback(rollback, ROLLBACK_INSERT_EXTENT, re);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(suppress: 28194)
#endif
__attribute__((nonnull(1,3,7)))
NTSTATUS add_extent_to_fcb(_In_ fcb* fcb, _In_ uint64_t offset, _In_reads_bytes_(edsize) EXTENT_DATA* ed, _In_ uint16_t edsize,
                           _In_ bool unique, _In_opt_ _When_(return >= 0, __drv_aliasesMem) void* csum, _In_ LIST_ENTRY* rollback) {
    extent* ext;
    LIST_ENTRY* le;

    ext = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + edsize, ALLOC_TAG);
    if (!ext) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ext->offset = offset;
    ext->datalen = edsize;
    ext->unique = unique;
    ext->ignore = false;
    ext->inserted = true;
    ext->csum = csum;

    RtlCopyMemory(&ext->extent_data, ed, edsize);

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* oldext = CONTAINING_RECORD(le, extent, list_entry);

        if (oldext->offset >= offset) {
            InsertHeadList(le->Blink, &ext->list_entry);
            goto end;
        }

        le = le->Flink;
    }

    InsertTailList(&fcb->extents, &ext->list_entry);

end:
    add_insert_extent_rollback(rollback, fcb, ext);

    return STATUS_SUCCESS;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

__attribute__((nonnull(1, 2, 3)))
static void remove_fcb_extent(fcb* fcb, extent* ext, LIST_ENTRY* rollback) {
    if (!ext->ignore) {
        rollback_extent* re;

        ext->ignore = true;

        re = ExAllocatePoolWithTag(NonPagedPool, sizeof(rollback_extent), ALLOC_TAG);
        if (!re) {
            ERR("out of memory\n");
            return;
        }

        re->fcb = fcb;
        re->ext = ext;

        add_rollback(rollback, ROLLBACK_DELETE_EXTENT, re);
    }
}

_Requires_lock_held_(c->lock)
_When_(return != 0, _Releases_lock_(c->lock))
__attribute__((nonnull(1,2,3,9)))
bool insert_extent_chunk(_In_ device_extension* Vcb, _In_ fcb* fcb, _In_ chunk* c, _In_ uint64_t start_data, _In_ uint64_t length, _In_ bool prealloc, _In_opt_ void* data,
                         _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback, _In_ uint8_t compression, _In_ uint64_t decoded_size, _In_ bool file_write, _In_ uint64_t irp_offset) {
    uint64_t address;
    NTSTATUS Status;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    uint16_t edsize = (uint16_t)(offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2));
    void* csum = NULL;

    TRACE("(%p, (%I64x, %I64x), %I64x, %I64x, %I64x, %u, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, c->offset, start_data, length, prealloc, data, rollback);

    if (!find_data_address_in_chunk(Vcb, c, length, &address))
        return false;

    // add extent data to inode
    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return false;
    }

    ed->generation = Vcb->superblock.generation;
    ed->decoded_size = decoded_size;
    ed->compression = compression;
    ed->encryption = BTRFS_ENCRYPTION_NONE;
    ed->encoding = BTRFS_ENCODING_NONE;
    ed->type = prealloc ? EXTENT_TYPE_PREALLOC : EXTENT_TYPE_REGULAR;

    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = address;
    ed2->size = length;
    ed2->offset = 0;
    ed2->num_bytes = decoded_size;

    if (!prealloc && data && !(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
        ULONG sl = (ULONG)(length >> Vcb->sector_shift);

        csum = ExAllocatePoolWithTag(PagedPool, sl * Vcb->csum_size, ALLOC_TAG);
        if (!csum) {
            ERR("out of memory\n");
            ExFreePool(ed);
            return false;
        }

        do_calc_job(Vcb, data, sl, csum);
    }

    Status = add_extent_to_fcb(fcb, start_data, ed, edsize, true, csum, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_extent_to_fcb returned %08lx\n", Status);
        if (csum) ExFreePool(csum);
        ExFreePool(ed);
        return false;
    }

    ExFreePool(ed);

    c->used += length;
    space_list_subtract(c, address, length, rollback);

    fcb->inode_item.st_blocks += decoded_size;

    fcb->extents_changed = true;
    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, true);

    add_changed_extent_ref(c, address, length, fcb->subvol->id, fcb->inode, start_data, 1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);

    ExReleaseResourceLite(&c->changed_extents_lock);

    release_chunk_lock(c, Vcb);

    if (data) {
        Status = write_data_complete(Vcb, address, data, (uint32_t)length, Irp, NULL, file_write, irp_offset,
                                     fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);
        if (!NT_SUCCESS(Status))
            ERR("write_data_complete returned %08lx\n", Status);
    }

    return true;
}

__attribute__((nonnull(1,2,5,7,10)))
static bool try_extend_data(device_extension* Vcb, fcb* fcb, uint64_t start_data, uint64_t length, void* data,
                            PIRP Irp, uint64_t* written, bool file_write, uint64_t irp_offset, LIST_ENTRY* rollback) {
    bool success = false;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    chunk* c;
    LIST_ENTRY* le;
    extent* ext = NULL;

    le = fcb->extents.Flink;

    while (le != &fcb->extents) {
        extent* nextext = CONTAINING_RECORD(le, extent, list_entry);

        if (!nextext->ignore) {
            if (nextext->offset == start_data) {
                ext = nextext;
                break;
            } else if (nextext->offset > start_data)
                break;

            ext = nextext;
        }

        le = le->Flink;
    }

    if (!ext)
        return false;

    ed = &ext->extent_data;

    if (ed->type != EXTENT_TYPE_REGULAR && ed->type != EXTENT_TYPE_PREALLOC) {
        TRACE("not extending extent which is not regular or prealloc\n");
        return false;
    }

    ed2 = (EXTENT_DATA2*)ed->data;

    if (ext->offset + ed2->num_bytes != start_data) {
        TRACE("last EXTENT_DATA does not run up to start_data (%I64x + %I64x != %I64x)\n", ext->offset, ed2->num_bytes, start_data);
        return false;
    }

    c = get_chunk_from_address(Vcb, ed2->address);

    if (c->reloc || c->readonly || c->chunk_item->type != Vcb->data_flags)
        return false;

    acquire_chunk_lock(c, Vcb);

    if (length > c->chunk_item->size - c->used) {
        release_chunk_lock(c, Vcb);
        return false;
    }

    if (!c->cache_loaded) {
        NTSTATUS Status = load_cache_chunk(Vcb, c, NULL);

        if (!NT_SUCCESS(Status)) {
            ERR("load_cache_chunk returned %08lx\n", Status);
            release_chunk_lock(c, Vcb);
            return false;
        }
    }

    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if (s->address == ed2->address + ed2->size) {
            uint64_t newlen = min(min(s->size, length), MAX_EXTENT_SIZE);

            success = insert_extent_chunk(Vcb, fcb, c, start_data, newlen, false, data, Irp, rollback, BTRFS_COMPRESSION_NONE, newlen, file_write, irp_offset);

            if (success)
                *written += newlen;
            else
                release_chunk_lock(c, Vcb);

            return success;
        } else if (s->address > ed2->address + ed2->size)
            break;

        le = le->Flink;
    }

    release_chunk_lock(c, Vcb);

    return false;
}

__attribute__((nonnull(1)))
static NTSTATUS insert_chunk_fragmented(fcb* fcb, uint64_t start, uint64_t length, uint8_t* data, bool prealloc, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    uint64_t flags = fcb->Vcb->data_flags;
    bool page_file = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE;
    NTSTATUS Status;
    chunk* c;

    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

    // first create as many chunks as we can
    do {
        Status = alloc_chunk(fcb->Vcb, flags, &c, false);
    } while (NT_SUCCESS(Status));

    if (Status != STATUS_DISK_FULL) {
        ERR("alloc_chunk returned %08lx\n", Status);
        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
        return Status;
    }

    le = fcb->Vcb->chunks.Flink;
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, fcb->Vcb);

            if (c->chunk_item->type == flags) {
                while (!IsListEmpty(&c->space_size) && length > 0) {
                    space* s = CONTAINING_RECORD(c->space_size.Flink, space, list_entry_size);
                    uint64_t extlen = min(length, s->size);

                    if (insert_extent_chunk(fcb->Vcb, fcb, c, start, extlen, prealloc && !page_file, data, NULL, rollback, BTRFS_COMPRESSION_NONE, extlen, false, 0)) {
                        start += extlen;
                        length -= extlen;
                        if (data) data += extlen;

                        acquire_chunk_lock(c, fcb->Vcb);
                    }
                }
            }

            release_chunk_lock(c, fcb->Vcb);

            if (length == 0)
                break;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    return length == 0 ? STATUS_SUCCESS : STATUS_DISK_FULL;
}

__attribute__((nonnull(1,4)))
static NTSTATUS insert_prealloc_extent(fcb* fcb, uint64_t start, uint64_t length, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    chunk* c;
    uint64_t flags;
    NTSTATUS Status;
    bool page_file = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE;

    flags = fcb->Vcb->data_flags;

    do {
        uint64_t extlen = min(MAX_EXTENT_SIZE, length);

        ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

        le = fcb->Vcb->chunks.Flink;
        while (le != &fcb->Vcb->chunks) {
            c = CONTAINING_RECORD(le, chunk, list_entry);

            if (!c->readonly && !c->reloc) {
                acquire_chunk_lock(c, fcb->Vcb);

                if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= extlen) {
                    if (insert_extent_chunk(fcb->Vcb, fcb, c, start, extlen, !page_file, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, extlen, false, 0)) {
                        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
                        goto cont;
                    }
                }

                release_chunk_lock(c, fcb->Vcb);
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

        ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, true);

        Status = alloc_chunk(fcb->Vcb, flags, &c, false);

        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

        if (!NT_SUCCESS(Status)) {
            ERR("alloc_chunk returned %08lx\n", Status);
            goto end;
        }

        acquire_chunk_lock(c, fcb->Vcb);

        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= extlen) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start, extlen, !page_file, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, extlen, false, 0))
                goto cont;
        }

        release_chunk_lock(c, fcb->Vcb);

        Status = insert_chunk_fragmented(fcb, start, length, NULL, true, rollback);
        if (!NT_SUCCESS(Status))
            ERR("insert_chunk_fragmented returned %08lx\n", Status);

        goto end;

cont:
        length -= extlen;
        start += extlen;
    } while (length > 0);

    Status = STATUS_SUCCESS;

end:
    return Status;
}

__attribute__((nonnull(1,2,5,9)))
static NTSTATUS insert_extent(device_extension* Vcb, fcb* fcb, uint64_t start_data, uint64_t length, void* data,
                              PIRP Irp, bool file_write, uint64_t irp_offset, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    chunk* c;
    uint64_t flags, orig_length = length, written = 0;

    TRACE("(%p, (%I64x, %I64x), %I64x, %I64x, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, length, data);

    if (start_data > 0) {
        try_extend_data(Vcb, fcb, start_data, length, data, Irp, &written, file_write, irp_offset, rollback);

        if (written == length)
            return STATUS_SUCCESS;
        else if (written > 0) {
            start_data += written;
            irp_offset += written;
            length -= written;
            data = &((uint8_t*)data)[written];
        }
    }

    flags = Vcb->data_flags;

    while (written < orig_length) {
        uint64_t newlen = min(length, MAX_EXTENT_SIZE);
        bool done = false;

        // Rather than necessarily writing the whole extent at once, we deal with it in blocks of 128 MB.
        // First, see if we can write the extent part to an existing chunk.

        ExAcquireResourceSharedLite(&Vcb->chunk_lock, true);

        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            c = CONTAINING_RECORD(le, chunk, list_entry);

            if (!c->readonly && !c->reloc) {
                acquire_chunk_lock(c, Vcb);

                if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= newlen &&
                    insert_extent_chunk(Vcb, fcb, c, start_data, newlen, false, data, Irp, rollback, BTRFS_COMPRESSION_NONE, newlen, file_write, irp_offset)) {
                    written += newlen;

                    if (written == orig_length) {
                        ExReleaseResourceLite(&Vcb->chunk_lock);
                        return STATUS_SUCCESS;
                    } else {
                        done = true;
                        start_data += newlen;
                        irp_offset += newlen;
                        length -= newlen;
                        data = &((uint8_t*)data)[newlen];
                        break;
                    }
                } else
                    release_chunk_lock(c, Vcb);
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->chunk_lock);

        if (done) continue;

        // Otherwise, see if we can put it in a new chunk.

        ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, true);

        Status = alloc_chunk(Vcb, flags, &c, false);

        ExReleaseResourceLite(&Vcb->chunk_lock);

        if (!NT_SUCCESS(Status)) {
            ERR("alloc_chunk returned %08lx\n", Status);
            return Status;
        }

        if (c) {
            acquire_chunk_lock(c, Vcb);

            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= newlen &&
                insert_extent_chunk(Vcb, fcb, c, start_data, newlen, false, data, Irp, rollback, BTRFS_COMPRESSION_NONE, newlen, file_write, irp_offset)) {
                written += newlen;

                if (written == orig_length)
                    return STATUS_SUCCESS;
                else {
                    done = true;
                    start_data += newlen;
                    irp_offset += newlen;
                    length -= newlen;
                    data = &((uint8_t*)data)[newlen];
                }
            } else
                release_chunk_lock(c, Vcb);
        }

        if (!done) {
            Status = insert_chunk_fragmented(fcb, start_data, length, data, false, rollback);
            if (!NT_SUCCESS(Status))
                ERR("insert_chunk_fragmented returned %08lx\n", Status);

            return Status;
        }
    }

    return STATUS_DISK_FULL;
}

__attribute__((nonnull(1,4)))
NTSTATUS truncate_file(fcb* fcb, uint64_t end, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;

    // FIXME - convert into inline extent if short enough

    if (end > 0 && fcb_is_inline(fcb)) {
        uint8_t* buf;
        bool make_inline = end <= fcb->Vcb->options.max_inline;

        buf = ExAllocatePoolWithTag(PagedPool, (ULONG)(make_inline ? (offsetof(EXTENT_DATA, data[0]) + end) : sector_align(end, fcb->Vcb->superblock.sector_size)), ALLOC_TAG);
        if (!buf) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = read_file(fcb, make_inline ? (buf + offsetof(EXTENT_DATA, data[0])) : buf, 0, end, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("read_file returned %08lx\n", Status);
            ExFreePool(buf);
            return Status;
        }

        Status = excise_extents(fcb->Vcb, fcb, 0, fcb->inode_item.st_size, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            ExFreePool(buf);
            return Status;
        }

        if (!make_inline) {
            RtlZeroMemory(buf + end, (ULONG)(sector_align(end, fcb->Vcb->superblock.sector_size) - end));

            Status = do_write_file(fcb, 0, sector_align(end, fcb->Vcb->superblock.sector_size), buf, Irp, false, 0, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("do_write_file returned %08lx\n", Status);
                ExFreePool(buf);
                return Status;
            }
        } else {
            EXTENT_DATA* ed = (EXTENT_DATA*)buf;

            ed->generation = fcb->Vcb->superblock.generation;
            ed->decoded_size = end;
            ed->compression = BTRFS_COMPRESSION_NONE;
            ed->encryption = BTRFS_ENCRYPTION_NONE;
            ed->encoding = BTRFS_ENCODING_NONE;
            ed->type = EXTENT_TYPE_INLINE;

            Status = add_extent_to_fcb(fcb, 0, ed, (uint16_t)(offsetof(EXTENT_DATA, data[0]) + end), false, NULL, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_extent_to_fcb returned %08lx\n", Status);
                ExFreePool(buf);
                return Status;
            }

            fcb->inode_item.st_blocks += end;

            fcb->inode_item.st_size = end;
            fcb->inode_item_changed = true;
            TRACE("setting st_size to %I64x\n", end);

            fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
            fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
            fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
        }

        ExFreePool(buf);
        return STATUS_SUCCESS;
    }

    Status = excise_extents(fcb->Vcb, fcb, sector_align(end, fcb->Vcb->superblock.sector_size),
                            sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08lx\n", Status);
        return Status;
    }

    fcb->inode_item.st_size = end;
    fcb->inode_item_changed = true;
    TRACE("setting st_size to %I64x\n", end);

    fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
    fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
    fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
    // FIXME - inform cache manager of this

    TRACE("fcb %p FileSize = %I64x\n", fcb, fcb->Header.FileSize.QuadPart);

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,6)))
NTSTATUS extend_file(fcb* fcb, file_ref* fileref, uint64_t end, bool prealloc, PIRP Irp, LIST_ENTRY* rollback) {
    uint64_t oldalloc, newalloc;
    bool cur_inline;
    NTSTATUS Status;

    TRACE("(%p, %p, %I64x, %u)\n", fcb, fileref, end, prealloc);

    if (fcb->ads) {
        if (end > 0xffff)
            return STATUS_DISK_FULL;

        return stream_set_end_of_file_information(fcb->Vcb, (uint16_t)end, fcb, fileref, false);
    } else {
        extent* ext = NULL;
        LIST_ENTRY* le;

        le = fcb->extents.Blink;
        while (le != &fcb->extents) {
            extent* ext2 = CONTAINING_RECORD(le, extent, list_entry);

            if (!ext2->ignore) {
                ext = ext2;
                break;
            }

            le = le->Blink;
        }

        oldalloc = 0;
        if (ext) {
            EXTENT_DATA* ed = &ext->extent_data;
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

            oldalloc = ext->offset + (ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes);
            cur_inline = ed->type == EXTENT_TYPE_INLINE;

            if (cur_inline && end > fcb->Vcb->options.max_inline) {
                uint64_t origlength, length;
                uint8_t* data;

                TRACE("giving inline file proper extents\n");

                origlength = ed->decoded_size;

                cur_inline = false;

                length = sector_align(origlength, fcb->Vcb->superblock.sector_size);

                data = ExAllocatePoolWithTag(PagedPool, (ULONG)length, ALLOC_TAG);
                if (!data) {
                    ERR("could not allocate %I64x bytes for data\n", length);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                Status = read_file(fcb, data, 0, origlength, NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("read_file returned %08lx\n", Status);
                    ExFreePool(data);
                    return Status;
                }

                RtlZeroMemory(data + origlength, (ULONG)(length - origlength));

                Status = excise_extents(fcb->Vcb, fcb, 0, fcb->inode_item.st_size, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("excise_extents returned %08lx\n", Status);
                    ExFreePool(data);
                    return Status;
                }

                Status = do_write_file(fcb, 0, length, data, Irp, false, 0, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_file returned %08lx\n", Status);
                    ExFreePool(data);
                    return Status;
                }

                oldalloc = ext->offset + length;

                ExFreePool(data);
            }

            if (cur_inline) {
                uint16_t edsize;

                if (end > oldalloc) {
                    edsize = (uint16_t)(offsetof(EXTENT_DATA, data[0]) + end - ext->offset);
                    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);

                    if (!ed) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    ed->generation = fcb->Vcb->superblock.generation;
                    ed->decoded_size = end - ext->offset;
                    ed->compression = BTRFS_COMPRESSION_NONE;
                    ed->encryption = BTRFS_ENCRYPTION_NONE;
                    ed->encoding = BTRFS_ENCODING_NONE;
                    ed->type = EXTENT_TYPE_INLINE;

                    Status = read_file(fcb, ed->data, ext->offset, oldalloc, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_file returned %08lx\n", Status);
                        ExFreePool(ed);
                        return Status;
                    }

                    RtlZeroMemory(ed->data + oldalloc - ext->offset, (ULONG)(end - oldalloc));

                    remove_fcb_extent(fcb, ext, rollback);

                    Status = add_extent_to_fcb(fcb, ext->offset, ed, edsize, ext->unique, NULL, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("add_extent_to_fcb returned %08lx\n", Status);
                        ExFreePool(ed);
                        return Status;
                    }

                    ExFreePool(ed);

                    fcb->extents_changed = true;
                    mark_fcb_dirty(fcb);
                }

                TRACE("extending inline file (oldalloc = %I64x, end = %I64x)\n", oldalloc, end);

                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %I64x\n", end);

                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            } else {
                newalloc = sector_align(end, fcb->Vcb->superblock.sector_size);

                if (newalloc > oldalloc) {
                    if (prealloc) {
                        // FIXME - try and extend previous extent first

                        Status = insert_prealloc_extent(fcb, oldalloc, newalloc - oldalloc, rollback);

                        if (!NT_SUCCESS(Status) && Status != STATUS_DISK_FULL) {
                            ERR("insert_prealloc_extent returned %08lx\n", Status);
                            return Status;
                        }
                    }

                    fcb->extents_changed = true;
                }

                fcb->inode_item.st_size = end;
                fcb->inode_item_changed = true;
                mark_fcb_dirty(fcb);

                TRACE("setting st_size to %I64x\n", end);

                TRACE("newalloc = %I64x\n", newalloc);

                fcb->Header.AllocationSize.QuadPart = newalloc;
                fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        } else {
            if (end > fcb->Vcb->options.max_inline) {
                newalloc = sector_align(end, fcb->Vcb->superblock.sector_size);

                if (prealloc) {
                    Status = insert_prealloc_extent(fcb, 0, newalloc, rollback);

                    if (!NT_SUCCESS(Status) && Status != STATUS_DISK_FULL) {
                        ERR("insert_prealloc_extent returned %08lx\n", Status);
                        return Status;
                    }
                }

                fcb->extents_changed = true;
                fcb->inode_item_changed = true;
                mark_fcb_dirty(fcb);

                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %I64x\n", end);

                TRACE("newalloc = %I64x\n", newalloc);

                fcb->Header.AllocationSize.QuadPart = newalloc;
                fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            } else {
                EXTENT_DATA* ed;
                uint16_t edsize;

                edsize = (uint16_t)(offsetof(EXTENT_DATA, data[0]) + end);
                ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);

                if (!ed) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                ed->generation = fcb->Vcb->superblock.generation;
                ed->decoded_size = end;
                ed->compression = BTRFS_COMPRESSION_NONE;
                ed->encryption = BTRFS_ENCRYPTION_NONE;
                ed->encoding = BTRFS_ENCODING_NONE;
                ed->type = EXTENT_TYPE_INLINE;

                RtlZeroMemory(ed->data, (ULONG)end);

                Status = add_extent_to_fcb(fcb, 0, ed, edsize, false, NULL, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_extent_to_fcb returned %08lx\n", Status);
                    ExFreePool(ed);
                    return Status;
                }

                ExFreePool(ed);

                fcb->extents_changed = true;
                fcb->inode_item_changed = true;
                mark_fcb_dirty(fcb);

                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %I64x\n", end);

                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        }
    }

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2,5,6,11)))
static NTSTATUS do_write_file_prealloc(fcb* fcb, extent* ext, uint64_t start_data, uint64_t end_data, void* data, uint64_t* written,
                                       PIRP Irp, bool file_write, uint64_t irp_offset, ULONG priority, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed = &ext->extent_data;
    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
    NTSTATUS Status;
    chunk* c = NULL;

    if (start_data <= ext->offset && end_data >= ext->offset + ed2->num_bytes) { // replace all
        extent* newext;

        newext = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(&newext->extent_data, &ext->extent_data, ext->datalen);

        newext->extent_data.type = EXTENT_TYPE_REGULAR;

        Status = write_data_complete(fcb->Vcb, ed2->address + ed2->offset, (uint8_t*)data + ext->offset - start_data, (uint32_t)ed2->num_bytes, Irp,
                                     NULL, file_write, irp_offset + ext->offset - start_data, priority);
        if (!NT_SUCCESS(Status)) {
            ERR("write_data_complete returned %08lx\n", Status);
            return Status;
        }

        if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
            ULONG sl = (ULONG)(ed2->num_bytes >> fcb->Vcb->sector_shift);
            void* csum = ExAllocatePoolWithTag(PagedPool, sl * fcb->Vcb->csum_size, ALLOC_TAG);

            if (!csum) {
                ERR("out of memory\n");
                ExFreePool(newext);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            do_calc_job(fcb->Vcb, (uint8_t*)data + ext->offset - start_data, sl, csum);

            newext->csum = csum;
        } else
            newext->csum = NULL;

        *written = ed2->num_bytes;

        newext->offset = ext->offset;
        newext->datalen = ext->datalen;
        newext->unique = ext->unique;
        newext->ignore = false;
        newext->inserted = true;
        InsertHeadList(&ext->list_entry, &newext->list_entry);

        add_insert_extent_rollback(rollback, fcb, newext);

        remove_fcb_extent(fcb, ext, rollback);

        c = get_chunk_from_address(fcb->Vcb, ed2->address);
    } else if (start_data <= ext->offset && end_data < ext->offset + ed2->num_bytes) { // replace beginning
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2;

        newext1 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newext2 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(&newext1->extent_data, &ext->extent_data, ext->datalen);
        newext1->extent_data.type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)newext1->extent_data.data;
        ned2->num_bytes = end_data - ext->offset;

        RtlCopyMemory(&newext2->extent_data, &ext->extent_data, ext->datalen);
        ned2 = (EXTENT_DATA2*)newext2->extent_data.data;
        ned2->offset += end_data - ext->offset;
        ned2->num_bytes -= end_data - ext->offset;

        Status = write_data_complete(fcb->Vcb, ed2->address + ed2->offset, (uint8_t*)data + ext->offset - start_data, (uint32_t)(end_data - ext->offset),
                                     Irp, NULL, file_write, irp_offset + ext->offset - start_data, priority);
        if (!NT_SUCCESS(Status)) {
            ERR("write_data_complete returned %08lx\n", Status);
            ExFreePool(newext1);
            ExFreePool(newext2);
            return Status;
        }

        if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
            ULONG sl = (ULONG)((end_data - ext->offset) >> fcb->Vcb->sector_shift);
            void* csum = ExAllocatePoolWithTag(PagedPool, sl * fcb->Vcb->csum_size, ALLOC_TAG);

            if (!csum) {
                ERR("out of memory\n");
                ExFreePool(newext1);
                ExFreePool(newext2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            do_calc_job(fcb->Vcb, (uint8_t*)data + ext->offset - start_data, sl, csum);

            newext1->csum = csum;
        } else
            newext1->csum = NULL;

        *written = end_data - ext->offset;

        newext1->offset = ext->offset;
        newext1->datalen = ext->datalen;
        newext1->unique = ext->unique;
        newext1->ignore = false;
        newext1->inserted = true;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);

        add_insert_extent_rollback(rollback, fcb, newext1);

        newext2->offset = end_data;
        newext2->datalen = ext->datalen;
        newext2->unique = ext->unique;
        newext2->ignore = false;
        newext2->inserted = true;
        newext2->csum = NULL;
        add_extent(fcb, &newext1->list_entry, newext2);

        add_insert_extent_rollback(rollback, fcb, newext2);

        c = get_chunk_from_address(fcb->Vcb, ed2->address);

        if (!c)
            ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                                fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08lx\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    } else if (start_data > ext->offset && end_data >= ext->offset + ed2->num_bytes) { // replace end
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2;

        newext1 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newext2 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(&newext1->extent_data, &ext->extent_data, ext->datalen);

        ned2 = (EXTENT_DATA2*)newext1->extent_data.data;
        ned2->num_bytes = start_data - ext->offset;

        RtlCopyMemory(&newext2->extent_data, &ext->extent_data, ext->datalen);

        newext2->extent_data.type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)newext2->extent_data.data;
        ned2->offset += start_data - ext->offset;
        ned2->num_bytes = ext->offset + ed2->num_bytes - start_data;

        Status = write_data_complete(fcb->Vcb, ed2->address + ned2->offset, data, (uint32_t)ned2->num_bytes, Irp, NULL, file_write, irp_offset, priority);
        if (!NT_SUCCESS(Status)) {
            ERR("write_data_complete returned %08lx\n", Status);
            ExFreePool(newext1);
            ExFreePool(newext2);
            return Status;
        }

        if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
            ULONG sl = (ULONG)(ned2->num_bytes >> fcb->Vcb->sector_shift);
            void* csum = ExAllocatePoolWithTag(PagedPool, sl * fcb->Vcb->csum_size, ALLOC_TAG);

            if (!csum) {
                ERR("out of memory\n");
                ExFreePool(newext1);
                ExFreePool(newext2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            do_calc_job(fcb->Vcb, data, sl, csum);

            newext2->csum = csum;
        } else
            newext2->csum = NULL;

        *written = ned2->num_bytes;

        newext1->offset = ext->offset;
        newext1->datalen = ext->datalen;
        newext1->unique = ext->unique;
        newext1->ignore = false;
        newext1->inserted = true;
        newext1->csum = NULL;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);

        add_insert_extent_rollback(rollback, fcb, newext1);

        newext2->offset = start_data;
        newext2->datalen = ext->datalen;
        newext2->unique = ext->unique;
        newext2->ignore = false;
        newext2->inserted = true;
        add_extent(fcb, &newext1->list_entry, newext2);

        add_insert_extent_rollback(rollback, fcb, newext2);

        c = get_chunk_from_address(fcb->Vcb, ed2->address);

        if (!c)
            ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                               fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08lx\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    } else if (start_data > ext->offset && end_data < ext->offset + ed2->num_bytes) { // replace middle
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2, *newext3;

        newext1 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newext2 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newext3 = ExAllocatePoolWithTag(PagedPool, offsetof(extent, extent_data) + ext->datalen, ALLOC_TAG);
        if (!newext3) {
            ERR("out of memory\n");
            ExFreePool(newext1);
            ExFreePool(newext2);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(&newext1->extent_data, &ext->extent_data, ext->datalen);
        RtlCopyMemory(&newext2->extent_data, &ext->extent_data, ext->datalen);
        RtlCopyMemory(&newext3->extent_data, &ext->extent_data, ext->datalen);

        ned2 = (EXTENT_DATA2*)newext1->extent_data.data;
        ned2->num_bytes = start_data - ext->offset;

        newext2->extent_data.type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)newext2->extent_data.data;
        ned2->offset += start_data - ext->offset;
        ned2->num_bytes = end_data - start_data;

        ned2 = (EXTENT_DATA2*)newext3->extent_data.data;
        ned2->offset += end_data - ext->offset;
        ned2->num_bytes -= end_data - ext->offset;

        ned2 = (EXTENT_DATA2*)newext2->extent_data.data;
        Status = write_data_complete(fcb->Vcb, ed2->address + ned2->offset, data, (uint32_t)(end_data - start_data), Irp, NULL, file_write, irp_offset, priority);
        if (!NT_SUCCESS(Status)) {
            ERR("write_data_complete returned %08lx\n", Status);
            ExFreePool(newext1);
            ExFreePool(newext2);
            ExFreePool(newext3);
            return Status;
        }

        if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
            ULONG sl = (ULONG)((end_data - start_data) >> fcb->Vcb->sector_shift);
            void* csum = ExAllocatePoolWithTag(PagedPool, sl * fcb->Vcb->csum_size, ALLOC_TAG);

            if (!csum) {
                ERR("out of memory\n");
                ExFreePool(newext1);
                ExFreePool(newext2);
                ExFreePool(newext3);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            do_calc_job(fcb->Vcb, data, sl, csum);

            newext2->csum = csum;
        } else
            newext2->csum = NULL;

        *written = end_data - start_data;

        newext1->offset = ext->offset;
        newext1->datalen = ext->datalen;
        newext1->unique = ext->unique;
        newext1->ignore = false;
        newext1->inserted = true;
        newext1->csum = NULL;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);

        add_insert_extent_rollback(rollback, fcb, newext1);

        newext2->offset = start_data;
        newext2->datalen = ext->datalen;
        newext2->unique = ext->unique;
        newext2->ignore = false;
        newext2->inserted = true;
        add_extent(fcb, &newext1->list_entry, newext2);

        add_insert_extent_rollback(rollback, fcb, newext2);

        newext3->offset = end_data;
        newext3->datalen = ext->datalen;
        newext3->unique = ext->unique;
        newext3->ignore = false;
        newext3->inserted = true;
        newext3->csum = NULL;
        add_extent(fcb, &newext2->list_entry, newext3);

        add_insert_extent_rollback(rollback, fcb, newext3);

        c = get_chunk_from_address(fcb->Vcb, ed2->address);

        if (!c)
            ERR("get_chunk_from_address(%I64x) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 2,
                                               fcb->inode_item.flags & BTRFS_INODE_NODATASUM, false, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08lx\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    }

    if (c)
        c->changed = true;

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1, 4)))
NTSTATUS do_write_file(fcb* fcb, uint64_t start, uint64_t end_data, void* data, PIRP Irp, bool file_write, uint32_t irp_offset, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY *le, *le2;
    uint64_t written = 0, length = end_data - start;
    uint64_t last_cow_start;
    ULONG priority = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority;
#ifdef DEBUG_PARANOID
    uint64_t last_off;
#endif
    bool extents_changed = false;

    last_cow_start = 0;

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        le2 = le->Flink;

        if (!ext->ignore) {
            EXTENT_DATA* ed = &ext->extent_data;
            uint64_t len;

            if (ed->type == EXTENT_TYPE_INLINE)
                len = ed->decoded_size;
            else
                len = ((EXTENT_DATA2*)ed->data)->num_bytes;

            if (ext->offset + len <= start)
                goto nextitem;

            if (ext->offset > start + written + length)
                break;

            if ((fcb->inode_item.flags & BTRFS_INODE_NODATACOW || ed->type == EXTENT_TYPE_PREALLOC) && ext->unique && ed->compression == BTRFS_COMPRESSION_NONE) {
                if (max(last_cow_start, start + written) < ext->offset) {
                    uint64_t start_write = max(last_cow_start, start + written);

                    extents_changed = true;

                    Status = excise_extents(fcb->Vcb, fcb, start_write, ext->offset, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("excise_extents returned %08lx\n", Status);
                        return Status;
                    }

                    Status = insert_extent(fcb->Vcb, fcb, start_write, ext->offset - start_write, (uint8_t*)data + written, Irp, file_write, irp_offset + written, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_extent returned %08lx\n", Status);
                        return Status;
                    }

                    written += ext->offset - start_write;
                    length -= ext->offset - start_write;

                    if (length == 0)
                        break;
                }

                if (ed->type == EXTENT_TYPE_REGULAR) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                    uint64_t writeaddr = ed2->address + ed2->offset + start + written - ext->offset;
                    uint64_t write_len = min(len, length);
                    chunk* c;

                    TRACE("doing non-COW write to %I64x\n", writeaddr);

                    Status = write_data_complete(fcb->Vcb, writeaddr, (uint8_t*)data + written, (uint32_t)write_len, Irp, NULL, file_write, irp_offset + written, priority);
                    if (!NT_SUCCESS(Status)) {
                        ERR("write_data_complete returned %08lx\n", Status);
                        return Status;
                    }

                    c = get_chunk_from_address(fcb->Vcb, writeaddr);
                    if (c)
                        c->changed = true;

                    // This shouldn't ever get called - nocow files should always also be nosum.
                    if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
                        do_calc_job(fcb->Vcb, (uint8_t*)data + written, (uint32_t)(write_len >> fcb->Vcb->sector_shift),
                                    (uint8_t*)ext->csum + (((start + written - ext->offset) * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift));

                        ext->inserted = true;
                        extents_changed = true;
                    }

                    written += write_len;
                    length -= write_len;

                    if (length == 0)
                        break;
                } else if (ed->type == EXTENT_TYPE_PREALLOC) {
                    uint64_t write_len;

                    Status = do_write_file_prealloc(fcb, ext, start + written, end_data, (uint8_t*)data + written, &write_len,
                                                    Irp, file_write, irp_offset + written, priority, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("do_write_file_prealloc returned %08lx\n", Status);
                        return Status;
                    }

                    extents_changed = true;

                    written += write_len;
                    length -= write_len;

                    if (length == 0)
                        break;
                }

                last_cow_start = ext->offset + len;
            }
        }

nextitem:
        le = le2;
    }

    if (length > 0) {
        uint64_t start_write = max(last_cow_start, start + written);

        extents_changed = true;

        Status = excise_extents(fcb->Vcb, fcb, start_write, end_data, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08lx\n", Status);
            return Status;
        }

        Status = insert_extent(fcb->Vcb, fcb, start_write, end_data - start_write, (uint8_t*)data + written, Irp, file_write, irp_offset + written, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_extent returned %08lx\n", Status);
            return Status;
        }
    }

#ifdef DEBUG_PARANOID
    last_off = 0xffffffffffffffff;

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if (!ext->ignore) {
            if (ext->offset == last_off) {
                ERR("offset %I64x duplicated\n", ext->offset);
                int3;
            } else if (ext->offset < last_off && last_off != 0xffffffffffffffff) {
                ERR("offsets out of order\n");
                int3;
            }

            last_off = ext->offset;
        }

        le = le->Flink;
    }
#endif

    if (extents_changed) {
        fcb->extents_changed = true;
        mark_fcb_dirty(fcb);
    }

    return STATUS_SUCCESS;
}

__attribute__((nonnull(1,2,4,5,11)))
NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, bool paging_io, bool no_cache,
                     bool wait, bool deferred_write, bool write_irp, LIST_ENTRY* rollback) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    EXTENT_DATA* ed2;
    uint64_t off64, newlength, start_data, end_data;
    uint32_t bufhead;
    bool make_inline;
    INODE_ITEM* origii;
    bool changed_length = false;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    bool paging_lock = false, acquired_fcb_lock = false, acquired_tree_lock = false, pagefile;
    ULONG filter = 0;

    TRACE("(%p, %p, %I64x, %p, %lx, %u, %u)\n", Vcb, FileObject, offset.QuadPart, buf, *length, paging_io, no_cache);

    if (*length == 0) {
        TRACE("returning success for zero-length write\n");
        return STATUS_SUCCESS;
    }

    if (!FileObject) {
        ERR("error - FileObject was NULL\n");
        return STATUS_ACCESS_DENIED;
    }

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;
    fileref = ccb ? ccb->fileref : NULL;

    if (!fcb->ads && fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK) {
        WARN("tried to write to something other than a file or symlink (inode %I64x, type %u, %p, %p)\n", fcb->inode, fcb->type, &fcb->type, fcb);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (offset.LowPart == FILE_WRITE_TO_END_OF_FILE && offset.HighPart == -1)
        offset = fcb->Header.FileSize;

    off64 = offset.QuadPart;

    TRACE("fcb->Header.Flags = %x\n", fcb->Header.Flags);

    if (!no_cache && !CcCanIWrite(FileObject, *length, wait, deferred_write))
        return STATUS_PENDING;

    if (!wait && no_cache)
        return STATUS_PENDING;

    if (no_cache && !paging_io && FileObject->SectionObjectPointer->DataSectionObject) {
        IO_STATUS_BLOCK iosb;

        ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, true);

        CcFlushCache(FileObject->SectionObjectPointer, &offset, *length, &iosb);

        if (!NT_SUCCESS(iosb.Status)) {
            ExReleaseResourceLite(fcb->Header.PagingIoResource);
            ERR("CcFlushCache returned %08lx\n", iosb.Status);
            return iosb.Status;
        }

        paging_lock = true;

        CcPurgeCacheSection(FileObject->SectionObjectPointer, &offset, *length, false);
    }

    if (paging_io) {
        if (!ExAcquireResourceSharedLite(fcb->Header.PagingIoResource, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            paging_lock = true;
    }

    pagefile = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE && paging_io;

    if (!pagefile && !ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
        if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            acquired_tree_lock = true;
    }

    if (pagefile) {
        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            acquired_fcb_lock = true;
    } else if (!ExIsResourceAcquiredExclusiveLite(fcb->Header.Resource)) {
        if (!ExAcquireResourceExclusiveLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            acquired_fcb_lock = true;
    }

    newlength = fcb->ads ? fcb->adsdata.Length : fcb->inode_item.st_size;

    if (fcb->deleted)
        newlength = 0;

    TRACE("newlength = %I64x\n", newlength);

    if (off64 + *length > newlength) {
        if (paging_io) {
            if (off64 >= newlength) {
                TRACE("paging IO tried to write beyond end of file (file size = %I64x, offset = %I64x, length = %lx)\n", newlength, off64, *length);
                TRACE("FileObject: AllocationSize = %I64x, FileSize = %I64x, ValidDataLength = %I64x\n",
                    fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
                Irp->IoStatus.Information = 0;
                Status = STATUS_SUCCESS;
                goto end;
            }

            *length = (ULONG)(newlength - off64);
        } else {
            newlength = off64 + *length;
            changed_length = true;

            TRACE("extending length to %I64x\n", newlength);
        }
    }

    if (fcb->ads)
        make_inline = false;
    else
        make_inline = newlength <= fcb->Vcb->options.max_inline;

    if (changed_length) {
        if (newlength > (uint64_t)fcb->Header.AllocationSize.QuadPart) {
            if (!acquired_tree_lock) {
                // We need to acquire the tree lock if we don't have it already -
                // we can't give an inline file proper extents at the same time as we're
                // doing a flush.
                if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, wait)) {
                    Status = STATUS_PENDING;
                    goto end;
                } else
                    acquired_tree_lock = true;
            }

            Status = extend_file(fcb, fileref, newlength, false, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("extend_file returned %08lx\n", Status);
                goto end;
            }
        } else if (!fcb->ads)
            fcb->inode_item.st_size = newlength;

        fcb->Header.FileSize.QuadPart = newlength;
        fcb->Header.ValidDataLength.QuadPart = newlength;

        TRACE("AllocationSize = %I64x\n", fcb->Header.AllocationSize.QuadPart);
        TRACE("FileSize = %I64x\n", fcb->Header.FileSize.QuadPart);
        TRACE("ValidDataLength = %I64x\n", fcb->Header.ValidDataLength.QuadPart);
    }

    if (!no_cache) {
        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            if (!FileObject->PrivateCacheMap || changed_length) {
                CC_FILE_SIZES ccfs;

                ccfs.AllocationSize = fcb->Header.AllocationSize;
                ccfs.FileSize = fcb->Header.FileSize;
                ccfs.ValidDataLength = fcb->Header.ValidDataLength;

                if (!FileObject->PrivateCacheMap)
                    init_file_cache(FileObject, &ccfs);

                CcSetFileSizes(FileObject, &ccfs);
            }

            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                CcPrepareMdlWrite(FileObject, &offset, *length, &Irp->MdlAddress, &Irp->IoStatus);

                Status = Irp->IoStatus.Status;
                goto end;
            } else {
                /* We have to wait in CcCopyWrite - if we return STATUS_PENDING and add this to the work queue,
                 * it can result in CcFlushCache being called before the job has run. See ifstest ReadWriteTest. */

                if (fCcCopyWriteEx) {
                    TRACE("CcCopyWriteEx(%p, %I64x, %lx, %u, %p, %p)\n", FileObject, off64, *length, true, buf, Irp->Tail.Overlay.Thread);
                    if (!fCcCopyWriteEx(FileObject, &offset, *length, true, buf, Irp->Tail.Overlay.Thread)) {
                        Status = STATUS_PENDING;
                        goto end;
                    }
                    TRACE("CcCopyWriteEx finished\n");
                } else {
                    TRACE("CcCopyWrite(%p, %I64x, %lx, %u, %p)\n", FileObject, off64, *length, true, buf);
                    if (!CcCopyWrite(FileObject, &offset, *length, true, buf)) {
                        Status = STATUS_PENDING;
                        goto end;
                    }
                    TRACE("CcCopyWrite finished\n");
                }

                Irp->IoStatus.Information = *length;
            }
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (changed_length) {
            queue_notification_fcb(fcb->ads ? fileref->parent : fileref, fcb->ads ? FILE_NOTIFY_CHANGE_STREAM_SIZE : FILE_NOTIFY_CHANGE_SIZE,
                                   fcb->ads ? FILE_ACTION_MODIFIED_STREAM : FILE_ACTION_MODIFIED, fcb->ads && fileref->dc ? &fileref->dc->name : NULL);
        }

        goto end;
    }

    if (fcb->ads) {
        if (changed_length) {
            char* data2;

            if (newlength > fcb->adsmaxlen) {
                ERR("error - xattr too long (%I64u > %lu)\n", newlength, fcb->adsmaxlen);
                Status = STATUS_DISK_FULL;
                goto end;
            }

            data2 = ExAllocatePoolWithTag(PagedPool, (ULONG)newlength, ALLOC_TAG);
            if (!data2) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            if (fcb->adsdata.Buffer) {
                RtlCopyMemory(data2, fcb->adsdata.Buffer, fcb->adsdata.Length);
                ExFreePool(fcb->adsdata.Buffer);
            }

            if (newlength > fcb->adsdata.Length)
                RtlZeroMemory(&data2[fcb->adsdata.Length], (ULONG)(newlength - fcb->adsdata.Length));


            fcb->adsdata.Buffer = data2;
            fcb->adsdata.Length = fcb->adsdata.MaximumLength = (USHORT)newlength;

            fcb->Header.AllocationSize.QuadPart = newlength;
            fcb->Header.FileSize.QuadPart = newlength;
            fcb->Header.ValidDataLength.QuadPart = newlength;
        }

        if (*length > 0)
            RtlCopyMemory(&fcb->adsdata.Buffer[off64], buf, *length);

        fcb->Header.ValidDataLength.QuadPart = newlength;

        mark_fcb_dirty(fcb);

        if (fileref)
            mark_fileref_dirty(fileref);
    } else {
        bool compress = write_fcb_compressed(fcb), no_buf = false;
        uint8_t* data;

        if (make_inline) {
            start_data = 0;
            end_data = sector_align(newlength, fcb->Vcb->superblock.sector_size);
            bufhead = sizeof(EXTENT_DATA) - 1;
        } else if (compress) {
            start_data = off64 & ~(uint64_t)(COMPRESSED_EXTENT_SIZE - 1);
            end_data = min(sector_align(off64 + *length, COMPRESSED_EXTENT_SIZE),
                           sector_align(newlength, fcb->Vcb->superblock.sector_size));
            bufhead = 0;
        } else {
            start_data = off64 & ~(uint64_t)(fcb->Vcb->superblock.sector_size - 1);
            end_data = sector_align(off64 + *length, fcb->Vcb->superblock.sector_size);
            bufhead = 0;
        }

        if (fcb_is_inline(fcb))
            end_data = max(end_data, sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size));

        fcb->Header.ValidDataLength.QuadPart = newlength;
        TRACE("fcb %p FileSize = %I64x\n", fcb, fcb->Header.FileSize.QuadPart);

        if (!make_inline && !compress && off64 == start_data && off64 + *length == end_data) {
            data = buf;
            no_buf = true;
        } else {
            data = ExAllocatePoolWithTag(PagedPool, (ULONG)(end_data - start_data + bufhead), ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlZeroMemory(data + bufhead, (ULONG)(end_data - start_data));

            TRACE("start_data = %I64x\n", start_data);
            TRACE("end_data = %I64x\n", end_data);

            if (off64 > start_data || off64 + *length < end_data) {
                if (changed_length) {
                    if (fcb->inode_item.st_size > start_data)
                        Status = read_file(fcb, data + bufhead, start_data, fcb->inode_item.st_size - start_data, NULL, Irp);
                    else
                        Status = STATUS_SUCCESS;
                } else
                    Status = read_file(fcb, data + bufhead, start_data, end_data - start_data, NULL, Irp);

                if (!NT_SUCCESS(Status)) {
                    ERR("read_file returned %08lx\n", Status);
                    ExFreePool(data);
                    goto end;
                }
            }

            RtlCopyMemory(data + bufhead + off64 - start_data, buf, *length);
        }

        if (make_inline) {
            Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("error - excise_extents returned %08lx\n", Status);
                ExFreePool(data);
                goto end;
            }

            ed2 = (EXTENT_DATA*)data;
            ed2->generation = fcb->Vcb->superblock.generation;
            ed2->decoded_size = newlength;
            ed2->compression = BTRFS_COMPRESSION_NONE;
            ed2->encryption = BTRFS_ENCRYPTION_NONE;
            ed2->encoding = BTRFS_ENCODING_NONE;
            ed2->type = EXTENT_TYPE_INLINE;

            Status = add_extent_to_fcb(fcb, 0, ed2, (uint16_t)(offsetof(EXTENT_DATA, data[0]) + newlength), false, NULL, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_extent_to_fcb returned %08lx\n", Status);
                ExFreePool(data);
                goto end;
            }

            fcb->inode_item.st_blocks += newlength;
        } else if (compress) {
            Status = write_compressed(fcb, start_data, end_data, data, Irp, rollback);

            if (!NT_SUCCESS(Status)) {
                ERR("write_compressed returned %08lx\n", Status);
                ExFreePool(data);
                goto end;
            }
        } else {
            if (write_irp && Irp->MdlAddress && no_buf) {
                bool locked = Irp->MdlAddress->MdlFlags & (MDL_PAGES_LOCKED | MDL_PARTIAL);

                if (!locked) {
                    Status = STATUS_SUCCESS;

                    _SEH2_TRY {
                        MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoReadAccess);
                    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                        Status = _SEH2_GetExceptionCode();
                    } _SEH2_END;

                    if (!NT_SUCCESS(Status)) {
                        ERR("MmProbeAndLockPages threw exception %08lx\n", Status);
                        goto end;
                    }
                }

                _SEH2_TRY {
                    Status = do_write_file(fcb, start_data, end_data, data, Irp, true, 0, rollback);
                } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                    Status = _SEH2_GetExceptionCode();
                } _SEH2_END;

                if (!locked)
                    MmUnlockPages(Irp->MdlAddress);
            } else {
                _SEH2_TRY {
                    Status = do_write_file(fcb, start_data, end_data, data, Irp, false, 0, rollback);
                } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
                    Status = _SEH2_GetExceptionCode();
                } _SEH2_END;
            }

            if (!NT_SUCCESS(Status)) {
                ERR("do_write_file returned %08lx\n", Status);
                if (!no_buf) ExFreePool(data);
                goto end;
            }
        }

        if (!no_buf)
            ExFreePool(data);
    }

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    if (!pagefile) {
        if (fcb->ads) {
            if (fileref && fileref->parent)
                origii = &fileref->parent->fcb->inode_item;
            else {
                ERR("no parent fcb found for stream\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
        } else
            origii = &fcb->inode_item;

        origii->transid = Vcb->superblock.generation;
        origii->sequence++;

        if (!ccb->user_set_change_time)
            origii->st_ctime = now;

        if (!fcb->ads) {
            if (changed_length) {
                TRACE("setting st_size to %I64x\n", newlength);
                origii->st_size = newlength;
                filter |= FILE_NOTIFY_CHANGE_SIZE;
            }

            fcb->inode_item_changed = true;
        } else {
            fileref->parent->fcb->inode_item_changed = true;

            if (changed_length)
                filter |= FILE_NOTIFY_CHANGE_STREAM_SIZE;

            filter |= FILE_NOTIFY_CHANGE_STREAM_WRITE;
        }

        if (!ccb->user_set_write_time) {
            origii->st_mtime = now;
            filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }

        mark_fcb_dirty(fcb->ads ? fileref->parent->fcb : fcb);
    }

    if (changed_length) {
        CC_FILE_SIZES ccfs;

        ccfs.AllocationSize = fcb->Header.AllocationSize;
        ccfs.FileSize = fcb->Header.FileSize;
        ccfs.ValidDataLength = fcb->Header.ValidDataLength;

        _SEH2_TRY {
            CcSetFileSizes(FileObject, &ccfs);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
            goto end;
        } _SEH2_END;
    }

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = *length;

    if (filter != 0)
        queue_notification_fcb(fcb->ads ? fileref->parent : fileref, filter, fcb->ads ? FILE_ACTION_MODIFIED_STREAM : FILE_ACTION_MODIFIED,
                               fcb->ads && fileref->dc ? &fileref->dc->name : NULL);

end:
    if (NT_SUCCESS(Status) && FileObject->Flags & FO_SYNCHRONOUS_IO && !paging_io) {
        TRACE("CurrentByteOffset was: %I64x\n", FileObject->CurrentByteOffset.QuadPart);
        FileObject->CurrentByteOffset.QuadPart = offset.QuadPart + (NT_SUCCESS(Status) ? *length : 0);
        TRACE("CurrentByteOffset now: %I64x\n", FileObject->CurrentByteOffset.QuadPart);
    }

    if (acquired_fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);

    if (acquired_tree_lock)
        ExReleaseResourceLite(&Vcb->tree_lock);

    if (paging_lock)
        ExReleaseResourceLite(fcb->Header.PagingIoResource);

    return Status;
}

__attribute__((nonnull(1,2)))
NTSTATUS write_file(device_extension* Vcb, PIRP Irp, bool wait, bool deferred_write) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    void* buf;
    NTSTATUS Status;
    LARGE_INTEGER offset = IrpSp->Parameters.Write.ByteOffset;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
    LIST_ENTRY rollback;

    InitializeListHead(&rollback);

    TRACE("write\n");

    Irp->IoStatus.Information = 0;

    TRACE("offset = %I64x\n", offset.QuadPart);
    TRACE("length = %lx\n", IrpSp->Parameters.Write.Length);

    if (!Irp->AssociatedIrp.SystemBuffer) {
        buf = map_user_buffer(Irp, fcb && fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);

        if (Irp->MdlAddress && !buf) {
            ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
    } else
        buf = Irp->AssociatedIrp.SystemBuffer;

    TRACE("buf = %p\n", buf);

    if (fcb && !(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForWriteAccess(&fcb->lock, Irp)) {
        WARN("tried to write to locked region\n");
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto exit;
    }

    Status = write_file2(Vcb, Irp, offset, buf, &IrpSp->Parameters.Write.Length, Irp->Flags & IRP_PAGING_IO, Irp->Flags & IRP_NOCACHE,
                         wait, deferred_write, true, &rollback);

    if (Status == STATUS_PENDING)
        goto exit;
    else if (!NT_SUCCESS(Status)) {
        ERR("write_file2 returned %08lx\n", Status);
        goto exit;
    }

    if (NT_SUCCESS(Status)) {
        if (diskacc && Status != STATUS_PENDING && Irp->Flags & IRP_NOCACHE) {
            PETHREAD thread = NULL;

            if (Irp->Tail.Overlay.Thread && !IoIsSystemThread(Irp->Tail.Overlay.Thread))
                thread = Irp->Tail.Overlay.Thread;
            else if (!IoIsSystemThread(PsGetCurrentThread()))
                thread = PsGetCurrentThread();
            else if (IoIsSystemThread(PsGetCurrentThread()) && IoGetTopLevelIrp() == Irp)
                thread = PsGetCurrentThread();

            if (thread)
                fPsUpdateDiskCounters(PsGetThreadProcess(thread), 0, IrpSp->Parameters.Write.Length, 0, 1, 0);
        }
    }

exit:
    if (NT_SUCCESS(Status))
        clear_rollback(&rollback);
    else
        do_rollback(Vcb, &rollback);

    return Status;
}

_Dispatch_type_(IRP_MJ_WRITE)
_Function_class_(DRIVER_DISPATCH)
__attribute__((nonnull(1,2)))
NTSTATUS __stdcall drv_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    bool top_level;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
    ccb* ccb = FileObject ? FileObject->FsContext2 : NULL;
    bool wait = FileObject ? IoIsOperationSynchronous(Irp) : true;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_write(DeviceObject, Irp);
        goto exit;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!ccb) {
        ERR("ccb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {
        WARN("insufficient permissions\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    if (fcb == Vcb->volume_fcb) {
        if (!Vcb->locked || Vcb->locked_fileobj != FileObject) {
            ERR("trying to write to volume when not locked, or locked with another FileObject\n");
            Status = STATUS_ACCESS_DENIED;
            goto end;
        }

        TRACE("writing directly to volume\n");

        IoSkipCurrentIrpStackLocation(Irp);

        Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);
        goto exit;
    }

    if (is_subvol_readonly(fcb->subvol, Irp)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }

    _SEH2_TRY {
        if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
            CcMdlWriteComplete(IrpSp->FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress);

            Irp->MdlAddress = NULL;
            Status = STATUS_SUCCESS;
        } else {
            if (!(Irp->Flags & IRP_PAGING_IO))
                FsRtlCheckOplock(fcb_oplock(fcb), Irp, NULL, NULL, NULL);

            // Don't offload jobs when doing paging IO - otherwise this can lead to
            // deadlocks in CcCopyWrite.
            if (Irp->Flags & IRP_PAGING_IO)
                wait = true;

            Status = write_file(Vcb, Irp, wait, false);
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

end:
    Irp->IoStatus.Status = Status;

    TRACE("wrote %Iu bytes\n", Irp->IoStatus.Information);

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    else {
        IoMarkIrpPending(Irp);

        if (!add_thread_job(Vcb, Irp))
            Status = do_write_job(Vcb, Irp);
    }

exit:
    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("returning %08lx\n", Status);

    FsRtlExitFileSystem();

    return Status;
}
