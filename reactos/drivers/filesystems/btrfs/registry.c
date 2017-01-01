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

extern UNICODE_STRING log_device, log_file, registry_path;

static WCHAR option_mounted[] = L"Mounted";

#define hex_digit(c) ((c) >= 0 && (c) <= 9) ? ((c) + '0') : ((c) - 10 + 'a')

NTSTATUS registry_load_volume_options(device_extension* Vcb) {
    BTRFS_UUID* uuid = &Vcb->superblock.uuid;
    mount_options* options = &Vcb->options;
    UNICODE_STRING path, ignoreus, compressus, compressforceus, compresstypeus, readonlyus, zliblevelus, flushintervalus,
                   maxinlineus, subvolidus, raid5recalcus, raid6recalcus, skipbalanceus;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    ULONG i, j, kvfilen, index, retlen;
    KEY_VALUE_FULL_INFORMATION* kvfi = NULL;
    HANDLE h;
    
    options->compress = mount_compress;
    options->compress_force = mount_compress_force;
    options->compress_type = mount_compress_type > BTRFS_COMPRESSION_LZO ? 0 : mount_compress_type;
    options->readonly = FALSE;
    options->zlib_level = mount_zlib_level;
    options->flush_interval = mount_flush_interval;
    options->max_inline = min(mount_max_inline, Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - sizeof(EXTENT_DATA) + 1);
    options->raid5_recalculation = mount_raid5_recalculation;
    options->raid6_recalculation = mount_raid6_recalculation;
    options->skip_balance = mount_skip_balance;
    options->subvol_id = 0;
    
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
        ERR("ZwOpenKey returned %08x\n", Status);
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
    RtlInitUnicodeString(&raid5recalcus, L"Raid5Recalculation");
    RtlInitUnicodeString(&raid6recalcus, L"Raid6Recalculation");
    RtlInitUnicodeString(&skipbalanceus, L"SkipBalance");
    
    do {
        Status = ZwEnumerateValueKey(h, index, KeyValueFullInformation, kvfi, kvfilen, &retlen);
        
        index++;
        
        if (NT_SUCCESS(Status)) {
            UNICODE_STRING us;
            
            us.Length = us.MaximumLength = kvfi->NameLength;
            us.Buffer = kvfi->Name;
            
            if (FsRtlAreNamesEqual(&ignoreus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->ignore = *val != 0 ? TRUE : FALSE;
            } else if (FsRtlAreNamesEqual(&compressus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->compress = *val != 0 ? TRUE : FALSE;
            } else if (FsRtlAreNamesEqual(&compressforceus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->compress_force = *val != 0 ? TRUE : FALSE;
            } else if (FsRtlAreNamesEqual(&compresstypeus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->compress_type = *val > BTRFS_COMPRESSION_LZO ? 0 : *val;
            } else if (FsRtlAreNamesEqual(&readonlyus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->readonly = *val != 0 ? TRUE : FALSE;
            } else if (FsRtlAreNamesEqual(&zliblevelus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->zlib_level = *val;
            } else if (FsRtlAreNamesEqual(&flushintervalus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->flush_interval = *val;
            } else if (FsRtlAreNamesEqual(&maxinlineus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->max_inline = min(*val, Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node) - sizeof(EXTENT_DATA) + 1);
            } else if (FsRtlAreNamesEqual(&subvolidus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_QWORD) {
                UINT64* val = (UINT64*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->subvol_id = *val;
            } else if (FsRtlAreNamesEqual(&raid5recalcus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->raid5_recalculation = *val;
            } else if (FsRtlAreNamesEqual(&raid6recalcus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->raid6_recalculation = *val;
            } else if (FsRtlAreNamesEqual(&skipbalanceus, &us, TRUE, NULL) && kvfi->DataOffset > 0 && kvfi->DataLength > 0 && kvfi->Type == REG_DWORD) {
                DWORD* val = (DWORD*)((UINT8*)kvfi + kvfi->DataOffset);
                
                options->skip_balance = *val;
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES) {
            ERR("ZwEnumerateValueKey returned %08x\n", Status);
            goto end2;
        }
    } while (NT_SUCCESS(Status));
    
    if (!options->compress && options->compress_force)
        options->compress = TRUE;
    
    if (options->zlib_level > 9)
        options->zlib_level = 9;
    
    if (options->flush_interval == 0)
        options->flush_interval = mount_flush_interval;
    
    if (options->raid5_recalculation > 1)
        options->raid5_recalculation = 1;
    
    if (options->raid6_recalculation > 2)
        options->raid6_recalculation = 2;

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
        ERR("ZwCreateKey returned %08x\n", Status);
        goto end;
    }
    
    mountedus.Buffer = option_mounted;
    mountedus.Length = mountedus.MaximumLength = wcslen(option_mounted) * sizeof(WCHAR);
    
    data = 1;
    
    Status = ZwSetValueKey(h, &mountedus, 0, REG_DWORD, &data, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        ERR("ZwSetValueKey returned %08x\n", Status);
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
    BOOL has_options = FALSE;
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
        ERR("ZwOpenKey returned %08x\n", Status);
        goto end;
    }
    
    index = 0;
    
    mountedus.Buffer = option_mounted;
    mountedus.Length = mountedus.MaximumLength = wcslen(option_mounted) * sizeof(WCHAR);
    
    do {
        Status = ZwEnumerateValueKey(h, index, KeyValueBasicInformation, kvbi, kvbilen, &retlen);
        
        index++;
        
        if (NT_SUCCESS(Status)) {
            UNICODE_STRING us;
            
            us.Length = us.MaximumLength = kvbi->NameLength;
            us.Buffer = kvbi->Name;
            
            if (!FsRtlAreNamesEqual(&mountedus, &us, TRUE, NULL)) {
                has_options = TRUE;
                break;
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES) {
            ERR("ZwEnumerateValueKey returned %08x\n", Status);
            goto end2;
        }
    } while (NT_SUCCESS(Status));
  
    if (has_options) {
        DWORD data = 0;
        
        Status = ZwSetValueKey(h, &mountedus, 0, REG_DWORD, &data, sizeof(DWORD));
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08x\n", Status);
            goto end2;
        }
    } else {
        Status = ZwDeleteKey(h);
        if (!NT_SUCCESS(Status)) {
            ERR("ZwDeleteKey returned %08x\n", Status);
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
        ERR("registry_mark_volume_unmounted_path returned %08x\n", Status);
        goto end;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    ExFreePool(path.Buffer);
    
    return Status;
}

#define is_hex(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

static BOOL is_uuid(ULONG namelen, WCHAR* name) {
    ULONG i;
    
    if (namelen != 36 * sizeof(WCHAR))
        return FALSE;
    
    for (i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (name[i] != '-')
                return FALSE;
        } else if (!is_hex(name[i]))
            return FALSE;
    }
    
    return TRUE;
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
            
            ERR("key: %.*S\n", kbi->NameLength / sizeof(WCHAR), kbi->Name);
            
            if (is_uuid(kbi->NameLength, kbi->Name)) {
                kn = ExAllocatePoolWithTag(PagedPool, sizeof(key_name), ALLOC_TAG);
                if (!kn) {
                    ERR("out of memory\n");
                    goto end;
                }
                
                kn->name.Length = kn->name.MaximumLength = kbi->NameLength;
                kn->name.Buffer = ExAllocatePoolWithTag(PagedPool, kn->name.Length, ALLOC_TAG);
                
                if (!kn->name.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kn);
                    goto end;
                }
                
                RtlCopyMemory(kn->name.Buffer, kbi->Name, kbi->NameLength);
                
                InsertTailList(&key_names, &kn->list_entry);
            }
        } else if (Status != STATUS_NO_MORE_ENTRIES)
            ERR("ZwEnumerateKey returned %08x\n", Status);
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
            WARN("registry_mark_volume_unmounted_path returned %08x\n", Status);
        
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
    ULONG kvfilen, retlen, i;
    KEY_VALUE_FULL_INFORMATION* kvfi;
    
    const WCHAR mappings[] = L"\\Mappings";
    
    path = ExAllocatePoolWithTag(PagedPool, regpath->Length + (wcslen(mappings) * sizeof(WCHAR)), ALLOC_TAG);
    if (!path) {
        ERR("out of memory\n");
        return;
    }
    
    RtlCopyMemory(path, regpath->Buffer, regpath->Length);
    RtlCopyMemory((UINT8*)path + regpath->Length, mappings, wcslen(mappings) * sizeof(WCHAR));
    
    us.Buffer = path;
    us.Length = us.MaximumLength = regpath->Length + ((USHORT)wcslen(mappings) * sizeof(WCHAR));
    
    InitializeObjectAttributes(&oa, &us, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    // FIXME - keep open and do notify for changes
    Status = ZwCreateKey(&h, KEY_QUERY_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);
    
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08x\n", Status);
        ExFreePool(path);
        return;
    }

    if (dispos == REG_OPENED_EXISTING_KEY) {
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
            
            if (NT_SUCCESS(Status) && kvfi->DataLength > 0) {
                UINT32 val = 0;
                
                RtlCopyMemory(&val, (UINT8*)kvfi + kvfi->DataOffset, min(kvfi->DataLength, sizeof(UINT32)));
                
                TRACE("entry %u = %.*S = %u\n", i, kvfi->NameLength / sizeof(WCHAR), kvfi->Name, val);
                
                add_user_mapping(kvfi->Name, kvfi->NameLength / sizeof(WCHAR), val);
            }
            
            i = i + 1;
        } while (Status != STATUS_NO_MORE_ENTRIES);
    }
    
    ZwClose(h);

    ExFreePool(path);
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
                RtlCopyMemory(val, ((UINT8*)kvfi) + kvfi->DataOffset, size);
            } else {
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }

                Status = ZwSetValueKey(h, &us, 0, type, val, size);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwSetValueKey returned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, type, val, size);
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08x\n", Status);
        }
    } else {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
}

void STDCALL read_registry(PUNICODE_STRING regpath) {
#ifndef __REACTOS__
    UNICODE_STRING us;
#endif
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    HANDLE h;
    ULONG dispos;
#ifndef __REACTOS__
    ULONG kvfilen;
    KEY_VALUE_FULL_INFORMATION* kvfi;
#endif
    
#ifndef __REACTOS__    
    static WCHAR def_log_file[] = L"\\??\\C:\\btrfs.log";
#endif
    
    read_mappings(regpath);
    
    InitializeObjectAttributes(&oa, regpath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    Status = ZwCreateKey(&h, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, &dispos);
    
    if (!NT_SUCCESS(Status)) {
        ERR("ZwCreateKey returned %08x\n", Status);
        return;
    }
    
    reset_subkeys(h, regpath);
    
    get_registry_value(h, L"Compress", REG_DWORD, &mount_compress, sizeof(mount_compress));
    get_registry_value(h, L"CompressForce", REG_DWORD, &mount_compress_force, sizeof(mount_compress_force));
    get_registry_value(h, L"CompressType", REG_DWORD, &mount_compress_type, sizeof(mount_compress_type));
    get_registry_value(h, L"ZlibLevel", REG_DWORD, &mount_zlib_level, sizeof(mount_zlib_level));
    get_registry_value(h, L"FlushInterval", REG_DWORD, &mount_flush_interval, sizeof(mount_flush_interval));
    get_registry_value(h, L"MaxInline", REG_DWORD, &mount_max_inline, sizeof(mount_max_inline));
    get_registry_value(h, L"Raid5Recalculation", REG_DWORD, &mount_raid5_recalculation, sizeof(mount_raid5_recalculation));
    get_registry_value(h, L"Raid6Recalculation", REG_DWORD, &mount_raid6_recalculation, sizeof(mount_raid6_recalculation));
    get_registry_value(h, L"SkipBalance", REG_DWORD, &mount_skip_balance, sizeof(mount_skip_balance));
    
    if (mount_flush_interval == 0)
        mount_flush_interval = 1;
    
    if (mount_raid5_recalculation > 1)
        mount_raid5_recalculation = 1;
    
    if (mount_raid6_recalculation > 2)
        mount_raid6_recalculation = 2;
    
#ifdef _DEBUG
    get_registry_value(h, L"DebugLogLevel", REG_DWORD, &debug_log_level, sizeof(debug_log_level));
    
    RtlInitUnicodeString(&us, L"LogDevice");
    
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
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_device.Length = log_device.MaximumLength = kvfi->DataLength;
                log_device.Buffer = ExAllocatePoolWithTag(PagedPool, kvfi->DataLength, ALLOC_TAG);
                
                if (!log_device.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_device.Buffer, ((UINT8*)kvfi) + kvfi->DataOffset, kvfi->DataLength);
                
                if (log_device.Buffer[(log_device.Length / sizeof(WCHAR)) - 1] == 0)
                    log_device.Length -= sizeof(WCHAR);
            } else {
                ERR("LogDevice was type %u, length %u\n", kvfi->Type, kvfi->DataLength);
                
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
    
    RtlInitUnicodeString(&us, L"LogFile");
    
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
            if ((kvfi->Type == REG_SZ || kvfi->Type == REG_EXPAND_SZ) && kvfi->DataLength >= sizeof(WCHAR)) {
                log_file.Length = log_file.MaximumLength = kvfi->DataLength;
                log_file.Buffer = ExAllocatePoolWithTag(PagedPool, kvfi->DataLength, ALLOC_TAG);
                
                if (!log_file.Buffer) {
                    ERR("out of memory\n");
                    ExFreePool(kvfi);
                    ZwClose(h);
                    return;
                }

                RtlCopyMemory(log_file.Buffer, ((UINT8*)kvfi) + kvfi->DataOffset, kvfi->DataLength);
                
                if (log_file.Buffer[(log_file.Length / sizeof(WCHAR)) - 1] == 0)
                    log_file.Length -= sizeof(WCHAR);
            } else {
                ERR("LogFile was type %u, length %u\n", kvfi->Type, kvfi->DataLength);
                
                Status = ZwDeleteValueKey(h, &us);
                if (!NT_SUCCESS(Status)) {
                    ERR("ZwDeleteValueKey returned %08x\n", Status);
                }
            }
        }
        
        ExFreePool(kvfi);
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = ZwSetValueKey(h, &us, 0, REG_SZ, def_log_file, (wcslen(def_log_file) + 1) * sizeof(WCHAR));
        
        if (!NT_SUCCESS(Status)) {
            ERR("ZwSetValueKey returned %08x\n", Status);
        }
    } else {
        ERR("ZwQueryValueKey returned %08x\n", Status);
    }
    
    if (log_file.Length == 0) {
        log_file.Length = log_file.MaximumLength = wcslen(def_log_file) * sizeof(WCHAR);
        log_file.Buffer = ExAllocatePoolWithTag(PagedPool, log_file.MaximumLength, ALLOC_TAG);
        
        if (!log_file.Buffer) {
            ERR("out of memory\n");
            ZwClose(h);
            return;
        }
        
        RtlCopyMemory(log_file.Buffer, def_log_file, log_file.Length);
    }
#endif
    
    ZwClose(h);
}
