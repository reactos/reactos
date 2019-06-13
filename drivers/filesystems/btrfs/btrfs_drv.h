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

#ifndef BTRFS_DRV_H_DEFINED
#define BTRFS_DRV_H_DEFINED

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
#ifndef __REACTOS__
// Not actually used
#include <emmintrin.h>
#endif /* __REACTOS__ */
#include "btrfs.h"
#include "btrfsioctl.h"

#ifdef _DEBUG
// #define DEBUG_FCB_REFCOUNTS
// #define DEBUG_LONG_MESSAGES
// #define DEBUG_FLUSH_TIMES
// #define DEBUG_STATS
// #define DEBUG_CHUNK_LOCKS
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

#define IO_REPARSE_TAG_LXSS_SYMLINK 0xa000001d // undocumented?

#define IO_REPARSE_TAG_LXSS_SOCKET      0x80000023
#define IO_REPARSE_TAG_LXSS_FIFO        0x80000024
#define IO_REPARSE_TAG_LXSS_CHARDEV     0x80000025
#define IO_REPARSE_TAG_LXSS_BLOCKDEV    0x80000026

#define BTRFS_VOLUME_PREFIX L"\\Device\\Btrfs{"

#ifdef _MSC_VER
#define try __try
#define except __except
#define finally __finally
#else
#define try if (1)
#define except(x) if (0 && (x))
#define finally if (1)
#endif

#ifndef FILE_SUPPORTS_BLOCK_REFCOUNTING
#define FILE_SUPPORTS_BLOCK_REFCOUNTING 0x08000000
#endif

#ifndef FILE_SUPPORTS_POSIX_UNLINK_RENAME
#define FILE_SUPPORTS_POSIX_UNLINK_RENAME 0x00000400
#endif

#ifndef FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL 0x00020000
#endif

typedef struct _FILE_ID_128 {
    UCHAR Identifier[16];
} FILE_ID_128, *PFILE_ID_128;

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
#define _Requires_lock_held_(a)
#define _Requires_exclusive_lock_held_(a)
#define _Releases_lock_(a)
#define _Out_writes_bytes_opt_(a)
#define _Pre_satisfies_(a)
#define _Post_satisfies_(a)
#define _Releases_exclusive_lock_(a)
#define _Dispatch_type_(a)
#define _Create_lock_level_(a)
#define _Lock_level_order_(a,b)
#define _Has_lock_level_(a)
#define _Requires_lock_not_held_(a)
#define _Acquires_exclusive_lock_(a)
#define _Acquires_shared_lock_(a)
#endif
#endif

_Create_lock_level_(tree_lock)
_Create_lock_level_(fcb_lock)
_Lock_level_order_(tree_lock, fcb_lock)

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
    UINT64 offset;
    UINT16 datalen;
    BOOL unique;
    BOOL ignore;
    BOOL inserted;
    UINT32* csum;

    LIST_ENTRY list_entry;

    EXTENT_DATA extent_data;
} extent;

typedef struct {
    UINT64 parent;
    UINT64 index;
    UNICODE_STRING name;
    ANSI_STRING utf8;
    LIST_ENTRY list_entry;
} hardlink;

struct _file_ref;

typedef struct {
    KEY key;
    UINT64 index;
    UINT8 type;
    ANSI_STRING utf8;
    UINT32 hash;
    UNICODE_STRING name;
    UINT32 hash_uc;
    UNICODE_STRING name_uc;
    ULONG size;
    struct _file_ref* fileref;
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
    BOOL dirty;
    char data[1];
} xattr;

typedef struct _fcb {
    FSRTL_ADVANCED_FCB_HEADER Header;
    struct _fcb_nonpaged* nonpaged;
    LONG refcount;
    POOL_TYPE pool_type;
    struct _device_extension* Vcb;
    struct _root* subvol;
    UINT64 inode;
    UINT32 hash;
    UINT8 type;
    INODE_ITEM inode_item;
    SECURITY_DESCRIPTOR* sd;
    FILE_LOCK lock;
    BOOL deleted;
    PKTHREAD lazy_writer_thread;
    ULONG atts;
    SHARE_ACCESS share_access;
    WCHAR* debug_desc;
    BOOL csum_loaded;
    LIST_ENTRY extents;
    ANSI_STRING reparse_xattr;
    ANSI_STRING ea_xattr;
    ULONG ealen;
    LIST_ENTRY hardlinks;
    struct _file_ref* fileref;
    BOOL inode_item_changed;
    enum prop_compression_type prop_compression;
    LIST_ENTRY xattrs;
    BOOL marked_as_orphan;
    BOOL case_sensitive;
    BOOL case_sensitive_set;

    LIST_ENTRY dir_children_index;
    LIST_ENTRY dir_children_hash;
    LIST_ENTRY dir_children_hash_uc;
    LIST_ENTRY** hash_ptrs;
    LIST_ENTRY** hash_ptrs_uc;

    BOOL dirty;
    BOOL sd_dirty, sd_deleted;
    BOOL atts_changed, atts_deleted;
    BOOL extents_changed;
    BOOL reparse_xattr_changed;
    BOOL ea_changed;
    BOOL prop_compression_changed;
    BOOL xattrs_changed;
    BOOL created;

    BOOL ads;
    UINT32 adshash;
    ULONG adsmaxlen;
    ANSI_STRING adsxattr;
    ANSI_STRING adsdata;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_all;
    LIST_ENTRY list_entry_dirty;
} fcb;

typedef struct {
    ERESOURCE fileref_lock;
} file_ref_nonpaged;

typedef struct _file_ref {
    fcb* fcb;
    ANSI_STRING oldutf8;
    UINT64 oldindex;
    BOOL delete_on_close;
    BOOL posix_delete;
    BOOL deleted;
    BOOL created;
    file_ref_nonpaged* nonpaged;
    LIST_ENTRY children;
    LONG refcount;
    LONG open_count;
    struct _file_ref* parent;
    WCHAR* debug_desc;
    dir_child* dc;

    BOOL dirty;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_dirty;
} file_ref;

typedef struct {
    HANDLE thread;
    struct _ccb* ccb;
    void* context;
    KEVENT cleared_event;
    BOOL cancelling;
    LIST_ENTRY list_entry;
} send_info;

typedef struct _ccb {
    USHORT NodeType;
    CSHORT NodeSize;
    ULONG disposition;
    ULONG options;
    UINT64 query_dir_offset;
    UNICODE_STRING query_string;
    BOOL has_wildcard;
    BOOL specific_file;
    BOOL manage_volume_privilege;
    BOOL allow_extended_dasd_io;
    BOOL reserving;
    ACCESS_MASK access;
    file_ref* fileref;
    UNICODE_STRING filename;
    ULONG ea_index;
    BOOL case_sensitive;
    BOOL user_set_creation_time;
    BOOL user_set_access_time;
    BOOL user_set_write_time;
    BOOL user_set_change_time;
    BOOL lxss;
    send_info* send;
    NTSTATUS send_status;
} ccb;

