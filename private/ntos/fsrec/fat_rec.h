/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fat_rec.h

Abstract:

    This module contains the mini-file system recognizer for FAT.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }

//
//  Define the Packed and Unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       // offset = 0x000  0
    UCHAR  SectorsPerCluster[1];                    // offset = 0x002  2
    UCHAR  ReservedSectors[2];                      // offset = 0x003  3
    UCHAR  Fats[1];                                 // offset = 0x005  5
    UCHAR  RootEntries[2];                          // offset = 0x006  6
    UCHAR  Sectors[2];                              // offset = 0x008  8
    UCHAR  Media[1];                                // offset = 0x00A 10
    UCHAR  SectorsPerFat[2];                        // offset = 0x00B 11
    UCHAR  SectorsPerTrack[2];                      // offset = 0x00D 13
    UCHAR  Heads[2];                                // offset = 0x00F 15
    UCHAR  HiddenSectors[4];                        // offset = 0x011 17
    UCHAR  LargeSectors[4];                         // offset = 0x015 21
} PACKED_BIOS_PARAMETER_BLOCK;                      // sizeof = 0x019 25

typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
//  Define the boot sector
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
    UCHAR Reserved;                                 // offset = 0x025  37
    UCHAR Signature;                                // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
} PACKED_BOOT_SECTOR;                               // sizeof = 0x03E  62

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

//
// Define the functions provided by this driver.
//

BOOLEAN
IsFatVolume(
    IN PPACKED_BOOT_SECTOR Buffer
    );

VOID
UnpackBiosParameterBlock(
    IN PPACKED_BIOS_PARAMETER_BLOCK Bios,
    OUT PBIOS_PARAMETER_BLOCK UnpackedBios
    );
