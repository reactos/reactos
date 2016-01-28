/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ext2.h
 * PURPOSE:          EXT2 Header File
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <pshpack1.h>
typedef struct _EXT2_SUPER_BLOCK {
    ULONG InodesCount;
    ULONG BlocksCount;
    ULONG ReservedBlocksCount;
    ULONG FreeBlocksCount;
    ULONG FreeInodesCount;
    ULONG FirstDataBlock;
    ULONG LogBlockSize;
    LONG LogFragSize;
    ULONG BlocksPerGroup;
    ULONG FragsPerGroup;
    ULONG InodesPerGroup;
    ULONG MountTime;
    ULONG WriteTime;
    USHORT MountCount;
    SHORT MaxMountCount;
    USHORT Magic;
    USHORT State;
    USHORT Errors;
    USHORT MinorRevLevel;
    ULONG LastCheck;
    ULONG CheckInterval;
    ULONG CreatorOS;
    ULONG RevLevel;
    USHORT DefResUid;
    USHORT DefResGid;
        // Partial
} EXT2_SUPER_BLOCK, *PEXT2_SUPER_BLOCK;
#include <poppack.h>

C_ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, FreeInodesCount) == 0x10);
C_ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, BlocksPerGroup) == 0x20);
C_ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, WriteTime) == 0x30);
C_ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, LastCheck) == 0x40);
C_ASSERT(FIELD_OFFSET(EXT2_SUPER_BLOCK, DefResUid) == 0x50);

#define EXT2_SUPER_MAGIC 0xEF53
