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
extern ERESOURCE pdo_list_lock;
extern LIST_ENTRY pdo_list;
extern UNICODE_STRING registry_path;

NTSTATUS vol_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    if (vde->removing)
        return STATUS_DEVICE_NOT_READY;

    Irp->IoStatus.Information = FILE_OPENED;
    InterlockedIncrement(&vde->open_count);

    return STATUS_SUCCESS;
}

NTSTATUS vol_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    volume_device_extension* vde = DeviceObject->DeviceExtension;
    pdo_device_extension* pdode = vde->pdode;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;

    ExAcquireResourceExclusiveLite(&pdo_list_lock, TRUE);

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    if (InterlockedDecrement(&vde->open_count) == 0 && vde->removing) {
        NTSTATUS Status;
        UNICODE_STRING mmdevpath;
        PDEVICE_OBJECT mountmgr;
        PFILE_OBJECT mountmgrfo;
        PDEVICE_OBJECT pdo;

        RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
        Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
        if (!NT_SUCCESS(Status))
            ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        else {
            remove_drive_letter(mountmgr, &vde->name);

            ObDereferenceObject(mountmgrfo);
        }

        if (vde->mounted_device) {
            device_extension* Vcb = vde->mounted_device->DeviceExtension;

            Vcb->vde = NULL;
        }

        if (vde->name.Buffer)
            ExFreePool(vde->name.Buffer);

        ExReleaseResourceLite(&pdode->child_lock);
        ExDeleteResourceLite(&pdode->child_lock);

        if (vde->pdo->AttachedDevice)
            IoDetachDevice(vde->pdo);

        pdo = vde->pdo;
        IoDeleteDevice(vde->device);

        if (!no_pnp)
            IoDeleteDevice(pdo);
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
#ifdef __REACTOS__
static NTSTATUS NTAPI vol_read_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS vol_read_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    vol_read_context* context = conptr;

    UNUSED(DeviceObject);

    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);

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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    if (IsListEmpty(&pdode->children)) {
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    // We can't use IoSkipCurrentIrpStackLocation as the device isn't in our stack

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, FALSE);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IRP_MJ_READ;

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

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    Irp2->UserIosb = &context.iosb;

    IoSetCompletionRoutine(Irp2, vol_read_completion, &context, TRUE, TRUE, TRUE);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);
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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

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

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, FALSE);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IRP_MJ_WRITE;

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

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    Irp2->UserIosb = &context.iosb;

    IoSetCompletionRoutine(Irp2, vol_read_completion, &context, TRUE, TRUE, TRUE);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);
        Status = context.iosb.Status;
    }

    ExReleaseResourceLite(&pdode->child_lock);

    Irp->IoStatus.Information = context.iosb.Information;

end:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS vol_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_flush_buffers(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_query_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_set_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_cleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;

    return STATUS_SUCCESS;
}

NTSTATUS vol_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_file_system_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_lock_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS vol_query_device_name(volume_device_extension* vde, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_NAME name;

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
    UINT8* buf;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0 || !Irp->AssociatedIrp.SystemBuffer)
        return STATUS_INVALID_PARAMETER;

    buf = (UINT8*)Irp->AssociatedIrp.SystemBuffer;

    *buf = 1;

    Irp->IoStatus.Information = 1;

    return STATUS_SUCCESS;
}

