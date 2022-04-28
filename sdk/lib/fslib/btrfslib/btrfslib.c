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

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#ifndef __REACTOS__
#include <winternl.h>
#include <devioctl.h>
#include <ntdddisk.h>
#else
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#endif
#include <ntddscsi.h>
#include <ntddstor.h>
#include <ata.h>
#include <mountmgr.h>
#ifdef __REACTOS__
#include <winnls.h>
#include <stdbool.h>
#include "btrfs.h"
#include "btrfsioctl.h"
#include "crc32c.h"
#include "xxhash.h"
#else
#include <stringapiset.h>
#include <stdbool.h>
#include "../btrfs.h"
#include "../btrfsioctl.h"
#include "../crc32c.h"
#include "../xxhash.h"

#if defined(_X86_) || defined(_AMD64_)
#ifndef _MSC_VER
#include <cpuid.h>
#else
#include <intrin.h>
#endif
#endif
#endif // __REACTOS__

#ifdef __REACTOS__
#define malloc(size)    RtlAllocateHeap(RtlGetProcessHeap(), 0, (size))
#define free(ptr)       RtlFreeHeap(RtlGetProcessHeap(), 0, (ptr))
#endif

#define SHA256_HASH_SIZE 32
void calc_sha256(uint8_t* hash, const void* input, size_t len);

#define BLAKE2_HASH_SIZE 32
void blake2b(void *out, size_t outlen, const void* in, size_t inlen);

#ifndef __REACTOS__
#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifndef _MSC_VER // not in mingw yet
#define DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED 0x80000000
#endif

#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
                           ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

NTSTATUS NTAPI NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
                          ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
#ifdef __cplusplus
}
#endif
#endif // __REACTOS__

// These are undocumented, and what comes from format.exe
typedef struct {
    void* table;
    void* unk1;
    WCHAR* string;
} DSTRING;

typedef struct {
    void* table;
} STREAM_MESSAGE;

#define FORMAT_FLAG_QUICK_FORMAT        0x00000001
#define FORMAT_FLAG_UNKNOWN1            0x00000002
#define FORMAT_FLAG_DISMOUNT_FIRST      0x00000004
#define FORMAT_FLAG_UNKNOWN2            0x00000040
#define FORMAT_FLAG_LARGE_RECORDS       0x00000100
#define FORMAT_FLAG_INTEGRITY_DISABLE   0x00000100

typedef struct {
    uint16_t unk1;
    uint16_t unk2;
    uint32_t flags;
    DSTRING* label;
} options;

#ifndef __REACTOS__
FORCEINLINE VOID InitializeListHead(PLIST_ENTRY ListHead) {
    ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry) {
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}
#endif

#ifdef __REACTOS__
ULONG NTAPI NtGetTickCount(VOID);
#endif

typedef struct {
    KEY key;
    uint16_t size;
    void* data;
    LIST_ENTRY list_entry;
} btrfs_item;

typedef struct {
    uint64_t offset;
    CHUNK_ITEM* chunk_item;
    uint64_t lastoff;
    uint64_t used;
    LIST_ENTRY list_entry;
} btrfs_chunk;

typedef struct {
    uint64_t id;
    tree_header header;
    btrfs_chunk* c;
    LIST_ENTRY items;
    LIST_ENTRY list_entry;
} btrfs_root;

typedef struct {
    DEV_ITEM dev_item;
    uint64_t last_alloc;
} btrfs_dev;

#define keycmp(key1, key2)\
    ((key1.obj_id < key2.obj_id) ? -1 :\
    ((key1.obj_id > key2.obj_id) ? 1 :\
    ((key1.obj_type < key2.obj_type) ? -1 :\
    ((key1.obj_type > key2.obj_type) ? 1 :\
    ((key1.offset < key2.offset) ? -1 :\
    ((key1.offset > key2.offset) ? 1 :\
    0))))))

HMODULE module;
ULONG def_sector_size = 0, def_node_size = 0;
uint64_t def_incompat_flags = BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF | BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA;
uint16_t def_csum_type = CSUM_TYPE_CRC32C;

#ifndef __REACTOS__
// the following definitions come from fmifs.h in ReactOS

typedef struct {
    ULONG Lines;
    PCHAR Output;
} TEXTOUTPUT, *PTEXTOUTPUT;

typedef enum {
    FMIFS_UNKNOWN0,
    FMIFS_UNKNOWN1,
    FMIFS_UNKNOWN2,
    FMIFS_UNKNOWN3,
    FMIFS_UNKNOWN4,
    FMIFS_UNKNOWN5,
    FMIFS_UNKNOWN6,
    FMIFS_UNKNOWN7,
    FMIFS_FLOPPY,
    FMIFS_UNKNOWN9,
    FMIFS_UNKNOWN10,
    FMIFS_REMOVABLE,
    FMIFS_HARDDISK,
    FMIFS_UNKNOWN13,
    FMIFS_UNKNOWN14,
    FMIFS_UNKNOWN15,
    FMIFS_UNKNOWN16,
    FMIFS_UNKNOWN17,
    FMIFS_UNKNOWN18,
    FMIFS_UNKNOWN19,
    FMIFS_UNKNOWN20,
    FMIFS_UNKNOWN21,
    FMIFS_UNKNOWN22,
    FMIFS_UNKNOWN23,
} FMIFS_MEDIA_FLAG;

typedef enum {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    FSNOTSUPPORTED,
    VOLUMEINUSE,
    UNKNOWN9,
    UNKNOWNA,
    DONE,
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS,
    CLUSTERSIZETOOSMALL,
} CALLBACKCOMMAND;

typedef BOOLEAN (NTAPI* PFMIFSCALLBACK)(CALLBACKCOMMAND Command, ULONG SubAction, PVOID ActionInfo);

#else

#include <fmifs/fmifs.h>

#endif // __REACTOS__

