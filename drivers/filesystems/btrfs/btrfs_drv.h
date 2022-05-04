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

#pragma once

#ifndef __REACTOS__
#undef _WIN32_WINNT
#undef NTDDI_VERSION

#define _WIN32_WINNT 0x0601
#define NTDDI_VERSION 0x06020000 // Win 8
#define _CRT_SECURE_NO_WARNINGS
#define _NO_CRT_STDIO_INLINE
#endif /* __REACTOS__ */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4163)
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <ntifs.h>
#include <ntddk.h>
#ifdef __REACTOS__
#include <ntdddisk.h>
#endif /* __REACTOS__ */
#include <mountmgr.h>
#ifdef __REACTOS__
#include <rtlfuncs.h>
#include <iotypes.h>
#include <pseh/pseh2.h>
#endif /* __REACTOS__ */
#include <windef.h>
#include <wdm.h>

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "btrfs.h"
#include "btrfsioctl.h"

#ifdef __REACTOS__
C_ASSERT(sizeof(bool) == 1);
#endif

#ifdef _DEBUG
// #define DEBUG_FCB_REFCOUNTS
// #define DEBUG_LONG_MESSAGES
// #define DEBUG_FLUSH_TIMES
// #define DEBUG_CHUNK_LOCKS
// #define DEBUG_TRIM_EMULATION
#define DEBUG_PARANOID
#endif

#define UNUSED(x) (void)(x)

#define BTRFS_NODE_TYPE_CCB 0x2295
#define BTRFS_NODE_TYPE_FCB 0x2296

#define ALLOC_TAG 0x7442484D //'MHBt'
#define ALLOC_TAG_ZLIB 0x7A42484D //'MHBz'

#define UID_NOBODY 65534
#define GID_NOBODY 65534

#define EA_NTACL "security.NTACL"
#define EA_NTACL_HASH 0x45922146

#define EA_DOSATTRIB "user.DOSATTRIB"
#define EA_DOSATTRIB_HASH 0x914f9939

#define EA_REPARSE "user.reparse"
#define EA_REPARSE_HASH 0xfabad1fe

#define EA_EA "user.EA"
#define EA_EA_HASH 0x8270dd43

#define EA_CASE_SENSITIVE "user.casesensitive"
#define EA_CASE_SENSITIVE_HASH 0x1a9d97d4

#define EA_PROP_COMPRESSION "btrfs.compression"
#define EA_PROP_COMPRESSION_HASH 0x20ccdf69

#define MAX_EXTENT_SIZE 0x8000000 // 128 MB
#define COMPRESSED_EXTENT_SIZE 0x20000 // 128 KB

#define READ_AHEAD_GRANULARITY COMPRESSED_EXTENT_SIZE // really ought to be a multiple of COMPRESSED_EXTENT_SIZE

#ifndef IO_REPARSE_TAG_LX_SYMLINK

#define IO_REPARSE_TAG_LX_SYMLINK 0xa000001d

#define IO_REPARSE_TAG_AF_UNIX          0x80000023
#define IO_REPARSE_TAG_LX_FIFO          0x80000024
#define IO_REPARSE_TAG_LX_CHR           0x80000025
#define IO_REPARSE_TAG_LX_BLK           0x80000026

#endif

#define BTRFS_VOLUME_PREFIX L"\\Device\\Btrfs{"

#if defined(_MSC_VER) || defined(__clang__)
#define try __try
#define except __except
#define finally __finally
#define leave __leave
#else
#define try if (1)
#define except(x) if (0 && (x))
#define finally if (1)
#define leave
#endif

#ifndef __REACTOS__
#ifndef InterlockedIncrement64
#define InterlockedIncrement64(a) __sync_add_and_fetch(a, 1)
#endif
#endif // __REACTOS__

#ifndef FILE_SUPPORTS_BLOCK_REFCOUNTING
#define FILE_SUPPORTS_BLOCK_REFCOUNTING 0x08000000
#endif

#ifndef FILE_SUPPORTS_POSIX_UNLINK_RENAME
#define FILE_SUPPORTS_POSIX_UNLINK_RENAME 0x00000400
#endif

#ifndef FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL 0x00020000
#endif

#ifndef __REACTOS__
#ifndef _MSC_VER
typedef struct _FILE_ID_128 {
    UCHAR Identifier[16];
} FILE_ID_128, *PFILE_ID_128;

#define FILE_CS_FLAG_CASE_SENSITIVE_DIR                 1
#endif
#else
typedef struct _FILE_ID_128 {
    UCHAR Identifier[16];
} FILE_ID_128, *PFILE_ID_128;

#define FILE_CS_FLAG_CASE_SENSITIVE_DIR                 1
#endif // __REACTOS__

typedef struct _DUPLICATE_EXTENTS_DATA {
    HANDLE FileHandle;
    LARGE_INTEGER SourceFileOffset;
    LARGE_INTEGER TargetFileOffset;
    LARGE_INTEGER ByteCount;
} DUPLICATE_EXTENTS_DATA, *PDUPLICATE_EXTENTS_DATA;

#define FSCTL_DUPLICATE_EXTENTS_TO_FILE CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 209, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _FSCTL_GET_INTEGRITY_INFORMATION_BUFFER {
    WORD ChecksumAlgorithm;
    WORD Reserved;
    DWORD Flags;
    DWORD ChecksumChunkSizeInBytes;
    DWORD ClusterSizeInBytes;
} FSCTL_GET_INTEGRITY_INFORMATION_BUFFER, *PFSCTL_GET_INTEGRITY_INFORMATION_BUFFER;

typedef struct _FSCTL_SET_INTEGRITY_INFORMATION_BUFFER {
    WORD ChecksumAlgorithm;
    WORD Reserved;
    DWORD Flags;
} FSCTL_SET_INTEGRITY_INFORMATION_BUFFER, *PFSCTL_SET_INTEGRITY_INFORMATION_BUFFER;

#define FSCTL_GET_INTEGRITY_INFORMATION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 159, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_INTEGRITY_INFORMATION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 160, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#ifndef __REACTOS__
#ifndef _MSC_VER
#define __drv_aliasesMem
#define _Dispatch_type_(a)
#define _Lock_level_order_(a,b)
#endif
#endif // __REACTOS__

_Create_lock_level_(tree_lock)
_Create_lock_level_(fcb_lock)
_Lock_level_order_(tree_lock, fcb_lock)

#define MAX_HASH_SIZE 32

struct _device_extension;

typedef struct _fcb_nonpaged {
    FAST_MUTEX HeaderMutex;
    SECTION_OBJECT_POINTERS segment_object;
    ERESOURCE resource;
    ERESOURCE paging_resource;
    ERESOURCE dir_children_lock;
} fcb_nonpaged;

struct _root;

typedef struct {
    uint64_t offset;
    uint16_t datalen;
    bool unique;
    bool ignore;
    bool inserted;
    void* csum;

    LIST_ENTRY list_entry;

    EXTENT_DATA extent_data;
} extent;

typedef struct {
    uint64_t parent;
    uint64_t index;
    UNICODE_STRING name;
    ANSI_STRING utf8;
    LIST_ENTRY list_entry;
} hardlink;

struct _file_ref;

typedef struct {
    KEY key;
    uint64_t index;
    uint8_t type;
    ANSI_STRING utf8;
    uint32_t hash;
    UNICODE_STRING name;
    uint32_t hash_uc;
    UNICODE_STRING name_uc;
    ULONG size;
    struct _file_ref* fileref;
    bool root_dir;
    LIST_ENTRY list_entry_index;
    LIST_ENTRY list_entry_hash;
    LIST_ENTRY list_entry_hash_uc;
} dir_child;

enum prop_compression_type {
    PropCompression_None,
    PropCompression_Zlib,
    PropCompression_LZO,
    PropCompression_ZSTD
};

typedef struct {
    LIST_ENTRY list_entry;
    USHORT namelen;
    USHORT valuelen;
    bool dirty;
    char data[1];
} xattr;

typedef struct _fcb {
    FSRTL_ADVANCED_FCB_HEADER Header;
    struct _fcb_nonpaged* nonpaged;
    LONG refcount;
    POOL_TYPE pool_type;
    struct _device_extension* Vcb;
    struct _root* subvol;
    uint64_t inode;
    uint32_t hash;
    uint8_t type;
    INODE_ITEM inode_item;
    SECURITY_DESCRIPTOR* sd;
    FILE_LOCK lock;
    bool deleted;
    PKTHREAD lazy_writer_thread;
    ULONG atts;
    SHARE_ACCESS share_access;
    bool csum_loaded;
    LIST_ENTRY extents;
    ANSI_STRING reparse_xattr;
    ANSI_STRING ea_xattr;
    ULONG ealen;
    LIST_ENTRY hardlinks;
    struct _file_ref* fileref;
    bool inode_item_changed;
    enum prop_compression_type prop_compression;
    LIST_ENTRY xattrs;
    bool marked_as_orphan;
    bool case_sensitive;
    bool case_sensitive_set;
    OPLOCK oplock;

    LIST_ENTRY dir_children_index;
    LIST_ENTRY dir_children_hash;
    LIST_ENTRY dir_children_hash_uc;
    LIST_ENTRY** hash_ptrs;
    LIST_ENTRY** hash_ptrs_uc;

    bool dirty;
    bool sd_dirty, sd_deleted;
    bool atts_changed, atts_deleted;
    bool extents_changed;
    bool reparse_xattr_changed;
    bool ea_changed;
    bool prop_compression_changed;
    bool xattrs_changed;
    bool created;

    bool ads;
    uint32_t adshash;
    ULONG adsmaxlen;
    ANSI_STRING adsxattr;
    ANSI_STRING adsdata;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_all;
    LIST_ENTRY list_entry_dirty;
} fcb;

