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
#include <ata.h>
#include <ntddscsi.h>
#include <ntddstor.h>

#define MAX_CSUM_SIZE (4096 - sizeof(tree_header) - sizeof(leaf_node))

// #define DEBUG_WRITE_LOOPS

typedef struct {
    KEVENT Event;
    IO_STATUS_BLOCK iosb;
} write_context;

typedef struct {
    EXTENT_ITEM_TREE eit;
    UINT8 type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_TREE2;

typedef struct {
    EXTENT_ITEM ei;
    UINT8 type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_SKINNY_METADATA;

static NTSTATUS create_chunk(device_extension* Vcb, chunk* c, PIRP Irp);
static NTSTATUS update_tree_extents(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback);

#ifndef _MSC_VER // not in mingw yet
#define DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED 0x80000000
#endif

_Function_class_(IO_COMPLETION_ROUTINE)
#ifdef __REACTOS__
static NTSTATUS NTAPI write_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS write_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    write_context* context = conptr;

    UNUSED(DeviceObject);

    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS write_data_phys(_In_ PDEVICE_OBJECT device, _In_ UINT64 address, _In_reads_bytes_(length) void* data, _In_ UINT32 length) {
    NTSTATUS Status;
    LARGE_INTEGER offset;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    write_context context;

    TRACE("(%p, %llx, %p, %x)\n", device, address, data, length);

    RtlZeroMemory(&context, sizeof(write_context));

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);

    offset.QuadPart = address;

    Irp = IoAllocateIrp(device->StackSize, FALSE);

    if (!Irp) {
        ERR("IoAllocateIrp failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_WRITE;

    if (device->Flags & DO_BUFFERED_IO) {
        Irp->AssociatedIrp.SystemBuffer = data;

        Irp->Flags = IRP_BUFFERED_IO;
    } else if (device->Flags & DO_DIRECT_IO) {
        Irp->MdlAddress = IoAllocateMdl(data, length, FALSE, FALSE, NULL);
        if (!Irp->MdlAddress) {
            DbgPrint("IoAllocateMdl failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        Status = STATUS_SUCCESS;

        _SEH2_TRY {
            MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoReadAccess);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

        if (!NT_SUCCESS(Status)) {
            ERR("MmProbeAndLockPages threw exception %08x\n", Status);
            IoFreeMdl(Irp->MdlAddress);
            goto exit;
        }
    } else {
        Irp->UserBuffer = data;
    }

    IrpSp->Parameters.Write.Length = length;
    IrpSp->Parameters.Write.ByteOffset = offset;

    Irp->UserIosb = &context.iosb;

    Irp->UserEvent = &context.Event;

    IoSetCompletionRoutine(Irp, write_completion, &context, TRUE, TRUE, TRUE);

    Status = IoCallDriver(device, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);
        Status = context.iosb.Status;
    }

    if (!NT_SUCCESS(Status)) {
        ERR("IoCallDriver returned %08x\n", Status);
    }

    if (device->Flags & DO_DIRECT_IO) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

exit:
    IoFreeIrp(Irp);

    return Status;
}

static void add_trim_entry(device* dev, UINT64 address, UINT64 size) {
    space* s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
    if (!s) {
        ERR("out of memory\n");
        return;
    }

    s->address = address;
    s->size = size;
    dev->num_trim_entries++;

    InsertTailList(&dev->trim_list, &s->list_entry);
}

static void clean_space_cache_chunk(device_extension* Vcb, chunk* c) {
    ULONG type;

    if (Vcb->trim && !Vcb->options.no_trim) {
        if (c->chunk_item->type & BLOCK_FLAG_DUPLICATE)
            type = BLOCK_FLAG_DUPLICATE;
        else if (c->chunk_item->type & BLOCK_FLAG_RAID0)
            type = BLOCK_FLAG_RAID0;
        else if (c->chunk_item->type & BLOCK_FLAG_RAID1)
            type = BLOCK_FLAG_DUPLICATE;
        else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
            type = BLOCK_FLAG_RAID10;
        else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
            type = BLOCK_FLAG_RAID5;
        else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
            type = BLOCK_FLAG_RAID6;
        else // SINGLE
            type = BLOCK_FLAG_DUPLICATE;
    }

    while (!IsListEmpty(&c->deleting)) {
        space* s = CONTAINING_RECORD(c->deleting.Flink, space, list_entry);

        if (Vcb->trim && !Vcb->options.no_trim && (!Vcb->options.no_barrier || !(c->chunk_item->type & BLOCK_FLAG_METADATA))) {
            CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

            if (type == BLOCK_FLAG_DUPLICATE) {
                UINT16 i;

                for (i = 0; i < c->chunk_item->num_stripes; i++) {
                    if (c->devices[i] && c->devices[i]->devobj && !c->devices[i]->readonly && c->devices[i]->trim)
                        add_trim_entry(c->devices[i], s->address - c->offset + cis[i].offset, s->size);
                }
            } else if (type == BLOCK_FLAG_RAID0) {
                UINT64 startoff, endoff;
                UINT16 startoffstripe, endoffstripe, i;

                get_raid0_offset(s->address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &startoff, &startoffstripe);
                get_raid0_offset(s->address - c->offset + s->size - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &endoff, &endoffstripe);

                for (i = 0; i < c->chunk_item->num_stripes; i++) {
                    if (c->devices[i] && c->devices[i]->devobj && !c->devices[i]->readonly && c->devices[i]->trim) {
                        UINT64 stripestart, stripeend;

                        if (startoffstripe > i)
                            stripestart = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
                        else if (startoffstripe == i)
                            stripestart = startoff;
                        else
                            stripestart = startoff - (startoff % c->chunk_item->stripe_length);

                        if (endoffstripe > i)
                            stripeend = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
                        else if (endoffstripe == i)
                            stripeend = endoff + 1;
                        else
                            stripeend = endoff - (endoff % c->chunk_item->stripe_length);

                        if (stripestart != stripeend)
                            add_trim_entry(c->devices[i], stripestart + cis[i].offset, stripeend - stripestart);
                    }
                }
            } else if (type == BLOCK_FLAG_RAID10) {
                UINT64 startoff, endoff;
                UINT16 sub_stripes, startoffstripe, endoffstripe, i;

                sub_stripes = max(1, c->chunk_item->sub_stripes);

                get_raid0_offset(s->address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes / sub_stripes, &startoff, &startoffstripe);
                get_raid0_offset(s->address - c->offset + s->size - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes / sub_stripes, &endoff, &endoffstripe);

                startoffstripe *= sub_stripes;
                endoffstripe *= sub_stripes;

                for (i = 0; i < c->chunk_item->num_stripes; i += sub_stripes) {
                    ULONG j;
                    UINT64 stripestart, stripeend;

                    if (startoffstripe > i)
                        stripestart = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
                    else if (startoffstripe == i)
                        stripestart = startoff;
                    else
                        stripestart = startoff - (startoff % c->chunk_item->stripe_length);

                    if (endoffstripe > i)
                        stripeend = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
                    else if (endoffstripe == i)
                        stripeend = endoff + 1;
                    else
                        stripeend = endoff - (endoff % c->chunk_item->stripe_length);

                    if (stripestart != stripeend) {
                        for (j = 0; j < sub_stripes; j++) {
                            if (c->devices[i+j] && c->devices[i+j]->devobj && !c->devices[i+j]->readonly && c->devices[i+j]->trim)
                                add_trim_entry(c->devices[i+j], stripestart + cis[i+j].offset, stripeend - stripestart);
                        }
                    }
                }
            }
            // FIXME - RAID5(?), RAID6(?)
        }

        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }
}

typedef struct {
    DEVICE_MANAGE_DATA_SET_ATTRIBUTES* dmdsa;
    ATA_PASS_THROUGH_EX apte;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
} ioctl_context_stripe;

typedef struct {
    KEVENT Event;
    LONG left;
    ioctl_context_stripe* stripes;
} ioctl_context;

_Function_class_(IO_COMPLETION_ROUTINE)
#ifdef __REACTOS__
static NTSTATUS NTAPI ioctl_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS ioctl_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    ioctl_context* context = (ioctl_context*)conptr;
    LONG left2 = InterlockedDecrement(&context->left);

    UNUSED(DeviceObject);
    UNUSED(Irp);

    if (left2 == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static void clean_space_cache(device_extension* Vcb) {
    LIST_ENTRY* le;
    chunk* c;
    ULONG num;

    TRACE("(%p)\n", Vcb);

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (c->space_changed) {
            acquire_chunk_lock(c, Vcb);

            if (c->space_changed)
                clean_space_cache_chunk(Vcb, c);

            c->space_changed = FALSE;

            release_chunk_lock(c, Vcb);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    if (Vcb->trim && !Vcb->options.no_trim) {
        ioctl_context context;
        ULONG total_num;

        context.left = 0;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (dev->devobj && !dev->readonly && dev->trim && dev->num_trim_entries > 0)
                context.left++;

            le = le->Flink;
        }

        if (context.left == 0)
            return;

        total_num = context.left;
        num = 0;

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);

        context.stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(ioctl_context_stripe) * context.left, ALLOC_TAG);
        if (!context.stripes) {
            ERR("out of memory\n");
            return;
        }

        RtlZeroMemory(context.stripes, sizeof(ioctl_context_stripe) * context.left);

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (dev->devobj && !dev->readonly && dev->trim && dev->num_trim_entries > 0) {
                LIST_ENTRY* le2;
                ioctl_context_stripe* stripe = &context.stripes[num];
                DEVICE_DATA_SET_RANGE* ranges;
                ULONG datalen = (ULONG)sector_align(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), sizeof(UINT64)) + (dev->num_trim_entries * sizeof(DEVICE_DATA_SET_RANGE)), i;
                PIO_STACK_LOCATION IrpSp;

                stripe->dmdsa = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
                if (!stripe->dmdsa) {
                    ERR("out of memory\n");
                    goto nextdev;
                }

                stripe->dmdsa->Size = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES);
                stripe->dmdsa->Action = DeviceDsmAction_Trim;
                stripe->dmdsa->Flags = DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED;
                stripe->dmdsa->ParameterBlockOffset = 0;
                stripe->dmdsa->ParameterBlockLength = 0;
                stripe->dmdsa->DataSetRangesOffset = (ULONG)sector_align(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), sizeof(UINT64));
                stripe->dmdsa->DataSetRangesLength = dev->num_trim_entries * sizeof(DEVICE_DATA_SET_RANGE);

                ranges = (DEVICE_DATA_SET_RANGE*)((UINT8*)stripe->dmdsa + stripe->dmdsa->DataSetRangesOffset);

                i = 0;

                le2 = dev->trim_list.Flink;
                while (le2 != &dev->trim_list) {
                    space* s = CONTAINING_RECORD(le2, space, list_entry);

                    ranges[i].StartingOffset = s->address;
                    ranges[i].LengthInBytes = s->size;
                    i++;

                    le2 = le2->Flink;
                }

                stripe->Irp = IoAllocateIrp(dev->devobj->StackSize, FALSE);

                if (!stripe->Irp) {
                    ERR("IoAllocateIrp failed\n");
                    goto nextdev;
                }

                IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
                IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

                IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES;
                IrpSp->Parameters.DeviceIoControl.InputBufferLength = datalen;
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;

                stripe->Irp->AssociatedIrp.SystemBuffer = stripe->dmdsa;
                stripe->Irp->Flags |= IRP_BUFFERED_IO;
                stripe->Irp->UserBuffer = NULL;
                stripe->Irp->UserIosb = &stripe->iosb;

                IoSetCompletionRoutine(stripe->Irp, ioctl_completion, &context, TRUE, TRUE, TRUE);

                IoCallDriver(dev->devobj, stripe->Irp);

nextdev:
                while (!IsListEmpty(&dev->trim_list)) {
                    space* s = CONTAINING_RECORD(RemoveHeadList(&dev->trim_list), space, list_entry);
                    ExFreePool(s);
                }

                dev->num_trim_entries = 0;

                num++;
            }

            le = le->Flink;
        }

        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

        for (num = 0; num < total_num; num++) {
            if (context.stripes[num].dmdsa)
                ExFreePool(context.stripes[num].dmdsa);
        }

        ExFreePool(context.stripes);
    }
}

static BOOL trees_consistent(device_extension* Vcb) {
    ULONG maxsize = Vcb->superblock.node_size - sizeof(tree_header);
    LIST_ENTRY* le;

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);

        if (t->write) {
            if (t->header.num_items == 0 && t->parent) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("empty tree found, looping again\n");
#endif
                return FALSE;
            }

            if (t->size > maxsize) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("overlarge tree found (%u > %u), looping again\n", t->size, maxsize);
#endif
                return FALSE;
            }

            if (!t->has_new_address) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("tree found without new address, looping again\n");
#endif
                return FALSE;
            }
        }

        le = le->Flink;
    }

    return TRUE;
}

static NTSTATUS add_parents(device_extension* Vcb, PIRP Irp) {
    ULONG level;
    LIST_ENTRY* le;

    for (level = 0; level <= 255; level++) {
        BOOL nothing_found = TRUE;

        TRACE("level = %u\n", level);

        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);

            if (t->write && t->header.level == level) {
                TRACE("tree %p: root = %llx, level = %x, parent = %p\n", t, t->header.tree_id, t->header.level, t->parent);

                nothing_found = FALSE;

                if (t->parent) {
                    if (!t->parent->write)
                        TRACE("adding tree %p (level %x)\n", t->parent, t->header.level);

                    t->parent->write = TRUE;
                } else if (t->root != Vcb->root_root && t->root != Vcb->chunk_root) {
                    KEY searchkey;
                    traverse_ptr tp;
                    NTSTATUS Status;
#ifdef __REACTOS__
                    tree* t2;
#endif

                    searchkey.obj_id = t->root->id;
                    searchkey.obj_type = TYPE_ROOT_ITEM;
                    searchkey.offset = 0xffffffffffffffff;

                    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        return Status;
                    }

                    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                        ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                        return STATUS_INTERNAL_ERROR;
                    }

                    if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, delete and create new entry
                        ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);

                        if (!ri) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory(ri, &t->root->root_item, sizeof(ROOT_ITEM));

                        Status = delete_tree_item(Vcb, &tp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("delete_tree_item returned %08x\n", Status);
                            ExFreePool(ri);
                            return Status;
                        }

                        Status = insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, Irp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("insert_tree_item returned %08x\n", Status);
                            ExFreePool(ri);
                            return Status;
                        }
                    }

#ifndef __REACTOS__
                    tree* t2 = tp.tree;
#else
                    t2 = tp.tree;
#endif
                    while (t2) {
                        t2->write = TRUE;

                        t2 = t2->parent;
                    }
                }
            }

            le = le->Flink;
        }

        if (nothing_found)
            break;
    }

    return STATUS_SUCCESS;
}

static void add_parents_to_cache(tree* t) {
    while (t->parent) {
        t = t->parent;
        t->write = TRUE;
    }
}

static BOOL insert_tree_extent_skinny(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64 address, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    EXTENT_ITEM_SKINNY_METADATA* eism;
    traverse_ptr insert_tp;

    eism = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_SKINNY_METADATA), ALLOC_TAG);
    if (!eism) {
        ERR("out of memory\n");
        return FALSE;
    }

    eism->ei.refcount = 1;
    eism->ei.generation = Vcb->superblock.generation;
    eism->ei.flags = EXTENT_ITEM_TREE_BLOCK;
    eism->type = TYPE_TREE_BLOCK_REF;
    eism->tbr.offset = root_id;

    Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(eism);
        return FALSE;
    }

    acquire_chunk_lock(c, Vcb);

    space_list_subtract(c, FALSE, address, Vcb->superblock.node_size, rollback);

    release_chunk_lock(c, Vcb);

    add_parents_to_cache(insert_tp.tree);

    return TRUE;
}

BOOL find_metadata_address_in_chunk(device_extension* Vcb, chunk* c, UINT64* address) {
    LIST_ENTRY* le;
    space* s;

    TRACE("(%p, %llx, %p)\n", Vcb, c->offset, address);

    if (Vcb->superblock.node_size > c->chunk_item->size - c->used)
        return FALSE;

    if (!c->cache_loaded) {
        NTSTATUS Status = load_cache_chunk(Vcb, c, NULL);

        if (!NT_SUCCESS(Status)) {
            ERR("load_cache_chunk returned %08x\n", Status);
            return FALSE;
        }
    }

    if (IsListEmpty(&c->space_size))
        return FALSE;

    if (!c->last_alloc_set) {
        s = CONTAINING_RECORD(c->space.Blink, space, list_entry);

        c->last_alloc = s->address;
        c->last_alloc_set = TRUE;

        if (s->size >= Vcb->superblock.node_size) {
            *address = s->address;
            c->last_alloc += Vcb->superblock.node_size;
            return TRUE;
        }
    }

    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);

        if (s->address <= c->last_alloc && s->address + s->size >= c->last_alloc + Vcb->superblock.node_size) {
            *address = c->last_alloc;
            c->last_alloc += Vcb->superblock.node_size;
            return TRUE;
        }

        le = le->Flink;
    }

    le = c->space_size.Flink;
    while (le != &c->space_size) {
        s = CONTAINING_RECORD(le, space, list_entry_size);

        if (s->size == Vcb->superblock.node_size) {
            *address = s->address;
            c->last_alloc = s->address + Vcb->superblock.node_size;
            return TRUE;
        } else if (s->size < Vcb->superblock.node_size) {
            if (le == c->space_size.Flink)
                return FALSE;

            s = CONTAINING_RECORD(le->Blink, space, list_entry_size);

            *address = s->address;
            c->last_alloc = s->address + Vcb->superblock.node_size;

            return TRUE;
        }

        le = le->Flink;
    }

    s = CONTAINING_RECORD(c->space_size.Blink, space, list_entry_size);

    if (s->size > Vcb->superblock.node_size) {
        *address = s->address;
        c->last_alloc = s->address + Vcb->superblock.node_size;
        return TRUE;
    }

    return FALSE;
}

static BOOL insert_tree_extent(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64* new_address, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 address;
    EXTENT_ITEM_TREE2* eit2;
    traverse_ptr insert_tp;

    TRACE("(%p, %x, %llx, %p, %p, %p, %p)\n", Vcb, level, root_id, c, new_address, rollback);

    if (!find_metadata_address_in_chunk(Vcb, c, &address))
        return FALSE;

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        BOOL b = insert_tree_extent_skinny(Vcb, level, root_id, c, address, Irp, rollback);

        if (b)
            *new_address = address;

        return b;
    }

    eit2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_TREE2), ALLOC_TAG);
    if (!eit2) {
        ERR("out of memory\n");
        return FALSE;
    }

    eit2->eit.extent_item.refcount = 1;
    eit2->eit.extent_item.generation = Vcb->superblock.generation;
    eit2->eit.extent_item.flags = EXTENT_ITEM_TREE_BLOCK;
    eit2->eit.level = level;
    eit2->type = TYPE_TREE_BLOCK_REF;
    eit2->tbr.offset = root_id;

    Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, eit2, sizeof(EXTENT_ITEM_TREE2), &insert_tp, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(eit2);
        return FALSE;
    }

    acquire_chunk_lock(c, Vcb);

    space_list_subtract(c, FALSE, address, Vcb->superblock.node_size, rollback);

    release_chunk_lock(c, Vcb);

    add_parents_to_cache(insert_tp.tree);

    *new_address = address;

    return TRUE;
}

NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    chunk *origchunk = NULL, *c;
    LIST_ENTRY* le;
    UINT64 flags, addr;

    if (t->root->id == BTRFS_ROOT_CHUNK)
        flags = Vcb->system_flags;
    else
        flags = Vcb->metadata_flags;

    if (t->has_address) {
        origchunk = get_chunk_from_address(Vcb, t->header.address);

        if (origchunk && !origchunk->readonly && !origchunk->reloc && origchunk->chunk_item->type == flags &&
            insert_tree_extent(Vcb, t->header.level, t->root->id, origchunk, &addr, Irp, rollback)) {
            t->new_address = addr;
            t->has_new_address = TRUE;
            return STATUS_SUCCESS;
        }
    }

    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, Vcb);

            if (c != origchunk && c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
                if (insert_tree_extent(Vcb, t->header.level, t->root->id, c, &addr, Irp, rollback)) {
                    release_chunk_lock(c, Vcb);
                    ExReleaseResourceLite(&Vcb->chunk_lock);
                    t->new_address = addr;
                    t->has_new_address = TRUE;
                    return STATUS_SUCCESS;
                }
            }

            release_chunk_lock(c, Vcb);
        }

        le = le->Flink;
    }

    // allocate new chunk if necessary

    Status = alloc_chunk(Vcb, flags, &c, FALSE);

    if (!NT_SUCCESS(Status)) {
        ERR("alloc_chunk returned %08x\n", Status);
        ExReleaseResourceLite(&Vcb->chunk_lock);
        return Status;
    }

    acquire_chunk_lock(c, Vcb);

    if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
        if (insert_tree_extent(Vcb, t->header.level, t->root->id, c, &addr, Irp, rollback)) {
            release_chunk_lock(c, Vcb);
            ExReleaseResourceLite(&Vcb->chunk_lock);
            t->new_address = addr;
            t->has_new_address = TRUE;
            return STATUS_SUCCESS;
        }
    }

    release_chunk_lock(c, Vcb);

    ExReleaseResourceLite(&Vcb->chunk_lock);

    ERR("couldn't find any metadata chunks with %x bytes free\n", Vcb->superblock.node_size);

    return STATUS_DISK_FULL;
}

