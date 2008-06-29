/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/fs_rec.h
 * PURPOSE:          Main Header File
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

//
// IFS Headers
//
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include "helper.h"

//
// Tag for memory allocations
//
#define FSREC_TAG TAG('F', 's', 'R', 'c')

//
// UDFS Offsets
//
#define UDFS_VRS_START_SECTOR   16
#define UDFS_AVDP_SECTOR       256

//
// Ext2 Magic
// Taken from Linux Kernel
//
#define EXT2_SUPER_MAGIC	   0xEF53

//
// Conversion types and macros taken from internal ntifs headers
//
typedef union _UCHAR1
{
    UCHAR Uchar[1];
    UCHAR ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2
{
    UCHAR Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4
{
    UCHAR Uchar[4];
    ULONG ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
}

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
}

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
}

#define FatUnpackBios(Bios,Pbios) {                                         \
    CopyUchar2(&(Bios)->BytesPerSector,    &(Pbios)->BytesPerSector[0]   ); \
    CopyUchar1(&(Bios)->SectorsPerCluster, &(Pbios)->SectorsPerCluster[0]); \
    CopyUchar2(&(Bios)->ReservedSectors,   &(Pbios)->ReservedSectors[0]  ); \
    CopyUchar1(&(Bios)->Fats,              &(Pbios)->Fats[0]             ); \
    CopyUchar2(&(Bios)->RootEntries,       &(Pbios)->RootEntries[0]      ); \
    CopyUchar2(&(Bios)->Sectors,           &(Pbios)->Sectors[0]          ); \
    CopyUchar1(&(Bios)->Media,             &(Pbios)->Media[0]            ); \
    CopyUchar2(&(Bios)->SectorsPerFat,     &(Pbios)->SectorsPerFat[0]    ); \
    CopyUchar2(&(Bios)->SectorsPerTrack,   &(Pbios)->SectorsPerTrack[0]  ); \
    CopyUchar2(&(Bios)->Heads,             &(Pbios)->Heads[0]            ); \
    CopyUchar4(&(Bios)->HiddenSectors,     &(Pbios)->HiddenSectors[0]    ); \
    CopyUchar4(&(Bios)->LargeSectors,      &(Pbios)->LargeSectors[0]     ); \
}

//
// Packed versions of the BPB and Boot Sector
//
typedef struct _PACKED_BIOS_PARAMETER_BLOCK
{
    UCHAR BytesPerSector[2];
    UCHAR SectorsPerCluster[1];
    UCHAR ReservedSectors[2];
    UCHAR Fats[1];
    UCHAR RootEntries[2];
    UCHAR Sectors[2];
    UCHAR Media[1];
    UCHAR SectorsPerFat[2];
    UCHAR SectorsPerTrack[2];
    UCHAR Heads[2];
    UCHAR HiddenSectors[4];
    UCHAR LargeSectors[4];
} PACKED_BIOS_PARAMETER_BLOCK, *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct _PACKED_BOOT_SECTOR
{
    UCHAR Jump[3];
    UCHAR Oem[8];
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;
    UCHAR PhysicalDriveNumber;
    UCHAR CurrentHead;
    UCHAR Signature;
    UCHAR Id[4];
    UCHAR VolumeLabel[11];
    UCHAR SystemId[8];
} PACKED_BOOT_SECTOR, *PPACKED_BOOT_SECTOR;

