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
#include <ata.h>
#include <mountmgr.h>
#ifdef __REACTOS__
#include "../../drivers/filesystems/btrfs/btrfs.h"
#include "../../drivers/filesystems/btrfs/btrfsioctl.h"
#else
#include "../btrfs.h"
#include "../btrfsioctl.h"
#endif

#ifndef __REACTOS__
#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef __REACTOS__
#ifdef __cplusplus
extern "C" {
#endif
NTSYSCALLAPI NTSTATUS NTAPI NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
                           ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

NTSTATUS NTAPI NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
                          ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

NTSYSAPI NTSTATUS NTAPI RtlUnicodeToUTF8N(PCHAR UTF8StringDestination, ULONG UTF8StringMaxByteCount, PULONG UTF8StringActualByteCount,
                                          PCWCH UnicodeStringSource, ULONG  UnicodeStringByteCount);
#ifdef __cplusplus
}
#endif

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

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7)
NTSTATUS WINAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                  ULONG *utf8_bytes_written,
                                  const WCHAR *uni_src, ULONG uni_bytes);
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7) */

#ifdef __REACTOS__
ULONG WINAPI NtGetTickCount(VOID);
#endif

typedef struct {
    KEY key;
    UINT16 size;
    void* data;
    LIST_ENTRY list_entry;
} btrfs_item;

typedef struct {
    UINT64 offset;
    CHUNK_ITEM* chunk_item;
    UINT64 lastoff;
    UINT64 used;
    LIST_ENTRY list_entry;
} btrfs_chunk;

typedef struct {
    UINT64 id;
    tree_header header;
    btrfs_chunk* c;
    LIST_ENTRY items;
    LIST_ENTRY list_entry;
} btrfs_root;

