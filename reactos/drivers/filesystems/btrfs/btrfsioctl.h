// No copyright claimed in this file - do what you want with it.

#ifndef BTRFSIOCTL_H_DEFINED
#define BTRFSIOCTL_H_DEFINED

#include "btrfs.h"

#define FSCTL_BTRFS_GET_FILE_IDS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x829, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_CREATE_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82a, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_CREATE_SNAPSHOT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82b, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_INODE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82c, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_SET_INODE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82d, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_DEVICES CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82e, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_USAGE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82f, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_START_BALANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x830, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_QUERY_BALANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x831, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_PAUSE_BALANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x832, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RESUME_BALANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x833, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_STOP_BALANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x834, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_ADD_DEVICE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x835, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_REMOVE_DEVICE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x836, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_BTRFS_QUERY_FILESYSTEMS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x837, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_UUID CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_START_SCRUB CTL_CODE(FILE_DEVICE_UNKNOWN, 0x839, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_QUERY_SCRUB CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83a, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_PAUSE_SCRUB CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83b, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RESUME_SCRUB CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83c, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_STOP_SCRUB CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83d, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_BTRFS_PROBE_VOLUME CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83e, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RESET_STATS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83f, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_MKNOD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x840, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RECEIVED_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x841, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_XATTRS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x842, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_SET_XATTR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x843, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RESERVE_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x844, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_FIND_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x845, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_SEND_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x846, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_READ_SEND_BUFFER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x847, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_RESIZE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x848, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

typedef struct {
    UINT64 subvol;
    UINT64 inode;
    BOOL top;
} btrfs_get_file_ids;

typedef struct {
    HANDLE subvol;
    BOOL readonly;
    BOOL posix;
    UINT16 namelen;
    WCHAR name[1];
} btrfs_create_snapshot;

typedef struct {
    void* POINTER_32 subvol;
    BOOL readonly;
    BOOL posix;
    UINT16 namelen;
    WCHAR name[1];
} btrfs_create_snapshot32;

#define BTRFS_COMPRESSION_ANY   0
#define BTRFS_COMPRESSION_ZLIB  1
#define BTRFS_COMPRESSION_LZO   2

typedef struct {
    UINT64 subvol;
    UINT64 inode;
    BOOL top;
    UINT8 type;
    UINT32 st_uid;
    UINT32 st_gid;
    UINT32 st_mode;
    UINT64 st_rdev;
    UINT64 flags;
    UINT32 inline_length;
    UINT64 disk_size[3];
    UINT8 compression_type;
} btrfs_inode_info;

typedef struct {
    UINT64 flags;
    BOOL flags_changed;
    UINT32 st_uid;
    BOOL uid_changed;
    UINT32 st_gid;
    BOOL gid_changed;
    UINT32 st_mode;
    BOOL mode_changed;
    UINT8 compression_type;
    BOOL compression_type_changed;
} btrfs_set_inode_info;

typedef struct {
    UINT32 next_entry;
    UINT64 dev_id;
    UINT64 size;
    UINT64 max_size;
    BOOL readonly;
    BOOL missing;
    ULONG device_number;
    ULONG partition_number;
    UINT64 stats[5];
    USHORT namelen;
    WCHAR name[1];
} btrfs_device;

typedef struct {
    UINT64 dev_id;
    UINT64 alloc;
} btrfs_usage_device;

typedef struct {
    UINT32 next_entry;
    UINT64 type;
    UINT64 size;
    UINT64 used;
    UINT64 num_devices;
    btrfs_usage_device devices[1];
} btrfs_usage;

#define BTRFS_BALANCE_OPTS_ENABLED      0x001
#define BTRFS_BALANCE_OPTS_PROFILES     0x002
#define BTRFS_BALANCE_OPTS_DEVID        0x004
#define BTRFS_BALANCE_OPTS_DRANGE       0x008
#define BTRFS_BALANCE_OPTS_VRANGE       0x010
#define BTRFS_BALANCE_OPTS_LIMIT        0x020
#define BTRFS_BALANCE_OPTS_STRIPES      0x040
#define BTRFS_BALANCE_OPTS_USAGE        0x080
#define BTRFS_BALANCE_OPTS_CONVERT      0x100
#define BTRFS_BALANCE_OPTS_SOFT         0x200

#define BLOCK_FLAG_SINGLE 0x1000000000000 // only used in balance

typedef struct {
    UINT64 flags;
    UINT64 profiles;
    UINT64 devid;
    UINT64 drange_start;
    UINT64 drange_end;
    UINT64 vrange_start;
    UINT64 vrange_end;
    UINT64 limit_start;
    UINT64 limit_end;
    UINT16 stripes_start;
    UINT16 stripes_end;
    UINT8 usage_start;
    UINT8 usage_end;
    UINT64 convert;
} btrfs_balance_opts;

#define BTRFS_BALANCE_STOPPED   0
#define BTRFS_BALANCE_RUNNING   1
#define BTRFS_BALANCE_PAUSED    2
#define BTRFS_BALANCE_REMOVAL   4
#define BTRFS_BALANCE_ERROR     8
#define BTRFS_BALANCE_SHRINKING 16

typedef struct {
    UINT32 status;
    UINT64 chunks_left;
    UINT64 total_chunks;
    NTSTATUS error;
    btrfs_balance_opts data_opts;
    btrfs_balance_opts metadata_opts;
    btrfs_balance_opts system_opts;
} btrfs_query_balance;

typedef struct {
    btrfs_balance_opts opts[3];
} btrfs_start_balance;

typedef struct {
    UINT8 uuid[16];
    BOOL missing;
    USHORT name_length;
    WCHAR name[1];
} btrfs_filesystem_device;

typedef struct {
    UINT32 next_entry;
    UINT8 uuid[16];
    UINT32 num_devices;
    btrfs_filesystem_device device;
} btrfs_filesystem;

#define BTRFS_SCRUB_STOPPED     0
#define BTRFS_SCRUB_RUNNING     1
#define BTRFS_SCRUB_PAUSED      2

typedef struct {
    UINT32 next_entry;
    UINT64 address;
    UINT64 device;
    BOOL recovered;
    BOOL is_metadata;
    BOOL parity;

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
} btrfs_scrub_error;

typedef struct {
    UINT32 status;
    LARGE_INTEGER start_time;
    LARGE_INTEGER finish_time;
    UINT64 chunks_left;
    UINT64 total_chunks;
    UINT64 data_scrubbed;
    UINT64 duration;
    NTSTATUS error;
    UINT32 num_errors;
    btrfs_scrub_error errors;
} btrfs_query_scrub;

typedef struct {
    UINT64 inode;
    UINT8 type;
    UINT64 st_rdev;
    UINT16 namelen;
    WCHAR name[1];
} btrfs_mknod;

typedef struct {
    UINT64 generation;
    BTRFS_UUID uuid;
} btrfs_received_subvol;

typedef struct {
    USHORT namelen;
    USHORT valuelen;
    char data[1];
} btrfs_set_xattr;

typedef struct {
    BOOL readonly;
    BOOL posix;
    USHORT namelen;
    WCHAR name[1];
} btrfs_create_subvol;

typedef struct {
    BTRFS_UUID uuid;
    UINT64 ctransid;
} btrfs_find_subvol;

typedef struct {
    HANDLE parent;
    ULONG num_clones;
    HANDLE clones[1];
} btrfs_send_subvol;

typedef struct {
    void* POINTER_32 parent;
    ULONG num_clones;
    void* POINTER_32 clones[1];
} btrfs_send_subvol32;

typedef struct {
    UINT64 device;
    UINT64 size;
} btrfs_resize;

#endif
