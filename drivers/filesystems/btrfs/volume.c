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
#include <mountdev.h>
#include <ntddvol.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <wdmguid.h>

#define IOCTL_VOLUME_IS_DYNAMIC     CTL_CODE(IOCTL_VOLUME_BASE, 18, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VOLUME_POST_ONLINE    CTL_CODE(IOCTL_VOLUME_BASE, 25, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

extern PDRIVER_OBJECT drvobj;
extern PDEVICE_OBJECT master_devobj;
extern PDEVICE_OBJECT busobj;
extern ERESOURCE pdo_list_lock;
extern LIST_ENTRY pdo_list;
extern UNICODE_STRING registry_path;
extern tIoUnregisterPlugPlayNotificationEx fIoUnregisterPlugPlayNotificationEx;

NTSTATUS vol_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    if (vde->removing)
        return STATUS_DEVICE_NOT_READY;

    Irp->IoStatus.Information = FILE_OPENED;
    InterlockedIncrement(&vde->open_count);

    return STATUS_SUCCESS;
}

void free_vol(volume_device_extension* vde) {
    PDEVICE_OBJECT pdo;

    vde->dead = true;

    if (vde->mounted_device) {
        device_extension* Vcb = vde->mounted_device->DeviceExtension;

        Vcb->vde = NULL;
    }

    if (vde->name.Buffer)
        ExFreePool(vde->name.Buffer);

    ExDeleteResourceLite(&vde->pdode->child_lock);

    if (vde->pdo->AttachedDevice)
        IoDetachDevice(vde->pdo);

    while (!IsListEmpty(&vde->pdode->children)) {
        volume_child* vc = CONTAINING_RECORD(RemoveHeadList(&vde->pdode->children), volume_child, list_entry);

        if (vc->notification_entry) {
            if (fIoUnregisterPlugPlayNotificationEx)
                fIoUnregisterPlugPlayNotificationEx(vc->notification_entry);
            else
                IoUnregisterPlugPlayNotification(vc->notification_entry);
        }

        if (vc->pnp_name.Buffer)
            ExFreePool(vc->pnp_name.Buffer);

        ExFreePool(vc);
    }

    if (no_pnp)
        ExFreePool(vde->pdode);

    pdo = vde->pdo;
    IoDeleteDevice(vde->device);

    if (!no_pnp)
        IoDeleteDevice(pdo);
}

NTSTATUS vol_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    pdo_device_extension* pdode = vde->pdode;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;

    if (vde->dead)
        return STATUS_SUCCESS;

    ExAcquireResourceExclusiveLite(&pdo_list_lock, true);

    if (vde->dead) {
        ExReleaseResourceLite(&pdo_list_lock);
        return STATUS_SUCCESS;
    }

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (InterlockedDecrement(&vde->open_count) == 0 && vde->removing) {
        ExReleaseResourceLite(&pdode->child_lock);

        free_vol(vde);
    } else
        ExReleaseResourceLite(&pdode->child_lock);

    ExReleaseResourceLite(&pdo_list_lock);

    return STATUS_SUCCESS;
}

typedef struct {
    IO_STATUS_BLOCK iosb;
    KEVENT Event;
} vol_read_context;

_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall vol_read_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    vol_read_context* context = conptr;

    UNUSED(DeviceObject);

    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS vol_read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    pdo_device_extension* pdode = vde->pdode;
    volume_child* vc;
    NTSTATUS Status;
    PIRP Irp2;
    vol_read_context context;
    PIO_STACK_LOCATION IrpSp, IrpSp2;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (IsListEmpty(&pdode->children)) {
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    // We can't use IoSkipCurrentIrpStackLocation as the device isn't in our stack

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, false);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IRP_MJ_READ;
    IrpSp2->FileObject = vc->fileobj;

    if (vc->devobj->Flags & DO_BUFFERED_IO) {
        Irp2->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, IrpSp->Parameters.Read.Length, ALLOC_TAG);
        if (!Irp2->AssociatedIrp.SystemBuffer) {
            ERR("out of memory\n");
            ExReleaseResourceLite(&pdode->child_lock);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        Irp2->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;

        Irp2->UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    } else if (vc->devobj->Flags & DO_DIRECT_IO)
        Irp2->MdlAddress = Irp->MdlAddress;
    else
        Irp2->UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

    IrpSp2->Parameters.Read.Length = IrpSp->Parameters.Read.Length;
    IrpSp2->Parameters.Read.ByteOffset.QuadPart = IrpSp->Parameters.Read.ByteOffset.QuadPart;

    KeInitializeEvent(&context.Event, NotificationEvent, false);
    Irp2->UserIosb = &context.iosb;

    IoSetCompletionRoutine(Irp2, vol_read_completion, &context, true, true, true);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, false, NULL);
        Status = context.iosb.Status;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    Irp->IoStatus.Information = context.iosb.Information;

end:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS vol_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    pdo_device_extension* pdode = vde->pdode;
    volume_child* vc;
    NTSTATUS Status;
    PIRP Irp2;
    vol_read_context context;
    PIO_STACK_LOCATION IrpSp, IrpSp2;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (IsListEmpty(&pdode->children)) {
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    if (vc->list_entry.Flink != &pdode->children) { // more than once device
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    // We can't use IoSkipCurrentIrpStackLocation as the device isn't in our stack

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, false);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IRP_MJ_WRITE;
    IrpSp2->FileObject = vc->fileobj;

    if (vc->devobj->Flags & DO_BUFFERED_IO) {
        Irp2->AssociatedIrp.SystemBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        Irp2->Flags |= IRP_BUFFERED_IO;

        Irp2->UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    } else if (vc->devobj->Flags & DO_DIRECT_IO)
        Irp2->MdlAddress = Irp->MdlAddress;
    else
        Irp2->UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

    IrpSp2->Parameters.Write.Length = IrpSp->Parameters.Write.Length;
    IrpSp2->Parameters.Write.ByteOffset.QuadPart = IrpSp->Parameters.Write.ByteOffset.QuadPart;

    KeInitializeEvent(&context.Event, NotificationEvent, false);
    Irp2->UserIosb = &context.iosb;

    IoSetCompletionRoutine(Irp2, vol_read_completion, &context, true, true, true);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, false, NULL);
        Status = context.iosb.Status;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    Irp->IoStatus.Information = context.iosb.Information;

end:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static NTSTATUS vol_query_device_name(volume_device_extension* vde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_NAME name;

    if (IrpSp->FileObject && IrpSp->FileObject->FsContext)
        return STATUS_INVALID_PARAMETER;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME)) {
        Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
        return STATUS_BUFFER_TOO_SMALL;
    }

    name = Irp->AssociatedIrp.SystemBuffer;
    name->NameLength = vde->name.Length;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < offsetof(MOUNTDEV_NAME, Name[0]) + name->NameLength) {
        Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(name->Name, vde->name.Buffer, vde->name.Length);

    Irp->IoStatus.Information = offsetof(MOUNTDEV_NAME, Name[0]) + name->NameLength;

    return STATUS_SUCCESS;
}

static NTSTATUS vol_query_unique_id(volume_device_extension* vde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    MOUNTDEV_UNIQUE_ID* mduid;
    pdo_device_extension* pdode;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_UNIQUE_ID)) {
        Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
        return STATUS_BUFFER_TOO_SMALL;
    }

    mduid = Irp->AssociatedIrp.SystemBuffer;
    mduid->UniqueIdLength = sizeof(BTRFS_UUID);

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < offsetof(MOUNTDEV_UNIQUE_ID, UniqueId[0]) + mduid->UniqueIdLength) {
        Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (!vde->pdo)
        return STATUS_INVALID_PARAMETER;

    pdode = vde->pdode;

    RtlCopyMemory(mduid->UniqueId, &pdode->uuid, sizeof(BTRFS_UUID));

    Irp->IoStatus.Information = offsetof(MOUNTDEV_UNIQUE_ID, UniqueId[0]) + mduid->UniqueIdLength;

    return STATUS_SUCCESS;
}

static NTSTATUS vol_is_dynamic(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    uint8_t* buf;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0 || !Irp->AssociatedIrp.SystemBuffer)
        return STATUS_INVALID_PARAMETER;

    buf = (uint8_t*)Irp->AssociatedIrp.SystemBuffer;

    *buf = 1;

    Irp->IoStatus.Information = 1;

    return STATUS_SUCCESS;
}

static NTSTATUS vol_check_verify(volume_device_extension* vde) {
    pdo_device_extension* pdode = vde->pdode;
    NTSTATUS Status;
    LIST_ENTRY* le;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        Status = dev_ioctl(vc->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, false, NULL);
        if (!NT_SUCCESS(Status))
            goto end;

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&pdode->child_lock);

    return Status;
}