typedef struct {
    DEV_ITEM dev_item;
    UINT64 last_alloc;
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

static const UINT32 crctable[] = {
    0x00000000, 0xf26b8303, 0xe13b70f7, 0x1350f3f4, 0xc79a971f, 0x35f1141c, 0x26a1e7e8, 0xd4ca64eb, 
    0x8ad958cf, 0x78b2dbcc, 0x6be22838, 0x9989ab3b, 0x4d43cfd0, 0xbf284cd3, 0xac78bf27, 0x5e133c24, 
    0x105ec76f, 0xe235446c, 0xf165b798, 0x030e349b, 0xd7c45070, 0x25afd373, 0x36ff2087, 0xc494a384, 
    0x9a879fa0, 0x68ec1ca3, 0x7bbcef57, 0x89d76c54, 0x5d1d08bf, 0xaf768bbc, 0xbc267848, 0x4e4dfb4b, 
    0x20bd8ede, 0xd2d60ddd, 0xc186fe29, 0x33ed7d2a, 0xe72719c1, 0x154c9ac2, 0x061c6936, 0xf477ea35, 
    0xaa64d611, 0x580f5512, 0x4b5fa6e6, 0xb93425e5, 0x6dfe410e, 0x9f95c20d, 0x8cc531f9, 0x7eaeb2fa, 
    0x30e349b1, 0xc288cab2, 0xd1d83946, 0x23b3ba45, 0xf779deae, 0x05125dad, 0x1642ae59, 0xe4292d5a, 
    0xba3a117e, 0x4851927d, 0x5b016189, 0xa96ae28a, 0x7da08661, 0x8fcb0562, 0x9c9bf696, 0x6ef07595, 
    0x417b1dbc, 0xb3109ebf, 0xa0406d4b, 0x522bee48, 0x86e18aa3, 0x748a09a0, 0x67dafa54, 0x95b17957, 
    0xcba24573, 0x39c9c670, 0x2a993584, 0xd8f2b687, 0x0c38d26c, 0xfe53516f, 0xed03a29b, 0x1f682198, 
    0x5125dad3, 0xa34e59d0, 0xb01eaa24, 0x42752927, 0x96bf4dcc, 0x64d4cecf, 0x77843d3b, 0x85efbe38, 
    0xdbfc821c, 0x2997011f, 0x3ac7f2eb, 0xc8ac71e8, 0x1c661503, 0xee0d9600, 0xfd5d65f4, 0x0f36e6f7, 
    0x61c69362, 0x93ad1061, 0x80fde395, 0x72966096, 0xa65c047d, 0x5437877e, 0x4767748a, 0xb50cf789, 
    0xeb1fcbad, 0x197448ae, 0x0a24bb5a, 0xf84f3859, 0x2c855cb2, 0xdeeedfb1, 0xcdbe2c45, 0x3fd5af46, 
    0x7198540d, 0x83f3d70e, 0x90a324fa, 0x62c8a7f9, 0xb602c312, 0x44694011, 0x5739b3e5, 0xa55230e6, 
    0xfb410cc2, 0x092a8fc1, 0x1a7a7c35, 0xe811ff36, 0x3cdb9bdd, 0xceb018de, 0xdde0eb2a, 0x2f8b6829, 
    0x82f63b78, 0x709db87b, 0x63cd4b8f, 0x91a6c88c, 0x456cac67, 0xb7072f64, 0xa457dc90, 0x563c5f93, 
    0x082f63b7, 0xfa44e0b4, 0xe9141340, 0x1b7f9043, 0xcfb5f4a8, 0x3dde77ab, 0x2e8e845f, 0xdce5075c, 
    0x92a8fc17, 0x60c37f14, 0x73938ce0, 0x81f80fe3, 0x55326b08, 0xa759e80b, 0xb4091bff, 0x466298fc, 
    0x1871a4d8, 0xea1a27db, 0xf94ad42f, 0x0b21572c, 0xdfeb33c7, 0x2d80b0c4, 0x3ed04330, 0xccbbc033, 
    0xa24bb5a6, 0x502036a5, 0x4370c551, 0xb11b4652, 0x65d122b9, 0x97baa1ba, 0x84ea524e, 0x7681d14d, 
    0x2892ed69, 0xdaf96e6a, 0xc9a99d9e, 0x3bc21e9d, 0xef087a76, 0x1d63f975, 0x0e330a81, 0xfc588982, 
    0xb21572c9, 0x407ef1ca, 0x532e023e, 0xa145813d, 0x758fe5d6, 0x87e466d5, 0x94b49521, 0x66df1622, 
    0x38cc2a06, 0xcaa7a905, 0xd9f75af1, 0x2b9cd9f2, 0xff56bd19, 0x0d3d3e1a, 0x1e6dcdee, 0xec064eed, 
    0xc38d26c4, 0x31e6a5c7, 0x22b65633, 0xd0ddd530, 0x0417b1db, 0xf67c32d8, 0xe52cc12c, 0x1747422f, 
    0x49547e0b, 0xbb3ffd08, 0xa86f0efc, 0x5a048dff, 0x8ecee914, 0x7ca56a17, 0x6ff599e3, 0x9d9e1ae0, 
    0xd3d3e1ab, 0x21b862a8, 0x32e8915c, 0xc083125f, 0x144976b4, 0xe622f5b7, 0xf5720643, 0x07198540, 
    0x590ab964, 0xab613a67, 0xb831c993, 0x4a5a4a90, 0x9e902e7b, 0x6cfbad78, 0x7fab5e8c, 0x8dc0dd8f, 
    0xe330a81a, 0x115b2b19, 0x020bd8ed, 0xf0605bee, 0x24aa3f05, 0xd6c1bc06, 0xc5914ff2, 0x37faccf1, 
    0x69e9f0d5, 0x9b8273d6, 0x88d28022, 0x7ab90321, 0xae7367ca, 0x5c18e4c9, 0x4f48173d, 0xbd23943e, 
    0xf36e6f75, 0x0105ec76, 0x12551f82, 0xe03e9c81, 0x34f4f86a, 0xc69f7b69, 0xd5cf889d, 0x27a40b9e, 
    0x79b737ba, 0x8bdcb4b9, 0x988c474d, 0x6ae7c44e, 0xbe2da0a5, 0x4c4623a6, 0x5f16d052, 0xad7d5351,
};

static UINT32 calc_crc32c(UINT32 seed, UINT8* msg, ULONG msglen) {
    UINT32 rem;
    ULONG i;
    
    rem = seed;
    
    for (i = 0; i < msglen; i++) {
        rem = crctable[(rem ^ msg[i]) & 0xff] ^ (rem >> 8);
    }
    
    return rem;
}

#ifndef __REACTOS__
NTSTATUS WINAPI ChkdskEx(PUNICODE_STRING DriveRoot, BOOLEAN FixErrors, BOOLEAN Verbose, BOOLEAN CheckOnlyIfDirty,
#else
NTSTATUS WINAPI BtrfsChkdskEx(PUNICODE_STRING DriveRoot, BOOLEAN FixErrors, BOOLEAN Verbose, BOOLEAN CheckOnlyIfDirty,
#endif
                         BOOLEAN ScanDrive, PFMIFSCALLBACK Callback) {
    // STUB
    
    if (Callback) {
        TEXTOUTPUT TextOut;
        
        TextOut.Lines = 1;
        TextOut.Output = "stub, not implemented";
        
        Callback(OUTPUT, 0, &TextOut);
    }
    
    return STATUS_SUCCESS;
}

static btrfs_root* add_root(LIST_ENTRY* roots, UINT64 id) {
    btrfs_root* root;
    
#ifdef __REACTOS__
    root = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(btrfs_root));
#else
    root = malloc(sizeof(btrfs_root));
#endif
    
    root->id = id;
#ifndef __REACTOS__
    RtlZeroMemory(&root->header, sizeof(tree_header));
#endif
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
#ifdef __REACTOS__
                RtlFreeHeap(RtlGetProcessHeap(), 0, item->data);
            
            RtlFreeHeap(RtlGetProcessHeap(), 0, item);
#else
                free(item->data);
            
            free(item);
#endif
            
            le3 = le4;
        }
        
#ifdef __REACTOS__
        RtlFreeHeap(RtlGetProcessHeap(), 0, r);
#else
        free(r);
#endif
        
        le = le2;
    }
}

