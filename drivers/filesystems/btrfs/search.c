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

#ifndef __REACTOS__
#include <ntddk.h>
#include <ntifs.h>
#include <mountmgr.h>
#include <windef.h>
#endif
#include <ntddstor.h>
#include <ntdddisk.h>

#include <initguid.h>
#include <wdmguid.h>

extern ERESOURCE pdo_list_lock;
extern LIST_ENTRY pdo_list;
extern UNICODE_STRING registry_path;
extern KEVENT mountmgr_thread_event;
extern HANDLE mountmgr_thread_handle;
extern BOOL shutting_down;

typedef void (*pnp_callback)(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath);

extern PDEVICE_OBJECT master_devobj;

static BOOL fs_ignored(BTRFS_UUID* uuid) {
    UNICODE_STRING path, ignoreus;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    KEY_VALUE_FULL_INFORMATION* kvfi;
    ULONG dispos, retlen, kvfilen, i, j;
    HANDLE h;
    BOOL ret = FALSE;

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

    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);

    if (!NT_SUCCESS(Status)) {
        TRACE("ZwCreateKey returned %08x\n", Status);
        ExFreePool(path.Buffer);
        return FALSE;
    }

    RtlInitUnicodeString(&ignoreus, L"Ignore");

    kvfilen = (ULONG)offsetof(KEY_VALUE_FULL_INFORMATION, Name[0]) + (255 * sizeof(WCHAR));
    kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
    if (!kvfi) {
        ERR("out of memory\n");
        ZwClose(h);
        ExFreePool(path.Buffer);
        return FALSE;
    }

    Status = ZwQueryValueKey(h, &ignoreus, KeyValueFullInformation, kvfi, kvfilen, &retlen);
    if (NT_SUCCESS(Status)) {
        if (kvfi->Type == REG_DWORD && kvfi->DataLength >= sizeof(UINT32)) {
            UINT32* pr = (UINT32*)((UINT8*)kvfi + kvfi->DataOffset);

            ret = *pr;
        }
    }

    ZwClose(h);
    ExFreePool(kvfi);
    ExFreePool(path.Buffer);

    return ret;
}

static void test_vol(PDEVICE_OBJECT mountmgr, PDEVICE_OBJECT DeviceObject, PUNICODE_STRING devpath,
                     DWORD disk_num, DWORD part_num, UINT64 length) {
    NTSTATUS Status;
    ULONG toread;
    UINT8* data = NULL;
    UINT32 sector_size;

    TRACE("%.*S\n", devpath->Length / sizeof(WCHAR), devpath->Buffer);

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

    toread = (ULONG)sector_align(sizeof(superblock), sector_size);
    data = ExAllocatePoolWithTag(NonPagedPool, toread, ALLOC_TAG);
    if (!data) {
        ERR("out of memory\n");
        goto deref;
    }

    Status = sync_read_phys(DeviceObject, superblock_addrs[0], toread, data, TRUE);

    if (NT_SUCCESS(Status) && ((superblock*)data)->magic == BTRFS_MAGIC) {
        superblock* sb = (superblock*)data;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));

        if (crc32 != *((UINT32*)sb->checksum))
            ERR("checksum error on superblock\n");
        else {
            TRACE("volume found\n");

            if (length >= superblock_addrs[1] + toread) {
                ULONG i = 1;

                superblock* sb2 = ExAllocatePoolWithTag(NonPagedPool, toread, ALLOC_TAG);
                if (!sb2) {
                    ERR("out of memory\n");
                    goto deref;
                }

                while (superblock_addrs[i] > 0 && length >= superblock_addrs[i] + toread) {
                    Status = sync_read_phys(DeviceObject, superblock_addrs[i], toread, (PUCHAR)sb2, TRUE);

                    if (NT_SUCCESS(Status) && sb2->magic == BTRFS_MAGIC) {
                        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb2->uuid, (ULONG)sizeof(superblock) - sizeof(sb2->checksum));

                        if (crc32 == *((UINT32*)sb2->checksum) && sb2->generation > sb->generation)
                            RtlCopyMemory(sb, sb2, toread);
                    }

                    i++;
                }

                ExFreePool(sb2);
            }

            if (!fs_ignored(&sb->uuid)) {
                DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
                add_volume_device(sb, mountmgr, devpath, length, disk_num, part_num);
            }
        }
    }