typedef struct _file_ref {
    fcb* fcb;
    ANSI_STRING oldutf8;
    uint64_t oldindex;
    bool delete_on_close;
    bool posix_delete;
    bool deleted;
    bool created;
    LIST_ENTRY children;
    LONG refcount;
    LONG open_count;
    struct _file_ref* parent;
    dir_child* dc;

    bool dirty;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_dirty;
} file_ref;

typedef struct {
    HANDLE thread;
    struct _ccb* ccb;
    void* context;
    KEVENT cleared_event;
    bool cancelling;
    LIST_ENTRY list_entry;
} send_info;

typedef struct _ccb {
    USHORT NodeType;
    CSHORT NodeSize;
    ULONG disposition;
    ULONG options;
    uint64_t query_dir_offset;
    UNICODE_STRING query_string;
    bool has_wildcard;
    bool specific_file;
    bool manage_volume_privilege;
    bool allow_extended_dasd_io;
    bool reserving;
    ACCESS_MASK access;
    file_ref* fileref;
    UNICODE_STRING filename;
    ULONG ea_index;
    bool case_sensitive;
    bool user_set_creation_time;
    bool user_set_access_time;
    bool user_set_write_time;
    bool user_set_change_time;
    bool lxss;
    send_info* send;
    NTSTATUS send_status;
} ccb;

struct _device_extension;

typedef struct {
    uint64_t address;
    uint64_t generation;
    struct _tree* tree;
} tree_holder;

typedef struct _tree_data {
    KEY key;
    LIST_ENTRY list_entry;
    bool ignore;
    bool inserted;

    union {
        tree_holder treeholder;

        struct {
            uint16_t size;
            uint8_t* data;
        };
    };
} tree_data;

typedef struct {
    FAST_MUTEX mutex;
} tree_nonpaged;

typedef struct _tree {
    tree_nonpaged* nonpaged;
    tree_header header;
    uint32_t hash;
    bool has_address;
    uint32_t size;
    struct _device_extension* Vcb;
    struct _tree* parent;
    tree_data* paritem;
    struct _root* root;
    LIST_ENTRY itemlist;
    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_hash;
    uint64_t new_address;
    bool has_new_address;
    bool updated_extents;
    bool write;
    bool is_unique;
    bool uniqueness_determined;
    uint8_t* buf;
} tree;

typedef struct {
    ERESOURCE load_tree_lock;
} root_nonpaged;

typedef struct _root {
    uint64_t id;
    LONGLONG lastinode; // signed so we can use InterlockedIncrement64
    tree_holder treeholder;
    root_nonpaged* nonpaged;
    ROOT_ITEM root_item;
    bool dirty;
    bool received;
    PEPROCESS reserved;
    uint64_t parent;
    LONG send_ops;
    uint64_t fcbs_version;
    bool checked_for_orphans;
    bool dropped;
    LIST_ENTRY fcbs;
    LIST_ENTRY* fcbs_ptrs[256];
    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_dirty;
} root;

enum batch_operation {
    Batch_Delete,
    Batch_DeleteInode,
    Batch_DeleteDirItem,
    Batch_DeleteInodeRef,
    Batch_DeleteInodeExtRef,
    Batch_DeleteXattr,
    Batch_DeleteExtentData,
    Batch_DeleteFreeSpace,
    Batch_Insert,
    Batch_SetXattr,
    Batch_DirItem,
    Batch_InodeRef,
    Batch_InodeExtRef,
};

typedef struct {
    KEY key;
    void* data;
    uint16_t datalen;
    enum batch_operation operation;
    LIST_ENTRY list_entry;
} batch_item;

typedef struct {
    root* r;
    LIST_ENTRY items;
    LIST_ENTRY list_entry;
} batch_root;

typedef struct {
    tree* tree;
    tree_data* item;
} traverse_ptr;

typedef struct _root_cache {
    root* root;
    struct _root_cache* next;
} root_cache;

typedef struct {
    uint64_t address;
    uint64_t size;
    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_size;
} space;

typedef struct {
    PDEVICE_OBJECT devobj;
    PFILE_OBJECT fileobj;
    DEV_ITEM devitem;
    bool removable;
    bool seeding;
    bool readonly;
    bool reloc;
    bool trim;
    bool can_flush;
    ULONG change_count;
    ULONG disk_num;
    ULONG part_num;
    uint64_t stats[5];
    bool stats_changed;
    LIST_ENTRY space;
    LIST_ENTRY list_entry;
    ULONG num_trim_entries;
    LIST_ENTRY trim_list;
} device;

typedef struct {
    uint64_t start;
    uint64_t length;
    PETHREAD thread;
    LIST_ENTRY list_entry;
} range_lock;

typedef struct {
    uint64_t address;
    ULONG* bmparr;
    ULONG bmplen;
    RTL_BITMAP bmp;
    LIST_ENTRY list_entry;
    uint8_t data[1];
} partial_stripe;

typedef struct {
    CHUNK_ITEM* chunk_item;
    uint16_t size;
    uint64_t offset;
    uint64_t used;
    uint64_t oldused;
    device** devices;
    fcb* cache;
    fcb* old_cache;
    LIST_ENTRY space;
    LIST_ENTRY space_size;
    LIST_ENTRY deleting;
    LIST_ENTRY changed_extents;
    LIST_ENTRY range_locks;
    ERESOURCE range_locks_lock;
    KEVENT range_locks_event;
    ERESOURCE lock;
    ERESOURCE changed_extents_lock;
    bool created;
    bool readonly;
    bool reloc;
    bool last_alloc_set;
    bool cache_loaded;
    bool changed;
    bool space_changed;
    uint64_t last_alloc;
    uint16_t last_stripe;
    LIST_ENTRY partial_stripes;
    ERESOURCE partial_stripes_lock;
    ULONG balance_num;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_balance;
} chunk;

typedef struct {
    uint64_t address;
    uint64_t size;
    uint64_t old_size;
    uint64_t count;
    uint64_t old_count;
    bool no_csum;
    bool superseded;
    LIST_ENTRY refs;
    LIST_ENTRY old_refs;
    LIST_ENTRY list_entry;
} changed_extent;

typedef struct {
    uint8_t type;

    union {
        EXTENT_DATA_REF edr;
        SHARED_DATA_REF sdr;
    };

    LIST_ENTRY list_entry;
} changed_extent_ref;

typedef struct {
    KEY key;
    void* data;
    USHORT size;
    LIST_ENTRY list_entry;
} sys_chunk;

enum calc_thread_type {
    calc_thread_crc32c,
    calc_thread_xxhash,
    calc_thread_sha256,
    calc_thread_blake2,
    calc_thread_decomp_zlib,
    calc_thread_decomp_lzo,
    calc_thread_decomp_zstd,
    calc_thread_comp_zlib,
    calc_thread_comp_lzo,
    calc_thread_comp_zstd,
};

typedef struct {
    LIST_ENTRY list_entry;
    void* in;
    void* out;
    unsigned int inlen, outlen, off, space_left;
    LONG left, not_started;
    KEVENT event;
    enum calc_thread_type type;
    NTSTATUS Status;
} calc_job;

typedef struct {
    PDEVICE_OBJECT DeviceObject;
    HANDLE handle;
    KEVENT finished;
    unsigned int number;
    bool quit;
} drv_calc_thread;

typedef struct {
    ULONG num_threads;
    LIST_ENTRY job_list;
    KSPIN_LOCK spinlock;
    drv_calc_thread* threads;
    KEVENT event;
} drv_calc_threads;

typedef struct {
    bool ignore;
    bool compress;
    bool compress_force;
    uint8_t compress_type;
    bool readonly;
    uint32_t zlib_level;
    uint32_t zstd_level;
    uint32_t flush_interval;
    uint32_t max_inline;
    uint64_t subvol_id;
    bool skip_balance;
    bool no_barrier;
    bool no_trim;
    bool clear_cache;
    bool allow_degraded;
    bool no_root_dir;
} mount_options;

#define VCB_TYPE_FS         1
#define VCB_TYPE_CONTROL    2
#define VCB_TYPE_VOLUME     3
#define VCB_TYPE_PDO        4
#define VCB_TYPE_BUS        5

