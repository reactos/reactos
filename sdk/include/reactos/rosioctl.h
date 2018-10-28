/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS PSDK extensions
 * FILE:            include/reactos/rosioctl.h
 * PURPOSE:         Additional partition types
 *                  (partition types not covered by winioctl.h of PSDK)
 *
 * PROGRAMMERS:     Matthias Kupfer (mkupfer@reactos.com)
 */

#ifndef __ROSIOCTL_H
#define __ROSIOCTL_H

#define PARTITION_OS2BOOTMGR          0x0A // OS/2 Boot Manager/OPUS/Coherent swap
#define PARTITION_LINUX_SWAP          0x82 // Linux Swap Partition
#define PARTITION_LINUX               0x83 // Linux Partition Ext2/Ext3/Ext4
#define PARTITION_LINUX_EXT           0x85 // Linux Extended Partition
#define PARTITION_LINUX_LVM           0x8E

#endif /* __ROSIOCTL_H */

/* EOF */

