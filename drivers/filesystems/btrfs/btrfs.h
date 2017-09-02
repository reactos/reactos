/* btrfs.h
 * Generic btrfs header file. Thanks to whoever it was who wrote
 * https://btrfs.wiki.kernel.org/index.php/On-disk_Format - you saved me a lot of time!
 *
 * I release this file, and this file only, into the public domain - do whatever
 * you want with it. You don't have to, but I'd appreciate if you let me know if you
 * use it anything cool - mark@harmstone.com. */

#ifndef BTRFS_H_DEFINED
#define BTRFS_H_DEFINED

static const UINT64 superblock_addrs[] = { 0x10000, 0x4000000, 0x4000000000, 0x4000000000000, 0 };

#define BTRFS_MAGIC         0x4d5f53665248425f
#define MAX_LABEL_SIZE      0x100
#define SUBVOL_ROOT_INODE   0x100

#define TYPE_INODE_ITEM        0x01
#define TYPE_INODE_REF         0x0C
#define TYPE_INODE_EXTREF      0x0D
#define TYPE_XATTR_ITEM        0x18
#define TYPE_DIR_ITEM          0x54
#define TYPE_DIR_INDEX         0x60
#define TYPE_EXTENT_DATA       0x6C
#define TYPE_EXTENT_CSUM       0x80
#define TYPE_ROOT_ITEM         0x84
#define TYPE_ROOT_BACKREF      0x90
#define TYPE_ROOT_REF          0x9C
#define TYPE_EXTENT_ITEM       0xA8
#define TYPE_METADATA_ITEM     0xA9
#define TYPE_TREE_BLOCK_REF    0xB0
#define TYPE_EXTENT_DATA_REF   0xB2
#define TYPE_EXTENT_REF_V0     0xB4
#define TYPE_SHARED_BLOCK_REF  0xB6
#define TYPE_SHARED_DATA_REF   0xB8
#define TYPE_BLOCK_GROUP_ITEM  0xC0
#define TYPE_DEV_EXTENT        0xCC
#define TYPE_DEV_ITEM          0xD8
#define TYPE_CHUNK_ITEM        0xE4
#define TYPE_TEMP_ITEM         0xF8
#define TYPE_DEV_STATS         0xF9
#define TYPE_SUBVOL_UUID       0xFB

#define BTRFS_ROOT_ROOT         1
#define BTRFS_ROOT_EXTENT       2
#define BTRFS_ROOT_CHUNK        3
#define BTRFS_ROOT_DEVTREE      4
#define BTRFS_ROOT_FSTREE       5
#define BTRFS_ROOT_CHECKSUM     7
#define BTRFS_ROOT_UUID         9
#define BTRFS_ROOT_DATA_RELOC   0xFFFFFFFFFFFFFFF7

#define BTRFS_COMPRESSION_NONE  0
#define BTRFS_COMPRESSION_ZLIB  1
#define BTRFS_COMPRESSION_LZO   2

#define BTRFS_ENCRYPTION_NONE   0

#define BTRFS_ENCODING_NONE     0

#define EXTENT_TYPE_INLINE      0
#define EXTENT_TYPE_REGULAR     1
#define EXTENT_TYPE_PREALLOC    2

#define BLOCK_FLAG_DATA         0x001
#define BLOCK_FLAG_SYSTEM       0x002
#define BLOCK_FLAG_METADATA     0x004
#define BLOCK_FLAG_RAID0        0x008
#define BLOCK_FLAG_RAID1        0x010
#define BLOCK_FLAG_DUPLICATE    0x020
#define BLOCK_FLAG_RAID10       0x040
#define BLOCK_FLAG_RAID5        0x080
#define BLOCK_FLAG_RAID6        0x100

#define FREE_SPACE_CACHE_ID     0xFFFFFFFFFFFFFFF5
#define EXTENT_CSUM_ID          0xFFFFFFFFFFFFFFF6
#define BALANCE_ITEM_ID         0xFFFFFFFFFFFFFFFC