static NTSTATUS reduce_tree_extent(device_extension* Vcb, UINT64 address, tree* t, UINT64 parent_root, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 rc, root;

    TRACE("(%p, %llx, %p)\n", Vcb, address, t);

    rc = get_extent_refcount(Vcb, address, Vcb->superblock.node_size, Irp);
    if (rc == 0) {
        ERR("error - refcount for extent %llx was 0\n", address);
        return STATUS_INTERNAL_ERROR;
    }

    if (!t || t->parent)
        root = parent_root;
    else
        root = t->header.tree_id;

    Status = decrease_extent_refcount_tree(Vcb, address, Vcb->superblock.node_size, root, level, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("decrease_extent_refcount_tree returned %08x\n", Status);
        return Status;
    }

    if (rc == 1) {
        chunk* c = get_chunk_from_address(Vcb, address);

        if (c) {
            acquire_chunk_lock(c, Vcb);

            if (!c->cache_loaded) {
                Status = load_cache_chunk(Vcb, c, NULL);

                if (!NT_SUCCESS(Status)) {
                    ERR("load_cache_chunk returned %08x\n", Status);
                    release_chunk_lock(c, Vcb);
                    return Status;
                }
            }

            c->used -= Vcb->superblock.node_size;

            space_list_add(c, address, Vcb->superblock.node_size, rollback);

            release_chunk_lock(c, Vcb);
        } else
            ERR("could not find chunk for address %llx\n", address);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS add_changed_extent_ref_edr(changed_extent* ce, EXTENT_DATA_REF* edr, BOOL old) {
    LIST_ENTRY *le2, *list;
    changed_extent_ref* cer;

    list = old ? &ce->old_refs : &ce->refs;

    le2 = list->Flink;
    while (le2 != list) {
        cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

        if (cer->type == TYPE_EXTENT_DATA_REF && cer->edr.root == edr->root && cer->edr.objid == edr->objid && cer->edr.offset == edr->offset) {
            cer->edr.count += edr->count;
            goto end;
        }

        le2 = le2->Flink;
    }

    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    if (!cer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cer->type = TYPE_EXTENT_DATA_REF;
    RtlCopyMemory(&cer->edr, edr, sizeof(EXTENT_DATA_REF));
    InsertTailList(list, &cer->list_entry);

end:
    if (old)
        ce->old_count += edr->count;
    else
        ce->count += edr->count;

    return STATUS_SUCCESS;
}

static NTSTATUS add_changed_extent_ref_sdr(changed_extent* ce, SHARED_DATA_REF* sdr, BOOL old) {
    LIST_ENTRY *le2, *list;
    changed_extent_ref* cer;

    list = old ? &ce->old_refs : &ce->refs;

    le2 = list->Flink;
    while (le2 != list) {
        cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

        if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr->offset) {
            cer->sdr.count += sdr->count;
            goto end;
        }

        le2 = le2->Flink;
    }

    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    if (!cer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cer->type = TYPE_SHARED_DATA_REF;
    RtlCopyMemory(&cer->sdr, sdr, sizeof(SHARED_DATA_REF));
    InsertTailList(list, &cer->list_entry);

end:
    if (old)
        ce->old_count += sdr->count;
    else
        ce->count += sdr->count;

    return STATUS_SUCCESS;
}

static BOOL shared_tree_is_unique(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    if (!t->updated_extents && t->has_address) {
        Status = update_tree_extents(Vcb, t, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return FALSE;
        }
    }

    searchkey.obj_id = t->header.address;
    searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }

    if (tp.item->key.obj_id == t->header.address && (tp.item->key.obj_type == TYPE_METADATA_ITEM || tp.item->key.obj_type == TYPE_EXTENT_ITEM))
        return FALSE;
    else
        return TRUE;
}

static NTSTATUS update_tree_extents(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 rc = get_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, Irp);
    UINT64 flags = get_extent_flags(Vcb, t->header.address, Irp);

    if (rc == 0) {
        ERR("refcount for extent %llx was 0\n", t->header.address);
        return STATUS_INTERNAL_ERROR;
    }

    if (flags & EXTENT_ITEM_SHARED_BACKREFS || t->header.flags & HEADER_FLAG_SHARED_BACKREF || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
        TREE_BLOCK_REF tbr;
        BOOL unique = rc > 1 ? FALSE : (t->parent ? shared_tree_is_unique(Vcb, t->parent, Irp, rollback) : FALSE);

        if (t->header.level == 0) {
            LIST_ENTRY* le;

            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

                if (!td->inserted && td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;

                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                        if (ed2->size > 0) {
                            EXTENT_DATA_REF edr;
                            changed_extent* ce = NULL;
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);

                            if (c) {
                                LIST_ENTRY* le2;

                                le2 = c->changed_extents.Flink;
                                while (le2 != &c->changed_extents) {
                                    changed_extent* ce2 = CONTAINING_RECORD(le2, changed_extent, list_entry);

                                    if (ce2->address == ed2->address) {
                                        ce = ce2;
                                        break;
                                    }

                                    le2 = le2->Flink;
                                }
                            }

                            edr.root = t->root->id;
                            edr.objid = td->key.obj_id;
                            edr.offset = td->key.offset - ed2->offset;
                            edr.count = 1;

                            if (ce) {
                                Status = add_changed_extent_ref_edr(ce, &edr, TRUE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                    return Status;
                                }

                                Status = add_changed_extent_ref_edr(ce, &edr, FALSE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                    return Status;
                                }
                            }

                            Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("increase_extent_refcount returned %08x\n", Status);
                                return Status;
                            }

                            if ((flags & EXTENT_ITEM_SHARED_BACKREFS && unique) || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
                                UINT64 sdrrc = find_extent_shared_data_refcount(Vcb, ed2->address, t->header.address, Irp);

                                if (sdrrc > 0) {
                                    SHARED_DATA_REF sdr;

                                    sdr.offset = t->header.address;
                                    sdr.count = 1;

                                    Status = decrease_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0,
                                                                      t->header.address, ce ? ce->superseded : FALSE, Irp);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("decrease_extent_refcount returned %08x\n", Status);
                                        return Status;
                                    }

                                    if (ce) {
                                        LIST_ENTRY* le2;

                                        le2 = ce->refs.Flink;
                                        while (le2 != &ce->refs) {
                                            changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                                            if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr.offset) {
                                                ce->count--;
                                                cer->sdr.count--;
                                                break;
                                            }

                                            le2 = le2->Flink;
                                        }

                                        le2 = ce->old_refs.Flink;
                                        while (le2 != &ce->old_refs) {
                                            changed_extent_ref* cer = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                                            if (cer->type == TYPE_SHARED_DATA_REF && cer->sdr.offset == sdr.offset) {
                                                ce->old_count--;

                                                if (cer->sdr.count > 1)
                                                    cer->sdr.count--;
                                                else {
                                                    RemoveEntryList(&cer->list_entry);
                                                    ExFreePool(cer);
                                                }

                                                break;
                                            }

                                            le2 = le2->Flink;
                                        }
                                    }
                                }
                            }

                            // FIXME - clear shared flag if unique?
                        }
                    }
                }

                le = le->Flink;
            }
        } else {
            LIST_ENTRY* le;

            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

                if (!td->inserted) {
                    tbr.offset = t->root->id;

                    Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF,
                                                      &tbr, &td->key, t->header.level - 1, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }

                    if (unique || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
                        UINT64 sbrrc = find_extent_shared_tree_refcount(Vcb, td->treeholder.address, t->header.address, Irp);

                        if (sbrrc > 0) {
                            SHARED_BLOCK_REF sbr;

                            sbr.offset = t->header.address;

                            Status = decrease_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                              t->header.address, FALSE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("decrease_extent_refcount returned %08x\n", Status);
                                return Status;
                            }
                        }
                    }

                    // FIXME - clear shared flag if unique?
                }

                le = le->Flink;
            }
        }

        if (unique) {
            UINT64 sbrrc = find_extent_shared_tree_refcount(Vcb, t->header.address, t->parent->header.address, Irp);

            if (sbrrc == 1) {
                SHARED_BLOCK_REF sbr;

                sbr.offset = t->parent->header.address;

                Status = decrease_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, NULL, 0,
                                                  t->parent->header.address, FALSE, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("decrease_extent_refcount returned %08x\n", Status);
                    return Status;
                }
            }
        }

        if (t->parent)
            tbr.offset = t->parent->header.tree_id;
        else
            tbr.offset = t->header.tree_id;

        Status = increase_extent_refcount(Vcb, t->header.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr,
                                          t->parent ? &t->paritem->key : NULL, t->header.level, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("increase_extent_refcount returned %08x\n", Status);
            return Status;
        }

        // FIXME - clear shared flag if unique?

        t->header.flags &= ~HEADER_FLAG_SHARED_BACKREF;
    }

    if (rc > 1 || t->header.tree_id == t->root->id) {
        Status = reduce_tree_extent(Vcb, t->header.address, t, t->parent ? t->parent->header.tree_id : t->header.tree_id, t->header.level, Irp, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("reduce_tree_extent returned %08x\n", Status);
            return Status;
        }
    }

    t->has_address = FALSE;

    if ((rc > 1 || t->header.tree_id != t->root->id) && !(flags & EXTENT_ITEM_SHARED_BACKREFS)) {
        if (t->header.tree_id == t->root->id) {
            flags |= EXTENT_ITEM_SHARED_BACKREFS;
            update_extent_flags(Vcb, t->header.address, flags, Irp);
        }

        if (t->header.level > 0) {
            LIST_ENTRY* le;

            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

                if (!td->inserted) {
                    if (t->header.tree_id == t->root->id) {
                        SHARED_BLOCK_REF sbr;

                        sbr.offset = t->header.address;

                        Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_SHARED_BLOCK_REF, &sbr, &td->key, t->header.level - 1, Irp);
                    } else {
                        TREE_BLOCK_REF tbr;

                        tbr.offset = t->root->id;

                        Status = increase_extent_refcount(Vcb, td->treeholder.address, Vcb->superblock.node_size, TYPE_TREE_BLOCK_REF, &tbr, &td->key, t->header.level - 1, Irp);
                    }

                    if (!NT_SUCCESS(Status)) {
                        ERR("increase_extent_refcount returned %08x\n", Status);
                        return Status;
                    }
                }

                le = le->Flink;
            }
        } else {
            LIST_ENTRY* le;

            le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

                if (!td->inserted && td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;

                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                        if (ed2->size > 0) {
                            changed_extent* ce = NULL;
                            chunk* c = get_chunk_from_address(Vcb, ed2->address);

                            if (c) {
                                LIST_ENTRY* le2;

                                le2 = c->changed_extents.Flink;
                                while (le2 != &c->changed_extents) {
                                    changed_extent* ce2 = CONTAINING_RECORD(le2, changed_extent, list_entry);

                                    if (ce2->address == ed2->address) {
                                        ce = ce2;
                                        break;
                                    }

                                    le2 = le2->Flink;
                                }
                            }

                            if (t->header.tree_id == t->root->id) {
                                SHARED_DATA_REF sdr;

                                sdr.offset = t->header.address;
                                sdr.count = 1;

                                if (ce) {
                                    Status = add_changed_extent_ref_sdr(ce, &sdr, TRUE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }

                                    Status = add_changed_extent_ref_sdr(ce, &sdr, FALSE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                }

                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_SHARED_DATA_REF, &sdr, NULL, 0, Irp);
                            } else {
                                EXTENT_DATA_REF edr;

                                edr.root = t->root->id;
                                edr.objid = td->key.obj_id;
                                edr.offset = td->key.offset - ed2->offset;
                                edr.count = 1;

                                if (ce) {
                                    Status = add_changed_extent_ref_edr(ce, &edr, TRUE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }

                                    Status = add_changed_extent_ref_edr(ce, &edr, FALSE);
                                    if (!NT_SUCCESS(Status)) {
                                        ERR("add_changed_extent_ref_edr returned %08x\n", Status);
                                        return Status;
                                    }
                                }

                                Status = increase_extent_refcount(Vcb, ed2->address, ed2->size, TYPE_EXTENT_DATA_REF, &edr, NULL, 0, Irp);
                            }

                            if (!NT_SUCCESS(Status)) {
                                ERR("increase_extent_refcount returned %08x\n", Status);
                                return Status;
                            }
                        }
                    }
                }

                le = le->Flink;
            }
        }
    }

    t->updated_extents = TRUE;
    t->header.tree_id = t->root->id;

    return STATUS_SUCCESS;
}

static NTSTATUS allocate_tree_extents(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    BOOL changed = FALSE;
    UINT8 max_level = 0, level;

    TRACE("(%p)\n", Vcb);

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);

        if (t->write && !t->has_new_address) {
            chunk* c;

            if (t->has_address) {
                c = get_chunk_from_address(Vcb, t->header.address);

                if (c) {
                    if (!c->cache_loaded) {
                        acquire_chunk_lock(c, Vcb);

                        if (!c->cache_loaded) {
                            Status = load_cache_chunk(Vcb, c, NULL);

                            if (!NT_SUCCESS(Status)) {
                                ERR("load_cache_chunk returned %08x\n", Status);
                                release_chunk_lock(c, Vcb);
                                return Status;
                            }
                        }

                        release_chunk_lock(c, Vcb);
                    }
                }
            }

            Status = get_tree_new_address(Vcb, t, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("get_tree_new_address returned %08x\n", Status);
                return Status;
            }

            TRACE("allocated extent %llx\n", t->new_address);

            c = get_chunk_from_address(Vcb, t->new_address);

            if (c)
                c->used += Vcb->superblock.node_size;
            else {
                ERR("could not find chunk for address %llx\n", t->new_address);
                return STATUS_INTERNAL_ERROR;
            }

            changed = TRUE;

            if (t->header.level > max_level)
                max_level = t->header.level;
        }

        le = le->Flink;
    }

    if (!changed)
        return STATUS_SUCCESS;

    level = max_level;
    do {
        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);

            if (t->write && !t->updated_extents && t->has_address && t->header.level == level) {
                Status = update_tree_extents(Vcb, t, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("update_tree_extents returned %08x\n", Status);
                    return Status;
                }
            }

            le = le->Flink;
        }

        if (level == 0)
            break;

        level--;
    } while (TRUE);

    return STATUS_SUCCESS;
}

static NTSTATUS update_root_root(device_extension* Vcb, BOOL no_cache, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;

    TRACE("(%p)\n", Vcb);

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);

        if (t->write && !t->parent) {
            if (t->root != Vcb->root_root && t->root != Vcb->chunk_root) {
                KEY searchkey;
                traverse_ptr tp;

                searchkey.obj_id = t->root->id;
                searchkey.obj_type = TYPE_ROOT_ITEM;
                searchkey.offset = 0xffffffffffffffff;

                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }

                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                    return STATUS_INTERNAL_ERROR;
                }

                TRACE("updating the address for root %llx to %llx\n", searchkey.obj_id, t->new_address);

                t->root->root_item.block_number = t->new_address;
                t->root->root_item.root_level = t->header.level;
                t->root->root_item.generation = Vcb->superblock.generation;
                t->root->root_item.generation2 = Vcb->superblock.generation;

                // item is guaranteed to be at least sizeof(ROOT_ITEM), due to add_parents

                RtlCopyMemory(tp.item->data, &t->root->root_item, sizeof(ROOT_ITEM));
            }

            t->root->treeholder.address = t->new_address;
            t->root->treeholder.generation = Vcb->superblock.generation;
        }

        le = le->Flink;
    }

    if (!no_cache && !(Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE)) {
        ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
        Status = update_chunk_caches(Vcb, Irp, rollback);
        ExReleaseResourceLite(&Vcb->chunk_lock);

        if (!NT_SUCCESS(Status)) {
            ERR("update_chunk_caches returned %08x\n", Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS do_tree_writes(device_extension* Vcb, LIST_ENTRY* tree_writes, BOOL no_free) {
    chunk* c;
    LIST_ENTRY* le;
    tree_write* tw;
    NTSTATUS Status;
    ULONG i, num_bits;
    write_data_context* wtc;
    ULONG bit_num = 0;
    BOOL raid56 = FALSE;

    // merge together runs
    c = NULL;
    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);

        if (!c || tw->address < c->offset || tw->address >= c->offset + c->chunk_item->size)
            c = get_chunk_from_address(Vcb, tw->address);
        else {
            tree_write* tw2 = CONTAINING_RECORD(le->Blink, tree_write, list_entry);

            if (tw->address == tw2->address + tw2->length) {
                UINT8* data = ExAllocatePoolWithTag(NonPagedPool, tw2->length + tw->length, ALLOC_TAG);

                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(data, tw2->data, tw2->length);
                RtlCopyMemory(&data[tw2->length], tw->data, tw->length);

                if (!no_free)
                    ExFreePool(tw2->data);

                tw2->data = data;
                tw2->length += tw->length;

                if (!no_free) // FIXME - what if we allocated this just now?
                    ExFreePool(tw->data);

                RemoveEntryList(&tw->list_entry);
                ExFreePool(tw);

                le = tw2->list_entry.Flink;
                continue;
            }
        }

        tw->c = c;

        if (c->chunk_item->type & (BLOCK_FLAG_RAID5 | BLOCK_FLAG_RAID6))
            raid56 = TRUE;

        le = le->Flink;
    }

    num_bits = 0;

    le = tree_writes->Flink;
    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);

        num_bits++;

        le = le->Flink;
    }

    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_context) * num_bits, ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    le = tree_writes->Flink;

    while (le != tree_writes) {
        tw = CONTAINING_RECORD(le, tree_write, list_entry);

        TRACE("address: %llx, size: %x\n", tw->address, tw->length);

        KeInitializeEvent(&wtc[bit_num].Event, NotificationEvent, FALSE);
        InitializeListHead(&wtc[bit_num].stripes);
        wtc[bit_num].need_wait = FALSE;
        wtc[bit_num].stripes_left = 0;
        wtc[bit_num].parity1 = wtc[bit_num].parity2 = wtc[bit_num].scratch = NULL;
        wtc[bit_num].mdl = wtc[bit_num].parity1_mdl = wtc[bit_num].parity2_mdl = NULL;

        Status = write_data(Vcb, tw->address, tw->data, tw->length, &wtc[bit_num], NULL, NULL, FALSE, 0, HighPagePriority);
        if (!NT_SUCCESS(Status)) {
            ERR("write_data returned %08x\n", Status);

            for (i = 0; i < num_bits; i++) {
                free_write_data_stripes(&wtc[i]);
            }
            ExFreePool(wtc);

            return Status;
        }

        bit_num++;

        le = le->Flink;
    }

    for (i = 0; i < num_bits; i++) {
        if (wtc[i].stripes.Flink != &wtc[i].stripes) {
            // launch writes and wait
            le = wtc[i].stripes.Flink;
            while (le != &wtc[i].stripes) {
                write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

                if (stripe->status != WriteDataStatus_Ignore) {
                    wtc[i].need_wait = TRUE;
                    IoCallDriver(stripe->device->devobj, stripe->Irp);
                }

                le = le->Flink;
            }
        }
    }

    for (i = 0; i < num_bits; i++) {
        if (wtc[i].need_wait)
            KeWaitForSingleObject(&wtc[i].Event, Executive, KernelMode, FALSE, NULL);
    }

    for (i = 0; i < num_bits; i++) {
        le = wtc[i].stripes.Flink;
        while (le != &wtc[i].stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);

            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                log_device_error(Vcb, stripe->device, BTRFS_DEV_STAT_WRITE_ERRORS);
                break;
            }

            le = le->Flink;
        }

        free_write_data_stripes(&wtc[i]);
    }

    ExFreePool(wtc);

    if (raid56) {
        c = NULL;

        le = tree_writes->Flink;
        while (le != tree_writes) {
            tw = CONTAINING_RECORD(le, tree_write, list_entry);

            if (tw->c != c) {
                c = tw->c;

                ExAcquireResourceExclusiveLite(&c->partial_stripes_lock, TRUE);

                while (!IsListEmpty(&c->partial_stripes)) {
                    partial_stripe* ps = CONTAINING_RECORD(RemoveHeadList(&c->partial_stripes), partial_stripe, list_entry);

                    Status = flush_partial_stripe(Vcb, c, ps);

                    if (ps->bmparr)
                        ExFreePool(ps->bmparr);

                    ExFreePool(ps);

                    if (!NT_SUCCESS(Status)) {
                        ERR("flush_partial_stripe returned %08x\n", Status);
                        ExReleaseResourceLite(&c->partial_stripes_lock);
                        return Status;
                    }
                }

                ExReleaseResourceLite(&c->partial_stripes_lock);
            }

            le = le->Flink;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS write_trees(device_extension* Vcb, PIRP Irp) {
    ULONG level;
    UINT8 *data, *body;
    UINT32 crc32;
    NTSTATUS Status;
    LIST_ENTRY* le;
    LIST_ENTRY tree_writes;
    tree_write* tw;

    TRACE("(%p)\n", Vcb);

    InitializeListHead(&tree_writes);

    for (level = 0; level <= 255; level++) {
        BOOL nothing_found = TRUE;

        TRACE("level = %u\n", level);

        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);

            if (t->write && t->header.level == level) {
                KEY firstitem, searchkey;
                LIST_ENTRY* le2;
                traverse_ptr tp;

                if (!t->has_new_address) {
                    ERR("error - tried to write tree with no new address\n");
                    return STATUS_INTERNAL_ERROR;
                }

                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        firstitem = td->key;
                        break;
                    }
                    le2 = le2->Flink;
                }

                if (t->parent) {
                    t->paritem->key = firstitem;
                    t->paritem->treeholder.address = t->new_address;
                    t->paritem->treeholder.generation = Vcb->superblock.generation;
                }

                if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
                    EXTENT_ITEM_TREE* eit;

                    searchkey.obj_id = t->new_address;
                    searchkey.obj_type = TYPE_EXTENT_ITEM;
                    searchkey.offset = Vcb->superblock.node_size;

                    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        return Status;
                    }

                    if (keycmp(searchkey, tp.item->key)) {
                        ERR("could not find %llx,%x,%llx in extent_root (found %llx,%x,%llx instead)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        return STATUS_INTERNAL_ERROR;
                    }

                    if (tp.item->size < sizeof(EXTENT_ITEM_TREE)) {
                        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_TREE));
                        return STATUS_INTERNAL_ERROR;
                    }

                    eit = (EXTENT_ITEM_TREE*)tp.item->data;
                    eit->firstitem = firstitem;
                }

                nothing_found = FALSE;
            }

            le = le->Flink;
        }

        if (nothing_found)
            break;
    }

    TRACE("allocated tree extents\n");

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        LIST_ENTRY* le2;
#ifdef DEBUG_PARANOID
        UINT32 num_items = 0, size = 0;
        BOOL crash = FALSE;
#endif

        if (t->write) {
#ifdef DEBUG_PARANOID
            BOOL first = TRUE;
            KEY lastkey;

            le2 = t->itemlist.Flink;
            while (le2 != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                if (!td->ignore) {
                    num_items++;

                    if (!first) {
                        if (keycmp(td->key, lastkey) == 0) {
                            ERR("(%llx,%x,%llx): duplicate key\n", td->key.obj_id, td->key.obj_type, td->key.offset);
                            crash = TRUE;
                        } else if (keycmp(td->key, lastkey) == -1) {
                            ERR("(%llx,%x,%llx): key out of order\n", td->key.obj_id, td->key.obj_type, td->key.offset);
                            crash = TRUE;
                        }
                    } else
                        first = FALSE;

                    lastkey = td->key;

                    if (t->header.level == 0)
                        size += td->size;
                }
                le2 = le2->Flink;
            }

            if (t->header.level == 0)
                size += num_items * sizeof(leaf_node);
            else
                size += num_items * sizeof(internal_node);

            if (num_items != t->header.num_items) {
                ERR("tree %llx, level %x: num_items was %x, expected %x\n", t->root->id, t->header.level, num_items, t->header.num_items);
                crash = TRUE;
            }

            if (size != t->size) {
                ERR("tree %llx, level %x: size was %x, expected %x\n", t->root->id, t->header.level, size, t->size);
                crash = TRUE;
            }

            if (t->header.num_items == 0 && t->parent) {
                ERR("tree %llx, level %x: tried to write empty tree with parent\n", t->root->id, t->header.level);
                crash = TRUE;
            }

            if (t->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                ERR("tree %llx, level %x: tried to write overlarge tree (%x > %x)\n", t->root->id, t->header.level, t->size, Vcb->superblock.node_size - sizeof(tree_header));
                crash = TRUE;
            }

            if (crash) {
                ERR("tree %p\n", t);
                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        ERR("%llx,%x,%llx inserted=%u\n", td->key.obj_id, td->key.obj_type, td->key.offset, td->inserted);
                    }
                    le2 = le2->Flink;
                }
                int3;
            }
#endif
            t->header.address = t->new_address;
            t->header.generation = Vcb->superblock.generation;
            t->header.tree_id = t->root->id;
            t->header.flags |= HEADER_FLAG_MIXED_BACKREF;
            t->header.fs_uuid = Vcb->superblock.uuid;
            t->has_address = TRUE;

            data = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            body = data + sizeof(tree_header);

            RtlCopyMemory(data, &t->header, sizeof(tree_header));
            RtlZeroMemory(body, Vcb->superblock.node_size - sizeof(tree_header));

            if (t->header.level == 0) {
                leaf_node* itemptr = (leaf_node*)body;
                int i = 0;
                UINT8* dataptr = data + Vcb->superblock.node_size;

                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        dataptr = dataptr - td->size;

                        itemptr[i].key = td->key;
                        itemptr[i].offset = (UINT32)((UINT8*)dataptr - (UINT8*)body);
                        itemptr[i].size = td->size;
                        i++;

                        if (td->size > 0)
                            RtlCopyMemory(dataptr, td->data, td->size);
                    }

                    le2 = le2->Flink;
                }
            } else {
                internal_node* itemptr = (internal_node*)body;
                int i = 0;

                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        itemptr[i].key = td->key;
                        itemptr[i].address = td->treeholder.address;
                        itemptr[i].generation = td->treeholder.generation;
                        i++;
                    }

                    le2 = le2->Flink;
                }
            }

            crc32 = calc_crc32c(0xffffffff, (UINT8*)&((tree_header*)data)->fs_uuid, Vcb->superblock.node_size - sizeof(((tree_header*)data)->csum));
            crc32 = ~crc32;
            *((UINT32*)data) = crc32;
            TRACE("setting crc32 to %08x\n", crc32);

            tw = ExAllocatePoolWithTag(PagedPool, sizeof(tree_write), ALLOC_TAG);
            if (!tw) {
                ERR("out of memory\n");
                ExFreePool(data);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            tw->address = t->new_address;
            tw->length = Vcb->superblock.node_size;
            tw->data = data;

            if (IsListEmpty(&tree_writes))
                InsertTailList(&tree_writes, &tw->list_entry);
            else {
                BOOL inserted = FALSE;

                le2 = tree_writes.Flink;
                while (le2 != &tree_writes) {
                    tree_write* tw2 = CONTAINING_RECORD(le2, tree_write, list_entry);

                    if (tw2->address > tw->address) {
                        InsertHeadList(le2->Blink, &tw->list_entry);
                        inserted = TRUE;
                        break;
                    }

                    le2 = le2->Flink;
                }

                if (!inserted)
                    InsertTailList(&tree_writes, &tw->list_entry);
            }
        }

        le = le->Flink;
    }

    Status = do_tree_writes(Vcb, &tree_writes, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("do_tree_writes returned %08x\n", Status);
        goto end;
    }

    Status = STATUS_SUCCESS;

