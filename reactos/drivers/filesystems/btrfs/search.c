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

#ifndef __REACTOS__
typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;
#endif

#if !defined (_GNU_NTIFS_) || defined(__REACTOS__)
NTSTATUS WINAPI ZwQueryDirectoryObject(HANDLE DirectoryHandle, PVOID Buffer, ULONG Length,
                                       BOOLEAN ReturnSingleEntry, BOOLEAN RestartScan, PULONG Context,
                                       PULONG  ReturnLength);
#endif

VOID WINAPI IopNotifyPlugPlayNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN LPCGUID Event,
    IN PVOID EventCategoryData1,
    IN PVOID EventCategoryData2
);

static const WCHAR devpath[] = {'\\','D','e','v','i','c','e',0};

static NTSTATUS create_part0(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PUNICODE_STRING pardir, PUNICODE_STRING nameus,
                             BTRFS_UUID* uuid) {
    PDEVICE_OBJECT newdevobj;
    UNICODE_STRING name;
    NTSTATUS Status;
    part0_device_extension* p0de;
    
    static const WCHAR btrfs_partition[] = L"\\BtrfsPartition";
    
    name.Length = name.MaximumLength = pardir->Length + (wcslen(btrfs_partition) * sizeof(WCHAR));
    name.Buffer = ExAllocatePoolWithTag(PagedPool, name.Length, ALLOC_TAG);
    if (!name.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(name.Buffer, pardir->Buffer, pardir->Length);
    RtlCopyMemory(&name.Buffer[pardir->Length / sizeof(WCHAR)], btrfs_partition, wcslen(btrfs_partition) * sizeof(WCHAR));
    
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
    
    newdevobj->Flags |= DO_DIRECT_IO;
    newdevobj->Flags &= ~DO_DEVICE_INITIALIZING;
    
    *nameus = name;
    
    return STATUS_SUCCESS;
}

static void STDCALL add_volume(PDEVICE_OBJECT mountmgr, PUNICODE_STRING us) {
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
    
    mmdltsize = sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) - 1 + us->Length;
    
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

static void STDCALL test_vol(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT mountmgr, PUNICODE_STRING pardir, PUNICODE_STRING us, BOOL part0, LIST_ENTRY* volumes) {
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    LARGE_INTEGER Offset;
    ULONG toread;
    UINT8* data;
    UNICODE_STRING us2;
    BOOL added_entry = FALSE;
    UINT32 sector_size;
    
    TRACE("%.*S\n", us->Length / sizeof(WCHAR), us->Buffer);
    
    us2.Length = pardir->Length + sizeof(WCHAR) + us->Length;
    us2.MaximumLength = us2.Length;
    us2.Buffer = ExAllocatePoolWithTag(PagedPool, us2.Length, ALLOC_TAG);
    if (!us2.Buffer) {
        ERR("out of memory\n");
        return;
    }
    
    RtlCopyMemory(us2.Buffer, pardir->Buffer, pardir->Length);
    us2.Buffer[pardir->Length / sizeof(WCHAR)] = '\\';
    RtlCopyMemory(&us2.Buffer[(pardir->Length / sizeof(WCHAR))+1], us->Buffer, us->Length);
    
    TRACE("%.*S\n", us2.Length / sizeof(WCHAR), us2.Buffer);
    
    Status = IoGetDeviceObjectPointer(&us2, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        goto exit;
    }

    sector_size = DeviceObject->SectorSize;
    
    if (sector_size == 0) {
        DISK_GEOMETRY geometry;
        IO_STATUS_BLOCK iosb;
        
        Status = dev_ioctl(DeviceObject, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                        &geometry, sizeof(DISK_GEOMETRY), TRUE, &iosb);
        
        if (!NT_SUCCESS(Status)) {
            ERR("%.*S had a sector size of 0, and IOCTL_DISK_GET_DRIVE_GEOMETRY returned %08x\n",
                us2.Length / sizeof(WCHAR), us2.Buffer, Status);
            goto exit;
        }
        
        if (iosb.Information < sizeof(DISK_GEOMETRY)) {
            ERR("%.*S: IOCTL_DISK_GET_DRIVE_GEOMETRY returned %u bytes, expected %u\n",
                us2.Length / sizeof(WCHAR), us2.Buffer, iosb.Information, sizeof(DISK_GEOMETRY));
        }
        
        sector_size = geometry.BytesPerSector;
        
        if (sector_size == 0) {
            ERR("%.*S had a sector size of 0\n", us2.Length / sizeof(WCHAR), us2.Buffer);
            goto exit;
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
            goto deref;
        }
        
        if (part0) {
            UNICODE_STRING us3;
            
            Status = create_part0(DriverObject, DeviceObject, pardir, &us3, &sb->dev_item.device_uuid);
            
            if (!NT_SUCCESS(Status)) {
                ERR("create_part0 returned %08x\n", Status);
                goto deref;
            }
            
            ExFreePool(us2.Buffer);
            us2 = us3;
        }
        
        RtlCopyMemory(&v->fsuuid, &sb->uuid, sizeof(BTRFS_UUID));
        RtlCopyMemory(&v->devuuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID));
        v->devnum = sb->dev_item.dev_id;
        v->devpath = us2;
        v->processed = FALSE;
        v->length = gli.Length.QuadPart;
        v->gen1 = sb->generation;
        v->gen2 = 0;
        v->seeding = sb->flags & BTRFS_SUPERBLOCK_FLAGS_SEEDING ? TRUE : FALSE;
        InsertTailList(volumes, &v->list_entry);
        
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

        added_entry = TRUE;
    }
    
deref:
    ExFreePool(data);
    ObDereferenceObject(FileObject);
    
exit:
    if (!added_entry)
        ExFreePool(us2.Buffer);
}