static NTSTATUS vol_get_disk_extents(volume_device_extension* vde, PIRP Irp) {
    pdo_device_extension* pdode = vde->pdode;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    LIST_ENTRY* le;
    ULONG num_extents = 0, i, max_extents = 1;
    NTSTATUS Status;
    VOLUME_DISK_EXTENTS *ext, *ext3;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_DISK_EXTENTS))
        return STATUS_BUFFER_TOO_SMALL;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);
        VOLUME_DISK_EXTENTS ext2;

        Status = dev_ioctl(vc->devobj, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &ext2, sizeof(VOLUME_DISK_EXTENTS), false, NULL);
        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            ERR("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS returned %08lx\n", Status);
            goto end;
        }

        num_extents += ext2.NumberOfDiskExtents;

        if (ext2.NumberOfDiskExtents > max_extents)
            max_extents = ext2.NumberOfDiskExtents;

        le = le->Flink;
    }

    ext = Irp->AssociatedIrp.SystemBuffer;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < offsetof(VOLUME_DISK_EXTENTS, Extents[0]) + (num_extents * sizeof(DISK_EXTENT))) {
        Irp->IoStatus.Information = offsetof(VOLUME_DISK_EXTENTS, Extents[0]);
        ext->NumberOfDiskExtents = num_extents;
        Status = STATUS_BUFFER_OVERFLOW;
        goto end;
    }

    ext3 = ExAllocatePoolWithTag(PagedPool, offsetof(VOLUME_DISK_EXTENTS, Extents[0]) + (max_extents * sizeof(DISK_EXTENT)), ALLOC_TAG);
    if (!ext3) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    i = 0;
    ext->NumberOfDiskExtents = 0;

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        Status = dev_ioctl(vc->devobj, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, ext3,
                           (ULONG)offsetof(VOLUME_DISK_EXTENTS, Extents[0]) + (max_extents * sizeof(DISK_EXTENT)), false, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS returned %08lx\n", Status);
            ExFreePool(ext3);
            goto end;
        }

        if (i + ext3->NumberOfDiskExtents > num_extents) {
            Irp->IoStatus.Information = offsetof(VOLUME_DISK_EXTENTS, Extents[0]);
            ext->NumberOfDiskExtents = i + ext3->NumberOfDiskExtents;
            Status = STATUS_BUFFER_OVERFLOW;
            ExFreePool(ext3);
            goto end;
        }

        RtlCopyMemory(&ext->Extents[i], ext3->Extents, sizeof(DISK_EXTENT) * ext3->NumberOfDiskExtents);
        i += ext3->NumberOfDiskExtents;

        le = le->Flink;
    }

    ExFreePool(ext3);

    Status = STATUS_SUCCESS;

    ext->NumberOfDiskExtents = i;
    Irp->IoStatus.Information = offsetof(VOLUME_DISK_EXTENTS, Extents[0]) + (i * sizeof(DISK_EXTENT));

end:
    ExReleaseResourceLite(&pdode->child_lock);

    return Status;
}

static NTSTATUS vol_is_writable(volume_device_extension* vde) {
    pdo_device_extension* pdode = vde->pdode;
    NTSTATUS Status;
    LIST_ENTRY* le;
    bool writable = false;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        Status = dev_ioctl(vc->devobj, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, true, NULL);

        if (NT_SUCCESS(Status)) {
            writable = true;
            break;
        } else if (Status != STATUS_MEDIA_WRITE_PROTECTED)
            goto end;

        le = le->Flink;
    }

    Status = writable ? STATUS_SUCCESS : STATUS_MEDIA_WRITE_PROTECTED;

end:
ExReleaseResourceLite(&pdode->child_lock);

    return STATUS_SUCCESS;
}