#ifndef __REACTOS__
NTSTATUS WINAPI ChkdskEx(PUNICODE_STRING DriveRoot, BOOLEAN FixErrors, BOOLEAN Verbose, BOOLEAN CheckOnlyIfDirty,
                         BOOLEAN ScanDrive, PFMIFSCALLBACK Callback) {
#else
BOOLEAN
NTAPI
BtrfsChkdsk(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID pUnknown1,
    IN PVOID pUnknown2,
    IN PVOID pUnknown3,
    IN PVOID pUnknown4,
    IN PULONG ExitStatus)
{
#endif
    // STUB

    if (Callback) {
        TEXTOUTPUT TextOut;

        TextOut.Lines = 1;
        TextOut.Output = "stub, not implemented";

        Callback(OUTPUT, 0, &TextOut);
    }

#ifndef __REACTOS__
    return STATUS_SUCCESS;
#else
    *ExitStatus = (ULONG)STATUS_SUCCESS;
    return TRUE;
#endif
}

static btrfs_root* add_root(LIST_ENTRY* roots, uint64_t id) {
    btrfs_root* root;

    root = malloc(sizeof(btrfs_root));

    root->id = id;
    RtlZeroMemory(&root->header, sizeof(tree_header));
    InitializeListHead(&root->items);
    InsertTailList(roots, &root->list_entry);

    return root;
}

static void free_roots(LIST_ENTRY* roots) {
    LIST_ENTRY* le;

    le = roots->Flink;
    while (le != roots) {
        LIST_ENTRY *le2 = le->Flink, *le3;
        btrfs_root* r = CONTAINING_RECORD(le, btrfs_root, list_entry);

        le3 = r->items.Flink;
        while (le3 != &r->items) {
            LIST_ENTRY* le4 = le3->Flink;
            btrfs_item* item = CONTAINING_RECORD(le3, btrfs_item, list_entry);

            if (item->data)
                free(item->data);

            free(item);

            le3 = le4;
        }

        free(r);

        le = le2;
    }
}

static void free_chunks(LIST_ENTRY* chunks) {
    LIST_ENTRY* le;

    le = chunks->Flink;
    while (le != chunks) {
        LIST_ENTRY *le2 = le->Flink;
        btrfs_chunk* c = CONTAINING_RECORD(le, btrfs_chunk, list_entry);

        free(c->chunk_item);
        free(c);

        le = le2;
    }
}

static void add_item(btrfs_root* r, uint64_t obj_id, uint8_t obj_type, uint64_t offset, void* data, uint16_t size) {
    LIST_ENTRY* le;
    btrfs_item* item;

    item = malloc(sizeof(btrfs_item));

    item->key.obj_id = obj_id;
    item->key.obj_type = obj_type;
    item->key.offset = offset;
    item->size = size;

    if (size == 0)
        item->data = NULL;
    else {
        item->data = malloc(size);
        memcpy(item->data, data, size);
    }

    le = r->items.Flink;
    while (le != &r->items) {
        btrfs_item* i2 = CONTAINING_RECORD(le, btrfs_item, list_entry);

        if (keycmp(item->key, i2->key) != 1) {
            InsertTailList(le, &item->list_entry);
            return;
        }

        le = le->Flink;
    }

    InsertTailList(&r->items, &item->list_entry);
}

static uint64_t find_chunk_offset(uint64_t size, uint64_t offset, btrfs_dev* dev, btrfs_root* dev_root, BTRFS_UUID* chunkuuid) {
    uint64_t off;
    DEV_EXTENT de;

    off = dev->last_alloc;
    dev->last_alloc += size;

    dev->dev_item.bytes_used += size;

    de.chunktree = BTRFS_ROOT_CHUNK;
    de.objid = 0x100;
    de.address = offset;
    de.length = size;
    de.chunktree_uuid = *chunkuuid;

    add_item(dev_root, dev->dev_item.dev_id, TYPE_DEV_EXTENT, off, &de, sizeof(DEV_EXTENT));

    return off;
}

static btrfs_chunk* add_chunk(LIST_ENTRY* chunks, uint64_t flags, btrfs_root* chunk_root, btrfs_dev* dev, btrfs_root* dev_root, BTRFS_UUID* chunkuuid, uint32_t sector_size) {
    uint64_t off, size;
    uint16_t stripes, i;
    btrfs_chunk* c;
    LIST_ENTRY* le;
    CHUNK_ITEM_STRIPE* cis;

    off = 0xc00000;
    le = chunks->Flink;
    while (le != chunks) {
        c = CONTAINING_RECORD(le, btrfs_chunk, list_entry);

        if (c->offset + c->chunk_item->size > off)
            off = c->offset + c->chunk_item->size;

        le = le->Flink;
    }

    if (flags & BLOCK_FLAG_METADATA) {
        if (dev->dev_item.num_bytes > 0xC80000000) // 50 GB
            size = 0x40000000; // 1 GB
        else
            size = 0x10000000; // 256 MB
    } else if (flags & BLOCK_FLAG_SYSTEM)
        size = 0x800000;

    size = min(size, dev->dev_item.num_bytes / 10); // cap at 10%
    size &= ~(sector_size - 1);

    stripes = flags & BLOCK_FLAG_DUPLICATE ? 2 : 1;

    if (dev->dev_item.num_bytes - dev->dev_item.bytes_used < stripes * size) // not enough space
        return NULL;

    c = malloc(sizeof(btrfs_chunk));
    c->offset = off;
    c->lastoff = off;
    c->used = 0;

    c->chunk_item = malloc(sizeof(CHUNK_ITEM) + (stripes * sizeof(CHUNK_ITEM_STRIPE)));

    c->chunk_item->size = size;
    c->chunk_item->root_id = BTRFS_ROOT_EXTENT;
    c->chunk_item->stripe_length = max(sector_size, 0x10000);
    c->chunk_item->type = flags;
    c->chunk_item->opt_io_alignment = max(sector_size, 0x10000);
    c->chunk_item->opt_io_width = max(sector_size, 0x10000);
    c->chunk_item->sector_size = sector_size;
    c->chunk_item->num_stripes = stripes;
    c->chunk_item->sub_stripes = 0;

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

    for (i = 0; i < stripes; i++) {
        cis[i].dev_id = dev->dev_item.dev_id;
        cis[i].offset = find_chunk_offset(size, c->offset, dev, dev_root, chunkuuid);
        cis[i].dev_uuid = dev->dev_item.device_uuid;
    }

    add_item(chunk_root, 0x100, TYPE_CHUNK_ITEM, c->offset, c->chunk_item, sizeof(CHUNK_ITEM) + (stripes * sizeof(CHUNK_ITEM_STRIPE)));

    InsertTailList(chunks, &c->list_entry);

    return c;
}

static bool superblock_collision(btrfs_chunk* c, uint64_t address) {
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    uint64_t stripe = (address - c->offset) / c->chunk_item->stripe_length;
    uint16_t i, j;

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        j = 0;
        while (superblock_addrs[j] != 0) {
            if (superblock_addrs[j] >= cis[i].offset) {
                uint64_t stripe2 = (superblock_addrs[j] - cis[i].offset) / c->chunk_item->stripe_length;

                if (stripe2 == stripe)
                    return true;
            }
            j++;
        }
    }

    return false;
}