//
// Unpacked version of the BPB
//
typedef struct BIOS_PARAMETER_BLOCK
{
    USHORT BytesPerSector;
    UCHAR SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG32 HiddenSectors;
    ULONG32 LargeSectors;
    ULONG32 LargeSectorsPerFat;
    union
    {
        USHORT ExtendedFlags;
        struct
        {
            ULONG ActiveFat:4;
            ULONG Reserved0:3;
            ULONG MirrorDisabled:1;
            ULONG Reserved1:8;
        };
    };
    USHORT FsVersion;
    ULONG32 RootDirFirstCluster;
    USHORT FsInfoSector;
    USHORT BackupBootSector;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
// UDFS Structures
//
#include <pshpack1.h>
typedef struct _TAG
{
    USHORT Identifier;
    USHORT Version;
    UCHAR  Checksum;
    UCHAR  Reserved;
    USHORT SerialNumber;
    USHORT Crc;
    USHORT CrcLength;
    ULONG  Location;
} TAG, *PTAG;

typedef struct _EXTENT
{
    ULONG Length;
    ULONG Location;
} EXTENT, *PEXTENT;

typedef struct _AVDP
{
    TAG DescriptorTag;
    EXTENT MainVolumeDescriptorExtent;
    EXTENT ReserveVolumeDescriptorExtent;
} AVDP, *PAVDP;
#include <poppack.h>

//
// Ext2 Structure
// Taken from Linux Kernel and adapted for ReactOS
//
/*
 * Structure of the super block
 */
typedef struct _ext2_super_block
{
    ULONG   s_inodes_count;           /* Inodes count */
    ULONG   s_blocks_count;           /* Blocks count */
    ULONG   s_r_blocks_count;         /* Reserved blocks count */
    ULONG   s_free_blocks_count;      /* Free blocks count */
    ULONG   s_free_inodes_count;      /* Free inodes count */
    ULONG   s_first_data_block;       /* First Data Block */
    ULONG   s_log_block_size;         /* Block size */
    LONG    s_log_frag_size;          /* Fragment size */
    ULONG   s_blocks_per_group;       /* # Blocks per group */
    ULONG   s_frags_per_group;        /* # Fragments per group */
    ULONG   s_inodes_per_group;       /* # Inodes per group */
    ULONG   s_mtime;                  /* Mount time */
    ULONG   s_wtime;                  /* Write time */
    USHORT  s_mnt_count;              /* Mount count */
    SHORT   s_max_mnt_count;          /* Maximal mount count */
    USHORT  s_magic;                  /* Magic signature */
    USHORT  s_state;                  /* File system state */
    USHORT  s_errors;                 /* Behaviour when detecting errors */
    USHORT  s_minor_rev_level;        /* minor revision level */
    ULONG   s_lastcheck;              /* time of last check */
    ULONG   s_checkinterval;          /* max. time between checks */
    ULONG   s_creator_os;             /* OS */
    ULONG   s_rev_level;              /* Revision level */
    USHORT  s_def_resuid;             /* Default uid for reserved blocks */
    USHORT  s_def_resgid;             /* Default gid for reserved blocks */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     * 
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    ULONG   s_first_ino;              /* First non-reserved inode */
    USHORT  s_inode_size;             /* size of inode structure */
    USHORT  s_block_group_nr;         /* block group # of this superblock */
    ULONG   s_feature_compat;         /* compatible feature set */
    ULONG   s_feature_incompat;       /* incompatible feature set */
    ULONG   s_feature_ro_compat;      /* readonly-compatible feature set */
    UCHAR   s_uuid[16];               /* 128-bit uuid for volume */
    char    s_volume_name[16];        /* volume name */
    char    s_last_mounted[64];       /* directory where last mounted */
    ULONG   s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    UCHAR   s_prealloc_blocks;        /* Nr of blocks to try to preallocate*/
    UCHAR   s_prealloc_dir_blocks;    /* Nr to preallocate for dirs */
    USHORT  s_padding1;
    ULONG   s_reserved[204];          /* Padding to the end of the block */
} ext2_super_block, *pext2_super_block;

//
// Filesystem Types
//
typedef enum _FILE_SYSTEM_TYPE
{
    FS_TYPE_UNUSED,
    FS_TYPE_VFAT,
    FS_TYPE_NTFS,
    FS_TYPE_CDFS,
    FS_TYPE_UDFS,
    FS_TYPE_EXT2,
} FILE_SYSTEM_TYPE, *PFILE_SYSTEM_TYPE;

//
// FS Recognizer State
//
typedef enum _FS_REC_STATE
{
    Pending,
    Loaded,
    Unloading
} FS_REC_STATE, *PFS_REC_STATE;

//
// Device extension
//
typedef struct _DEVICE_EXTENSION
{
    FS_REC_STATE State;
    FILE_SYSTEM_TYPE FsType;
    PDEVICE_OBJECT Alternate;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Prototypes
//
NTSTATUS
NTAPI
FsRecCdfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecVfatFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecNtfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecUdfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecExt2FsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

BOOLEAN
NTAPI
FsRecGetDeviceSectors(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    OUT PLARGE_INTEGER SectorCount
);

BOOLEAN
NTAPI
FsRecGetDeviceSectorSize(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG SectorSize
);

BOOLEAN
NTAPI
FsRecReadBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLARGE_INTEGER Offset,
    IN ULONG Length,
    IN ULONG SectorSize,
    IN OUT PVOID *Buffer,
    OUT PBOOLEAN DeviceError OPTIONAL
);

NTSTATUS
NTAPI
FsRecLoadFileSystem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR DriverServiceName
);
