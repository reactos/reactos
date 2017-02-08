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
        ERR("IoBuildDeviceIoControlRequest failed\n");
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
        ERR("IoCallDriver (1) returned %08x\n", Status);
    
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
        ERR("IoBuildDeviceIoControlRequest failed\n");
        return;
    }

    Status = IoCallDriver(mountmgr, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
        ERR("IoCallDriver (2) returned %08x\n", Status);
    else
        TRACE("DriveLetterWasAssigned = %u, CurrentDriveLetter = %c\n", mmdli.DriveLetterWasAssigned, mmdli.CurrentDriveLetter);
    
    ExFreePool(mmdlt);
}

static void STDCALL test_vol(PDEVICE_OBJECT mountmgr, PUNICODE_STRING us, LIST_ENTRY* volumes) {
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
    
    TRACE("%.*S\n", us->Length / sizeof(WCHAR), us->Buffer);
    
    us2.Length = ((wcslen(devpath) + 1) * sizeof(WCHAR)) + us->Length;
    us2.MaximumLength = us2.Length;
    us2.Buffer = ExAllocatePoolWithTag(PagedPool, us2.Length, ALLOC_TAG);
    if (!us2.Buffer) {
        ERR("out of memory\n");
        return;
    }
    
    RtlCopyMemory(us2.Buffer, devpath, wcslen(devpath) * sizeof(WCHAR));
    us2.Buffer[wcslen(devpath)] = '\\';
    RtlCopyMemory(&us2.Buffer[wcslen(devpath)+1], us->Buffer, us->Length);
    
    TRACE("%.*S\n", us2.Length / sizeof(WCHAR), us2.Buffer);
    
    Status = IoGetDeviceObjectPointer(&us2, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        goto exit;
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Offset.QuadPart = superblock_addrs[0];
    
    toread = sector_align(sizeof(superblock), DeviceObject->SectorSize);
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
        superblock* sb = (superblock*)data;
        volume* v = ExAllocatePoolWithTag(PagedPool, sizeof(volume), ALLOC_TAG);
        if (!v) {
            ERR("out of memory\n");
            goto deref;
        }
        
        v->devobj = DeviceObject;
        RtlCopyMemory(&v->fsuuid, &sb->uuid, sizeof(BTRFS_UUID));
        RtlCopyMemory(&v->devuuid, &sb->dev_item.device_uuid, sizeof(BTRFS_UUID));
        v->devnum = sb->dev_item.dev_id;
        v->devpath = us2;
        v->processed = FALSE;
        InsertTailList(volumes, &v->list_entry);
        
        TRACE("volume found\n");
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

void STDCALL look_for_vols(LIST_ENTRY* volumes) {
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
    
    static const WCHAR hdv[] = {'H','a','r','d','d','i','s','k','V','o','l','u','m','e',0};
    static const WCHAR device[] = {'D','e','v','i','c','e',0};
    
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
                if (odi2->TypeName.Length == wcslen(device) * sizeof(WCHAR) &&
                    RtlCompareMemory(odi2->TypeName.Buffer, device, wcslen(device) * sizeof(WCHAR)) == wcslen(device) * sizeof(WCHAR) &&
                    odi2->Name.Length > wcslen(hdv) * sizeof(WCHAR) &&
                    RtlCompareMemory(odi2->Name.Buffer, hdv, wcslen(hdv) * sizeof(WCHAR)) == wcslen(hdv) * sizeof(WCHAR)) {
                        test_vol(mountmgr, &odi2->Name, volumes);
                }
                odi2 = &odi2[1];
            }
        }
    } while (NT_SUCCESS(Status));
    
    ZwClose(h);
    
    // FIXME - if Windows has already added the second device of a filesystem itself, delete it
    
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
                    
                    if (v2->devnum < mountvol->devnum)
                        mountvol = v2;
                }
                
                le2 = le2->Flink;
            }
            
            add_volume(mountmgr, &mountvol->devpath);
        }
        
        le = le->Flink;
    }
    
    ObDereferenceObject(FileObject);
}