static uint64_t get_next_address(btrfs_chunk* c) {
    uint64_t addr;

    addr = c->lastoff;

    while (superblock_collision(c, addr)) {
        addr = addr - ((addr - c->offset) % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;

        if (addr >= c->offset + c->chunk_item->size) // chunk has been exhausted
            return 0;
    }

    return addr;
}

typedef struct {
    EXTENT_ITEM ei;
    uint8_t type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_METADATA;

typedef struct {
    EXTENT_ITEM ei;
    EXTENT_ITEM2 ei2;
    uint8_t type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_METADATA2;

static void assign_addresses(LIST_ENTRY* roots, btrfs_chunk* sys_chunk, btrfs_chunk* metadata_chunk, uint32_t node_size,
                             btrfs_root* root_root, btrfs_root* extent_root, bool skinny) {
    LIST_ENTRY* le;

    le = roots->Flink;
    while (le != roots) {
        btrfs_root* r = CONTAINING_RECORD(le, btrfs_root, list_entry);
        btrfs_chunk* c = r->id == BTRFS_ROOT_CHUNK ? sys_chunk : metadata_chunk;

        r->header.address = get_next_address(c);
        r->c = c;
        c->lastoff = r->header.address + node_size;
        c->used += node_size;

        if (skinny) {
            EXTENT_ITEM_METADATA eim;

            eim.ei.refcount = 1;
            eim.ei.generation = 1;
            eim.ei.flags = EXTENT_ITEM_TREE_BLOCK;
            eim.type = TYPE_TREE_BLOCK_REF;
            eim.tbr.offset = r->id;

            add_item(extent_root, r->header.address, TYPE_METADATA_ITEM, 0, &eim, sizeof(EXTENT_ITEM_METADATA));
        } else {
            EXTENT_ITEM_METADATA2 eim2;
            KEY firstitem;

            if (r->items.Flink == &r->items) {
                firstitem.obj_id = 0;
                firstitem.obj_type = 0;
                firstitem.offset = 0;
            } else {
                btrfs_item* bi = CONTAINING_RECORD(r->items.Flink, btrfs_item, list_entry);

                firstitem = bi->key;
            }

            eim2.ei.refcount = 1;
            eim2.ei.generation = 1;
            eim2.ei.flags = EXTENT_ITEM_TREE_BLOCK;
            eim2.ei2.firstitem = firstitem;
            eim2.ei2.level = 0;
            eim2.type = TYPE_TREE_BLOCK_REF;
            eim2.tbr.offset = r->id;

            add_item(extent_root, r->header.address, TYPE_EXTENT_ITEM, node_size, &eim2, sizeof(EXTENT_ITEM_METADATA2));
        }

        if (r->id != BTRFS_ROOT_ROOT && r->id != BTRFS_ROOT_CHUNK) {
            ROOT_ITEM ri;

            memset(&ri, 0, sizeof(ROOT_ITEM));

            ri.inode.generation = 1;
            ri.inode.st_size = 3;
            ri.inode.st_blocks = node_size;
            ri.inode.st_nlink = 1;
            ri.inode.st_mode = 040755;
            ri.generation = 1;
            ri.objid = r->id == 5 || r->id >= 0x100 ? SUBVOL_ROOT_INODE : 0;
            ri.block_number = r->header.address;
            ri.bytes_used = node_size;
            ri.num_references = 1;
            ri.generation2 = ri.generation;

            add_item(root_root, r->id, TYPE_ROOT_ITEM, 0, &ri, sizeof(ROOT_ITEM));
        }

        le = le->Flink;
    }
}

static NTSTATUS write_data(HANDLE h, uint64_t address, btrfs_chunk* c, void* data, ULONG size) {
    NTSTATUS Status;
    uint16_t i;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    CHUNK_ITEM_STRIPE* cis;

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        off.QuadPart = cis[i].offset + address - c->offset;

        Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, data, size, &off, NULL);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

static void calc_tree_checksum(tree_header* th, uint32_t node_size) {
    switch (def_csum_type) {
        case CSUM_TYPE_CRC32C:
            *(uint32_t*)th = ~calc_crc32c(0xffffffff, (uint8_t*)&th->fs_uuid, node_size - sizeof(th->csum));
        break;

        case CSUM_TYPE_XXHASH:
            *(uint64_t*)th = XXH64((uint8_t*)&th->fs_uuid, node_size - sizeof(th->csum), 0);
        break;

        case CSUM_TYPE_SHA256:
            calc_sha256((uint8_t*)th, &th->fs_uuid, node_size - sizeof(th->csum));
        break;

        case CSUM_TYPE_BLAKE2:
            blake2b((uint8_t*)th, BLAKE2_HASH_SIZE, &th->fs_uuid, node_size - sizeof(th->csum));
        break;
    }
}

static NTSTATUS write_roots(HANDLE h, LIST_ENTRY* roots, uint32_t node_size, BTRFS_UUID* fsuuid, BTRFS_UUID* chunkuuid) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    uint8_t* tree;

    tree = malloc(node_size);

    le = roots->Flink;
    while (le != roots) {
        btrfs_root* r = CONTAINING_RECORD(le, btrfs_root, list_entry);
        uint8_t* dp;
        leaf_node* ln;

        memset(tree, 0, node_size);

        r->header.num_items = 0;
        r->header.fs_uuid = *fsuuid;
        r->header.flags = HEADER_FLAG_MIXED_BACKREF | HEADER_FLAG_WRITTEN;
        r->header.chunk_tree_uuid = *chunkuuid;
        r->header.generation = 1;
        r->header.tree_id = r->id;

        ln = (leaf_node*)(tree + sizeof(tree_header));

        dp = tree + node_size;

        le2 = r->items.Flink;
        while (le2 != &r->items) {
            btrfs_item* item = CONTAINING_RECORD(le2, btrfs_item, list_entry);

            ln->key = item->key;
            ln->size = item->size;

            if (item->size > 0) {
                dp -= item->size;
                memcpy(dp, item->data, item->size);

                ln->offset = (uint32_t)(dp - tree - sizeof(tree_header));
            } else
                ln->offset = 0;

            ln = &ln[1];

            r->header.num_items++;

            le2 = le2->Flink;
        }

        memcpy(tree, &r->header, sizeof(tree_header));

        calc_tree_checksum((tree_header*)tree, node_size);

        Status = write_data(h, r->header.address, r->c, tree, node_size);
        if (!NT_SUCCESS(Status)) {
            free(tree);
            return Status;
        }

        le = le->Flink;
    }

    free(tree);

    return STATUS_SUCCESS;
}

#ifndef __REACTOS__
static void get_uuid(BTRFS_UUID* uuid) {
#else
static void get_uuid(BTRFS_UUID* uuid, ULONG* seed) {
#endif
    uint8_t i;

    for (i = 0; i < 16; i+=2) {
#ifndef __REACTOS__
        ULONG r = rand();
#else
        ULONG r = RtlRandom(seed);
#endif

        uuid->uuid[i] = (r & 0xff00) >> 8;
        uuid->uuid[i+1] = r & 0xff;
    }
}

#ifndef __REACTOS__
static void init_device(btrfs_dev* dev, uint64_t id, uint64_t size, BTRFS_UUID* fsuuid, uint32_t sector_size) {
#else
static void init_device(btrfs_dev* dev, uint64_t id, uint64_t size, BTRFS_UUID* fsuuid, uint32_t sector_size, ULONG* seed) {
#endif
    dev->dev_item.dev_id = id;
    dev->dev_item.num_bytes = size;
    dev->dev_item.bytes_used = 0;
    dev->dev_item.optimal_io_align = sector_size;
    dev->dev_item.optimal_io_width = sector_size;
    dev->dev_item.minimal_io_size = sector_size;
    dev->dev_item.type = 0;
    dev->dev_item.generation = 0;
    dev->dev_item.start_offset = 0;
    dev->dev_item.dev_group = 0;
    dev->dev_item.seek_speed = 0;
    dev->dev_item.bandwidth = 0;
#ifndef __REACTOS__
    get_uuid(&dev->dev_item.device_uuid);
#else
    get_uuid(&dev->dev_item.device_uuid, seed);
#endif
    dev->dev_item.fs_uuid = *fsuuid;

    dev->last_alloc = 0x100000; // skip first megabyte
}

static void calc_superblock_checksum(superblock* sb) {
    switch (def_csum_type) {
        case CSUM_TYPE_CRC32C:
            *(uint32_t*)sb = ~calc_crc32c(0xffffffff, (uint8_t*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));
        break;

        case CSUM_TYPE_XXHASH:
            *(uint64_t*)sb = XXH64(&sb->uuid, sizeof(superblock) - sizeof(sb->checksum), 0);
        break;

        case CSUM_TYPE_SHA256:
            calc_sha256((uint8_t*)sb, &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));
        break;

        case CSUM_TYPE_BLAKE2:
            blake2b((uint8_t*)sb, BLAKE2_HASH_SIZE, &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));
        break;
    }
}

static NTSTATUS write_superblocks(HANDLE h, btrfs_dev* dev, btrfs_root* chunk_root, btrfs_root* root_root, btrfs_root* extent_root,
                                  btrfs_chunk* sys_chunk, uint32_t node_size, BTRFS_UUID* fsuuid, uint32_t sector_size, PUNICODE_STRING label, uint64_t incompat_flags) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    ULONG sblen;
    int i;
    superblock* sb;
    KEY* key;
    uint64_t bytes_used;
    LIST_ENTRY* le;

    sblen = sizeof(*sb);
    if (sblen & (sector_size - 1))
        sblen = (sblen & sector_size) + sector_size;

    bytes_used = 0;

    le = extent_root->items.Flink;
    while (le != &extent_root->items) {
        btrfs_item* item = CONTAINING_RECORD(le, btrfs_item, list_entry);

        if (item->key.obj_type == TYPE_EXTENT_ITEM)
            bytes_used += item->key.offset;
        else if (item->key.obj_type == TYPE_METADATA_ITEM)
            bytes_used += node_size;

        le = le->Flink;
    }

    sb = malloc(sblen);
    memset(sb, 0, sblen);

    sb->uuid = *fsuuid;
    sb->flags = 1;
    sb->magic = BTRFS_MAGIC;
    sb->generation = 1;
    sb->root_tree_addr = root_root->header.address;
    sb->chunk_tree_addr = chunk_root->header.address;
    sb->total_bytes = dev->dev_item.num_bytes;
    sb->bytes_used = bytes_used;
    sb->root_dir_objectid = BTRFS_ROOT_TREEDIR;
    sb->num_devices = 1;
    sb->sector_size = sector_size;
    sb->node_size = node_size;
    sb->leaf_size = node_size;
    sb->stripe_size = sector_size;
    sb->n = sizeof(KEY) + sizeof(CHUNK_ITEM) + (sys_chunk->chunk_item->num_stripes * sizeof(CHUNK_ITEM_STRIPE));
    sb->chunk_root_generation = 1;
    sb->incompat_flags = incompat_flags;
    sb->csum_type = def_csum_type;
    memcpy(&sb->dev_item, &dev->dev_item, sizeof(DEV_ITEM));

    if (label->Length > 0) {
#ifdef __REACTOS__
        ANSI_STRING as;
        unsigned int i;

        for (i = 0; i < label->Length / sizeof(WCHAR); i++) {
#else
        ULONG utf8len;

        for (unsigned int i = 0; i < label->Length / sizeof(WCHAR); i++) {
#endif
            if (label->Buffer[i] == '/' || label->Buffer[i] == '\\') {
                free(sb);
                return STATUS_INVALID_VOLUME_LABEL;
            }
        }

#ifndef __REACTOS__
        utf8len = WideCharToMultiByte(CP_UTF8, 0, label->Buffer, label->Length / sizeof(WCHAR), NULL, 0, NULL, NULL);

        if (utf8len == 0 || utf8len > MAX_LABEL_SIZE) {
            free(sb);
            return STATUS_INVALID_VOLUME_LABEL;
        }

        if (WideCharToMultiByte(CP_UTF8, 0, label->Buffer, label->Length / sizeof(WCHAR), sb->label, utf8len, NULL, NULL) == 0) {
#else
        as.Buffer = sb->label;
        as.Length = 0;
        as.MaximumLength = MAX_LABEL_SIZE;

        if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&as, label, FALSE)))
        {
#endif
            free(sb);
            return STATUS_INVALID_VOLUME_LABEL;
        }
    }
    sb->cache_generation = 0xffffffffffffffff;

    key = (KEY*)sb->sys_chunk_array;
    key->obj_id = 0x100;
    key->obj_type = TYPE_CHUNK_ITEM;
    key->offset = sys_chunk->offset;
    memcpy(&key[1], sys_chunk->chunk_item, sizeof(CHUNK_ITEM) + (sys_chunk->chunk_item->num_stripes * sizeof(CHUNK_ITEM_STRIPE)));

    i = 0;
    while (superblock_addrs[i] != 0) {
        LARGE_INTEGER off;

        if (superblock_addrs[i] > dev->dev_item.num_bytes)
            break;

        sb->sb_phys_addr = superblock_addrs[i];

        calc_superblock_checksum(sb);

        off.QuadPart = superblock_addrs[i];

        Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, sb, sblen, &off, NULL);
        if (!NT_SUCCESS(Status)) {
            free(sb);
            return Status;
        }

        i++;
    }

    free(sb);

    return STATUS_SUCCESS;
}