#define BALANCE_OPTS_DATA       0
#define BALANCE_OPTS_METADATA   1
#define BALANCE_OPTS_SYSTEM     2

typedef struct {
    HANDLE thread;
    uint64_t total_chunks;
    uint64_t chunks_left;
    btrfs_balance_opts opts[3];
    bool paused;
    bool stopping;
    bool removing;
    bool shrinking;
    bool dev_readonly;
    ULONG balance_num;
    NTSTATUS status;
    KEVENT event;
    KEVENT finished;
} balance_info;

typedef struct {
    uint64_t address;
    uint64_t device;
    bool recovered;
    bool is_metadata;
    bool parity;
    LIST_ENTRY list_entry;

    union {
        struct {
            uint64_t subvol;
            uint64_t offset;
            uint16_t filename_length;
            WCHAR filename[1];
        } data;

        struct {
            uint64_t root;
            uint8_t level;
            KEY firstitem;
        } metadata;
    };
} scrub_error;

typedef struct {
    HANDLE thread;
    ERESOURCE stats_lock;
    KEVENT event;
    KEVENT finished;
    bool stopping;
    bool paused;
    LARGE_INTEGER start_time;
    LARGE_INTEGER finish_time;
    LARGE_INTEGER resume_time;
    LARGE_INTEGER duration;
    uint64_t total_chunks;
    uint64_t chunks_left;
    uint64_t data_scrubbed;
    NTSTATUS error;
    ULONG num_errors;
    LIST_ENTRY errors;
} scrub_info;

struct _volume_device_extension;

typedef struct _device_extension {
    uint32_t type;
    mount_options options;
    PVPB Vpb;
    PDEVICE_OBJECT devobj;
    struct _volume_device_extension* vde;
    LIST_ENTRY devices;
#ifdef DEBUG_CHUNK_LOCKS
    LONG chunk_locks_held;
#endif
    uint64_t devices_loaded;
    superblock superblock;
    unsigned int sector_shift;
    unsigned int csum_size;
    bool readonly;
    bool removing;
    bool locked;
    bool lock_paused_balance;
    bool disallow_dismount;
    LONG page_file_count;
    bool trim;
    PFILE_OBJECT locked_fileobj;
    fcb* volume_fcb;
    fcb* dummy_fcb;
    file_ref* root_fileref;
    LONG open_files;
    _Has_lock_level_(fcb_lock) ERESOURCE fcb_lock;
    ERESOURCE fileref_lock;
    ERESOURCE load_lock;
    _Has_lock_level_(tree_lock) ERESOURCE tree_lock;
    PNOTIFY_SYNC NotifySync;
    LIST_ENTRY DirNotifyList;
    bool need_write;
    bool stats_changed;
    uint64_t data_flags;
    uint64_t metadata_flags;
    uint64_t system_flags;
    LIST_ENTRY roots;
    LIST_ENTRY drop_roots;
    root* chunk_root;
    root* root_root;
    root* extent_root;
    root* checksum_root;
    root* dev_root;
    root* uuid_root;
    root* data_reloc_root;
    root* space_root;
    bool log_to_phys_loaded;
    bool chunk_usage_found;
    LIST_ENTRY sys_chunks;
    LIST_ENTRY chunks;
    LIST_ENTRY trees;
    LIST_ENTRY trees_hash;
    LIST_ENTRY* trees_ptrs[256];
    FAST_MUTEX trees_list_mutex;
    LIST_ENTRY all_fcbs;
    LIST_ENTRY dirty_fcbs;
    ERESOURCE dirty_fcbs_lock;
    LIST_ENTRY dirty_filerefs;
    ERESOURCE dirty_filerefs_lock;
    LIST_ENTRY dirty_subvols;
    ERESOURCE dirty_subvols_lock;
    ERESOURCE chunk_lock;
    HANDLE flush_thread_handle;
    KTIMER flush_thread_timer;
    KEVENT flush_thread_finished;
    drv_calc_threads calcthreads;
    balance_info balance;
    scrub_info scrub;
    ERESOURCE send_load_lock;
    LONG running_sends;
    LIST_ENTRY send_ops;
    PFILE_OBJECT root_file;
    PAGED_LOOKASIDE_LIST tree_data_lookaside;
    PAGED_LOOKASIDE_LIST traverse_ptr_lookaside;
    PAGED_LOOKASIDE_LIST batch_item_lookaside;
    PAGED_LOOKASIDE_LIST fileref_lookaside;
    PAGED_LOOKASIDE_LIST fcb_lookaside;
    PAGED_LOOKASIDE_LIST name_bit_lookaside;
    NPAGED_LOOKASIDE_LIST range_lock_lookaside;
    NPAGED_LOOKASIDE_LIST fcb_np_lookaside;
    LIST_ENTRY list_entry;
} device_extension;

typedef struct {
    uint32_t type;
} control_device_extension;

typedef struct {
    uint32_t type;
    PDEVICE_OBJECT buspdo;
    PDEVICE_OBJECT attached_device;
    UNICODE_STRING bus_name;
} bus_device_extension;

typedef struct {
    BTRFS_UUID uuid;
    uint64_t devid;
    uint64_t generation;
    PDEVICE_OBJECT devobj;
    PFILE_OBJECT fileobj;
    UNICODE_STRING pnp_name;
    uint64_t size;
    bool seeding;
    bool had_drive_letter;
    void* notification_entry;
    ULONG disk_num;
    ULONG part_num;
    bool boot_volume;
    LIST_ENTRY list_entry;
} volume_child;

struct pdo_device_extension;

typedef struct _volume_device_extension {
    uint32_t type;
    UNICODE_STRING name;
    PDEVICE_OBJECT device;
    PDEVICE_OBJECT mounted_device;
    PDEVICE_OBJECT pdo;
    struct pdo_device_extension* pdode;
    UNICODE_STRING bus_name;
    PDEVICE_OBJECT attached_device;
    bool removing;
    bool dead;
    LONG open_count;
} volume_device_extension;

typedef struct pdo_device_extension {
    uint32_t type;
    BTRFS_UUID uuid;
    volume_device_extension* vde;
    PDEVICE_OBJECT pdo;
    bool removable;
    bool dont_report;

    uint64_t num_children;
    uint64_t children_loaded;
    ERESOURCE child_lock;
    LIST_ENTRY children;

    LIST_ENTRY list_entry;
} pdo_device_extension;

typedef struct {
    LIST_ENTRY listentry;
    PSID sid;
    uint32_t uid;
} uid_map;

typedef struct {
    LIST_ENTRY listentry;
    PSID sid;
    uint32_t gid;
} gid_map;

enum write_data_status {
    WriteDataStatus_Pending,
    WriteDataStatus_Success,
    WriteDataStatus_Error,
    WriteDataStatus_Cancelling,
    WriteDataStatus_Cancelled,
    WriteDataStatus_Ignore
};

struct _write_data_context;

typedef struct {
    struct _write_data_context* context;
    uint8_t* buf;
    PMDL mdl;
    device* device;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum write_data_status status;
    LIST_ENTRY list_entry;
} write_data_stripe;

typedef struct _write_data_context {
    KEVENT Event;
    LIST_ENTRY stripes;
    LONG stripes_left;
    bool need_wait;
    uint8_t *parity1, *parity2, *scratch;
    PMDL mdl, parity1_mdl, parity2_mdl;
} write_data_context;

typedef struct {
    uint64_t address;
    uint32_t length;
    uint8_t* data;
    chunk* c;
    bool allocated;
    LIST_ENTRY list_entry;
} tree_write;

typedef struct {
    UNICODE_STRING us;
    LIST_ENTRY list_entry;
} name_bit;

_Requires_lock_not_held_(Vcb->fcb_lock)
_Acquires_shared_lock_(Vcb->fcb_lock)
static __inline void acquire_fcb_lock_shared(device_extension* Vcb) {
    ExAcquireResourceSharedLite(&Vcb->fcb_lock, true);
}

_Requires_lock_not_held_(Vcb->fcb_lock)
_Acquires_exclusive_lock_(Vcb->fcb_lock)
static __inline void acquire_fcb_lock_exclusive(device_extension* Vcb) {
    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, true);
}

_Requires_lock_held_(Vcb->fcb_lock)
_Releases_lock_(Vcb->fcb_lock)
static __inline void release_fcb_lock(device_extension* Vcb) {
    ExReleaseResourceLite(&Vcb->fcb_lock);
}

static __inline void* map_user_buffer(PIRP Irp, ULONG priority) {
    if (!Irp->MdlAddress) {
        return Irp->UserBuffer;
    } else {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, priority);
    }
}

static __inline uint64_t unix_time_to_win(BTRFS_TIME* t) {
    return (t->seconds * 10000000) + (t->nanoseconds / 100) + 116444736000000000;
}