deref:
    if (data)
        ExFreePool(data);
}

NTSTATUS remove_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath) {
    NTSTATUS Status;
    MOUNTMGR_MOUNT_POINT* mmp;
    ULONG mmpsize;
    MOUNTMGR_MOUNT_POINTS mmps1, *mmps2;

    TRACE("removing drive letter\n");

    mmpsize = sizeof(MOUNTMGR_MOUNT_POINT) + devpath->Length;

    mmp = ExAllocatePoolWithTag(PagedPool, mmpsize, ALLOC_TAG);
    if (!mmp) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(mmp, mmpsize);

    mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mmp->DeviceNameLength = devpath->Length;
    RtlCopyMemory(&mmp[1], devpath->Buffer, devpath->Length);

    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_DELETE_POINTS, mmp, mmpsize, &mmps1, sizeof(MOUNTMGR_MOUNT_POINTS), FALSE, NULL);

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
        ERR("IOCTL_MOUNTMGR_DELETE_POINTS 1 returned %08x\n", Status);
        ExFreePool(mmp);
        return Status;
    }

    if (Status != STATUS_BUFFER_OVERFLOW || mmps1.Size == 0) {
        ExFreePool(mmp);
        return STATUS_NOT_FOUND;
    }

    mmps2 = ExAllocatePoolWithTag(PagedPool, mmps1.Size, ALLOC_TAG);
    if (!mmps2) {
        ERR("out of memory\n");
        ExFreePool(mmp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_DELETE_POINTS, mmp, mmpsize, mmps2, mmps1.Size, FALSE, NULL);

    if (!NT_SUCCESS(Status))
        ERR("IOCTL_MOUNTMGR_DELETE_POINTS 2 returned %08x\n", Status);

    ExFreePool(mmps2);
    ExFreePool(mmp);

    return Status;
}

void disk_arrival(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath) {
    PFILE_OBJECT FileObject, mountmgrfo;
    PDEVICE_OBJECT devobj, mountmgr;
    NTSTATUS Status;
    STORAGE_DEVICE_NUMBER sdn;
    ULONG dlisize;
    DRIVE_LAYOUT_INFORMATION_EX* dli = NULL;
    IO_STATUS_BLOCK iosb;
    GET_LENGTH_INFORMATION gli;
    UNICODE_STRING mmdevpath;

    UNUSED(DriverObject);

    Status = IoGetDeviceObjectPointer(devpath, FILE_READ_ATTRIBUTES, &FileObject, &devobj);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        ObDereferenceObject(FileObject);
        return;
    }

    dlisize = 0;

    do {
        dlisize += 1024;

        if (dli)
            ExFreePool(dli);

        dli = ExAllocatePoolWithTag(PagedPool, dlisize, ALLOC_TAG);
        if (!dli) {
            ERR("out of memory\n");
            goto end;
        }

        Status = dev_ioctl(devobj, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0,
                           dli, dlisize, TRUE, &iosb);
    } while (Status == STATUS_BUFFER_TOO_SMALL);

    // only consider disk as a potential filesystem if it has no partitions
    if (NT_SUCCESS(Status) && dli->PartitionCount > 0) {
        ExFreePool(dli);
        goto end;
    }

    ExFreePool(dli);

    Status = dev_ioctl(devobj, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                        &gli, sizeof(gli), TRUE, NULL);

    if (!NT_SUCCESS(Status)) {
        ERR("error reading length information: %08x\n", Status);
        goto end;
    }

    Status = dev_ioctl(devobj, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                       &sdn, sizeof(STORAGE_DEVICE_NUMBER), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        TRACE("IOCTL_STORAGE_GET_DEVICE_NUMBER returned %08x\n", Status);
        sdn.DeviceNumber = 0xffffffff;
        sdn.PartitionNumber = 0;
    } else
        TRACE("DeviceType = %u, DeviceNumber = %u, PartitionNumber = %u\n", sdn.DeviceType, sdn.DeviceNumber, sdn.PartitionNumber);

    test_vol(mountmgr, devobj, devpath, sdn.DeviceNumber, sdn.PartitionNumber, gli.Length.QuadPart);