static void free_chunks(LIST_ENTRY* chunks) {
    LIST_ENTRY* le;
    
    le = chunks->Flink;
    while (le != chunks) {
        LIST_ENTRY *le2 = le->Flink;
        btrfs_chunk* c = CONTAINING_RECORD(le, btrfs_chunk, list_entry);
        
#ifndef __REACTOS__
        free(c->chunk_item);
        free(c);
#else
        RtlFreeHeap(RtlGetProcessHeap(), 0, c->chunk_item);
        RtlFreeHeap(RtlGetProcessHeap(), 0, c);
#endif
        
        le = le2;
    }
}

static void add_item(btrfs_root* r, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, UINT16 size) {
    LIST_ENTRY* le;
    btrfs_item* item;
    
#ifndef __REACTOS__
    item = malloc(sizeof(btrfs_item));
#else
    item = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(btrfs_item));
#endif
    
    item->key.obj_id = obj_id;
    item->key.obj_type = obj_type;
    item->key.offset = offset;
    item->size = size;
    
    if (size == 0)
        item->data = NULL;
    else {
#ifndef __REACTOS__
        item->data = malloc(size);
#else
        item->data = RtlAllocateHeap(RtlGetProcessHeap(), 0, size);
#endif
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

static UINT64 find_chunk_offset(UINT64 size, UINT64 offset, btrfs_dev* dev, btrfs_root* dev_root, BTRFS_UUID* chunkuuid) {
    UINT64 off;
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

static btrfs_chunk* add_chunk(LIST_ENTRY* chunks, UINT64 flags, btrfs_root* chunk_root, btrfs_dev* dev, btrfs_root* dev_root, BTRFS_UUID* chunkuuid, UINT32 sector_size) {
    UINT64 off, size;
    UINT16 stripes, i;
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
    
#ifndef __REACTOS__
    c = malloc(sizeof(btrfs_chunk));
#else
    c = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(btrfs_chunk));
#endif
    c->offset = off;
    c->lastoff = off;
    c->used = 0;
    
#ifndef __REACTOS__
    c->chunk_item = malloc(sizeof(CHUNK_ITEM) + (stripes * sizeof(CHUNK_ITEM_STRIPE))); 
#else
    c->chunk_item = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(CHUNK_ITEM) + (stripes * sizeof(CHUNK_ITEM_STRIPE)));
#endif
    
    c->chunk_item->size = size;
    c->chunk_item->root_id = BTRFS_ROOT_EXTENT;
    c->chunk_item->stripe_length = 0x10000;
    c->chunk_item->type = flags;
    c->chunk_item->opt_io_alignment = 0x10000;
    c->chunk_item->opt_io_width = 0x10000;
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

static BOOL superblock_collision(btrfs_chunk* c, UINT64 address) {
    CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    UINT64 stripe = (address - c->offset) / c->chunk_item->stripe_length;
    UINT16 i, j;
    
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        j = 0;
        while (superblock_addrs[j] != 0) {
            if (superblock_addrs[j] >= cis[i].offset) {
                UINT64 stripe2 = (superblock_addrs[j] - cis[i].offset) / c->chunk_item->stripe_length;
                
                if (stripe2 == stripe)
                    return TRUE;
            }
            j++;
        }
    }
    
    return FALSE;
}

