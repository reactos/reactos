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

#ifndef BTRFS_DRV_H_DEFINED
#define BTRFS_DRV_H_DEFINED

#ifndef __REACTOS__
#undef _WIN32_WINNT
#undef NTDDI_VERSION

#define _WIN32_WINNT 0x0600
#define NTDDI_VERSION 0x06010000 // Win 7
#define _CRT_SECURE_NO_WARNINGS
#endif /* __REACTOS__ */

#include <ntifs.h>
#include <ntddk.h>
#ifdef __REACTOS__
#include <ntdddisk.h>
#endif /* __REACTOS__ */
#include <mountmgr.h>
#ifdef __REACTOS__
#include <rtlfuncs.h>
#include <iotypes.h>
#endif /* __REACTOS__ */
//#include <windows.h>
#include <windef.h>
#include <wdm.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include "btrfs.h"

#ifdef _DEBUG
// #define DEBUG_TREE_REFCOUNTS
// #define DEBUG_FCB_REFCOUNTS
// #define DEBUG_LONG_MESSAGES
#define DEBUG_PARANOID
#endif

#define BTRFS_NODE_TYPE_CCB 0x2295
#define BTRFS_NODE_TYPE_FCB 0x2296

#define ALLOC_TAG 0x7442484D //'MHBt'

#define STDCALL __stdcall

#define UID_NOBODY 65534
#define GID_NOBODY 65534

#define EA_NTACL "security.NTACL"
#define EA_NTACL_HASH 0x45922146

#define EA_DOSATTRIB "user.DOSATTRIB"
#define EA_DOSATTRIB_HASH 0x914f9939

#define READ_AHEAD_GRANULARITY 0x10000 // 64 KB

#ifdef _MSC_VER
#define try __try
#define except __except
#define finally __finally
#else
#define try if (1)
#define except(x) if (0 && (x))
#define finally if (1)
#endif

// #pragma pack(push, 1)

struct device_extension;

typedef struct {
    PDEVICE_OBJECT devobj;
    BTRFS_UUID fsuuid;
    BTRFS_UUID devuuid;
    UINT64 devnum;
    UNICODE_STRING devpath;
    BOOL processed;
    LIST_ENTRY list_entry;
} volume;

typedef struct _fcb_nonpaged {
    FAST_MUTEX HeaderMutex;
    SECTION_OBJECT_POINTERS segment_object;
    ERESOURCE resource;
    ERESOURCE paging_resource;
} fcb_nonpaged;

struct _root;

typedef struct _fcb {
    FSRTL_ADVANCED_FCB_HEADER Header;
    struct _fcb_nonpaged* nonpaged;
    LONG refcount;
    LONG open_count;
    UNICODE_STRING filepart;
    ANSI_STRING utf8;
    struct _device_extension* Vcb;
    struct _fcb* par;
    struct _fcb* prev;
    struct _fcb* next;
    struct _root* subvol;
    LIST_ENTRY children;
    UINT64 inode;
    UINT8 type;
    BOOL delete_on_close;
    INODE_ITEM inode_item;
    UNICODE_STRING full_filename;
    ULONG name_offset;
    SECURITY_DESCRIPTOR* sd;
    FILE_LOCK lock;
    BOOL deleted;
    PKTHREAD lazy_writer_thread;
    ULONG atts;
    SHARE_ACCESS share_access;
    
    BOOL ads;
    UINT32 adssize;
    UINT32 adshash;
    ANSI_STRING adsxattr;
    
    LIST_ENTRY list_entry;
} fcb;

typedef struct _ccb {
    USHORT NodeType;
    CSHORT NodeSize;
    ULONG disposition;
    ULONG options;
    UINT64 query_dir_offset;
//     char* query_string;
    UNICODE_STRING query_string;
    BOOL has_wildcard;
    BOOL specific_file;
} ccb;

// typedef struct _log_to_phys {
//     UINT64 address;
//     UINT64 size;
//     UINT64 physaddr;
//     UINT32 sector_size;
//     struct _log_to_phys* next;
// } log_to_phys;

struct _device_extension;

// enum tree_holder_status {
//     tree_holder_unloaded,
//     tree_holder_loading,
//     tree_holder_loaded,
//     tree_holder_unloading
// };

// typedef struct {
//     enum tree_holder_status status;
//     KSPIN_LOCK spin_lock;
//     ERESOURCE lock;
// } tree_holder_nonpaged;

typedef struct {
    UINT64 address;
    UINT64 generation;
    struct _tree* tree;
//     tree_holder_nonpaged* nonpaged;
} tree_holder;