end:
    ObDereferenceObject(FileObject);
    ObDereferenceObject(mountmgrfo);
}

void remove_volume_child(_Inout_ _Requires_exclusive_lock_held_(_Curr_->child_lock) _Releases_exclusive_lock_(_Curr_->child_lock) _In_ volume_device_extension* vde,
                         _In_ volume_child* vc, _In_ BOOL skip_dev) {
    NTSTATUS Status;
    pdo_device_extension* pdode = vde->pdode;
    device_extension* Vcb = vde->mounted_device ? vde->mounted_device->DeviceExtension : NULL;

    if (vc->notification_entry)
#ifdef __REACTOS__
        IoUnregisterPlugPlayNotification(vc->notification_entry);
#else
        IoUnregisterPlugPlayNotificationEx(vc->notification_entry);
#endif

    if (vde->mounted_device && (!Vcb || !Vcb->options.allow_degraded)) {
        Status = pnp_surprise_removal(vde->mounted_device, NULL);
        if (!NT_SUCCESS(Status))
            ERR("pnp_surprise_removal returned %08x\n", Status);
    }

    if (!Vcb || !Vcb->options.allow_degraded) {
        Status = IoSetDeviceInterfaceState(&vde->bus_name, FALSE);
        if (!NT_SUCCESS(Status))
            WARN("IoSetDeviceInterfaceState returned %08x\n", Status);
    }

    if (pdode->children_loaded > 0) {
        UNICODE_STRING mmdevpath;
        PFILE_OBJECT FileObject;
        PDEVICE_OBJECT mountmgr;
        LIST_ENTRY* le;

        if (!Vcb || !Vcb->options.allow_degraded) {
            RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
            Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
            if (!NT_SUCCESS(Status))
                ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
            else {
                le = pdode->children.Flink;

                while (le != &pdode->children) {
                    volume_child* vc2 = CONTAINING_RECORD(le, volume_child, list_entry);

                    if (vc2->had_drive_letter) { // re-add entry to mountmgr
                        MOUNTDEV_NAME mdn;

                        Status = dev_ioctl(vc2->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), TRUE, NULL);
                        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
                            ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                        else {
                            MOUNTDEV_NAME* mdn2;
                            ULONG mdnsize = (ULONG)offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

                            mdn2 = ExAllocatePoolWithTag(PagedPool, mdnsize, ALLOC_TAG);
                            if (!mdn2)
                                ERR("out of memory\n");
                            else {
                                Status = dev_ioctl(vc2->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, mdnsize, TRUE, NULL);
                                if (!NT_SUCCESS(Status))
                                    ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                                else {
                                    UNICODE_STRING name;

                                    name.Buffer = mdn2->Name;
                                    name.Length = name.MaximumLength = mdn2->NameLength;

                                    Status = mountmgr_add_drive_letter(mountmgr, &name);
                                    if (!NT_SUCCESS(Status))
                                        WARN("mountmgr_add_drive_letter returned %08x\n", Status);
                                }

                                ExFreePool(mdn2);
                            }
                        }
                    }

                    le = le->Flink;
                }

                ObDereferenceObject(FileObject);
            }
        } else if (!skip_dev) {
            ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

            le = Vcb->devices.Flink;
            while (le != &Vcb->devices) {
                device* dev = CONTAINING_RECORD(le, device, list_entry);

                if (dev->devobj == vc->devobj) {
                    dev->devobj = NULL; // mark as missing
                    break;
                }

                le = le->Flink;
            }

            ExReleaseResourceLite(&Vcb->tree_lock);
        }

        if (vde->device->Characteristics & FILE_REMOVABLE_MEDIA) {
            vde->device->Characteristics &= ~FILE_REMOVABLE_MEDIA;

            le = pdode->children.Flink;
            while (le != &pdode->children) {
                volume_child* vc2 = CONTAINING_RECORD(le, volume_child, list_entry);

                if (vc2 != vc && vc2->devobj->Characteristics & FILE_REMOVABLE_MEDIA) {
                    vde->device->Characteristics |= FILE_REMOVABLE_MEDIA;
                    break;
                }

                le = le->Flink;
            }
        }
    }

    ObDereferenceObject(vc->fileobj);
    ExFreePool(vc->pnp_name.Buffer);
    RemoveEntryList(&vc->list_entry);
    ExFreePool(vc);

    pdode->children_loaded--;

    if (pdode->children_loaded == 0) { // remove volume device
        BOOL remove = FALSE;

        RemoveEntryList(&pdode->list_entry);

        vde->removing = TRUE;

        Status = IoSetDeviceInterfaceState(&vde->bus_name, FALSE);
        if (!NT_SUCCESS(Status))
            WARN("IoSetDeviceInterfaceState returned %08x\n", Status);

        if (vde->pdo->AttachedDevice)
            IoDetachDevice(vde->pdo);

        if (vde->open_count == 0)
            remove = TRUE;

        ExReleaseResourceLite(&pdode->child_lock);

        if (!no_pnp) {
            control_device_extension* cde = master_devobj->DeviceExtension;

            IoInvalidateDeviceRelations(cde->buspdo, BusRelations);
        }

        if (remove) {
            if (vde->name.Buffer)
                ExFreePool(vde->name.Buffer);

            if (Vcb)
                Vcb->vde = NULL;

            ExDeleteResourceLite(&pdode->child_lock);

            IoDeleteDevice(vde->device);
        }
    } else
        ExReleaseResourceLite(&pdode->child_lock);
}