static UINT64 get_next_address(btrfs_chunk* c) {
    UINT64 addr;
    
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
    UINT8 type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_METADATA;

static void assign_addresses(LIST_ENTRY* roots, btrfs_chunk* sys_chunk, btrfs_chunk* metadata_chunk, UINT32 node_size,
                             btrfs_root* root_root, btrfs_root* extent_root) {
    LIST_ENTRY* le;
    
    le = roots->Flink;
    while (le != roots) {
        btrfs_root* r = CONTAINING_RECORD(le, btrfs_root, list_entry);
        btrfs_chunk* c = r->id == BTRFS_ROOT_CHUNK ? sys_chunk : metadata_chunk;
        EXTENT_ITEM_METADATA eim;
        
        r->header.address = get_next_address(c);
        r->c = c;
        c->lastoff = r->header.address + node_size;
        c->used += node_size;
        
        eim.ei.refcount = 1;
        eim.ei.generation = 1;
        eim.ei.flags = EXTENT_ITEM_TREE_BLOCK;
        eim.type = TYPE_TREE_BLOCK_REF;
        eim.tbr.offset = r->id;
        
        // FIXME - support non-skinny EXTENT_ITEM
        add_item(extent_root, r->header.address, TYPE_METADATA_ITEM, 0, &eim, sizeof(EXTENT_ITEM_METADATA));
        
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

static NTSTATUS write_data(HANDLE h, UINT64 address, btrfs_chunk* c, void* data, ULONG size) {
    NTSTATUS Status;
    UINT16 i;
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

static NTSTATUS write_roots(HANDLE h, LIST_ENTRY* roots, UINT32 node_size, BTRFS_UUID* fsuuid, BTRFS_UUID* chunkuuid) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    UINT8* tree;
    
#ifndef __REACTOS__
    tree = malloc(node_size);
#else
    tree = RtlAllocateHeap(RtlGetProcessHeap(), 0, node_size);
#endif
    
    le = roots->Flink;
    while (le != roots) {
        btrfs_root* r = CONTAINING_RECORD(le, btrfs_root, list_entry);
        UINT8* dp;
        leaf_node* ln;
        UINT32 crc32;

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
                
                ln->offset = dp - tree - sizeof(tree_header);
            } else
                ln->offset = 0;
            
            ln = &ln[1];
            
            r->header.num_items++;
            
            le2 = le2->Flink;
        }
        
        memcpy(tree, &r->header, sizeof(tree_header));
        
        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&((tree_header*)tree)->fs_uuid, node_size - sizeof(((tree_header*)tree)->csum));
        memcpy(tree, &crc32, sizeof(UINT32));
        
        Status = write_data(h, r->header.address, r->c, tree, node_size);
        if (!NT_SUCCESS(Status)) {
#ifndef __REACTOS__
            free(tree);
#else
            RtlFreeHeap(RtlGetProcessHeap(), 0, tree);
#endif
            return Status;
        }
        
        le = le->Flink;
    }
    
#ifndef __REACTOS__
    free(tree);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, tree);
#endif
    
    return STATUS_SUCCESS;
}

