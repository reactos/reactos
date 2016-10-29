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
#ifndef __REACTOS__
#include <winioctl.h>
#endif
#include <mountdev.h>
#include <initguid.h>
#include <diskguid.h>

static NTSTATUS part0_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    part0_device_extension* p0de = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    TRACE("control code = %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
    
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        {
            MOUNTDEV_UNIQUE_ID* mduid;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_UNIQUE_ID)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            mduid = Irp->AssociatedIrp.SystemBuffer;
            mduid->UniqueIdLength = sizeof(BTRFS_UUID);

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_UNIQUE_ID) - 1 + mduid->UniqueIdLength) {
                Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            RtlCopyMemory(mduid->UniqueId, &p0de->uuid, sizeof(BTRFS_UUID));

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID) - 1 + mduid->UniqueIdLength;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return Status;
        }
        
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            PMOUNTDEV_NAME name;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            name = Irp->AssociatedIrp.SystemBuffer;
            name->NameLength = p0de->name.Length;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME) - 1 + name->NameLength) {
                Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
            
            RtlCopyMemory(name->Name, p0de->name.Buffer, p0de->name.Length);

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME) - 1 + name->NameLength;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return Status;
        }
    }
    
    IoSkipCurrentIrpStackLocation(Irp);
    
    Status = IoCallDriver(p0de->devobj, Irp);
    
    TRACE("returning %08x\n", Status);
    
    return Status;
}

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

static NTSTATUS get_partition_info_ex(device_extension* Vcb, PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PARTITION_INFORMATION_EX* piex;
    
    TRACE("IOCTL_DISK_GET_PARTITION_INFO_EX\n");
    
    Status = dev_ioctl(Vcb->devices[0].devobj, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0,
                       Irp->UserBuffer, IrpSp->Parameters.DeviceIoControl.OutputBufferLength, TRUE, &Irp->IoStatus);
    if (!NT_SUCCESS(Status))
        return Status;
    
    piex = (PARTITION_INFORMATION_EX*)Irp->UserBuffer;
    
    if (piex->PartitionStyle == PARTITION_STYLE_MBR) {
        piex->Mbr.PartitionType = PARTITION_IFS;
        piex->Mbr.RecognizedPartition = TRUE;
    } else if (piex->PartitionStyle == PARTITION_STYLE_GPT) {
        piex->Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS is_writable(device_extension* Vcb, PIRP Irp) {
    TRACE("IOCTL_DISK_IS_WRITABLE\n");
    
    return Vcb->readonly ? STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;
}

NTSTATUS STDCALL drv_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    BOOL top_level;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    Irp->IoStatus.Information = 0;
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_device_control(DeviceObject, Irp);
        goto end2;
    }
    
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            Status = mountdev_query_stable_guid(Vcb, Irp);
            goto end;
            
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            Status = get_partition_info_ex(Vcb, Irp);
            goto end;
            
        case IOCTL_DISK_IS_WRITABLE:
            Status = is_writable(Vcb, Irp);
            goto end;
            
        default:
            TRACE("unhandled control code %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            break;
    }
    
    IoSkipCurrentIrpStackLocation(Irp);
    
    Status = IoCallDriver(Vcb->devices[0].devobj, Irp);
    
    goto end2;
    
end:
    Irp->IoStatus.Status = Status;

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
end2:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();

    return Status;
}
