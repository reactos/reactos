// No copyright claimed in this file - do what you want with it.

#ifndef BTRFSIOCTL_H_DEFINED
#define BTRFSIOCTL_H_DEFINED

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

typedef struct {
    UINT64 subvol;
    UINT64 inode;
    BOOL top;
} btrfs_get_file_ids;

typedef struct {
    HANDLE subvol;
    UINT32 namelen;
    WCHAR name[1];
} btrfs_create_snapshot;

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
} btrfs_set_inode_info;

typedef struct {
    UINT32 next_entry;
    UINT64 dev_id;
    UINT64 size;
    BOOL readonly;
    ULONG device_number;
    ULONG partition_number;
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
    USHORT name_length;
    WCHAR name[1];
} btrfs_filesystem_device;

typedef struct {
    UINT32 next_entry;
    UINT8 uuid[16];
    UINT32 num_devices;
    btrfs_filesystem_device device;
} btrfs_filesystem;

#endif