#ifndef __REACTOS__
static void get_uuid(BTRFS_UUID* uuid) {
#else
static void get_uuid(BTRFS_UUID* uuid, ULONG* seed) {
#endif
    UINT8 i;
    
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
static void init_device(btrfs_dev* dev, UINT64 id, UINT64 size, BTRFS_UUID* fsuuid, UINT32 sector_size) {
#else
static void init_device(btrfs_dev* dev, UINT64 id, UINT64 size, BTRFS_UUID* fsuuid, UINT32 sector_size, ULONG* seed) {
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

#ifdef __REACTOS__
NTSTATUS NTAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                 ULONG *utf8_bytes_written,
                                 const WCHAR *uni_src, ULONG uni_bytes)
{
    NTSTATUS status;
    ULONG i;
    ULONG written;
    ULONG ch;
    BYTE utf8_ch[4];
    ULONG utf8_ch_len;

    if (!uni_src)
        return STATUS_INVALID_PARAMETER_4;
    if (!utf8_bytes_written)
        return STATUS_INVALID_PARAMETER;
    if (utf8_dest && uni_bytes % sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER_5;

    written = 0;
    status = STATUS_SUCCESS;

    for (i = 0; i < uni_bytes / sizeof(WCHAR); i++)
    {
        /* decode UTF-16 into ch */
        ch = uni_src[i];
        if (ch >= 0xdc00 && ch <= 0xdfff)
        {
            ch = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        else if (ch >= 0xd800 && ch <= 0xdbff)
        {
            if (i + 1 < uni_bytes / sizeof(WCHAR))
            {
                ch -= 0xd800;
                ch <<= 10;
                if (uni_src[i + 1] >= 0xdc00 && uni_src[i + 1] <= 0xdfff)
                {
                    ch |= uni_src[i + 1] - 0xdc00;
                    ch += 0x010000;
                    i++;
                }
                else
                {
                    ch = 0xfffd;
                    status = STATUS_SOME_NOT_MAPPED;
                }
            }
            else
            {
                ch = 0xfffd;
                status = STATUS_SOME_NOT_MAPPED;
            }
        }

        /* encode ch as UTF-8 */
        ASSERT(ch <= 0x10ffff);
        if (ch < 0x80)
        {
            utf8_ch[0] = ch & 0x7f;
            utf8_ch_len = 1;
        }
        else if (ch < 0x800)
        {
            utf8_ch[0] = 0xc0 | (ch >>  6 & 0x1f);
            utf8_ch[1] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 2;
        }
        else if (ch < 0x10000)
        {
            utf8_ch[0] = 0xe0 | (ch >> 12 & 0x0f);
            utf8_ch[1] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 3;
        }
        else if (ch < 0x200000)
        {
            utf8_ch[0] = 0xf0 | (ch >> 18 & 0x07);
            utf8_ch[1] = 0x80 | (ch >> 12 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[3] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 4;
        }

        if (!utf8_dest)
        {
            written += utf8_ch_len;
            continue;
        }

        if (utf8_bytes_max >= utf8_ch_len)
        {
            memcpy(utf8_dest, utf8_ch, utf8_ch_len);
            utf8_dest += utf8_ch_len;
            utf8_bytes_max -= utf8_ch_len;
            written += utf8_ch_len;
        }
        else
        {
            utf8_bytes_max = 0;
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    *utf8_bytes_written = written;
    return status;
}
#endif

static NTSTATUS write_superblocks(HANDLE h, btrfs_dev* dev, btrfs_root* chunk_root, btrfs_root* root_root, btrfs_root* extent_root,
                                  btrfs_chunk* sys_chunk, UINT32 node_size, BTRFS_UUID* fsuuid, UINT32 sector_size, PUNICODE_STRING label) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    ULONG sblen;
    int i;
    UINT32 crc32;
    superblock* sb;
    KEY* key;
    UINT64 bytes_used;
    LIST_ENTRY* le;
    
    sblen = sizeof(sb);
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
    
#ifndef __REACTOS__
    sb = malloc(sblen);
    memset(sb, 0, sblen);
#else
    sb = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sblen);
#endif
    
    sb->uuid = *fsuuid;
    sb->flags = 1;
    sb->magic = BTRFS_MAGIC;
    sb->generation = 1;
    sb->root_tree_addr = root_root->header.address;
    sb->chunk_tree_addr = chunk_root->header.address;
    sb->total_bytes = dev->dev_item.num_bytes;
    sb->bytes_used = bytes_used;
    sb->root_dir_objectid = 6;
    sb->num_devices = 1;
    sb->sector_size = sector_size;
    sb->node_size = node_size;
    sb->leaf_size = node_size;
    sb->stripe_size = sector_size;
    sb->n = sizeof(KEY) + sizeof(CHUNK_ITEM) + (sys_chunk->chunk_item->num_stripes * sizeof(CHUNK_ITEM_STRIPE));
    sb->chunk_root_generation = 1;
    sb->incompat_flags = BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF | BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF | BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA;
    memcpy(&sb->dev_item, &dev->dev_item, sizeof(DEV_ITEM));
    
    if (label->Length > 0) {
        int i;
        ULONG utf8len;
        
        for (i = 0; i < label->Length / sizeof(WCHAR); i++) {
            if (label->Buffer[i] == '/' || label->Buffer[i] == '\\') {
#ifndef __REACTOS__
                free(sb);
#else
                RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
                return STATUS_INVALID_VOLUME_LABEL;
            }
        }
        
        Status = RtlUnicodeToUTF8N(NULL, 0, &utf8len, label->Buffer, label->Length);
        if (!NT_SUCCESS(Status)) {
#ifndef __REACTOS__
            free(sb);
#else
            RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
            return Status;
        }
        
        if (utf8len > MAX_LABEL_SIZE) {
#ifndef __REACTOS__
            free(sb);
#else
            RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
            return STATUS_INVALID_VOLUME_LABEL;
        }
        
        Status = RtlUnicodeToUTF8N((PCHAR)&sb->label, MAX_LABEL_SIZE, &utf8len, label->Buffer, label->Length);
        if (!NT_SUCCESS(Status)) {
#ifndef __REACTOS__
            free(sb);
#else
            RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
            return Status;
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
        
        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));
        memcpy(&sb->checksum, &crc32, sizeof(UINT32));
        
        off.QuadPart = superblock_addrs[i];
        
        Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, sb, sblen, &off, NULL);
        if (!NT_SUCCESS(Status)) {
#ifndef __REACTOS__
            free(sb);
#else
            RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
            return Status;
        }
        
        i++;
    }
    
#ifndef __REACTOS__
    free(sb);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
    
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

static void init_fs_tree(btrfs_root* r, UINT32 node_size) {
    INODE_ITEM ii;
    INODE_REF* ir;
#ifndef __REACTOS__
    SYSTEMTIME systime;
#endif
    FILETIME filetime;
    LARGE_INTEGER time;
    
    memset(&ii, 0, sizeof(INODE_ITEM));
    
    ii.generation = 1;
    ii.st_blocks = node_size;
    ii.st_nlink = 1;
    ii.st_mode = 040755;
    
#ifndef __REACTOS__
    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &filetime);
#else
    GetSystemTimeAsFileTime(&filetime);
#endif
    time.LowPart = filetime.dwLowDateTime;
    time.HighPart = filetime.dwHighDateTime;

    win_time_to_unix(time, &ii.st_atime);
    ii.st_ctime = ii.st_mtime = ii.st_atime;
    
    add_item(r, SUBVOL_ROOT_INODE, TYPE_INODE_ITEM, 0, &ii, sizeof(INODE_ITEM));
    
#ifndef __REACTOS__
    ir = malloc(sizeof(INODE_REF) - 1 + 2);
#else
    ir = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(INODE_REF) - 1 + 2);
#endif
    
    ir->index = 0;
    ir->n = 2;
    ir->name[0] = '.';
    ir->name[1] = '.';
    
    add_item(r, SUBVOL_ROOT_INODE, TYPE_INODE_REF, SUBVOL_ROOT_INODE, ir, sizeof(INODE_REF) - 1 + ir->n);
    
#ifndef __REACTOS__
    free(ir);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, ir);
#endif
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
    UINT8* mb;
    
#ifndef __REACTOS__
    mb = malloc(0x100000);
#else
    mb = RtlAllocateHeap(RtlGetProcessHeap(), 0, 0x100000);
#endif
    memset(mb, 0, 0x100000);
    
    zero.QuadPart = 0;
    
    Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, mb, 0x100000, &zero, NULL);
    
#ifndef __REACTOS__
    free(mb);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, mb);
#endif
    
    return Status;
}

