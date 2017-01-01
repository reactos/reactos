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

static NTSTATUS STDCALL pnp_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    pnp_stripe* stripe = conptr;
    pnp_context* context = (pnp_context*)stripe->context;
    
    stripe->Status = Irp->IoStatus.Status;
    
    InterlockedDecrement(&context->left);
    
    if (context->left == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS send_disks_pnp_message(device_extension* Vcb, UCHAR minor) {
    pnp_context* context;
    UINT64 num_devices, i;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(pnp_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE);
    
    RtlZeroMemory(context, sizeof(pnp_context));
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    num_devices = Vcb->superblock.num_devices;
    
    context->stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(pnp_stripe) * num_devices, ALLOC_TAG);
    if (!context->stripes) {
        ERR("out of memory\n");
        ExFreePool(context);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end2;
    }
    
    RtlZeroMemory(context->stripes, sizeof(pnp_stripe) * num_devices);
    
    i = 0;
    le = Vcb->devices.Flink;
    
    while (le != &Vcb->devices) {
        PIO_STACK_LOCATION IrpSp;
        device* dev = CONTAINING_RECORD(le, device, list_entry);
        
        if (dev->devobj) {
            context->stripes[i].context = (struct pnp_context*)context;

            context->stripes[i].Irp = IoAllocateIrp(dev->devobj->StackSize, FALSE);
            
            if (!context->stripes[i].Irp) {
                UINT64 j;
                
                ERR("IoAllocateIrp failed\n");
                
                for (j = 0; j < i; j++) {
                    if (context->stripes[j].dev->devobj) {
                        IoFreeIrp(context->stripes[j].Irp);
                    }
                }
                ExFreePool(context->stripes);
                ExFreePool(context);
                
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end2;
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_PNP;
            IrpSp->MinorFunction = minor;

            context->stripes[i].Irp->UserIosb = &context->stripes[i].iosb;
            
            IoSetCompletionRoutine(context->stripes[i].Irp, pnp_completion, &context->stripes[i], TRUE, TRUE, TRUE);
            
            context->stripes[i].Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            context->stripes[i].dev = dev;
            
            context->left++;
        }
        
        le = le->Flink;
    }
    
    if (context->left == 0) {
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    for (i = 0; i < num_devices; i++) {
        if (context->stripes[i].Irp) {
            IoCallDriver(context->stripes[i].dev->devobj, context->stripes[i].Irp);
        }
    }
    
    KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
    
    Status = STATUS_SUCCESS;
    
    for (i = 0; i < num_devices; i++) {
        if (context->stripes[i].Irp) {
            if (context->stripes[i].Status != STATUS_SUCCESS)
                Status = context->stripes[i].Status;
        }
    }
    
end:
    for (i = 0; i < num_devices; i++) {
        if (context->stripes[i].Irp) {
            IoFreeIrp(context->stripes[i].Irp);
        }
    }

    ExFreePool(context->stripes);
    ExFreePool(context);
    
end2:
    ExReleaseResourceLite(&Vcb->tree_lock);

    return Status;
}

static NTSTATUS pnp_cancel_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);

    if (Vcb->root_fileref && Vcb->root_fileref->fcb && (Vcb->root_fileref->open_count > 0 || has_open_children(Vcb->root_fileref))) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    Status = send_disks_pnp_message(Vcb, IRP_MN_CANCEL_REMOVE_DEVICE);
    if (!NT_SUCCESS(Status)) {
        WARN("send_disks_pnp_message returned %08x\n", Status);
        goto end;
    }

    Vcb->removing = FALSE;
end:
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    return STATUS_SUCCESS;
}

static NTSTATUS pnp_query_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    LIST_ENTRY rollback;
    
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);

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
    
    InitializeListHead(&rollback);
    
    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, Irp, &rollback);
    
    clear_rollback(Vcb, &rollback);

    ExReleaseResourceLite(&Vcb->tree_lock);

    Status = STATUS_SUCCESS;
end:
    ExReleaseResourceLite(&Vcb->fcb_lock);
    
    return Status;
}

static NTSTATUS pnp_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    
    Status = send_disks_pnp_message(Vcb, IRP_MN_REMOVE_DEVICE);
    if (!NT_SUCCESS(Status)) {
        WARN("send_disks_pnp_message returned %08x\n", Status);
    }
    
    if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {
        Status = FsRtlNotifyVolumeEvent(Vcb->root_file, FSRTL_VOLUME_DISMOUNT);
        if (!NT_SUCCESS(Status)) {
            WARN("FsRtlNotifyVolumeEvent returned %08x\n", Status);
        }
        
        if (Vcb->open_files > 0) {
            Vcb->removing = TRUE;
            Vcb->Vpb->Flags &= ~VPB_MOUNTED;
        } else
            uninit(Vcb, FALSE);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS pnp_start_device(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    FIXME("STUB\n");

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pnp_surprise_removal(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    
    TRACE("(%p, %p)\n", DeviceObject, Irp);
    
    if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {
        if (Vcb->open_files > 0) {
            Vcb->removing = TRUE;
            Vcb->Vpb->Flags &= ~VPB_MOUNTED;
        } else
            uninit(Vcb, FALSE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL drv_pnp(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto end;
    }
    
    Status = STATUS_NOT_IMPLEMENTED;
    
    switch (IrpSp->MinorFunction) {
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = pnp_cancel_remove_device(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = pnp_query_remove_device(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            Status = pnp_remove_device(DeviceObject, Irp);
            break;

        case IRP_MN_START_DEVICE:
            Status = pnp_start_device(DeviceObject, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            Status = pnp_surprise_removal(DeviceObject, Irp);
            break;

        default:
            TRACE("passing minor function 0x%x on\n", IrpSp->MinorFunction);
            
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);
            goto end;
    }

// //     Irp->IoStatus.Status = Status;
// //     Irp->IoStatus.Information = 0;
// 
//     IoSkipCurrentIrpStackLocation(Irp);
//     
//     Status = IoCallDriver(first_device(Vcb)->devobj, Irp);
// 
// //     IoCompleteRequest(Irp, IO_NO_INCREMENT);

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
end:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