static __inline void win_time_to_unix(LARGE_INTEGER t, BTRFS_TIME* out) {
    ULONGLONG l = (ULONGLONG)t.QuadPart - 116444736000000000;

    out->seconds = l / 10000000;
    out->nanoseconds = (uint32_t)((l % 10000000) * 100);
}

_Post_satisfies_(*stripe>=0&&*stripe<num_stripes)
static __inline void get_raid0_offset(_In_ uint64_t off, _In_ uint64_t stripe_length, _In_ uint16_t num_stripes, _Out_ uint64_t* stripeoff, _Out_ uint16_t* stripe) {
    uint64_t initoff, startoff;

    startoff = off % (num_stripes * stripe_length);
    initoff = (off / (num_stripes * stripe_length)) * stripe_length;

    *stripe = (uint16_t)(startoff / stripe_length);
    *stripeoff = initoff + startoff - (*stripe * stripe_length);
}

/* We only have 64 bits for a file ID, which isn't technically enough to be
 * unique on Btrfs. We fudge it by having three bytes for the subvol and
 * five for the inode, which should be good enough.
 * Inodes are also 64 bits on Linux, but the Linux driver seems to get round
 * this by tricking it into thinking subvols are separate volumes. */
static __inline uint64_t make_file_id(root* r, uint64_t inode) {
    return (r->id << 40) | (inode & 0xffffffffff);
}

#define keycmp(key1, key2)\
    ((key1.obj_id < key2.obj_id) ? -1 :\
    ((key1.obj_id > key2.obj_id) ? 1 :\
    ((key1.obj_type < key2.obj_type) ? -1 :\
    ((key1.obj_type > key2.obj_type) ? 1 :\
    ((key1.offset < key2.offset) ? -1 :\
    ((key1.offset > key2.offset) ? 1 :\
    0))))))

_Post_satisfies_(return>=n)
__inline static uint64_t sector_align(_In_ uint64_t n, _In_ uint64_t a) {
    if (n & (a - 1))
        n = (n + a) & ~(a - 1);

    return n;
}

__inline static bool is_subvol_readonly(root* r, PIRP Irp) {
    if (!(r->root_item.flags & BTRFS_SUBVOL_READONLY))
        return false;

    if (!r->reserved)
        return true;

    return (!Irp || Irp->RequestorMode == UserMode) && PsGetCurrentProcess() != r->reserved ? true : false;
}

__inline static uint16_t get_extent_data_len(uint8_t type) {
    switch (type) {
        case TYPE_TREE_BLOCK_REF:
            return sizeof(TREE_BLOCK_REF);

        case TYPE_EXTENT_DATA_REF:
            return sizeof(EXTENT_DATA_REF);

        case TYPE_EXTENT_REF_V0:
            return sizeof(EXTENT_REF_V0);

        case TYPE_SHARED_BLOCK_REF:
            return sizeof(SHARED_BLOCK_REF);

        case TYPE_SHARED_DATA_REF:
            return sizeof(SHARED_DATA_REF);

        default:
            return 0;
    }
}

__inline static uint32_t get_extent_data_refcount(uint8_t type, void* data) {
    switch (type) {
        case TYPE_TREE_BLOCK_REF:
            return 1;

        case TYPE_EXTENT_DATA_REF:
        {
            EXTENT_DATA_REF* edr = (EXTENT_DATA_REF*)data;
            return edr->count;
        }

        case TYPE_EXTENT_REF_V0:
        {
            EXTENT_REF_V0* erv0 = (EXTENT_REF_V0*)data;
            return erv0->count;
        }

        case TYPE_SHARED_BLOCK_REF:
            return 1;

        case TYPE_SHARED_DATA_REF:
        {
            SHARED_DATA_REF* sdr = (SHARED_DATA_REF*)data;
            return sdr->count;
        }

        default:
            return 0;
    }
}

// in xor-gas.S
#if defined(_X86_) || defined(_AMD64_)
void __stdcall do_xor_sse2(uint8_t* buf1, uint8_t* buf2, uint32_t len);
void __stdcall do_xor_avx2(uint8_t* buf1, uint8_t* buf2, uint32_t len);
#endif

// in btrfs.c
_Ret_maybenull_
device* find_device_from_uuid(_In_ device_extension* Vcb, _In_ BTRFS_UUID* uuid);

_Success_(return)
bool get_file_attributes_from_xattr(_In_reads_bytes_(len) char* val, _In_ uint16_t len, _Out_ ULONG* atts);

ULONG get_file_attributes(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ uint64_t inode,
                          _In_ uint8_t type, _In_ bool dotfile, _In_ bool ignore_xa, _In_opt_ PIRP Irp);

_Success_(return)
bool get_xattr(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* subvol, _In_ uint64_t inode, _In_z_ char* name, _In_ uint32_t crc32,
               _Out_ uint8_t** data, _Out_ uint16_t* datalen, _In_opt_ PIRP Irp);

#ifndef DEBUG_FCB_REFCOUNTS
void free_fcb(_Inout_ fcb* fcb);
#endif
void free_fileref(_Inout_ file_ref* fr);
void protect_superblocks(_Inout_ chunk* c);
bool is_top_level(_In_ PIRP Irp);
NTSTATUS create_root(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ uint64_t id,
                     _Out_ root** rootptr, _In_ bool no_tree, _In_ uint64_t offset, _In_opt_ PIRP Irp);
void uninit(_In_ device_extension* Vcb);
NTSTATUS dev_ioctl(_In_ PDEVICE_OBJECT DeviceObject, _In_ ULONG ControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize,
                   _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _In_ bool Override, _Out_opt_ IO_STATUS_BLOCK* iosb);
NTSTATUS check_file_name_valid(_In_ PUNICODE_STRING us, _In_ bool posix, _In_ bool stream);
void send_notification_fileref(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream);
void queue_notification_fcb(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream);

typedef void (__stdcall *xor_func)(uint8_t* buf1, uint8_t* buf2, uint32_t len);

extern xor_func do_xor;

#ifdef DEBUG_CHUNK_LOCKS
#define acquire_chunk_lock(c, Vcb) { ExAcquireResourceExclusiveLite(&c->lock, true); InterlockedIncrement(&Vcb->chunk_locks_held); }
#define release_chunk_lock(c, Vcb) { InterlockedDecrement(&Vcb->chunk_locks_held); ExReleaseResourceLite(&c->lock); }
#else
#define acquire_chunk_lock(c, Vcb) ExAcquireResourceExclusiveLite(&(c)->lock, true)
#define release_chunk_lock(c, Vcb) ExReleaseResourceLite(&(c)->lock)
#endif

void mark_fcb_dirty(_In_ fcb* fcb);
void mark_fileref_dirty(_In_ file_ref* fileref);
NTSTATUS delete_fileref(_In_ file_ref* fileref, _In_opt_ PFILE_OBJECT FileObject, _In_ bool make_orphan, _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback);
void chunk_lock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ uint64_t start, _In_ uint64_t length);
void chunk_unlock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ uint64_t start, _In_ uint64_t length);
void init_device(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ bool get_nums);
void init_file_cache(_In_ PFILE_OBJECT FileObject, _In_ CC_FILE_SIZES* ccfs);
NTSTATUS sync_read_phys(_In_ PDEVICE_OBJECT DeviceObject, _In_ PFILE_OBJECT FileObject, _In_ uint64_t StartingOffset, _In_ ULONG Length,
                        _Out_writes_bytes_(Length) PUCHAR Buffer, _In_ bool override);
NTSTATUS get_device_pnp_name(_In_ PDEVICE_OBJECT DeviceObject, _Out_ PUNICODE_STRING pnp_name, _Out_ const GUID** guid);
void log_device_error(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ int error);
NTSTATUS find_chunk_usage(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp);

_Function_class_(DRIVER_ADD_DEVICE)
NTSTATUS __stdcall AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);

void reap_fcb(fcb* fcb);
void reap_fcbs(device_extension* Vcb);
void reap_fileref(device_extension* Vcb, file_ref* fr);
void reap_filerefs(device_extension* Vcb, file_ref* fr);
NTSTATUS utf8_to_utf16(WCHAR* dest, ULONG dest_max, ULONG* dest_len, char* src, ULONG src_len);
NTSTATUS utf16_to_utf8(char* dest, ULONG dest_max, ULONG* dest_len, WCHAR* src, ULONG src_len);
uint32_t get_num_of_processors();

_Ret_maybenull_
root* find_default_subvol(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp);

void do_shutdown(PIRP Irp);
bool check_superblock_checksum(superblock* sb);

#ifdef _MSC_VER
#define funcname __FUNCTION__
#else
#define funcname __func__
#endif

extern uint32_t mount_compress;
extern uint32_t mount_compress_force;
extern uint32_t mount_compress_type;
extern uint32_t mount_zlib_level;
extern uint32_t mount_zstd_level;
extern uint32_t mount_flush_interval;
extern uint32_t mount_max_inline;
extern uint32_t mount_skip_balance;
extern uint32_t mount_no_barrier;
extern uint32_t mount_no_trim;
extern uint32_t mount_clear_cache;
extern uint32_t mount_allow_degraded;
extern uint32_t mount_readonly;
extern uint32_t mount_no_root_dir;
extern uint32_t no_pnp;

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef _DEBUG