static BOOL is_ssd(HANDLE h) {
    ULONG aptelen;
    ATA_PASS_THROUGH_EX* apte;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    IDENTIFY_DEVICE_DATA* idd;
    
    aptelen = sizeof(ATA_PASS_THROUGH_EX) + 512;
#ifndef __REACTOS__
    apte = malloc(aptelen);
    
    RtlZeroMemory(apte, aptelen);
#else
    apte = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, aptelen);
#endif
    
    apte->Length = sizeof(ATA_PASS_THROUGH_EX);
    apte->AtaFlags = ATA_FLAGS_DATA_IN;
    apte->DataTransferLength = aptelen - sizeof(ATA_PASS_THROUGH_EX);
    apte->TimeOutValue = 3;
    apte->DataBufferOffset = apte->Length;
    apte->CurrentTaskFile[6] = IDE_COMMAND_IDENTIFY;
    
    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_ATA_PASS_THROUGH, apte, aptelen, apte, aptelen);
    
    if (NT_SUCCESS(Status)) {
        idd = (IDENTIFY_DEVICE_DATA*)((UINT8*)apte + sizeof(ATA_PASS_THROUGH_EX));
        
        if (idd->NominalMediaRotationRate == 1)
            return TRUE;
    }
    
    return FALSE;
}