static __inline void win_time_to_unix(LARGE_INTEGER t, BTRFS_TIME* out) {
    ULONGLONG l = t.QuadPart - 116444736000000000;

    out->seconds = l / 10000000;
    out->nanoseconds = (l % 10000000) * 100;
}

#ifdef __REACTOS__
VOID
WINAPI
GetSystemTimeAsFileTime(OUT PFILETIME lpFileTime)
{
    LARGE_INTEGER SystemTime;

    do
    {
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
    }
    while (SystemTime.HighPart != SharedUserData->SystemTime.High2Time);

    lpFileTime->dwLowDateTime = SystemTime.LowPart;
    lpFileTime->dwHighDateTime = SystemTime.HighPart;
}
#endif

static void add_inode_ref(btrfs_root* r, uint64_t inode, uint64_t parent, uint64_t index, const char* name) {
    uint16_t name_len = (uint16_t)strlen(name);
    INODE_REF* ir = malloc(offsetof(INODE_REF, name[0]) + name_len);

    ir->index = 0;
    ir->n = name_len;
    memcpy(ir->name, name, name_len);

    add_item(r, inode, TYPE_INODE_REF, parent, ir, (uint16_t)offsetof(INODE_REF, name[0]) + ir->n);

    free(ir);
}

static void init_fs_tree(btrfs_root* r, uint32_t node_size) {
    INODE_ITEM ii;
    FILETIME filetime;
    LARGE_INTEGER time;

    memset(&ii, 0, sizeof(INODE_ITEM));

    ii.generation = 1;
    ii.st_blocks = node_size;
    ii.st_nlink = 1;
    ii.st_mode = 040755;

    GetSystemTimeAsFileTime(&filetime);
    time.LowPart = filetime.dwLowDateTime;
    time.HighPart = filetime.dwHighDateTime;

    win_time_to_unix(time, &ii.st_atime);
    ii.st_ctime = ii.st_mtime = ii.st_atime;

    add_item(r, SUBVOL_ROOT_INODE, TYPE_INODE_ITEM, 0, &ii, sizeof(INODE_ITEM));

    add_inode_ref(r, SUBVOL_ROOT_INODE, SUBVOL_ROOT_INODE, 0, "..");
}