extern bool log_started;
extern uint32_t debug_log_level;

#ifdef DEBUG_LONG_MESSAGES

#define MSG(fn, file, line, s, level, ...) (!log_started || level <= debug_log_level) ? _debug_message(fn, file, line, s, ##__VA_ARGS__) : (void)0

#define TRACE(s, ...) MSG(funcname, __FILE__, __LINE__, s, 3, ##__VA_ARGS__)
#define WARN(s, ...) MSG(funcname, __FILE__, __LINE__, s, 2, ##__VA_ARGS__)
#define FIXME(s, ...) MSG(funcname, __FILE__, __LINE__, s, 1, ##__VA_ARGS__)
#define ERR(s, ...) MSG(funcname, __FILE__, __LINE__, s, 1, ##__VA_ARGS__)

void _debug_message(_In_ const char* func, _In_ const char* file, _In_ unsigned int line, _In_ char* s, ...) __attribute__((format(printf, 4, 5)));

#else

#define MSG(fn, s, level, ...) (!log_started || level <= debug_log_level) ? _debug_message(fn, s, ##__VA_ARGS__) : (void)0

#define TRACE(s, ...) MSG(funcname, s, 3, ##__VA_ARGS__)
#define WARN(s, ...) MSG(funcname, s, 2, ##__VA_ARGS__)
#define FIXME(s, ...) MSG(funcname, s, 1, ##__VA_ARGS__)
#define ERR(s, ...) MSG(funcname, s, 1, ##__VA_ARGS__)

void _debug_message(_In_ const char* func, _In_ char* s, ...) __attribute__((format(printf, 2, 3)));

#endif

#else

#define TRACE(s, ...) do { } while(0)
#define WARN(s, ...) do { } while(0)
#define FIXME(s, ...) DbgPrint("Btrfs FIXME : %s : " s, funcname, ##__VA_ARGS__)
#define ERR(s, ...) DbgPrint("Btrfs ERR : %s : " s, funcname, ##__VA_ARGS__)

#endif

#ifdef DEBUG_FCB_REFCOUNTS
void _free_fcb(_Inout_ fcb* fcb, _In_ const char* func);
#define free_fcb(fcb) _free_fcb(fcb, funcname)
#endif

// in fastio.c
void init_fast_io_dispatch(FAST_IO_DISPATCH** fiod);

// in sha256.c
void calc_sha256(uint8_t* hash, const void* input, size_t len);
#define SHA256_HASH_SIZE 32

// in blake2b-ref.c
void blake2b(void *out, size_t outlen, const void* in, size_t inlen);
#define BLAKE2_HASH_SIZE 32

typedef struct {
    LIST_ENTRY* list;
    LIST_ENTRY* list_size;
    uint64_t address;
    uint64_t length;
    chunk* chunk;
} rollback_space;

typedef struct {
    fcb* fcb;
    extent* ext;
} rollback_extent;

enum rollback_type {
    ROLLBACK_INSERT_EXTENT,
    ROLLBACK_DELETE_EXTENT,
    ROLLBACK_ADD_SPACE,
    ROLLBACK_SUBTRACT_SPACE
};

typedef struct {
    enum rollback_type type;
    void* ptr;
    LIST_ENTRY list_entry;
} rollback_item;

typedef struct {
    ANSI_STRING name;
    ANSI_STRING value;
    UCHAR flags;
    LIST_ENTRY list_entry;
} ea_item;

static const char lxuid[] = "$LXUID";
static const char lxgid[] = "$LXGID";
static const char lxmod[] = "$LXMOD";
static const char lxdev[] = "$LXDEV";

// in treefuncs.c
NTSTATUS find_item(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _Out_ traverse_ptr* tp,
                   _In_ const KEY* searchkey, _In_ bool ignore, _In_opt_ PIRP Irp) __attribute__((nonnull(1,2,3,4)));
NTSTATUS find_item_to_level(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, bool ignore,
                            uint8_t level, PIRP Irp) __attribute__((nonnull(1,2,3,4)));
bool find_next_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp,
                    traverse_ptr* next_tp, bool ignore, PIRP Irp) __attribute__((nonnull(1,2,3)));
bool find_prev_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp,
                    traverse_ptr* prev_tp, PIRP Irp) __attribute__((nonnull(1,2,3)));
void free_trees(device_extension* Vcb) __attribute__((nonnull(1)));
NTSTATUS insert_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ uint64_t obj_id,
                          _In_ uint8_t obj_type, _In_ uint64_t offset, _In_reads_bytes_opt_(size) _When_(return >= 0, __drv_aliasesMem) void* data,
                          _In_ uint16_t size, _Out_opt_ traverse_ptr* ptp, _In_opt_ PIRP Irp) __attribute__((nonnull(1,2)));
NTSTATUS delete_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb,
                          _Inout_ traverse_ptr* tp) __attribute__((nonnull(1,2)));
void free_tree(tree* t) __attribute__((nonnull(1)));
NTSTATUS load_tree(device_extension* Vcb, uint64_t addr, uint8_t* buf, root* r, tree** pt) __attribute__((nonnull(1,3,4,5)));
NTSTATUS do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, PIRP Irp) __attribute__((nonnull(1,2,3)));
void clear_rollback(LIST_ENTRY* rollback) __attribute__((nonnull(1)));
void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback) __attribute__((nonnull(1,2)));
void free_trees_root(device_extension* Vcb, root* r) __attribute__((nonnull(1,2)));
void add_rollback(_In_ LIST_ENTRY* rollback, _In_ enum rollback_type type, _In_ __drv_aliasesMem void* ptr) __attribute__((nonnull(1,3)));
NTSTATUS commit_batch_list(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb,
                           LIST_ENTRY* batchlist, PIRP Irp) __attribute__((nonnull(1,2)));
void clear_batch_list(device_extension* Vcb, LIST_ENTRY* batchlist) __attribute__((nonnull(1,2)));
NTSTATUS skip_to_difference(device_extension* Vcb, traverse_ptr* tp, traverse_ptr* tp2, bool* ended1, bool* ended2) __attribute__((nonnull(1,2,3,4,5)));

// in search.c
NTSTATUS remove_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath);

_Function_class_(KSTART_ROUTINE)
void __stdcall mountmgr_thread(_In_ void* context);

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
NTSTATUS __stdcall pnp_notification(PVOID NotificationStructure, PVOID Context);

void disk_arrival(PUNICODE_STRING devpath);
bool volume_arrival(PUNICODE_STRING devpath, bool fve_callback);
void volume_removal(PUNICODE_STRING devpath);

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
NTSTATUS __stdcall volume_notification(PVOID NotificationStructure, PVOID Context);

void remove_volume_child(_Inout_ _Requires_exclusive_lock_held_(_Curr_->child_lock) _Releases_exclusive_lock_(_Curr_->child_lock) _In_ volume_device_extension* vde,
                         _In_ volume_child* vc, _In_ bool skip_dev);
extern KSPIN_LOCK fve_data_lock;

// in cache.c
void init_cache();
extern CACHE_MANAGER_CALLBACKS cache_callbacks;

// in write.c
NTSTATUS write_file(device_extension* Vcb, PIRP Irp, bool wait, bool deferred_write) __attribute__((nonnull(1,2)));
NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, bool paging_io, bool no_cache,
                     bool wait, bool deferred_write, bool write_irp, LIST_ENTRY* rollback) __attribute__((nonnull(1,2,4,5,11)));
NTSTATUS truncate_file(fcb* fcb, uint64_t end, PIRP Irp, LIST_ENTRY* rollback) __attribute__((nonnull(1,4)));
NTSTATUS extend_file(fcb* fcb, file_ref* fileref, uint64_t end, bool prealloc, PIRP Irp, LIST_ENTRY* rollback) __attribute__((nonnull(1,6)));
NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, uint64_t start_data, uint64_t end_data, PIRP Irp, LIST_ENTRY* rollback) __attribute__((nonnull(1,2,6)));
chunk* get_chunk_from_address(device_extension* Vcb, uint64_t address) __attribute__((nonnull(1)));
NTSTATUS alloc_chunk(device_extension* Vcb, uint64_t flags, chunk** pc, bool full_size) __attribute__((nonnull(1,3)));
NTSTATUS write_data(_In_ device_extension* Vcb, _In_ uint64_t address, _In_reads_bytes_(length) void* data, _In_ uint32_t length, _In_ write_data_context* wtc,
                    _In_opt_ PIRP Irp, _In_opt_ chunk* c, _In_ bool file_write, _In_ uint64_t irp_offset, _In_ ULONG priority) __attribute__((nonnull(1,3,5)));
NTSTATUS write_data_complete(device_extension* Vcb, uint64_t address, void* data, uint32_t length, PIRP Irp, chunk* c, bool file_write,
                             uint64_t irp_offset, ULONG priority) __attribute__((nonnull(1,3)));
