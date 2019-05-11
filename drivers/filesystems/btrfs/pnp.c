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

struct pnp_context;

typedef struct {
    struct pnp_context* context;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    device* dev;
} pnp_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    LONG left;
    pnp_stripe* stripes;
} pnp_context;

extern ERESOURCE pdo_list_lock;
extern LIST_ENTRY pdo_list;

_Function_class_(IO_COMPLETION_ROUTINE)
#ifdef __REACTOS__
static NTSTATUS NTAPI pnp_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS pnp_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    pnp_stripe* stripe = conptr;
    pnp_context* context = (pnp_context*)stripe->context;

    UNUSED(DeviceObject);

    stripe->Status = Irp->IoStatus.Status;

    InterlockedDecrement(&context->left);

    if (context->left == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS send_disks_pnp_message(device_extension* Vcb, UCHAR minor) {
    pnp_context context;
    ULONG num_devices, i;
    NTSTATUS Status;
    LIST_ENTRY* le;

    RtlZeroMemory(&context, sizeof(pnp_context));
    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);

    num_devices = (ULONG)min(0xffffffff, Vcb->superblock.num_devices);

    context.stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(pnp_stripe) * num_devices, ALLOC_TAG);
    if (!context.stripes) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(context.stripes, sizeof(pnp_stripe) * num_devices);

    i = 0;
    le = Vcb->devices.Flink;

    while (le != &Vcb->devices) {
        PIO_STACK_LOCATION IrpSp;
        device* dev = CONTAINING_RECORD(le, device, list_entry);

        if (dev->devobj) {
            context.stripes[i].context = (struct pnp_context*)&context;

            context.stripes[i].Irp = IoAllocateIrp(dev->devobj->StackSize, FALSE);

            if (!context.stripes[i].Irp) {
                UINT64 j;

                ERR("IoAllocateIrp failed\n");

                for (j = 0; j < i; j++) {
                    if (context.stripes[j].dev->devobj) {
                        IoFreeIrp(context.stripes[j].Irp);
                    }
                }
                ExFreePool(context.stripes);

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            IrpSp = IoGetNextIrpStackLocation(context.stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_PNP;
            IrpSp->MinorFunction = minor;

            context.stripes[i].Irp->UserIosb = &context.stripes[i].iosb;

            IoSetCompletionRoutine(context.stripes[i].Irp, pnp_completion, &context.stripes[i], TRUE, TRUE, TRUE);

            context.stripes[i].Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            context.stripes[i].dev = dev;

            context.left++;
        }

        le = le->Flink;
    }

    if (context.left == 0) {
        Status = STATUS_SUCCESS;
        goto end;
    }

    for (i = 0; i < num_devices; i++) {
        if (context.stripes[i].Irp) {
            IoCallDriver(context.stripes[i].dev->devobj, context.stripes[i].Irp);
        }
    }

    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

    Status = STATUS_SUCCESS;

    for (i = 0; i < num_devices; i++) {
        if (context.stripes[i].Irp) {
            if (context.stripes[i].Status != STATUS_SUCCESS)
                Status = context.stripes[i].Status;
        }
    }

end:
    for (i = 0; i < num_devices; i++) {
        if (context.stripes[i].Irp) {
            IoFreeIrp(context.stripes[i].Irp);
        }
    }

    ExFreePool(context.stripes);

    return Status;
}

static NTSTATUS pnp_cancel_remove_device(PDEVICE_OBJECT DeviceObject) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

    ExAcquireResourceExclusiveLite(&Vcb->fileref_lock, TRUE);

    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Status = send_disks_pnp_message(Vcb, IRP_MN_CANCEL_REMOVE_DEVICE);
    if (!NT_SUCCESS(Status)) {
        WARN("send_disks_pnp_message returned %08x\n", Status);
        goto end;
    }

end:
    ExReleaseResourceLite(&Vcb->fileref_lock);
    ExReleaseResourceLite(&Vcb->tree_lock);

    return STATUS_SUCCESS;
}

NTSTATUS pnp_query_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Status = send_disks_pnp_message(Vcb, IRP_MN_QUERY_REMOVE_DEVICE);
    if (!NT_SUCCESS(Status)) {
        WARN("send_disks_pnp_message returned %08x\n", Status);
        goto end;
    }

    Vcb->removing = TRUE;

    if (Vcb->need_write && !Vcb->readonly) {
        Status = do_write(Vcb, Irp);

        free_trees(Vcb);

        if (!NT_SUCCESS(Status)) {
            ERR("do_write returned %08x\n", Status);
            goto end;
        }
    }


    Status = STATUS_SUCCESS;
end:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS pnp_remove_device(PDEVICE_OBJECT DeviceObject) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);

    Status = send_disks_pnp_message(Vcb, IRP_MN_REMOVE_DEVICE);

    if (!NT_SUCCESS(Status))
        WARN("send_disks_pnp_message returned %08x\n", Status);

    ExReleaseResourceLite(&Vcb->tree_lock);

    if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {
        Status = FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_DISMOUNT);
        if (!NT_SUCCESS(Status)) {
            WARN("FsRtlNotifyVolumeEvent returned %08x\n", Status);
        }

        if (Vcb->vde)
            Vcb->vde->mounted_device = NULL;

        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);
        Vcb->removing = TRUE;
        ExReleaseResourceLite(&Vcb->tree_lock);

        if (Vcb->open_files == 0)
            uninit(Vcb);
    }

    return STATUS_SUCCESS;
}

NTSTATUS pnp_surprise_removal(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

        if (Vcb->vde)
            Vcb->vde->mounted_device = NULL;

        Vcb->removing = TRUE;

        ExReleaseResourceLite(&Vcb->tree_lock);

        if (Vcb->open_files == 0)
            uninit(Vcb);
    }

    return STATUS_SUCCESS;
}

