/* Copyright (c) Mark Harmstone 2019
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
#ifdef _MSC_VER
#include <ntstrsafe.h>
#endif
#else
#include <ntstrsafe.h>
#endif

extern ERESOURCE pdo_list_lock;
extern LIST_ENTRY pdo_list;
extern ERESOURCE boot_lock;
extern PDRIVER_OBJECT drvobj;

BTRFS_UUID boot_uuid; // initialized to 0
uint64_t boot_subvol = 0;

// Not in any headers? Windbg knows about it though.
#define DOE_START_PENDING 0x10

// Just as much as we need - the version in mingw is truncated still further
typedef struct {
    CSHORT Type;
    USHORT Size;
    PDEVICE_OBJECT DeviceObject;
    ULONG PowerFlags;
    void* Dope;
    ULONG ExtensionFlags;
} DEVOBJ_EXTENSION2;

static bool get_system_root() {
    NTSTATUS Status;
    HANDLE h;
    UNICODE_STRING us, target;
    OBJECT_ATTRIBUTES objatt;
    ULONG retlen = 0;
    bool second_time = false;

    static const WCHAR system_root[] = L"\\SystemRoot";
    static const WCHAR boot_device[] = L"\\Device\\BootDevice";
    static const WCHAR arc_btrfs_prefix[] = L"\\ArcName\\btrfs(";

    us.Buffer = (WCHAR*)system_root;
    us.Length = us.MaximumLength = sizeof(system_root) - sizeof(WCHAR);

    InitializeObjectAttributes(&objatt, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    while (true) {
        Status = ZwOpenSymbolicLinkObject(&h, GENERIC_READ, &objatt);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwOpenSymbolicLinkObject returned %08lx\n", Status);
            return false;
        }

        target.Length = target.MaximumLength = 0;

        Status = ZwQuerySymbolicLinkObject(h, &target, &retlen);
        if (Status != STATUS_BUFFER_TOO_SMALL) {
            ERR("ZwQuerySymbolicLinkObject returned %08lx\n", Status);
            NtClose(h);
            return false;
        }

        if (retlen == 0) {
            NtClose(h);
            return false;
        }

        target.Buffer = ExAllocatePoolWithTag(NonPagedPool, retlen, ALLOC_TAG);
        if (!target.Buffer) {
            ERR("out of memory\n");
            NtClose(h);
            return false;
        }

        target.Length = target.MaximumLength = (USHORT)retlen;

        Status = ZwQuerySymbolicLinkObject(h, &target, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwQuerySymbolicLinkObject returned %08lx\n", Status);
            NtClose(h);
            ExFreePool(target.Buffer);
            return false;
        }

        NtClose(h);

        if (second_time) {
            TRACE("boot device is %.*S\n", (int)(target.Length / sizeof(WCHAR)), target.Buffer);
        } else {
            TRACE("system root is %.*S\n", (int)(target.Length / sizeof(WCHAR)), target.Buffer);
        }

        if (!second_time && target.Length >= sizeof(boot_device) - sizeof(WCHAR) &&
            RtlCompareMemory(target.Buffer, boot_device, sizeof(boot_device) - sizeof(WCHAR)) == sizeof(boot_device) - sizeof(WCHAR)) {
            ExFreePool(target.Buffer);

            us.Buffer = (WCHAR*)boot_device;
            us.Length = us.MaximumLength = sizeof(boot_device) - sizeof(WCHAR);

            second_time = true;
        } else
            break;
    }

    if (target.Length >= sizeof(arc_btrfs_prefix) - sizeof(WCHAR) &&
        RtlCompareMemory(target.Buffer, arc_btrfs_prefix, sizeof(arc_btrfs_prefix) - sizeof(WCHAR)) == sizeof(arc_btrfs_prefix) - sizeof(WCHAR)) {
        WCHAR* s = &target.Buffer[(sizeof(arc_btrfs_prefix) / sizeof(WCHAR)) - 1];

        for (unsigned int i = 0; i < 16; i++) {
            if (*s >= '0' && *s <= '9')
                boot_uuid.uuid[i] = (*s - '0') << 4;
            else if (*s >= 'a' && *s <= 'f')
                boot_uuid.uuid[i] = (*s - 'a' + 0xa) << 4;
            else if (*s >= 'A' && *s <= 'F')
                boot_uuid.uuid[i] = (*s - 'A' + 0xa) << 4;
            else {
                ExFreePool(target.Buffer);
                return false;
            }

            s++;

            if (*s >= '0' && *s <= '9')
                boot_uuid.uuid[i] |= *s - '0';
            else if (*s >= 'a' && *s <= 'f')
                boot_uuid.uuid[i] |= *s - 'a' + 0xa;
            else if (*s >= 'A' && *s <= 'F')
                boot_uuid.uuid[i] |= *s - 'A' + 0xa;
            else {
                ExFreePool(target.Buffer);
                return false;
            }

            s++;

            if (i == 3 || i == 5 || i == 7 || i == 9) {
                if (*s != '-') {
                    ExFreePool(target.Buffer);
                    return false;
                }

                s++;
            }
        }

        if (*s != ')') {
            ExFreePool(target.Buffer);
            return false;
        }

        ExFreePool(target.Buffer);

        return true;
    }

    ExFreePool(target.Buffer);

    return false;
}

static void mountmgr_notification(BTRFS_UUID* uuid) {
    UNICODE_STRING mmdevpath;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT mountmgr;
    ULONG mmtnlen;
    MOUNTMGR_TARGET_NAME* mmtn;
    WCHAR* w;

    RtlInitUnicodeString(&mmdevpath, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&mmdevpath, FILE_READ_ATTRIBUTES, &FileObject, &mountmgr);
    if (!NT_SUCCESS(Status)) {
        ERR("IoGetDeviceObjectPointer returned %08lx\n", Status);
        return;
    }

    mmtnlen = offsetof(MOUNTMGR_TARGET_NAME, DeviceName[0]) + sizeof(BTRFS_VOLUME_PREFIX) + (36 * sizeof(WCHAR));

    mmtn = ExAllocatePoolWithTag(NonPagedPool, mmtnlen, ALLOC_TAG);
    if (!mmtn) {
        ERR("out of memory\n");
        return;
    }

    mmtn->DeviceNameLength = sizeof(BTRFS_VOLUME_PREFIX) + (36 * sizeof(WCHAR));

    RtlCopyMemory(mmtn->DeviceName, BTRFS_VOLUME_PREFIX, sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR));

    w = &mmtn->DeviceName[(sizeof(BTRFS_VOLUME_PREFIX) / sizeof(WCHAR)) - 1];

    for (unsigned int i = 0; i < 16; i++) {
        *w = hex_digit(uuid->uuid[i] >> 4); w++;
        *w = hex_digit(uuid->uuid[i] & 0xf); w++;

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *w = L'-';
            w++;
        }
    }

    *w = L'}';

    Status = dev_ioctl(mountmgr, IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION, mmtn, mmtnlen, NULL, 0, false, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION returned %08lx\n", Status);
        ExFreePool(mmtn);
        return;
    }

    ExFreePool(mmtn);
}

static void check_boot_options() {
    NTSTATUS Status;
    WCHAR* s;

    static const WCHAR pathw[] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control";
    static const WCHAR namew[] = L"SystemStartOptions";
    static const WCHAR subvol[] = L"SUBVOL=";

    _SEH2_TRY {
        HANDLE control;
        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING path;
        ULONG kvfilen = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR) + (255 * sizeof(WCHAR));
        KEY_VALUE_FULL_INFORMATION* kvfi;
        UNICODE_STRING name;
        WCHAR* options;

        path.Buffer = (WCHAR*)pathw;
        path.Length = path.MaximumLength = sizeof(pathw) - sizeof(WCHAR);

        InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        Status = ZwOpenKey(&control, KEY_QUERY_VALUE, &oa);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwOpenKey returned %08lx\n", Status);
            return;
        }

        // FIXME - don't fail if value too long (can we query for the length?)

        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
        if (!kvfi) {
            ERR("out of memory\n");
            NtClose(control);
            return;
        }

        name.Buffer = (WCHAR*)namew;
        name.Length = name.MaximumLength = sizeof(namew) - sizeof(WCHAR);

        Status = ZwQueryValueKey(control, &name, KeyValueFullInformation, kvfi,
                                 kvfilen, &kvfilen);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwQueryValueKey returned %08lx\n", Status);
            NtClose(control);
            return;
        }

        NtClose(control);

        options = (WCHAR*)((uint8_t*)kvfi + kvfi->DataOffset);
        options[kvfi->DataLength / sizeof(WCHAR)] = 0; // FIXME - make sure buffer long enough to allow this

        s = wcsstr(options, subvol);

        if (!s)
            return;

        s += (sizeof(subvol) / sizeof(WCHAR)) - 1;

        boot_subvol = 0;

        while (true) {
            if (*s >= '0' && *s <= '9') {
                boot_subvol <<= 4;
                boot_subvol |= *s - '0';
            } else if (*s >= 'a' && *s <= 'f') {
                boot_subvol <<= 4;
                boot_subvol |= *s - 'a' + 0xa;
            } else if (*s >= 'A' && *s <= 'F') {
                boot_subvol <<= 4;
                boot_subvol |= *s - 'A' + 0xa;
            } else
                break;

            s++;
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        return;
    } _SEH2_END;

    if (boot_subvol != 0) {
        TRACE("passed subvol %I64x in boot options\n", boot_subvol);
    }
}

void boot_add_device(DEVICE_OBJECT* pdo) {
    pdo_device_extension* pdode = pdo->DeviceExtension;

    AddDevice(drvobj, pdo);

    // To stop Windows sneakily setting DOE_START_PENDING
    pdode->dont_report = true;

    if (pdo->DeviceObjectExtension) {
        ((DEVOBJ_EXTENSION2*)pdo->DeviceObjectExtension)->ExtensionFlags &= ~DOE_START_PENDING;

        if (pdode && pdode->vde && pdode->vde->device)
            ((DEVOBJ_EXTENSION2*)pdode->vde->device->DeviceObjectExtension)->ExtensionFlags &= ~DOE_START_PENDING;
    }

    mountmgr_notification(&pdode->uuid);
}

void check_system_root() {
    LIST_ENTRY* le;
    PDEVICE_OBJECT pdo_to_add = NULL;

    TRACE("()\n");

    // wait for any PNP notifications in progress to finish
    ExAcquireResourceExclusiveLite(&boot_lock, TRUE);
    ExReleaseResourceLite(&boot_lock);

    if (!get_system_root())
        return;

    ExAcquireResourceSharedLite(&pdo_list_lock, true);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

        if (RtlCompareMemory(&pdode->uuid, &boot_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
            if (!pdode->vde)
                pdo_to_add = pdode->pdo;
            else if (pdode->vde->device && !(pdode->vde->device->Flags & DO_SYSTEM_BOOT_PARTITION)) { // AddDevice has beaten us to it
                NTSTATUS Status;

                pdode->vde->device->Flags |= DO_SYSTEM_BOOT_PARTITION;
                pdode->pdo->Flags |= DO_SYSTEM_BOOT_PARTITION;

                Status = IoSetDeviceInterfaceState(&pdode->vde->bus_name, false);
                if (!NT_SUCCESS(Status))
                    ERR("IoSetDeviceInterfaceState returned %08lx\n", Status);

                Status = IoSetDeviceInterfaceState(&pdode->vde->bus_name, true);
                if (!NT_SUCCESS(Status))
                    ERR("IoSetDeviceInterfaceState returned %08lx\n", Status);
            }

            break;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&pdo_list_lock);

    check_boot_options();

    // If our FS depends on volumes that aren't there when we do our IoRegisterPlugPlayNotification calls
    // in DriverEntry, bus_query_device_relations won't get called until it's too late. We need to do our
    // own call to AddDevice here as a result. We need to clear the DOE_START_PENDING bits, or NtOpenFile
    // will return STATUS_NO_SUCH_DEVICE.
    if (pdo_to_add)
        boot_add_device(pdo_to_add);
}