void free_write_data_stripes(write_data_context* wtc) __attribute__((nonnull(1)));

_Dispatch_type_(IRP_MJ_WRITE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) __attribute__((nonnull(1,2)));

_Requires_lock_held_(c->lock)
_When_(return != 0, _Releases_lock_(c->lock))
bool insert_extent_chunk(_In_ device_extension* Vcb, _In_ fcb* fcb, _In_ chunk* c, _In_ uint64_t start_data, _In_ uint64_t length, _In_ bool prealloc,
                         _In_opt_ void* data, _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback, _In_ uint8_t compression, _In_ uint64_t decoded_size,
                         _In_ bool file_write, _In_ uint64_t irp_offset) __attribute__((nonnull(1,2,3,9)));

NTSTATUS do_write_file(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, PIRP Irp, bool file_write, uint32_t irp_offset, LIST_ENTRY* rollback) __attribute__((nonnull(1, 4)));
bool find_data_address_in_chunk(device_extension* Vcb, chunk* c, uint64_t length, uint64_t* address) __attribute__((nonnull(1, 2, 4)));
void get_raid56_lock_range(chunk* c, uint64_t address, uint64_t length, uint64_t* lockaddr, uint64_t* locklen) __attribute__((nonnull(1,4,5)));
NTSTATUS add_extent_to_fcb(_In_ fcb* fcb, _In_ uint64_t offset, _In_reads_bytes_(edsize) EXTENT_DATA* ed, _In_ uint16_t edsize,
                           _In_ bool unique, _In_opt_ _When_(return >= 0, __drv_aliasesMem) void* csum, _In_ LIST_ENTRY* rollback) __attribute__((nonnull(1,3,7)));
void add_extent(_In_ fcb* fcb, _In_ LIST_ENTRY* prevextle, _In_ __drv_aliasesMem extent* newext) __attribute__((nonnull(1,2,3)));

// in dirctrl.c

_Dispatch_type_(IRP_MJ_DIRECTORY_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

ULONG get_reparse_tag(device_extension* Vcb, root* subvol, uint64_t inode, uint8_t type, ULONG atts, bool lxss, PIRP Irp);
ULONG get_reparse_tag_fcb(fcb* fcb);

// in security.c

_Dispatch_type_(IRP_MJ_QUERY_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_SET_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

void fcb_get_sd(fcb* fcb, struct _fcb* parent, bool look_for_xattr, PIRP Irp);
void add_user_mapping(WCHAR* sidstring, ULONG sidstringlength, uint32_t uid);
void add_group_mapping(WCHAR* sidstring, ULONG sidstringlength, uint32_t gid);
uint32_t sid_to_uid(PSID sid);
NTSTATUS uid_to_sid(uint32_t uid, PSID* sid);
NTSTATUS fcb_get_new_sd(fcb* fcb, file_ref* parfileref, ACCESS_STATE* as);
void find_gid(struct _fcb* fcb, struct _fcb* parfcb, PSECURITY_SUBJECT_CONTEXT subjcont);

// in fileinfo.c

_Dispatch_type_(IRP_MJ_SET_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_QUERY_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_QUERY_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_SET_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

bool has_open_children(file_ref* fileref);
NTSTATUS stream_set_end_of_file_information(device_extension* Vcb, uint16_t end, fcb* fcb, file_ref* fileref, bool advance_only);
NTSTATUS fileref_get_filename(file_ref* fileref, PUNICODE_STRING fn, USHORT* name_offset, ULONG* preqlen);
void insert_dir_child_into_hash_lists(fcb* fcb, dir_child* dc);
void remove_dir_child_from_hash_lists(fcb* fcb, dir_child* dc);
void add_fcb_to_subvol(_In_ _Requires_exclusive_lock_held_(_Curr_->Vcb->fcb_lock) fcb* fcb);
void remove_fcb_from_subvol(_In_ _Requires_exclusive_lock_held_(_Curr_->Vcb->fcb_lock) fcb* fcb);

// in reparse.c
NTSTATUS get_reparse_point(PFILE_OBJECT FileObject, void* buffer, DWORD buflen, ULONG_PTR* retlen);
NTSTATUS set_reparse_point2(fcb* fcb, REPARSE_DATA_BUFFER* rdb, ULONG buflen, ccb* ccb, file_ref* fileref, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS set_reparse_point(PIRP Irp);
NTSTATUS delete_reparse_point(PIRP Irp);

// in create.c

_Dispatch_type_(IRP_MJ_CREATE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS open_fileref(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb, _Out_ file_ref** pfr,
                      _In_ PUNICODE_STRING fnus, _In_opt_ file_ref* related, _In_ bool parent, _Out_opt_ USHORT* parsed, _Out_opt_ ULONG* fn_offset, _In_ POOL_TYPE pooltype,
                      _In_ bool case_sensitive, _In_opt_ PIRP Irp);
NTSTATUS open_fcb(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                  root* subvol, uint64_t inode, uint8_t type, PANSI_STRING utf8, bool always_add_hl, fcb* parent, fcb** pfcb, POOL_TYPE pooltype, PIRP Irp);
NTSTATUS load_csum(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, void* csum, uint64_t start, uint64_t length, PIRP Irp);
NTSTATUS load_dir_children(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, fcb* fcb, bool ignore_size, PIRP Irp);
NTSTATUS add_dir_child(fcb* fcb, uint64_t inode, bool subvol, PANSI_STRING utf8, PUNICODE_STRING name, uint8_t type, dir_child** pdc);
NTSTATUS open_fileref_child(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb,
                            _In_ file_ref* sf, _In_ PUNICODE_STRING name, _In_ bool case_sensitive, _In_ bool lastpart, _In_ bool streampart,
                            _In_ POOL_TYPE pooltype, _Out_ file_ref** psf2, _In_opt_ PIRP Irp);
fcb* create_fcb(device_extension* Vcb, POOL_TYPE pool_type);
NTSTATUS find_file_in_dir(PUNICODE_STRING filename, fcb* fcb, root** subvol, uint64_t* inode, dir_child** pdc, bool case_sensitive);
uint32_t inherit_mode(fcb* parfcb, bool is_dir);
file_ref* create_fileref(device_extension* Vcb);
NTSTATUS open_fileref_by_inode(_Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb, root* subvol, uint64_t inode, file_ref** pfr, PIRP Irp);

// in fsctl.c
NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP* Pirp, uint32_t type);
void do_unlock_volume(device_extension* Vcb);
void trim_whole_device(device* dev);
void flush_subvol_fcbs(root* subvol);
bool fcb_is_inline(fcb* fcb);
NTSTATUS dismount_volume(device_extension* Vcb, bool shutdown, PIRP Irp);

// in flushthread.c

_Function_class_(KSTART_ROUTINE)
void __stdcall flush_thread(void* context);

NTSTATUS do_write(device_extension* Vcb, PIRP Irp);
NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS flush_fcb(fcb* fcb, bool cache, LIST_ENTRY* batchlist, PIRP Irp);
NTSTATUS write_data_phys(_In_ PDEVICE_OBJECT device, _In_ PFILE_OBJECT fileobj, _In_ uint64_t address,
                         _In_reads_bytes_(length) void* data, _In_ uint32_t length);
bool is_tree_unique(device_extension* Vcb, tree* t, PIRP Irp);
NTSTATUS do_tree_writes(device_extension* Vcb, LIST_ENTRY* tree_writes, bool no_free);
void add_checksum_entry(device_extension* Vcb, uint64_t address, ULONG length, void* csum, PIRP Irp);
bool find_metadata_address_in_chunk(device_extension* Vcb, chunk* c, uint64_t* address);
void add_trim_entry_avoid_sb(device_extension* Vcb, device* dev, uint64_t address, uint64_t size);
NTSTATUS insert_tree_item_batch(LIST_ENTRY* batchlist, device_extension* Vcb, root* r, uint64_t objid, uint8_t objtype, uint64_t offset,
                                _In_opt_ _When_(return >= 0, __drv_aliasesMem) void* data, uint16_t datalen, enum batch_operation operation);
NTSTATUS flush_partial_stripe(device_extension* Vcb, chunk* c, partial_stripe* ps);
NTSTATUS update_dev_item(device_extension* Vcb, device* device, PIRP Irp);
void calc_tree_checksum(device_extension* Vcb, tree_header* th);

// in read.c

_Dispatch_type_(IRP_MJ_READ)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS read_data(_In_ device_extension* Vcb, _In_ uint64_t addr, _In_ uint32_t length, _In_reads_bytes_opt_(length*sizeof(uint32_t)/Vcb->superblock.sector_size) void* csum,
                   _In_ bool is_tree, _Out_writes_bytes_(length) uint8_t* buf, _In_opt_ chunk* c, _Out_opt_ chunk** pc, _In_opt_ PIRP Irp, _In_ uint64_t generation, _In_ bool file_read,
                   _In_ ULONG priority);
NTSTATUS read_file(fcb* fcb, uint8_t* data, uint64_t start, uint64_t length, ULONG* pbr, PIRP Irp) __attribute__((nonnull(1, 2)));
NTSTATUS read_stream(fcb* fcb, uint8_t* data, uint64_t start, ULONG length, ULONG* pbr) __attribute__((nonnull(1, 2)));
NTSTATUS do_read(PIRP Irp, bool wait, ULONG* bytes_read);
NTSTATUS check_csum(device_extension* Vcb, uint8_t* data, uint32_t sectors, void* csum);
void raid6_recover2(uint8_t* sectors, uint16_t num_stripes, ULONG sector_size, uint16_t missing1, uint16_t missing2, uint8_t* out);
void get_tree_checksum(device_extension* Vcb, tree_header* th, void* csum);
bool check_tree_checksum(device_extension* Vcb, tree_header* th);
void get_sector_csum(device_extension* Vcb, void* buf, void* csum);
bool check_sector_csum(device_extension* Vcb, void* buf, void* csum);

// in pnp.c

_Dispatch_type_(IRP_MJ_PNP)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_pnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS pnp_surprise_removal(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS pnp_query_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// in free-space.c
NTSTATUS load_cache_chunk(device_extension* Vcb, chunk* c, PIRP Irp);
NTSTATUS clear_free_space_cache(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp);
NTSTATUS allocate_cache(device_extension* Vcb, bool* changed, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS update_chunk_caches(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS update_chunk_caches_tree(device_extension* Vcb, PIRP Irp);
NTSTATUS add_space_entry(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t offset, uint64_t size);
void space_list_add(chunk* c, uint64_t address, uint64_t length, LIST_ENTRY* rollback);
void space_list_add2(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t address, uint64_t length, chunk* c, LIST_ENTRY* rollback);
void space_list_subtract(chunk* c, uint64_t address, uint64_t length, LIST_ENTRY* rollback);
void space_list_subtract2(LIST_ENTRY* list, LIST_ENTRY* list_size, uint64_t address, uint64_t length, chunk* c, LIST_ENTRY* rollback);
void space_list_merge(LIST_ENTRY* spacelist, LIST_ENTRY* spacelist_size, LIST_ENTRY* deleting);
NTSTATUS load_stored_free_space_cache(device_extension* Vcb, chunk* c, bool load_only, PIRP Irp);

// in extent-tree.c
NTSTATUS increase_extent_refcount_data(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint64_t inode, uint64_t offset, uint32_t refcount, PIRP Irp);
NTSTATUS decrease_extent_refcount_data(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint64_t inode, uint64_t offset,
                                       uint32_t refcount, bool superseded, PIRP Irp);
NTSTATUS decrease_extent_refcount_tree(device_extension* Vcb, uint64_t address, uint64_t size, uint64_t root, uint8_t level, PIRP Irp);
uint64_t get_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, PIRP Irp);
bool is_extent_unique(device_extension* Vcb, uint64_t address, uint64_t size, PIRP Irp);
NTSTATUS increase_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, uint8_t type, void* data, KEY* firstitem, uint8_t level, PIRP Irp);
uint64_t get_extent_flags(device_extension* Vcb, uint64_t address, PIRP Irp);
void update_extent_flags(device_extension* Vcb, uint64_t address, uint64_t flags, PIRP Irp);
NTSTATUS update_changed_extent_ref(device_extension* Vcb, chunk* c, uint64_t address, uint64_t size, uint64_t root, uint64_t objid, uint64_t offset,
                                   int32_t count, bool no_csum, bool superseded, PIRP Irp);
void add_changed_extent_ref(chunk* c, uint64_t address, uint64_t size, uint64_t root, uint64_t objid, uint64_t offset, uint32_t count, bool no_csum);
uint64_t find_extent_shared_tree_refcount(device_extension* Vcb, uint64_t address, uint64_t parent, PIRP Irp);
uint32_t find_extent_shared_data_refcount(device_extension* Vcb, uint64_t address, uint64_t parent, PIRP Irp);
NTSTATUS decrease_extent_refcount(device_extension* Vcb, uint64_t address, uint64_t size, uint8_t type, void* data, KEY* firstitem,
                                  uint8_t level, uint64_t parent, bool superseded, PIRP Irp);
uint64_t get_extent_data_ref_hash2(uint64_t root, uint64_t objid, uint64_t offset);

// in worker-thread.c
NTSTATUS do_read_job(PIRP Irp);
NTSTATUS do_write_job(device_extension* Vcb, PIRP Irp);
bool add_thread_job(device_extension* Vcb, PIRP Irp);

// in registry.c
void read_registry(PUNICODE_STRING regpath, bool refresh);
NTSTATUS registry_mark_volume_mounted(BTRFS_UUID* uuid);
NTSTATUS registry_mark_volume_unmounted(BTRFS_UUID* uuid);
NTSTATUS registry_load_volume_options(device_extension* Vcb);
void watch_registry(HANDLE regh);

// in compress.c
NTSTATUS zlib_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen);
NTSTATUS lzo_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, uint32_t inpageoff);
NTSTATUS zstd_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen);
NTSTATUS write_compressed(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS zlib_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, unsigned int level, unsigned int* space_left);
NTSTATUS lzo_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, unsigned int* space_left);
NTSTATUS zstd_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, uint32_t level, unsigned int* space_left);