static NTSTATUS look_in_harddisk_dir(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT mountmgr, PUNICODE_STRING name, LIST_ENTRY* volumes) {
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS Status;
    HANDLE h;
    OBJECT_DIRECTORY_INFORMATION* odi;
    ULONG odisize, context;
    BOOL restart, has_part0 = FALSE, has_parts = FALSE;
    
    static const WCHAR partition[] = L"Partition";
    static WCHAR partition0[] = L"Partition0";
    
    path.Buffer = ExAllocatePoolWithTag(PagedPool, ((wcslen(devpath) + 1) * sizeof(WCHAR)) + name->Length, ALLOC_TAG);
    if (!path.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(path.Buffer, devpath, wcslen(devpath) * sizeof(WCHAR));
    path.Buffer[wcslen(devpath)] = '\\';
    RtlCopyMemory(&path.Buffer[wcslen(devpath) + 1], name->Buffer, name->Length);
    path.Length = path.MaximumLength = ((wcslen(devpath) + 1) * sizeof(WCHAR)) + name->Length;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &path;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    
    Status = ZwOpenDirectoryObject(&h, DIRECTORY_TRAVERSE, &attr);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenDirectoryObject returned %08x\n", Status);
        goto end;
    }
    
    odisize = sizeof(OBJECT_DIRECTORY_INFORMATION) * 16;
    odi = ExAllocatePoolWithTag(PagedPool, odisize, ALLOC_TAG);
    if (!odi) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ZwClose(h);
        goto end;
    }
    
    restart = TRUE;
    do {
        Status = ZwQueryDirectoryObject(h, odi, odisize, FALSE, restart, &context, NULL/*&retlen*/);
        restart = FALSE;
        
        if (!NT_SUCCESS(Status)) {
            if (Status != STATUS_NO_MORE_ENTRIES)
                ERR("ZwQueryDirectoryObject returned %08x\n", Status);
        } else {
            OBJECT_DIRECTORY_INFORMATION* odi2 = odi;
            
            while (odi2->Name.Buffer) {
                TRACE("%.*S, %.*S\n", odi2->TypeName.Length / sizeof(WCHAR), odi2->TypeName.Buffer, odi2->Name.Length / sizeof(WCHAR), odi2->Name.Buffer);
                
                if (odi2->Name.Length > wcslen(partition) * sizeof(WCHAR) &&
                    RtlCompareMemory(odi2->Name.Buffer, partition, wcslen(partition) * sizeof(WCHAR)) == wcslen(partition) * sizeof(WCHAR)) {
                    
                    if (odi2->Name.Length == (wcslen(partition) + 1) * sizeof(WCHAR) && odi2->Name.Buffer[(odi2->Name.Length / sizeof(WCHAR)) - 1] == '0') {
                        // Partition0 refers to the whole disk
                        has_part0 = TRUE;
                    } else {
                        has_parts = TRUE;
                        
                        test_vol(DriverObject, mountmgr, &path, &odi2->Name, FALSE, volumes);
                    }
                }
                
                odi2 = &odi2[1];
            }
        }
    } while (NT_SUCCESS(Status));
    
    // If disk had no partitions, test the whole disk
    if (!has_parts && has_part0) {
        UNICODE_STRING part0us;
        
        part0us.Buffer = partition0;
        part0us.Length = part0us.MaximumLength = wcslen(partition0) * sizeof(WCHAR);
        
        test_vol(DriverObject, mountmgr, &path, &part0us, TRUE, volumes);
    }
    
    ZwClose(h);
    
    ExFreePool(odi);
    
    Status = STATUS_SUCCESS;
    