static void bus_query_capabilities(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES dc = IrpSp->Parameters.DeviceCapabilities.Capabilities;

    dc->UniqueID = TRUE;
    dc->SilentInstall = TRUE;

    Irp->IoStatus.Status = STATUS_SUCCESS;
}

static NTSTATUS bus_query_device_relations(PIRP Irp) {
    NTSTATUS Status;
    ULONG num_children;
    LIST_ENTRY* le;
    ULONG drsize, i;
    DEVICE_RELATIONS* dr;

    ExAcquireResourceSharedLite(&pdo_list_lock, TRUE);

    num_children = 0;

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        num_children++;

        le = le->Flink;
    }

    drsize = offsetof(DEVICE_RELATIONS, Objects[0]) + (num_children * sizeof(PDEVICE_OBJECT));
    dr = ExAllocatePoolWithTag(PagedPool, drsize, ALLOC_TAG);

    if (!dr) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    dr->Count = num_children;

    i = 0;
    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

        ObReferenceObject(pdode->pdo);
        dr->Objects[i] = pdode->pdo;
        i++;

        le = le->Flink;
    }

    Irp->IoStatus.Information = (ULONG_PTR)dr;

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&pdo_list_lock);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static NTSTATUS bus_query_hardware_ids(PIRP Irp) {
    WCHAR* out;

    static const WCHAR ids[] = L"ROOT\\btrfs\0";

    out = ExAllocatePoolWithTag(PagedPool, sizeof(ids), ALLOC_TAG);
    if (!out) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(out, ids, sizeof(ids));

    Irp->IoStatus.Information = (ULONG_PTR)out;

    return STATUS_SUCCESS;
}

static NTSTATUS bus_pnp(control_device_extension* cde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction) {
        case IRP_MN_QUERY_CAPABILITIES:
            bus_query_capabilities(Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations || no_pnp)
                break;

            return bus_query_device_relations(Irp);

        case IRP_MN_QUERY_ID:
        {
            NTSTATUS Status;

            if (IrpSp->Parameters.QueryId.IdType != BusQueryHardwareIDs)
                break;

            Status = bus_query_hardware_ids(Irp);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return Status;
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(cde->attached_device, Irp);
}

static NTSTATUS pdo_query_device_id(pdo_device_extension* pdode, PIRP Irp) {
    WCHAR name[100], *noff, *out;
    int i;

    static const WCHAR pref[] = L"Btrfs\\";

    RtlCopyMemory(name, pref, sizeof(pref) - sizeof(WCHAR));

    noff = &name[(sizeof(pref) / sizeof(WCHAR)) - 1];
    for (i = 0; i < 16; i++) {
        *noff = hex_digit(pdode->uuid.uuid[i] >> 4); noff++;
        *noff = hex_digit(pdode->uuid.uuid[i] & 0xf); noff++;

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *noff = '-';
            noff++;
        }
    }
    *noff = 0;

    out = ExAllocatePoolWithTag(PagedPool, (wcslen(name) + 1) * sizeof(WCHAR), ALLOC_TAG);
    if (!out) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(out, name, (wcslen(name) + 1) * sizeof(WCHAR));

    Irp->IoStatus.Information = (ULONG_PTR)out;

    return STATUS_SUCCESS;
}

static NTSTATUS pdo_query_hardware_ids(PIRP Irp) {
    WCHAR* out;

    static const WCHAR ids[] = L"BtrfsVolume\0";

    out = ExAllocatePoolWithTag(PagedPool, sizeof(ids), ALLOC_TAG);
    if (!out) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(out, ids, sizeof(ids));

    Irp->IoStatus.Information = (ULONG_PTR)out;

    return STATUS_SUCCESS;
}

static NTSTATUS pdo_query_id(pdo_device_extension* pdode, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->Parameters.QueryId.IdType) {
        case BusQueryDeviceID:
            TRACE("BusQueryDeviceID\n");
            return pdo_query_device_id(pdode, Irp);

        case BusQueryHardwareIDs:
            TRACE("BusQueryHardwareIDs\n");
            return pdo_query_hardware_ids(Irp);

        default:
            break;
    }

    return Irp->IoStatus.Status;
}

static NTSTATUS pdo_pnp(PDEVICE_OBJECT pdo, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    pdo_device_extension* pdode = pdo->DeviceExtension;

    switch (IrpSp->MinorFunction) {
        case IRP_MN_QUERY_ID:
            return pdo_query_id(pdode, Irp);

        case IRP_MN_START_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            return STATUS_UNSUCCESSFUL;
    }

    return Irp->IoStatus.Status;
}

_Dispatch_type_(IRP_MJ_PNP)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_pnp(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_CONTROL) {
        Status = bus_pnp(DeviceObject->DeviceExtension, Irp);
        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        volume_device_extension* vde = DeviceObject->DeviceExtension;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(vde->pdo, Irp);
        goto exit;
    } else if (Vcb && Vcb->type == VCB_TYPE_PDO) {
        Status = pdo_pnp(DeviceObject, Irp);
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_NOT_IMPLEMENTED;

    switch (IrpSp->MinorFunction) {
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = pnp_cancel_remove_device(DeviceObject);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = pnp_query_remove_device(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            Status = pnp_remove_device(DeviceObject);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            Status = pnp_surprise_removal(DeviceObject, Irp);
            break;

        default:
            TRACE("passing minor function 0x%x on\n", IrpSp->MinorFunction);

            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);
            goto exit;
    }

end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

exit:
    TRACE("returning %08x\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