// in galois.c
void galois_double(uint8_t* data, uint32_t len);
void galois_divpower(uint8_t* data, uint8_t div, uint32_t readlen);
uint8_t gpow2(uint8_t e);
uint8_t gmul(uint8_t a, uint8_t b);
uint8_t gdiv(uint8_t a, uint8_t b);

// in devctrl.c

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// in calcthread.c

_Function_class_(KSTART_ROUTINE)
void __stdcall calc_thread(void* context);

void do_calc_job(device_extension* Vcb, uint8_t* data, uint32_t sectors, void* csum);
NTSTATUS add_calc_job_decomp(device_extension* Vcb, uint8_t compression, void* in, unsigned int inlen,
                             void* out, unsigned int outlen, unsigned int off, calc_job** pcj);
NTSTATUS add_calc_job_comp(device_extension* Vcb, uint8_t compression, void* in, unsigned int inlen,
                           void* out, unsigned int outlen, calc_job** pcj);
void calc_thread_main(device_extension* Vcb, calc_job* cj);

// in balance.c
NTSTATUS start_balance(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode);
NTSTATUS query_balance(device_extension* Vcb, void* data, ULONG length);
NTSTATUS pause_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS resume_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS stop_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS look_for_balance_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb);
NTSTATUS remove_device(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode);

_Function_class_(KSTART_ROUTINE)
void __stdcall balance_thread(void* context);

// in volume.c
NTSTATUS vol_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
void add_volume_device(superblock* sb, PUNICODE_STRING devpath, uint64_t length, ULONG disk_num, ULONG part_num);
NTSTATUS mountmgr_add_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath);

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
NTSTATUS __stdcall pnp_removal(PVOID NotificationStructure, PVOID Context);

void free_vol(volume_device_extension* vde);

// in scrub.c
NTSTATUS start_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS query_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode, void* data, ULONG length);
NTSTATUS pause_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS resume_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS stop_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);

// in send.c
NTSTATUS send_subvol(device_extension* Vcb, void* data, ULONG datalen, PFILE_OBJECT FileObject, PIRP Irp);
NTSTATUS read_send_buffer(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, ULONG_PTR* retlen, KPROCESSOR_MODE processor_mode);

// in fsrtl.c
NTSTATUS __stdcall compat_FsRtlValidateReparsePointBuffer(IN ULONG BufferLength, IN PREPARSE_DATA_BUFFER ReparseBuffer);

// in boot.c
void check_system_root();
void boot_add_device(DEVICE_OBJECT* pdo);
extern BTRFS_UUID boot_uuid;

// based on function in sys/sysmacros.h
#define makedev(major, minor) (((minor) & 0xFF) | (((major) & 0xFFF) << 8) | (((uint64_t)((minor) & ~0xFF)) << 12) | (((uint64_t)((major) & ~0xFFF)) << 32))

#ifndef __REACTOS__
// not in mingw yet
#ifndef _MSC_VER
typedef struct {
    FSRTL_COMMON_FCB_HEADER Header;
    PFAST_MUTEX FastMutex;
    LIST_ENTRY FilterContexts;
    EX_PUSH_LOCK PushLock;
    PVOID* FileContextSupportPointer;
    union {
        OPLOCK Oplock;
        PVOID ReservedForRemote;
    };
    PVOID ReservedContext;
} FSRTL_ADVANCED_FCB_HEADER_NEW;

#define FSRTL_FCB_HEADER_V2 2