static void add_block_group_items(LIST_ENTRY* chunks, btrfs_root* extent_root) {
    LIST_ENTRY* le;

    le = chunks->Flink;
    while (le != chunks) {
        btrfs_chunk* c = CONTAINING_RECORD(le, btrfs_chunk, list_entry);
        BLOCK_GROUP_ITEM bgi;

        bgi.used = c->used;
        bgi.chunk_tree = 0x100;
        bgi.flags = c->chunk_item->type;
        add_item(extent_root, c->offset, TYPE_BLOCK_GROUP_ITEM, c->chunk_item->size, &bgi, sizeof(BLOCK_GROUP_ITEM));

        le = le->Flink;
    }
}

static NTSTATUS clear_first_megabyte(HANDLE h) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER zero;
    uint8_t* mb;

    mb = malloc(0x100000);
    memset(mb, 0, 0x100000);

    zero.QuadPart = 0;

    Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, mb, 0x100000, &zero, NULL);

    free(mb);

    return Status;
}

static bool is_ssd(HANDLE h) {
    ULONG aptelen;
    ATA_PASS_THROUGH_EX* apte;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    IDENTIFY_DEVICE_DATA* idd;

    aptelen = sizeof(ATA_PASS_THROUGH_EX) + 512;
    apte = malloc(aptelen);

    RtlZeroMemory(apte, aptelen);

    apte->Length = sizeof(ATA_PASS_THROUGH_EX);
    apte->AtaFlags = ATA_FLAGS_DATA_IN;
    apte->DataTransferLength = aptelen - sizeof(ATA_PASS_THROUGH_EX);
    apte->TimeOutValue = 3;
    apte->DataBufferOffset = apte->Length;
    apte->CurrentTaskFile[6] = IDE_COMMAND_IDENTIFY;

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_ATA_PASS_THROUGH, apte, aptelen, apte, aptelen);

    if (NT_SUCCESS(Status)) {
        idd = (IDENTIFY_DEVICE_DATA*)((uint8_t*)apte + sizeof(ATA_PASS_THROUGH_EX));

        if (idd->NominalMediaRotationRate == 1) {
            free(apte);
            return true;
        }
    }

    free(apte);

    return false;
}