end:
    while (!IsListEmpty(&tree_writes)) {
        le = RemoveHeadList(&tree_writes);
        tw = CONTAINING_RECORD(le, tree_write, list_entry);

        if (tw->data)
            ExFreePool(tw->data);

        ExFreePool(tw);
    }

    return Status;
}

static void update_backup_superblock(device_extension* Vcb, superblock_backup* sb, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;

    RtlZeroMemory(sb, sizeof(superblock_backup));

    sb->root_tree_addr = Vcb->superblock.root_tree_addr;
    sb->root_tree_generation = Vcb->superblock.generation;
    sb->root_level = Vcb->superblock.root_level;

    sb->chunk_tree_addr = Vcb->superblock.chunk_tree_addr;
    sb->chunk_tree_generation = Vcb->superblock.chunk_root_generation;
    sb->chunk_root_level = Vcb->superblock.chunk_root_level;

    searchkey.obj_id = BTRFS_ROOT_EXTENT;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;

            sb->extent_tree_addr = ri->block_number;
            sb->extent_tree_generation = ri->generation;
            sb->extent_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_FSTREE;

    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;

            sb->fs_tree_addr = ri->block_number;
            sb->fs_tree_generation = ri->generation;
            sb->fs_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_DEVTREE;

    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;

            sb->dev_root_addr = ri->block_number;
            sb->dev_root_generation = ri->generation;
            sb->dev_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_CHECKSUM;

    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;

            sb->csum_root_addr = ri->block_number;
            sb->csum_root_generation = ri->generation;
            sb->csum_root_level = ri->root_level;
        }
    }

    sb->total_bytes = Vcb->superblock.total_bytes;
    sb->bytes_used = Vcb->superblock.bytes_used;
    sb->num_devices = Vcb->superblock.num_devices;
}

typedef struct {
    void* context;
    UINT8* buf;
    PMDL mdl;
    device* device;
    NTSTATUS Status;
    PIRP Irp;
    LIST_ENTRY list_entry;
} write_superblocks_stripe;

typedef struct _write_superblocks_context {
    KEVENT Event;
    LIST_ENTRY stripes;
    LONG left;
} write_superblocks_context;

_Function_class_(IO_COMPLETION_ROUTINE)
#ifdef __REACTOS__
static NTSTATUS NTAPI write_superblock_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS write_superblock_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    write_superblocks_stripe* stripe = conptr;
    write_superblocks_context* context = stripe->context;

    UNUSED(DeviceObject);

    stripe->Status = Irp->IoStatus.Status;

    if (InterlockedDecrement(&context->left) == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS write_superblock(device_extension* Vcb, device* device, write_superblocks_context* context) {
    unsigned int i = 0;

    // All the documentation says that the Linux driver only writes one superblock
    // if it thinks a disk is an SSD, but this doesn't seem to be the case!

    while (superblock_addrs[i] > 0 && device->devitem.num_bytes >= superblock_addrs[i] + sizeof(superblock)) {
        ULONG sblen = (ULONG)sector_align(sizeof(superblock), Vcb->superblock.sector_size);
        superblock* sb;
        UINT32 crc32;
        write_superblocks_stripe* stripe;
        PIO_STACK_LOCATION IrpSp;

        sb = ExAllocatePoolWithTag(NonPagedPool, sblen, ALLOC_TAG);
        if (!sb) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(sb, &Vcb->superblock, sizeof(superblock));

        if (sblen > sizeof(superblock))
            RtlZeroMemory((UINT8*)sb + sizeof(superblock), sblen - sizeof(superblock));

        RtlCopyMemory(&sb->dev_item, &device->devitem, sizeof(DEV_ITEM));
        sb->sb_phys_addr = superblock_addrs[i];

        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));
        RtlCopyMemory(&sb->checksum, &crc32, sizeof(UINT32));

        stripe = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_superblocks_stripe), ALLOC_TAG);
        if (!stripe) {
            ERR("out of memory\n");
            ExFreePool(sb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        stripe->buf = (UINT8*)sb;

        stripe->Irp = IoAllocateIrp(device->devobj->StackSize, FALSE);
        if (!stripe->Irp) {
            ERR("IoAllocateIrp failed\n");
            ExFreePool(stripe);
            ExFreePool(sb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
        IrpSp->MajorFunction = IRP_MJ_WRITE;

        if (i == 0)
            IrpSp->Flags |= SL_WRITE_THROUGH;

        if (device->devobj->Flags & DO_BUFFERED_IO) {
            stripe->Irp->AssociatedIrp.SystemBuffer = sb;
            stripe->mdl = NULL;

            stripe->Irp->Flags = IRP_BUFFERED_IO;
        } else if (device->devobj->Flags & DO_DIRECT_IO) {
            stripe->mdl = IoAllocateMdl(sb, sblen, FALSE, FALSE, NULL);
            if (!stripe->mdl) {
                ERR("IoAllocateMdl failed\n");
                IoFreeIrp(stripe->Irp);
                ExFreePool(stripe);
                ExFreePool(sb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            stripe->Irp->MdlAddress = stripe->mdl;

            MmBuildMdlForNonPagedPool(stripe->mdl);
        } else {
            stripe->Irp->UserBuffer = sb;
            stripe->mdl = NULL;
        }

        IrpSp->Parameters.Write.Length = sblen;
        IrpSp->Parameters.Write.ByteOffset.QuadPart = superblock_addrs[i];

        IoSetCompletionRoutine(stripe->Irp, write_superblock_completion, stripe, TRUE, TRUE, TRUE);

        stripe->context = context;
        stripe->device = device;
        InsertTailList(&context->stripes, &stripe->list_entry);

        context->left++;

        i++;
    }

    if (i == 0)
        ERR("no superblocks written!\n");

    return STATUS_SUCCESS;
}

static NTSTATUS write_superblocks(device_extension* Vcb, PIRP Irp) {
    UINT64 i;
    NTSTATUS Status;
    LIST_ENTRY* le;
    write_superblocks_context context;

    TRACE("(%p)\n", Vcb);

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);

        if (t->write && !t->parent) {
            if (t->root == Vcb->root_root) {
                Vcb->superblock.root_tree_addr = t->new_address;
                Vcb->superblock.root_level = t->header.level;
            } else if (t->root == Vcb->chunk_root) {
                Vcb->superblock.chunk_tree_addr = t->new_address;
                Vcb->superblock.chunk_root_generation = t->header.generation;
                Vcb->superblock.chunk_root_level = t->header.level;
            }
        }

        le = le->Flink;
    }

    for (i = 0; i < BTRFS_NUM_BACKUP_ROOTS - 1; i++) {
        RtlCopyMemory(&Vcb->superblock.backup[i], &Vcb->superblock.backup[i+1], sizeof(superblock_backup));
    }

    update_backup_superblock(Vcb, &Vcb->superblock.backup[BTRFS_NUM_BACKUP_ROOTS - 1], Irp);

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    InitializeListHead(&context.stripes);
    context.left = 0;

    le = Vcb->devices.Flink;
    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj && !dev->readonly) {
            Status = write_superblock(Vcb, dev, &context);
            if (!NT_SUCCESS(Status)) {
                ERR("write_superblock returned %08x\n", Status);
                goto end;
            }
        }

        le = le->Flink;
    }

    if (IsListEmpty(&context.stripes)) {
        ERR("error - not writing any superblocks\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    le = context.stripes.Flink;
    while (le != &context.stripes) {
        write_superblocks_stripe* stripe = CONTAINING_RECORD(le, write_superblocks_stripe, list_entry);

        IoCallDriver(stripe->device->devobj, stripe->Irp);

        le = le->Flink;
    }

    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

    le = context.stripes.Flink;
    while (le != &context.stripes) {
        write_superblocks_stripe* stripe = CONTAINING_RECORD(le, write_superblocks_stripe, list_entry);

        if (!NT_SUCCESS(stripe->Status)) {
            ERR("device %llx returned %08x\n", stripe->device->devitem.dev_id, stripe->Status);
            log_device_error(Vcb, stripe->device, BTRFS_DEV_STAT_WRITE_ERRORS);
            Status = stripe->Status;
            goto end;
        }

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    while (!IsListEmpty(&context.stripes)) {
        write_superblocks_stripe* stripe = CONTAINING_RECORD(RemoveHeadList(&context.stripes), write_superblocks_stripe, list_entry);

        if (stripe->mdl) {
            if (stripe->mdl->MdlFlags & MDL_PAGES_LOCKED)
                MmUnlockPages(stripe->mdl);

            IoFreeMdl(stripe->mdl);
        }

        if (stripe->Irp)
            IoFreeIrp(stripe->Irp);

        if (stripe->buf)
            ExFreePool(stripe->buf);

        ExFreePool(stripe);
    }

    return Status;
}

static NTSTATUS flush_changed_extent(device_extension* Vcb, chunk* c, changed_extent* ce, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    UINT64 old_size;

    if (ce->count == 0 && ce->old_count == 0) {
        while (!IsListEmpty(&ce->refs)) {
            changed_extent_ref* cer = CONTAINING_RECORD(RemoveHeadList(&ce->refs), changed_extent_ref, list_entry);
            ExFreePool(cer);
        }

        while (!IsListEmpty(&ce->old_refs)) {
            changed_extent_ref* cer = CONTAINING_RECORD(RemoveHeadList(&ce->old_refs), changed_extent_ref, list_entry);
            ExFreePool(cer);
        }

        goto end;
    }

    le = ce->refs.Flink;
    while (le != &ce->refs) {
        changed_extent_ref* cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        UINT32 old_count = 0;

        if (cer->type == TYPE_EXTENT_DATA_REF) {
            le2 = ce->old_refs.Flink;
            while (le2 != &ce->old_refs) {
                changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                if (cer2->type == TYPE_EXTENT_DATA_REF && cer2->edr.root == cer->edr.root && cer2->edr.objid == cer->edr.objid && cer2->edr.offset == cer->edr.offset) {
                    old_count = cer2->edr.count;
                    break;
                }

                le2 = le2->Flink;
            }

            old_size = ce->old_count > 0 ? ce->old_size : ce->size;

            if (cer->edr.count > old_count) {
                Status = increase_extent_refcount_data(Vcb, ce->address, old_size, cer->edr.root, cer->edr.objid, cer->edr.offset, cer->edr.count - old_count, Irp);

                if (!NT_SUCCESS(Status)) {
                    ERR("increase_extent_refcount_data returned %08x\n", Status);
                    return Status;
                }
            }
        } else if (cer->type == TYPE_SHARED_DATA_REF) {
            le2 = ce->old_refs.Flink;
            while (le2 != &ce->old_refs) {
                changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                if (cer2->type == TYPE_SHARED_DATA_REF && cer2->sdr.offset == cer->sdr.offset) {
                    RemoveEntryList(&cer2->list_entry);
                    ExFreePool(cer2);
                    break;
                }

                le2 = le2->Flink;
            }
        }

        le = le->Flink;
    }

    le = ce->refs.Flink;
    while (le != &ce->refs) {
        changed_extent_ref* cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        LIST_ENTRY* le3 = le->Flink;
        UINT32 old_count = 0;

        if (cer->type == TYPE_EXTENT_DATA_REF) {
            le2 = ce->old_refs.Flink;
            while (le2 != &ce->old_refs) {
                changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);

                if (cer2->type == TYPE_EXTENT_DATA_REF && cer2->edr.root == cer->edr.root && cer2->edr.objid == cer->edr.objid && cer2->edr.offset == cer->edr.offset) {
                    old_count = cer2->edr.count;

                    RemoveEntryList(&cer2->list_entry);
                    ExFreePool(cer2);
                    break;
                }

                le2 = le2->Flink;
            }

            old_size = ce->old_count > 0 ? ce->old_size : ce->size;

            if (cer->edr.count < old_count) {
                Status = decrease_extent_refcount_data(Vcb, ce->address, old_size, cer->edr.root, cer->edr.objid, cer->edr.offset,
                                                       old_count - cer->edr.count, ce->superseded, Irp);

                if (!NT_SUCCESS(Status)) {
                    ERR("decrease_extent_refcount_data returned %08x\n", Status);
                    return Status;
                }
            }

            if (ce->size != ce->old_size && ce->old_count > 0) {
                KEY searchkey;
                traverse_ptr tp;
                void* data;

                searchkey.obj_id = ce->address;
                searchkey.obj_type = TYPE_EXTENT_ITEM;
                searchkey.offset = ce->old_size;

                Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }

                if (keycmp(searchkey, tp.item->key)) {
                    ERR("could not find (%llx,%x,%llx) in extent tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                    return STATUS_INTERNAL_ERROR;
                }

                if (tp.item->size > 0) {
                    data = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);

                    if (!data) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(data, tp.item->data, tp.item->size);
                } else
                    data = NULL;

                Status = insert_tree_item(Vcb, Vcb->extent_root, ce->address, TYPE_EXTENT_ITEM, ce->size, data, tp.item->size, NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    if (data) ExFreePool(data);
                    return Status;
                }

                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    return Status;
                }
            }
        }

        RemoveEntryList(&cer->list_entry);
        ExFreePool(cer);

        le = le3;
    }

#ifdef DEBUG_PARANOID
    if (!IsListEmpty(&ce->old_refs))
        WARN("old_refs not empty\n");
#endif

end:
    if (ce->count == 0 && !ce->superseded) {
        c->used -= ce->size;
        space_list_add(c, ce->address, ce->size, rollback);
    }

    RemoveEntryList(&ce->list_entry);
    ExFreePool(ce);

    return STATUS_SUCCESS;
}

void add_checksum_entry(device_extension* Vcb, UINT64 address, ULONG length, UINT32* csum, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    UINT64 startaddr, endaddr;
    ULONG len;
    UINT32* checksums;
    RTL_BITMAP bmp;
    ULONG* bmparr;
    ULONG runlength, index;

    searchkey.obj_id = EXTENT_CSUM_ID;
    searchkey.obj_type = TYPE_EXTENT_CSUM;
    searchkey.offset = address;

    // FIXME - create checksum_root if it doesn't exist at all

    Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
    if (Status == STATUS_NOT_FOUND) { // tree is completely empty
        if (csum) { // not deleted
            ULONG length2 = length;
            UINT64 off = address;
            UINT32* data = csum;

            do {
                UINT16 il = (UINT16)min(length2, MAX_CSUM_SIZE / sizeof(UINT32));

                checksums = ExAllocatePoolWithTag(PagedPool, il * sizeof(UINT32), ALLOC_TAG);
                if (!checksums) {
                    ERR("out of memory\n");
                    return;
                }

                RtlCopyMemory(checksums, data, il * sizeof(UINT32));

                Status = insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, off, checksums,
                                          il * sizeof(UINT32), NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    ExFreePool(checksums);
                    return;
                }

                length2 -= il;

                if (length2 > 0) {
                    off += il * Vcb->superblock.sector_size;
                    data += il;
                }
            } while (length2 > 0);
        }
    } else if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return;
    } else {
        UINT32 tplen;

        // FIXME - check entry is TYPE_EXTENT_CSUM?

        if (tp.item->key.offset < address && tp.item->key.offset + (tp.item->size * Vcb->superblock.sector_size / sizeof(UINT32)) >= address)
            startaddr = tp.item->key.offset;
        else
            startaddr = address;

        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = address + (length * Vcb->superblock.sector_size);

        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return;
        }

        tplen = tp.item->size / sizeof(UINT32);

        if (tp.item->key.offset + (tplen * Vcb->superblock.sector_size) >= address + (length * Vcb->superblock.sector_size))
            endaddr = tp.item->key.offset + (tplen * Vcb->superblock.sector_size);
        else
            endaddr = address + (length * Vcb->superblock.sector_size);

        TRACE("cs starts at %llx (%x sectors)\n", address, length);
        TRACE("startaddr = %llx\n", startaddr);
        TRACE("endaddr = %llx\n", endaddr);

        len = (ULONG)((endaddr - startaddr) / Vcb->superblock.sector_size);

        checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * len, ALLOC_TAG);
        if (!checksums) {
            ERR("out of memory\n");
            return;
        }

        bmparr = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) * ((len/8)+1), ALLOC_TAG);
        if (!bmparr) {
            ERR("out of memory\n");
            ExFreePool(checksums);
            return;
        }

        RtlInitializeBitMap(&bmp, bmparr, len);
        RtlSetAllBits(&bmp);

        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = address;

        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            ExFreePool(checksums);
            ExFreePool(bmparr);
            return;
        }

        // set bit = free space, cleared bit = allocated sector

        while (tp.item->key.offset < endaddr) {
            if (tp.item->key.offset >= startaddr) {
                if (tp.item->size > 0) {
                    ULONG itemlen = (ULONG)min((len - (tp.item->key.offset - startaddr) / Vcb->superblock.sector_size) * sizeof(UINT32), tp.item->size);

                    RtlCopyMemory(&checksums[(tp.item->key.offset - startaddr) / Vcb->superblock.sector_size], tp.item->data, itemlen);
                    RtlClearBits(&bmp, (ULONG)((tp.item->key.offset - startaddr) / Vcb->superblock.sector_size), itemlen / sizeof(UINT32));
                }

                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    ExFreePool(checksums);
                    ExFreePool(bmparr);
                    return;
                }
            }

            if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
                tp = next_tp;
            } else
                break;
        }

        if (!csum) { // deleted
            RtlSetBits(&bmp, (ULONG)((address - startaddr) / Vcb->superblock.sector_size), length);
        } else {
            RtlCopyMemory(&checksums[(address - startaddr) / Vcb->superblock.sector_size], csum, length * sizeof(UINT32));
            RtlClearBits(&bmp, (ULONG)((address - startaddr) / Vcb->superblock.sector_size), length);
        }

        runlength = RtlFindFirstRunClear(&bmp, &index);

        while (runlength != 0) {
            do {
                UINT16 rl;
                UINT64 off;
                UINT32* data;

                if (runlength * sizeof(UINT32) > MAX_CSUM_SIZE)
                    rl = MAX_CSUM_SIZE / sizeof(UINT32);
                else
                    rl = (UINT16)runlength;

                data = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * rl, ALLOC_TAG);
                if (!data) {
                    ERR("out of memory\n");
                    ExFreePool(bmparr);
                    ExFreePool(checksums);
                    return;
                }

                RtlCopyMemory(data, &checksums[index], sizeof(UINT32) * rl);

                off = startaddr + UInt32x32To64(index, Vcb->superblock.sector_size);

                Status = insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, off, data, sizeof(UINT32) * rl, NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    ExFreePool(data);
                    ExFreePool(bmparr);
                    ExFreePool(checksums);
                    return;
                }

                runlength -= rl;
                index += rl;
            } while (runlength > 0);

            runlength = RtlFindNextForwardRunClear(&bmp, index, &index);
        }

        ExFreePool(bmparr);
        ExFreePool(checksums);
    }
}

static NTSTATUS update_chunk_usage(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks.Flink, *le2;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;

    TRACE("(%p)\n", Vcb);

    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);

    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        acquire_chunk_lock(c, Vcb);

        if (!c->cache_loaded && (!IsListEmpty(&c->changed_extents) || c->used != c->oldused)) {
            Status = load_cache_chunk(Vcb, c, NULL);

            if (!NT_SUCCESS(Status)) {
                ERR("load_cache_chunk returned %08x\n", Status);
                release_chunk_lock(c, Vcb);
                goto end;
            }
        }

        le2 = c->changed_extents.Flink;
        while (le2 != &c->changed_extents) {
            LIST_ENTRY* le3 = le2->Flink;
            changed_extent* ce = CONTAINING_RECORD(le2, changed_extent, list_entry);

            Status = flush_changed_extent(Vcb, c, ce, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("flush_changed_extent returned %08x\n", Status);
                release_chunk_lock(c, Vcb);
                goto end;
            }

            le2 = le3;
        }

        // This is usually done by update_chunks, but we have to check again in case any new chunks
        // have been allocated since.
        if (c->created) {
            Status = create_chunk(Vcb, c, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("create_chunk returned %08x\n", Status);
                release_chunk_lock(c, Vcb);
                goto end;
            }
        }

        if (c->old_cache) {
            if (c->old_cache->dirty) {
                LIST_ENTRY batchlist;

                InitializeListHead(&batchlist);

                Status = flush_fcb(c->old_cache, FALSE, &batchlist, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("flush_fcb returned %08x\n", Status);
                    release_chunk_lock(c, Vcb);
                    clear_batch_list(Vcb, &batchlist);
                    goto end;
                }

                Status = commit_batch_list(Vcb, &batchlist, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("commit_batch_list returned %08x\n", Status);
                    release_chunk_lock(c, Vcb);
                    goto end;
                }
            }

            free_fcb(c->old_cache);

            if (c->old_cache->refcount == 0)
                reap_fcb(c->old_cache);

            c->old_cache = NULL;
        }

        if (c->used != c->oldused) {
#ifdef __REACTOS__
            UINT64 old_phys_used, phys_used;
#endif
            searchkey.obj_id = c->offset;
            searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
            searchkey.offset = c->chunk_item->size;

            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                release_chunk_lock(c, Vcb);
                goto end;
            }

            if (keycmp(searchkey, tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                Status = STATUS_INTERNAL_ERROR;
                release_chunk_lock(c, Vcb);
                goto end;
            }

            if (tp.item->size < sizeof(BLOCK_GROUP_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
                Status = STATUS_INTERNAL_ERROR;
                release_chunk_lock(c, Vcb);
                goto end;
            }

            bgi = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!bgi) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                release_chunk_lock(c, Vcb);
                goto end;
            }

            RtlCopyMemory(bgi, tp.item->data, tp.item->size);
            bgi->used = c->used;

            TRACE("adjusting usage of chunk %llx to %llx\n", c->offset, c->used);

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                ExFreePool(bgi);
                release_chunk_lock(c, Vcb);
                goto end;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, bgi, tp.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08x\n", Status);
                ExFreePool(bgi);
                release_chunk_lock(c, Vcb);
                goto end;
            }

#ifndef __REACTOS__
            UINT64 old_phys_used = chunk_estimate_phys_size(Vcb, c, c->oldused);
            UINT64 phys_used = chunk_estimate_phys_size(Vcb, c, c->used);
#else
            old_phys_used = chunk_estimate_phys_size(Vcb, c, c->oldused);
            phys_used = chunk_estimate_phys_size(Vcb, c, c->used);
#endif

            if (Vcb->superblock.bytes_used + phys_used > old_phys_used)
                Vcb->superblock.bytes_used += phys_used - old_phys_used;
            else
                Vcb->superblock.bytes_used = 0;

            c->oldused = c->used;
        }

        release_chunk_lock(c, Vcb);

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&Vcb->chunk_lock);

    return Status;
}

static void get_first_item(tree* t, KEY* key) {
    LIST_ENTRY* le;

    le = t->itemlist.Flink;
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

        *key = td->key;
        return;
    }
}