typedef struct _tree_data {
    KEY key;
    LIST_ENTRY list_entry;
    BOOL ignore;
    BOOL inserted;
    
    union {
        tree_holder treeholder;
        
        struct {
            UINT32 size;
            UINT8* data;
        };
    };
} tree_data;

// typedef struct _tree_nonpaged {
//     ERESOURCE load_tree_lock;
// } tree_nonpaged;

typedef struct _tree {
//     UINT64 address;
//     UINT8 level;
    tree_header header;
    LONG refcount;
    UINT32 size;
    struct _device_extension* Vcb;
    struct _tree* parent;
    tree_data* paritem;
    struct _root* root;
//     tree_nonpaged* nonpaged;
    LIST_ENTRY itemlist;
    LIST_ENTRY list_entry;
    UINT64 new_address;
    UINT64 flags;
} tree;

typedef struct {
//     KSPIN_LOCK load_tree_lock;
    ERESOURCE load_tree_lock;
} root_nonpaged;

typedef struct _root {
    UINT64 id;
    tree_holder treeholder;
    root_nonpaged* nonpaged;
    UINT64 lastinode;
    ROOT_ITEM root_item;
    
    struct _root* prev;
    struct _root* next;
} root;

typedef struct {
    tree* tree;
    tree_data* item;
} traverse_ptr;

typedef struct _tree_cache {
    tree* tree;
    BOOL write;
    LIST_ENTRY list_entry;
} tree_cache;

typedef struct _root_cache {
    root* root;
    struct _root_cache* next;
} root_cache;

#define SPACE_TYPE_FREE     0
#define SPACE_TYPE_USED     1
#define SPACE_TYPE_DELETING 2
#define SPACE_TYPE_WRITING  3

typedef struct {
    UINT64 offset;
    UINT64 size;
    UINT8 type;
    LIST_ENTRY list_entry;
} space;

typedef struct {
    UINT64 address;
    UINT64 size;
    BOOL provisional;
    LIST_ENTRY listentry;
} disk_hole;

typedef struct {
    PDEVICE_OBJECT devobj;
    DEV_ITEM devitem;
    LIST_ENTRY disk_holes;
} device;

typedef struct {
    CHUNK_ITEM* chunk_item;
    UINT32 size;
    UINT64 offset;
    UINT64 used;
    UINT32 oldused;
    BOOL space_changed;
    device** devices;
    LIST_ENTRY space;
    LIST_ENTRY list_entry;
} chunk;

typedef struct {
    KEY key;
    void* data;
    USHORT size;
    LIST_ENTRY list_entry;
} sys_chunk;

typedef struct _device_extension {
    device* devices;
//     DISK_GEOMETRY geometry;
    UINT64 length;
    superblock superblock;
//     WCHAR label[MAX_LABEL_SIZE];
    BOOL readonly;
    fcb* fcbs;
    fcb* volume_fcb;
    fcb* root_fcb;
    ERESOURCE DirResource;
    KSPIN_LOCK FcbListLock;
    ERESOURCE fcb_lock;
    ERESOURCE load_lock;
    ERESOURCE tree_lock;
    PNOTIFY_SYNC NotifySync;
    LIST_ENTRY DirNotifyList;
    LONG tree_lock_counter;
    LONG open_trees;
    ULONG write_trees;
//     ERESOURCE LogToPhysLock;
//     UINT64 chunk_root_phys_addr;
    UINT64 root_tree_phys_addr;
//     log_to_phys* log_to_phys;
    root* roots;
    root* chunk_root;
    root* root_root;
    root* extent_root;
    root* checksum_root;
    root* dev_root;
    BOOL log_to_phys_loaded;
    UINT32 max_inline;
    LIST_ENTRY sys_chunks;
    LIST_ENTRY chunks;
    LIST_ENTRY trees;
    LIST_ENTRY tree_cache;
    HANDLE flush_thread_handle;
    KTIMER flush_thread_timer;
    LIST_ENTRY list_entry;
} device_extension;

typedef struct {
    LIST_ENTRY listentry;
    PSID sid;
    UINT32 uid;
} uid_map;

// #pragma pack(pop)

static __inline void init_tree_holder(tree_holder* th) {
//     th->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_holder_nonpaged), ALLOC_TAG);
//     KeInitializeSpinLock(&th->nonpaged->spin_lock);
//     ExInitializeResourceLite(&th->nonpaged->lock); // FIXME - delete this later
}

static __inline void* map_user_buffer(PIRP Irp) {
    if (!Irp->MdlAddress) {
        return Irp->UserBuffer;
    } else {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    }
}