static void add_dir_item(btrfs_root* root, uint64_t inode, uint32_t hash, uint64_t key_objid, uint8_t key_type,
                         uint64_t key_offset, uint64_t transid, uint8_t type, const char* name) {
    uint16_t name_len = (uint16_t)strlen(name);
    DIR_ITEM* di = malloc(offsetof(DIR_ITEM, name[0]) + name_len);

    di->key.obj_id = key_objid;
    di->key.obj_type = key_type;
    di->key.offset = key_offset;
    di->transid = transid;
    di->m = 0;
    di->n = name_len;
    di->type = type;
    memcpy(di->name, name, name_len);

    add_item(root, inode, TYPE_DIR_ITEM, hash, di, (uint16_t)(offsetof(DIR_ITEM, name[0]) + di->m + di->n));

    free(di);
}

static void set_default_subvol(btrfs_root* root_root, uint32_t node_size) {
    INODE_ITEM ii;
    FILETIME filetime;
    LARGE_INTEGER time;

    static const char default_subvol[] = "default";
    static const uint32_t default_hash = 0x8dbfc2d2;

    add_inode_ref(root_root, BTRFS_ROOT_FSTREE, BTRFS_ROOT_TREEDIR, 0, default_subvol);

    memset(&ii, 0, sizeof(INODE_ITEM));

    ii.generation = 1;
    ii.st_blocks = node_size;
    ii.st_nlink = 1;
    ii.st_mode = 040755;

    GetSystemTimeAsFileTime(&filetime);
    time.LowPart = filetime.dwLowDateTime;
    time.HighPart = filetime.dwHighDateTime;

    win_time_to_unix(time, &ii.st_atime);
    ii.st_ctime = ii.st_mtime = ii.otime = ii.st_atime;

    add_item(root_root, BTRFS_ROOT_TREEDIR, TYPE_INODE_ITEM, 0, &ii, sizeof(INODE_ITEM));

    add_inode_ref(root_root, BTRFS_ROOT_TREEDIR, BTRFS_ROOT_TREEDIR, 0, "..");

    add_dir_item(root_root, BTRFS_ROOT_TREEDIR, default_hash, BTRFS_ROOT_FSTREE, TYPE_ROOT_ITEM,
                 0xffffffffffffffff, 0, BTRFS_TYPE_DIRECTORY, default_subvol);
}

static NTSTATUS write_btrfs(HANDLE h, uint64_t size, PUNICODE_STRING label, uint32_t sector_size, uint32_t node_size, uint64_t incompat_flags) {
    NTSTATUS Status;
    LIST_ENTRY roots, chunks;
    btrfs_root *root_root, *chunk_root, *extent_root, *dev_root, *fs_root, *reloc_root;
    btrfs_chunk *sys_chunk, *metadata_chunk;
    btrfs_dev dev;
    BTRFS_UUID fsuuid, chunkuuid;
    bool ssd;
    uint64_t metadata_flags;
#ifdef __REACTOS__
    ULONG seed;
#endif

#ifndef __REACTOS__
    srand((unsigned int)time(0));
    get_uuid(&fsuuid);
    get_uuid(&chunkuuid);
#else
    seed = NtGetTickCount();
    get_uuid(&fsuuid, &seed);
    get_uuid(&chunkuuid, &seed);
#endif

    InitializeListHead(&roots);
    InitializeListHead(&chunks);

    root_root = add_root(&roots, BTRFS_ROOT_ROOT);
    chunk_root = add_root(&roots, BTRFS_ROOT_CHUNK);
    extent_root = add_root(&roots, BTRFS_ROOT_EXTENT);
    dev_root = add_root(&roots, BTRFS_ROOT_DEVTREE);
    add_root(&roots, BTRFS_ROOT_CHECKSUM);
    fs_root = add_root(&roots, BTRFS_ROOT_FSTREE);
    reloc_root = add_root(&roots, BTRFS_ROOT_DATA_RELOC);

#ifndef __REACTOS__
    init_device(&dev, 1, size, &fsuuid, sector_size);
#else
    init_device(&dev, 1, size, &fsuuid, sector_size, &seed);
#endif

    ssd = is_ssd(h);

    sys_chunk = add_chunk(&chunks, BLOCK_FLAG_SYSTEM | (ssd ? 0 : BLOCK_FLAG_DUPLICATE), chunk_root, &dev, dev_root, &chunkuuid, sector_size);
    if (!sys_chunk)
        return STATUS_INTERNAL_ERROR;

    metadata_flags = BLOCK_FLAG_METADATA;

    if (!ssd && !(incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS))
        metadata_flags |= BLOCK_FLAG_DUPLICATE;

    if (incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS)
        metadata_flags |= BLOCK_FLAG_DATA;

    metadata_chunk = add_chunk(&chunks, metadata_flags, chunk_root, &dev, dev_root, &chunkuuid, sector_size);
    if (!metadata_chunk)
        return STATUS_INTERNAL_ERROR;

    add_item(chunk_root, 1, TYPE_DEV_ITEM, dev.dev_item.dev_id, &dev.dev_item, sizeof(DEV_ITEM));

    set_default_subvol(root_root, node_size);

    init_fs_tree(fs_root, node_size);
    init_fs_tree(reloc_root, node_size);

    assign_addresses(&roots, sys_chunk, metadata_chunk, node_size, root_root, extent_root, incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA);

    add_block_group_items(&chunks, extent_root);

    Status = write_roots(h, &roots, node_size, &fsuuid, &chunkuuid);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = clear_first_megabyte(h);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = write_superblocks(h, &dev, chunk_root, root_root, extent_root, sys_chunk, node_size, &fsuuid, sector_size, label, incompat_flags);
    if (!NT_SUCCESS(Status))
        return Status;

    free_roots(&roots);
    free_chunks(&chunks);

    return STATUS_SUCCESS;
}