static NTSTATUS write_btrfs(HANDLE h, UINT64 size, PUNICODE_STRING label, UINT32 sector_size) {
    NTSTATUS Status;
    UINT32 node_size;
    LIST_ENTRY roots, chunks;
    btrfs_root *root_root, *chunk_root, *extent_root, *dev_root, *fs_root, *reloc_root;
    btrfs_chunk *sys_chunk, *metadata_chunk; 
    btrfs_dev dev;
    BTRFS_UUID fsuuid, chunkuuid;
    BOOL ssd;
#ifdef __REACTOS__
    ULONG seed;
#endif
    
#ifndef __REACTOS__
    srand(time(0));
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
    
    metadata_chunk = add_chunk(&chunks, BLOCK_FLAG_METADATA | (ssd ? 0 : BLOCK_FLAG_DUPLICATE), chunk_root, &dev, dev_root, &chunkuuid, sector_size);
    if (!metadata_chunk)
        return STATUS_INTERNAL_ERROR;
    
    node_size = 0x4000;
    assign_addresses(&roots, sys_chunk, metadata_chunk, node_size, root_root, extent_root);
    
    add_item(chunk_root, 1, TYPE_DEV_ITEM, dev.dev_item.dev_id, &dev.dev_item, sizeof(DEV_ITEM));
    
    init_fs_tree(fs_root, node_size);
    init_fs_tree(reloc_root, node_size);
    
    add_block_group_items(&chunks, extent_root);
    
    Status = write_roots(h, &roots, node_size, &fsuuid, &chunkuuid);
    if (!NT_SUCCESS(Status))
        return Status;
    
    Status = clear_first_megabyte(h);
    if (!NT_SUCCESS(Status))
        return Status;
    
    Status = write_superblocks(h, &dev, chunk_root, root_root, extent_root, sys_chunk, node_size, &fsuuid, sector_size, label);
    if (!NT_SUCCESS(Status))
        return Status;
    
    free_roots(&roots);
    free_chunks(&chunks);
    
    return STATUS_SUCCESS;
}

static BOOL look_for_device(btrfs_filesystem* bfs, BTRFS_UUID* devuuid) {
    UINT32 i;
    btrfs_filesystem_device* dev;
    
    for (i = 0; i < bfs->num_devices; i++) {
        if (i == 0)
            dev = &bfs->device;
        else
            dev = (btrfs_filesystem_device*)((UINT8*)dev + offsetof(btrfs_filesystem_device, name[0]) + dev->name_length);
        
        if (RtlCompareMemory(&dev->uuid, devuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID))
            return TRUE;
    }
    
    return FALSE;
}

static BOOL is_mounted_multi_device(HANDLE h, UINT32 sector_size) {
    NTSTATUS Status;
    superblock* sb;
    ULONG sblen;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    BTRFS_UUID fsuuid, devuuid;
    UINT32 crc32;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES atts;
    HANDLE h2;
    btrfs_filesystem *bfs = NULL, *bfs2;
    ULONG bfssize;
    BOOL ret = FALSE;
    
    static WCHAR btrfs[] = L"\\Btrfs";
    
    sblen = sizeof(sb);
    if (sblen & (sector_size - 1))
        sblen = (sblen & sector_size) + sector_size;
    
#ifndef __REACTOS__
    sb = malloc(sblen);
#else
    sb = RtlAllocateHeap(RtlGetProcessHeap(), 0, sblen);
#endif
    
    off.QuadPart = superblock_addrs[0];
    
    Status = NtReadFile(h, NULL, NULL, NULL, &iosb, sb, sblen, &off, NULL);
    if (!NT_SUCCESS(Status)) {
#ifndef __REACTOS__
        free(sb);
#else
        RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
        return FALSE;
    }
    
    if (sb->magic != BTRFS_MAGIC) {
#ifndef __REACTOS__
        free(sb);
#else
        RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
        return FALSE;
    }
    
    crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&sb->uuid, (ULONG)sizeof(superblock) - sizeof(sb->checksum));
    if (crc32 != *((UINT32*)sb)) {
#ifndef __REACTOS__
        free(sb);
#else
        RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
        return FALSE;
    }
    
    fsuuid = sb->uuid;
    devuuid = sb->dev_item.device_uuid;
    
#ifndef __REACTOS__
    free(sb);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, sb);
#endif
    
    us.Length = us.MaximumLength = wcslen(btrfs) * sizeof(WCHAR);
    us.Buffer = btrfs;
    
    InitializeObjectAttributes(&atts, &us, 0, NULL, NULL);
    
    Status = NtOpenFile(&h2, SYNCHRONIZE | FILE_READ_ATTRIBUTES, &atts, &iosb,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status)) // not a problem, it usually just means the driver isn't loaded
        return FALSE;
    
    bfssize = 0;
    
    do {
        bfssize += 1024;
        
#ifndef __REACTOS__
        if (bfs) free(bfs);
        bfs = malloc(bfssize);
#else
        if (bfs) RtlFreeHeap(RtlGetProcessHeap(), 0, bfs);
        bfs = RtlAllocateHeap(RtlGetProcessHeap(), 0, bfssize);
#endif
        
        Status = NtDeviceIoControlFile(h2, NULL, NULL, NULL, &iosb, IOCTL_BTRFS_QUERY_FILESYSTEMS, NULL, 0, bfs, bfssize);
        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            NtClose(h2);
            return FALSE;
        }
    } while (Status == STATUS_BUFFER_OVERFLOW);
    
    if (!NT_SUCCESS(Status))
        goto end;
    
    if (bfs->num_devices != 0) {
        bfs2 = bfs;
        while (TRUE) {
            if (RtlCompareMemory(&bfs2->uuid, &fsuuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                if (bfs2->num_devices == 1)
                    ret = FALSE;
                else
                    ret = look_for_device(bfs2, &devuuid);
                
                goto end;
            }
            
            if (bfs2->next_entry == 0)
                break;
            else
                bfs2 = (btrfs_filesystem*)((UINT8*)bfs2 + bfs2->next_entry);
        }
    }
    
end:
    NtClose(h2);
    
    if (bfs)
#ifndef __REACTOS__
        free(bfs);
#else
        RtlFreeHeap(RtlGetProcessHeap(), 0, bfs);
#endif
    
    return ret;
}