static NTSTATUS vol_get_length(volume_device_extension* vde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    pdo_device_extension* pdode = vde->pdode;
    GET_LENGTH_INFORMATION* gli;
    LIST_ENTRY* le;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    gli = (GET_LENGTH_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;

    gli->Length.QuadPart = 0;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        gli->Length.QuadPart += vc->size;

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS vol_get_drive_geometry(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    pdo_device_extension* pdode = vde->pdode;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    DISK_GEOMETRY* geom;
    uint64_t length;
    LIST_ENTRY* le;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
        return STATUS_BUFFER_TOO_SMALL;

    length = 0;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        length += vc->size;

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    geom = (DISK_GEOMETRY*)Irp->AssociatedIrp.SystemBuffer;
    geom->BytesPerSector = DeviceObject->SectorSize == 0 ? 0x200 : DeviceObject->SectorSize;
    geom->SectorsPerTrack = 0x3f;
    geom->TracksPerCylinder = 0xff;
    geom->Cylinders.QuadPart = length / (UInt32x32To64(geom->TracksPerCylinder, geom->SectorsPerTrack) * geom->BytesPerSector);
    geom->MediaType = DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA ? RemovableMedia : FixedMedia;

    Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

    return STATUS_SUCCESS;
}

static NTSTATUS vol_get_gpt_attributes(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION* vggai;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    vggai = (VOLUME_GET_GPT_ATTRIBUTES_INFORMATION*)Irp->AssociatedIrp.SystemBuffer;

    vggai->GptAttributes = 0;

    Irp->IoStatus.Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);

    return STATUS_SUCCESS;
}

static NTSTATUS vol_get_device_number(volume_device_extension* vde, PIRP Irp) {
    pdo_device_extension* pdode = vde->pdode;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    volume_child* vc;
    STORAGE_DEVICE_NUMBER* sdn;

    // If only one device, return its disk number. This is needed for ejection to work.

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DEVICE_NUMBER))
        return STATUS_BUFFER_TOO_SMALL;

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (IsListEmpty(&pdode->children) || pdode->num_children > 1) {
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    if (vc->disk_num == 0xffffffff) {
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    sdn = (STORAGE_DEVICE_NUMBER*)Irp->AssociatedIrp.SystemBuffer;

    sdn->DeviceType = FILE_DEVICE_DISK;
    sdn->DeviceNumber = vc->disk_num;
    sdn->PartitionNumber = vc->part_num;

    ExReleaseResourceLite(&pdode->child_lock);

    Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);

    return STATUS_SUCCESS;
}

_Function_class_(IO_COMPLETION_ROUTINE)
static NTSTATUS __stdcall vol_ioctl_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    KEVENT* event = conptr;

    UNUSED(DeviceObject);
    UNUSED(Irp);

    KeSetEvent(event, 0, false);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS vol_ioctl_passthrough(volume_device_extension* vde, PIRP Irp) {
    NTSTATUS Status;
    volume_child* vc;
    PIRP Irp2;
    PIO_STACK_LOCATION IrpSp, IrpSp2;
    KEVENT Event;
    pdo_device_extension* pdode = vde->pdode;

    TRACE("(%p, %p)\n", vde, Irp);

    ExAcquireResourceSharedLite(&pdode->child_lock, true);

    if (IsListEmpty(&pdode->children)) {
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    if (vc->list_entry.Flink != &pdode->children) { // more than one device
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, false);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IrpSp->MajorFunction;
    IrpSp2->MinorFunction = IrpSp->MinorFunction;
    IrpSp2->FileObject = vc->fileobj;

    IrpSp2->Parameters.DeviceIoControl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    IrpSp2->Parameters.DeviceIoControl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    IrpSp2->Parameters.DeviceIoControl.IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    IrpSp2->Parameters.DeviceIoControl.Type3InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    Irp2->AssociatedIrp.SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    Irp2->MdlAddress = Irp->MdlAddress;
    Irp2->UserBuffer = Irp->UserBuffer;
    Irp2->Flags = Irp->Flags;

    KeInitializeEvent(&Event, NotificationEvent, false);

    IoSetCompletionRoutine(Irp2, vol_ioctl_completion, &Event, true, true, true);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, false, NULL);
        Status = Irp2->IoStatus.Status;
    }

    Irp->IoStatus.Status = Irp2->IoStatus.Status;
    Irp->IoStatus.Information = Irp2->IoStatus.Information;

    ExReleaseResourceLite(&pdode->child_lock);

    IoFreeIrp(Irp2);

    return Status;
}

static NTSTATUS vol_query_stable_guid(volume_device_extension* vde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    MOUNTDEV_STABLE_GUID* mdsg;
    pdo_device_extension* pdode;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_STABLE_GUID)) {
        Irp->IoStatus.Information = sizeof(MOUNTDEV_STABLE_GUID);
        return STATUS_BUFFER_TOO_SMALL;
    }

    mdsg = Irp->AssociatedIrp.SystemBuffer;

    if (!vde->pdo)
        return STATUS_INVALID_PARAMETER;

    pdode = vde->pdode;

    RtlCopyMemory(&mdsg->StableGuid, &pdode->uuid, sizeof(BTRFS_UUID));

    Irp->IoStatus.Information = sizeof(MOUNTDEV_STABLE_GUID);

    return STATUS_SUCCESS;
}

