/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     BTRFS support for FreeLoader
 * COPYRIGHT:   Copyright 2018 Victor Perevertkin (victor@perevertkin.ru)
 */

/* Structures were taken from u-boot, https://github.com/u-boot/u-boot/tree/master/fs/btrfs */

#pragma once

typedef UCHAR u8;
typedef USHORT u16;
typedef ULONG32 u32;
typedef ULONG64 u64;
/* type that store on disk, but it is same as cpu type for i386 arch */
typedef u8 __u8;
typedef u16 __u16;
typedef u32 __u32;
typedef u64 __u64;

#include <fs/crc32c.h>
#define btrfs_crc32c(name, len) crc32c_le((u32)~1, name, len)

#define BTRFS_SUPER_INFO_OFFSET (64 * 1024)
#define BTRFS_SUPER_INFO_SIZE 4096
#define BTRFS_MAX_LEAF_SIZE 4096
#define BTRFS_BLOCK_SHIFT 12
#define BTRFS_BLOCK_SIZE  (1 << BTRFS_BLOCK_SHIFT)

#define BTRFS_SUPER_MIRROR_MAX   3
#define BTRFS_SUPER_MIRROR_SHIFT 12
#define BTRFS_CSUM_SIZE 32
#define BTRFS_FSID_SIZE 16
#define BTRFS_LABEL_SIZE 256
#define BTRFS_SYSTEM_CHUNK_ARRAY_SIZE 2048
#define BTRFS_UUID_SIZE 16

#define BTRFS_VOL_NAME_MAX 255
#define BTRFS_NAME_MAX 255

#define BTRFS_MAGIC "_BHRfS_M"
#define BTRFS_MAGIC_L 8
#define BTRFS_MAGIC_N 0x4d5f53665248425fULL

#define BTRFS_SUPER_FLAG_METADUMP   (1ULL << 33)

#define BTRFS_DEV_ITEM_KEY      216
#define BTRFS_CHUNK_ITEM_KEY    228
#define BTRFS_ROOT_REF_KEY      156
#define BTRFS_ROOT_ITEM_KEY     132
#define BTRFS_EXTENT_DATA_KEY   108
#define BTRFS_DIR_ITEM_KEY      84
#define BTRFS_DIR_INDEX_KEY     96
#define BTRFS_INODE_ITEM_KEY    1
#define BTRFS_INODE_REF_KEY     12

#define BTRFS_EXTENT_TREE_OBJECTID 2ULL
#define BTRFS_FS_TREE_OBJECTID 5ULL

#define BTRFS_FIRST_FREE_OBJECTID 256ULL
#define BTRFS_LAST_FREE_OBJECTID -256ULL
#define BTRFS_FIRST_CHUNK_TREE_OBJECTID 256ULL

#define BTRFS_FILE_EXTENT_INLINE 0
#define BTRFS_FILE_EXTENT_REG 1
#define BTRFS_FILE_EXTENT_PREALLOC 2

#define BTRFS_MAX_LEVEL 8
#define BTRFS_MAX_CHUNK_ENTRIES 256

#define BTRFS_DEV_ITEMS_OBJECTID 1ULL

#define BTRFS_FT_REG_FILE   1
#define BTRFS_FT_DIR        2
#define BTRFS_FT_SYMLINK    7
#define BTRFS_FT_XATTR      8
#define BTRFS_FT_MAX        9

#define BTRFS_COMPRESS_NONE  0
#define BTRFS_COMPRESS_ZLIB  1
#define BTRFS_COMPRESS_LZO   2

#define ROOT_DIR_WORD 0x002f

#include <pshpack1.h>

struct btrfs_disk_key {
    __u64 objectid;
    __u8 type;
    __u64 offset;
};

struct btrfs_header {
    /* these first four must match the super block */
    __u8 csum[BTRFS_CSUM_SIZE];
    __u8 fsid[BTRFS_FSID_SIZE]; /* FS specific uuid */
    __u64 bytenr; /* which block this node is supposed to live in */
    __u64 flags;

