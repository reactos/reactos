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
#include <ntddk.h>
#include <ntifs.h>
#include <mountmgr.h>
#include <windef.h>
#endif

#include <initguid.h>
#ifndef __REACTOS__
#include <winioctl.h>
#endif
#include <wdmguid.h>

extern LIST_ENTRY volumes;
extern ERESOURCE volumes_lock;
extern LIST_ENTRY pnp_disks;

static NTSTATUS create_part0(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PUNICODE_STRING devpath,
                             PUNICODE_STRING nameus, BTRFS_UUID* uuid) {
    PDEVICE_OBJECT newdevobj;
    UNICODE_STRING name;
    NTSTATUS Status;
    part0_device_extension* p0de;
    
    static const WCHAR part0_suffix[] = L"Btrfs";
    
    name.Length = name.MaximumLength = devpath->Length + (wcslen(part0_suffix) * sizeof(WCHAR));
    name.Buffer = ExAllocatePoolWithTag(PagedPool, name.Length, ALLOC_TAG);
    if (!name.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(name.Buffer, devpath->Buffer, devpath->Length);
    RtlCopyMemory(&name.Buffer[devpath->Length / sizeof(WCHAR)], part0_suffix, wcslen(part0_suffix) * sizeof(WCHAR));
    
    Status = IoCreateDevice(DriverObject, sizeof(part0_device_extension), &name, FILE_DEVICE_DISK, FILE_DEVICE_SECURE_OPEN, FALSE, &newdevobj);
    if (!NT_SUCCESS(Status)) {
        ERR("IoCreateDevice returned %08x\n", Status);
        ExFreePool(name.Buffer);
        return Status;
    }
    
    p0de = newdevobj->DeviceExtension;
    p0de->type = VCB_TYPE_PARTITION0;
    p0de->devobj = DeviceObject;
    RtlCopyMemory(&p0de->uuid, uuid, sizeof(BTRFS_UUID));
    
    p0de->name.Length = name.Length;
    p0de->name.MaximumLength = name.MaximumLength;
    p0de->name.Buffer = ExAllocatePoolWithTag(PagedPool, p0de->name.MaximumLength, ALLOC_TAG);
    
    if (!p0de->name.Buffer) {
        ERR("out of memory\b");
        ExFreePool(name.Buffer);
        ExFreePool(p0de->name.Buffer);
        IoDeleteDevice(newdevobj);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(p0de->name.Buffer, name.Buffer, name.Length);
    
    ObReferenceObject(DeviceObject);
    
    newdevobj->StackSize = DeviceObject->StackSize + 1;
    newdevobj->SectorSize = DeviceObject->SectorSize;
    
    newdevobj->Flags |= DO_DIRECT_IO;
    newdevobj->Flags &= ~DO_DEVICE_INITIALIZING;
    
    *nameus = name;
    
    return STATUS_SUCCESS;
}

void add_volume(PDEVICE_OBJECT mountmgr, PUNICODE_STRING us) {
    ULONG tnsize;
    MOUNTMGR_TARGET_NAME* tn;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    NTSTATUS Status;
    ULONG mmdltsize;
    MOUNTMGR_DRIVE_LETTER_TARGET* mmdlt;
    MOUNTMGR_DRIVE_LETTER_INFORMATION mmdli;
    
    TRACE("found BTRFS volume\n");
    
    tnsize = sizeof(MOUNTMGR_TARGET_NAME) - sizeof(WCHAR) + us->Length;
    tn = ExAllocatePoolWithTag(NonPagedPool, tnsize, ALLOC_TAG);
    if (!tn) {
        ERR("out of memory\n");
        return;
    }
    
    tn->DeviceNameLength = us->Length;
    RtlCopyMemory(tn->DeviceName, us->Buffer, tn->DeviceNameLength);
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
                                        mountmgr, tn, tnsize, 
                                        NULL, 0, FALSE, &Event, &IoStatusBlock);
    if (!Irp) {
        ERR("%.*S: IoBuildDeviceIoControlRequest 1 failed\n", us->Length / sizeof(WCHAR), us->Buffer);
        ExFreePool(tn);
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        ERR("%.*S: IoCallDriver 1 returned %08x\n", us->Length / sizeof(WCHAR), us->Buffer, Status);
        return;
    }
    
    ExFreePool(tn);
    
    mmdltsize = offsetof(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName[0]) + us->Length;
    
    mmdlt = ExAllocatePoolWithTag(NonPagedPool, mmdltsize, ALLOC_TAG);
    if (!mmdlt) {
        ERR("out of memory\n");
        return;
    }
    
    mmdlt->DeviceNameLength = us->Length;
    RtlCopyMemory(&mmdlt->DeviceName, us->Buffer, us->Length);
    TRACE("mmdlt = %.*S\n", mmdlt->DeviceNameLength / sizeof(WCHAR), mmdlt->DeviceName);
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                        mountmgr, mmdlt, mmdltsize, 
                                        &mmdli, sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION), FALSE, &Event, &IoStatusBlock);
    if (!Irp) {
        ERR("%.*S: IoBuildDeviceIoControlRequest 2 failed\n", us->Length / sizeof(WCHAR), us->Buffer);
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        ERR("%.*S: IoCallDriver 2 returned %08x\n", us->Length / sizeof(WCHAR), us->Buffer, Status);
    } else
        TRACE("DriveLetterWasAssigned = %u, CurrentDriveLetter = %c\n", mmdli.DriveLetterWasAssigned, mmdli.CurrentDriveLetter);
    
    ExFreePool(mmdlt);
}

