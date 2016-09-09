// No copyright claimed in this file - do what you want with it.

#ifndef BTRFSIOCTL_H_DEFINED
#define BTRFSIOCTL_H_DEFINED

#define FSCTL_BTRFS_GET_FILE_IDS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x829, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_CREATE_SUBVOL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82a, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_CREATE_SNAPSHOT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82b, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_GET_INODE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82c, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_BTRFS_SET_INODE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x82d, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

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

#endif