struct _device_extension;

typedef struct {
    UINT64 address;
    UINT64 generation;
    struct _tree* tree;
} tree_holder;

typedef struct _tree_data {
    KEY key;
    LIST_ENTRY list_entry;
    BOOL ignore;
    BOOL inserted;

    union {
        tree_holder treeholder;

        struct {
            UINT16 size;
            UINT8* data;
        };
    };
} tree_data;

typedef struct {
    FAST_MUTEX mutex;
} tree_nonpaged;

typedef struct _tree {
    tree_nonpaged* nonpaged;
    tree_header header;
    UINT32 hash;
    BOOL has_address;
    UINT32 size;
    struct _device_extension* Vcb;
    struct _tree* parent;
    tree_data* paritem;
    struct _root* root;
    LIST_ENTRY itemlist;
    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_hash;
    UINT64 new_address;
    BOOL has_new_address;
    BOOL updated_extents;
    BOOL write;
    BOOL is_unique;
    BOOL uniqueness_determined;
    UINT8* buf;
} tree;

typedef struct {
    ERESOURCE load_tree_lock;
} root_nonpaged;

typedef struct _root {
    UINT64 id;
    LONGLONG lastinode; // signed so we can use InterlockedIncrement64
    tree_holder treeholder;
    root_nonpaged* nonpaged;
    ROOT_ITEM root_item;
    BOOL dirty;
    BOOL received;
    PEPROCESS reserved;
    UINT64 parent;
    LONG send_ops;
    UINT64 fcbs_version;
    BOOL checked_for_orphans;
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
    UINT16 datalen;
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
    UINT64 address;
    UINT64 size;
    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_size;
} space;

typedef struct {
    PDEVICE_OBJECT devobj;
    DEV_ITEM devitem;
    BOOL removable;
    BOOL seeding;
    BOOL readonly;
    BOOL reloc;
    BOOL trim;
    BOOL can_flush;
    ULONG change_count;
    ULONG disk_num;
    ULONG part_num;
    UINT64 stats[5];
    BOOL stats_changed;
    LIST_ENTRY space;
    LIST_ENTRY list_entry;
    ULONG num_trim_entries;
    LIST_ENTRY trim_list;
} device;

typedef struct {
    UINT64 start;
    UINT64 length;
    PETHREAD thread;
    LIST_ENTRY list_entry;
} range_lock;

typedef struct {
    UINT64 address;
    ULONG* bmparr;
    RTL_BITMAP bmp;
    LIST_ENTRY list_entry;
    UINT8 data[1];
} partial_stripe;

typedef struct {
    CHUNK_ITEM* chunk_item;
    UINT16 size;
    UINT64 offset;
    UINT64 used;
    UINT64 oldused;
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
    BOOL created;
    BOOL readonly;
    BOOL reloc;
    BOOL last_alloc_set;
    BOOL cache_loaded;
    BOOL changed;
    BOOL space_changed;
    UINT64 last_alloc;
    UINT16 last_stripe;
    LIST_ENTRY partial_stripes;
    ERESOURCE partial_stripes_lock;
    ULONG balance_num;

    LIST_ENTRY list_entry;
    LIST_ENTRY list_entry_balance;
} chunk;

typedef struct {
    UINT64 address;
    UINT64 size;
    UINT64 old_size;
    UINT64 count;
    UINT64 old_count;
    BOOL no_csum;
    BOOL superseded;
    LIST_ENTRY refs;
    LIST_ENTRY old_refs;
    LIST_ENTRY list_entry;
} changed_extent;