#else
#define FSRTL_ADVANCED_FCB_HEADER_NEW FSRTL_ADVANCED_FCB_HEADER
#endif
#else
typedef struct {
    FSRTL_COMMON_FCB_HEADER Header;
    PFAST_MUTEX FastMutex;
    LIST_ENTRY FilterContexts;
    EX_PUSH_LOCK PushLock;
    PVOID* FileContextSupportPointer;
    union {
        OPLOCK Oplock;
        PVOID ReservedForRemote;
    };
    PVOID ReservedContext;
} FSRTL_ADVANCED_FCB_HEADER_NEW;

#define FSRTL_FCB_HEADER_V2 2
#endif // __REACTOS__

static __inline POPLOCK fcb_oplock(fcb* fcb) {
    if (fcb->Header.Version >= FSRTL_FCB_HEADER_V2)
        return &((FSRTL_ADVANCED_FCB_HEADER_NEW*)&fcb->Header)->Oplock;
    else
        return &fcb->oplock;
}

static __inline FAST_IO_POSSIBLE fast_io_possible(fcb* fcb) {
    if (!FsRtlOplockIsFastIoPossible(fcb_oplock(fcb)))
        return FastIoIsNotPossible;

    if (!FsRtlAreThereCurrentFileLocks(&fcb->lock) && !fcb->Vcb->readonly)
        return FastIoIsPossible;

    return FastIoIsQuestionable;
}

static __inline void print_open_trees(device_extension* Vcb) {
    LIST_ENTRY* le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        tree_data* td = CONTAINING_RECORD(t->itemlist.Flink, tree_data, list_entry);
        ERR("tree %p: root %I64x, level %u, first key (%I64x,%x,%I64x)\n",
                      t, t->root->id, t->header.level, td->key.obj_id, td->key.obj_type, td->key.offset);

        le = le->Flink;
    }
}

static __inline bool write_fcb_compressed(fcb* fcb) {
    // make sure we don't accidentally write the cache inodes or pagefile compressed
    if (fcb->subvol->id == BTRFS_ROOT_ROOT || fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE)
        return false;

    if (fcb->Vcb->options.compress_force)
        return true;

    if (fcb->inode_item.flags & BTRFS_INODE_NOCOMPRESS)
        return false;

    if (fcb->inode_item.flags & BTRFS_INODE_COMPRESS || fcb->Vcb->options.compress)
        return true;

    return false;
}

#ifdef DEBUG_FCB_REFCOUNTS
#ifdef DEBUG_LONG_MESSAGES
#define increase_fileref_refcount(fileref) {\
    LONG rc = InterlockedIncrement(&fileref->refcount);\
    MSG(funcname, __FILE__, __LINE__, "fileref %p: refcount now %i\n", 1, fileref, rc);\
}
#else
#define increase_fileref_refcount(fileref) {\
    LONG rc = InterlockedIncrement(&fileref->refcount);\
    MSG(funcname, "fileref %p: refcount now %i\n", 1, fileref, rc);\
}
#endif
#else
#define increase_fileref_refcount(fileref) InterlockedIncrement(&fileref->refcount)
#endif

#ifdef _MSC_VER
#define int3 __debugbreak()
#else
#define int3 asm("int3;")
#endif

#define hex_digit(c) ((c) <= 9) ? ((c) + '0') : ((c) - 10 + 'a')

// FIXME - find a way to catch unfreed trees again

// from sys/stat.h
#define __S_IFMT        0170000 /* These bits determine file type.  */
#define __S_IFDIR       0040000 /* Directory.  */
#define __S_IFCHR       0020000 /* Character device.  */
#define __S_IFBLK       0060000 /* Block device.  */
#define __S_IFREG       0100000 /* Regular file.  */
#define __S_IFIFO       0010000 /* FIFO.  */
#define __S_IFLNK       0120000 /* Symbolic link.  */
#define __S_IFSOCK      0140000 /* Socket.  */
#define __S_ISTYPE(mode, mask)  (((mode) & __S_IFMT) == (mask))

#ifndef S_ISDIR
#define S_ISDIR(mode)    __S_ISTYPE((mode), __S_IFDIR)
#endif

#ifndef S_IRUSR
#define S_IRUSR 0000400
#endif

#ifndef S_IWUSR
#define S_IWUSR 0000200
#endif

#ifndef S_IXUSR
#define S_IXUSR 0000100
#endif

#ifdef __REACTOS__
#define S_IFDIR __S_IFDIR
#define S_IFREG __S_IFREG
#endif /* __REACTOS__ */

#ifndef S_IRGRP
#define S_IRGRP (S_IRUSR >> 3)
#endif

#ifndef S_IWGRP
#define S_IWGRP (S_IWUSR >> 3)
#endif

#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3)
#endif

#ifndef S_IROTH
#define S_IROTH (S_IRGRP >> 3)
#endif

#ifndef S_IWOTH
#define S_IWOTH (S_IWGRP >> 3)
#endif

#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3)
#endif

#ifndef S_ISUID
#define S_ISUID 0004000
#endif

#ifndef S_ISGID
#define S_ISGID 0002000
#endif

#ifndef S_ISVTX
#define S_ISVTX 0001000
#endif

// based on functions in sys/sysmacros.h
#define major(rdev) ((((rdev) >> 8) & 0xFFF) | ((uint32_t)((rdev) >> 32) & ~0xFFF))
#define minor(rdev) (((rdev) & 0xFF) | ((uint32_t)((rdev) >> 12) & ~0xFF))

static __inline uint64_t fcb_alloc_size(fcb* fcb) {
    if (S_ISDIR(fcb->inode_item.st_mode))
        return 0;
    else if (fcb->atts & FILE_ATTRIBUTE_SPARSE_FILE)
        return fcb->inode_item.st_blocks;
    else
        return sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
}

typedef BOOLEAN (__stdcall *tPsIsDiskCountersEnabled)();

typedef VOID (__stdcall *tPsUpdateDiskCounters)(PEPROCESS Process, ULONG64 BytesRead, ULONG64 BytesWritten,
                                                ULONG ReadOperationCount, ULONG WriteOperationCount, ULONG FlushOperationCount);

typedef BOOLEAN (__stdcall *tCcCopyWriteEx)(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait,
                                            PVOID Buffer, PETHREAD IoIssuerThread);

typedef BOOLEAN (__stdcall *tCcCopyReadEx)(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait,
                                           PVOID Buffer, PIO_STATUS_BLOCK IoStatus, PETHREAD IoIssuerThread);

#ifndef CC_ENABLE_DISK_IO_ACCOUNTING
#define CC_ENABLE_DISK_IO_ACCOUNTING 0x00000010
#endif

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_VISTA)
typedef struct _ECP_LIST ECP_LIST;
typedef struct _ECP_LIST *PECP_LIST;
#endif

typedef VOID (__stdcall *tCcSetAdditionalCacheAttributesEx)(PFILE_OBJECT FileObject, ULONG Flags);

typedef VOID (__stdcall *tFsRtlUpdateDiskCounters)(ULONG64 BytesRead, ULONG64 BytesWritten);

typedef NTSTATUS (__stdcall *tIoUnregisterPlugPlayNotificationEx)(PVOID NotificationEntry);

typedef NTSTATUS (__stdcall *tFsRtlGetEcpListFromIrp)(PIRP Irp, PECP_LIST* EcpList);

typedef NTSTATUS (__stdcall *tFsRtlGetNextExtraCreateParameter)(PECP_LIST EcpList, PVOID CurrentEcpContext, LPGUID NextEcpType,
                                                                PVOID* NextEcpContext, ULONG* NextEcpContextSize);

typedef NTSTATUS (__stdcall *tFsRtlValidateReparsePointBuffer)(ULONG BufferLength, PREPARSE_DATA_BUFFER ReparseBuffer);

typedef BOOLEAN (__stdcall *tFsRtlCheckLockForOplockRequest)(PFILE_LOCK FileLock, PLARGE_INTEGER AllocationSize);

typedef BOOLEAN (__stdcall *tFsRtlAreThereCurrentOrInProgressFileLocks)(PFILE_LOCK FileLock);

#ifndef __REACTOS__
#ifndef _MSC_VER
PEPROCESS __stdcall PsGetThreadProcess(_In_ PETHREAD Thread); // not in mingw
#endif

// not in DDK headers - taken from winternl.h
typedef struct _LDR_DATA_TABLE_ENTRY {
    PVOID Reserved1[2];
    LIST_ENTRY InMemoryOrderLinks;
    PVOID Reserved2[2];
    PVOID DllBase;
    PVOID Reserved3[2];
    UNICODE_STRING FullDllName;
    BYTE Reserved4[8];
    PVOID Reserved5[3];
    union {
        ULONG CheckSum;
        PVOID Reserved6;
    };
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY,*PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA,*PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS,*PRTL_USER_PROCESS_PARAMETERS;

typedef VOID (NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    BYTE Reserved4[104];
    PVOID Reserved5[52];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
} PEB,*PPEB;
#endif /* __REACTOS__ */

#ifdef _MSC_VER
__kernel_entry
NTSTATUS NTAPI ZwQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);
#endif