static NTSTATUS split_tree_at(device_extension* Vcb, tree* t, tree_data* newfirstitem, UINT32 numitems, UINT32 size) {
    tree *nt, *pt;
    tree_data* td;
    tree_data* oldlastitem;

    TRACE("splitting tree in %llx at (%llx,%x,%llx)\n", t->root->id, newfirstitem->key.obj_id, newfirstitem->key.obj_type, newfirstitem->key.offset);

    nt = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!nt) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (t->header.level > 0) {
        nt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
        if (!nt->nonpaged) {
            ERR("out of memory\n");
            ExFreePool(nt);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ExInitializeFastMutex(&nt->nonpaged->mutex);
    } else
        nt->nonpaged = NULL;

    RtlCopyMemory(&nt->header, &t->header, sizeof(tree_header));
    nt->header.address = 0;
    nt->header.generation = Vcb->superblock.generation;
    nt->header.num_items = t->header.num_items - numitems;
    nt->header.flags = HEADER_FLAG_MIXED_BACKREF | HEADER_FLAG_WRITTEN;

    nt->has_address = FALSE;
    nt->Vcb = Vcb;
    nt->parent = t->parent;

#ifdef DEBUG_PARANOID
    if (nt->parent && nt->parent->header.level <= nt->header.level) int3;
#endif

    nt->root = t->root;
    nt->new_address = 0;
    nt->has_new_address = FALSE;
    nt->updated_extents = FALSE;
    nt->uniqueness_determined = TRUE;
    nt->is_unique = TRUE;
    nt->list_entry_hash.Flink = NULL;
    nt->buf = NULL;
    InitializeListHead(&nt->itemlist);

    oldlastitem = CONTAINING_RECORD(newfirstitem->list_entry.Blink, tree_data, list_entry);

    nt->itemlist.Flink = &newfirstitem->list_entry;
    nt->itemlist.Blink = t->itemlist.Blink;
    nt->itemlist.Flink->Blink = &nt->itemlist;
    nt->itemlist.Blink->Flink = &nt->itemlist;

    t->itemlist.Blink = &oldlastitem->list_entry;
    t->itemlist.Blink->Flink = &t->itemlist;

    nt->size = t->size - size;
    t->size = size;
    t->header.num_items = numitems;
    nt->write = TRUE;

    InsertTailList(&Vcb->trees, &nt->list_entry);

    if (nt->header.level > 0) {
        LIST_ENTRY* le = nt->itemlist.Flink;

        while (le != &nt->itemlist) {
            tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);

            if (td2->treeholder.tree) {
                td2->treeholder.tree->parent = nt;
#ifdef DEBUG_PARANOID
                if (td2->treeholder.tree->parent && td2->treeholder.tree->parent->header.level <= td2->treeholder.tree->header.level) int3;
#endif
            }

            le = le->Flink;
        }
    } else {
        LIST_ENTRY* le = nt->itemlist.Flink;

        while (le != &nt->itemlist) {
            tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);

            if (!td2->inserted && td2->data) {
                UINT8* data = ExAllocatePoolWithTag(PagedPool, td2->size, ALLOC_TAG);

                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(data, td2->data, td2->size);
                td2->data = data;
                td2->inserted = TRUE;
            }

            le = le->Flink;
        }
    }

    if (nt->parent) {
        td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
        if (!td) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        td->key = newfirstitem->key;

        InsertHeadList(&t->paritem->list_entry, &td->list_entry);

        td->ignore = FALSE;
        td->inserted = TRUE;
        td->treeholder.tree = nt;
        nt->paritem = td;

        nt->parent->header.num_items++;
        nt->parent->size += sizeof(internal_node);

        goto end;
    }

    TRACE("adding new tree parent\n");

    if (nt->header.level == 255) {
        ERR("cannot add parent to tree at level 255\n");
        return STATUS_INTERNAL_ERROR;
    }

    pt = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!pt) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    if (!pt->nonpaged) {
        ERR("out of memory\n");
        ExFreePool(pt);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeFastMutex(&pt->nonpaged->mutex);

    RtlCopyMemory(&pt->header, &nt->header, sizeof(tree_header));
    pt->header.address = 0;
    pt->header.num_items = 2;
    pt->header.level = nt->header.level + 1;
    pt->header.flags = HEADER_FLAG_MIXED_BACKREF | HEADER_FLAG_WRITTEN;

    pt->has_address = FALSE;
    pt->Vcb = Vcb;
    pt->parent = NULL;
    pt->paritem = NULL;
    pt->root = t->root;
    pt->new_address = 0;
    pt->has_new_address = FALSE;
    pt->updated_extents = FALSE;
    pt->size = pt->header.num_items * sizeof(internal_node);
    pt->uniqueness_determined = TRUE;
    pt->is_unique = TRUE;
    pt->list_entry_hash.Flink = NULL;
    pt->buf = NULL;
    InitializeListHead(&pt->itemlist);

    InsertTailList(&Vcb->trees, &pt->list_entry);

    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
    if (!td) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    get_first_item(t, &td->key);
    td->ignore = FALSE;
    td->inserted = FALSE;
    td->treeholder.address = 0;
    td->treeholder.generation = Vcb->superblock.generation;
    td->treeholder.tree = t;
    InsertTailList(&pt->itemlist, &td->list_entry);
    t->paritem = td;

    td = ExAllocateFromPagedLookasideList(&Vcb->tree_data_lookaside);
    if (!td) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    td->key = newfirstitem->key;
    td->ignore = FALSE;
    td->inserted = FALSE;
    td->treeholder.address = 0;
    td->treeholder.generation = Vcb->superblock.generation;
    td->treeholder.tree = nt;
    InsertTailList(&pt->itemlist, &td->list_entry);
    nt->paritem = td;

    pt->write = TRUE;

    t->root->treeholder.tree = pt;

    t->parent = pt;
    nt->parent = pt;

#ifdef DEBUG_PARANOID
    if (t->parent && t->parent->header.level <= t->header.level) int3;
    if (nt->parent && nt->parent->header.level <= nt->header.level) int3;
#endif

end:
    t->root->root_item.bytes_used += Vcb->superblock.node_size;

    return STATUS_SUCCESS;
}

static NTSTATUS split_tree(device_extension* Vcb, tree* t) {
    LIST_ENTRY* le;
    UINT32 size, ds, numitems;

    size = 0;
    numitems = 0;

    // FIXME - naïve implementation: maximizes number of filled trees

    le = t->itemlist.Flink;
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

        if (!td->ignore) {
            if (t->header.level == 0)
                ds = sizeof(leaf_node) + td->size;
            else
                ds = sizeof(internal_node);

            if (numitems == 0 && ds > Vcb->superblock.node_size - sizeof(tree_header)) {
                ERR("(%llx,%x,%llx) in tree %llx is too large (%x > %x)\n",
                    td->key.obj_id, td->key.obj_type, td->key.offset, t->root->id,
                    ds, Vcb->superblock.node_size - sizeof(tree_header));
                return STATUS_INTERNAL_ERROR;
            }

            // FIXME - move back if previous item was deleted item with same key
            if (size + ds > Vcb->superblock.node_size - sizeof(tree_header))
                return split_tree_at(Vcb, t, td, numitems, size);

            size += ds;
            numitems++;
        }

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

BOOL is_tree_unique(device_extension* Vcb, tree* t, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    BOOL ret = FALSE;
    EXTENT_ITEM* ei;
    UINT8* type;

    if (t->uniqueness_determined)
        return t->is_unique;

    if (t->parent && !is_tree_unique(Vcb, t->parent, Irp))
        goto end;

    if (t->has_address) {
        searchkey.obj_id = t->header.address;
        searchkey.obj_type = Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA ? TYPE_METADATA_ITEM : TYPE_EXTENT_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }

        if (tp.item->key.obj_id != t->header.address || (tp.item->key.obj_type != TYPE_METADATA_ITEM && tp.item->key.obj_type != TYPE_EXTENT_ITEM))
            goto end;

        if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && tp.item->size == sizeof(EXTENT_ITEM_V0))
            goto end;

        if (tp.item->size < sizeof(EXTENT_ITEM))
            goto end;

        ei = (EXTENT_ITEM*)tp.item->data;

        if (ei->refcount > 1)
            goto end;

        if (tp.item->key.obj_type == TYPE_EXTENT_ITEM && ei->flags & EXTENT_ITEM_TREE_BLOCK) {
            EXTENT_ITEM2* ei2;

            if (tp.item->size < sizeof(EXTENT_ITEM) + sizeof(EXTENT_ITEM2))
                goto end;

            ei2 = (EXTENT_ITEM2*)&ei[1];
            type = (UINT8*)&ei2[1];
        } else
            type = (UINT8*)&ei[1];

        if (type >= tp.item->data + tp.item->size || *type != TYPE_TREE_BLOCK_REF)
            goto end;
    }

    ret = TRUE;

end:
    t->is_unique = ret;
    t->uniqueness_determined = TRUE;

    return ret;
}

static NTSTATUS try_tree_amalgamate(device_extension* Vcb, tree* t, BOOL* done, BOOL* done_deletions, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    tree_data* nextparitem = NULL;
    NTSTATUS Status;
    tree *next_tree, *par;

    *done = FALSE;

    TRACE("trying to amalgamate tree in root %llx, level %x (size %u)\n", t->root->id, t->header.level, t->size);

    // FIXME - doesn't capture everything, as it doesn't ascend
    le = t->paritem->list_entry.Flink;
    while (le != &t->parent->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

        if (!td->ignore) {
            nextparitem = td;
            break;
        }

        le = le->Flink;
    }

    if (!nextparitem)
        return STATUS_SUCCESS;

    TRACE("nextparitem: key = %llx,%x,%llx\n", nextparitem->key.obj_id, nextparitem->key.obj_type, nextparitem->key.offset);

    if (!nextparitem->treeholder.tree) {
        Status = do_load_tree(Vcb, &nextparitem->treeholder, t->root, t->parent, nextparitem, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("do_load_tree returned %08x\n", Status);
            return Status;
        }
    }

    if (!is_tree_unique(Vcb, nextparitem->treeholder.tree, Irp))
        return STATUS_SUCCESS;

    next_tree = nextparitem->treeholder.tree;

    if (!next_tree->updated_extents && next_tree->has_address) {
        Status = update_tree_extents(Vcb, next_tree, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return Status;
        }
    }

    if (t->size + next_tree->size <= Vcb->superblock.node_size - sizeof(tree_header)) {
        // merge two trees into one

        t->header.num_items += next_tree->header.num_items;
        t->size += next_tree->size;

        if (next_tree->header.level > 0) {
            le = next_tree->itemlist.Flink;

            while (le != &next_tree->itemlist) {
                tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);

                if (td2->treeholder.tree) {
                    td2->treeholder.tree->parent = t;
#ifdef DEBUG_PARANOID
                    if (td2->treeholder.tree->parent && td2->treeholder.tree->parent->header.level <= td2->treeholder.tree->header.level) int3;
#endif
                }

                td2->inserted = TRUE;
                le = le->Flink;
            }
        } else {
            le = next_tree->itemlist.Flink;

            while (le != &next_tree->itemlist) {
                tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);

                if (!td2->inserted && td2->data) {
                    UINT8* data = ExAllocatePoolWithTag(PagedPool, td2->size, ALLOC_TAG);

                    if (!data) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(data, td2->data, td2->size);
                    td2->data = data;
                    td2->inserted = TRUE;
                }

                le = le->Flink;
            }
        }

        t->itemlist.Blink->Flink = next_tree->itemlist.Flink;
        t->itemlist.Blink->Flink->Blink = t->itemlist.Blink;
        t->itemlist.Blink = next_tree->itemlist.Blink;
        t->itemlist.Blink->Flink = &t->itemlist;

        next_tree->itemlist.Flink = next_tree->itemlist.Blink = &next_tree->itemlist;

        next_tree->header.num_items = 0;
        next_tree->size = 0;

        if (next_tree->has_new_address) { // delete associated EXTENT_ITEM
            Status = reduce_tree_extent(Vcb, next_tree->new_address, next_tree, next_tree->parent->header.tree_id, next_tree->header.level, Irp, rollback);

            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        } else if (next_tree->has_address) {
            Status = reduce_tree_extent(Vcb, next_tree->header.address, next_tree, next_tree->parent->header.tree_id, next_tree->header.level, Irp, rollback);

            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        }

        if (!nextparitem->ignore) {
            nextparitem->ignore = TRUE;
            next_tree->parent->header.num_items--;
            next_tree->parent->size -= sizeof(internal_node);

            *done_deletions = TRUE;
        }

        par = next_tree->parent;
        while (par) {
            par->write = TRUE;
            par = par->parent;
        }

        RemoveEntryList(&nextparitem->list_entry);
        ExFreePool(next_tree->paritem);
        next_tree->paritem = NULL;

        next_tree->root->root_item.bytes_used -= Vcb->superblock.node_size;

        free_tree(next_tree);

        *done = TRUE;
    } else {
        // rebalance by moving items from second tree into first
        ULONG avg_size = (t->size + next_tree->size) / 2;
        KEY firstitem = {0, 0, 0};
        BOOL changed = FALSE;

        TRACE("attempting rebalance\n");

        le = next_tree->itemlist.Flink;
        while (le != &next_tree->itemlist && t->size < avg_size && next_tree->header.num_items > 1) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            ULONG size;

            if (!td->ignore) {
                if (next_tree->header.level == 0)
                    size = sizeof(leaf_node) + td->size;
                else
                    size = sizeof(internal_node);
            } else
                size = 0;

            if (t->size + size < Vcb->superblock.node_size - sizeof(tree_header)) {
                RemoveEntryList(&td->list_entry);
                InsertTailList(&t->itemlist, &td->list_entry);

                if (next_tree->header.level > 0 && td->treeholder.tree) {
                    td->treeholder.tree->parent = t;
#ifdef DEBUG_PARANOID
                    if (td->treeholder.tree->parent && td->treeholder.tree->parent->header.level <= td->treeholder.tree->header.level) int3;
#endif
                } else if (next_tree->header.level == 0 && !td->inserted && td->size > 0) {
                    UINT8* data = ExAllocatePoolWithTag(PagedPool, td->size, ALLOC_TAG);

                    if (!data) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(data, td->data, td->size);
                    td->data = data;
                }

                td->inserted = TRUE;

                if (!td->ignore) {
                    next_tree->size -= size;
                    t->size += size;
                    next_tree->header.num_items--;
                    t->header.num_items++;
                }

                changed = TRUE;
            } else
                break;

            le = next_tree->itemlist.Flink;
        }

        le = next_tree->itemlist.Flink;
        while (le != &next_tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

            if (!td->ignore) {
                firstitem = td->key;
                break;
            }

            le = le->Flink;
        }

        // FIXME - once ascension is working, make this work with parent's parent, etc.
        if (next_tree->paritem)
            next_tree->paritem->key = firstitem;

        par = next_tree;
        while (par) {
            par->write = TRUE;
            par = par->parent;
        }

        if (changed)
            *done = TRUE;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS update_extent_level(device_extension* Vcb, UINT64 address, tree* t, UINT8 level, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = t->header.level;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(tp.item->key, searchkey)) {
            EXTENT_ITEM_SKINNY_METADATA* eism;

            if (tp.item->size > 0) {
                eism = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);

                if (!eism) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(eism, tp.item->data, tp.item->size);
            } else
                eism = NULL;

            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                if (eism) ExFreePool(eism);
                return Status;
            }

            Status = insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, tp.item->size, NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08x\n", Status);
                if (eism) ExFreePool(eism);
                return Status;
            }

            return STATUS_SUCCESS;
        }
    }

    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        EXTENT_ITEM_TREE* eit;

        if (tp.item->size < sizeof(EXTENT_ITEM_TREE)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_TREE));
            return STATUS_INTERNAL_ERROR;
        }

        eit = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);

        if (!eit) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(eit, tp.item->data, tp.item->size);

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            ExFreePool(eit);
            return Status;
        }

        eit->level = level;

        Status = insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, eit, tp.item->size, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(eit);
            return Status;
        }

        return STATUS_SUCCESS;
    }

    ERR("could not find EXTENT_ITEM for address %llx\n", address);

    return STATUS_INTERNAL_ERROR;
}

static NTSTATUS update_tree_extents_recursive(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;

    if (t->parent && !t->parent->updated_extents && t->parent->has_address) {
        Status = update_tree_extents_recursive(Vcb, t->parent, Irp, rollback);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Status = update_tree_extents(Vcb, t, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_tree_extents returned %08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS do_splits(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    ULONG level, max_level;
    UINT32 min_size;
    BOOL empty, done_deletions = FALSE;
    NTSTATUS Status;
    tree* t;

    TRACE("(%p)\n", Vcb);

    max_level = 0;

    for (level = 0; level <= 255; level++) {
        LIST_ENTRY *le, *nextle;

        empty = TRUE;

        TRACE("doing level %u\n", level);

        le = Vcb->trees.Flink;

        while (le != &Vcb->trees) {
            t = CONTAINING_RECORD(le, tree, list_entry);

            nextle = le->Flink;

            if (t->write && t->header.level == level) {
                empty = FALSE;

                if (t->header.num_items == 0) {
                    if (t->parent) {
                        done_deletions = TRUE;

                        TRACE("deleting tree in root %llx\n", t->root->id);

                        t->root->root_item.bytes_used -= Vcb->superblock.node_size;

                        if (t->has_new_address) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, t->new_address, t, t->parent->header.tree_id, t->header.level, Irp, rollback);

                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }

                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, t->parent->header.tree_id, t->header.level, Irp, rollback);

                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }

                            t->has_address = FALSE;
                        }

                        if (!t->paritem->ignore) {
                            t->paritem->ignore = TRUE;
                            t->parent->header.num_items--;
                            t->parent->size -= sizeof(internal_node);
                        }

                        RemoveEntryList(&t->paritem->list_entry);
                        ExFreePool(t->paritem);
                        t->paritem = NULL;

                        free_tree(t);
                    } else if (t->header.level != 0) {
                        if (t->has_new_address) {
                            Status = update_extent_level(Vcb, t->new_address, t, 0, Irp);

                            if (!NT_SUCCESS(Status)) {
                                ERR("update_extent_level returned %08x\n", Status);
                                return Status;
                            }
                        }

                        t->header.level = 0;
                    }
                } else if (t->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                    TRACE("splitting overlarge tree (%x > %x)\n", t->size, Vcb->superblock.node_size - sizeof(tree_header));

                    if (!t->updated_extents && t->has_address) {
                        Status = update_tree_extents_recursive(Vcb, t, Irp, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("update_tree_extents_recursive returned %08x\n", Status);
                            return Status;
                        }
                    }

                    Status = split_tree(Vcb, t);

                    if (!NT_SUCCESS(Status)) {
                        ERR("split_tree returned %08x\n", Status);
                        return Status;
                    }
                }
            }

            le = nextle;
        }

        if (!empty) {
            max_level = level;
        } else {
            TRACE("nothing found for level %u\n", level);
            break;
        }
    }

    min_size = (Vcb->superblock.node_size - sizeof(tree_header)) / 2;

    for (level = 0; level <= max_level; level++) {
        LIST_ENTRY* le;

        le = Vcb->trees.Flink;

        while (le != &Vcb->trees) {
            t = CONTAINING_RECORD(le, tree, list_entry);

            if (t->write && t->header.level == level && t->header.num_items > 0 && t->parent && t->size < min_size &&
                t->root->id != BTRFS_ROOT_FREE_SPACE && is_tree_unique(Vcb, t, Irp)) {
                BOOL done;

                do {
                    Status = try_tree_amalgamate(Vcb, t, &done, &done_deletions, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("try_tree_amalgamate returned %08x\n", Status);
                        return Status;
                    }
                } while (done && t->size < min_size);
            }

            le = le->Flink;
        }
    }

    // simplify trees if top tree only has one entry

    if (done_deletions) {
        for (level = max_level; level > 0; level--) {
            LIST_ENTRY *le, *nextle;

            le = Vcb->trees.Flink;
            while (le != &Vcb->trees) {
                nextle = le->Flink;
                t = CONTAINING_RECORD(le, tree, list_entry);

                if (t->write && t->header.level == level) {
                    if (!t->parent && t->header.num_items == 1) {
                        LIST_ENTRY* le2 = t->itemlist.Flink;
                        tree_data* td = NULL;
                        tree* child_tree = NULL;

                        while (le2 != &t->itemlist) {
                            td = CONTAINING_RECORD(le2, tree_data, list_entry);
                            if (!td->ignore)
                                break;
                            le2 = le2->Flink;
                        }

                        TRACE("deleting top-level tree in root %llx with one item\n", t->root->id);

                        if (t->has_new_address) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, t->new_address, t, t->header.tree_id, t->header.level, Irp, rollback);

                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }

                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, t->header.tree_id, t->header.level, Irp, rollback);

                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }

                            t->has_address = FALSE;
                        }

                        if (!td->treeholder.tree) { // load first item if not already loaded
                            KEY searchkey = {0,0,0};
                            traverse_ptr tp;

                            Status = find_item(Vcb, t->root, &tp, &searchkey, FALSE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("error - find_item returned %08x\n", Status);
                                return Status;
                            }
                        }

                        child_tree = td->treeholder.tree;

                        if (child_tree) {
                            child_tree->parent = NULL;
                            child_tree->paritem = NULL;
                        }

                        t->root->root_item.bytes_used -= Vcb->superblock.node_size;

                        free_tree(t);

                        if (child_tree)
                            child_tree->root->treeholder.tree = child_tree;
                    }
                }

                le = nextle;
            }
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS remove_root_extents(device_extension* Vcb, root* r, tree_holder* th, UINT8 level, tree* parent, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;

    if (!th->tree) {
        UINT8* buf;
        chunk* c;

        buf = ExAllocatePoolWithTag(PagedPool, Vcb->superblock.node_size, ALLOC_TAG);
        if (!buf) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = read_data(Vcb, th->address, Vcb->superblock.node_size, NULL, TRUE, buf, NULL,
                           &c, Irp, th->generation, FALSE, NormalPagePriority);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data returned 0x%08x\n", Status);
            ExFreePool(buf);
            return Status;
        }

        Status = load_tree(Vcb, th->address, buf, r, &th->tree);

        if (!th->tree || th->tree->buf != buf)
            ExFreePool(buf);

        if (!NT_SUCCESS(Status)) {
            ERR("load_tree(%llx) returned %08x\n", th->address, Status);
            return Status;
        }
    }

    if (level > 0) {
        LIST_ENTRY* le = th->tree->itemlist.Flink;

        while (le != &th->tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

            if (!td->ignore) {
                Status = remove_root_extents(Vcb, r, &td->treeholder, th->tree->header.level - 1, th->tree, Irp, rollback);

                if (!NT_SUCCESS(Status)) {
                    ERR("remove_root_extents returned %08x\n", Status);
                    return Status;
                }
            }

            le = le->Flink;
        }
    }

    if (th->tree && !th->tree->updated_extents && th->tree->has_address) {
        Status = update_tree_extents(Vcb, th->tree, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_tree_extents returned %08x\n", Status);
            return Status;
        }
    }

    if (!th->tree || th->tree->has_address) {
        Status = reduce_tree_extent(Vcb, th->address, NULL, parent ? parent->header.tree_id : r->id, level, Irp, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("reduce_tree_extent(%llx) returned %08x\n", th->address, Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS drop_root(device_extension* Vcb, root* r, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;

    Status = remove_root_extents(Vcb, r, &r->treeholder, r->root_item.root_level, NULL, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("remove_root_extents returned %08x\n", Status);
        return Status;
    }

    // remove entries in uuid root (tree 9)
    if (Vcb->uuid_root) {
        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid.uuid[0], sizeof(UINT64));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(UINT64)], sizeof(UINT64));

        if (searchkey.obj_id != 0 || searchkey.offset != 0) {
            Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                WARN("find_item returned %08x\n", Status);
            } else {
                if (!keycmp(tp.item->key, searchkey)) {
                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }
                } else
                    WARN("could not find (%llx,%x,%llx) in uuid tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            }
        }

        if (r->root_item.rtransid > 0) {
            RtlCopyMemory(&searchkey.obj_id, &r->root_item.received_uuid.uuid[0], sizeof(UINT64));
            searchkey.obj_type = TYPE_SUBVOL_REC_UUID;
            RtlCopyMemory(&searchkey.offset, &r->root_item.received_uuid.uuid[sizeof(UINT64)], sizeof(UINT64));

            Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status))
                WARN("find_item returned %08x\n", Status);
            else {
                if (!keycmp(tp.item->key, searchkey)) {
                    if (tp.item->size == sizeof(UINT64)) {
                        UINT64* id = (UINT64*)tp.item->data;

                        if (*id == r->id) {
                            Status = delete_tree_item(Vcb, &tp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("delete_tree_item returned %08x\n", Status);
                                return Status;
                            }
                        }
                    } else if (tp.item->size > sizeof(UINT64)) {
                        ULONG i;
                        UINT64* ids = (UINT64*)tp.item->data;

                        for (i = 0; i < tp.item->size / sizeof(UINT64); i++) {
                            if (ids[i] == r->id) {
                                UINT64* ne;

                                ne = ExAllocatePoolWithTag(PagedPool, tp.item->size - sizeof(UINT64), ALLOC_TAG);
                                if (!ne) {
                                    ERR("out of memory\n");
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                if (i > 0)
                                    RtlCopyMemory(ne, ids, sizeof(UINT64) * i);

                                if ((i + 1) * sizeof(UINT64) < tp.item->size)
                                    RtlCopyMemory(&ne[i], &ids[i + 1], tp.item->size - ((i + 1) * sizeof(UINT64)));

                                Status = delete_tree_item(Vcb, &tp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("delete_tree_item returned %08x\n", Status);
                                    ExFreePool(ne);
                                    return Status;
                                }

                                Status = insert_tree_item(Vcb, Vcb->uuid_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                                                          ne, tp.item->size - sizeof(UINT64), NULL, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("insert_tree_item returned %08x\n", Status);
                                    ExFreePool(ne);
                                    return Status;
                                }

                                break;
                            }
                        }
                    }
                } else
                    WARN("could not find (%llx,%x,%llx) in uuid tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            }
        }
    }

    // delete ROOT_ITEM

    searchkey.obj_id = r->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        Status = delete_tree_item(Vcb, &tp);

        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            return Status;
        }
    } else
        WARN("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);

    // delete items in tree cache

    free_trees_root(Vcb, r);

    return STATUS_SUCCESS;
}