void volume_arrival(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath) {
    STORAGE_DEVICE_NUMBER sdn;
    PFILE_OBJECT FileObject, mountmgrfo;
    UNICODE_STRING mmdevpath;
    PDEVICE_OBJECT devobj, mountmgr;
    GET_LENGTH_INFORMATION gli;
    NTSTATUS Status;

    TRACE("%.*S\n", devpath->Length / sizeof(WCHAR), devpath->Buffer);

    Status = IoGetDeviceObjectPointer(devpath, FILE_READ_ATTRIBUTES, &FileObject, &devobj);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }

    // make sure we're not processing devices we've created ourselves

    if (devobj->DriverObject == DriverObject)
        goto end;

    Status = dev_ioctl(devobj, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_DISK_GET_LENGTH_INFO returned %08x\n", Status);
        goto end;
    }

    Status = dev_ioctl(devobj, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                       &sdn, sizeof(STORAGE_DEVICE_NUMBER), TRUE, NULL);
    if (!NT_SUCCESS(Status)) {
        TRACE("IOCTL_STORAGE_GET_DEVICE_NUMBER returned %08x\n", Status);
        sdn.DeviceNumber = 0xffffffff;
        sdn.PartitionNumber = 0;
    } else
        TRACE("DeviceType = %u, DeviceNumber = %u, PartitionNumber = %u\n", sdn.DeviceType, sdn.DeviceNumber, sdn.PartitionNumber);

    // If we've just added a partition to a whole-disk filesystem, unmount it
    if (sdn.DeviceNumber != 0xffffffff && sdn.PartitionNumber != 0) {
        LIST_ENTRY* le;

        ExAcquireResourceExclusiveLite(&pdo_list_lock, TRUE);

        le = pdo_list.Flink;
        while (le != &pdo_list) {
            pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);
            LIST_ENTRY* le2;
            BOOL changed = FALSE;

            if (pdode->vde) {
                ExAcquireResourceExclusiveLite(&pdode->child_lock, TRUE);

                le2 = pdode->children.Flink;
                while (le2 != &pdode->children) {
                    volume_child* vc = CONTAINING_RECORD(le2, volume_child, list_entry);
                    LIST_ENTRY* le3 = le2->Flink;

                    if (vc->disk_num == sdn.DeviceNumber && vc->part_num == 0) {
                        TRACE("removing device\n");

                        remove_volume_child(pdode->vde, vc, FALSE);
                        changed = TRUE;

                        break;
                    }

                    le2 = le3;
                }

                if (!changed)
                    ExReleaseResourceLite(&pdode->child_lock);
                else
                    break;
            }

            le = le->Flink;
        }

        ExReleaseResourceLite(&pdo_list_lock);
    }

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &mountmgrfo, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        goto end;
    }

    test_vol(mountmgr, devobj, devpath, sdn.DeviceNumber, sdn.PartitionNumber, gli.Length.QuadPart);

    ObDereferenceObject(mountmgrfo);