static void STDCALL test_vol(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath, DWORD disk_num, DWORD part_num, LIST_ENTRY* volumes) {
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    LARGE_INTEGER Offset;
    ULONG toread;
    UINT8* data = NULL;
    UINT32 sector_size;
    
    TRACE("%.*S\n", devpath->Length / sizeof(WCHAR), devpath->Buffer);
    
    Status = IoGetDeviceObjectPointer(devpath, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }

    sector_size = DeviceObject->SectorSize;
    
    if (sector_size == 0) {
        DISK_GEOMETRY geometry;
        IO_STATUS_BLOCK iosb;
        
        Status = dev_ioctl(DeviceObject, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                        &geometry, sizeof(DISK_GEOMETRY), TRUE, &iosb);
        
        if (!NT_SUCCESS(Status)) {
            ERR("%.*S had a sector size of 0, and IOCTL_DISK_GET_DRIVE_GEOMETRY returned %08x\n",
                devpath->Length / sizeof(WCHAR), devpath->Buffer, Status);
            goto deref;
        }
        
        if (iosb.Information < sizeof(DISK_GEOMETRY)) {
            ERR("%.*S: IOCTL_DISK_GET_DRIVE_GEOMETRY returned %u bytes, expected %u\n",
                devpath->Length / sizeof(WCHAR), devpath->Buffer, iosb.Information, sizeof(DISK_GEOMETRY));
        }
        
        sector_size = geometry.BytesPerSector;
        
        if (sector_size == 0) {
            ERR("%.*S had a sector size of 0\n", devpath->Length / sizeof(WCHAR), devpath->Buffer);
            goto deref;
        }
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Offset.QuadPart = superblock_addrs[0];
    
    toread = sector_align(sizeof(superblock), sector_size);
    data = ExAllocatePoolWithTag(NonPagedPool, toread, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        goto deref;
    }

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, DeviceObject, data, toread, &Offset, &Event, &IoStatusBlock);
    
    if (!Irp) {
        ERR("IoBuildSynchronousFsdRequest failed\n");
        goto deref;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS(Status) && IoStatusBlock.Information > 0 && ((superblock*)data)->magic == BTRFS_MAGIC) {
        int i;
        GET_LENGTH_INFORMATION gli;
        superblock* sb = (superblock*)data;
        volume* v = ExAllocatePoolWithTag(PagedPool, sizeof(volume), ALLOC_TAG);
        
        if (!v) {
            ERR("out of memory\n");
            goto deref;
        }
        
        Status = dev_ioctl(DeviceObject, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                           &gli, sizeof(gli), TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("error reading length information: %08x\n", Status);
            ExFreePool(v);
            goto deref;
        }
        
        if (part_num == 0) {
            UNICODE_STRING us3;
            
            Status = create_part0(DriverObject, DeviceObject, devpath, &us3, &sb->dev_item.device_uuid);
            
            if (!NT_SUCCESS(Status)) {
                ERR("create_part0 returned %08x\n", Status);
                ExFreePool(v);
                goto deref;
            }
            
            v->devpath = us3;
        } else {
            v->devpath.Length = v->devpath.MaximumLength = devpath->Length;
            v->devpath.Buffer = ExAllocatePoolWithTag(PagedPool, v->devpath.Length, ALLOC_TAG);
            
            if (!v->devpath.Buffer) {
                ERR("out of memory\n");
                ExFreePool(v);
                goto deref;
            }
            
            RtlCopyMemory(v->devpath.Buffer, devpath->Buffer, v->devpath.Length);
        }
        
        RtlCopyMemory(&v->fsuuid, &sb->uuid, sizeof(BTRFS_UUID));
        RtlCopyMemory(&v->devuuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID));
        v->devnum = sb->dev_item.dev_id;
        v->processed = FALSE;
        v->length = gli.Length.QuadPart;
        v->gen1 = sb->generation;
        v->gen2 = 0;
        v->seeding = sb->flags & BTRFS_SUPERBLOCK_FLAGS_SEEDING ? TRUE : FALSE;
        v->disk_num = disk_num;
        v->part_num = part_num;
        
        ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
        InsertTailList(volumes, &v->list_entry);
        ExReleaseResourceLite(&volumes_lock);
        
        i = 1;
        while (superblock_addrs[i] != 0 && superblock_addrs[i] + toread <= v->length) {
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            
            Offset.QuadPart = superblock_addrs[i];
            
            Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, DeviceObject, data, toread, &Offset, &Event, &IoStatusBlock);
            
            if (!Irp) {
                ERR("IoBuildSynchronousFsdRequest failed\n");
                goto deref;
            }

            Status = IoCallDriver(DeviceObject, Irp);

            if (Status == STATUS_PENDING) {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }
            
            if (NT_SUCCESS(Status)) {
                UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));
                
                if (crc32 != *((UINT32*)sb->checksum))
                    WARN("superblock %u CRC error\n", i);
                else if (sb->generation > v->gen1) {
                    v->gen2 = v->gen1;
                    v->gen1 = sb->generation;
                }
            } else {
                ERR("got error %08x while reading superblock %u\n", Status, i);
            }
            
            i++;
        }
        
        TRACE("volume found\n");
        TRACE("gen1 = %llx, gen2 = %llx\n", v->gen1, v->gen2);
        TRACE("FS uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
              v->fsuuid.uuid[0], v->fsuuid.uuid[1], v->fsuuid.uuid[2], v->fsuuid.uuid[3], v->fsuuid.uuid[4], v->fsuuid.uuid[5], v->fsuuid.uuid[6], v->fsuuid.uuid[7],
              v->fsuuid.uuid[8], v->fsuuid.uuid[9], v->fsuuid.uuid[10], v->fsuuid.uuid[11], v->fsuuid.uuid[12], v->fsuuid.uuid[13], v->fsuuid.uuid[14], v->fsuuid.uuid[15]);
        TRACE("device uuid %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
              v->devuuid.uuid[0], v->devuuid.uuid[1], v->devuuid.uuid[2], v->devuuid.uuid[3], v->devuuid.uuid[4], v->devuuid.uuid[5], v->devuuid.uuid[6], v->devuuid.uuid[7],
              v->devuuid.uuid[8], v->devuuid.uuid[9], v->devuuid.uuid[10], v->devuuid.uuid[11], v->devuuid.uuid[12], v->devuuid.uuid[13], v->devuuid.uuid[14], v->devuuid.uuid[15]);
        TRACE("device number %llx\n", v->devnum);
    }
    