    /* allowed to be different from the super from here on down */
    __u8 chunk_tree_uuid[BTRFS_UUID_SIZE];
    __u64 generation;
    __u64 owner;
    __u32 nritems;
    __u8 level;
};

struct btrfs_item {
    struct btrfs_disk_key key;
    __u32 offset;
    __u32 size;
};

struct btrfs_leaf {
    struct btrfs_header header;
    struct btrfs_item items[];
};

struct btrfs_key_ptr {
    struct btrfs_disk_key key;
    __u64 blockptr;
    __u64 generation;
};

struct btrfs_node {
    struct btrfs_header header;
    struct btrfs_key_ptr ptrs[];
};

struct btrfs_dev_item {
    /* the internal btrfs device id */
    __u64 devid;

    /* size of the device */
    __u64 total_bytes;

    /* bytes used */
    __u64 bytes_used;

    /* optimal io alignment for this device */
    __u32 io_align;

    /* optimal io width for this device */
    __u32 io_width;

    /* minimal io size for this device */
    __u32 sector_size;

    /* type and info about this device */
    __u64 type;

    /* expected generation for this device */
    __u64 generation;

    /*
     * starting byte of this partition on the device,
     * to allow for stripe alignment in the future
     */
    __u64 start_offset;

    /* grouping information for allocation decisions */
    __u32 dev_group;

    /* seek speed 0-100 where 100 is fastest */
    __u8 seek_speed;

    /* bandwidth 0-100 where 100 is fastest */
    __u8 bandwidth;

    /* btrfs generated uuid for this device */
    __u8 uuid[BTRFS_UUID_SIZE];

    /* uuid of FS who owns this device */
    __u8 fsid[BTRFS_UUID_SIZE];
};

struct btrfs_stripe {
    __u64 devid;
    __u64 offset;
    __u8 dev_uuid[BTRFS_UUID_SIZE];
};

struct btrfs_chunk {
    /* size of this chunk in bytes */
    __u64 length;

    /* objectid of the root referencing this chunk */
    __u64 owner;

    __u64 stripe_len;
    __u64 type;

    /* optimal io alignment for this chunk */
    __u32 io_align;

    /* optimal io width for this chunk */
    __u32 io_width;

    /* minimal io size for this chunk */
    __u32 sector_size;

    /* 2^16 stripes is quite a lot, a second limit is the size of a single
     * item in the btree
     */
    __u16 num_stripes;

    /* sub stripes only matter for raid10 */
    __u16 sub_stripes;
    struct btrfs_stripe stripe;
    /* additional stripes go here */
};

struct btrfs_inode_ref {
    __u64 index;
    __u16 name_len;
    /* name goes here */
};

struct btrfs_timespec {
    __u64 sec;
    __u32 nsec;
};

struct btrfs_inode_item {
    /* nfs style generation number */
    __u64 generation;
    /* transid that last touched this inode */
    __u64 transid;
    __u64 size;
    __u64 nbytes;
    __u64 block_group;
    __u32 nlink;
    __u32 uid;
    __u32 gid;
    __u32 mode;
    __u64 rdev;
    __u64 flags;

    /* modification sequence number for NFS */
    __u64 sequence;

    /*
     * a little future expansion, for more than this we can
     * just grow the inode item and version it
     */
    __u64 reserved[4];
    struct btrfs_timespec atime;
    struct btrfs_timespec ctime;
    struct btrfs_timespec mtime;
    struct btrfs_timespec otime;
};


struct btrfs_dir_item {
    struct btrfs_disk_key location;
    __u64 transid;
    __u16 data_len;
    __u16 name_len;
    __u8 type;
};

struct btrfs_root_item {
    struct btrfs_inode_item inode;
    __u64 generation;
    __u64 root_dirid;
    __u64 bytenr;
    __u64 byte_limit;
    __u64 bytes_used;
    __u64 last_snapshot;
    __u64 flags;
    __u32 refs;
    struct btrfs_disk_key drop_progress;
    __u8 drop_level;
    __u8 level;
};