typedef struct {
    UINT8 type;

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

typedef struct {
    UINT8* data;
    UINT32* csum;
    UINT32 sectors;
    LONG pos, done;
    KEVENT event;
    LONG refcount;
    LIST_ENTRY list_entry;
} calc_job;

typedef struct {
    PDEVICE_OBJECT DeviceObject;
    HANDLE handle;
    KEVENT finished;
    BOOL quit;
} drv_calc_thread;

typedef struct {
    ULONG num_threads;
    LIST_ENTRY job_list;
    ERESOURCE lock;
    drv_calc_thread* threads;
    KEVENT event;
} drv_calc_threads;

typedef struct {
    BOOL ignore;
    BOOL compress;
    BOOL compress_force;
    UINT8 compress_type;
    BOOL readonly;
    UINT32 zlib_level;
    UINT32 zstd_level;
    UINT32 flush_interval;
    UINT32 max_inline;
    UINT64 subvol_id;
    BOOL skip_balance;
    BOOL no_barrier;
    BOOL no_trim;
    BOOL clear_cache;
    BOOL allow_degraded;
} mount_options;

#define VCB_TYPE_FS         1
#define VCB_TYPE_CONTROL    2
#define VCB_TYPE_VOLUME     3
#define VCB_TYPE_PDO        4

#ifdef DEBUG_STATS
typedef struct {
    UINT64 num_reads;
    UINT64 data_read;
    UINT64 read_total_time;
    UINT64 read_csum_time;
    UINT64 read_disk_time;

    UINT64 num_opens;
    UINT64 open_total_time;
    UINT64 num_overwrites;
    UINT64 overwrite_total_time;
    UINT64 num_creates;
    UINT64 create_total_time;
    UINT64 open_fcb_calls;
    UINT64 open_fcb_time;
    UINT64 open_fileref_child_calls;
    UINT64 open_fileref_child_time;
    UINT64 fcb_lock_time;
} debug_stats;
#endif

#define BALANCE_OPTS_DATA       0
#define BALANCE_OPTS_METADATA   1
#define BALANCE_OPTS_SYSTEM     2

typedef struct {
    HANDLE thread;
    UINT64 total_chunks;
    UINT64 chunks_left;
    btrfs_balance_opts opts[3];
    BOOL paused;
    BOOL stopping;
    BOOL removing;
    BOOL shrinking;
    BOOL dev_readonly;
    ULONG balance_num;
    NTSTATUS status;
    KEVENT event;
    KEVENT finished;
} balance_info;

typedef struct {
    UINT64 address;
    UINT64 device;
    BOOL recovered;
    BOOL is_metadata;
    BOOL parity;
    LIST_ENTRY list_entry;

    union {
        struct {
            UINT64 subvol;
            UINT64 offset;
            UINT16 filename_length;
            WCHAR filename[1];
        } data;

        struct {
            UINT64 root;
            UINT8 level;
            KEY firstitem;
        } metadata;
    };
} scrub_error;

typedef struct {
    HANDLE thread;
    ERESOURCE stats_lock;
    KEVENT event;
    KEVENT finished;
    BOOL stopping;
    BOOL paused;
    LARGE_INTEGER start_time;
    LARGE_INTEGER finish_time;
    LARGE_INTEGER resume_time;
    LARGE_INTEGER duration;
    UINT64 total_chunks;
    UINT64 chunks_left;
    UINT64 data_scrubbed;
    NTSTATUS error;
    ULONG num_errors;
    LIST_ENTRY errors;
} scrub_info;

struct _volume_device_extension;

typedef struct _device_extension {
    UINT32 type;
    mount_options options;
    PVPB Vpb;
    struct _volume_device_extension* vde;
    LIST_ENTRY devices;
#ifdef DEBUG_STATS
    debug_stats stats;
#endif
#ifdef DEBUG_CHUNK_LOCKS
    LONG chunk_locks_held;
#endif
    UINT64 devices_loaded;
    superblock superblock;
    BOOL readonly;
    BOOL removing;
    BOOL locked;
    BOOL lock_paused_balance;
    BOOL disallow_dismount;
    BOOL trim;
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
    BOOL need_write;
    BOOL stats_changed;
    UINT64 data_flags;
    UINT64 metadata_flags;
    UINT64 system_flags;
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
    BOOL log_to_phys_loaded;
    BOOL chunk_usage_found;
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
    NPAGED_LOOKASIDE_LIST fileref_np_lookaside;
    NPAGED_LOOKASIDE_LIST fcb_np_lookaside;
    LIST_ENTRY list_entry;
} device_extension;

typedef struct {
    UINT32 type;
    PDEVICE_OBJECT buspdo;
    PDEVICE_OBJECT attached_device;
    UNICODE_STRING bus_name;
} control_device_extension;

typedef struct {
    BTRFS_UUID uuid;
    UINT64 devid;
    UINT64 generation;
    PDEVICE_OBJECT devobj;
    PFILE_OBJECT fileobj;
    UNICODE_STRING pnp_name;
    UINT64 size;
    BOOL seeding;
    BOOL had_drive_letter;
    void* notification_entry;
    ULONG disk_num;
    ULONG part_num;
    LIST_ENTRY list_entry;
} volume_child;

struct pdo_device_extension;

typedef struct _volume_device_extension {
    UINT32 type;
    UNICODE_STRING name;
    PDEVICE_OBJECT device;
    PDEVICE_OBJECT mounted_device;
    PDEVICE_OBJECT pdo;
    struct pdo_device_extension* pdode;
    UNICODE_STRING bus_name;
    PDEVICE_OBJECT attached_device;
    BOOL removing;
    LONG open_count;
} volume_device_extension;

typedef struct pdo_device_extension {
    UINT32 type;
    BTRFS_UUID uuid;
    volume_device_extension* vde;
    PDEVICE_OBJECT pdo;
    BOOL removable;

    UINT64 num_children;
    UINT64 children_loaded;
    ERESOURCE child_lock;
    LIST_ENTRY children;

    LIST_ENTRY list_entry;
} pdo_device_extension;

typedef struct {
    LIST_ENTRY listentry;
    PSID sid;
    UINT32 uid;
} uid_map;

typedef struct {
    LIST_ENTRY listentry;
    PSID sid;
    UINT32 gid;
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
    UINT8* buf;
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
    BOOL need_wait;
    UINT8 *parity1, *parity2, *scratch;
    PMDL mdl, parity1_mdl, parity2_mdl;
} write_data_context;

typedef struct {
    UINT64 address;
    UINT32 length;
    UINT8* data;
    chunk* c;
    LIST_ENTRY list_entry;
} tree_write;

typedef struct {
    UNICODE_STRING us;
    LIST_ENTRY list_entry;
} name_bit;

_Requires_lock_not_held_(Vcb->fcb_lock)
_Acquires_shared_lock_(Vcb->fcb_lock)
static __inline void acquire_fcb_lock_shared(device_extension* Vcb) {
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;

    if (ExAcquireResourceSharedLite(&Vcb->fcb_lock, FALSE))
        return;

    time1 = KeQueryPerformanceCounter(NULL);
#endif

    ExAcquireResourceSharedLite(&Vcb->fcb_lock, TRUE);

#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);
    Vcb->stats.fcb_lock_time += time2.QuadPart - time1.QuadPart;
#endif
}

_Requires_lock_not_held_(Vcb->fcb_lock)
_Acquires_exclusive_lock_(Vcb->fcb_lock)
static __inline void acquire_fcb_lock_exclusive(device_extension* Vcb) {
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;

    if (ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, FALSE))
        return;

    time1 = KeQueryPerformanceCounter(NULL);
#endif

    ExAcquireResourceExclusiveLite(&Vcb->fcb_lock, TRUE);

#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);
    Vcb->stats.fcb_lock_time += time2.QuadPart - time1.QuadPart;
#endif
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

static __inline UINT64 unix_time_to_win(BTRFS_TIME* t) {
    return (t->seconds * 10000000) + (t->nanoseconds / 100) + 116444736000000000;
}

static __inline void win_time_to_unix(LARGE_INTEGER t, BTRFS_TIME* out) {
    ULONGLONG l = (ULONGLONG)t.QuadPart - 116444736000000000;

    out->seconds = l / 10000000;
    out->nanoseconds = (UINT32)((l % 10000000) * 100);
}

_Post_satisfies_(*stripe>=0&&*stripe<num_stripes)
static __inline void get_raid0_offset(_In_ UINT64 off, _In_ UINT64 stripe_length, _In_ UINT16 num_stripes, _Out_ UINT64* stripeoff, _Out_ UINT16* stripe) {
    UINT64 initoff, startoff;

    startoff = off % (num_stripes * stripe_length);
    initoff = (off / (num_stripes * stripe_length)) * stripe_length;

    *stripe = (UINT16)(startoff / stripe_length);
    *stripeoff = initoff + startoff - (*stripe * stripe_length);
}

/* We only have 64 bits for a file ID, which isn't technically enough to be
 * unique on Btrfs. We fudge it by having three bytes for the subvol and
 * five for the inode, which should be good enough.
 * Inodes are also 64 bits on Linux, but the Linux driver seems to get round
 * this by tricking it into thinking subvols are separate volumes. */
static __inline UINT64 make_file_id(root* r, UINT64 inode) {
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
__inline static UINT64 sector_align(_In_ UINT64 n, _In_ UINT64 a) {
    if (n & (a - 1))
        n = (n + a) & ~(a - 1);

    return n;
}

__inline static BOOL is_subvol_readonly(root* r, PIRP Irp) {
    if (!(r->root_item.flags & BTRFS_SUBVOL_READONLY))
        return FALSE;

    if (!r->reserved)
        return TRUE;

    return (!Irp || Irp->RequestorMode == UserMode) && PsGetCurrentProcess() != r->reserved ? TRUE : FALSE;
}

__inline static UINT16 get_extent_data_len(UINT8 type) {
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

__inline static UINT32 get_extent_data_refcount(UINT8 type, void* data) {
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

// in btrfs.c
_Ret_maybenull_
device* find_device_from_uuid(_In_ device_extension* Vcb, _In_ BTRFS_UUID* uuid);

_Success_(return)
BOOL get_file_attributes_from_xattr(_In_reads_bytes_(len) char* val, _In_ UINT16 len, _Out_ ULONG* atts);

ULONG get_file_attributes(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ UINT64 inode,
                          _In_ UINT8 type, _In_ BOOL dotfile, _In_ BOOL ignore_xa, _In_opt_ PIRP Irp);

_Success_(return)
BOOL get_xattr(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* subvol, _In_ UINT64 inode, _In_z_ char* name, _In_ UINT32 crc32,
               _Out_ UINT8** data, _Out_ UINT16* datalen, _In_opt_ PIRP Irp);

#ifndef DEBUG_FCB_REFCOUNTS
void free_fcb(_Inout_ fcb* fcb);
#endif
void free_fileref(_Inout_ file_ref* fr);
void protect_superblocks(_Inout_ chunk* c);
BOOL is_top_level(_In_ PIRP Irp);
NTSTATUS create_root(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ UINT64 id,
                     _Out_ root** rootptr, _In_ BOOL no_tree, _In_ UINT64 offset, _In_opt_ PIRP Irp);
void uninit(_In_ device_extension* Vcb);
NTSTATUS dev_ioctl(_In_ PDEVICE_OBJECT DeviceObject, _In_ ULONG ControlCode, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer, _In_ ULONG InputBufferSize,
                   _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer, _In_ ULONG OutputBufferSize, _In_ BOOLEAN Override, _Out_opt_ IO_STATUS_BLOCK* iosb);
BOOL is_file_name_valid(_In_ PUNICODE_STRING us, _In_ BOOL posix);
void send_notification_fileref(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream);
void send_notification_fcb(_In_ file_ref* fileref, _In_ ULONG filter_match, _In_ ULONG action, _In_opt_ PUNICODE_STRING stream);

#ifdef DEBUG_CHUNK_LOCKS
#define acquire_chunk_lock(c, Vcb) { ExAcquireResourceExclusiveLite(&c->lock, TRUE); InterlockedIncrement(&Vcb->chunk_locks_held); }
#define release_chunk_lock(c, Vcb) { InterlockedDecrement(&Vcb->chunk_locks_held); ExReleaseResourceLite(&c->lock); }
#else
#define acquire_chunk_lock(c, Vcb) ExAcquireResourceExclusiveLite(&(c)->lock, TRUE)
#define release_chunk_lock(c, Vcb) ExReleaseResourceLite(&(c)->lock)
#endif

_Ret_z_
WCHAR* file_desc(_In_ PFILE_OBJECT FileObject);
WCHAR* file_desc_fileref(_In_ file_ref* fileref);
void mark_fcb_dirty(_In_ fcb* fcb);
void mark_fileref_dirty(_In_ file_ref* fileref);
NTSTATUS delete_fileref(_In_ file_ref* fileref, _In_opt_ PFILE_OBJECT FileObject, _In_ BOOL make_orphan, _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback);
void chunk_lock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ UINT64 start, _In_ UINT64 length);
void chunk_unlock_range(_In_ device_extension* Vcb, _In_ chunk* c, _In_ UINT64 start, _In_ UINT64 length);
void init_device(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ BOOL get_nums);
void init_file_cache(_In_ PFILE_OBJECT FileObject, _In_ CC_FILE_SIZES* ccfs);
NTSTATUS sync_read_phys(_In_ PDEVICE_OBJECT DeviceObject, _In_ UINT64 StartingOffset, _In_ ULONG Length,
                        _Out_writes_bytes_(Length) PUCHAR Buffer, _In_ BOOL override);
NTSTATUS get_device_pnp_name(_In_ PDEVICE_OBJECT DeviceObject, _Out_ PUNICODE_STRING pnp_name, _Out_ const GUID** guid);
void log_device_error(_In_ device_extension* Vcb, _Inout_ device* dev, _In_ int error);
NTSTATUS find_chunk_usage(_In_ _Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_opt_ PIRP Irp);
#ifdef __REACTOS__
NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);
#else
NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);
#endif
void reap_fcb(fcb* fcb);
void reap_fcbs(device_extension* Vcb);
void reap_fileref(device_extension* Vcb, file_ref* fr);
void reap_filerefs(device_extension* Vcb, file_ref* fr);
UINT64 chunk_estimate_phys_size(device_extension* Vcb, chunk* c, UINT64 u);

#ifdef _MSC_VER
#define funcname __FUNCTION__
#else
#define funcname __func__
#endif

extern BOOL have_sse2;

extern UINT32 mount_compress;
extern UINT32 mount_compress_force;
extern UINT32 mount_compress_type;
extern UINT32 mount_zlib_level;
extern UINT32 mount_zstd_level;
extern UINT32 mount_flush_interval;
extern UINT32 mount_max_inline;
extern UINT32 mount_skip_balance;
extern UINT32 mount_no_barrier;
extern UINT32 mount_no_trim;
extern UINT32 mount_clear_cache;
extern UINT32 mount_allow_degraded;
extern UINT32 mount_readonly;
extern UINT32 no_pnp;

#ifdef _DEBUG

extern BOOL log_started;
extern UINT32 debug_log_level;

#ifdef DEBUG_LONG_MESSAGES

#define MSG(fn, file, line, s, level, ...) (!log_started || level <= debug_log_level) ? _debug_message(fn, file, line, s, ##__VA_ARGS__) : 0

#define TRACE(s, ...) MSG(funcname, __FILE__, __LINE__, s, 3, ##__VA_ARGS__)
#define WARN(s, ...) MSG(funcname, __FILE__, __LINE__, s, 2, ##__VA_ARGS__)
#define FIXME(s, ...) MSG(funcname, __FILE__, __LINE__, s, 1, ##__VA_ARGS__)
#define ERR(s, ...) MSG(funcname, __FILE__, __LINE__, s, 1, ##__VA_ARGS__)

void _debug_message(_In_ const char* func, _In_ const char* file, _In_ unsigned int line, _In_ char* s, ...);

#else

#define MSG(fn, s, level, ...) (!log_started || level <= debug_log_level) ? _debug_message(fn, s, ##__VA_ARGS__) : 0

#define TRACE(s, ...) MSG(funcname, s, 3, ##__VA_ARGS__)
#define WARN(s, ...) MSG(funcname, s, 2, ##__VA_ARGS__)
#define FIXME(s, ...) MSG(funcname, s, 1, ##__VA_ARGS__)
#define ERR(s, ...) MSG(funcname, s, 1, ##__VA_ARGS__)

void _debug_message(_In_ const char* func, _In_ char* s, ...);

#endif

#else

#define TRACE(s, ...)
#define WARN(s, ...)
#define FIXME(s, ...) DbgPrint("Btrfs FIXME : %s : " s, funcname, ##__VA_ARGS__)
#define ERR(s, ...) DbgPrint("Btrfs ERR : %s : " s, funcname, ##__VA_ARGS__)

#endif

#ifdef DEBUG_FCB_REFCOUNTS
void _free_fcb(_Inout_ fcb* fcb, _In_ const char* func);
#define free_fcb(fcb) _free_fcb(fcb, funcname)
#endif

// in fastio.c
void init_fast_io_dispatch(FAST_IO_DISPATCH** fiod);

// in crc32c.c
UINT32 calc_crc32c(_In_ UINT32 seed, _In_reads_bytes_(msglen) UINT8* msg, _In_ ULONG msglen);

typedef struct {
    LIST_ENTRY* list;
    LIST_ENTRY* list_size;
    UINT64 address;
    UINT64 length;
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
                   _In_ const KEY* searchkey, _In_ BOOL ignore, _In_opt_ PIRP Irp);
NTSTATUS find_item_to_level(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, UINT8 level, PIRP Irp);
BOOL find_next_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* next_tp, BOOL ignore, PIRP Irp);
BOOL find_prev_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* prev_tp, PIRP Irp);
void free_trees(device_extension* Vcb);
NTSTATUS insert_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _In_ root* r, _In_ UINT64 obj_id,
                          _In_ UINT8 obj_type, _In_ UINT64 offset, _In_reads_bytes_opt_(size) _When_(return >= 0, __drv_aliasesMem) void* data,
                          _In_ UINT16 size, _Out_opt_ traverse_ptr* ptp, _In_opt_ PIRP Irp);
