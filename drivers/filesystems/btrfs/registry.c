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
#include "zstd/zstd.h"

extern UNICODE_STRING log_device, log_file, registry_path;
extern LIST_ENTRY uid_map_list, gid_map_list;
extern ERESOURCE mapping_lock;

#ifdef _DEBUG
extern HANDLE log_handle;
extern ERESOURCE log_lock;
extern PFILE_OBJECT comfo;
extern PDEVICE_OBJECT comdo;
#endif

WORK_QUEUE_ITEM wqi;

static const WCHAR option_mounted[] = L"Mounted";

NTSTATUS registry_load_volume_options(device_extension* Vcb) {
    BTRFS_UUID* uuid = &Vcb->superblock.uuid;
    mount_options* options = &Vcb->options;
    UNICODE_STRING path, ignoreus, compressus, compressforceus, compresstypeus, readonlyus, zliblevelus, flushintervalus,
                   maxinlineus, subvolidus, skipbalanceus, nobarrierus, notrimus, clearcacheus, allowdegradedus, zstdlevelus,
                   norootdirus, nodatacowus;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    ULONG i, j, kvfilen, index, retlen;
    KEY_VALUE_FULL_INFORMATION* kvfi = NULL;
    HANDLE h;

    options->compress = mount_compress;
    options->compress_force = mount_compress_force;
    options->compress_type = mount_compress_type > BTRFS_COMPRESSION_ZSTD ? 0 : mount_compress_type;
    options->readonly = mount_readonly;
    options->zlib_level = mount_zlib_level;
    options->zstd_level = mount_zstd_level;
    options->flush_interval = mount_flush_interval;
    options->max_inline = min(mount_max_inline, Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - sizeof(EXTENT_DATA) + 1);
    options->skip_balance = mount_skip_balance;
    options->no_barrier = mount_no_barrier;
    options->no_trim = mount_no_trim;
    options->clear_cache = mount_clear_cache;
    options->allow_degraded = mount_allow_degraded;
    options->subvol_id = 0;
    options->no_root_dir = mount_no_root_dir;
    options->nodatacow = mount_nodatacow;

    path.Length = path.MaximumLength = registry_path.Length + (37 * sizeof(WCHAR));
    path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

    if (!path.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
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

    kvfilen = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR) + (255 * sizeof(WCHAR));
    kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);
    if (!kvfi) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwOpenKey(&h, KEY_QUERY_VALUE, &oa);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
        goto end;
    } else if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenKey returned %08lx\n", Status);
        goto end;
    }

    index = 0;

    RtlInitUnicodeString(&ignoreus, L"Ignore");
    RtlInitUnicodeString(&compressus, L"Compress");
    RtlInitUnicodeString(&compressforceus, L"CompressForce");
    RtlInitUnicodeString(&compresstypeus, L"CompressType");
    RtlInitUnicodeString(&readonlyus, L"Readonly");
    RtlInitUnicodeString(&zliblevelus, L"ZlibLevel");
    RtlInitUnicodeString(&flushintervalus, L"FlushInterval");
    RtlInitUnicodeString(&maxinlineus, L"MaxInline");
    RtlInitUnicodeString(&subvolidus, L"SubvolId");
    RtlInitUnicodeString(&skipbalanceus, L"SkipBalance");
    RtlInitUnicodeString(&nobarrierus, L"NoBarrier");
    RtlInitUnicodeString(&notrimus, L"NoTrim");
    RtlInitUnicodeString(&clearcacheus, L"ClearCache");
    RtlInitUnicodeString(&allowdegradedus, L"AllowDegraded");
    RtlInitUnicodeString(&zstdlevelus, L"ZstdLevel");
    RtlInitUnicodeString(&norootdirus, L"NoRootDir");
    RtlInitUnicodeString(&nodatacowus, L"NoDataCOW");

    do {
        Status = ZwEnumerateValueKey(h, index, KeyValueFullInformation, kvfi, kvfilen, &retlen);

        index++;

        if (NT_SUCCESS(Status)) {
            UNICODE_STRING us;

            us.Length = us.MaximumLength = (USHORT)kvfi->NameLength;
            us.Buffer = kvfi->Name;

            if (FsRtlAreNamesEqual(&ignoreus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->ignore = *val != 0 ? true : false;
            } else if (FsRtlAreNamesEqual(&compressus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->compress = *val != 0 ? true : false;
            } else if (FsRtlAreNamesEqual(&compressforceus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->compress_force = *val != 0 ? true : false;
            } else if (FsRtlAreNamesEqual(&compresstypeus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->compress_type = (uint8_t)(*val > BTRFS_COMPRESSION_ZSTD ? 0 : *val);
            } else if (FsRtlAreNamesEqual(&readonlyus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->readonly = *val != 0 ? true : false;
            } else if (FsRtlAreNamesEqual(&zliblevelus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->zlib_level = *val;
            } else if (FsRtlAreNamesEqual(&flushintervalus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->flush_interval = *val;
            } else if (FsRtlAreNamesEqual(&maxinlineus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->max_inline = min(*val, Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - sizeof(EXTENT_DATA) + 1);
            } else if (FsRtlAreNamesEqual(&subvolidus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_QWORD) {
                uint64_t* val = (uint64_t*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->subvol_id = *val;
            } else if (FsRtlAreNamesEqual(&skipbalanceus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->skip_balance = *val;
            } else if (FsRtlAreNamesEqual(&nobarrierus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->no_barrier = *val;
            } else if (FsRtlAreNamesEqual(&notrimus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->no_trim = *val;
            } else if (FsRtlAreNamesEqual(&clearcacheus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->clear_cache = *val;
            } else if (FsRtlAreNamesEqual(&allowdegradedus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->allow_degraded = *val;
            } else if (FsRtlAreNamesEqual(&zstdlevelus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->zstd_level = *val;
            } else if (FsRtlAreNamesEqual(&norootdirus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->no_root_dir = *val;
            } else if (FsRtlAreNamesEqual(&nodatacowus, &us, true, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((uint8_t*)kvfi + kvfi->DataOffset);

                options->nodatacow = *val;
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES) {
            ERR("ZwEnumerateValueKey returned %08lx\n", Status);
            goto end2;
        }
    } while (NT_SUCCESS(Status));

    if (!options->compress && options->compress_force)
        options->compress = true;

    if (options->zlib_level > 9)
        options->zlib_level = 9;

    if (options->zstd_level > (uint32_t)ZSTD_maxCLevel())
        options->zstd_level = ZSTD_maxCLevel();

    if (options->flush_interval == 0)
        options->flush_interval = mount_flush_interval;

    Status = STATUS_SUCCESS;

end2:
    ZwClose(h);

end:
    ExFreePool(path.Buffer);

    if (kvfi)
        ExFreePool(kvfi);

    return Status;
}

NTSTATUS registry_mark_volume_mounted(BTRFS_UUID* uuid) {
    UNICODE_STRING path, mountedus;
    ULONG i, j;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    HANDLE h;
    DWORD data;

    path.Length = path.MaximumLength = registry_path.Length + (37 * sizeof(WCHAR));
    path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

    if (!path.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
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

    Status = ZwCreateKey(&h, KEY_SET_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08lx\n", Status);
        goto end;
    }

    mountedus.Buffer = (WCHAR*)option_mounted;
    mountedus.Length = mountedus.MaximumLength = sizeof(option_mounted) - sizeof(WCHAR);

    data = 1;

    Status = ZwSetValueKey(h, &mountedus, 0, REG_DWORD, &data, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        ERR("ZwSetValueKey returned %08lx\n", Status);
        goto end2;
    }

    Status = STATUS_SUCCESS;

end2:
    ZwClose(h);

end:
    ExFreePool(path.Buffer);

    return Status;
}

static NTSTATUS registry_mark_volume_unmounted_path(PUNICODE_STRING path) {
    HANDLE h;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    ULONG index, kvbilen = sizeof(KEY_VALUE_BASIC_INFORMATION) - sizeof(WCHAR) + (255 * sizeof(WCHAR)), retlen;
    KEY_VALUE_BASIC_INFORMATION* kvbi;
    bool has_options = false;
    UNICODE_STRING mountedus;

    // If a volume key has any options in it, we set Mounted to 0 and return. Otherwise,
    // we delete the whole thing.

    kvbi = ExAllocatePoolWithTag(PagedPool, kvbilen, ALLOC_TAG);
    if (!kvbi) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeObjectAttributes(&oa, path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwOpenKey(&h, KEY_QUERY_VALUE | KEY_SET_VALUE | DELETE, &oa);
    if (!NT_SUCCESS(Status)) {
        ERR("ZwOpenKey returned %08lx\n", Status);
        goto end;
    }

    index = 0;

    mountedus.Buffer = (WCHAR*)option_mounted;
    mountedus.Length = mountedus.MaximumLength = sizeof(option_mounted) - sizeof(WCHAR);

    do {
        Status = ZwEnumerateValueKey(h, index, KeyValueBasicInformation, kvbi, kvbilen, &retlen);

        index++;

        if (NT_SUCCESS(Status)) {
            UNICODE_STRING us;

            us.Length = us.MaximumLength = (USHORT)kvbi->NameLength;
            us.Buffer = kvbi->Name;

            if (!FsRtlAreNamesEqual(&mountedus, &us, true, NULL)) {
                has_options = true;
                break;
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES) {
            ERR("ZwEnumerateValueKey returned %08lx\n", Status);
            goto end2;
        }
    } while (NT_SUCCESS(Status));

    if (has_options) {
        DWORD data = 0;

        Status = ZwSetValueKey(h, &mountedus, 0, REG_DWORD, &data, sizeof(DWORD));
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08lx\n", Status);
            goto end2;
        }
    } else {
        Status = ZwDeleteKey(h);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwDeleteKey returned %08lx\n", Status);
            goto end2;
        }
    }

    Status = STATUS_SUCCESS;

end2:
    ZwClose(h);

end:
    ExFreePool(kvbi);

    return Status;
}

NTSTATUS registry_mark_volume_unmounted(BTRFS_UUID* uuid) {
    UNICODE_STRING path;
    NTSTATUS Status;
    ULONG i, j;

    path.Length = path.MaximumLength = registry_path.Length + (37 * sizeof(WCHAR));
    path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

    if (!path.Buffer) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
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

    Status = registry_mark_volume_unmounted_path(&path);
    if (!NT_SUCCESS(Status)) {
        ERR("registry_mark_volume_unmounted_path returned %08lx\n", Status);
        goto end;
    }

    Status = STATUS_SUCCESS;

end:
    ExFreePool(path.Buffer);

    return Status;
}

#define is_hex(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

static bool is_uuid(ULONG namelen, WCHAR* name) {
    ULONG i;

    if (namelen != 36 * sizeof(WCHAR))
        return false;

    for (i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (name[i] != '-')
                return false;
        } else if (!is_hex(name[i]))
            return false;
    }

    return true;
}

typedef struct {
    UNICODE_STRING name;
    LIST_ENTRY list_entry;
} key_name;

static void reset_subkeys(HANDLE h, PUNICODE_STRING reg_path) {
    NTSTATUS Status;
    KEY_BASIC_INFORMATION* kbi;
    ULONG kbilen = sizeof(KEY_BASIC_INFORMATION) - sizeof(WCHAR) + (255 * sizeof(WCHAR)), retlen, index = 0;
    LIST_ENTRY key_names, *le;

    InitializeListHead(&key_names);

    kbi = ExAllocatePoolWithTag(PagedPool, kbilen, ALLOC_TAG);
    if (!kbi) {
        ERR("out of memory\n");
        return;
    }

    do {
        Status = ZwEnumerateKey(h, index, KeyBasicInformation, kbi, kbilen, &retlen);

        index++;

        if (NT_SUCCESS(Status)) {
            key_name* kn;

            TRACE("key: %.*S\n", (int)(kbi->NameLength / sizeof(WCHAR)), kbi->Name);

            if (is_uuid(kbi->NameLength, kbi->Name)) {
                kn = ExAllocatePoolWithTag(PagedPool, sizeof(key_name), ALLOC_TAG);
                if (!kn) {
                    ERR("out of memory\n");
                    goto end;
                }

                kn->name.Length = kn->name.MaximumLength = (USHORT)min(0xffff, kbi->NameLength);
                kn->name.Buffer = ExAllocatePoolWithTag(PagedPool, kn->name.MaximumLength, ALLOC_TAG);

                if (!kn->name.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kn);
                    goto end;
                }

                RtlCopyMemory(kn->name.Buffer, kbi->Name, kn->name.Length);

                InsertTailList(&key_names, &kn->list_entry);
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES)
            ERR("ZwEnumerateKey returned %08lx\n", Status);
    } while (NT_SUCCESS(Status));

    le = key_names.Flink;
    while (le != &key_names) {
        key_name* kn = CONTAINING_RECORD(le, key_name, list_entry);
        UNICODE_STRING path;

        path.Length = path.MaximumLength = reg_path->Length + sizeof(WCHAR) + kn->name.Length;
        path.Buffer = ExAllocatePoolWithTag(PagedPool, path.Length, ALLOC_TAG);

        if (!path.Buffer) {
            ERR("out of memory\n");
            goto end;
        }

        RtlCopyMemory(path.Buffer, reg_path->Buffer, reg_path->Length);
        path.Buffer[reg_path->Length / sizeof(WCHAR)] = '\\';
        RtlCopyMemory(&path.Buffer[(reg_path->Length / sizeof(WCHAR)) + 1], kn->name.Buffer, kn->name.Length);

        Status = registry_mark_volume_unmounted_path(&path);
        if (!NT_SUCCESS(Status))
            WARN("registry_mark_volume_unmounted_path returned %08lx\n", Status);

        ExFreePool(path.Buffer);

        le = le->Flink;
    }

end:
    while (!IsListEmpty(&key_names)) {
        key_name* kn;

        le = RemoveHeadList(&key_names);
        kn = CONTAINING_RECORD(le, key_name, list_entry);

        if (kn->name.Buffer)
            ExFreePool(kn->name.Buffer);

        ExFreePool(kn);
    }

    ExFreePool(kbi);
}

static void read_mappings(PUNICODE_STRING regpath) {
    WCHAR* path;
    UNICODE_STRING us;
    HANDLE h;
    OBJECT_ATTRIBUTES oa;
    ULONG dispos;
    NTSTATUS Status;

    static const WCHAR mappings[] = L"\\Mappings";

    while (!IsListEmpty(&uid_map_list)) {
        uid_map* um = CONTAINING_RECORD(RemoveHeadList(&uid_map_list), uid_map, listentry);

        if (um->sid) ExFreePool(um->sid);
        ExFreePool(um);
    }

    path = ExAllocatePoolWithTag(PagedPool, regpath->Length + sizeof(mappings) - sizeof(WCHAR), ALLOC_TAG);
    if (!path) {
        ERR("out of memory\n");
        return;
    }

    RtlCopyMemory(path, regpath->Buffer, regpath->Length);
    RtlCopyMemory((uint8_t*)path + regpath->Length, mappings, sizeof(mappings) - sizeof(WCHAR));

    us.Buffer = path;
    us.Length = us.MaximumLength = regpath->Length + sizeof(mappings) - sizeof(WCHAR);

    InitializeObjectAttributes(&oa, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08lx\n", Status);
        ExFreePool(path);
        return;
    }

    if (dispos == REG_OPENED_EXISTING_KEY) {
        KEY_VALUE_FULL_INFORMATION* kvfi;
        ULONG kvfilen, retlen, i;

        kvfilen = sizeof(KEY_VALUE_FULL_INFORMATION) + 256;
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);

        if (!kvfi) {
            ERR("out of memory\n");
            ExFreePool(path);
            ZwClose(h);
            return;
        }

        i = 0;
        do {
            Status = ZwEnumerateValueKey(h, i, KeyValueFullInformation, kvfi, kvfilen, &retlen);

            if (NT_SUCCESS(Status) && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                uint32_t val = 0;

                RtlCopyMemory(&val, (uint8_t*)kvfi + kvfi->DataOffset, min(kvfi->DataLength, sizeof(uint32_t)));

                TRACE("entry %lu = %.*S = %u\n", i, (int)(kvfi->NameLength / sizeof(WCHAR)), kvfi->Name, val);

                add_user_mapping(kvfi->Name, kvfi->NameLength / sizeof(WCHAR), val);
            }

            i = i + 1;
        } while (Status != STATUS_NO_MORE_ENTRIES);

        ExFreePool(kvfi);
    }

    ZwClose(h);

    ExFreePool(path);
}

static void read_group_mappings(PUNICODE_STRING regpath) {
    WCHAR* path;
    UNICODE_STRING us;
    HANDLE h;
    OBJECT_ATTRIBUTES oa;
    ULONG dispos;
    NTSTATUS Status;

    static const WCHAR mappings[] = L"\\GroupMappings";

    while (!IsListEmpty(&gid_map_list)) {
        gid_map* gm = CONTAINING_RECORD(RemoveHeadList(&gid_map_list), gid_map, listentry);

        if (gm->sid) ExFreePool(gm->sid);
        ExFreePool(gm);
    }

    path = ExAllocatePoolWithTag(PagedPool, regpath->Length + sizeof(mappings) - sizeof(WCHAR), ALLOC_TAG);
    if (!path) {
        ERR("out of memory\n");
        return;
    }

    RtlCopyMemory(path, regpath->Buffer, regpath->Length);
    RtlCopyMemory((uint8_t*)path + regpath->Length, mappings, sizeof(mappings) - sizeof(WCHAR));

    us.Buffer = path;
    us.Length = us.MaximumLength = regpath->Length + sizeof(mappings) - sizeof(WCHAR);

    InitializeObjectAttributes(&oa, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08lx\n", Status);
        ExFreePool(path);
        return;
    }

    ExFreePool(path);

    if (dispos == REG_OPENED_EXISTING_KEY) {
        KEY_VALUE_FULL_INFORMATION* kvfi;
        ULONG kvfilen, retlen, i;

        kvfilen = sizeof(KEY_VALUE_FULL_INFORMATION) + 256;
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);

        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }

        i = 0;
        do {
            Status = ZwEnumerateValueKey(h, i, KeyValueFullInformation, kvfi, kvfilen, &retlen);

            if (NT_SUCCESS(Status) && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                uint32_t val = 0;

                RtlCopyMemory(&val, (uint8_t*)kvfi + kvfi->DataOffset, min(kvfi->DataLength, sizeof(uint32_t)));

                TRACE("entry %lu = %.*S = %u\n", i, (int)(kvfi->NameLength / sizeof(WCHAR)), kvfi->Name, val);

                add_group_mapping(kvfi->Name, kvfi->NameLength / sizeof(WCHAR), val);
            }

            i = i + 1;
        } while (Status != STATUS_NO_MORE_ENTRIES);

        ExFreePool(kvfi);
    } else if (dispos == REG_CREATED_NEW_KEY) {
        static const WCHAR builtin_users[] = L"S-1-5-32-545";

        UNICODE_STRING us2;
        DWORD val;

        // If we're creating the key for the first time, we add a default mapping of
        // BUILTIN\Users to gid 100, which ought to correspond to the "users" group on Linux.

        us2.Length = us2.MaximumLength = sizeof(builtin_users) - sizeof(WCHAR);
        us2.Buffer = ExAllocatePoolWithTag(PagedPool, us2.MaximumLength, ALLOC_TAG);

        if (us2.Buffer) {
            RtlCopyMemory(us2.Buffer, builtin_users, us2.Length);

            val = 100;
            Status = ZwSetValueKey(h, &us2, 0, REG_DWORD, &val, sizeof(DWORD));
            if (!NT_SUCCESS(Status)) {
                ERR("ZwSetValueKey returned %08lx\n", Status);
                ZwClose(h);
                return;
            }

            add_group_mapping(us2.Buffer, us2.Length / sizeof(WCHAR), val);

            ExFreePool(us2.Buffer);
        }
    }

    ZwClose(h);
}

static void get_registry_value(HANDLE h, WCHAR* string, ULONG type, void* val, ULONG size) {
    ULONG kvfilen;
    KEY_VALUE_FULL_INFORMATION* kvfi;
    UNICODE_STRING us;
    NTSTATUS Status;

    RtlInitUnicodeString(&us, string);

    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);

        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }

        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

        if (NT_SUCCESS(Status)) {
            if (kvfi->Type == type && kvfi->DataLength >= size) {
                RtlCopyMemory(val, ((uint8_t*)kvfi) + kvfi->DataOffset, size);
            } else {
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08lx\n", Status);
                }

                Status = ZwSetValueKey(h, &us, 0, type, val, size);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwSetValueKey returned %08lx\n", Status);
                }
            }
        }

        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, type, val, size);

        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08lx\n", Status);
        }
    } else {
        ERR("ZwQueryValueKey returned %08lx\n", Status);
    }
}

void read_registry(PUNICODE_STRING regpath, bool refresh) {
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    HANDLE h;
    ULONG dispos;
#ifdef _DEBUG
    KEY_VALUE_FULL_INFORMATION* kvfi;
    ULONG kvfilen, old_debug_log_level = debug_log_level;
    UNICODE_STRING us, old_log_file, old_log_device;

    static const WCHAR def_log_file[] = L"\\??\\C:\\btrfs.log";
#endif

    ExAcquireResourceExclusiveLite(&mapping_lock, true);

    read_mappings(regpath);
    read_group_mappings(regpath);

    ExReleaseResourceLite(&mapping_lock);

    InitializeObjectAttributes(&oa, regpath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwCreateKey(&h, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);

    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08lx\n", Status);
        return;
    }

    if (!refresh)
        reset_subkeys(h, regpath);

    get_registry_value(h, L"Compress", REG_DWORD, &mount_compress, sizeof(mount_compress));
    get_registry_value(h, L"CompressForce", REG_DWORD, &mount_compress_force, sizeof(mount_compress_force));
    get_registry_value(h, L"CompressType", REG_DWORD, &mount_compress_type, sizeof(mount_compress_type));
    get_registry_value(h, L"ZlibLevel", REG_DWORD, &mount_zlib_level, sizeof(mount_zlib_level));
    get_registry_value(h, L"FlushInterval", REG_DWORD, &mount_flush_interval, sizeof(mount_flush_interval));
    get_registry_value(h, L"MaxInline", REG_DWORD, &mount_max_inline, sizeof(mount_max_inline));
    get_registry_value(h, L"SkipBalance", REG_DWORD, &mount_skip_balance, sizeof(mount_skip_balance));
    get_registry_value(h, L"NoBarrier", REG_DWORD, &mount_no_barrier, sizeof(mount_no_barrier));
    get_registry_value(h, L"NoTrim", REG_DWORD, &mount_no_trim, sizeof(mount_no_trim));
    get_registry_value(h, L"ClearCache", REG_DWORD, &mount_clear_cache, sizeof(mount_clear_cache));
    get_registry_value(h, L"AllowDegraded", REG_DWORD, &mount_allow_degraded, sizeof(mount_allow_degraded));
    get_registry_value(h, L"Readonly", REG_DWORD, &mount_readonly, sizeof(mount_readonly));
    get_registry_value(h, L"ZstdLevel", REG_DWORD, &mount_zstd_level, sizeof(mount_zstd_level));
    get_registry_value(h, L"NoRootDir", REG_DWORD, &mount_no_root_dir, sizeof(mount_no_root_dir));
    get_registry_value(h, L"NoDataCOW", REG_DWORD, &mount_nodatacow, sizeof(mount_nodatacow));

    if (!refresh)
        get_registry_value(h, L"NoPNP", REG_DWORD, &no_pnp, sizeof(no_pnp));

    if (mount_flush_interval == 0)
        mount_flush_interval = 1;

#ifdef _DEBUG
    get_registry_value(h, L"DebugLogLevel", REG_DWORD, &debug_log_level, sizeof(debug_log_level));

    RtlInitUnicodeString(&us, L"LogDevice");

    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

    old_log_device = log_device;

    log_device.Length = log_device.MaximumLength = 0;
    log_device.Buffer = NULL;

    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);

        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }

        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

        if (NT_SUCCESS(Status)) {
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_device.Length = log_device.MaximumLength = (USHORT)min(0xffff, kvfi->DataLength);
                log_device.Buffer = ExAllocatePoolWithTag(PagedPool, log_device.MaximumLength, ALLOC_TAG);

                if (!log_device.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_device.Buffer, ((uint8_t*)kvfi) + kvfi->DataOffset, log_device.Length);

                if (log_device.Buffer[(log_device.Length / sizeof(WCHAR)) - 1] == 0)
                    log_device.Length -= sizeof(WCHAR);
            } else {
                ERR("LogDevice was type %lu, length %lu\n", kvfi->Type, kvfi->DataLength);

                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08lx\n", Status);
                }
            }
        }

        ExFreePool(kvfi);
    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("ZwQueryValueKey returned %08lx\n", Status);
    }

    ExAcquireResourceExclusiveLite(&log_lock, true);

    if (refresh && (log_device.Length != old_log_device.Length || RtlCompareMemory(log_device.Buffer, old_log_device.Buffer, log_device.Length) != log_device.Length ||
        (!comfo && log_device.Length > 0) || (old_debug_log_level == 0 && debug_log_level > 0) || (old_debug_log_level > 0 && debug_log_level == 0))) {
        if (comfo)
            ObDereferenceObject(comfo);

        if (log_handle) {
            ZwClose(log_handle);
            log_handle = NULL;
        }

        comfo = NULL;
        comdo = NULL;

        if (log_device.Length > 0 && debug_log_level > 0) {
            Status = IoGetDeviceObjectPointer(&log_device, FILE_WRITE_DATA, &comfo, &comdo);
            if (!NT_SUCCESS(Status))
                DbgPrint("IoGetDeviceObjectPointer returned %08lx\n", Status);
        }
    }

    ExReleaseResourceLite(&log_lock);

    if (old_log_device.Buffer)
        ExFreePool(old_log_device.Buffer);

    RtlInitUnicodeString(&us, L"LogFile");

    kvfi = NULL;
    kvfilen = 0;
    Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

    old_log_file = log_file;

    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && kvfilen > 0) {
        kvfi = ExAllocatePoolWithTag(PagedPool, kvfilen, ALLOC_TAG);

        if (!kvfi) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }

        Status = ZwQueryValueKey(h, &us, KeyValueFullInformation, kvfi, kvfilen, &kvfilen);

        if (NT_SUCCESS(Status)) {
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_file.Length = log_file.MaximumLength = (USHORT)min(0xffff, kvfi->DataLength);
                log_file.Buffer = ExAllocatePoolWithTag(PagedPool, log_file.MaximumLength, ALLOC_TAG);

                if (!log_file.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_file.Buffer, ((uint8_t*)kvfi) + kvfi->DataOffset, log_file.Length);

                if (log_file.Buffer[(log_file.Length / sizeof(WCHAR)) - 1] == 0)
                    log_file.Length -= sizeof(WCHAR);
            } else {
                ERR("LogFile was type %lu, length %lu\n", kvfi->Type, kvfi->DataLength);

                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status))
                    ERR("ZwDeleteValueKey returned %08lx\n", Status);

                log_file.Length = 0;
            }
        } else {
            ERR("ZwQueryValueKey returned %08lx\n", Status);
            log_file.Length = 0;
        }

        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, REG_SZ, (void*)def_log_file, sizeof(def_log_file));

        if (!NT_SUCCESS(Status))
            ERR("ZwSetValueKey returned %08lx\n", Status);

        log_file.Length = 0;
    } else {
        ERR("ZwQueryValueKey returned %08lx\n", Status);
        log_file.Length = 0;
    }

    if (log_file.Length == 0) {
        log_file.Length = log_file.MaximumLength = sizeof(def_log_file) - sizeof(WCHAR);
        log_file.Buffer = ExAllocatePoolWithTag(PagedPool, log_file.MaximumLength, ALLOC_TAG);

        if (!log_file.Buffer) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }

        RtlCopyMemory(log_file.Buffer, def_log_file, log_file.Length);
    }

    ExAcquireResourceExclusiveLite(&log_lock, true);

    if (refresh && (log_file.Length != old_log_file.Length || RtlCompareMemory(log_file.Buffer, old_log_file.Buffer, log_file.Length) != log_file.Length ||
        (!log_handle && log_file.Length > 0) || (old_debug_log_level == 0 && debug_log_level > 0) || (old_debug_log_level > 0 && debug_log_level == 0))) {
        if (log_handle) {
            ZwClose(log_handle);
            log_handle = NULL;
        }

        if (!comfo && log_file.Length > 0 && refresh && debug_log_level > 0) {
            IO_STATUS_BLOCK iosb;

            InitializeObjectAttributes(&oa, &log_file, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

            Status = ZwCreateFile(&log_handle, FILE_WRITE_DATA, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                                  FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_ALERT, NULL, 0);
            if (!NT_SUCCESS(Status)) {
                DbgPrint("ZwCreateFile returned %08lx\n", Status);
                log_handle = NULL;
            }
        }
    }

    ExReleaseResourceLite(&log_lock);

    if (old_log_file.Buffer)
        ExFreePool(old_log_file.Buffer);
#endif

    ZwClose(h);
}

_Function_class_(WORKER_THREAD_ROUTINE)
static void __stdcall registry_work_item(PVOID Parameter) {
    NTSTATUS Status;
    HANDLE regh = (HANDLE)Parameter;
    IO_STATUS_BLOCK iosb;

    TRACE("registry changed\n");

    read_registry(&registry_path, true);

    Status = ZwNotifyChangeKey(regh, NULL, (PVOID)&wqi, (PVOID)DelayedWorkQueue, &iosb, REG_NOTIFY_CHANGE_LAST_SET, true, NULL, 0, true);
    if (!NT_SUCCESS(Status))
        ERR("ZwNotifyChangeKey returned %08lx\n", Status);
}

void watch_registry(HANDLE regh) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    ExInitializeWorkItem(&wqi, registry_work_item, regh);

    Status = ZwNotifyChangeKey(regh, NULL, (PVOID)&wqi, (PVOID)DelayedWorkQueue, &iosb, REG_NOTIFY_CHANGE_LAST_SET, true, NULL, 0, true);
    if (!NT_SUCCESS(Status))
        ERR("ZwNotifyChangeKey returned %08lx\n", Status);
}
