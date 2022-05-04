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
#include <ntdddisk.h>
#include <mountdev.h>
#include <diskguid.h>

extern PDRIVER_OBJECT drvobj;
extern LIST_ENTRY VcbList;
extern ERESOURCE global_loading_lock;

static NTSTATUS mountdev_query_stable_guid(device_extension* Vcb, PIRP Irp) {
    MOUNTDEV_STABLE_GUID* msg = Irp->UserBuffer;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    TRACE("IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n");

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_STABLE_GUID))
        return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(&msg->StableGuid, &Vcb->superblock.uuid, sizeof(GUID));

    Irp->IoStatus.Information = sizeof(MOUNTDEV_STABLE_GUID);

    return STATUS_SUCCESS;
}

static NTSTATUS is_writable(device_extension* Vcb) {
    TRACE("IOCTL_DISK_IS_WRITABLE\n");

    return Vcb->readonly ? STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;
}

static NTSTATUS query_filesystems(void* data, ULONG length) {
    NTSTATUS Status;
    LIST_ENTRY *le, *le2;
    btrfs_filesystem* bfs = NULL;
    ULONG itemsize;

    ExAcquireResourceSharedLite(&global_loading_lock, true);

    if (IsListEmpty(&VcbList)) {
        if (length < sizeof(btrfs_filesystem)) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto end;
        } else {
            RtlZeroMemory(data, sizeof(btrfs_filesystem));
            Status = STATUS_SUCCESS;
            goto end;
        }
    }

    le = VcbList.Flink;

    while (le != &VcbList) {
        device_extension* Vcb = CONTAINING_RECORD(le, device_extension, list_entry);
        btrfs_filesystem_device* bfd;

        if (bfs) {
            bfs->next_entry = itemsize;
            bfs = (btrfs_filesystem*)((uint8_t*)bfs + itemsize);
        } else
            bfs = data;

        if (length < offsetof(btrfs_filesystem, device)) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto end;
        }

        itemsize = offsetof(btrfs_filesystem, device);
        length -= offsetof(btrfs_filesystem, device);

        bfs->next_entry = 0;
        RtlCopyMemory(&bfs->uuid, &Vcb->superblock.uuid, sizeof(BTRFS_UUID));

        ExAcquireResourceSharedLite(&Vcb->tree_lock, true);

        bfs->num_devices = (uint32_t)Vcb->superblock.num_devices;

        bfd = NULL;

        le2 = Vcb->devices.Flink;
        while (le2 != &Vcb->devices) {
            device* dev = CONTAINING_RECORD(le2, device, list_entry);
            MOUNTDEV_NAME mdn;

            if (bfd)
                bfd = (btrfs_filesystem_device*)((uint8_t*)bfd + offsetof(btrfs_filesystem_device, name[0]) + bfd->name_length);
            else
                bfd = &bfs->device;

            if (length < offsetof(btrfs_filesystem_device, name[0])) {
                ExReleaseResourceLite(&Vcb->tree_lock);
                Status = STATUS_BUFFER_OVERFLOW;
                goto end;
            }

            itemsize += (ULONG)offsetof(btrfs_filesystem_device, name[0]);
            length -= (ULONG)offsetof(btrfs_filesystem_device, name[0]);

            RtlCopyMemory(&bfd->uuid, &dev->devitem.device_uuid, sizeof(BTRFS_UUID));

            if (dev->devobj) {
                Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), true, NULL);
                if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
                    ExReleaseResourceLite(&Vcb->tree_lock);
                    ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08lx\n", Status);
                    goto end;
                }

                if (mdn.NameLength > length) {
                    ExReleaseResourceLite(&Vcb->tree_lock);
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto end;
                }

                Status = dev_ioctl(dev->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &bfd->name_length, (ULONG)offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength, true, NULL);
                if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
                    ExReleaseResourceLite(&Vcb->tree_lock);
                    ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08lx\n", Status);
                    goto end;
                }

                itemsize += bfd->name_length;
                length -= bfd->name_length;
            } else {
                bfd->missing = true;
                bfd->name_length = 0;
            }

            le2 = le2->Flink;
        }

        ExReleaseResourceLite(&Vcb->tree_lock);

        le = le->Flink;
    }

    Status = STATUS_SUCCESS;

end:
    ExReleaseResourceLite(&global_loading_lock);

    return Status;
}

static NTSTATUS probe_volume(void* data, ULONG length, KPROCESSOR_MODE processor_mode) {
    MOUNTDEV_NAME* mdn = (MOUNTDEV_NAME*)data;
    UNICODE_STRING path, pnp_name;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    const GUID* guid;

    if (length < sizeof(MOUNTDEV_NAME))
        return STATUS_INVALID_PARAMETER;

    if (length < offsetof(MOUNTDEV_NAME, Name[0]) + mdn->NameLength)
        return STATUS_INVALID_PARAMETER;

    TRACE("%.*S\n", (int)(mdn->NameLength / sizeof(WCHAR)), mdn->Name);

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    path.Buffer = mdn->Name;
    path.Length = path.MaximumLength = mdn->NameLength;

    Status = IoGetDeviceObjectPointer(&path, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
        return Status;
    }

    Status = get_device_pnp_name(DeviceObject, &pnp_name, &guid);
    if (!NT_SUCCESS(Status)) {
        ERR("get_device_pnp_name returned %08lx\n", Status);
        ObDereferenceObject(FileObject);
        return Status;
    }

    if (RtlCompareMemory(guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID)) {
        Status = dev_ioctl(DeviceObject, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, true, NULL);
        if (!NT_SUCCESS(Status))
            WARN("IOCTL_DISK_UPDATE_PROPERTIES returned %08lx\n", Status);
    }

    ObDereferenceObject(FileObject);

    volume_removal(&pnp_name);

    if (RtlCompareMemory(guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID))
        disk_arrival(&pnp_name);
    else
        volume_arrival(&pnp_name, false);

    return STATUS_SUCCESS;
}

static NTSTATUS ioctl_unload(PIRP Irp) {
    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_LOAD_DRIVER_PRIVILEGE), Irp->RequestorMode)) {
        ERR("insufficient privileges\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    do_shutdown(Irp);

    return STATUS_SUCCESS;
}

static NTSTATUS control_ioctl(PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_BTRFS_QUERY_FILESYSTEMS:
            Status = query_filesystems(map_user_buffer(Irp, NormalPagePriority), IrpSp->Parameters.FileSystemControl.OutputBufferLength);
            break;

        case IOCTL_BTRFS_PROBE_VOLUME:
            Status = probe_volume(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.FileSystemControl.InputBufferLength, Irp->RequestorMode);
            break;

        case IOCTL_BTRFS_UNLOAD:
            Status = ioctl_unload(Irp);
            break;

        default:
            TRACE("unhandled ioctl %lx\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return Status;
}

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    bool top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);

    Irp->IoStatus.Information = 0;

    if (Vcb) {
        if (Vcb->type == VCB_TYPE_CONTROL) {
            Status = control_ioctl(Irp);
            goto end;
        } else if (Vcb->type == VCB_TYPE_VOLUME) {
            Status = vol_device_control(DeviceObject, Irp);
            goto end;
        } else if (Vcb->type != VCB_TYPE_FS) {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            Status = mountdev_query_stable_guid(Vcb, Irp);
            goto end;

        case IOCTL_DISK_IS_WRITABLE:
            Status = is_writable(Vcb);
            goto end;

        default:
            TRACE("unhandled control code %lx\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);

    Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);

    goto end2;

end:
    Irp->IoStatus.Status = Status;

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

end2:
    TRACE("returning %08lx\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}