end:
    ExFreePool(path.Buffer);
    
    return Status;
}

static void remove_drive_letter(PDEVICE_OBJECT mountmgr, volume* v) {
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

void STDCALL look_for_vols(PDRIVER_OBJECT DriverObject, LIST_ENTRY* volumes) {
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT mountmgr;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING mmdevpath, us;
    HANDLE h;
    OBJECT_DIRECTORY_INFORMATION* odi;
    ULONG odisize;
    ULONG context;
    BOOL restart;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    static const WCHAR directory[] = L"Directory";
    static const WCHAR harddisk[] = L"Harddisk";
    
    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }
    
    RtlInitUnicodeString(&us, devpath);
    
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &us;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    Status = ZwOpenDirectoryObject(&h, DIRECTORY_TRAVERSE, &attr);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenDirectoryObject returned %08x\n", Status);
        return;
    }
    
    odisize = sizeof(OBJECT_DIRECTORY_INFORMATION) * 16;
    odi = ExAllocatePoolWithTag(PagedPool, odisize, ALLOC_TAG);
    if (!odi) {
        ERR("out of memory\n");
        ZwClose(h);
        return;
    }
    
    restart = TRUE;
    do {
        Status = ZwQueryDirectoryObject(h, odi, odisize, FALSE, restart, &context, NULL/*&retlen*/);
        restart = FALSE;
        
        if (!NT_SUCCESS(Status)) {
            if (Status != STATUS_NO_MORE_ENTRIES)
                ERR("ZwQueryDirectoryObject returned %08x\n", Status);
        } else {
            OBJECT_DIRECTORY_INFORMATION* odi2 = odi;
            
            while (odi2->Name.Buffer) {
                if (odi2->TypeName.Length == wcslen(directory) * sizeof(WCHAR) &&
                    RtlCompareMemory(odi2->TypeName.Buffer, directory, odi2->TypeName.Length) == odi2->TypeName.Length &&
                    odi2->Name.Length > wcslen(harddisk) * sizeof(WCHAR) &&
                    RtlCompareMemory(odi2->Name.Buffer, harddisk, wcslen(harddisk) * sizeof(WCHAR)) == wcslen(harddisk) * sizeof(WCHAR)) {
                        look_in_harddisk_dir(DriverObject, mountmgr, &odi2->Name, volumes);
                }
                
                odi2 = &odi2[1];
            }
        }
    } while (NT_SUCCESS(Status));
    
    ExFreePool(odi);
    ZwClose(h);
    
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
    
    ObDereferenceObject(FileObject);
}