deref:
    if (data)
        ExFreePool(data);
    
    ObDereferenceObject(FileObject);
}

void remove_drive_letter(PDEVICE_OBJECT mountmgr, volume* v) {
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;
    MOUNTMGR_MOUNT_POINT* mmp;
    ULONG mmpsize;
    MOUNTMGR_MOUNT_POINTS mmps1, *mmps2;
    IO_STATUS_BLOCK IoStatusBlock;
    
    mmpsize = sizeof(MOUNTMGR_MOUNT_POINT) + v->devpath.Length;
    
    mmp = ExAllocatePoolWithTag(PagedPool, mmpsize, ALLOC_TAG);
    if (!mmp) {
        ERR("out of memory\n");
        return;
    }
    
    RtlZeroMemory(mmp, mmpsize);
    
    mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mmp->DeviceNameLength = v->devpath.Length;
    RtlCopyMemory(&mmp[1], v->devpath.Buffer, v->devpath.Length);
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_DELETE_POINTS,
                                        mountmgr, mmp, mmpsize, 
                                        &mmps1, sizeof(MOUNTMGR_MOUNT_POINTS), FALSE, &Event, &IoStatusBlock);
    if (!Irp) {
        ERR("%.*S: IoBuildDeviceIoControlRequest 1 failed\n", v->devpath.Length / sizeof(WCHAR), v->devpath.Buffer);
        ExFreePool(mmp);
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("%.*S: IoCallDriver 1  returned %08x\n", v->devpath.Length / sizeof(WCHAR), v->devpath.Buffer, Status);
        ExFreePool(mmp);
        return;
    }
    
    if (Status != STATUS_BUFFER_OVERFLOW || mmps1.Size == 0) {
        ExFreePool(mmp);
        return;
    }
    
    mmps2 = ExAllocatePoolWithTag(PagedPool, mmps1.Size, ALLOC_TAG);
    if (!mmps2) {
        ERR("out of memory\n");
        ExFreePool(mmp);
        return;
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_DELETE_POINTS,
                                        mountmgr, mmp, mmpsize, 
                                        mmps2, mmps1.Size, FALSE, &Event, &IoStatusBlock);
    if (!Irp) {
        ERR("%.*S: IoBuildDeviceIoControlRequest 2 failed\n", v->devpath.Length / sizeof(WCHAR), v->devpath.Buffer);
        ExFreePool(mmps2);
        ExFreePool(mmp);
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        ERR("%.*S: IoCallDriver 2 returned %08x\n", v->devpath.Length / sizeof(WCHAR), v->devpath.Buffer, Status);
        ExFreePool(mmps2);
        ExFreePool(mmp);
        return;
    }
    
    ExFreePool(mmps2);
    ExFreePool(mmp);
}

