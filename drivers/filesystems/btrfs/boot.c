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

#ifndef _MSC_VER
NTSTATUS RtlUnicodeStringPrintf(PUNICODE_STRING DestinationString, const WCHAR* pszFormat, ...); // not in mingw
#endif

static bool get_system_root_partition(uint32_t* disk_num, uint32_t* partition_num) {
    NTSTATUS Status;
    HANDLE h;
    UNICODE_STRING us, target;
    OBJECT_ATTRIBUTES objatt;
    WCHAR* s;
    ULONG retlen = 0, left;

    static const WCHAR system_root[] = L"\\SystemRoot";
    static const WCHAR arc_prefix[] = L"\\ArcName\\multi(0)disk(0)rdisk(";
    static const WCHAR arc_middle[] = L")partition(";

    us.Buffer = (WCHAR*)system_root;
    us.Length = us.MaximumLength = sizeof(system_root) - sizeof(WCHAR);

    InitializeObjectAttributes(&objatt, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwOpenSymbolicLinkObject(&h, GENERIC_READ, &objatt);
    if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenSymbolicLinkObject returned %08x\n", Status);
        return false;
    }

    target.Length = target.MaximumLength = 0;

    Status = ZwQuerySymbolicLinkObject(h, &target, &retlen);
    if (Status != STATUS_BUFFER_TOO_SMALL) {
        ERR("ZwQuerySymbolicLinkObject returned %08x\n", Status);
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
        ERR("ZwQuerySymbolicLinkObject returned %08x\n", Status);
        NtClose(h);
        ExFreePool(target.Buffer);
        return false;
    }

    NtClose(h);

    TRACE("system root is %.*S\n", target.Length / sizeof(WCHAR), target.Buffer);

    if (target.Length <= sizeof(arc_prefix) - sizeof(WCHAR) ||
        RtlCompareMemory(target.Buffer, arc_prefix, sizeof(arc_prefix) - sizeof(WCHAR)) != sizeof(arc_prefix) - sizeof(WCHAR)) {
        ExFreePool(target.Buffer);
        return false;
    }

    s = &target.Buffer[(sizeof(arc_prefix) / sizeof(WCHAR)) - 1];
    left = ((target.Length - sizeof(arc_prefix)) / sizeof(WCHAR)) + 1;

    if (left == 0 || s[0] < '0' || s[0] > '9') {
        ExFreePool(target.Buffer);
        return false;
    }

    *disk_num = 0;

    while (left > 0 && s[0] >= '0' && s[0] <= '9') {
        *disk_num *= 10;
        *disk_num += s[0] - '0';
        s++;
        left--;
    }

    if (left <= (sizeof(arc_middle) / sizeof(WCHAR)) - 1 ||
        RtlCompareMemory(s, arc_middle, sizeof(arc_middle) - sizeof(WCHAR)) != sizeof(arc_middle) - sizeof(WCHAR)) {
        ExFreePool(target.Buffer);
        return false;
    }

    s = &s[(sizeof(arc_middle) / sizeof(WCHAR)) - 1];
    left -= (sizeof(arc_middle) / sizeof(WCHAR)) - 1;

    if (left == 0 || s[0] < '0' || s[0] > '9') {
        ExFreePool(target.Buffer);
        return false;
    }

    *partition_num = 0;

    while (left > 0 && s[0] >= '0' && s[0] <= '9') {
        *partition_num *= 10;
        *partition_num += s[0] - '0';
        s++;
        left--;
    }

    ExFreePool(target.Buffer);

    return true;
}

static void change_symlink(uint32_t disk_num, uint32_t partition_num, BTRFS_UUID* uuid) {
    NTSTATUS Status;
    UNICODE_STRING us, us2;
    WCHAR symlink[60], target[(sizeof(BTRFS_VOLUME_PREFIX) / sizeof(WCHAR)) + 36], *w;
#ifdef __REACTOS__
    unsigned int i;
#endif

    us.Buffer = symlink;
    us.Length = 0;
    us.MaximumLength = sizeof(symlink);

    Status = RtlUnicodeStringPrintf(&us, L"\\Device\\Harddisk%u\\Partition%u", disk_num, partition_num);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlUnicodeStringPrintf returned %08x\n", Status);
        return;
    }

    Status = IoDeleteSymbolicLink(&us);
    if (!NT_SUCCESS(Status))
        ERR("IoDeleteSymbolicLink returned %08x\n", Status);

    RtlCopyMemory(target, BTRFS_VOLUME_PREFIX, sizeof(BTRFS_VOLUME_PREFIX) - sizeof(WCHAR));

    w = &target[(sizeof(BTRFS_VOLUME_PREFIX) / sizeof(WCHAR)) - 1];

#ifndef __REACTOS__
    for (unsigned int i = 0; i < 16; i++) {
#else
    for (i = 0; i < 16; i++) {
#endif
        *w = hex_digit(uuid->uuid[i] >> 4); w++;
        *w = hex_digit(uuid->uuid[i] & 0xf); w++;

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *w = L'-';
            w++;
        }
    }

    *w = L'}';

    us2.Buffer = target;
    us2.Length = us2.MaximumLength = sizeof(target);

    Status = IoCreateSymbolicLink(&us, &us2);
    if (!NT_SUCCESS(Status))
        ERR("IoCreateSymbolicLink returned %08x\n", Status);
}

/* If booting from Btrfs, Windows will pass the device object for the raw partition to
 * mount_vol - which is no good to us, as we only use the \Device\Btrfs{} devices we
 * create so that RAID works correctly.
 * At the time check_system_root gets called, \SystemRoot is a symlink to the ARC device,
 * e.g. \ArcName\multi(0)disk(0)rdisk(0)partition(1)\Windows. We can't change the symlink,
 * as it gets clobbered by IopReassignSystemRoot shortly afterwards, and we can't touch
 * the \ArcName symlinks as they haven't been created yet. Instead, we need to change the
 * symlink \Device\HarddiskX\PartitionY, which is what the ArcName symlink will shortly
 * point to.
 */
void __stdcall check_system_root(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count) {
    uint32_t disk_num, partition_num;
    LIST_ENTRY* le;
    bool done = false;

    TRACE("(%p, %p, %u)\n", DriverObject, Context, Count);

    if (!get_system_root_partition(&disk_num, &partition_num))
        return;

    TRACE("system boot partition is disk %u, partition %u\n", disk_num, partition_num);

    ExAcquireResourceSharedLite(&pdo_list_lock, true);

    le = pdo_list.Flink;
    while (le != &pdo_list) {
        LIST_ENTRY* le2;
        pdo_device_extension* pdode = CONTAINING_RECORD(le, pdo_device_extension, list_entry);

        ExAcquireResourceSharedLite(&pdode->child_lock, true);

        le2 = pdode->children.Flink;

        while (le2 != &pdode->children) {
            volume_child* vc = CONTAINING_RECORD(le2, volume_child, list_entry);

            if (vc->disk_num == disk_num && vc->part_num == partition_num) {
                change_symlink(disk_num, partition_num, &pdode->uuid);
                done = true;
                break;
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