#define BTRFS_INODE_NODATASUM   0x001
#define BTRFS_INODE_NODATACOW   0x002
#define BTRFS_INODE_READONLY    0x004
#define BTRFS_INODE_NOCOMPRESS  0x008
#define BTRFS_INODE_PREALLOC    0x010
#define BTRFS_INODE_SYNC        0x020
#define BTRFS_INODE_IMMUTABLE   0x040
#define BTRFS_INODE_APPEND      0x080
#define BTRFS_INODE_NODUMP      0x100
#define BTRFS_INODE_NOATIME     0x200
#define BTRFS_INODE_DIRSYNC     0x400
#define BTRFS_INODE_COMPRESS    0x800

#define BTRFS_SUBVOL_READONLY   0x1

#define BTRFS_COMPAT_RO_FLAGS_FREE_SPACE_CACHE  0x1

#define BTRFS_INCOMPAT_FLAGS_MIXED_BACKREF      0x0001
#define BTRFS_INCOMPAT_FLAGS_DEFAULT_SUBVOL     0x0002
#define BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS       0x0004
#define BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO       0x0008
#define BTRFS_INCOMPAT_FLAGS_COMPRESS_LZOV2     0x0010
#define BTRFS_INCOMPAT_FLAGS_BIG_METADATA       0x0020
#define BTRFS_INCOMPAT_FLAGS_EXTENDED_IREF      0x0040
#define BTRFS_INCOMPAT_FLAGS_RAID56             0x0080
#define BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA    0x0100
#define BTRFS_INCOMPAT_FLAGS_NO_HOLES           0x0200

#define BTRFS_SUPERBLOCK_FLAGS_SEEDING   0x100000000

#pragma pack(push, 1)

typedef struct {
    UINT8 uuid[16];
} BTRFS_UUID;

typedef struct {
    UINT64 obj_id;
    UINT8 obj_type;
    UINT64 offset;
} KEY;

#define HEADER_FLAG_WRITTEN         0x000000000000001
#define HEADER_FLAG_SHARED_BACKREF  0x000000000000002
#define HEADER_FLAG_MIXED_BACKREF   0x100000000000000

typedef struct {
    UINT8 csum[32];
    BTRFS_UUID fs_uuid;
    UINT64 address;
    UINT64 flags;
    BTRFS_UUID chunk_tree_uuid;
    UINT64 generation;
    UINT64 tree_id;
    UINT32 num_items;
    UINT8 level;
} tree_header;

typedef struct {
    KEY key;
    UINT32 offset;
    UINT32 size;
} leaf_node;

typedef struct {
    KEY key;
    UINT64 address;
    UINT64 generation;
} internal_node;

typedef struct {
    UINT64 dev_id;
    UINT64 num_bytes;
    UINT64 bytes_used;
    UINT32 optimal_io_align;
    UINT32 optimal_io_width;
    UINT32 minimal_io_size;
    UINT64 type;
    UINT64 generation;
    UINT64 start_offset;
    UINT32 dev_group;
    UINT8 seek_speed;
    UINT8 bandwidth;
    BTRFS_UUID device_uuid;
    BTRFS_UUID fs_uuid;
} DEV_ITEM;

#define SYS_CHUNK_ARRAY_SIZE 0x800
#define BTRFS_NUM_BACKUP_ROOTS 4

typedef struct {
    UINT64 root_tree_addr;
    UINT64 root_tree_generation;
    UINT64 chunk_tree_addr;
    UINT64 chunk_tree_generation;
    UINT64 extent_tree_addr;
    UINT64 extent_tree_generation;
    UINT64 fs_tree_addr;
    UINT64 fs_tree_generation;
    UINT64 dev_root_addr;
    UINT64 dev_root_generation;
    UINT64 csum_root_addr;
    UINT64 csum_root_generation;
    UINT64 total_bytes;
    UINT64 bytes_used;
    UINT64 num_devices;
    UINT64 reserved[4];
    UINT8 root_level;
    UINT8 chunk_root_level;
    UINT8 extent_root_level;
    UINT8 fs_root_level;
    UINT8 dev_root_level;
    UINT8 csum_root_level;
    UINT8 reserved2[10];
} superblock_backup;

