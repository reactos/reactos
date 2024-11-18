/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/fs_rec.h
 * PURPOSE:          Main Header File
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

#ifndef _FS_REC_H
#define _FS_REC_H

#include <ntifs.h>

/* Tag for memory allocations */
#define FSREC_TAG 'cRsF'

/* UDFS Offsets */
#define UDFS_VRS_START_OFFSET  32768
#define UDFS_AVDP_SECTOR       256

/* Non-standard rounding macros */
#ifndef ROUND_UP
#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))
#endif

/* Conversion types and macros taken from internal ntifs headers */
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

/* Packed versions of the BPB and Boot Sector */
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

/* Unpacked version of the BPB */
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

/* UDFS Structures */
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

/* Filesystem Types */
typedef enum _FILE_SYSTEM_TYPE
{
    FS_TYPE_UNUSED,
    FS_TYPE_VFAT,
    FS_TYPE_NTFS,
    FS_TYPE_CDFS,
    FS_TYPE_UDFS,
    FS_TYPE_EXT,
    FS_TYPE_BTRFS,
    FS_TYPE_REISERFS,
    FS_TYPE_FFS,
    FS_TYPE_FATX,
} FILE_SYSTEM_TYPE, *PFILE_SYSTEM_TYPE;

/* FS Recognizer State */
typedef enum _FS_REC_STATE
{
    Pending,
    Loaded,
    Unloading
} FS_REC_STATE, *PFS_REC_STATE;

/* Device extension */
typedef struct _DEVICE_EXTENSION
{
    FS_REC_STATE State;
    FILE_SYSTEM_TYPE FsType;
    PDEVICE_OBJECT Alternate;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Prototypes */
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
FsRecExtFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecBtrfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecReiserfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecFfsFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
FsRecFatxFsControl(
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

#endif /* _FS_REC_H */