static __inline UINT64 unix_time_to_win(BTRFS_TIME* t) {
    return (t->seconds * 10000000) + (t->nanoseconds / 100) + 116444736000000000;
}

static __inline void win_time_to_unix(LARGE_INTEGER t, BTRFS_TIME* out) {
    ULONGLONG l = t.QuadPart - 116444736000000000;
    
    out->seconds = l / 10000000;
    out->nanoseconds = (l % 10000000) * 100;
}

// in btrfs.c
device* find_device_from_uuid(device_extension* Vcb, BTRFS_UUID* uuid);
ULONG sector_align( ULONG NumberToBeAligned, ULONG Alignment );
int keycmp(const KEY* key1, const KEY* key2);
ULONG STDCALL get_file_attributes(device_extension* Vcb, INODE_ITEM* ii, root* r, UINT64 inode, UINT8 type, BOOL dotfile, BOOL ignore_xa);
BOOL STDCALL get_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, UINT8** data, UINT16* datalen);
NTSTATUS STDCALL set_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, UINT8* data, UINT16 datalen, LIST_ENTRY* rollback);
BOOL STDCALL delete_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, LIST_ENTRY* rollback);
void _free_fcb(fcb* fcb, const char* func, const char* file, unsigned int line);
BOOL STDCALL get_last_inode(device_extension* Vcb, root* r);
NTSTATUS add_dir_item(device_extension* Vcb, root* subvol, UINT64 inode, UINT32 crc32, DIR_ITEM* di, ULONG disize, LIST_ENTRY* rollback);
NTSTATUS delete_dir_item(device_extension* Vcb, root* subvol, UINT64 parinode, UINT32 crc32, PANSI_STRING utf8, LIST_ENTRY* rollback);
UINT64 find_next_dir_index(device_extension* Vcb, root* subvol, UINT64 inode);
NTSTATUS delete_inode_ref(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 parinode, PANSI_STRING utf8, UINT64* index, LIST_ENTRY* rollback);
NTSTATUS delete_fcb(fcb* fcb, PFILE_OBJECT FileObject, LIST_ENTRY* rollback);
fcb* create_fcb();
void protect_superblocks(device_extension* Vcb, chunk* c);
BOOL is_top_level(PIRP Irp);

#ifdef _MSC_VER
#define funcname __FUNCTION__
#else
#define funcname __func__
#endif

// FIXME - we probably shouldn't be moving funcname etc. around if we're not printing debug messages
#define free_fcb(fcb) _free_fcb(fcb, funcname, __FILE__, __LINE__)

#ifdef _DEBUG

#ifdef DEBUG_LONG_MESSAGES

#define TRACE(s, ...) _debug_message(funcname, 3, __FILE__, __LINE__, s, ##__VA_ARGS__)
#define WARN(s, ...) _debug_message(funcname, 2, __FILE__, __LINE__, s, ##__VA_ARGS__)
#define FIXME(s, ...) _debug_message(funcname, 1, __FILE__, __LINE__, s, ##__VA_ARGS__)
#define ERR(s, ...) _debug_message(funcname, 1, __FILE__, __LINE__, s, ##__VA_ARGS__)

void STDCALL _debug_message(const char* func, UINT8 priority, const char* file, unsigned int line, char* s, ...);

#else

#define TRACE(s, ...) _debug_message(funcname, 3, s, ##__VA_ARGS__)
#define WARN(s, ...) _debug_message(funcname, 2, s, ##__VA_ARGS__)
#define FIXME(s, ...) _debug_message(funcname, 1, s, ##__VA_ARGS__)
#define ERR(s, ...) _debug_message(funcname, 1, s, ##__VA_ARGS__)

void STDCALL _debug_message(const char* func, UINT8 priority, char* s, ...);

#endif

#else

#define TRACE(s, ...)
#define WARN(s, ...)
#ifndef __REACTOS__
#define FIXME(s, ...) DbgPrint("Btrfs FIXME : " funcname " : " s, ##__VA_ARGS__)
#define ERR(s, ...) DbgPrint("Btrfs ERR : " funcname " : " s, ##__VA_ARGS__)
#else
#define FIXME(s, ...) DbgPrint("Btrfs FIXME : %s : " s, funcname, ##__VA_ARGS__)
#define ERR(s, ...) DbgPrint("Btrfs ERR : %s : " s, funcname, ##__VA_ARGS__)
#endif

#endif

// in fastio.c
void STDCALL init_fast_io_dispatch(FAST_IO_DISPATCH** fiod);

// in crc32c.c
UINT32 STDCALL calc_crc32c(UINT32 seed, UINT8* msg, ULONG msglen);

