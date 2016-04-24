#ifndef BTRFSIOCTL_H_DEFINED
#define BTRFSIOCTL_H_DEFINED

#define FSCTL_BTRFS_GET_FILE_IDS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x829, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

typedef struct {
    UINT64 subvol;
    UINT64 inode;
    BOOL top;
} btrfs_get_file_ids;

#endif