static bool look_for_device(btrfs_filesystem* bfs, BTRFS_UUID* devuuid) {
    uint32_t i;
    btrfs_filesystem_device* dev;

    for (i = 0; i < bfs->num_devices; i++) {
        if (i == 0)
            dev = &bfs->device;
        else
            dev = (btrfs_filesystem_device*)((uint8_t*)dev + offsetof(btrfs_filesystem_device, name[0]) + dev->name_length);

        if (RtlCompareMemory(&dev->uuid, devuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID))
            return true;
    }

    return false;
}

static bool check_superblock_checksum(superblock* sb) {
    switch (sb->csum_type) {
        case CSUM_TYPE_CRC32C: {
            uint32_t crc32 = ~calc_crc32c(0xffffffff, (uint8_t*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));

            return crc32 == *(uint32_t*)sb;
        }

        case CSUM_TYPE_XXHASH: {
            uint64_t hash = XXH64(&sb->uuid, sizeof(superblock) - sizeof(sb->checksum), 0);

            return hash == *(uint64_t*)sb;
        }

        case CSUM_TYPE_SHA256: {
            uint8_t hash[SHA256_HASH_SIZE];

            calc_sha256(hash, &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));

            return !memcmp(hash, sb, SHA256_HASH_SIZE);
        }

        case CSUM_TYPE_BLAKE2: {
            uint8_t hash[BLAKE2_HASH_SIZE];

            blake2b(hash, sizeof(hash), &sb->uuid, sizeof(superblock) - sizeof(sb->checksum));

            return !memcmp(hash, sb, BLAKE2_HASH_SIZE);
        }

        default:
            return false;
    }
}

static bool is_mounted_multi_device(HANDLE h, uint32_t sector_size) {
    NTSTATUS Status;
    superblock* sb;
    ULONG sblen;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    BTRFS_UUID fsuuid, devuuid;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES atts;
    HANDLE h2;
    btrfs_filesystem *bfs = NULL, *bfs2;
    ULONG bfssize;
    bool ret = false;

    static WCHAR btrfs[] = L"\\Btrfs";

    sblen = sizeof(*sb);
    if (sblen & (sector_size - 1))
        sblen = (sblen & sector_size) + sector_size;

    sb = malloc(sblen);

    off.QuadPart = superblock_addrs[0];

    Status = NtReadFile(h, NULL, NULL, NULL, &iosb, sb, sblen, &off, NULL);
    if (!NT_SUCCESS(Status)) {
        free(sb);
        return false;
    }

    if (sb->magic != BTRFS_MAGIC) {
        free(sb);
        return false;
    }

    if (!check_superblock_checksum(sb)) {
        free(sb);
        return false;
    }

    fsuuid = sb->uuid;
    devuuid = sb->dev_item.device_uuid;

    free(sb);

    us.Length = us.MaximumLength = (USHORT)(wcslen(btrfs) * sizeof(WCHAR));
    us.Buffer = btrfs;

    InitializeObjectAttributes(&atts, &us, 0, NULL, NULL);

    Status = NtOpenFile(&h2, SYNCHRONIZE | FILE_READ_ATTRIBUTES, &atts, &iosb,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status)) // not a problem, it usually just means the driver isn't loaded
        return false;

    bfssize = 0;

    do {
        bfssize += 1024;

        if (bfs) free(bfs);
        bfs = malloc(bfssize);

        Status = NtDeviceIoControlFile(h2, NULL, NULL, NULL, &iosb, IOCTL_BTRFS_QUERY_FILESYSTEMS, NULL, 0, bfs, bfssize);
    } while (Status == STATUS_BUFFER_OVERFLOW);

    if (!NT_SUCCESS(Status))
        goto end;

    if (bfs->num_devices != 0) {
        bfs2 = bfs;
        while (true) {
            if (RtlCompareMemory(&bfs2->uuid, &fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                if (bfs2->num_devices == 1)
                    ret = false;
                else
                    ret = look_for_device(bfs2, &devuuid);

                goto end;
            }

            if (bfs2->next_entry == 0)
                break;
            else
                bfs2 = (btrfs_filesystem*)((uint8_t*)bfs2 + bfs2->next_entry);
        }
    }

end:
    NtClose(h2);

    if (bfs)
        free(bfs);

    return ret;
}

static void do_full_trim(HANDLE h) {
    IO_STATUS_BLOCK iosb;
    DEVICE_MANAGE_DATA_SET_ATTRIBUTES dmdsa;

    RtlZeroMemory(&dmdsa, sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES));

    dmdsa.Size = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES);
    dmdsa.Action = DeviceDsmAction_Trim;
    dmdsa.Flags = DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE | DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED;
    dmdsa.ParameterBlockOffset = 0;
    dmdsa.ParameterBlockLength = 0;
    dmdsa.DataSetRangesOffset = 0;
    dmdsa.DataSetRangesLength = 0;

    NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES, &dmdsa, sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES), NULL, 0);
}

static bool is_power_of_two(ULONG i) {
    return ((i != 0) && !(i & (i - 1)));
}

#if !defined(__REACTOS__) && (defined(_X86_) || defined(_AMD64_))
static void check_cpu() {
    unsigned int cpuInfo[4];
    bool have_sse42;

#ifndef _MSC_VER
    __get_cpuid(1, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);
    have_sse42 = cpuInfo[2] & bit_SSE4_2;
#else
    __cpuid(cpuInfo, 1);
    have_sse42 = cpuInfo[2] & (1 << 20);
#endif

    if (have_sse42)
        calc_crc32c = calc_crc32c_hw;
}
#endif