end:
    ObDereferenceObject(FileObject);
}

void volume_removal(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath) {
    LIST_ENTRY* le;
    UNICODE_STRING devpath2;

    TRACE("%.*S\n", devpath->Length / sizeof(WCHAR), devpath->Buffer);

    UNUSED(DriverObject);

    devpath2 = *devpath;

    if (devpath->Length > 4 * sizeof(WCHAR) && devpath->Buffer[0] == '\\' && (devpath->Buffer[1] == '\\' || devpath->Buffer[1] == '?') &&
        devpath->Buffer[2] == '?' && devpath->Buffer[3] == '\\') {
        devpath2.Buffer = &devpath2.Buffer[3];
        devpath2.Length -= 3 * sizeof(WCHAR);
        devpath2.MaximumLength -= 3 * sizeof(WCHAR);
    }

    ExAcquireResourceExclusiveLite(&pdo_list_lock, TRUE);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);
        LIST_ENTRY* le2;
        BOOL changed = FALSE;

        if (pdode->vde) {
            ExAcquireResourceExclusiveLite(&pdode->child_lock, TRUE);

            le2 = pdode->children.Flink;
            while (le2 != &pdode->children) {
                volume_child* vc = CONTAINING_RECORD(le2, volume_child, list_entry);
                LIST_ENTRY* le3 = le2->Flink;

                if (vc->pnp_name.Length == devpath2.Length && RtlCompareMemory(vc->pnp_name.Buffer, devpath2.Buffer, devpath2.Length) == devpath2.Length) {
                    TRACE("removing device\n");

                    remove_volume_child(pdode->vde, vc, FALSE);
                    changed = TRUE;

                    break;
                }

                le2 = le3;
            }

            if (!changed)
                ExReleaseResourceLite(&pdode->child_lock);
            else
                break;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdo_list_lock);
}

typedef struct {
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING name;
    pnp_callback func;
    PIO_WORKITEM work_item;
} pnp_callback_context;