static NTSTATUS drop_roots(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->drop_roots.Flink, *le2;
    NTSTATUS Status;

    while (le != &Vcb->drop_roots) {
        root* r = CONTAINING_RECORD(le, root, list_entry);

        le2 = le->Flink;

        Status = drop_root(Vcb, r, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("drop_root(%llx) returned %08x\n", r->id, Status);
            return Status;
        }

        le = le2;
    }

    return STATUS_SUCCESS;
}

NTSTATUS update_dev_item(device_extension* Vcb, device* device, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    DEV_ITEM* di;
    NTSTATUS Status;

    searchkey.obj_id = 1;
    searchkey.obj_type = TYPE_DEV_ITEM;
    searchkey.offset = device->devitem.dev_id;

    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        ERR("error - could not find DEV_ITEM for device %llx\n", device->devitem.dev_id);
        return STATUS_INTERNAL_ERROR;
    }

    Status = delete_tree_item(Vcb, &tp);
    if (!NT_SUCCESS(Status)) {
        ERR("delete_tree_item returned %08x\n", Status);
        return Status;
    }

    di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(di, &device->devitem, sizeof(DEV_ITEM));

    Status = insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, device->devitem.dev_id, di, sizeof(DEV_ITEM), NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(di);
        return Status;
    }

    return STATUS_SUCCESS;
}

static void regen_bootstrap(device_extension* Vcb) {
    sys_chunk* sc2;
    USHORT i = 0;
    LIST_ENTRY* le;

    i = 0;
    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);

        TRACE("%llx,%x,%llx\n", sc2->key.obj_id, sc2->key.obj_type, sc2->key.offset);

        RtlCopyMemory(&Vcb->superblock.sys_chunk_array[i], &sc2->key, sizeof(KEY));
        i += sizeof(KEY);

        RtlCopyMemory(&Vcb->superblock.sys_chunk_array[i], sc2->data, sc2->size);
        i += sc2->size;

        le = le->Flink;
    }
}

static NTSTATUS add_to_bootstrap(device_extension* Vcb, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, UINT16 size) {
    sys_chunk* sc;
    LIST_ENTRY* le;

    if (Vcb->superblock.n + sizeof(KEY) + size > SYS_CHUNK_ARRAY_SIZE) {
        ERR("error - bootstrap is full\n");
        return STATUS_INTERNAL_ERROR;
    }

    sc = ExAllocatePoolWithTag(PagedPool, sizeof(sys_chunk), ALLOC_TAG);
    if (!sc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    sc->key.obj_id = obj_id;
    sc->key.obj_type = obj_type;
    sc->key.offset = offset;
    sc->size = size;
    sc->data = ExAllocatePoolWithTag(PagedPool, sc->size, ALLOC_TAG);
    if (!sc->data) {
        ERR("out of memory\n");
        ExFreePool(sc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(sc->data, data, sc->size);

    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sys_chunk* sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);

        if (keycmp(sc2->key, sc->key) == 1)
            break;

        le = le->Flink;
    }
    InsertTailList(le, &sc->list_entry);

    Vcb->superblock.n += sizeof(KEY) + size;

    regen_bootstrap(Vcb);

    return STATUS_SUCCESS;
}

static NTSTATUS create_chunk(device_extension* Vcb, chunk* c, PIRP Irp) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    BLOCK_GROUP_ITEM* bgi;
    UINT16 i, factor;
    NTSTATUS Status;

    ci = ExAllocatePoolWithTag(PagedPool, c->size, ALLOC_TAG);
    if (!ci) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(ci, c->chunk_item, c->size);

    Status = insert_tree_item(Vcb, Vcb->chunk_root, 0x100, TYPE_CHUNK_ITEM, c->offset, ci, c->size, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ci);
        return Status;
    }

    if (c->chunk_item->type & BLOCK_FLAG_SYSTEM) {
        Status = add_to_bootstrap(Vcb, 0x100, TYPE_CHUNK_ITEM, c->offset, ci, c->size);
        if (!NT_SUCCESS(Status)) {
            ERR("add_to_bootstrap returned %08x\n", Status);
            return Status;
        }
    }

    // add BLOCK_GROUP_ITEM to tree 2

    bgi = ExAllocatePoolWithTag(PagedPool, sizeof(BLOCK_GROUP_ITEM), ALLOC_TAG);
    if (!bgi) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    bgi->used = c->used;
    bgi->chunk_tree = 0x100;
    bgi->flags = c->chunk_item->type;

    Status = insert_tree_item(Vcb, Vcb->extent_root, c->offset, TYPE_BLOCK_GROUP_ITEM, c->chunk_item->size, bgi, sizeof(BLOCK_GROUP_ITEM), NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(bgi);
        return Status;
    }

    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
        factor = c->chunk_item->num_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
        factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
        factor = c->chunk_item->num_stripes - 1;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
        factor = c->chunk_item->num_stripes - 2;
    else // SINGLE, DUPLICATE, RAID1
        factor = 1;

    // add DEV_EXTENTs to tree 4

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        DEV_EXTENT* de;

        de = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_EXTENT), ALLOC_TAG);
        if (!de) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        de->chunktree = Vcb->chunk_root->id;
        de->objid = 0x100;
        de->address = c->offset;
        de->length = c->chunk_item->size / factor;
        de->chunktree_uuid = Vcb->chunk_root->treeholder.tree->header.chunk_tree_uuid;

        Status = insert_tree_item(Vcb, Vcb->dev_root, c->devices[i]->devitem.dev_id, TYPE_DEV_EXTENT, cis[i].offset, de, sizeof(DEV_EXTENT), NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(de);
            return Status;
        }

        // FIXME - no point in calling this twice for the same device
        Status = update_dev_item(Vcb, c->devices[i], Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("update_dev_item returned %08x\n", Status);
            return Status;
        }
    }

    c->created = FALSE;
    c->oldused = c->used;

    Vcb->superblock.bytes_used += chunk_estimate_phys_size(Vcb, c, c->used);

    return STATUS_SUCCESS;
}

static void remove_from_bootstrap(device_extension* Vcb, UINT64 obj_id, UINT8 obj_type, UINT64 offset) {
    sys_chunk* sc2;
    LIST_ENTRY* le;

    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);

        if (sc2->key.obj_id == obj_id && sc2->key.obj_type == obj_type && sc2->key.offset == offset) {
            RemoveEntryList(&sc2->list_entry);

            Vcb->superblock.n -= sizeof(KEY) + sc2->size;

            ExFreePool(sc2->data);
            ExFreePool(sc2);
            regen_bootstrap(Vcb);
            return;
        }

        le = le->Flink;
    }
}

static NTSTATUS set_xattr(device_extension* Vcb, LIST_ENTRY* batchlist, root* subvol, UINT64 inode, char* name, UINT16 namelen,
                          UINT32 crc32, UINT8* data, UINT16 datalen) {
    NTSTATUS Status;
    UINT16 xasize;
    DIR_ITEM* xa;

    TRACE("(%p, %llx, %llx, %.*s, %08x, %p, %u)\n", Vcb, subvol->id, inode, namelen, name, crc32, data, datalen);

    xasize = (UINT16)offsetof(DIR_ITEM, name[0]) + namelen + datalen;

    xa = ExAllocatePoolWithTag(PagedPool, xasize, ALLOC_TAG);
    if (!xa) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    xa->key.obj_id = 0;
    xa->key.obj_type = 0;
    xa->key.offset = 0;
    xa->transid = Vcb->superblock.generation;
    xa->m = datalen;
    xa->n = namelen;
    xa->type = BTRFS_TYPE_EA;
    RtlCopyMemory(xa->name, name, namelen);
    RtlCopyMemory(xa->name + namelen, data, datalen);

    Status = insert_tree_item_batch(batchlist, Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, xa, xasize, Batch_SetXattr);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item_batch returned %08x\n", Status);
        ExFreePool(xa);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS delete_xattr(device_extension* Vcb, LIST_ENTRY* batchlist, root* subvol, UINT64 inode, char* name,
                             UINT16 namelen, UINT32 crc32) {
    NTSTATUS Status;
    UINT16 xasize;
    DIR_ITEM* xa;

    TRACE("(%p, %llx, %llx, %.*s, %08x)\n", Vcb, subvol->id, inode, namelen, name, crc32);

    xasize = (UINT16)offsetof(DIR_ITEM, name[0]) + namelen;

    xa = ExAllocatePoolWithTag(PagedPool, xasize, ALLOC_TAG);
    if (!xa) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    xa->key.obj_id = 0;
    xa->key.obj_type = 0;
    xa->key.offset = 0;
    xa->transid = Vcb->superblock.generation;
    xa->m = 0;
    xa->n = namelen;
    xa->type = BTRFS_TYPE_EA;
    RtlCopyMemory(xa->name, name, namelen);

    Status = insert_tree_item_batch(batchlist, Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, xa, xasize, Batch_DeleteXattr);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item_batch returned %08x\n", Status);
        ExFreePool(xa);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS insert_sparse_extent(fcb* fcb, LIST_ENTRY* batchlist, UINT64 start, UINT64 length) {
    NTSTATUS Status;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;

    TRACE("((%llx, %llx), %llx, %llx)\n", fcb->subvol->id, fcb->inode, start, length);

    ed = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ed->generation = fcb->Vcb->superblock.generation;
    ed->decoded_size = length;
    ed->compression = BTRFS_COMPRESSION_NONE;
    ed->encryption = BTRFS_ENCRYPTION_NONE;
    ed->encoding = BTRFS_ENCODING_NONE;
    ed->type = EXTENT_TYPE_REGULAR;

    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = 0;
    ed2->size = 0;
    ed2->offset = 0;
    ed2->num_bytes = length;

    Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, start, ed, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), Batch_Insert);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item_batch returned %08x\n", Status);
        ExFreePool(ed);
        return Status;
    }

    return STATUS_SUCCESS;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(suppress: 28194)
#endif
NTSTATUS insert_tree_item_batch(LIST_ENTRY* batchlist, device_extension* Vcb, root* r, UINT64 objid, UINT8 objtype, UINT64 offset,
                                _In_opt_ _When_(return >= 0, __drv_aliasesMem) void* data, UINT16 datalen, enum batch_operation operation) {
    LIST_ENTRY* le;
    batch_root* br = NULL;
    batch_item* bi;

    le = batchlist->Flink;
    while (le != batchlist) {
        batch_root* br2 = CONTAINING_RECORD(le, batch_root, list_entry);

        if (br2->r == r) {
            br = br2;
            break;
        }

        le = le->Flink;
    }

    if (!br) {
        br = ExAllocatePoolWithTag(PagedPool, sizeof(batch_root), ALLOC_TAG);
        if (!br) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        br->r = r;
        InitializeListHead(&br->items);
        InsertTailList(batchlist, &br->list_entry);
    }

    bi = ExAllocateFromPagedLookasideList(&Vcb->batch_item_lookaside);
    if (!bi) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    bi->key.obj_id = objid;
    bi->key.obj_type = objtype;
    bi->key.offset = offset;
    bi->data = data;
    bi->datalen = datalen;
    bi->operation = operation;

    le = br->items.Blink;
    while (le != &br->items) {
        batch_item* bi2 = CONTAINING_RECORD(le, batch_item, list_entry);
        int cmp = keycmp(bi2->key, bi->key);

        if (cmp == -1 || (cmp == 0 && bi->operation >= bi2->operation)) {
            InsertHeadList(&bi2->list_entry, &bi->list_entry);
            return STATUS_SUCCESS;
        }

        le = le->Blink;
    }

    InsertHeadList(&br->items, &bi->list_entry);

    return STATUS_SUCCESS;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

typedef struct {
    UINT64 address;
    UINT64 length;
    UINT64 offset;
    BOOL changed;
    chunk* chunk;
    UINT64 skip_start;
    UINT64 skip_end;
    LIST_ENTRY list_entry;
} extent_range;

static void rationalize_extents(fcb* fcb, PIRP Irp) {
    LIST_ENTRY* le;
    LIST_ENTRY extent_ranges;
    extent_range* er;
    BOOL changed = FALSE, truncating = FALSE;
    UINT32 num_extents = 0;

    InitializeListHead(&extent_ranges);

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if ((ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) && ext->extent_data.compression == BTRFS_COMPRESSION_NONE && ext->unique) {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

            if (ed2->size != 0) {
                LIST_ENTRY* le2;

                le2 = extent_ranges.Flink;
                while (le2 != &extent_ranges) {
                    extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);

                    if (er2->address == ed2->address) {
                        er2->skip_start = min(er2->skip_start, ed2->offset);
                        er2->skip_end = min(er2->skip_end, ed2->size - ed2->offset - ed2->num_bytes);
                        goto cont;
                    } else if (er2->address > ed2->address)
                        break;

                    le2 = le2->Flink;
                }

                er = ExAllocatePoolWithTag(PagedPool, sizeof(extent_range), ALLOC_TAG); // FIXME - should be from lookaside?
                if (!er) {
                    ERR("out of memory\n");
                    goto end;
                }

                er->address = ed2->address;
                er->length = ed2->size;
                er->offset = ext->offset - ed2->offset;
                er->changed = FALSE;
                er->chunk = NULL;
                er->skip_start = ed2->offset;
                er->skip_end = ed2->size - ed2->offset - ed2->num_bytes;

                if (er->skip_start != 0 || er->skip_end != 0)
                    truncating = TRUE;

                InsertHeadList(le2->Blink, &er->list_entry);
                num_extents++;
            }
        }

cont:
        le = le->Flink;
    }

    if (num_extents == 0 || (num_extents == 1 && !truncating))
        goto end;

    le = extent_ranges.Flink;
    while (le != &extent_ranges) {
        er = CONTAINING_RECORD(le, extent_range, list_entry);

        if (!er->chunk) {
            LIST_ENTRY* le2;

            er->chunk = get_chunk_from_address(fcb->Vcb, er->address);

            if (!er->chunk) {
                ERR("get_chunk_from_address(%llx) failed\n", er->address);
                goto end;
            }

            le2 = le->Flink;
            while (le2 != &extent_ranges) {
                extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);

                if (!er2->chunk && er2->address >= er->chunk->offset && er2->address < er->chunk->offset + er->chunk->chunk_item->size)
                    er2->chunk = er->chunk;

                le2 = le2->Flink;
            }
        }

        le = le->Flink;
    }

    if (truncating) {
        // truncate beginning or end of extent if unused

        le = extent_ranges.Flink;
        while (le != &extent_ranges) {
            er = CONTAINING_RECORD(le, extent_range, list_entry);

            if (er->skip_start > 0) {
                LIST_ENTRY* le2 = fcb->extents.Flink;
                while (le2 != &fcb->extents) {
                    extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                    if ((ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) && ext->extent_data.compression == BTRFS_COMPRESSION_NONE && ext->unique) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                        if (ed2->size != 0 && ed2->address == er->address) {
                            NTSTATUS Status;

                            Status = update_changed_extent_ref(fcb->Vcb, er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                               -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                                goto end;
                            }

                            ext->extent_data.decoded_size -= er->skip_start;
                            ed2->size -= er->skip_start;
                            ed2->address += er->skip_start;
                            ed2->offset -= er->skip_start;

                            add_changed_extent_ref(er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                   1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
                        }
                    }

                    le2 = le2->Flink;
                }

                if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM))
                    add_checksum_entry(fcb->Vcb, er->address, (ULONG)(er->skip_start / fcb->Vcb->superblock.sector_size), NULL, NULL);

                acquire_chunk_lock(er->chunk, fcb->Vcb);

                if (!er->chunk->cache_loaded) {
                    NTSTATUS Status = load_cache_chunk(fcb->Vcb, er->chunk, NULL);

                    if (!NT_SUCCESS(Status)) {
                        ERR("load_cache_chunk returned %08x\n", Status);
                        release_chunk_lock(er->chunk, fcb->Vcb);
                        goto end;
                    }
                }

                er->chunk->used -= er->skip_start;

                space_list_add(er->chunk, er->address, er->skip_start, NULL);

                release_chunk_lock(er->chunk, fcb->Vcb);

                er->address += er->skip_start;
                er->length -= er->skip_start;
            }

            if (er->skip_end > 0) {
                LIST_ENTRY* le2 = fcb->extents.Flink;
                while (le2 != &fcb->extents) {
                    extent* ext = CONTAINING_RECORD(le2, extent, list_entry);

                    if ((ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) && ext->extent_data.compression == BTRFS_COMPRESSION_NONE && ext->unique) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                        if (ed2->size != 0 && ed2->address == er->address) {
                            NTSTATUS Status;

                            Status = update_changed_extent_ref(fcb->Vcb, er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                               -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                                goto end;
                            }

                            ext->extent_data.decoded_size -= er->skip_end;
                            ed2->size -= er->skip_end;

                            add_changed_extent_ref(er->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                   1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
                        }
                    }

                    le2 = le2->Flink;
                }

                if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM))
                    add_checksum_entry(fcb->Vcb, er->address + er->length - er->skip_end, (ULONG)(er->skip_end / fcb->Vcb->superblock.sector_size), NULL, NULL);

                acquire_chunk_lock(er->chunk, fcb->Vcb);

                if (!er->chunk->cache_loaded) {
                    NTSTATUS Status = load_cache_chunk(fcb->Vcb, er->chunk, NULL);

                    if (!NT_SUCCESS(Status)) {
                        ERR("load_cache_chunk returned %08x\n", Status);
                        release_chunk_lock(er->chunk, fcb->Vcb);
                        goto end;
                    }
                }

                er->chunk->used -= er->skip_end;

                space_list_add(er->chunk, er->address + er->length - er->skip_end, er->skip_end, NULL);

                release_chunk_lock(er->chunk, fcb->Vcb);

                er->length -= er->skip_end;
            }

            le = le->Flink;
        }
    }

    if (num_extents < 2)
        goto end;

    // merge together adjacent extents
    le = extent_ranges.Flink;
    while (le != &extent_ranges) {
        er = CONTAINING_RECORD(le, extent_range, list_entry);

        if (le->Flink != &extent_ranges && er->length < MAX_EXTENT_SIZE) {
            extent_range* er2 = CONTAINING_RECORD(le->Flink, extent_range, list_entry);

            if (er->chunk == er2->chunk) {
                if (er2->address == er->address + er->length && er2->offset >= er->offset + er->length) {
                    if (er->length + er2->length <= MAX_EXTENT_SIZE) {
                        er->length += er2->length;
                        er->changed = TRUE;

                        RemoveEntryList(&er2->list_entry);
                        ExFreePool(er2);

                        changed = TRUE;
                        continue;
                    }
                }
            }
        }

        le = le->Flink;
    }

    if (!changed)
        goto end;

    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);

        if ((ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) && ext->extent_data.compression == BTRFS_COMPRESSION_NONE && ext->unique) {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

            if (ed2->size != 0) {
                LIST_ENTRY* le2;

                le2 = extent_ranges.Flink;
                while (le2 != &extent_ranges) {
                    extent_range* er2 = CONTAINING_RECORD(le2, extent_range, list_entry);

                    if (ed2->address >= er2->address && ed2->address + ed2->size <= er2->address + er2->length && er2->changed) {
                        NTSTATUS Status;

                        Status = update_changed_extent_ref(fcb->Vcb, er2->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                                           -1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, TRUE, Irp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("update_changed_extent_ref returned %08x\n", Status);
                            goto end;
                        }

                        ed2->offset += ed2->address - er2->address;
                        ed2->address = er2->address;
                        ed2->size = er2->length;
                        ext->extent_data.decoded_size = ed2->size;

                        add_changed_extent_ref(er2->chunk, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset,
                                               1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);

                        break;
                    }

                    le2 = le2->Flink;
                }
            }
        }

        le = le->Flink;
    }

end:
    while (!IsListEmpty(&extent_ranges)) {
        le = RemoveHeadList(&extent_ranges);
        er = CONTAINING_RECORD(le, extent_range, list_entry);

        ExFreePool(er);
    }
}

NTSTATUS flush_fcb(fcb* fcb, BOOL cache, LIST_ENTRY* batchlist, PIRP Irp) {
    traverse_ptr tp;
    KEY searchkey;
    NTSTATUS Status;
    INODE_ITEM* ii;
    UINT64 ii_offset;
#ifdef DEBUG_PARANOID
    UINT64 old_size = 0;
    BOOL extents_changed;
#endif

    if (fcb->ads) {
        if (fcb->deleted) {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adsxattr.Length, fcb->adshash);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        } else {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adsxattr.Length,
                               fcb->adshash, (UINT8*)fcb->adsdata.Buffer, fcb->adsdata.Length);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        }

        if (fcb->marked_as_orphan) {
            Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, BTRFS_ORPHAN_INODE_OBJID, TYPE_ORPHAN_INODE,
                                            fcb->inode, NULL, 0, Batch_Delete);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                goto end;
            }
        }

        Status = STATUS_SUCCESS;
        goto end;
    }

    if (fcb->deleted) {
        Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0xffffffffffffffff, NULL, 0, Batch_DeleteInode);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            goto end;
        }

        Status = STATUS_SUCCESS;
        goto end;
    }

#ifdef DEBUG_PARANOID
    extents_changed = fcb->extents_changed;