NTSTATUS delete_tree_item(_In_ _Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, _Inout_ traverse_ptr* tp);
void free_tree(tree* t);
NTSTATUS load_tree(device_extension* Vcb, UINT64 addr, UINT8* buf, root* r, tree** pt);
NTSTATUS do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, PIRP Irp);
void clear_rollback(LIST_ENTRY* rollback);
void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback);
void free_trees_root(device_extension* Vcb, root* r);
void add_rollback(_In_ LIST_ENTRY* rollback, _In_ enum rollback_type type, _In_ __drv_aliasesMem void* ptr);
NTSTATUS commit_batch_list(_Requires_exclusive_lock_held_(_Curr_->tree_lock) device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp);
void clear_batch_list(device_extension* Vcb, LIST_ENTRY* batchlist);
NTSTATUS skip_to_difference(device_extension* Vcb, traverse_ptr* tp, traverse_ptr* tp2, BOOL* ended1, BOOL* ended2);

// in search.c
NTSTATUS remove_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath);

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI mountmgr_thread(_In_ void* context);
#else
void mountmgr_thread(_In_ void* context);
#endif

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI pnp_notification(PVOID NotificationStructure, PVOID Context);
#else
NTSTATUS pnp_notification(PVOID NotificationStructure, PVOID Context);
#endif