static void refresh_mountmgr(PDEVICE_OBJECT mountmgr, LIST_ENTRY* volumes) {
    LIST_ENTRY* le;
    
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    
    le = volumes->Flink;
    while (le != volumes) {
        volume* v = CONTAINING_RECORD(le, volume, list_entry);
        
        if (!v->processed) {
            LIST_ENTRY* le2 = le;
            volume* mountvol = v;
            
            while (le2 != volumes) {
                volume* v2 = CONTAINING_RECORD(le2, volume, list_entry);
                
                if (RtlCompareMemory(&v2->fsuuid, &v->fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                    v2->processed = TRUE;
                    
                    if (v2->devnum < mountvol->devnum) {
                        remove_drive_letter(mountmgr, mountvol);
                        mountvol = v2;
                    } else if (v2->devnum > mountvol->devnum)
                        remove_drive_letter(mountmgr, v2);
                }
                
                le2 = le2->Flink;
            }
            
            add_volume(mountmgr, &mountvol->devpath);
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&volumes_lock);
}

static void add_pnp_disk(ULONG disk_num, PUNICODE_STRING devpath) {
    LIST_ENTRY* le;
    pnp_disk* disk;
    
    le = pnp_disks.Flink;
    while (le != &pnp_disks) {
        disk = CONTAINING_RECORD(le, pnp_disk, list_entry);
        
        if (disk->devpath.Length == devpath->Length &&
            RtlCompareMemory(disk->devpath.Buffer, devpath->Buffer, devpath->Length) == devpath->Length)
            return;
        
        le = le->Flink;
    }
    
    disk = ExAllocatePoolWithTag(PagedPool, sizeof(pnp_disk), ALLOC_TAG);
    if (!disk) {
        ERR("out of memory\n");
        return;
    }
    
    disk->devpath.Length = disk->devpath.MaximumLength = devpath->Length;
    disk->devpath.Buffer = ExAllocatePoolWithTag(PagedPool, devpath->Length, ALLOC_TAG);
    
    if (!disk->devpath.Buffer) {
        ERR("out of memory\n");
        ExFreePool(disk);
        return;
    }
    
    RtlCopyMemory(disk->devpath.Buffer, devpath->Buffer, devpath->Length);
    
    disk->disk_num = disk_num;
    
    InsertTailList(&pnp_disks, &disk->list_entry);
}

static void disk_arrival(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath) {
    PFILE_OBJECT FileObject, FileObject2;
    PDEVICE_OBJECT devobj, mountmgr;
    NTSTATUS Status;
    STORAGE_DEVICE_NUMBER sdn;
    ULONG dlisize;
    DRIVE_LAYOUT_INFORMATION_EX* dli;
    IO_STATUS_BLOCK iosb;
    int i, num_parts = 0;
    UNICODE_STRING devname, num, bspus, mmdevpath;
    WCHAR devnamew[255], numw[20];
    USHORT preflen;
    
    static WCHAR device_harddisk[] = L"\\Device\\Harddisk";
    static WCHAR bs_partition[] = L"\\Partition";
    
    // FIXME - work with CD-ROMs and floppies(?)
        
    Status = IoGetDeviceObjectPointer(devpath, FILE_READ_ATTRIBUTES, &FileObject, &devobj);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }
    
    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject2, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        ObDereferenceObject(FileObject);
        return;
    }
    
    Status = dev_ioctl(devobj, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                       &sdn, sizeof(STORAGE_DEVICE_NUMBER), TRUE, &iosb);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_STORAGE_GET_DEVICE_NUMBER returned %08x\n", Status);
        goto end;
    }
    
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    add_pnp_disk(sdn.DeviceNumber, devpath);
    ExReleaseResourceLite(&volumes_lock);
    
    dlisize = 0;
    
    do {
        dlisize += 1024;
        dli = ExAllocatePoolWithTag(PagedPool, dlisize, ALLOC_TAG);
    
        Status = dev_ioctl(devobj, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0,
                           dli, dlisize, TRUE, &iosb);
    } while (Status == STATUS_BUFFER_TOO_SMALL);
    
    if (!NT_SUCCESS(Status)) {
        ExFreePool(dli);
        goto no_parts;
    }
    
    wcscpy(devnamew, device_harddisk);
    devname.Buffer = devnamew;
    devname.MaximumLength = sizeof(devnamew);
    devname.Length = wcslen(device_harddisk) * sizeof(WCHAR);

    num.Buffer = numw;
    num.MaximumLength = sizeof(numw);
    RtlIntegerToUnicodeString(sdn.DeviceNumber, 10, &num);
    RtlAppendUnicodeStringToString(&devname, &num);
    
    bspus.Buffer = bs_partition;
    bspus.Length = bspus.MaximumLength = wcslen(bs_partition) * sizeof(WCHAR);
    RtlAppendUnicodeStringToString(&devname, &bspus);
    
    preflen = devname.Length;
    
    for (i = 0; i < dli->PartitionCount; i++) {
        if (dli->PartitionEntry[i].PartitionLength.QuadPart != 0 && dli->PartitionEntry[i].PartitionNumber != 0) {
            devname.Length = preflen;
            RtlIntegerToUnicodeString(dli->PartitionEntry[i].PartitionNumber, 10, &num);
            RtlAppendUnicodeStringToString(&devname, &num);
            
            test_vol(DriverObject, mountmgr, &devname, sdn.DeviceNumber, dli->PartitionEntry[i].PartitionNumber, &volumes);
            
            num_parts++;
        }
    }
    
    ExFreePool(dli);
    