_Function_class_(IO_WORKITEM_ROUTINE)
#ifdef __REACTOS__
static void NTAPI do_pnp_callback(PDEVICE_OBJECT DeviceObject, PVOID con) {
#else
static void do_pnp_callback(PDEVICE_OBJECT DeviceObject, PVOID con) {
#endif
    pnp_callback_context* context = con;

    UNUSED(DeviceObject);

    context->func(context->DriverObject, &context->name);

    if (context->name.Buffer)
        ExFreePool(context->name.Buffer);

    IoFreeWorkItem(context->work_item);
}

static void enqueue_pnp_callback(PDRIVER_OBJECT DriverObject, PUNICODE_STRING name, pnp_callback func) {
    PIO_WORKITEM work_item;
    pnp_callback_context* context;

    work_item = IoAllocateWorkItem(master_devobj);

    context = ExAllocatePoolWithTag(PagedPool, sizeof(pnp_callback_context), ALLOC_TAG);

    if (!context) {
        ERR("out of memory\n");
        IoFreeWorkItem(work_item);
        return;
    }

    context->DriverObject = DriverObject;

    if (name->Length > 0) {
        context->name.Buffer = ExAllocatePoolWithTag(PagedPool, name->Length, ALLOC_TAG);
        if (!context->name.Buffer) {
            ERR("out of memory\n");
            ExFreePool(context);
            IoFreeWorkItem(work_item);
            return;
        }

        RtlCopyMemory(context->name.Buffer, name->Buffer, name->Length);
        context->name.Length = context->name.MaximumLength = name->Length;
    } else {
        context->name.Length = context->name.MaximumLength = 0;
        context->name.Buffer = NULL;
    }

    context->func = func;
    context->work_item = work_item;

    IoQueueWorkItem(work_item, do_pnp_callback, DelayedWorkQueue, context);
}

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI volume_notification(PVOID NotificationStructure, PVOID Context) {
#else
NTSTATUS volume_notification(PVOID NotificationStructure, PVOID Context) {
#endif
    DEVICE_INTERFACE_CHANGE_NOTIFICATION* dicn = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Context;

    if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID)) == sizeof(GUID))
        enqueue_pnp_callback(DriverObject, dicn->SymbolicLinkName, volume_arrival);
    else if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID)) == sizeof(GUID))
        enqueue_pnp_callback(DriverObject, dicn->SymbolicLinkName, volume_removal);

    return STATUS_SUCCESS;
}

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI pnp_notification(PVOID NotificationStructure, PVOID Context) {
#else
NTSTATUS pnp_notification(PVOID NotificationStructure, PVOID Context) {
#endif
    DEVICE_INTERFACE_CHANGE_NOTIFICATION* dicn = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Context;

    if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID)) == sizeof(GUID))
        enqueue_pnp_callback(DriverObject, dicn->SymbolicLinkName, disk_arrival);
    else if (RtlCompareMemory(&dicn->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID)) == sizeof(GUID))
        enqueue_pnp_callback(DriverObject, dicn->SymbolicLinkName, volume_removal);

    return STATUS_SUCCESS;
}

static void mountmgr_process_drive(PDEVICE_OBJECT mountmgr, PUNICODE_STRING device_name) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    BOOL done = FALSE;

    ExAcquireResourceSharedLite(&pdo_list_lock, TRUE);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);
        LIST_ENTRY* le2;

        ExAcquireResourceSharedLite(&pdode->child_lock, TRUE);

        le2 = pdode->children.Flink;

        while (le2 != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le2, volume_child, list_entry);

            if (vc->devobj) {
                MOUNTDEV_NAME mdn;

                Status = dev_ioctl(vc->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(MOUNTDEV_NAME), TRUE, NULL);
                if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
                    ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                else {
                    MOUNTDEV_NAME* mdn2;
                    ULONG mdnsize = (ULONG)offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

                    mdn2 = ExAllocatePoolWithTag(NonPagedPool, mdnsize, ALLOC_TAG);
                    if (!mdn2)
                        ERR("out of memory\n");
                    else {
                        Status = dev_ioctl(vc->devobj, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, mdnsize, TRUE, NULL);
                        if (!NT_SUCCESS(Status))
                            ERR("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returned %08x\n", Status);
                        else {
                            if (mdn2->NameLength == device_name->Length && RtlCompareMemory(mdn2->Name, device_name->Buffer, device_name->Length) == device_name->Length) {
                                Status = remove_drive_letter(mountmgr, device_name);
                                if (!NT_SUCCESS(Status))
                                    ERR("remove_drive_letter returned %08x\n", Status);
                                else
                                    vc->had_drive_letter = TRUE;

                                done = TRUE;
                                break;
                            }
                        }

                        ExFreePool(mdn2);
                    }
                }
            }

            le2 = le2->Flink;
        }

        ExReleaseResourceLite(&pdode->child_lock);

        if (done)
            break;

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdo_list_lock);
}