static NTSTATUS NTAPI FormatEx2(PUNICODE_STRING DriveRoot, FMIFS_MEDIA_FLAG MediaFlag, PUNICODE_STRING Label,
                                BOOLEAN QuickFormat, ULONG ClusterSize, PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;
    HANDLE h, btrfsh;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    GET_LENGTH_INFORMATION gli;
    DISK_GEOMETRY dg;
    uint32_t sector_size, node_size;
    UNICODE_STRING btrfsus;
#ifndef __REACTOS__
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
#endif
    uint64_t incompat_flags;
    UNICODE_STRING empty_label;

    static WCHAR btrfs[] = L"\\Btrfs";

#ifndef __REACTOS__
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return STATUS_PRIVILEGE_NOT_HELD;

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        CloseHandle(token);
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        CloseHandle(token);
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    CloseHandle(token);

#if defined(_X86_) || defined(_AMD64_)
    check_cpu();
#endif
#endif

    if (def_csum_type != CSUM_TYPE_CRC32C && def_csum_type != CSUM_TYPE_XXHASH && def_csum_type != CSUM_TYPE_SHA256 &&
        def_csum_type != CSUM_TYPE_BLAKE2)
        return STATUS_INVALID_PARAMETER;

    InitializeObjectAttributes(&attr, DriveRoot, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                        FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli));
    if (!NT_SUCCESS(Status)) {
        NtClose(h);
        return Status;
    }

    // MSDN tells us to use IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, but there are
    // some instances where it fails and IOCTL_DISK_GET_DRIVE_GEOMETRY succeeds -
    // such as with spanned volumes.
    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg));
    if (!NT_SUCCESS(Status)) {
        NtClose(h);
        return Status;
    }

    if (def_sector_size == 0) {
        sector_size = dg.BytesPerSector;

        if (sector_size == 0x200 || sector_size == 0)
            sector_size = 0x1000;
    } else {
        if (dg.BytesPerSector != 0 && (def_sector_size < dg.BytesPerSector || def_sector_size % dg.BytesPerSector != 0 || !is_power_of_two(def_sector_size / dg.BytesPerSector))) {
            NtClose(h);
            return STATUS_INVALID_PARAMETER;
        }

        sector_size = def_sector_size;
    }

    if (def_node_size == 0)
        node_size = 0x4000;
    else {
        if (def_node_size < sector_size || def_node_size % sector_size != 0 || !is_power_of_two(def_node_size / sector_size)) {
            NtClose(h);
            return STATUS_INVALID_PARAMETER;
        }

        node_size = def_node_size;
    }

    if (Callback) {
        ULONG pc = 0;
        Callback(PROGRESS, 0, (PVOID)&pc);
    }

    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);

    if (is_mounted_multi_device(h, sector_size)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    do_full_trim(h);

    incompat_flags = def_incompat_flags;
    incompat_flags |= BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF | BTRFS_INCOMPAT_FLAGS_BIG_METADATA;

    if (!Label) {
        empty_label.Buffer = NULL;
        empty_label.Length = empty_label.MaximumLength = 0;
        Label = &empty_label;
    }

    Status = write_btrfs(h, gli.Length.QuadPart, Label, sector_size, node_size, incompat_flags);

    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);

end:
    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);

    NtClose(h);

    if (NT_SUCCESS(Status)) {
        btrfsus.Buffer = btrfs;
        btrfsus.Length = btrfsus.MaximumLength = (USHORT)(wcslen(btrfs) * sizeof(WCHAR));

        InitializeObjectAttributes(&attr, &btrfsus, 0, NULL, NULL);

        Status = NtOpenFile(&btrfsh, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                            FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);

        if (NT_SUCCESS(Status)) {
            MOUNTDEV_NAME* mdn;
            ULONG mdnsize;

            mdnsize = (ULONG)(offsetof(MOUNTDEV_NAME, Name[0]) + DriveRoot->Length);
            mdn = malloc(mdnsize);

            mdn->NameLength = DriveRoot->Length;
            memcpy(mdn->Name, DriveRoot->Buffer, DriveRoot->Length);

            NtDeviceIoControlFile(btrfsh, NULL, NULL, NULL, &iosb, IOCTL_BTRFS_PROBE_VOLUME, mdn, mdnsize, NULL, 0);

            free(mdn);

            NtClose(btrfsh);
        }

        Status = STATUS_SUCCESS;
    }

#ifndef __REACTOS__
    if (Callback) {
        bool success = NT_SUCCESS(Status);
        Callback(DONE, 0, (PVOID)&success);
    }
#endif

    return Status;
}

#ifdef __REACTOS__

BOOLEAN
NTAPI
BtrfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize)
{
    NTSTATUS Status;

    if (BackwardCompatible)
    {
        // DPRINT1("BackwardCompatible == TRUE is unsupported!\n");
        return FALSE;
    }

    Status = FormatEx2(DriveRoot, (FMIFS_MEDIA_FLAG)MediaType, Label, QuickFormat, ClusterSize, Callback);

    return NT_SUCCESS(Status);
}

#else

BOOL __stdcall FormatEx(DSTRING* root, STREAM_MESSAGE* message, options* opts, uint32_t unk1) {
    UNICODE_STRING DriveRoot, Label;
    NTSTATUS Status;

    if (!root || !root->string)
        return false;

    DriveRoot.Length = DriveRoot.MaximumLength = (USHORT)(wcslen(root->string) * sizeof(WCHAR));
    DriveRoot.Buffer = root->string;

    if (opts && opts->label && opts->label->string) {
        Label.Length = Label.MaximumLength = (USHORT)(wcslen(opts->label->string) * sizeof(WCHAR));
        Label.Buffer = opts->label->string;
    } else {
        Label.Length = Label.MaximumLength = 0;
        Label.Buffer = NULL;
    }

    Status = FormatEx2(&DriveRoot, FMIFS_HARDDISK, &Label, opts && opts->flags & FORMAT_FLAG_QUICK_FORMAT, 0, NULL);

    return NT_SUCCESS(Status);
}

#endif // __REACTOS__

void __stdcall SetSizes(ULONG sector, ULONG node) {
    if (sector != 0)
        def_sector_size = sector;

    if (node != 0)
        def_node_size = node;
}

void __stdcall SetIncompatFlags(uint64_t incompat_flags) {
    def_incompat_flags = incompat_flags;
}

void __stdcall SetCsumType(uint16_t csum_type) {
    def_csum_type = csum_type;
}

BOOL __stdcall GetFilesystemInformation(uint32_t unk1, uint32_t unk2, void* unk3) {
    // STUB - undocumented

    return true;
}

#ifndef __REACTOS__
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH)
        module = (HMODULE)hModule;

    return true;
}
#endif