#endif

    if (fcb->extents_changed) {
        LIST_ENTRY* le;
        BOOL prealloc = FALSE, extents_inline = FALSE;
        UINT64 last_end;

        // delete ignored extent items
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            LIST_ENTRY* le2 = le->Flink;
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (ext->ignore) {
                RemoveEntryList(&ext->list_entry);

                if (ext->csum)
                    ExFreePool(ext->csum);

                ExFreePool(ext);
            }

            le = le2;
        }

        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);

            if (ext->inserted && ext->csum && ext->extent_data.type == EXTENT_TYPE_REGULAR) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;

                if (ed2->size > 0) { // not sparse
                    if (ext->extent_data.compression == BTRFS_COMPRESSION_NONE)
                        add_checksum_entry(fcb->Vcb, ed2->address + ed2->offset, (ULONG)(ed2->num_bytes / fcb->Vcb->superblock.sector_size), ext->csum, Irp);
                    else
                        add_checksum_entry(fcb->Vcb, ed2->address, (ULONG)(ed2->size / fcb->Vcb->superblock.sector_size), ext->csum, Irp);
                }
            }

            le = le->Flink;
        }

        if (!IsListEmpty(&fcb->extents)) {
            rationalize_extents(fcb, Irp);

            // merge together adjacent EXTENT_DATAs pointing to same extent

            le = fcb->extents.Flink;
            while (le != &fcb->extents) {
                LIST_ENTRY* le2 = le->Flink;
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);

                if ((ext->extent_data.type == EXTENT_TYPE_REGULAR || ext->extent_data.type == EXTENT_TYPE_PREALLOC) && le->Flink != &fcb->extents) {
                    extent* nextext = CONTAINING_RECORD(le->Flink, extent, list_entry);

                    if (ext->extent_data.type == nextext->extent_data.type) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->extent_data.data;
                        EXTENT_DATA2* ned2 = (EXTENT_DATA2*)nextext->extent_data.data;

                        if (ed2->size != 0 && ed2->address == ned2->address && ed2->size == ned2->size &&
                            nextext->offset == ext->offset + ed2->num_bytes && ned2->offset == ed2->offset + ed2->num_bytes) {
                            chunk* c;

                            if (ext->extent_data.compression == BTRFS_COMPRESSION_NONE && ext->csum) {
                                ULONG len = (ULONG)((ed2->num_bytes + ned2->num_bytes) / fcb->Vcb->superblock.sector_size);
                                UINT32* csum;

                                csum = ExAllocatePoolWithTag(NonPagedPool, len * sizeof(UINT32), ALLOC_TAG);
                                if (!csum) {
                                    ERR("out of memory\n");
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    goto end;
                                }

                                RtlCopyMemory(csum, ext->csum, (ULONG)(ed2->num_bytes * sizeof(UINT32) / fcb->Vcb->superblock.sector_size));
                                RtlCopyMemory(&csum[ed2->num_bytes / fcb->Vcb->superblock.sector_size], nextext->csum,
                                              (ULONG)(ned2->num_bytes * sizeof(UINT32) / fcb->Vcb->superblock.sector_size));

                                ExFreePool(ext->csum);
                                ext->csum = csum;
                            }

                            ext->extent_data.generation = fcb->Vcb->superblock.generation;
                            ed2->num_bytes += ned2->num_bytes;

                            RemoveEntryList(&nextext->list_entry);

                            if (nextext->csum)
                                ExFreePool(nextext->csum);

                            ExFreePool(nextext);

                            c = get_chunk_from_address(fcb->Vcb, ed2->address);

                            if (!c) {
                                ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, -1,
                                                                fcb->inode_item.flags & BTRFS_INODE_NODATASUM, FALSE, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("update_changed_extent_ref returned %08x\n", Status);
                                    goto end;
                                }
                            }

                            le2 = le;
                        }
                    }
                }

                le = le2;
            }
        }

        if (!fcb->created) {
            // delete existing EXTENT_DATA items

            Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, 0, NULL, 0, Batch_DeleteExtentData);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                goto end;
            }
        }

        // add new EXTENT_DATAs

        last_end = 0;

        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            EXTENT_DATA* ed;

            ext->inserted = FALSE;

            if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES) && ext->offset > last_end) {
                Status = insert_sparse_extent(fcb, batchlist, last_end, ext->offset - last_end);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_sparse_extent returned %08x\n", Status);
                    goto end;
                }
            }

            ed = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
            if (!ed) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlCopyMemory(ed, &ext->extent_data, ext->datalen);

            Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, ext->offset,
                                            ed, ext->datalen, Batch_Insert);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                goto end;
            }

            if (ed->type == EXTENT_TYPE_PREALLOC)
                prealloc = TRUE;

            if (ed->type == EXTENT_TYPE_INLINE)
                extents_inline = TRUE;

            if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES)) {
                if (ed->type == EXTENT_TYPE_INLINE)
                    last_end = ext->offset + ed->decoded_size;
                else {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                    last_end = ext->offset + ed2->num_bytes;
                }
            }

            le = le->Flink;
        }

        if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES) && !extents_inline &&
            sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) > last_end) {
            Status = insert_sparse_extent(fcb, batchlist, last_end, sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) - last_end);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_sparse_extent returned %08x\n", Status);
                goto end;
            }
        }

        // update prealloc flag in INODE_ITEM

        if (!prealloc)
            fcb->inode_item.flags &= ~BTRFS_INODE_PREALLOC;
        else
            fcb->inode_item.flags |= BTRFS_INODE_PREALLOC;

        fcb->inode_item_changed = TRUE;

        fcb->extents_changed = FALSE;
    }

    if ((!fcb->created && fcb->inode_item_changed) || cache) {
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            if (cache) {
                ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
                if (!ii) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));

                Status = insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    goto end;
                }

                ii_offset = 0;
            } else {
                ERR("could not find INODE_ITEM for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
        } else {
#ifdef DEBUG_PARANOID
            INODE_ITEM* ii2 = (INODE_ITEM*)tp.item->data;

            old_size = ii2->st_size;
#endif

            ii_offset = tp.item->key.offset;
        }

        if (!cache) {
            Status = delete_tree_item(fcb->Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                goto end;
            }
        } else {
            searchkey.obj_id = fcb->inode;
            searchkey.obj_type = TYPE_INODE_ITEM;
            searchkey.offset = ii_offset;

            Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }

            if (keycmp(tp.item->key, searchkey)) {
                ERR("could not find INODE_ITEM for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            } else
                RtlCopyMemory(tp.item->data, &fcb->inode_item, min(tp.item->size, sizeof(INODE_ITEM)));
        }

#ifdef DEBUG_PARANOID
        if (!extents_changed && fcb->type != BTRFS_TYPE_DIRECTORY && old_size != fcb->inode_item.st_size) {
            ERR("error - size has changed but extents not marked as changed\n");
            int3;
        }
#endif
    } else
        ii_offset = 0;

    fcb->created = FALSE;

    if (!cache && fcb->inode_item_changed) {
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));

        Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, ii_offset, ii, sizeof(INODE_ITEM),
                                        Batch_Insert);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            goto end;
        }

        fcb->inode_item_changed = FALSE;
    }

    if (fcb->sd_dirty) {
        if (!fcb->sd_deleted) {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_NTACL, sizeof(EA_NTACL) - 1,
                               EA_NTACL_HASH, (UINT8*)fcb->sd, (UINT16)RtlLengthSecurityDescriptor(fcb->sd));
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_NTACL, sizeof(EA_NTACL) - 1, EA_NTACL_HASH);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        }

        fcb->sd_deleted = FALSE;
        fcb->sd_dirty = FALSE;
    }

    if (fcb->atts_changed) {
        if (!fcb->atts_deleted) {
            UINT8 val[16], *val2;
            ULONG atts = fcb->atts;

            TRACE("inserting new DOSATTRIB xattr\n");

            if (fcb->inode == SUBVOL_ROOT_INODE)
                atts &= ~FILE_ATTRIBUTE_READONLY;

            val2 = &val[sizeof(val) - 1];

            do {
                UINT8 c = atts % 16;
                *val2 = c <= 9 ? (c + '0') : (c - 0xa + 'a');

                val2--;
                atts >>= 4;
            } while (atts != 0);

            *val2 = 'x';
            val2--;
            *val2 = '0';

            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_DOSATTRIB, sizeof(EA_DOSATTRIB) - 1,
                               EA_DOSATTRIB_HASH, val2, (UINT16)(val + sizeof(val) - val2));
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_DOSATTRIB, sizeof(EA_DOSATTRIB) - 1, EA_DOSATTRIB_HASH);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        }

        fcb->atts_changed = FALSE;
        fcb->atts_deleted = FALSE;
    }

    if (fcb->reparse_xattr_changed) {
        if (fcb->reparse_xattr.Buffer && fcb->reparse_xattr.Length > 0) {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_REPARSE, sizeof(EA_REPARSE) - 1,
                               EA_REPARSE_HASH, (UINT8*)fcb->reparse_xattr.Buffer, (UINT16)fcb->reparse_xattr.Length);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_REPARSE, sizeof(EA_REPARSE) - 1, EA_REPARSE_HASH);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        }

        fcb->reparse_xattr_changed = FALSE;
    }

    if (fcb->ea_changed) {
        if (fcb->ea_xattr.Buffer && fcb->ea_xattr.Length > 0) {
            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_EA, sizeof(EA_EA) - 1,
                               EA_EA_HASH, (UINT8*)fcb->ea_xattr.Buffer, (UINT16)fcb->ea_xattr.Length);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_EA, sizeof(EA_EA) - 1, EA_EA_HASH);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        }

        fcb->ea_changed = FALSE;
    }

    if (fcb->prop_compression_changed) {
        if (fcb->prop_compression == PropCompression_None) {
            Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_PROP_COMPRESSION, sizeof(EA_PROP_COMPRESSION) - 1, EA_PROP_COMPRESSION_HASH);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_xattr returned %08x\n", Status);
                goto end;
            }
        } else if (fcb->prop_compression == PropCompression_Zlib) {
            static const char zlib[] = "zlib";

            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_PROP_COMPRESSION, sizeof(EA_PROP_COMPRESSION) - 1,
                               EA_PROP_COMPRESSION_HASH, (UINT8*)zlib, sizeof(zlib) - 1);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else if (fcb->prop_compression == PropCompression_LZO) {
            static const char lzo[] = "lzo";

            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_PROP_COMPRESSION, sizeof(EA_PROP_COMPRESSION) - 1,
                               EA_PROP_COMPRESSION_HASH, (UINT8*)lzo, sizeof(lzo) - 1);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else if (fcb->prop_compression == PropCompression_ZSTD) {
            static const char zstd[] = "zstd";

            Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_PROP_COMPRESSION, sizeof(EA_PROP_COMPRESSION) - 1,
                               EA_PROP_COMPRESSION_HASH, (UINT8*)zstd, sizeof(zstd) - 1);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        }

        fcb->prop_compression_changed = FALSE;
    }

    if (fcb->xattrs_changed) {
        LIST_ENTRY* le;

        le = fcb->xattrs.Flink;
        while (le != &fcb->xattrs) {
            xattr* xa = CONTAINING_RECORD(le, xattr, list_entry);
            LIST_ENTRY* le2 = le->Flink;

            if (xa->dirty) {
                UINT32 hash = calc_crc32c(0xfffffffe, (UINT8*)xa->data, xa->namelen);

                if (xa->valuelen == 0) {
                    Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, xa->data, xa->namelen, hash);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_xattr returned %08x\n", Status);
                        goto end;
                    }

                    RemoveEntryList(&xa->list_entry);
                    ExFreePool(xa);
                } else {
                    Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, xa->data, xa->namelen,
                                       hash, (UINT8*)&xa->data[xa->namelen], xa->valuelen);
                    if (!NT_SUCCESS(Status)) {
                        ERR("set_xattr returned %08x\n", Status);
                        goto end;
                    }

                    xa->dirty = FALSE;
                }
            }

            le = le2;
        }

        fcb->xattrs_changed = FALSE;
    }

    if ((fcb->case_sensitive_set && !fcb->case_sensitive)) {
        Status = delete_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_CASE_SENSITIVE,
                              sizeof(EA_CASE_SENSITIVE) - 1, EA_CASE_SENSITIVE_HASH);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_xattr returned %08x\n", Status);
            goto end;
        }

        fcb->case_sensitive_set = FALSE;
    } else if ((!fcb->case_sensitive_set && fcb->case_sensitive)) {
        Status = set_xattr(fcb->Vcb, batchlist, fcb->subvol, fcb->inode, EA_CASE_SENSITIVE,
                           sizeof(EA_CASE_SENSITIVE) - 1, EA_CASE_SENSITIVE_HASH, (UINT8*)"1", 1);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            goto end;
        }

        fcb->case_sensitive_set = TRUE;
    }

    if (fcb->inode_item.st_nlink == 0 && !fcb->marked_as_orphan) { // mark as orphan
        Status = insert_tree_item_batch(batchlist, fcb->Vcb, fcb->subvol, BTRFS_ORPHAN_INODE_OBJID, TYPE_ORPHAN_INODE,
                                        fcb->inode, NULL, 0, Batch_Insert);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            goto end;
        }

        fcb->marked_as_orphan = TRUE;
    }

    Status = STATUS_SUCCESS;

end:
    if (fcb->dirty) {
        BOOL lock = FALSE;

        fcb->dirty = FALSE;

        if (!ExIsResourceAcquiredExclusiveLite(&fcb->Vcb->dirty_fcbs_lock)) {
            ExAcquireResourceExclusiveLite(&fcb->Vcb->dirty_fcbs_lock, TRUE);
            lock = TRUE;
        }

        RemoveEntryList(&fcb->list_entry_dirty);

        if (lock)
            ExReleaseResourceLite(&fcb->Vcb->dirty_fcbs_lock);
    }

    return Status;
}

void add_trim_entry_avoid_sb(device_extension* Vcb, device* dev, UINT64 address, UINT64 size) {
    int i;
    ULONG sblen = (ULONG)sector_align(sizeof(superblock), Vcb->superblock.sector_size);

    i = 0;
    while (superblock_addrs[i] != 0) {
        if (superblock_addrs[i] + sblen >= address && superblock_addrs[i] < address + size) {
            if (superblock_addrs[i] > address)
                add_trim_entry(dev, address, superblock_addrs[i] - address);

            if (size <= superblock_addrs[i] + sblen - address)
                return;

            size -= superblock_addrs[i] + sblen - address;
            address = superblock_addrs[i] + sblen;
        } else if (superblock_addrs[i] > address + size)
            break;

        i++;
    }

    add_trim_entry(dev, address, size);
}

static NTSTATUS drop_chunk(device_extension* Vcb, chunk* c, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 i, factor;
#ifdef __REACTOS__
    UINT64 phys_used;
#endif
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];;

    TRACE("dropping chunk %llx\n", c->offset);

    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
        factor = c->chunk_item->num_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
        factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID5)
        factor = c->chunk_item->num_stripes - 1;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID6)
        factor = c->chunk_item->num_stripes - 2;
    else // SINGLE, DUPLICATE, RAID1
        factor = 1;

    // do TRIM
    if (Vcb->trim && !Vcb->options.no_trim) {
        UINT64 len = c->chunk_item->size / factor;

        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            if (c->devices[i] && c->devices[i]->devobj && !c->devices[i]->readonly && c->devices[i]->trim)
                add_trim_entry_avoid_sb(Vcb, c->devices[i], cis[i].offset, len);
        }
    }

    if (!c->cache) {
        Status = load_stored_free_space_cache(Vcb, c, TRUE, Irp);

        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
            WARN("load_stored_free_space_cache returned %08x\n", Status);
    }

    // remove free space cache
    if (c->cache) {
        c->cache->deleted = TRUE;

        Status = excise_extents(Vcb, c->cache, 0, c->cache->inode_item.st_size, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08x\n", Status);
            return Status;
        }

        Status = flush_fcb(c->cache, TRUE, batchlist, Irp);

        free_fcb(c->cache);

        if (c->cache->refcount == 0)
            reap_fcb(c->cache);

        if (!NT_SUCCESS(Status)) {
            ERR("flush_fcb returned %08x\n", Status);
            return Status;
        }

        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(tp.item->key, searchkey)) {
            Status = delete_tree_item(Vcb, &tp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                return Status;
            }
        }
    }

    if (Vcb->space_root) {
        Status = insert_tree_item_batch(batchlist, Vcb, Vcb->space_root, c->offset, TYPE_FREE_SPACE_INFO, c->chunk_item->size,
                                        NULL, 0, Batch_DeleteFreeSpace);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            return Status;
        }
    }

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (!c->created) {
            // remove DEV_EXTENTs from tree 4
            searchkey.obj_id = cis[i].dev_id;
            searchkey.obj_type = TYPE_DEV_EXTENT;
            searchkey.offset = cis[i].offset;

            Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }

            if (!keycmp(tp.item->key, searchkey)) {
                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    return Status;
                }

                if (tp.item->size >= sizeof(DEV_EXTENT)) {
                    DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;

                    c->devices[i]->devitem.bytes_used -= de->length;

                    if (Vcb->balance.thread && Vcb->balance.shrinking && Vcb->balance.opts[0].devid == c->devices[i]->devitem.dev_id) {
                        if (cis[i].offset < Vcb->balance.opts[0].drange_start && cis[i].offset + de->length > Vcb->balance.opts[0].drange_start)
                            space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, Vcb->balance.opts[0].drange_start - cis[i].offset, NULL, rollback);
                    } else
                        space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, de->length, NULL, rollback);
                }
            } else
                WARN("could not find (%llx,%x,%llx) in dev tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        } else {
            UINT64 len = c->chunk_item->size / factor;

            c->devices[i]->devitem.bytes_used -= len;

            if (Vcb->balance.thread && Vcb->balance.shrinking && Vcb->balance.opts[0].devid == c->devices[i]->devitem.dev_id) {
                if (cis[i].offset < Vcb->balance.opts[0].drange_start && cis[i].offset + len > Vcb->balance.opts[0].drange_start)
                    space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, Vcb->balance.opts[0].drange_start - cis[i].offset, NULL, rollback);
            } else
                space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, len, NULL, rollback);
        }
    }

    // modify DEV_ITEMs in chunk tree
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (c->devices[i]) {
            UINT64 j;
            DEV_ITEM* di;

            searchkey.obj_id = 1;
            searchkey.obj_type = TYPE_DEV_ITEM;
            searchkey.offset = c->devices[i]->devitem.dev_id;

            Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }

            if (!keycmp(tp.item->key, searchkey)) {
                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    return Status;
                }

                di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
                if (!di) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(di, &c->devices[i]->devitem, sizeof(DEV_ITEM));

                Status = insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, c->devices[i]->devitem.dev_id, di, sizeof(DEV_ITEM), NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    return Status;
                }
            }

            for (j = i + 1; j < c->chunk_item->num_stripes; j++) {
                if (c->devices[j] == c->devices[i])
                    c->devices[j] = NULL;
            }
        }
    }

    if (!c->created) {
        // remove CHUNK_ITEM from chunk tree
        searchkey.obj_id = 0x100;
        searchkey.obj_type = TYPE_CHUNK_ITEM;
        searchkey.offset = c->offset;

        Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(tp.item->key, searchkey)) {
            Status = delete_tree_item(Vcb, &tp);

            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                return Status;
            }
        } else
            WARN("could not find CHUNK_ITEM for chunk %llx\n", c->offset);

        // remove BLOCK_GROUP_ITEM from extent tree
        searchkey.obj_id = c->offset;
        searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            Status = delete_tree_item(Vcb, &tp);

            if (!NT_SUCCESS(Status)) {
                ERR("delete_tree_item returned %08x\n", Status);
                return Status;
            }
        } else
            WARN("could not find BLOCK_GROUP_ITEM for chunk %llx\n", c->offset);
    }

    if (c->chunk_item->type & BLOCK_FLAG_SYSTEM)
        remove_from_bootstrap(Vcb, 0x100, TYPE_CHUNK_ITEM, c->offset);

    RemoveEntryList(&c->list_entry);

    // clear raid56 incompat flag if dropping last RAID5/6 chunk

    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6) {
        LIST_ENTRY* le;
        BOOL clear_flag = TRUE;

        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);

            if (c2->chunk_item->type & BLOCK_FLAG_RAID5 || c2->chunk_item->type & BLOCK_FLAG_RAID6) {
                clear_flag = FALSE;
                break;
            }

            le = le->Flink;
        }

        if (clear_flag)
            Vcb->superblock.incompat_flags &= ~BTRFS_INCOMPAT_FLAGS_RAID56;
    }

#ifndef __REACTOS__
    UINT64 phys_used = chunk_estimate_phys_size(Vcb, c, c->oldused);
#else
    phys_used = chunk_estimate_phys_size(Vcb, c, c->oldused);
#endif

    if (phys_used < Vcb->superblock.bytes_used)
        Vcb->superblock.bytes_used -= phys_used;
    else
        Vcb->superblock.bytes_used = 0;

    ExFreePool(c->chunk_item);
    ExFreePool(c->devices);

    while (!IsListEmpty(&c->space)) {
        space* s = CONTAINING_RECORD(c->space.Flink, space, list_entry);

        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }

    while (!IsListEmpty(&c->deleting)) {
        space* s = CONTAINING_RECORD(c->deleting.Flink, space, list_entry);

        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }

    release_chunk_lock(c, Vcb);

    ExDeleteResourceLite(&c->partial_stripes_lock);
    ExDeleteResourceLite(&c->range_locks_lock);
    ExDeleteResourceLite(&c->lock);
    ExDeleteResourceLite(&c->changed_extents_lock);

    ExFreePool(c);

    return STATUS_SUCCESS;
}

static NTSTATUS partial_stripe_read(device_extension* Vcb, chunk* c, partial_stripe* ps, UINT64 startoff, UINT16 parity, ULONG offset, ULONG len) {
    NTSTATUS Status;
    ULONG sl = (ULONG)(c->chunk_item->stripe_length / Vcb->superblock.sector_size);
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

    while (len > 0) {
        ULONG readlen = min(offset + len, offset + (sl - (offset % sl))) - offset;
        UINT16 stripe;

        stripe = (parity + (offset / sl) + 1) % c->chunk_item->num_stripes;

        if (c->devices[stripe]->devobj) {
            Status = sync_read_phys(c->devices[stripe]->devobj, cis[stripe].offset + startoff + ((offset % sl) * Vcb->superblock.sector_size),
                                    readlen * Vcb->superblock.sector_size, ps->data + (offset * Vcb->superblock.sector_size), FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("sync_read_phys returned %08x\n", Status);
                return Status;
            }
        } else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
            UINT16 i;
            UINT8* scratch;

            scratch = ExAllocatePoolWithTag(NonPagedPool, readlen * Vcb->superblock.sector_size, ALLOC_TAG);
            if (!scratch) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            for (i = 0; i < c->chunk_item->num_stripes; i++) {
                if (i != stripe) {
                    if (!c->devices[i]->devobj) {
                        ExFreePool(scratch);
                        return STATUS_UNEXPECTED_IO_ERROR;
                    }

                    if (i == 0 || (stripe == 0 && i == 1)) {
                        Status = sync_read_phys(c->devices[i]->devobj, cis[i].offset + startoff + ((offset % sl) * Vcb->superblock.sector_size),
                                                readlen * Vcb->superblock.sector_size, ps->data + (offset * Vcb->superblock.sector_size), FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08x\n", Status);
                            ExFreePool(scratch);
                            return Status;
                        }
                    } else {
                        Status = sync_read_phys(c->devices[i]->devobj, cis[i].offset + startoff + ((offset % sl) * Vcb->superblock.sector_size),
                                                readlen * Vcb->superblock.sector_size, scratch, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08x\n", Status);
                            ExFreePool(scratch);
                            return Status;
                        }

                        do_xor(ps->data + (offset * Vcb->superblock.sector_size), scratch, readlen * Vcb->superblock.sector_size);
                    }
                }
            }

            ExFreePool(scratch);
        } else {
            UINT8* scratch;
            UINT16 k, i, logstripe, error_stripe, num_errors = 0;

            scratch = ExAllocatePoolWithTag(NonPagedPool, (c->chunk_item->num_stripes + 2) * readlen * Vcb->superblock.sector_size, ALLOC_TAG);
            if (!scratch) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            i = (parity + 1) % c->chunk_item->num_stripes;
            for (k = 0; k < c->chunk_item->num_stripes; k++) {
                if (i != stripe) {
                    if (c->devices[i]->devobj) {
                        Status = sync_read_phys(c->devices[i]->devobj, cis[i].offset + startoff + ((offset % sl) * Vcb->superblock.sector_size),
                                                readlen * Vcb->superblock.sector_size, scratch + (k * readlen * Vcb->superblock.sector_size), FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ERR("sync_read_phys returned %08x\n", Status);
                            num_errors++;
                            error_stripe = k;
                        }
                    } else {
                        num_errors++;
                        error_stripe = k;
                    }

                    if (num_errors > 1) {
                        ExFreePool(scratch);
                        return STATUS_UNEXPECTED_IO_ERROR;
                    }
                } else
                    logstripe = k;

                i = (i + 1) % c->chunk_item->num_stripes;
            }

            if (num_errors == 0 || error_stripe == c->chunk_item->num_stripes - 1) {
                for (k = 0; k < c->chunk_item->num_stripes - 1; k++) {
                    if (k != logstripe) {
                        if (k == 0 || (k == 1 && logstripe == 0)) {
                            RtlCopyMemory(ps->data + (offset * Vcb->superblock.sector_size), scratch + (k * readlen * Vcb->superblock.sector_size),
                                          readlen * Vcb->superblock.sector_size);
                        } else {
                            do_xor(ps->data + (offset * Vcb->superblock.sector_size), scratch + (k * readlen * Vcb->superblock.sector_size),
                                   readlen * Vcb->superblock.sector_size);
                        }
                    }
                }
            } else {
                raid6_recover2(scratch, c->chunk_item->num_stripes, readlen * Vcb->superblock.sector_size, logstripe,
                               error_stripe, scratch + (c->chunk_item->num_stripes * readlen * Vcb->superblock.sector_size));

                RtlCopyMemory(ps->data + (offset * Vcb->superblock.sector_size), scratch + (c->chunk_item->num_stripes * readlen * Vcb->superblock.sector_size),
                              readlen * Vcb->superblock.sector_size);
            }

            ExFreePool(scratch);
        }

        offset += readlen;
        len -= readlen;
    }

    return STATUS_SUCCESS;
}

