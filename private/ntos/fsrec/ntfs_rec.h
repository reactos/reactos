/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntfs_rec.h

Abstract:

    This module contains the mini-file system recognizer for NTFS.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

//
//  The fundamental unit of allocation on an Ntfs volume is the
//  cluster.  Format guarantees that the cluster size is an integral
//  power of two times the physical sector size of the device.  Ntfs
//  reserves 64-bits to describe a cluster, in order to support
//  large disks.  The LCN represents a physical cluster number on
//  the disk, and the VCN represents a virtual cluster number within
//  an attribute.
//

typedef LARGE_INTEGER LCN;
typedef LCN *PLCN;

typedef LARGE_INTEGER VCN;
typedef VCN *PVCN;

typedef LARGE_INTEGER LBO;
typedef LBO *PLBO;

typedef LARGE_INTEGER VBO;
typedef VBO *PVBO;

//
//  The boot sector is duplicated on the partition.  The first copy
//  is on the first physical sector (LBN == 0) of the partition, and
//  the second copy is at <number sectors on partition> / 2.  If the
//  first copy can not be read when trying to mount the disk, the
//  second copy may be read and has the identical contents.  Format
//  must figure out which cluster the second boot record belongs in,
//  and it must zero all of the other sectors that happen to be in
//  the same cluster.  The boot file minimally contains with two
//  clusters, which are the two clusters which contain the copies of
//  the boot record.  If format knows that some system likes to put
//  code somewhere, then it should also align this requirement to
//  even clusters, and add that to the boot file as well.
//
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in
//  the sector is packed (i.e., unaligned) so we'll supply an
//  unpacking macro to translate a packed BIOS into its unpacked
//  equivalent.  The unpacked BIOS structure is already defined in
//  ntioapi.h so we only need to define the packed BIOS.
//

//
//  Define the Packed and Unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {

    UCHAR  BytesPerSector[2];                               //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                            //  offset = 0x002
    UCHAR  ReservedSectors[2];                              //  offset = 0x003 (zero)
    UCHAR  Fats[1];                                         //  offset = 0x005 (zero)
    UCHAR  RootEntries[2];                                  //  offset = 0x006 (zero)
    UCHAR  Sectors[2];                                      //  offset = 0x008 (zero)
    UCHAR  Media[1];                                        //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                                //  offset = 0x00B (zero)
    UCHAR  SectorsPerTrack[2];                              //  offset = 0x00D
    UCHAR  Heads[2];                                        //  offset = 0x00F
    UCHAR  HiddenSectors[4];                                //  offset = 0x011 (zero)
    UCHAR  LargeSectors[4];                                 //  offset = 0x015 (zero)

} PACKED_BIOS_PARAMETER_BLOCK;                              //  sizeof = 0x019

typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

//
//  Define the boot sector.  Note that MFT2 is exactly three file
//  record segments long, and it mirrors the first three file record
//  segments from the MFT, which are MFT, MFT2 and the Log File.
//
//  The Oem field contains the ASCII characters "NTFS    ".
//
//  The Checksum field is a simple additive checksum of all of the
//  ULONGs which precede the Checksum ULONG.  The rest of the sector
//  is not included in this Checksum.
//

typedef struct _PACKED_BOOT_SECTOR {

    UCHAR Jump[3];                                                  //  offset = 0x000
    UCHAR Oem[8];                                                   //  offset = 0x003
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;                          //  offset = 0x00B
    UCHAR Unused[4];                                                //  offset = 0x024
    LARGE_INTEGER NumberSectors;                                    //  offset = 0x028
    LCN MftStartLcn;                                                //  offset = 0x030
    LCN Mft2StartLcn;                                               //  offset = 0x038
    CHAR ClustersPerFileRecordSegment;                              //  offset = 0x040
    UCHAR Reserved0[3];
    CHAR DefaultClustersPerIndexAllocationBuffer;                   //  offset = 0x044
    UCHAR Reserved1[3];
    LARGE_INTEGER SerialNumber;                                     //  offset = 0x048
    ULONG Checksum;                                                 //  offset = 0x050
    UCHAR BootStrap[0x200-0x054];                                   //  offset = 0x054

} PACKED_BOOT_SECTOR;                                               //  sizeof = 0x200

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

//
// Define the functions provided by this driver.
//

BOOLEAN
IsNtfsVolume(
    IN PPACKED_BOOT_SECTOR BootSector,
    IN ULONG BytesPerSector,
    IN PLARGE_INTEGER NumberOfSectors
    );