NTSTATUS vol_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            return vol_query_device_name(vde, Irp);

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            return vol_query_unique_id(vde, Irp);

        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            return vol_get_device_number(vde, Irp);

        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
            TRACE("unhandled control code IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n");
            break;

        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            return vol_query_stable_guid(vde, Irp);

        case IOCTL_MOUNTDEV_LINK_CREATED:
            TRACE("unhandled control code IOCTL_MOUNTDEV_LINK_CREATED\n");
            break;

        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
            return vol_get_gpt_attributes(Irp);

        case IOCTL_VOLUME_IS_DYNAMIC:
            return vol_is_dynamic(Irp);

        case IOCTL_VOLUME_ONLINE:
            Irp->IoStatus.Information = 0;
            return STATUS_SUCCESS;

        case IOCTL_VOLUME_POST_ONLINE:
            Irp->IoStatus.Information = 0;
            return STATUS_SUCCESS;

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            return vol_get_drive_geometry(DeviceObject, Irp);

        case IOCTL_DISK_IS_WRITABLE:
            return vol_is_writable(vde);

        case IOCTL_DISK_GET_LENGTH_INFO:
            return vol_get_length(vde, Irp);

        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_DISK_CHECK_VERIFY:
            return vol_check_verify(vde);

        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
            return vol_get_disk_extents(vde, Irp);

        default: { // pass ioctl through if only one child device
            NTSTATUS Status = vol_ioctl_passthrough(vde, Irp);
#ifdef _DEBUG
            ULONG code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

            if (NT_SUCCESS(Status))
                TRACE("passing through ioctl %lx (returning %08lx)\n", code, Status);
            else
                WARN("passing through ioctl %lx (returning %08lx)\n", code, Status);
#endif

            return Status;
        }
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS mountmgr_add_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath) {
    NTSTATUS Status;
    ULONG mmdltsize;
    MOUNTMGR_DRIVE_LETTER_TARGET* mmdlt;
    MOUNTMGR_DRIVE_LETTER_INFORMATION mmdli;

    mmdltsize = (ULONG)offsetof(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName[0]) + devpath->Length;

    mmdlt = ExAllocatePoolWithTag(NonPagedPool, mmdltsize, ALLOC_TAG);
    if (!mmdlt) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mmdlt->DeviceNameLength = devpath->Length;
    RtlCopyMemory(&mmdlt->DeviceName, devpath->Buffer, devpath->Length);
    TRACE("mmdlt = %.*S\n", (int)(mmdlt->DeviceNameLength / sizeof(WCHAR)), mmdlt->DeviceName);

    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER, mmdlt, mmdltsize, &mmdli, sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION), false, NULL);

    if (!NT_SUCCESS(Status))
        ERR("IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER returned %08lx\n", Status);
    else
        TRACE("DriveLetterWasAssigned = %u, CurrentDriveLetter = %c\n", mmdli.DriveLetterWasAssigned, mmdli.CurrentDriveLetter);

    ExFreePool(mmdlt);

    return Status;
}

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
NTSTATUS __stdcall pnp_removal(PVOID NotificationStructure, PVOID Context) {
    TARGET_DEVICE_REMOVAL_NOTIFICATION* tdrn = (TARGET_DEVICE_REMOVAL_NOTIFICATION*)NotificationStructure;
    pdo_device_extension* pdode = (pdo_device_extension*)Context;

    if (RtlCompareMemory(&tdrn->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE, sizeof(GUID)) == sizeof(GUID)) {
        TRACE("GUID_TARGET_DEVICE_QUERY_REMOVE\n");

        if (pdode->vde && pdode->vde->mounted_device)
            pnp_query_remove_device(pdode->vde->mounted_device, NULL);
    }

    return STATUS_SUCCESS;
}