NTSTATUS flush_partial_stripe(device_extension* Vcb, chunk* c, partial_stripe* ps) {
    NTSTATUS Status;
    UINT16 parity2, stripe, startoffstripe;
    UINT8* data;
    UINT64 startoff;
    ULONG runlength, index, last1;
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    LIST_ENTRY* le;
    UINT16 k, num_data_stripes = c->chunk_item->num_stripes - (c->chunk_item->type & BLOCK_FLAG_RAID5 ? 1 : 2);
    UINT64 ps_length = num_data_stripes * c->chunk_item->stripe_length;
    ULONG stripe_length = (ULONG)c->chunk_item->stripe_length;

    // FIXME - do writes asynchronously?

    get_raid0_offset(ps->address - c->offset, stripe_length, num_data_stripes, &startoff, &startoffstripe);

    parity2 = (((ps->address - c->offset) / ps_length) + c->chunk_item->num_stripes - 1) % c->chunk_item->num_stripes;

    // read data (or reconstruct if degraded)

    runlength = RtlFindFirstRunClear(&ps->bmp, &index);
    last1 = 0;

    while (runlength != 0) {
        if (index > last1) {
            Status = partial_stripe_read(Vcb, c, ps, startoff, parity2, last1, index - last1);
            if (!NT_SUCCESS(Status)) {
                ERR("partial_stripe_read returned %08x\n", Status);
                return Status;
            }
        }

        last1 = index + runlength;

        runlength = RtlFindNextForwardRunClear(&ps->bmp, index + runlength, &index);
    }

    if (last1 < ps_length / Vcb->superblock.sector_size) {
        Status = partial_stripe_read(Vcb, c, ps, startoff, parity2, last1, (ULONG)((ps_length / Vcb->superblock.sector_size) - last1));
        if (!NT_SUCCESS(Status)) {
            ERR("partial_stripe_read returned %08x\n", Status);
            return Status;
        }
    }

    // set unallocated data to 0
    le = c->space.Flink;
    while (le != &c->space) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if (s->address + s->size > ps->address && s->address < ps->address + ps_length) {
            UINT64 start = max(ps->address, s->address);
            UINT64 end = min(ps->address + ps_length, s->address + s->size);

            RtlZeroMemory(ps->data + start - ps->address, (ULONG)(end - start));
        } else if (s->address >= ps->address + ps_length)
            break;

        le = le->Flink;
    }

    le = c->deleting.Flink;
    while (le != &c->deleting) {
        space* s = CONTAINING_RECORD(le, space, list_entry);

        if (s->address + s->size > ps->address && s->address < ps->address + ps_length) {
            UINT64 start = max(ps->address, s->address);
            UINT64 end = min(ps->address + ps_length, s->address + s->size);

            RtlZeroMemory(ps->data + start - ps->address, (ULONG)(end - start));
        } else if (s->address >= ps->address + ps_length)
            break;

        le = le->Flink;
    }

    stripe = (parity2 + 1) % c->chunk_item->num_stripes;

    data = ps->data;
    for (k = 0; k < num_data_stripes; k++) {
        if (c->devices[stripe]->devobj) {
            Status = write_data_phys(c->devices[stripe]->devobj, cis[stripe].offset + startoff, data, stripe_length);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data_phys returned %08x\n", Status);
                return Status;
            }
        }

        data += stripe_length;
        stripe = (stripe + 1) % c->chunk_item->num_stripes;
    }

    // write parity
    if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
        if (c->devices[parity2]->devobj) {
            UINT16 i;

            for (i = 1; i < c->chunk_item->num_stripes - 1; i++) {
                do_xor(ps->data, ps->data + (i * stripe_length), stripe_length);
            }

            Status = write_data_phys(c->devices[parity2]->devobj, cis[parity2].offset + startoff, ps->data, stripe_length);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data_phys returned %08x\n", Status);
                return Status;
            }
        }
    } else {
        UINT16 parity1 = (parity2 + c->chunk_item->num_stripes - 1) % c->chunk_item->num_stripes;

        if (c->devices[parity1]->devobj || c->devices[parity2]->devobj) {
            UINT8* scratch;
            UINT16 i;

            scratch = ExAllocatePoolWithTag(NonPagedPool, stripe_length * 2, ALLOC_TAG);
            if (!scratch) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            i = c->chunk_item->num_stripes - 3;

            while (TRUE) {
                if (i == c->chunk_item->num_stripes - 3) {
                    RtlCopyMemory(scratch, ps->data + (i * stripe_length), stripe_length);
                    RtlCopyMemory(scratch + stripe_length, ps->data + (i * stripe_length), stripe_length);
                } else {
                    do_xor(scratch, ps->data + (i * stripe_length), stripe_length);

                    galois_double(scratch + stripe_length, stripe_length);
                    do_xor(scratch + stripe_length, ps->data + (i * stripe_length), stripe_length);
                }

                if (i == 0)
                    break;

                i--;
            }

            if (c->devices[parity1]->devobj) {
                Status = write_data_phys(c->devices[parity1]->devobj, cis[parity1].offset + startoff, scratch, stripe_length);
                if (!NT_SUCCESS(Status)) {
                    ERR("write_data_phys returned %08x\n", Status);
                    ExFreePool(scratch);
                    return Status;
                }
            }

            if (c->devices[parity2]->devobj) {
                Status = write_data_phys(c->devices[parity2]->devobj, cis[parity2].offset + startoff, scratch + stripe_length, stripe_length);
                if (!NT_SUCCESS(Status)) {
                    ERR("write_data_phys returned %08x\n", Status);
                    ExFreePool(scratch);
                    return Status;
                }
            }

            ExFreePool(scratch);
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS update_chunks(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    UINT64 used_minus_cache;

    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);

    // FIXME - do tree chunks before data chunks

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        le2 = le->Flink;

        if (c->changed) {
            acquire_chunk_lock(c, Vcb);

            // flush partial stripes
            if (!Vcb->readonly && (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)) {
                ExAcquireResourceExclusiveLite(&c->partial_stripes_lock, TRUE);

                while (!IsListEmpty(&c->partial_stripes)) {
                    partial_stripe* ps = CONTAINING_RECORD(RemoveHeadList(&c->partial_stripes), partial_stripe, list_entry);

                    Status = flush_partial_stripe(Vcb, c, ps);

                    if (ps->bmparr)
                        ExFreePool(ps->bmparr);

                    ExFreePool(ps);

                    if (!NT_SUCCESS(Status)) {
                        ERR("flush_partial_stripe returned %08x\n", Status);
                        ExReleaseResourceLite(&c->partial_stripes_lock);
                        release_chunk_lock(c, Vcb);
                        ExReleaseResourceLite(&Vcb->chunk_lock);
                        return Status;
                    }
                }

                ExReleaseResourceLite(&c->partial_stripes_lock);
            }

            if (c->list_entry_balance.Flink) {
                release_chunk_lock(c, Vcb);
                le = le2;
                continue;
            }

            if (c->space_changed || c->created) {
                BOOL created = c->created;

                used_minus_cache = c->used;

                // subtract self-hosted cache
                if (used_minus_cache > 0 && c->chunk_item->type & BLOCK_FLAG_DATA && c->cache && c->cache->inode_item.st_size == c->used) {
                    LIST_ENTRY* le3;

                    le3 = c->cache->extents.Flink;
                    while (le3 != &c->cache->extents) {
                        extent* ext = CONTAINING_RECORD(le3, extent, list_entry);
                        EXTENT_DATA* ed = &ext->extent_data;

                        if (!ext->ignore) {
                            if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                                if (ed2->size != 0 && ed2->address >= c->offset && ed2->address + ed2->size <= c->offset + c->chunk_item->size)
                                    used_minus_cache -= ed2->size;
                            }
                        }

                        le3 = le3->Flink;
                    }
                }

                if (used_minus_cache == 0) {
                    Status = drop_chunk(Vcb, c, batchlist, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("drop_chunk returned %08x\n", Status);
                        release_chunk_lock(c, Vcb);
                        ExReleaseResourceLite(&Vcb->chunk_lock);
                        return Status;
                    }

                    // c is now freed, so avoid releasing non-existent lock
                    le = le2;
                    continue;
                } else if (c->created) {
                    Status = create_chunk(Vcb, c, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("create_chunk returned %08x\n", Status);
                        release_chunk_lock(c, Vcb);
                        ExReleaseResourceLite(&Vcb->chunk_lock);
                        return Status;
                    }
                }

                if (used_minus_cache > 0 || created)
                    release_chunk_lock(c, Vcb);
            } else
                release_chunk_lock(c, Vcb);
        }

        le = le2;
    }

    ExReleaseResourceLite(&Vcb->chunk_lock);

    return STATUS_SUCCESS;
}

static NTSTATUS delete_root_ref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, UINT64 parinode, PANSI_STRING utf8, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        if (tp.item->size < sizeof(ROOT_REF)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(ROOT_REF));
            return STATUS_INTERNAL_ERROR;
        } else {
            ROOT_REF* rr;
            ULONG len;

            rr = (ROOT_REF*)tp.item->data;
            len = tp.item->size;

            do {
                UINT16 itemlen;

                if (len < sizeof(ROOT_REF) || len < offsetof(ROOT_REF, name[0]) + rr->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }

                itemlen = (UINT16)offsetof(ROOT_REF, name[0]) + rr->n;

                if (rr->dir == parinode && rr->n == utf8->Length && RtlCompareMemory(rr->name, utf8->Buffer, rr->n) == rr->n) {
                    UINT16 newlen = tp.item->size - itemlen;

                    Status = delete_tree_item(Vcb, &tp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("delete_tree_item returned %08x\n", Status);
                        return Status;
                    }

                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newrr = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *rroff;

                        if (!newrr) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)rr > tp.item->data) {
                            RtlCopyMemory(newrr, tp.item->data, (UINT8*)rr - tp.item->data);
                            rroff = newrr + ((UINT8*)rr - tp.item->data);
                        } else {
                            rroff = newrr;
                        }

                        if ((UINT8*)&rr->name[rr->n] < tp.item->data + tp.item->size)
                            RtlCopyMemory(rroff, &rr->name[rr->n], tp.item->size - ((UINT8*)&rr->name[rr->n] - tp.item->data));

                        Status = insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newrr, newlen, NULL, Irp);
                        if (!NT_SUCCESS(Status)) {
                            ERR("insert_tree_item returned %08x\n", Status);
                            ExFreePool(newrr);
                            return Status;
                        }
                    }

                    break;
                }

                if (len > itemlen) {
                    len -= itemlen;
                    rr = (ROOT_REF*)&rr->name[rr->n];
                } else
                    break;
            } while (len > 0);
        }
    } else {
        WARN("could not find ROOT_REF entry for subvol %llx in %llx\n", searchkey.offset, searchkey.obj_id);
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(suppress: 28194)
#endif
static NTSTATUS add_root_ref(_In_ device_extension* Vcb, _In_ UINT64 subvolid, _In_ UINT64 parsubvolid, _In_ __drv_aliasesMem ROOT_REF* rr, _In_opt_ PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(searchkey, tp.item->key)) {
        UINT16 rrsize = tp.item->size + (UINT16)offsetof(ROOT_REF, name[0]) + rr->n;
        UINT8* rr2;

        rr2 = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
        if (!rr2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (tp.item->size > 0)
            RtlCopyMemory(rr2, tp.item->data, tp.item->size);

        RtlCopyMemory(rr2 + tp.item->size, rr, offsetof(ROOT_REF, name[0]) + rr->n);
        ExFreePool(rr);

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            ExFreePool(rr2);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr2, rrsize, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(rr2);
            return Status;
        }
    } else {
        Status = insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr, (UINT16)offsetof(ROOT_REF, name[0]) + rr->n, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(rr);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static NTSTATUS update_root_backref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    UINT8* data;
    UINT16 datalen;
    NTSTATUS Status;

    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(tp.item->key, searchkey) && tp.item->size > 0) {
        datalen = tp.item->size;

        data = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
        if (!data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(data, tp.item->data, datalen);
    } else {
        datalen = 0;
        data = NULL;
    }

    searchkey.obj_id = subvolid;
    searchkey.obj_type = TYPE_ROOT_BACKREF;
    searchkey.offset = parsubvolid;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);

        if (datalen > 0)
            ExFreePool(data);

        return Status;
    }

    if (!keycmp(tp.item->key, searchkey)) {
        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);

            if (datalen > 0)
                ExFreePool(data);

            return Status;
        }
    }

    if (datalen > 0) {
        Status = insert_tree_item(Vcb, Vcb->root_root, subvolid, TYPE_ROOT_BACKREF, parsubvolid, data, datalen, NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(data);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS add_root_item_to_cache(device_extension* Vcb, UINT64 root, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;

    searchkey.obj_id = root;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;

    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
        return STATUS_INTERNAL_ERROR;
    }

    if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, create new entry with new bits zeroed
        ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
        if (!ri) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (tp.item->size > 0)
            RtlCopyMemory(ri, tp.item->data, tp.item->size);

        RtlZeroMemory(((UINT8*)ri) + tp.item->size, sizeof(ROOT_ITEM) - tp.item->size);

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            ExFreePool(ri);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            ExFreePool(ri);
            return Status;
        }
    } else {
        tp.tree->write = TRUE;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS flush_fileref(file_ref* fileref, LIST_ENTRY* batchlist, PIRP Irp) {
    NTSTATUS Status;

    // if fileref created and then immediately deleted, do nothing
    if (fileref->created && fileref->deleted) {
        fileref->dirty = FALSE;
        return STATUS_SUCCESS;
    }

    if (fileref->fcb->ads) {
        fileref->dirty = FALSE;
        return STATUS_SUCCESS;
    }

    if (fileref->created) {
        UINT16 disize;
        DIR_ITEM *di, *di2;
        UINT32 crc32;

        crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        disize = (UINT16)(offsetof(DIR_ITEM, name[0]) + fileref->dc->utf8.Length);
        di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            di->key.obj_id = fileref->fcb->inode;
            di->key.obj_type = TYPE_INODE_ITEM;
            di->key.offset = 0;
        } else { // subvolume
            di->key.obj_id = fileref->fcb->subvol->id;
            di->key.obj_type = TYPE_ROOT_ITEM;
            di->key.offset = 0xffffffffffffffff;
        }

        di->transid = fileref->fcb->Vcb->superblock.generation;
        di->m = 0;
        di->n = (UINT16)fileref->dc->utf8.Length;
        di->type = fileref->fcb->type;
        RtlCopyMemory(di->name, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(di2, di, disize);

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                        fileref->dc->index, di, disize, Batch_Insert);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            return Status;
        }

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM, crc32,
                                        di2, disize, Batch_DirItem);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            return Status;
        }

        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF* ir;

            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + fileref->dc->utf8.Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir->index = fileref->dc->index;
            ir->n = fileref->dc->utf8.Length;
            RtlCopyMemory(ir->name, fileref->dc->utf8.Buffer, ir->n);

            Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                            ir, sizeof(INODE_REF) - 1 + ir->n, Batch_InodeRef);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                return Status;
            }
        } else if (fileref->fcb != fileref->fcb->Vcb->dummy_fcb) {
            ULONG rrlen;
            ROOT_REF* rr;

            rrlen = sizeof(ROOT_REF) - 1 + fileref->dc->utf8.Length;

            rr = ExAllocatePoolWithTag(PagedPool, rrlen, ALLOC_TAG);
            if (!rr) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            rr->dir = fileref->parent->fcb->inode;
            rr->index = fileref->dc->index;
            rr->n = fileref->dc->utf8.Length;
            RtlCopyMemory(rr->name, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

            Status = add_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, rr, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("add_root_ref returned %08x\n", Status);
                return Status;
            }

            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }

        fileref->created = FALSE;
    } else if (fileref->deleted) {
        UINT32 crc32;
        ANSI_STRING* name;
        DIR_ITEM* di;

        name = &fileref->oldutf8;

        crc32 = calc_crc32c(0xfffffffe, (UINT8*)name->Buffer, name->Length);

        TRACE("deleting %.*S\n", file_desc_fileref(fileref));

        di = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + name->Length, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        di->m = 0;
        di->n = name->Length;
        RtlCopyMemory(di->name, name->Buffer, name->Length);

        // delete DIR_ITEM (0x54)

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM,
                                        crc32, di, sizeof(DIR_ITEM) - 1 + name->Length, Batch_DeleteDirItem);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            return Status;
        }

        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF* ir;

            // delete INODE_REF (0xc)

            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + name->Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir->index = fileref->oldindex;
            ir->n = name->Length;
            RtlCopyMemory(ir->name, name->Buffer, name->Length);

            Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF,
                                            fileref->parent->fcb->inode, ir, sizeof(INODE_REF) - 1 + name->Length, Batch_DeleteInodeRef);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                return Status;
            }
        } else if (fileref->fcb != fileref->fcb->Vcb->dummy_fcb) { // subvolume
            Status = delete_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, name, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_root_ref returned %08x\n", Status);
                return Status;
            }

            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }

        // delete DIR_INDEX (0x60)

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                        fileref->oldindex, NULL, 0, Batch_Delete);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            return Status;
        }

        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    } else { // rename or change type
        PANSI_STRING oldutf8 = fileref->oldutf8.Buffer ? &fileref->oldutf8 : &fileref->dc->utf8;
        UINT32 crc32, oldcrc32;
        UINT16 disize;
        DIR_ITEM *olddi, *di, *di2;

        crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        if (!fileref->oldutf8.Buffer)
            oldcrc32 = crc32;
        else
            oldcrc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->oldutf8.Buffer, fileref->oldutf8.Length);

        olddi = ExAllocatePoolWithTag(PagedPool, sizeof(DIR_ITEM) - 1 + oldutf8->Length, ALLOC_TAG);
        if (!olddi) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        olddi->m = 0;
        olddi->n = (UINT16)oldutf8->Length;
        RtlCopyMemory(olddi->name, oldutf8->Buffer, oldutf8->Length);

        // delete DIR_ITEM (0x54)

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM,
                                        oldcrc32, olddi, sizeof(DIR_ITEM) - 1 + oldutf8->Length, Batch_DeleteDirItem);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            ExFreePool(olddi);
            return Status;
        }

        // add DIR_ITEM (0x54)

        disize = (UINT16)(offsetof(DIR_ITEM, name[0]) + fileref->dc->utf8.Length);
        di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di2) {
            ERR("out of memory\n");
            ExFreePool(di);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (fileref->dc)
            di->key = fileref->dc->key;
        else if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            di->key.obj_id = fileref->fcb->inode;
            di->key.obj_type = TYPE_INODE_ITEM;
            di->key.offset = 0;
        } else { // subvolume
            di->key.obj_id = fileref->fcb->subvol->id;
            di->key.obj_type = TYPE_ROOT_ITEM;
            di->key.offset = 0xffffffffffffffff;
        }

        di->transid = fileref->fcb->Vcb->superblock.generation;
        di->m = 0;
        di->n = (UINT16)fileref->dc->utf8.Length;
        di->type = fileref->fcb->type;
        RtlCopyMemory(di->name, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

        RtlCopyMemory(di2, di, disize);

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_ITEM, crc32,
                                        di, disize, Batch_DirItem);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            ExFreePool(di2);
            ExFreePool(di);
            return Status;
        }

        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            INODE_REF *ir, *ir2;

            // delete INODE_REF (0xc)

            ir = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + oldutf8->Length, ALLOC_TAG);
            if (!ir) {
                ERR("out of memory\n");
                ExFreePool(di2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir->index = fileref->dc->index;
            ir->n = oldutf8->Length;
            RtlCopyMemory(ir->name, oldutf8->Buffer, ir->n);

            Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                            ir, sizeof(INODE_REF) - 1 + ir->n, Batch_DeleteInodeRef);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                ExFreePool(ir);
                ExFreePool(di2);
                return Status;
            }

            // add INODE_REF (0xc)

            ir2 = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_REF) - 1 + fileref->dc->utf8.Length, ALLOC_TAG);
            if (!ir2) {
                ERR("out of memory\n");
                ExFreePool(di2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ir2->index = fileref->dc->index;
            ir2->n = fileref->dc->utf8.Length;
            RtlCopyMemory(ir2->name, fileref->dc->utf8.Buffer, ir2->n);

            Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->fcb->subvol, fileref->fcb->inode, TYPE_INODE_REF, fileref->parent->fcb->inode,
                                            ir2, sizeof(INODE_REF) - 1 + ir2->n, Batch_InodeRef);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item_batch returned %08x\n", Status);
                ExFreePool(ir2);
                ExFreePool(di2);
                return Status;
            }
        } else if (fileref->fcb != fileref->fcb->Vcb->dummy_fcb) { // subvolume
            ULONG rrlen;
            ROOT_REF* rr;

            Status = delete_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, oldutf8, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_root_ref returned %08x\n", Status);
                ExFreePool(di2);
                return Status;
            }

            rrlen = sizeof(ROOT_REF) - 1 + fileref->dc->utf8.Length;

            rr = ExAllocatePoolWithTag(PagedPool, rrlen, ALLOC_TAG);
            if (!rr) {
                ERR("out of memory\n");
                ExFreePool(di2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            rr->dir = fileref->parent->fcb->inode;
            rr->index = fileref->dc->index;
            rr->n = fileref->dc->utf8.Length;
            RtlCopyMemory(rr->name, fileref->dc->utf8.Buffer, fileref->dc->utf8.Length);

            Status = add_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, rr, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("add_root_ref returned %08x\n", Status);
                ExFreePool(di2);
                return Status;
            }

            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                ExFreePool(di2);
                return Status;
            }
        }

        // delete DIR_INDEX (0x60)

        Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                        fileref->dc->index, NULL, 0, Batch_Delete);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            ExFreePool(di2);
            return Status;
        }

        // add DIR_INDEX (0x60)

       Status = insert_tree_item_batch(batchlist, fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX,
                                       fileref->dc->index, di2, disize, Batch_Insert);
       if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item_batch returned %08x\n", Status);
            ExFreePool(di2);
            return Status;
        }

        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    }

    fileref->dirty = FALSE;

    return STATUS_SUCCESS;
}