static void add_drive_letter(HANDLE h) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    MOUNTDEV_NAME mdn, *mdn2;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES attr;
    HANDLE mountmgr;
    MOUNTMGR_DRIVE_LETTER_INFORMATION mdli;
    
    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &mdn, sizeof(mdn));
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        return;
    
#ifndef __REACTOS__
    mdn2 = malloc(offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength);
#else
    mdn2 = RtlAllocateHeap(RtlGetProcessHeap(), 0, offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength);
#endif
    
    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdn2, offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength);
    if (!NT_SUCCESS(Status))
        goto end;
    
    RtlInitUnicodeString(&us, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&attr, &us, 0, NULL, NULL);
    
    Status = NtOpenFile(&mountmgr, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                        FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);
    
    if (!NT_SUCCESS(Status))
        goto end;
    
    // MOUNTDEV_NAME is identical to MOUNTMGR_TARGET_NAME
    Status = NtDeviceIoControlFile(mountmgr, NULL, NULL, NULL, &iosb, IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
                                   mdn2, offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength, NULL, 0);
    if (!NT_SUCCESS(Status))
        goto end2;
    
    // MOUNTDEV_NAME is identical to MOUNTMGR_DRIVE_LETTER_TARGET
    Status = NtDeviceIoControlFile(mountmgr, NULL, NULL, NULL, &iosb, IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                   mdn2, offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength, &mdli, sizeof(mdli));
    if (!NT_SUCCESS(Status))
        goto end2;

end2:    
    NtClose(mountmgr);
    
end:
#ifndef __REACTOS__
    free(mdn2);
#else
    RtlFreeHeap(RtlGetProcessHeap(), 0, mdn2);
#endif
}

#ifndef __REACTOS__
NTSTATUS NTAPI FormatEx(PUNICODE_STRING DriveRoot, FMIFS_MEDIA_FLAG MediaFlag, PUNICODE_STRING Label,
#else
NTSTATUS NTAPI BtrfsFormatEx(PUNICODE_STRING DriveRoot, FMIFS_MEDIA_FLAG MediaFlag, PUNICODE_STRING Label,
#endif
                        BOOLEAN QuickFormat, ULONG ClusterSize, PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;
    HANDLE h;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    GET_LENGTH_INFORMATION gli;
    DISK_GEOMETRY_EX dgex;
    UINT32 sector_size;
    
    InitializeObjectAttributes(&attr, DriveRoot, 0, NULL, NULL);
    
    Status = NtOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                        FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);
    
    if (!NT_SUCCESS(Status))
        return Status;
    
    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli));
    if (!NT_SUCCESS(Status)) {
        NtClose(h);
        return Status;
    }

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &dgex, sizeof(dgex));
    if (!NT_SUCCESS(Status)) {
        NtClose(h);
        return Status;
    }
    
    sector_size = dgex.Geometry.BytesPerSector;
    
    if (sector_size == 0x200 || sector_size == 0)
        sector_size = 0x1000;
    
    if (Callback) {
        ULONG pc = 0;
        Callback(PROGRESS, 0, (PVOID)&pc);
    }
    
    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
    
    if (is_mounted_multi_device(h, sector_size)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    Status = write_btrfs(h, gli.Length.QuadPart, Label, sector_size);
    
    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
    
end:
    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
    
    if (NT_SUCCESS(Status))
        add_drive_letter(h);
    
    NtClose(h);
    
    if (Callback) {
        BOOL success = NT_SUCCESS(Status);
        Callback(DONE, 0, (PVOID)&success);
    }
    
    return Status;
}

#ifndef __REACTOS__
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH)
        module = (HMODULE)hModule;
        
    return TRUE;
}
#endif