// in treefuncs.c
NTSTATUS STDCALL _find_item(device_extension* Vcb, root* r, traverse_ptr* tp, const KEY* searchkey, BOOL ignore, const char* func, const char* file, unsigned int line);
BOOL STDCALL _find_next_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* next_tp, BOOL ignore, const char* func, const char* file, unsigned int line);
BOOL STDCALL _find_prev_item(device_extension* Vcb, const traverse_ptr* tp, traverse_ptr* prev_tp, BOOL ignore, const char* func, const char* file, unsigned int line);
void STDCALL _free_traverse_ptr(traverse_ptr* tp, const char* func, const char* file, unsigned int line);
void STDCALL free_tree_cache(LIST_ENTRY* tc);
BOOL STDCALL insert_tree_item(device_extension* Vcb, root* r, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, UINT32 size, traverse_ptr* ptp, LIST_ENTRY* rollback);
void STDCALL delete_tree_item(device_extension* Vcb, traverse_ptr* tp, LIST_ENTRY* rollback);
void STDCALL add_to_tree_cache(device_extension* Vcb, tree* t, BOOL write);
tree* STDCALL _free_tree(tree* t, const char* func, const char* file, unsigned int line);
NTSTATUS STDCALL _load_tree(device_extension* Vcb, UINT64 addr, root* r, tree** pt, const char* func, const char* file, unsigned int line);
NTSTATUS STDCALL _do_load_tree(device_extension* Vcb, tree_holder* th, root* r, tree* t, tree_data* td, BOOL* loaded, const char* func, const char* file, unsigned int line);
void clear_rollback(LIST_ENTRY* rollback);
void do_rollback(device_extension* Vcb, LIST_ENTRY* rollback);

#define find_item(Vcb, r, tp, searchkey, ignore) _find_item(Vcb, r, tp, searchkey, ignore, funcname, __FILE__, __LINE__)
#define find_next_item(Vcb, tp, next_tp, ignore) _find_next_item(Vcb, tp, next_tp, ignore, funcname, __FILE__, __LINE__)
#define find_prev_item(Vcb, tp, prev_tp, ignore) _find_prev_item(Vcb, tp, prev_tp, ignore, funcname, __FILE__, __LINE__)
#define free_tree(t) _free_tree(t, funcname, __FILE__, __LINE__)
#define load_tree(t, addr, r, pt) _load_tree(t, addr, r, pt, funcname, __FILE__, __LINE__)
#define free_traverse_ptr(tp) _free_traverse_ptr(tp, funcname, __FILE__, __LINE__)
#define do_load_tree(Vcb, th, r, t, td, loaded) _do_load_tree(Vcb, th, r, t, td, loaded, funcname, __FILE__, __LINE__)  

// in search.c
void STDCALL look_for_vols(LIST_ENTRY* volumes);

// in cache.c
NTSTATUS STDCALL init_cache();
void STDCALL free_cache();
extern CACHE_MANAGER_CALLBACKS* cache_callbacks;

// in write.c
NTSTATUS STDCALL do_write(device_extension* Vcb, LIST_ENTRY* rollback);
NTSTATUS write_file(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, BOOL paging_io, BOOL no_cache, LIST_ENTRY* rollback);
NTSTATUS truncate_file(fcb* fcb, UINT64 end, LIST_ENTRY* rollback);
NTSTATUS extend_file(fcb* fcb, UINT64 end, LIST_ENTRY* rollback);
NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback);
void update_checksum_tree(device_extension* Vcb, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback);
NTSTATUS insert_sparse_extent(device_extension* Vcb, root* r, UINT64 inode, UINT64 start, UINT64 length, LIST_ENTRY* rollback);
NTSTATUS STDCALL add_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* rollback);
NTSTATUS STDCALL remove_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback);
void print_trees(LIST_ENTRY* tc);
chunk* get_chunk_from_address(device_extension* Vcb, UINT64 address);
void add_to_space_list(chunk* c, UINT64 offset, UINT64 size, UINT8 type);
NTSTATUS consider_write(device_extension* Vcb);