typedef struct {
    UINT8 checksum[32];
    BTRFS_UUID uuid;
    UINT64 sb_phys_addr;
    UINT64 flags;
    UINT64 magic;
    UINT64 generation;
    UINT64 root_tree_addr;
    UINT64 chunk_tree_addr;
    UINT64 log_tree_addr;
    UINT64 log_root_transid;
    UINT64 total_bytes;
    UINT64 bytes_used;
    UINT64 root_dir_objectid;
    UINT64 num_devices;
    UINT32 sector_size;
    UINT32 node_size;
    UINT32 leaf_size;
    UINT32 stripe_size;
    UINT32 n;
    UINT64 chunk_root_generation;
    UINT64 compat_flags;
    UINT64 compat_ro_flags;
    UINT64 incompat_flags;
    UINT16 csum_type;
    UINT8 root_level;
    UINT8 chunk_root_level;
    UINT8 log_root_level;
    DEV_ITEM dev_item;
    char label[MAX_LABEL_SIZE];
    UINT64 cache_generation;
    UINT64 uuid_tree_generation;
    UINT64 reserved[30];
    UINT8 sys_chunk_array[SYS_CHUNK_ARRAY_SIZE];
    superblock_backup backup[BTRFS_NUM_BACKUP_ROOTS];
    UINT8 reserved2[565];
} superblock;

#define BTRFS_TYPE_UNKNOWN   0
#define BTRFS_TYPE_FILE      1
#define BTRFS_TYPE_DIRECTORY 2
#define BTRFS_TYPE_CHARDEV   3
#define BTRFS_TYPE_BLOCKDEV  4
#define BTRFS_TYPE_FIFO      5
#define BTRFS_TYPE_SOCKET    6
#define BTRFS_TYPE_SYMLINK   7
#define BTRFS_TYPE_EA        8

typedef struct {
    KEY key;
    UINT64 transid;
    UINT16 m;
    UINT16 n;
    UINT8 type;
    char name[1];
} DIR_ITEM;

typedef struct {
    UINT64 seconds;
    UINT32 nanoseconds;
} BTRFS_TIME;

typedef struct {
    UINT64 generation;
    UINT64 transid;
    UINT64 st_size;
    UINT64 st_blocks;
    UINT64 block_group;
    UINT32 st_nlink;
    UINT32 st_uid;
    UINT32 st_gid;
    UINT32 st_mode;
    UINT64 st_rdev;
    UINT64 flags;
    UINT64 sequence;
    UINT8 reserved[32];
    BTRFS_TIME st_atime;
    BTRFS_TIME st_ctime;
    BTRFS_TIME st_mtime;
    BTRFS_TIME otime;
} INODE_ITEM;

typedef struct {
    INODE_ITEM inode;
    UINT64 generation;
    UINT64 objid;
    UINT64 block_number;
    UINT64 byte_limit;
    UINT64 bytes_used;
    UINT64 last_snapshot_generation;
    UINT64 flags;
    UINT32 num_references;
    KEY drop_progress;
    UINT8 drop_level;
    UINT8 root_level;
    UINT64 generation2;
    BTRFS_UUID uuid;
    BTRFS_UUID parent_uuid;
    BTRFS_UUID received_uuid;
    UINT64 ctransid;
    UINT64 otransid;
    UINT64 stransid;
    UINT64 rtransid;
    BTRFS_TIME ctime;
    BTRFS_TIME otime;
    BTRFS_TIME stime;
    BTRFS_TIME rtime;
    UINT64 reserved[8];
} ROOT_ITEM;

typedef struct {
    UINT64 size;
    UINT64 root_id;
    UINT64 stripe_length;
    UINT64 type;
    UINT32 opt_io_alignment;
    UINT32 opt_io_width;
    UINT32 sector_size;
    UINT16 num_stripes;
    UINT16 sub_stripes;
} CHUNK_ITEM;

typedef struct {
    UINT64 dev_id;
    UINT64 offset;
    BTRFS_UUID dev_uuid;
} CHUNK_ITEM_STRIPE;

typedef struct {
    UINT64 generation;
    UINT64 decoded_size;
    UINT8 compression;
    UINT8 encryption;
    UINT16 encoding;
    UINT8 type;
    UINT8 data[1];
} EXTENT_DATA;

typedef struct {
    UINT64 address;
    UINT64 size;
    UINT64 offset;
    UINT64 num_bytes;
} EXTENT_DATA2;