no_parts:
    if (num_parts == 0) {
        devname.Length = preflen;
        devname.Buffer[devname.Length / sizeof(WCHAR)] = '0';
        devname.Length += sizeof(WCHAR);
        
        test_vol(DriverObject, mountmgr, &devname, sdn.DeviceNumber, 0, &volumes);
    }
    
end:
    refresh_mountmgr(mountmgr, &volumes);

    ObDereferenceObject(FileObject);
    ObDereferenceObject(FileObject2);
}

static void disk_removal(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath) {
    LIST_ENTRY* le;
    pnp_disk* disk = NULL;
    
    // FIXME - remove Partition0Btrfs devices and unlink from mountmgr
    // FIXME - emergency unmount of RAIDed volumes
    
    ExAcquireResourceExclusiveLite(&volumes_lock, TRUE);
    
    le = pnp_disks.Flink;
    while (le != &pnp_disks) {
        pnp_disk* disk2 = CONTAINING_RECORD(le, pnp_disk, list_entry);
        
        if (disk2->devpath.Length == devpath->Length &&
            RtlCompareMemory(disk2->devpath.Buffer, devpath->Buffer, devpath->Length) == devpath->Length) {
            disk = disk2;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!disk) {
        ExReleaseResourceLite(&volumes_lock);
        return;
    }

    le = volumes.Flink;
    while (le != &volumes) {
        volume* v = CONTAINING_RECORD(le, volume, list_entry);
        LIST_ENTRY* le2 = le->Flink;
        
        if (v->disk_num == disk->disk_num) {
            if (v->devpath.Buffer)
                ExFreePool(v->devpath.Buffer);
            
            RemoveEntryList(&v->list_entry);
            
            ExFreePool(v);
        }
        
        le = le2;
    }
    
    ExReleaseResourceLite(&volumes_lock);
    
    ExFreePool(disk->devpath.Buffer);
    
    RemoveEntryList(&disk->list_entry);
    
    ExFreePool(disk);
}

#ifdef __REACTOS__
NTSTATUS NTAPI pnp_notification(PVOID NotificationStructure, PVOID Context) {
#else
NTSTATUS pnp_notification(PVOID NotificationStructure, PVOID Context) {
#endif
    DEVICE_INTERFACE_CHANGE_NOTIFICATION* dicn = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Context;
    
    if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID)) == sizeof(GUID))
        disk_arrival(DriverObject, dicn->SymbolicLinkName);
    else if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID)) == sizeof(GUID))
        disk_removal(DriverObject, dicn->SymbolicLinkName);
    
    return STATUS_SUCCESS;
}