// in dirctrl.c
NTSTATUS STDCALL drv_directory_control(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// in security.c
NTSTATUS STDCALL drv_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS STDCALL drv_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
void fcb_get_sd(fcb* fcb);
// UINT32 STDCALL get_uid();
void add_user_mapping(WCHAR* sidstring, ULONG sidstringlength, UINT32 uid);
NTSTATUS fcb_get_new_sd(fcb* fcb, ACCESS_STATE* as);

// in fileinfo.c
NTSTATUS STDCALL drv_set_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS STDCALL drv_query_information(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS add_inode_ref(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 parinode, UINT64 index, PANSI_STRING utf8, LIST_ENTRY* rollback);

// in reparse.c
BOOL follow_symlink(fcb* fcb, PFILE_OBJECT FileObject);
NTSTATUS get_reparse_point(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject, void* buffer, DWORD buflen, DWORD* retlen);
NTSTATUS set_reparse_point(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// in create.c
NTSTATUS STDCALL drv_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS get_fcb(device_extension* Vcb, fcb** pfcb, PUNICODE_STRING fnus, fcb* relatedfcb, BOOL parent);
BOOL STDCALL find_file_in_dir_with_crc32(device_extension* Vcb, PUNICODE_STRING filename, UINT32 crc32, root* r, UINT64 parinode, root** subvol,
                                         UINT64* inode, UINT8* type, PANSI_STRING utf8);

// in fsctl.c
NTSTATUS fsctl_request(PDEVICE_OBJECT DeviceObject, PIRP Irp, UINT32 type, BOOL user);

// in flushthread.c
void STDCALL flush_thread(void* context);

// in read.c
NTSTATUS STDCALL drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL read_file(device_extension* Vcb, root* subvol, UINT64 inode, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr);

static __inline void print_open_trees(device_extension* Vcb) {
    LIST_ENTRY* le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        tree_data* td = CONTAINING_RECORD(t->itemlist.Flink, tree_data, list_entry);
        ERR("tree %p: root %llx, level %u, refcount %u, first key (%llx,%x,%llx)\n",
                      t, t->root->id, t->header.level, t->refcount, td->key.obj_id, td->key.obj_type, td->key.offset);

        le = le->Flink;
    }
}

static __inline void InsertAfter(LIST_ENTRY* head, LIST_ENTRY* item, LIST_ENTRY* before) {
    item->Flink = before->Flink;
    before->Flink = item;
    item->Blink = before;

    if (item->Flink != head)
        item->Flink->Blink = item;
    else
        head->Blink = item;
}

#ifdef _MSC_VER
// #define int3 __asm { int 3 }
#define int3 __debugbreak()
#else
#define int3 asm("int3;")
#endif

#define acquire_tree_lock(Vcb, exclusive) {\
    LONG ref = InterlockedIncrement(&Vcb->tree_lock_counter); \
    ref = ref; \
    if (exclusive) { \
        TRACE("getting tree_lock (exclusive) %u->%u\n", ref-1, ref); \
        ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE); \
        TRACE("open tree count = %i\n", Vcb->open_trees); \
    } else { \
        TRACE("getting tree_lock %u->%u\n", ref-1, ref); \
        ExAcquireResourceSharedLite(&Vcb->tree_lock, TRUE); \
    } \
} 

// if (Vcb->open_trees > 0) { ERR("open tree count = %i\n", Vcb->open_trees); print_open_trees(Vcb); int3; }
// else TRACE("open tree count = %i\n", Vcb->open_trees);

// FIXME - find a way to catch unfreed trees again

#define release_tree_lock(Vcb, exclusive) {\
    LONG ref = InterlockedDecrement(&Vcb->tree_lock_counter); \
    ref = ref; \
    TRACE("releasing tree_lock %u->%u\n", ref+1, ref); \
    if (exclusive) {\
        TRACE("open tree count = %i\n", Vcb->open_trees); \
    } \
    ExReleaseResourceLite(&Vcb->tree_lock); \
}

#ifdef DEBUG_TREE_REFCOUNTS
#ifdef DEBUG_LONG_MESSAGES
#define _increase_tree_rc(t, func, file, line) { \
    LONG rc = InterlockedIncrement(&t->refcount); \
    _debug_message(func, file, line, "tree %p: refcount increased to %i (increase_tree_rc)\n", t, rc); \
}
#else
#define _increase_tree_rc(t, func, file, line) { \
    LONG rc = InterlockedIncrement(&t->refcount); \
    _debug_message(func, "tree %p: refcount increased to %i (increase_tree_rc)\n", t, rc); \
}
#endif
#define increase_tree_rc(t) _increase_tree_rc(t, funcname, __FILE__, __LINE__)
#else
#define increase_tree_rc(t) InterlockedIncrement(&t->refcount);
#define _increase_tree_rc(t, func, file, line) increase_tree_rc(t)
#endif

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

#ifndef S_IXUSR
#define S_IXUSR 0000100
#endif

#ifdef __REACTOS__
#define S_IFDIR __S_IFDIR
#define S_IFREG __S_IFREG
#endif /* __REACTOS__ */

#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3)
#endif

#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3)
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
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7) */

#endif