static void flush_disk_caches(device_extension* Vcb) {
    LIST_ENTRY* le;
    ioctl_context context;
    ULONG num;

    context.left = 0;

    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj && !dev->readonly && dev->can_flush)
            context.left++;

        le = le->Flink;
    }

    if (context.left == 0)
        return;

    num = 0;

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);

    context.stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(ioctl_context_stripe) * context.left, ALLOC_TAG);
    if (!context.stripes) {
        ERR("out of memory\n");
        return;
    }

    RtlZeroMemory(context.stripes, sizeof(ioctl_context_stripe) * context.left);

    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj && !dev->readonly && dev->can_flush) {
            PIO_STACK_LOCATION IrpSp;
            ioctl_context_stripe* stripe = &context.stripes[num];

            RtlZeroMemory(&stripe->apte, sizeof(ATA_PASS_THROUGH_EX));

            stripe->apte.Length = sizeof(ATA_PASS_THROUGH_EX);
            stripe->apte.TimeOutValue = 5;
            stripe->apte.CurrentTaskFile[6] = IDE_COMMAND_FLUSH_CACHE;

            stripe->Irp = IoAllocateIrp(dev->devobj->StackSize, FALSE);

            if (!stripe->Irp) {
                ERR("IoAllocateIrp failed\n");
                goto nextdev;
            }

            IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
            IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

            IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ATA_PASS_THROUGH;
            IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ATA_PASS_THROUGH_EX);
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ATA_PASS_THROUGH_EX);

            stripe->Irp->AssociatedIrp.SystemBuffer = &stripe->apte;
            stripe->Irp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
            stripe->Irp->UserBuffer = &stripe->apte;
            stripe->Irp->UserIosb = &stripe->iosb;

            IoSetCompletionRoutine(stripe->Irp, ioctl_completion, &context, TRUE, TRUE, TRUE);

            IoCallDriver(dev->devobj, stripe->Irp);

nextdev:
            num++;
        }

        le = le->Flink;
    }

    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

    ExFreePool(context.stripes);
}

static NTSTATUS flush_changed_dev_stats(device_extension* Vcb, device* dev, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT16 statslen;
    UINT64* stats;

    searchkey.obj_id = 0;
    searchkey.obj_type = TYPE_DEV_STATS;
    searchkey.offset = dev->devitem.dev_id;

    Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (!keycmp(tp.item->key, searchkey)) {
        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            return Status;
        }
    }

    statslen = sizeof(UINT64) * 5;
    stats = ExAllocatePoolWithTag(PagedPool, statslen, ALLOC_TAG);
    if (!stats) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(stats, dev->stats, statslen);

    Status = insert_tree_item(Vcb, Vcb->dev_root, 0, TYPE_DEV_STATS, dev->devitem.dev_id, stats, statslen, NULL, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("insert_tree_item returned %08x\n", Status);
        ExFreePool(stats);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS flush_subvol(device_extension* Vcb, root* r, PIRP Irp) {
    NTSTATUS Status;

    if (r != Vcb->root_root && r != Vcb->chunk_root) {
        KEY searchkey;
        traverse_ptr tp;
        ROOT_ITEM* ri;

        searchkey.obj_id = r->id;
        searchkey.obj_type = TYPE_ROOT_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
            return STATUS_INTERNAL_ERROR;
        }

        ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
        if (!ri) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(ri, &r->root_item, sizeof(ROOT_ITEM));

        Status = delete_tree_item(Vcb, &tp);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_tree_item returned %08x\n", Status);
            return Status;
        }

        Status = insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_tree_item returned %08x\n", Status);
            return Status;
        }
    }

    if (r->received) {
        KEY searchkey;
        traverse_ptr tp;

        if (!Vcb->uuid_root) {
            root* uuid_root;

            TRACE("uuid root doesn't exist, creating it\n");

            Status = create_root(Vcb, BTRFS_ROOT_UUID, &uuid_root, FALSE, 0, Irp);

            if (!NT_SUCCESS(Status)) {
                ERR("create_root returned %08x\n", Status);
                return Status;
            }

            Vcb->uuid_root = uuid_root;
        }

        RtlCopyMemory(&searchkey.obj_id, &r->root_item.received_uuid, sizeof(UINT64));
        searchkey.obj_type = TYPE_SUBVOL_REC_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.received_uuid.uuid[sizeof(UINT64)], sizeof(UINT64));

        Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(tp.item->key, searchkey)) {
            if (tp.item->size + sizeof(UINT64) <= Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node)) {
                UINT64* ids;

                ids = ExAllocatePoolWithTag(PagedPool, tp.item->size + sizeof(UINT64), ALLOC_TAG);
                if (!ids) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(ids, tp.item->data, tp.item->size);
                RtlCopyMemory((UINT8*)ids + tp.item->size, &r->id, sizeof(UINT64));

                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    ExFreePool(ids);
                    return Status;
                }

                Status = insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ids, tp.item->size + sizeof(UINT64), NULL, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_tree_item returned %08x\n", Status);
                    ExFreePool(ids);
                    return Status;
                }
            }
        } else {
            UINT64* root_num;

            root_num = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64), ALLOC_TAG);
            if (!root_num) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            *root_num = r->id;

            Status = insert_tree_item(Vcb, Vcb->uuid_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, root_num, sizeof(UINT64), NULL, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("insert_tree_item returned %08x\n", Status);
                ExFreePool(root_num);
                return Status;
            }
        }

        r->received = FALSE;
    }

    r->dirty = FALSE;

    return STATUS_SUCCESS;
}

static NTSTATUS test_not_full(device_extension* Vcb) {
    UINT64 reserve, could_alloc, free_space;
    LIST_ENTRY* le;

    // This function ensures we drop into readonly mode if we're about to leave very little
    // space for metadata - this is similar to the "global reserve" of the Linux driver.
    // Otherwise we might completely fill our space, at which point due to COW we can't
    // delete anything in order to fix this.

    reserve = Vcb->extent_root->root_item.bytes_used;
    reserve += Vcb->root_root->root_item.bytes_used;
    if (Vcb->checksum_root) reserve += Vcb->checksum_root->root_item.bytes_used;

    reserve = max(reserve, 0x1000000); // 16 M
    reserve = min(reserve, 0x20000000); // 512 M

    // Find out how much space would be available for new metadata chunks

    could_alloc = 0;

    if (Vcb->metadata_flags & BLOCK_FLAG_RAID5) {
        UINT64 s1 = 0, s2 = 0, s3 = 0;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly) {
                UINT64 space = dev->devitem.num_bytes - dev->devitem.bytes_used;

                if (space >= s1) {
                    s3 = s2;
                    s2 = s1;
                    s1 = space;
                } else if (space >= s2) {
                    s3 = s2;
                    s2 = space;
                } else if (space >= s3)
                    s3 = space;
            }

            le = le->Flink;
        }

        could_alloc = s3 * 2;
    } else if (Vcb->metadata_flags & (BLOCK_FLAG_RAID10 | BLOCK_FLAG_RAID6)) {
        UINT64 s1 = 0, s2 = 0, s3 = 0, s4 = 0;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly) {
                UINT64 space = dev->devitem.num_bytes - dev->devitem.bytes_used;

                if (space >= s1) {
                    s4 = s3;
                    s3 = s2;
                    s2 = s1;
                    s1 = space;
                } else if (space >= s2) {
                    s4 = s3;
                    s3 = s2;
                    s2 = space;
                } else if (space >= s3) {
                    s4 = s3;
                    s3 = space;
                } else if (space >= s4)
                    s4 = space;
            }

            le = le->Flink;
        }

        could_alloc = s4 * 2;
    } else if (Vcb->metadata_flags & (BLOCK_FLAG_RAID0 | BLOCK_FLAG_RAID1)) {
        UINT64 s1 = 0, s2 = 0;

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly) {
                UINT64 space = dev->devitem.num_bytes - dev->devitem.bytes_used;

                if (space >= s1) {
                    s2 = s1;
                    s1 = space;
                } else if (space >= s2)
                    s2 = space;
            }

            le = le->Flink;
        }

        if (Vcb->metadata_flags & BLOCK_FLAG_RAID1)
            could_alloc = s2;
        else // RAID0
            could_alloc = s2 * 2;
    } else if (Vcb->metadata_flags & BLOCK_FLAG_DUPLICATE) {
        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly) {
                UINT64 space = (dev->devitem.num_bytes - dev->devitem.bytes_used) / 2;

                could_alloc = max(could_alloc, space);
            }

            le = le->Flink;
        }
    } else { // SINGLE
        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->readonly) {
                UINT64 space = dev->devitem.num_bytes - dev->devitem.bytes_used;

                could_alloc = max(could_alloc, space);
            }

            le = le->Flink;
        }
    }

    if (could_alloc >= reserve)
        return STATUS_SUCCESS;

    free_space = 0;

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->reloc && !c->readonly && c->chunk_item->type & BLOCK_FLAG_METADATA) {
            free_space += c->chunk_item->size - c->used;

            if (free_space + could_alloc >= reserve)
                return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    return STATUS_DISK_FULL;
}

static NTSTATUS check_for_orphans_root(device_extension* Vcb, root* r, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    LIST_ENTRY rollback;

    TRACE("(%p, %p)\n", Vcb, r);

    InitializeListHead(&rollback);

    searchkey.obj_id = BTRFS_ORPHAN_INODE_OBJID;
    searchkey.obj_type = TYPE_ORPHAN_INODE;
    searchkey.offset = 0;

    Status = find_item(Vcb, r, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    do {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
            break;

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            fcb* fcb;

            TRACE("removing orphaned inode %llx\n", tp.item->key.offset);

            Status = open_fcb(Vcb, r, tp.item->key.offset, 0, NULL, FALSE, NULL, &fcb, PagedPool, Irp);
            if (!NT_SUCCESS(Status))
                ERR("open_fcb returned %08x\n", Status);
            else {
                if (fcb->inode_item.st_nlink == 0) {
                    if (fcb->type != BTRFS_TYPE_DIRECTORY && fcb->inode_item.st_size > 0) {
                        Status = excise_extents(Vcb, fcb, 0, sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size), Irp, &rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("excise_extents returned %08x\n", Status);
                            goto end;
                        }
                    }

                    fcb->deleted = TRUE;

                    mark_fcb_dirty(fcb);
                }

                free_fcb(fcb);

                Status = delete_tree_item(Vcb, &tp);
                if (!NT_SUCCESS(Status)) {
                    ERR("delete_tree_item returned %08x\n", Status);
                    goto end;
                }
            }
        }

        if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp))
            tp = next_tp;
        else
            break;
    } while (TRUE);

    Status = STATUS_SUCCESS;

    clear_rollback(&rollback);

end:
    do_rollback(Vcb, &rollback);

    return Status;
}

static NTSTATUS check_for_orphans(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    LIST_ENTRY* le;

    if (IsListEmpty(&Vcb->dirty_filerefs))
        return STATUS_SUCCESS;

    le = Vcb->dirty_filerefs.Flink;
    while (le != &Vcb->dirty_filerefs) {
        file_ref* fr = CONTAINING_RECORD(le, file_ref, list_entry_dirty);

        if (!fr->fcb->subvol->checked_for_orphans) {
            Status = check_for_orphans_root(Vcb, fr->fcb->subvol, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("check_for_orphans_root returned %08x\n", Status);
                return Status;
            }

            fr->fcb->subvol->checked_for_orphans = TRUE;
        }

        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS do_write2(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY *le, batchlist;
    BOOL cache_changed = FALSE;
    volume_device_extension* vde;
    BOOL no_cache = FALSE;
#ifdef DEBUG_FLUSH_TIMES
    UINT64 filerefs = 0, fcbs = 0;
    LARGE_INTEGER freq, time1, time2;
#endif
#ifdef DEBUG_WRITE_LOOPS
    UINT loops = 0;
#endif

    TRACE("(%p)\n", Vcb);

    InitializeListHead(&batchlist);

#ifdef DEBUG_FLUSH_TIMES
    time1 = KeQueryPerformanceCounter(&freq);
#endif

    Status = check_for_orphans(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("check_for_orphans returned %08x\n", Status);
        return Status;
    }

    ExAcquireResourceExclusiveLite(&Vcb->dirty_filerefs_lock, TRUE);

    while (!IsListEmpty(&Vcb->dirty_filerefs)) {
        file_ref* fr = CONTAINING_RECORD(RemoveHeadList(&Vcb->dirty_filerefs), file_ref, list_entry_dirty);

        flush_fileref(fr, &batchlist, Irp);
        free_fileref(fr);

#ifdef DEBUG_FLUSH_TIMES
        filerefs++;
#endif
    }

    ExReleaseResourceLite(&Vcb->dirty_filerefs_lock);

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08x\n", Status);
        return Status;
    }

#ifdef DEBUG_FLUSH_TIMES
    time2 = KeQueryPerformanceCounter(NULL);

    ERR("flushed %llu filerefs in %llu (freq = %llu)\n", filerefs, time2.QuadPart - time1.QuadPart, freq.QuadPart);

    time1 = KeQueryPerformanceCounter(&freq);
#endif

    // We process deleted streams first, so we don't run over our xattr
    // limit unless we absolutely have to.
    // We also process deleted normal files, to avoid any problems
    // caused by inode collisions.

    ExAcquireResourceExclusiveLite(&Vcb->dirty_fcbs_lock, TRUE);

    le = Vcb->dirty_fcbs.Flink;
    while (le != &Vcb->dirty_fcbs) {
        fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_dirty);
        LIST_ENTRY* le2 = le->Flink;

        if (fcb->deleted) {
            ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
            Status = flush_fcb(fcb, FALSE, &batchlist, Irp);
            ExReleaseResourceLite(fcb->Header.Resource);

            free_fcb(fcb);

            if (!NT_SUCCESS(Status)) {
                ERR("flush_fcb returned %08x\n", Status);
                clear_batch_list(Vcb, &batchlist);
                ExReleaseResourceLite(&Vcb->dirty_fcbs_lock);
                return Status;
            }

#ifdef DEBUG_FLUSH_TIMES
            fcbs++;
#endif
        }

        le = le2;
    }

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08x\n", Status);
        ExReleaseResourceLite(&Vcb->dirty_fcbs_lock);
        return Status;
    }

    le = Vcb->dirty_fcbs.Flink;
    while (le != &Vcb->dirty_fcbs) {
        fcb* fcb = CONTAINING_RECORD(le, struct _fcb, list_entry_dirty);
        LIST_ENTRY* le2 = le->Flink;

        if (fcb->subvol != Vcb->root_root) {
            ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
            Status = flush_fcb(fcb, FALSE, &batchlist, Irp);
            ExReleaseResourceLite(fcb->Header.Resource);
            free_fcb(fcb);

            if (!NT_SUCCESS(Status)) {
                ERR("flush_fcb returned %08x\n", Status);
                ExReleaseResourceLite(&Vcb->dirty_fcbs_lock);
                return Status;
            }

#ifdef DEBUG_FLUSH_TIMES
            fcbs++;
#endif
        }

        le = le2;
    }

    ExReleaseResourceLite(&Vcb->dirty_fcbs_lock);

    Status = commit_batch_list(Vcb, &batchlist, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("commit_batch_list returned %08x\n", Status);
        return Status;
    }

#ifdef DEBUG_FLUSH_TIMES
    time2 = KeQueryPerformanceCounter(NULL);

    ERR("flushed %llu fcbs in %llu (freq = %llu)\n", filerefs, time2.QuadPart - time1.QuadPart, freq.QuadPart);
#endif

    // no need to get dirty_subvols_lock here, as we have tree_lock exclusively
    while (!IsListEmpty(&Vcb->dirty_subvols)) {
        root* r = CONTAINING_RECORD(RemoveHeadList(&Vcb->dirty_subvols), root, list_entry_dirty);

        Status = flush_subvol(Vcb, r, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_subvol returned %08x\n", Status);
            return Status;
        }
    }

    if (!IsListEmpty(&Vcb->drop_roots)) {
        Status = drop_roots(Vcb, Irp, rollback);

        if (!NT_SUCCESS(Status)) {
            ERR("drop_roots returned %08x\n", Status);
            return Status;
        }
    }

    Status = update_chunks(Vcb, &batchlist, Irp, rollback);

    if (!NT_SUCCESS(Status)) {
        ERR("update_chunks returned %08x\n", Status);
        return Status;
    }

    Status = commit_batch_list(Vcb, &batchlist, Irp);

    // If only changing superblock, e.g. changing label, we still need to rewrite
    // the root tree so the generations match, otherwise you won't be able to mount on Linux.
    if (!Vcb->root_root->treeholder.tree || !Vcb->root_root->treeholder.tree->write) {
        KEY searchkey;

        traverse_ptr tp;

        searchkey.obj_id = 0;
        searchkey.obj_type = 0;
        searchkey.offset = 0;

        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        Vcb->root_root->treeholder.tree->write = TRUE;
    }

    // make sure we always update the extent tree
    Status = add_root_item_to_cache(Vcb, BTRFS_ROOT_EXTENT, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("add_root_item_to_cache returned %08x\n", Status);
        return Status;
    }

    if (Vcb->stats_changed) {
        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (dev->stats_changed) {
                Status = flush_changed_dev_stats(Vcb, dev, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("flush_changed_dev_stats returned %08x\n", Status);
                    return Status;
                }
                dev->stats_changed = FALSE;
            }

            le = le->Flink;
        }

        Vcb->stats_changed = FALSE;
    }

    do {
        Status = add_parents(Vcb, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("add_parents returned %08x\n", Status);
            goto end;
        }

        Status = allocate_tree_extents(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("allocate_tree_extents returned %08x\n", Status);
            goto end;
        }

        Status = do_splits(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("do_splits returned %08x\n", Status);
            goto end;
        }

        Status = update_chunk_usage(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_chunk_usage returned %08x\n", Status);
            goto end;
        }

        if (!(Vcb->superblock.compat_ro_flags & BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE)) {
            if (!no_cache) {
                Status = allocate_cache(Vcb, &cache_changed, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    WARN("allocate_cache returned %08x\n", Status);
                    no_cache = TRUE;
                    cache_changed = FALSE;
                }
            }
        } else {
            Status = update_chunk_caches_tree(Vcb, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("update_chunk_caches_tree returned %08x\n", Status);
                goto end;
            }
        }

#ifdef DEBUG_WRITE_LOOPS
        loops++;

        if (cache_changed)
            ERR("cache has changed, looping again\n");
#endif
    } while (cache_changed || !trees_consistent(Vcb));

#ifdef DEBUG_WRITE_LOOPS
    ERR("%u loops\n", loops);
#endif

    TRACE("trees consistent\n");

    Status = update_root_root(Vcb, no_cache, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_root_root returned %08x\n", Status);
        goto end;
    }

    Status = write_trees(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("write_trees returned %08x\n", Status);
        goto end;
    }

    Status = test_not_full(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("test_not_full returned %08x\n", Status);
        goto end;
    }

#ifdef DEBUG_PARANOID
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        KEY searchkey;
        traverse_ptr tp;

        searchkey.obj_id = t->header.address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            searchkey.obj_id = t->header.address;
            searchkey.obj_type = TYPE_EXTENT_ITEM;
            searchkey.offset = 0xffffffffffffffff;

            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }

            if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                ERR("error - could not find entry in extent tree for tree at %llx\n", t->header.address);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
        }

        le = le->Flink;
    }
#endif

    Vcb->superblock.cache_generation = Vcb->superblock.generation;

    if (!Vcb->options.no_barrier)
        flush_disk_caches(Vcb);

    Status = write_superblocks(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("write_superblocks returned %08x\n", Status);
        goto end;
    }

    vde = Vcb->vde;

    if (vde) {
        pdo_device_extension* pdode = vde->pdode;

        ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

        le = pdode->children.Flink;

        while (le != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

            vc->generation = Vcb->superblock.generation;
            le = le->Flink;
        }

        ExReleaseResourceLite(&pdode->child_lock);
    }

    clean_space_cache(Vcb);

    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);

        c->changed = FALSE;
        c->space_changed = FALSE;

        le = le->Flink;
    }

    Vcb->superblock.generation++;

    Status = STATUS_SUCCESS;

    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);

        t->write = FALSE;

        le = le->Flink;
    }

    Vcb->need_write = FALSE;

    while (!IsListEmpty(&Vcb->drop_roots)) {
        root* r = CONTAINING_RECORD(RemoveHeadList(&Vcb->drop_roots), root, list_entry);

        ExDeleteResourceLite(&r->nonpaged->load_tree_lock);
        ExFreePool(r->nonpaged);
        ExFreePool(r);
    }

end:
    TRACE("do_write returning %08x\n", Status);

    return Status;
}

NTSTATUS do_write(device_extension* Vcb, PIRP Irp) {
    LIST_ENTRY rollback;
    NTSTATUS Status;

    InitializeListHead(&rollback);

    Status = do_write2(Vcb, Irp, &rollback);

    if (!NT_SUCCESS(Status)) {
        ERR("do_write2 returned %08x, dropping into readonly mode\n", Status);
        Vcb->readonly = TRUE;
        FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_FORCED_CLOSED);
        do_rollback(Vcb, &rollback);
    } else
        clear_rollback(&rollback);

    return Status;
}

#ifdef DEBUG_STATS
static void print_stats(device_extension* Vcb) {
    LARGE_INTEGER freq;

    ERR("READ STATS:\n");
    ERR("number of reads: %llu\n", Vcb->stats.num_reads);
    ERR("data read: %llu bytes\n", Vcb->stats.data_read);
    ERR("total time taken: %llu\n", Vcb->stats.read_total_time);
    ERR("csum time taken: %llu\n", Vcb->stats.read_csum_time);
    ERR("disk time taken: %llu\n", Vcb->stats.read_disk_time);
    ERR("other time taken: %llu\n", Vcb->stats.read_total_time - Vcb->stats.read_csum_time - Vcb->stats.read_disk_time);

    KeQueryPerformanceCounter(&freq);

    ERR("OPEN STATS (freq = %llu):\n", freq.QuadPart);
    ERR("number of opens: %llu\n", Vcb->stats.num_opens);
    ERR("total time taken: %llu\n", Vcb->stats.open_total_time);
    ERR("number of overwrites: %llu\n", Vcb->stats.num_overwrites);
    ERR("total time taken: %llu\n", Vcb->stats.overwrite_total_time);
    ERR("number of creates: %llu\n", Vcb->stats.num_creates);
    ERR("calls to open_fcb: %llu\n", Vcb->stats.open_fcb_calls);
    ERR("time spent in open_fcb: %llu\n", Vcb->stats.open_fcb_time);
    ERR("calls to open_fileref_child: %llu\n", Vcb->stats.open_fileref_child_calls);
    ERR("time spent in open_fileref_child: %llu\n", Vcb->stats.open_fileref_child_time);
    ERR("time spent waiting for fcb_lock: %llu\n", Vcb->stats.fcb_lock_time);
    ERR("total time taken: %llu\n", Vcb->stats.create_total_time);

    RtlZeroMemory(&Vcb->stats, sizeof(debug_stats));
}
#endif

static void do_flush(device_extension* Vcb) {
    NTSTATUS Status;

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

#ifdef DEBUG_STATS
    print_stats(Vcb);
#endif

    if (Vcb->need_write && !Vcb->readonly)
        Status = do_write(Vcb, NULL);
    else
        Status = STATUS_SUCCESS;

    free_trees(Vcb);

    if (!NT_SUCCESS(Status))
        ERR("do_write returned %08x\n", Status);

    ExReleaseResourceLite(&Vcb->tree_lock);
}

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI flush_thread(void* context) {
#else
void flush_thread(void* context) {
#endif
    DEVICE_OBJECT* devobj = context;
    device_extension* Vcb = devobj->DeviceExtension;
    LARGE_INTEGER due_time;

    ObReferenceObject(devobj);

    KeInitializeTimer(&Vcb->flush_thread_timer);

    due_time.QuadPart = (UINT64)Vcb->options.flush_interval * -10000000;

    KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);

    while (TRUE) {
        KeWaitForSingleObject(&Vcb->flush_thread_timer, Executive, KernelMode, FALSE, NULL);

        if (!(devobj->Vpb->Flags & VPB_MOUNTED) || Vcb->removing)
            break;

        if (!Vcb->locked)
            do_flush(Vcb);

        KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);
    }

    ObDereferenceObject(devobj);
    KeCancelTimer(&Vcb->flush_thread_timer);

    KeSetEvent(&Vcb->flush_thread_finished, 0, FALSE);

    PsTerminateSystemThread(STATUS_SUCCESS);
}