void disk_arrival(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath);
void volume_arrival(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath);
void volume_removal(PDRIVER_OBJECT DriverObject, PUNICODE_STRING devpath);

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI volume_notification(PVOID NotificationStructure, PVOID Context);
#else
NTSTATUS volume_notification(PVOID NotificationStructure, PVOID Context);
#endif

void remove_volume_child(_Inout_ _Requires_exclusive_lock_held_(_Curr_->child_lock) _Releases_exclusive_lock_(_Curr_->child_lock) _In_ volume_device_extension* vde,
                         _In_ volume_child* vc, _In_ BOOL skip_dev);

// in cache.c
NTSTATUS init_cache();
void free_cache();
extern CACHE_MANAGER_CALLBACKS* cache_callbacks;

// in write.c
NTSTATUS write_file(device_extension* Vcb, PIRP Irp, BOOLEAN wait, BOOLEAN deferred_write);
NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, BOOLEAN paging_io, BOOLEAN no_cache,
                     BOOLEAN wait, BOOLEAN deferred_write, BOOLEAN write_irp, LIST_ENTRY* rollback);
NTSTATUS truncate_file(fcb* fcb, UINT64 end, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS extend_file(fcb* fcb, file_ref* fileref, UINT64 end, BOOL prealloc, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, PIRP Irp, LIST_ENTRY* rollback);
chunk* get_chunk_from_address(device_extension* Vcb, UINT64 address);
NTSTATUS alloc_chunk(device_extension* Vcb, UINT64 flags, chunk** pc, BOOL full_size);
NTSTATUS write_data(_In_ device_extension* Vcb, _In_ UINT64 address, _In_reads_bytes_(length) void* data, _In_ UINT32 length, _In_ write_data_context* wtc,
                    _In_opt_ PIRP Irp, _In_opt_ chunk* c, _In_ BOOL file_write, _In_ UINT64 irp_offset, _In_ ULONG priority);
NTSTATUS write_data_complete(device_extension* Vcb, UINT64 address, void* data, UINT32 length, PIRP Irp, chunk* c, BOOL file_write, UINT64 irp_offset, ULONG priority);
void free_write_data_stripes(write_data_context* wtc);

_Dispatch_type_(IRP_MJ_WRITE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Requires_lock_held_(c->lock)
_When_(return != 0, _Releases_lock_(c->lock))
BOOL insert_extent_chunk(_In_ device_extension* Vcb, _In_ fcb* fcb, _In_ chunk* c, _In_ UINT64 start_data, _In_ UINT64 length, _In_ BOOL prealloc, _In_opt_ void* data,
                         _In_opt_ PIRP Irp, _In_ LIST_ENTRY* rollback, _In_ UINT8 compression, _In_ UINT64 decoded_size, _In_ BOOL file_write, _In_ UINT64 irp_offset);

NTSTATUS do_write_file(fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, PIRP Irp, BOOL file_write, UINT32 irp_offset, LIST_ENTRY* rollback);
NTSTATUS write_compressed(fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, PIRP Irp, LIST_ENTRY* rollback);
BOOL find_data_address_in_chunk(device_extension* Vcb, chunk* c, UINT64 length, UINT64* address);
void get_raid56_lock_range(chunk* c, UINT64 address, UINT64 length, UINT64* lockaddr, UINT64* locklen);
NTSTATUS calc_csum(_In_ device_extension* Vcb, _In_reads_bytes_(sectors*Vcb->superblock.sector_size) UINT8* data,
                   _In_ UINT32 sectors, _Out_writes_bytes_(sectors*sizeof(UINT32)) UINT32* csum);
void add_insert_extent_rollback(LIST_ENTRY* rollback, fcb* fcb, extent* ext);
NTSTATUS add_extent_to_fcb(_In_ fcb* fcb, _In_ UINT64 offset, _In_reads_bytes_(edsize) EXTENT_DATA* ed, _In_ UINT16 edsize,
                           _In_ BOOL unique, _In_opt_ _When_(return >= 0, __drv_aliasesMem) UINT32* csum, _In_ LIST_ENTRY* rollback);
void add_extent(_In_ fcb* fcb, _In_ LIST_ENTRY* prevextle, _In_ __drv_aliasesMem extent* newext);

// in dirctrl.c

_Dispatch_type_(IRP_MJ_DIRECTORY_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

ULONG get_reparse_tag(device_extension* Vcb, root* subvol, UINT64 inode, UINT8 type, ULONG atts, BOOL lxss, PIRP Irp);
ULONG get_reparse_tag_fcb(fcb* fcb);

// in security.c

_Dispatch_type_(IRP_MJ_QUERY_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_SET_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

void fcb_get_sd(fcb* fcb, struct _fcb* parent, BOOL look_for_xattr, PIRP Irp);
void add_user_mapping(WCHAR* sidstring, ULONG sidstringlength, UINT32 uid);
void add_group_mapping(WCHAR* sidstring, ULONG sidstringlength, UINT32 gid);
UINT32 sid_to_uid(PSID sid);
NTSTATUS uid_to_sid(UINT32 uid, PSID* sid);
NTSTATUS fcb_get_new_sd(fcb* fcb, file_ref* parfileref, ACCESS_STATE* as);
void find_gid(struct _fcb* fcb, struct _fcb* parfcb, PSECURITY_SUBJECT_CONTEXT subjcont);

// in fileinfo.c

_Dispatch_type_(IRP_MJ_SET_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_QUERY_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_QUERY_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

_Dispatch_type_(IRP_MJ_SET_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

BOOL has_open_children(file_ref* fileref);
NTSTATUS stream_set_end_of_file_information(device_extension* Vcb, UINT16 end, fcb* fcb, file_ref* fileref, BOOL advance_only);
NTSTATUS fileref_get_filename(file_ref* fileref, PUNICODE_STRING fn, USHORT* name_offset, ULONG* preqlen);
void insert_dir_child_into_hash_lists(fcb* fcb, dir_child* dc);
void remove_dir_child_from_hash_lists(fcb* fcb, dir_child* dc);

// in reparse.c
NTSTATUS get_reparse_point(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject, void* buffer, DWORD buflen, ULONG_PTR* retlen);
NTSTATUS set_reparse_point2(fcb* fcb, REPARSE_DATA_BUFFER* rdb, ULONG buflen, ccb* ccb, file_ref* fileref, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS set_reparse_point(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS delete_reparse_point(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// in create.c

_Dispatch_type_(IRP_MJ_CREATE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS open_fileref(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb, _Out_ file_ref** pfr,
                      _In_ PUNICODE_STRING fnus, _In_opt_ file_ref* related, _In_ BOOL parent, _Out_opt_ USHORT* parsed, _Out_opt_ ULONG* fn_offset, _In_ POOL_TYPE pooltype,
                      _In_ BOOL case_sensitive, _In_opt_ PIRP Irp);
NTSTATUS open_fcb(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb,
                  root* subvol, UINT64 inode, UINT8 type, PANSI_STRING utf8, BOOL always_add_hl, fcb* parent, fcb** pfcb, POOL_TYPE pooltype, PIRP Irp);
NTSTATUS load_csum(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, UINT32* csum, UINT64 start, UINT64 length, PIRP Irp);
NTSTATUS load_dir_children(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb, fcb* fcb, BOOL ignore_size, PIRP Irp);
NTSTATUS add_dir_child(fcb* fcb, UINT64 inode, BOOL subvol, PANSI_STRING utf8, PUNICODE_STRING name, UINT8 type, dir_child** pdc);
NTSTATUS open_fileref_child(_Requires_lock_held_(_Curr_->tree_lock) _Requires_exclusive_lock_held_(_Curr_->fcb_lock) _In_ device_extension* Vcb,
                            _In_ file_ref* sf, _In_ PUNICODE_STRING name, _In_ BOOL case_sensitive, _In_ BOOL lastpart, _In_ BOOL streampart,
                            _In_ POOL_TYPE pooltype, _Out_ file_ref** psf2, _In_opt_ PIRP Irp);
fcb* create_fcb(device_extension* Vcb, POOL_TYPE pool_type);
NTSTATUS find_file_in_dir(PUNICODE_STRING filename, fcb* fcb, root** subvol, UINT64* inode, dir_child** pdc, BOOL case_sensitive);
UINT32 inherit_mode(fcb* parfcb, BOOL is_dir);
file_ref* create_fileref(device_extension* Vcb);
NTSTATUS open_fileref_by_inode(_Requires_exclusive_lock_held_(_Curr_->fcb_lock) device_extension* Vcb, root* subvol, UINT64 inode, file_ref** pfr, PIRP Irp);

// in fsctl.c
NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP* Pirp, UINT32 type);
void do_unlock_volume(device_extension* Vcb);
void trim_whole_device(device* dev);
void flush_subvol_fcbs(root* subvol);
BOOL fcb_is_inline(fcb* fcb);

// in flushthread.c

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI flush_thread(void* context);
#else
void flush_thread(void* context);
#endif

NTSTATUS do_write(device_extension* Vcb, PIRP Irp);
NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS flush_fcb(fcb* fcb, BOOL cache, LIST_ENTRY* batchlist, PIRP Irp);
NTSTATUS write_data_phys(_In_ PDEVICE_OBJECT device, _In_ UINT64 address, _In_reads_bytes_(length) void* data, _In_ UINT32 length);
BOOL is_tree_unique(device_extension* Vcb, tree* t, PIRP Irp);
NTSTATUS do_tree_writes(device_extension* Vcb, LIST_ENTRY* tree_writes, BOOL no_free);
void add_checksum_entry(device_extension* Vcb, UINT64 address, ULONG length, UINT32* csum, PIRP Irp);
BOOL find_metadata_address_in_chunk(device_extension* Vcb, chunk* c, UINT64* address);
void add_trim_entry_avoid_sb(device_extension* Vcb, device* dev, UINT64 address, UINT64 size);
NTSTATUS insert_tree_item_batch(LIST_ENTRY* batchlist, device_extension* Vcb, root* r, UINT64 objid, UINT8 objtype, UINT64 offset,
                                _In_opt_ _When_(return >= 0, __drv_aliasesMem) void* data, UINT16 datalen, enum batch_operation operation);
NTSTATUS flush_partial_stripe(device_extension* Vcb, chunk* c, partial_stripe* ps);
NTSTATUS update_dev_item(device_extension* Vcb, device* device, PIRP Irp);

// in read.c

_Dispatch_type_(IRP_MJ_READ)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS read_data(_In_ device_extension* Vcb, _In_ UINT64 addr, _In_ UINT32 length, _In_reads_bytes_opt_(length*sizeof(UINT32)/Vcb->superblock.sector_size) UINT32* csum,
                   _In_ BOOL is_tree, _Out_writes_bytes_(length) UINT8* buf, _In_opt_ chunk* c, _Out_opt_ chunk** pc, _In_opt_ PIRP Irp, _In_ UINT64 generation, _In_ BOOL file_read,
                   _In_ ULONG priority);
NTSTATUS read_file(fcb* fcb, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr, PIRP Irp);
NTSTATUS read_stream(fcb* fcb, UINT8* data, UINT64 start, ULONG length, ULONG* pbr);
NTSTATUS do_read(PIRP Irp, BOOLEAN wait, ULONG* bytes_read);
NTSTATUS check_csum(device_extension* Vcb, UINT8* data, UINT32 sectors, UINT32* csum);
void raid6_recover2(UINT8* sectors, UINT16 num_stripes, ULONG sector_size, UINT16 missing1, UINT16 missing2, UINT8* out);

// in pnp.c

_Dispatch_type_(IRP_MJ_PNP)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_pnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS pnp_surprise_removal(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS pnp_query_remove_device(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// in free-space.c
NTSTATUS load_cache_chunk(device_extension* Vcb, chunk* c, PIRP Irp);
NTSTATUS clear_free_space_cache(device_extension* Vcb, LIST_ENTRY* batchlist, PIRP Irp);
NTSTATUS allocate_cache(device_extension* Vcb, BOOL* changed, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS update_chunk_caches(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback);
NTSTATUS update_chunk_caches_tree(device_extension* Vcb, PIRP Irp);
NTSTATUS add_space_entry(LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 offset, UINT64 size);
void space_list_add(chunk* c, UINT64 address, UINT64 length, LIST_ENTRY* rollback);
void space_list_add2(LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 address, UINT64 length, chunk* c, LIST_ENTRY* rollback);
void space_list_subtract(chunk* c, BOOL deleting, UINT64 address, UINT64 length, LIST_ENTRY* rollback);
void space_list_subtract2(LIST_ENTRY* list, LIST_ENTRY* list_size, UINT64 address, UINT64 length, chunk* c, LIST_ENTRY* rollback);
NTSTATUS load_stored_free_space_cache(device_extension* Vcb, chunk* c, BOOL load_only, PIRP Irp);

// in extent-tree.c
NTSTATUS increase_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT64 inode, UINT64 offset, UINT32 refcount, PIRP Irp);
NTSTATUS decrease_extent_refcount_data(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT64 inode, UINT64 offset,
                                       UINT32 refcount, BOOL superseded, PIRP Irp);
NTSTATUS decrease_extent_refcount_tree(device_extension* Vcb, UINT64 address, UINT64 size, UINT64 root, UINT8 level, PIRP Irp);
UINT64 get_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, PIRP Irp);
BOOL is_extent_unique(device_extension* Vcb, UINT64 address, UINT64 size, PIRP Irp);
NTSTATUS increase_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem, UINT8 level, PIRP Irp);
UINT64 get_extent_flags(device_extension* Vcb, UINT64 address, PIRP Irp);
void update_extent_flags(device_extension* Vcb, UINT64 address, UINT64 flags, PIRP Irp);
NTSTATUS update_changed_extent_ref(device_extension* Vcb, chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset,
                                   INT32 count, BOOL no_csum, BOOL superseded, PIRP Irp);
void add_changed_extent_ref(chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, UINT32 count, BOOL no_csum);
UINT64 find_extent_shared_tree_refcount(device_extension* Vcb, UINT64 address, UINT64 parent, PIRP Irp);
UINT32 find_extent_shared_data_refcount(device_extension* Vcb, UINT64 address, UINT64 parent, PIRP Irp);
NTSTATUS decrease_extent_refcount(device_extension* Vcb, UINT64 address, UINT64 size, UINT8 type, void* data, KEY* firstitem,
                                  UINT8 level, UINT64 parent, BOOL superseded, PIRP Irp);
UINT64 get_extent_data_ref_hash2(UINT64 root, UINT64 objid, UINT64 offset);

// in worker-thread.c
void do_read_job(PIRP Irp);
void do_write_job(device_extension* Vcb, PIRP Irp);
BOOL add_thread_job(device_extension* Vcb, PIRP Irp);

// in registry.c
void read_registry(PUNICODE_STRING regpath, BOOL refresh);
NTSTATUS registry_mark_volume_mounted(BTRFS_UUID* uuid);
NTSTATUS registry_mark_volume_unmounted(BTRFS_UUID* uuid);
NTSTATUS registry_load_volume_options(device_extension* Vcb);
void watch_registry(HANDLE regh);

// in compress.c
NTSTATUS zlib_decompress(UINT8* inbuf, UINT32 inlen, UINT8* outbuf, UINT32 outlen);
NTSTATUS lzo_decompress(UINT8* inbuf, UINT32 inlen, UINT8* outbuf, UINT32 outlen, UINT32 inpageoff);
NTSTATUS zstd_decompress(UINT8* inbuf, UINT32 inlen, UINT8* outbuf, UINT32 outlen);
NTSTATUS write_compressed_bit(fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, BOOL* compressed, PIRP Irp, LIST_ENTRY* rollback);

// in galois.c
void galois_double(UINT8* data, UINT32 len);
void galois_divpower(UINT8* data, UINT8 div, UINT32 readlen);
UINT8 gpow2(UINT8 e);
UINT8 gmul(UINT8 a, UINT8 b);
UINT8 gdiv(UINT8 a, UINT8 b);

// in devctrl.c

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS NTAPI drv_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// in calcthread.c

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI calc_thread(void* context);
#else
void calc_thread(void* context);
#endif

NTSTATUS add_calc_job(device_extension* Vcb, UINT8* data, UINT32 sectors, UINT32* csum, calc_job** pcj);
void free_calc_job(calc_job* cj);

// in balance.c
NTSTATUS start_balance(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode);
NTSTATUS query_balance(device_extension* Vcb, void* data, ULONG length);
NTSTATUS pause_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS resume_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS stop_balance(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS look_for_balance_item(_Requires_lock_held_(_Curr_->tree_lock) device_extension* Vcb);
NTSTATUS remove_device(device_extension* Vcb, void* data, ULONG length, KPROCESSOR_MODE processor_mode);

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
void NTAPI balance_thread(void* context);
#else
void balance_thread(void* context);
#endif

// in volume.c
NTSTATUS vol_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_query_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_set_ea(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_flush_buffers(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_query_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_set_volume_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_cleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_file_system_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_lock_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_device_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_shutdown(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vol_power(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
void add_volume_device(superblock* sb, PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath, UINT64 length, ULONG disk_num, ULONG part_num);
NTSTATUS mountmgr_add_drive_letter(PDEVICE_OBJECT mountmgr, PUNICODE_STRING devpath);

_Function_class_(DRIVER_NOTIFICATION_CALLBACK_ROUTINE)
#ifdef __REACTOS__
NTSTATUS NTAPI pnp_removal(PVOID NotificationStructure, PVOID Context);
#else
NTSTATUS pnp_removal(PVOID NotificationStructure, PVOID Context);
#endif

// in scrub.c
NTSTATUS start_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS query_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode, void* data, ULONG length);
NTSTATUS pause_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS resume_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);
NTSTATUS stop_scrub(device_extension* Vcb, KPROCESSOR_MODE processor_mode);

// in send.c
NTSTATUS send_subvol(device_extension* Vcb, void* data, ULONG datalen, PFILE_OBJECT FileObject, PIRP Irp);
NTSTATUS read_send_buffer(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, ULONG_PTR* retlen, KPROCESSOR_MODE processor_mode);

// based on function in sys/sysmacros.h
#define makedev(major, minor) (((minor) & 0xFF) | (((major) & 0xFFF) << 8) | (((UINT64)((minor) & ~0xFF)) << 12) | (((UINT64)((major) & ~0xFFF)) << 32))

#define fast_io_possible(fcb) (!FsRtlAreThereCurrentFileLocks(&fcb->lock) && !fcb->Vcb->readonly ? FastIoIsPossible : FastIoIsQuestionable)

static __inline void print_open_trees(device_extension* Vcb) {
    LIST_ENTRY* le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        tree_data* td = CONTAINING_RECORD(t->itemlist.Flink, tree_data, list_entry);
        ERR("tree %p: root %llx, level %u, first key (%llx,%x,%llx)\n",
                      t, t->root->id, t->header.level, td->key.obj_id, td->key.obj_type, td->key.offset);

        le = le->Flink;
    }
}

static __inline BOOL write_fcb_compressed(fcb* fcb) {
    // make sure we don't accidentally write the cache inodes or pagefile compressed
    if (fcb->subvol->id == BTRFS_ROOT_ROOT || fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE)
        return FALSE;

    if (fcb->Vcb->options.compress_force)
        return TRUE;

    if (fcb->inode_item.flags & BTRFS_INODE_NOCOMPRESS)
        return FALSE;

    if (fcb->inode_item.flags & BTRFS_INODE_COMPRESS || fcb->Vcb->options.compress)
        return TRUE;

    return FALSE;
}

static __inline void do_xor(UINT8* buf1, UINT8* buf2, UINT32 len) {
    UINT32 j;
#ifndef __REACTOS__
    __m128i x1, x2;
#endif

#ifndef __REACTOS__
    if (have_sse2 && ((uintptr_t)buf1 & 0xf) == 0 && ((uintptr_t)buf2 & 0xf) == 0) {
        while (len >= 16) {
            x1 = _mm_load_si128((__m128i*)buf1);
            x2 = _mm_load_si128((__m128i*)buf2);
            x1 = _mm_xor_si128(x1, x2);
            _mm_store_si128((__m128i*)buf1, x1);

            buf1 += 16;
            buf2 += 16;
            len -= 16;
        }
    }
#endif

    for (j = 0; j < len; j++) {
        *buf1 ^= *buf2;
        buf1++;
        buf2++;
    }
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
#define major(rdev) ((((rdev) >> 8) & 0xFFF) | ((UINT32)((rdev) >> 32) & ~0xFFF))
#define minor(rdev) (((rdev) & 0xFF) | ((UINT32)((rdev) >> 12) & ~0xFF))

static __inline UINT64 fcb_alloc_size(fcb* fcb) {
    if (S_ISDIR(fcb->inode_item.st_mode))
        return 0;
    else if (fcb->atts & FILE_ATTRIBUTE_SPARSE_FILE)
        return fcb->inode_item.st_blocks;
    else
        return sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
}

typedef BOOLEAN (*tPsIsDiskCountersEnabled)();

typedef VOID (*tPsUpdateDiskCounters)(PEPROCESS Process, ULONG64 BytesRead, ULONG64 BytesWritten,
                                      ULONG ReadOperationCount, ULONG WriteOperationCount, ULONG FlushOperationCount);

typedef BOOLEAN (*tCcCopyWriteEx)(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait,
                                  PVOID Buffer, PETHREAD IoIssuerThread);

typedef BOOLEAN (*tCcCopyReadEx)(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait,
                                 PVOID Buffer, PIO_STATUS_BLOCK IoStatus, PETHREAD IoIssuerThread);

#ifndef CC_ENABLE_DISK_IO_ACCOUNTING
#define CC_ENABLE_DISK_IO_ACCOUNTING 0x00000010
#endif

typedef VOID (*tCcSetAdditionalCacheAttributesEx)(PFILE_OBJECT FileObject, ULONG Flags);

typedef VOID (*tFsRtlUpdateDiskCounters)(ULONG64 BytesRead, ULONG64 BytesWritten);

#ifndef __REACTOS__
#ifndef _MSC_VER

#undef RtlIsNtDdiVersionAvailable

BOOLEAN RtlIsNtDdiVersionAvailable(ULONG Version);

PEPROCESS PsGetThreadProcess(_In_ PETHREAD Thread); // not in mingw
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
#endif

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_VISTA)
typedef struct _ECP_LIST ECP_LIST;
typedef struct _ECP_LIST *PECP_LIST;
#endif

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7)
NTSTATUS WINAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                  ULONG *utf8_bytes_written,
                                  const WCHAR *uni_src, ULONG uni_bytes);
NTSTATUS WINAPI RtlUTF8ToUnicodeN(WCHAR *uni_dest, ULONG uni_bytes_max,
                                  ULONG *uni_bytes_written,
                                  const CHAR *utf8_src, ULONG utf8_bytes);
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7) */
#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_VISTA)
NTSTATUS NTAPI FsRtlRemoveDotsFromPath(PWSTR OriginalString,
                                       USHORT PathLength, USHORT *NewLength);
NTSTATUS NTAPI FsRtlValidateReparsePointBuffer(ULONG BufferLength,
                                               PREPARSE_DATA_BUFFER ReparseBuffer);
ULONG NTAPI KeQueryActiveProcessorCount(PKAFFINITY ActiveProcessors);
NTSTATUS NTAPI FsRtlGetEcpListFromIrp(IN PIRP Irp,
                                      OUT PECP_LIST *EcpList);
NTSTATUS NTAPI FsRtlGetNextExtraCreateParameter(IN PECP_LIST EcpList,
                                                IN PVOID CurrentEcpContext,
                                                OUT LPGUID NextEcpType OPTIONAL,
                                                OUT PVOID *NextEcpContext,
                                                OUT PULONG NextEcpContextSize OPTIONAL);
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_VISTA) */

#endif