static bool allow_degraded_mount(BTRFS_UUID* uuid) {
    HANDLE h;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING path, adus;
    uint32_t degraded = mount_allow_degraded;
    ULONG i, j, kvfilen, retlen;
    KEY_VALUE_FULL_INFORMATION* kvfi;

    path.Length = path.MaximumLength = registry_path.Length + (37 * sizeof(WCHAR));
    path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

    if (!path.Buffer) {
        ERR("out of memory\n");
        return false;
    }

    RtlCopyMemory(path.Buffer, registry_path.Buffer, registry_path.Length);
    i = registry_path.Length / sizeof(WCHAR);

    path.Buffer[i] = '\\';
    i++;

    for (j = 0; j < 16; j++) {
        path.Buffer[i] = hex_digit((uuid->uuid[j] & 0xF0) >> 4);
        path.Buffer[i+1] = hex_digit(uuid->uuid[j] & 0xF);

        i += 2;

        if (j == 3 || j == 5 || j == 7 || j == 9) {
            path.Buffer[i] = '-';
            i++;
        }
    }

    InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    kvfilen = (ULONG)offsetof(KEY_VALUE_FULL_INFORMATION, Name[0]) + (255 * sizeof(WCHAR));
    kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
    if (!kvfi) {
        ERR("out of memory\n");
        ExFreePool(path.Buffer);
        return false;
    }

    Status = ZwOpenKey(&h, KEY_QUERY_VALUE, &oa);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        goto end;
    else if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenKey returned %08lx\n", Status);
        goto end;
    }

    adus.Buffer = L"AllowDegraded";
    adus.Length = adus.MaximumLength = sizeof(adus.Buffer) - sizeof(WCHAR);

    if (NT_SUCCESS(ZwQueryValueKey(h, &adus, KeyValueFullInformation, kvfi, kvfilen, &retlen))) {
        if (kvfi->Type == REG_DWORD && kvfi->DataLength >= sizeof(uint32_t)) {
            uint32_t* val = (uint32_t*)((uint8_t*)kvfi + kvfi->DataOffset);

            degraded = *val;
        }
    }

    ZwClose(h);

end:
    ExFreePool(kvfi);

    ExFreePool(path.Buffer);

    return degraded;
}

typedef struct {
    LIST_ENTRY list_entry;
    UNICODE_STRING name;
    NTSTATUS Status;
    BTRFS_UUID uuid;
} drive_letter_removal;