static NTSTATUS vol_check_verify(volume_device_extension* vde) {
    pdo_device_extension* pdode = vde->pdode;
    NTSTATUS Status;
    LIST_ENTRY* le;

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        Status = dev_ioctl(vc->devobj, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, FALSE, NULL);
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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);
        VOLUME_DISK_EXTENTS ext2;

        Status = dev_ioctl(vc->devobj, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &ext2, sizeof(VOLUME_DISK_EXTENTS), FALSE, NULL);
        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            ERR("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS returned %08x\n", Status);
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
                           (ULONG)offsetof(VOLUME_DISK_EXTENTS, Extents[0]) + (max_extents * sizeof(DISK_EXTENT)), FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS returned %08x\n", Status);
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
    BOOL writable = FALSE;

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc = CONTAINING_RECORD(le, volume_child, list_entry);

        Status = dev_ioctl(vc->devobj, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, TRUE, NULL);

        if (NT_SUCCESS(Status)) {
            writable = TRUE;
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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

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
    UINT64 length;
    LIST_ENTRY* le;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
        return STATUS_BUFFER_TOO_SMALL;

    length = 0;

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

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
#ifdef __REACTOS__
static NTSTATUS NTAPI vol_ioctl_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#else
static NTSTATUS vol_ioctl_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
#endif
    KEVENT* event = conptr;

    UNUSED(DeviceObject);
    UNUSED(Irp);

    KeSetEvent(event, 0, FALSE);

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

    ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

    if (IsListEmpty(&pdode->children)) {
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    vc = CONTAINING_RECORD(pdode->children.Flink, volume_child, list_entry);

    if (vc->list_entry.Flink != &pdode->children) { // more than one device
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp2 = IoAllocateIrp(vc->devobj->StackSize, FALSE);

    if (!Irp2) {
        ERR("IoAllocateIrp failed\n");
        ExReleaseResourceLite(&pdode->child_lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp2 = IoGetNextIrpStackLocation(Irp2);

    IrpSp2->MajorFunction = IrpSp->MajorFunction;
    IrpSp2->MinorFunction = IrpSp->MinorFunction;

    IrpSp2->Parameters.DeviceIoControl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    IrpSp2->Parameters.DeviceIoControl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    IrpSp2->Parameters.DeviceIoControl.IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    IrpSp2->Parameters.DeviceIoControl.Type3InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    Irp2->AssociatedIrp.SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    Irp2->MdlAddress = Irp->MdlAddress;
    Irp2->UserBuffer = Irp->UserBuffer;
    Irp2->Flags = Irp->Flags;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp2, vol_ioctl_completion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(vc->devobj, Irp2);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp2->IoStatus.Status;
    }

    Irp->IoStatus.Status = Irp2->IoStatus.Status;
    Irp->IoStatus.Information = Irp2->IoStatus.Information;

    ExReleaseResourceLite(&pdode->child_lock);

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

        default: // pass ioctl through if only one child device
            return vol_ioctl_passthrough(vde, Irp);
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_shutdown(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    TRACE("(%p, %p)\n", DeviceObject, Irp);

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS vol_power(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    TRACE("(%p, %p)\n", DeviceObject, Irp);

    if (IrpSp->MinorFunction == IRP_MN_SET_POWER || IrpSp->MinorFunction == IRP_MN_QUERY_POWER)
        Irp->IoStatus.Status = STATUS_SUCCESS;

    Status = Irp->IoStatus.Status;
    PoStartNextPowerIrp(Irp);

    return Status;
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
    TRACE("mmdlt = %.*S\n", mmdlt->DeviceNameLength / sizeof(WCHAR), mmdlt->DeviceName);

    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER, mmdlt, mmdltsize, &mmdli, sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION), FALSE, NULL);

    if (!NT_SUCCESS(Status))
        ERR("IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER returned %08x\n", Status);
    else
        TRACE("DriveLetterWasAssigned = %u, CurrentDriveLetter = %c\n", mmdli.DriveLetterWasAssigned, mmdli.CurrentDriveLetter);

    ExFreePool(mmdlt);

    return Status;
}

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI pnp_removal(PVOID NotificationStructure, PVOID Context) {
#else
NTSTATUS pnp_removal(PVOID NotificationStructure, PVOID Context) {
#endif
    TARGET_DEVICE_REMOVAL_NOTIFICATION* tdrn = (TARGET_DEVICE_REMOVAL_NOTIFICATION*)NotificationStructure;
    pdo_device_extension* pdode = (pdo_device_extension*)Context;

    if (RtlCompareMemory(&tdrn->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE, sizeof(GUID)) == sizeof(GUID)) {
        TRACE("GUID_TARGET_DEVICE_QUERY_REMOVE\n");

        if (pdode->vde && pdode->vde->mounted_device)
            return pnp_query_remove_device(pdode->vde->mounted_device, NULL);
    }

    return STATUS_SUCCESS;
}

static BOOL allow_degraded_mount(BTRFS_UUID* uuid) {
    HANDLE h;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING path, adus;
    UINT32 degraded = mount_allow_degraded;
    ULONG i, j, kvfilen, retlen;
    KEY_VALUE_FULL_INFORMATION* kvfi;

    path.Length = path.MaximumLength = registry_path.Length + (37 * sizeof(WCHAR));
    path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

    if (!path.Buffer) {
        ERR("out of memory\n");
        return FALSE;
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
        return FALSE;
    }

    Status = ZwOpenKey(&h, KEY_QUERY_VALUE, &oa);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        goto end;
    else if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenKey returned %08x\n", Status);
        goto end;
    }

    adus.Buffer = L"AllowDegraded";
    adus.Length = adus.MaximumLength = sizeof(adus.Buffer) - sizeof(WCHAR);

    if (NT_SUCCESS(ZwQueryValueKey(h, &adus, KeyValueFullInformation, kvfi, kvfilen, &retlen))) {
        if (kvfi->Type == REG_DWORD && kvfi->DataLength >= sizeof(UINT32)) {
            UINT32* val = (UINT32*)((UINT8*)kvfi + kvfi->DataOffset);

            degraded = *val;
        }
    }

    ZwClose(h);

end:
    ExFreePool(kvfi);

    ExFreePool(path.Buffer);

    return degraded;
}

void add_volume_device(superblock* sb, PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath, UINT64 length, ULONG disk_num, ULONG part_num) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    PDEVICE_OBJECT DeviceObject;
    volume_child* vc;
    PFILE_OBJECT FileObject;
    UNICODE_STRING devpath2;
    BOOL inserted = FALSE, new_pdo = FALSE;
    pdo_device_extension* pdode = NULL;
    PDEVICE_OBJECT pdo = NULL;

    if (devpath->Length == 0)
        return;

    ExAcquireResourceExclusiveLite(&pdo_list_lock, TRUE);

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
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        ExReleaseResourceLite(&pdo_list_lock);
        return;
    }

    if (!pdode) {
        if (no_pnp) {
            Status = IoReportDetectedDevice(drvobj, InterfaceTypeUndefined, 0xFFFFFFFF, 0xFFFFFFFF, NULL, NULL, 0, &pdo);

            if (!NT_SUCCESS(Status)) {
                ERR("IoReportDetectedDevice returned %08x\n", Status);
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
                                    FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN, FALSE, &pdo);
            if (!NT_SUCCESS(Status)) {
                ERR("IoCreateDevice returned %08x\n", Status);
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

        ExAcquireResourceExclusiveLite(&pdode->child_lock, TRUE);

        new_pdo = TRUE;
    } else {
        ExAcquireResourceExclusiveLite(&pdode->child_lock, TRUE);
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

    Status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange, 0, FileObject,
                                            drvobj, pnp_removal, pdode, &vc->notification_entry);
    if (!NT_SUCCESS(Status))
        WARN("IoRegisterPlugPlayNotification returned %08x\n", Status);

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
    vc->seeding = sb->flags & BTRFS_SUPERBLOCK_FLAGS_SEEDING ? TRUE : FALSE;
    vc->disk_num = disk_num;
    vc->part_num = part_num;
    vc->had_drive_letter = FALSE;

    le = pdode->children.Flink;
    while (le != &pdode->children) {
        volume_child* vc2 = CONTAINING_RECORD(le, volume_child, list_entry);

        if (vc2->generation < vc->generation) {
            if (le == pdode->children.Flink)
                pdode->num_children = sb->num_devices;

            InsertHeadList(vc2->list_entry.Blink, &vc->list_entry);
            inserted = TRUE;
            break;
        }

        le = le->Flink;
    }

    if (!inserted)
        InsertTailList(&pdode->children, &vc->list_entry);

    pdode->children_loaded++;

    if (pdode->vde && pdode->vde->mounted_device) {
        device_extension* Vcb = pdode->vde->mounted_device->DeviceExtension;

        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

        le = Vcb->devices.Flink;
        while (le != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le, device, list_entry);

            if (!dev->devobj && RtlCompareMemory(&dev->devitem.device_uuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                dev->devobj = DeviceObject;
                dev->disk_num = disk_num;
                dev->part_num = part_num;
                init_device(Vcb, dev, FALSE);
                break;
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&Vcb->tree_lock);
    }

    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
        pdode->removable = TRUE;

        if (pdode->vde && pdode->vde->device)
            pdode->vde->device->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    if (pdode->num_children == pdode->children_loaded || (pdode->children_loaded == 1 && allow_degraded_mount(&sb->uuid))) {
        if (pdode->num_children == 1) {
            Status = remove_drive_letter(mountmgr, devpath);
            if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
                WARN("remove_drive_letter returned %08x\n", Status);

            vc->had_drive_letter = NT_SUCCESS(Status);
        } else {
            le = pdode->children.Flink;

            while (le != &pdode->children) {
                UNICODE_STRING name;

                vc = CONTAINING_RECORD(le, volume_child, list_entry);

                name.Length = name.MaximumLength = vc->pnp_name.Length + (3 * sizeof(WCHAR));
                name.Buffer = ExAllocatePoolWithTag(PagedPool, name.Length, ALLOC_TAG);

                if (!name.Buffer) {
                    ERR("out of memory\n");

                    ExReleaseResourceLite(&pdode->child_lock);
                    ExReleaseResourceLite(&pdo_list_lock);

                    goto fail;
                }

                RtlCopyMemory(name.Buffer, L"\\??", 3 * sizeof(WCHAR));
                RtlCopyMemory(&name.Buffer[3], vc->pnp_name.Buffer, vc->pnp_name.Length);

                Status = remove_drive_letter(mountmgr, &name);

                if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
                    WARN("remove_drive_letter returned %08x\n", Status);

                ExFreePool(name.Buffer);

                vc->had_drive_letter = NT_SUCCESS(Status);

                le = le->Flink;
            }
        }

        if ((!new_pdo || !no_pnp) && pdode->vde) {
            Status = IoSetDeviceInterfaceState(&pdode->vde->bus_name, TRUE);
            if (!NT_SUCCESS(Status))
                WARN("IoSetDeviceInterfaceState returned %08x\n", Status);
        }
    }

    ExReleaseResourceLite(&pdode->child_lock);

    if (new_pdo) {
        control_device_extension* cde = master_devobj->DeviceExtension;

        InsertTailList(&pdo_list, &pdode->list_entry);

        if (!no_pnp)
            IoInvalidateDeviceRelations(cde->buspdo, BusRelations);
    }

    ExReleaseResourceLite(&pdo_list_lock);

    if (new_pdo && no_pnp)
        AddDevice(drvobj, pdo);

    return;

fail:
    ObDereferenceObject(FileObject);
}