typedef struct {
    UINT64 index;
    UINT16 n;
    char name[1];
} INODE_REF;

typedef struct {
    UINT64 dir;
    UINT64 index;
    UINT16 n;
    char name[1];
} INODE_EXTREF;

#define EXTENT_ITEM_DATA            0x001
#define EXTENT_ITEM_TREE_BLOCK      0x002
#define EXTENT_ITEM_SHARED_BACKREFS 0x100

typedef struct {
    UINT64 refcount;
    UINT64 generation;
    UINT64 flags;
} EXTENT_ITEM;

typedef struct {
    KEY firstitem;
    UINT8 level;
} EXTENT_ITEM2;

typedef struct {
    UINT32 refcount;
} EXTENT_ITEM_V0;

typedef struct {
    EXTENT_ITEM extent_item;
    KEY firstitem;
    UINT8 level;
} EXTENT_ITEM_TREE;

typedef struct {
    UINT64 offset;
} TREE_BLOCK_REF;

typedef struct {
    UINT64 root;
    UINT64 objid;
    UINT64 offset;
    UINT32 count;
} EXTENT_DATA_REF;

typedef struct {
    UINT64 used;
    UINT64 chunk_tree;
    UINT64 flags;
} BLOCK_GROUP_ITEM;

typedef struct {
    UINT64 root;
    UINT64 gen;
    UINT64 objid;
    UINT32 count;
} EXTENT_REF_V0;

typedef struct {
    UINT64 offset;
} SHARED_BLOCK_REF;

typedef struct {
    UINT64 offset;
    UINT32 count;
} SHARED_DATA_REF;

#define FREE_SPACE_EXTENT 1
#define FREE_SPACE_BITMAP 2

typedef struct {
    UINT64 offset;
    UINT64 size;
    UINT8 type;
} FREE_SPACE_ENTRY;

typedef struct {
    KEY key;
    UINT64 generation;
    UINT64 num_entries;
    UINT64 num_bitmaps;
} FREE_SPACE_ITEM;

typedef struct {
    UINT64 dir;
    UINT64 index;
    UINT16 n;
    char name[1];
} ROOT_REF;

typedef struct {
    UINT64 chunktree;
    UINT64 objid;
    UINT64 address;
    UINT64 length;
    BTRFS_UUID chunktree_uuid;
} DEV_EXTENT;

#define BALANCE_FLAGS_DATA          0x1
#define BALANCE_FLAGS_SYSTEM        0x2
#define BALANCE_FLAGS_METADATA      0x4

#define BALANCE_ARGS_FLAGS_PROFILES         0x001
#define BALANCE_ARGS_FLAGS_USAGE            0x002
#define BALANCE_ARGS_FLAGS_DEVID            0x004
#define BALANCE_ARGS_FLAGS_DRANGE           0x008
#define BALANCE_ARGS_FLAGS_VRANGE           0x010
#define BALANCE_ARGS_FLAGS_LIMIT            0x020
#define BALANCE_ARGS_FLAGS_LIMIT_RANGE      0x040
#define BALANCE_ARGS_FLAGS_STRIPES_RANGE    0x080
#define BALANCE_ARGS_FLAGS_CONVERT          0x100
#define BALANCE_ARGS_FLAGS_SOFT             0x200
#define BALANCE_ARGS_FLAGS_USAGE_RANGE      0x400

typedef struct {
    UINT64 profiles;

    union {
            UINT64 usage;
            struct {
                    UINT32 usage_start;
                    UINT32 usage_end;
            };
    };

    UINT64 devid;
    UINT64 drange_start;
    UINT64 drange_end;
    UINT64 vrange_start;
    UINT64 vrange_end;
    UINT64 convert;
    UINT64 flags;

    union {
            UINT64 limit;
            struct {
                    UINT32 limit_start;
                    UINT32 limit_end;
            };
    };

    UINT32 stripes_start;
    UINT32 stripes_end;
    UINT8 reserved[48];
} BALANCE_ARGS;

typedef struct {
    UINT64 flags;
    BALANCE_ARGS data;
    BALANCE_ARGS metadata;
    BALANCE_ARGS system;
    UINT8 reserved[32];
} BALANCE_ITEM;

#pragma pack(pop)

#endif