struct btrfs_root_ref {
    __u64 dirid;
    __u64 sequence;
    __u16 name_len;
};

struct btrfs_file_extent_item {
    /*
     * transaction id that created this extent
     */
    __u64 generation;
    /*
     * max number of bytes to hold this extent in ram
     * when we split a compressed extent we can't know how big
     * each of the resulting pieces will be.  So, this is
     * an upper limit on the size of the extent in ram instead of
     * an exact limit.
     */
    __u64 ram_bytes;

    /*
     * 32 bits for the various ways we might encode the data,
     * including compression and encryption.  If any of these
     * are set to something a given disk format doesn't understand
     * it is treated like an incompat flag for reading and writing,
     * but not for stat.
     */
    __u8 compression;
    __u8 encryption;
    __u16 other_encoding; /* spare for later use */

    /* are we inline data or a real extent? */
    __u8 type;

    /*
     * disk space consumed by the extent, checksum blocks are included
     * in these numbers
     *
     * At this offset in the structure, the inline extent data start.
     */
    __u64 disk_bytenr;
    __u64 disk_num_bytes;
    /*
     * the logical offset in file blocks (no csums)
     * this extent record is for.  This allows a file extent to point
     * into the middle of an existing extent on disk, sharing it
     * between two snapshots (useful if some bytes in the middle of the
     * extent have changed
     */
    __u64 offset;
    /*
     * the logical number of file blocks (no csums included).  This
     * always reflects the size uncompressed and without encoding.
     */
    __u64 num_bytes;

};

struct btrfs_super_block {
    __u8 csum[BTRFS_CSUM_SIZE];
    /* the first 4 fields must match struct btrfs_header */
    __u8 fsid[BTRFS_FSID_SIZE];    /* FS specific uuid */
    __u64 bytenr; /* this block number */
    __u64 flags;

    /* allowed to be different from the btrfs_header from here own down */
    __u64 magic;
    __u64 generation;
    __u64 root;
    __u64 chunk_root;
    __u64 log_root;

    /* this will help find the new super based on the log root */
    __u64 log_root_transid;
    __u64 total_bytes;
    __u64 bytes_used;
    __u64 root_dir_objectid;
    __u64 num_devices;
    __u32 sectorsize;
    __u32 nodesize;
    __u32 __unused_leafsize;
    __u32 stripesize;
    __u32 sys_chunk_array_size;
    __u64 chunk_root_generation;
    __u64 compat_flags;
    __u64 compat_ro_flags;
    __u64 incompat_flags;
    __u16 csum_type;
    __u8 root_level;
    __u8 chunk_root_level;
    __u8 log_root_level;
    struct btrfs_dev_item dev_item;

    char label[BTRFS_LABEL_SIZE];

    __u64 cache_generation;
    __u64 uuid_tree_generation;

    /* future expansion */
    __u64 reserved[30];
    __u8 sys_chunk_array[BTRFS_SYSTEM_CHUNK_ARRAY_SIZE];
};

#include <poppack.h>

union tree_buf {
    struct btrfs_header header;
    struct btrfs_node node;
    struct btrfs_leaf leaf;
};

/* remember how we get to a node/leaf */
struct btrfs_path {
    u64 offsets[BTRFS_MAX_LEVEL];
    int itemsnr[BTRFS_MAX_LEVEL];
    int slots[BTRFS_MAX_LEVEL];
    /* remember whole last leaf */
    union tree_buf *tree_buf;
};

/* store logical offset to physical offset mapping */
struct btrfs_chunk_map_item {
    u64 logical;
    u64 length;
    u64 devid;
    u64 physical;
};

struct btrfs_chunk_map {
    struct btrfs_chunk_map_item *map;
    u32 map_length;
    u32 cur_length;
};

typedef struct _BTRFS_INFO *PBTRFS_INFO;

typedef struct {
    u64 inr;
    u64 position;
    struct btrfs_inode_item inode;
    PBTRFS_INFO Volume;
} btrfs_file_info, * pbtrfs_file_info;

const DEVVTBL* BtrFsMount(ULONG DeviceId);