static void mountmgr_updated(PDEVICE_OBJECT mountmgr, MOUNTMGR_MOUNT_POINTS* mmps) {
    ULONG i;

    static const WCHAR pref[] = L"\\DosDevices\\";

    for (i = 0; i < mmps->NumberOfMountPoints; i++) {
        UNICODE_STRING symlink, device_name;

        if (mmps->MountPoints[i].SymbolicLinkNameOffset != 0) {
            symlink.Buffer = (WCHAR*)(((UINT8*)mmps) + mmps->MountPoints[i].SymbolicLinkNameOffset);
            symlink.Length = symlink.MaximumLength = mmps->MountPoints[i].SymbolicLinkNameLength;
        } else {
            symlink.Buffer = NULL;
            symlink.Length = symlink.MaximumLength = 0;
        }

        if (mmps->MountPoints[i].DeviceNameOffset != 0) {
            device_name.Buffer = (WCHAR*)(((UINT8*)mmps) + mmps->MountPoints[i].DeviceNameOffset);
            device_name.Length = device_name.MaximumLength = mmps->MountPoints[i].DeviceNameLength;
        } else {
            device_name.Buffer = NULL;
            device_name.Length = device_name.MaximumLength = 0;
        }

        if (symlink.Length > sizeof(pref) - sizeof(WCHAR) &&
            RtlCompareMemory(symlink.Buffer, pref, sizeof(pref) - sizeof(WCHAR)) == sizeof(pref) - sizeof(WCHAR))
            mountmgr_process_drive(mountmgr, &device_name);
    }
}

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI mountmgr_thread(_In_ void* context) {
#else
void mountmgr_thread(_In_ void* context) {
#endif
    UNICODE_STRING mmdevpath;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT mountmgr;
    MOUNTMGR_CHANGE_NOTIFY_INFO mcni;

    UNUSED(context);

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08x\n", Status);
        return;
    }

    mcni.EpicNumber = 0;

    while (TRUE) {
        PIRP Irp;
        MOUNTMGR_MOUNT_POINT mmp;
        MOUNTMGR_MOUNT_POINTS mmps;
        IO_STATUS_BLOCK iosb;

        KeClearEvent(&mountmgr_thread_event);

        Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_CHANGE_NOTIFY, mountmgr, &mcni, sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO),
                                            &mcni, sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO), FALSE, &mountmgr_thread_event, &iosb);

        if (!Irp) {
            ERR("out of memory\n");
            break;
        }

        Status = IoCallDriver(mountmgr, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&mountmgr_thread_event, Executive, KernelMode, FALSE, NULL);
            Status = iosb.Status;
        }

        if (shutting_down)
            break;

        if (!NT_SUCCESS(Status)) {
            ERR("IOCTL_MOUNTMGR_CHANGE_NOTIFY returned %08x\n", Status);
            break;
        }

        TRACE("mountmgr changed\n");

        RtlZeroMemory(&mmp, sizeof(MOUNTMGR_MOUNT_POINT));

        Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_QUERY_POINTS, &mmp, sizeof(MOUNTMGR_MOUNT_POINT), &mmps, sizeof(MOUNTMGR_MOUNT_POINTS),
                           FALSE, NULL);

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
            ERR("IOCTL_MOUNTMGR_QUERY_POINTS 1 returned %08x\n", Status);
        else if (mmps.Size > 0) {
            MOUNTMGR_MOUNT_POINTS* mmps2;

            mmps2 = ExAllocatePoolWithTag(NonPagedPool, mmps.Size, ALLOC_TAG);
            if (!mmps2) {
                ERR("out of memory\n");
                break;
            }

            Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_QUERY_POINTS, &mmp, sizeof(MOUNTMGR_MOUNT_POINTS), mmps2, mmps.Size,
                               FALSE, NULL);
            if (!NT_SUCCESS(Status))
                ERR("IOCTL_MOUNTMGR_QUERY_POINTS returned %08x\n", Status);
            else
                mountmgr_updated(mountmgr, mmps2);

            ExFreePool(mmps2);
        }
    }

    ObDereferenceObject(FileObject);

    mountmgr_thread_handle = NULL;

    PsTerminateSystemThread(STATUS_SUCCESS);
}