static void drive_letter_callback2(pdo_device_extension* pdode, PDEVICE_OBJECT mountmgr) {
    LIST_ENTRY* le;
    LIST_ENTRY dlrlist;

    InitializeListHead(&dlrlist);

    ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

    le = pdode->children.Flink;

    while (le != &pdode->children) {
        drive_letter_removal* dlr;

        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        dlr = ExAllocatePoolWithTag(PagedPool, sizeof(drive_letter_removal), ALLOC_TAG);
        if (!dlr) {
            ERR("out of memory\n");

            while (!IsListEmpty(&dlrlist)) {
                dlr = CONTAINING_RECORD(RemoveHeadList(&dlrlist), drive_letter_removal, list_entry);

                ExFreePool(dlr->name.Buffer);
                ExFreePool(dlr);
            }

            ExReleaseResourceLite(&pdode->child_lock);
            return;
        }

        dlr->name.Length = dlr->name.MaximumLength = vc->pnp_name.Length + (3 * sizeof(WCHAR));
        dlr->name.Buffer = ExAllocatePoolWithTag(PagedPool, dlr->name.Length, ALLOC_TAG);

        if (!dlr->name.Buffer) {
            ERR("out of memory\n");

            ExFreePool(dlr);

            while (!IsListEmpty(&dlrlist)) {
                dlr = CONTAINING_RECORD(RemoveHeadList(&dlrlist), drive_letter_removal, list_entry);

                ExFreePool(dlr->name.Buffer);
                ExFreePool(dlr);
            }

            ExReleaseResourceLite(&pdode->child_lock);
            return;
        }

        RtlCopyMemory(dlr->name.Buffer, L"\\??", 3 * sizeof(WCHAR));
        RtlCopyMemory(&dlr->name.Buffer[3], vc->pnp_name.Buffer, vc->pnp_name.Length);

        dlr->uuid = vc->uuid;

        InsertTailList(&dlrlist, &dlr->list_entry);

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    le = dlrlist.Flink;
    while (le != &dlrlist) {
        drive_letter_removal* dlr = CONTAINING_RECORD(le, drive_letter_removal, list_entry);

        dlr->Status = remove_drive_letter(mountmgr, &dlr->name);

        if (!NT_SUCCESS(dlr->Status) && dlr->Status != STATUS_NOT_FOUND)
            WARN("remove_drive_letter returned %08lx\n", dlr->Status);

        le = le->Flink;
    }

    // set vc->had_drive_letter

    ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

    while (!IsListEmpty(&dlrlist)) {
        drive_letter_removal* dlr = CONTAINING_RECORD(RemoveHeadList(&dlrlist), drive_letter_removal, list_entry);

        le = pdode->children.Flink;

        while (le != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

            if (RtlCompareMemory(&vc->uuid, &dlr->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                vc->had_drive_letter = NT_SUCCESS(dlr->Status);
                break;
            }

            le = le->Flink;
        }

        ExFreePool(dlr->name.Buffer);
        ExFreePool(dlr);
    }

    ExReleaseResourceLite(&pdode->child_lock);
}

_Function_class_(IO_WORKITEM_ROUTINE)
static void __stdcall drive_letter_callback(pdo_device_extension* pdode) {
    NTSTATUS Status;
    UNICODE_STRING mmdevpath;
    PDEVICE_OBJECT mountmgr;
    PFILE_OBJECT mountmgrfo;

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
        return;
    }

    drive_letter_callback2(pdode, mountmgr);

    ObDereferenceObject(mountmgrfo);
}

void add_volume_device(superblock* sb, PUNICODE_STRING devpath, uint64_t length, ULONG disk_num, ULONG part_num) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    PDEVICE_OBJECT DeviceObject;
    volume_child* vc;
    PFILE_OBJECT FileObject;
    UNICODE_STRING devpath2;
    bool inserted = false, new_pdo = false;
    pdo_device_extension* pdode = NULL;
    PDEVICE_OBJECT pdo = NULL;
    bool process_drive_letters = false;

    if (devpath->Length == 0)
        return;

    ExAcquireResourceExclusiveLite(&pdo_list_lock, true);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode2 = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

        if (RtlCompareMemory(&pdode2->uuid, &sb->uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            pdode = pdode2;
            break;
        }

        le = le->Flink;
    }

    Status = IoGetDeviceObjectPointer(devpath, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
        ExReleaseResourceLite(&pdo_list_lock);
        return;
    }

    if (!pdode) {
        if (no_pnp) {
            Status = IoReportDetectedDevice(drvobj, InterfaceTypeUndefined, 0xFFFFFFFF, 0xFFFFFFFF, NULL, NULL, 0, &pdo);

            if (!NT_SUCCESS(Status)) {
                ERR("IoReportDetectedDevice returned %08lx\n", Status);
                ExReleaseResourceLite(&pdo_list_lock);
                return;
            }

            pdode = ExAllocatePoolWithTag(NonPagedPool, sizeof(pdo_device_extension), ALLOC_TAG);

            if (!pdode) {
                ERR("out of memory\n");
                ExReleaseResourceLite(&pdo_list_lock);
                return;
            }
        } else {
            Status = IoCreateDevice(drvobj, sizeof(pdo_device_extension), NULL, FILE_DEVICE_DISK,
                                    FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN, false, &pdo);
            if (!NT_SUCCESS(Status)) {
                ERR("IoCreateDevice returned %08lx\n", Status);
                ExReleaseResourceLite(&pdo_list_lock);
                goto fail;
            }

            pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

            pdode = pdo->DeviceExtension;
        }

        RtlZeroMemory(pdode, sizeof(pdo_device_extension));

        pdode->type = VCB_TYPE_PDO;
        pdode->pdo = pdo;
        pdode->uuid = sb->uuid;

        ExInitializeResourceLite(&pdode->child_lock);
        InitializeListHead(&pdode->children);
        pdode->num_children = sb->num_devices;
        pdode->children_loaded = 0;

        pdo->Flags &= ~DO_DEVICE_INITIALIZING;
        pdo->SectorSize = (USHORT)sb->sector_size;

        ExAcquireResourceExclusiveLite(&pdode->child_lock, true);

        new_pdo = true;
    } else {
        ExAcquireResourceExclusiveLite(&pdode->child_lock, true);
        ExConvertExclusiveToSharedLite(&pdo_list_lock);

        le = pdode->children.Flink;
        while (le != &pdode->children) {
            volume_child* vc2 = CONTAINING_RECORD(le, volume_child, list_entry);

            if (RtlCompareMemory(&vc2->uuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                // duplicate, ignore
                ExReleaseResourceLite(&pdode->child_lock);
                ExReleaseResourceLite(&pdo_list_lock);
                goto fail;
            }

            le = le->Flink;
        }
    }

    vc = ExAllocatePoolWithTag(PagedPool, sizeof(volume_child), ALLOC_TAG);
    if (!vc) {
        ERR("out of memory\n");

        ExReleaseResourceLite(&pdode->child_lock);
        ExReleaseResourceLite(&pdo_list_lock);

        goto fail;
    }

    vc->uuid = sb->dev_item.device_uuid;
    vc->devid = sb->dev_item.dev_id;
    vc->generation = sb->generation;
    vc->notification_entry = NULL;
    vc->boot_volume = false;

    Status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange, 0, FileObject,
                                            drvobj, pnp_removal, pdode, &vc->notification_entry);
    if (!NT_SUCCESS(Status))
        WARN("IoRegisterPlugPlayNotification returned %08lx\n", Status);

    vc->devobj = DeviceObject;
    vc->fileobj = FileObject;

    devpath2 = *devpath;

    // The PNP path sometimes begins \\?\ and sometimes \??\. We need to remove this prefix
    // so we can compare properly if the device is removed.
    if (devpath->Length > 4 * sizeof(WCHAR) && devpath->Buffer[0] == '\\' && (devpath->Buffer[1] == '\\' || devpath->Buffer[1] == '?') &&
        devpath->Buffer[2] == '?' && devpath->Buffer[3] == '\\') {
        devpath2.Buffer = &devpath2.Buffer[3];
        devpath2.Length -= 3 * sizeof(WCHAR);
        devpath2.MaximumLength -= 3 * sizeof(WCHAR);
    }

    vc->pnp_name.Length = vc->pnp_name.MaximumLength = devpath2.Length;
    vc->pnp_name.Buffer = ExAllocatePoolWithTag(PagedPool, devpath2.Length, ALLOC_TAG);

    if (vc->pnp_name.Buffer)
        RtlCopyMemory(vc->pnp_name.Buffer, devpath2.Buffer, devpath2.Length);
    else {
        ERR("out of memory\n");
        vc->pnp_name.Length = vc->pnp_name.MaximumLength = 0;
    }

    vc->size = length;
    vc->seeding = sb->flags & BTRFS_SUPERBLOCK_FLAGS_SEEDING ? true : false;
    vc->disk_num = disk_num;
    vc->part_num = part_num;
    vc->had_drive_letter = false;

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc2 = CONTAINING_RECORD(le, volume_child, list_entry);

        if (vc2->generation < vc->generation) {
            if (le == pdode->children.Flink)
                pdode->num_children = sb->num_devices;

            InsertHeadList(vc2->list_entry.Blink, &vc->list_entry);
            inserted = true;
            break;
        }

        le = le->Flink;
    }

    if (!inserted)
        InsertTailList(&pdode->children, &vc->list_entry);

    pdode->children_loaded++;

    if (pdode->vde && pdode->vde->mounted_device) {
        device_extension* Vcb = pdode->vde->mounted_device->DeviceExtension;

        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, true);

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->devobj && RtlCompareMemory(&dev->devitem.device_uuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                dev->devobj = DeviceObject;
                dev->disk_num = disk_num;
                dev->part_num = part_num;
                init_device(Vcb, dev, false);
                break;
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->tree_lock);
    }

    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
        pdode->removable = true;

        if (pdode->vde && pdode->vde->device)
            pdode->vde->device->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    if (pdode->num_children == pdode->children_loaded || (pdode->children_loaded == 1 && allow_degraded_mount(&sb->uuid))) {
        if ((!new_pdo || !no_pnp) && pdode->vde) {
            Status = IoSetDeviceInterfaceState(&pdode->vde->bus_name, true);
            if (!NT_SUCCESS(Status))
                WARN("IoSetDeviceInterfaceState returned %08lx\n", Status);
        }

        process_drive_letters = true;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    if (new_pdo)
        InsertTailList(&pdo_list, &pdode->list_entry);

    ExReleaseResourceLite(&pdo_list_lock);

    if (process_drive_letters)
        drive_letter_callback(pdode);

    if (new_pdo) {
        if (RtlCompareMemory(&sb->uuid, &boot_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID))
            boot_add_device(pdo);
        else if (no_pnp)
            AddDevice(drvobj, pdo);
        else {
            bus_device_extension* bde = busobj->DeviceExtension;
            IoInvalidateDeviceRelations(bde->buspdo, BusRelations);
        }
    }

    return;

fail:
    ObDereferenceObject(FileObject);
}
