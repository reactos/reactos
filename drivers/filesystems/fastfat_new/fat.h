/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Fat.h

Abstract:

    This module defines the on-disk structure of the Fat file system.


--*/

#ifndef _FAT_
#define _FAT_

//
//  The following nomenclature is used to describe the Fat on-disk
//  structure:
//
//      LBN - is the number of a sector relative to the start of the disk.
//
//      VBN - is the number of a sector relative to the start of a file,
//          directory, or allocation.
//
//      LBO - is a byte offset relative to the start of the disk.
//
//      VBO - is a byte offset relative to the start of a file, directory
//          or allocation.
//

typedef LONGLONG LBO;    /* for Fat32, LBO is >32 bits */

typedef LBO *PLBO;

typedef ULONG32 VBO;
typedef VBO *PVBO;


//
//  The boot sector is the first physical sector (LBN == 0) on the volume.
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in the
//  sector is packed (i.e., unaligned) so we'll supply a unpacking macro
//  to translate a packed BIOS into its unpacked equivalent.  The unpacked
//  BIOS structure is already defined in ntioapi.h so we only need to define
//  the packed BIOS.
//

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

typedef struct _PACKED_BIOS_PARAMETER_BLOCK_EX {
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
    UCHAR  LargeSectorsPerFat[4];                   // offset = 0x019 25
    UCHAR  ExtendedFlags[2];                        // offset = 0x01D 29
    UCHAR  FsVersion[2];                            // offset = 0x01F 31
    UCHAR  RootDirFirstCluster[4];                  // offset = 0x021 33
    UCHAR  FsInfoSector[2];                         // offset = 0x025 37
    UCHAR  BackupBootSector[2];                     // offset = 0x027 39
    UCHAR  Reserved[12];                            // offset = 0x029 41
} PACKED_BIOS_PARAMETER_BLOCK_EX;                   // sizeof = 0x035 53

typedef PACKED_BIOS_PARAMETER_BLOCK_EX *PPACKED_BIOS_PARAMETER_BLOCK_EX;

//
//  The IsBpbFat32 macro is defined to work with both packed and unpacked
//  BPB structures.  Since we are only checking for zero, the byte order
//  does not matter.
//

#define IsBpbFat32(bpb) (*(USHORT *)(&(bpb)->SectorsPerFat) == 0)

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
    ULONG32  HiddenSectors;
    ULONG32  LargeSectors;
    ULONG32  LargeSectorsPerFat;
    union {
        USHORT ExtendedFlags;
        struct {
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
//  This macro takes a Packed BIOS and fills in its Unpacked equivalent
//

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
    CopyUchar4(&(Bios)->LargeSectorsPerFat,&((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->LargeSectorsPerFat[0]  ); \
    CopyUchar2(&(Bios)->ExtendedFlags,     &((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->ExtendedFlags[0]       ); \
    CopyUchar2(&(Bios)->FsVersion,         &((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->FsVersion[0]           ); \
    CopyUchar4(&(Bios)->RootDirFirstCluster,                                \
                                           &((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->RootDirFirstCluster[0] ); \
    CopyUchar2(&(Bios)->FsInfoSector,      &((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->FsInfoSector[0]        ); \
    CopyUchar2(&(Bios)->BackupBootSector,  &((PPACKED_BIOS_PARAMETER_BLOCK_EX)Pbios)->BackupBootSector[0]    ); \
}

//
//  Define the boot sector
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
    UCHAR CurrentHead;                              // offset = 0x025  37
    UCHAR Signature;                                // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
} PACKED_BOOT_SECTOR;                               // sizeof = 0x03E  62

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

typedef struct _PACKED_BOOT_SECTOR_EX {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK_EX PackedBpb;       // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x040  64
    UCHAR CurrentHead;                              // offset = 0x041  65
    UCHAR Signature;                                // offset = 0x042  66
    UCHAR Id[4];                                    // offset = 0x043  67
    UCHAR VolumeLabel[11];                          // offset = 0x047  71
    UCHAR SystemId[8];                              // offset = 0x058  88
} PACKED_BOOT_SECTOR_EX;                            // sizeof = 0x060  96

typedef PACKED_BOOT_SECTOR_EX *PPACKED_BOOT_SECTOR_EX;

//
//  Define the FAT32 FsInfo sector.
//

typedef struct _FSINFO_SECTOR {
    ULONG SectorBeginSignature;                     // offset = 0x000   0
    UCHAR ExtraBootCode[480];                       // offset = 0x004   4
    ULONG FsInfoSignature;                          // offset = 0x1e4 484
    ULONG FreeClusterCount;                         // offset = 0x1e8 488
    ULONG NextFreeCluster;                          // offset = 0x1ec 492
    UCHAR Reserved[12];                             // offset = 0x1f0 496
    ULONG SectorEndSignature;                       // offset = 0x1fc 508
} FSINFO_SECTOR, *PFSINFO_SECTOR;

#define FSINFO_SECTOR_BEGIN_SIGNATURE   0x41615252
#define FSINFO_SECTOR_END_SIGNATURE     0xAA550000

#define FSINFO_SIGNATURE                0x61417272

//
//  We use the CurrentHead field for our dirty partition info.
//

#define FAT_BOOT_SECTOR_DIRTY            0x01
#define FAT_BOOT_SECTOR_TEST_SURFACE     0x02

//
//  Define a Fat Entry type.
//
//  This type is used when representing a fat table entry.  It also used
//  to be used when dealing with a fat table index and a count of entries,
//  but the ensuing type casting nightmare sealed this fate.  These other
//  two types are represented as ULONGs.
//

typedef ULONG32 FAT_ENTRY;

#define FAT32_ENTRY_MASK 0x0FFFFFFFUL

//
//  We use these special index values to set the dirty info for
//  DOS/Win9x compatibility.
//

#define FAT_CLEAN_VOLUME        (~FAT32_ENTRY_MASK | 0)
#define FAT_DIRTY_VOLUME        (~FAT32_ENTRY_MASK | 1)

#define FAT_DIRTY_BIT_INDEX     1

//
//  Physically, the entry is fully set if clean, and the high
//  bit knocked out if it is dirty (i.e., it is really a clean
//  bit).  This means it is different per-FAT size.
//

#define FAT_CLEAN_ENTRY         (~0)

#define FAT12_DIRTY_ENTRY       0x7ff
#define FAT16_DIRTY_ENTRY       0x7fff
#define FAT32_DIRTY_ENTRY       0x7fffffff

//
//  The following constants the are the valid Fat index values.
//

#define FAT_CLUSTER_AVAILABLE            (FAT_ENTRY)0x00000000
#define FAT_CLUSTER_RESERVED             (FAT_ENTRY)0x0ffffff0
#define FAT_CLUSTER_BAD                  (FAT_ENTRY)0x0ffffff7
#define FAT_CLUSTER_LAST                 (FAT_ENTRY)0x0fffffff

//
//  Fat files have the following time/date structures.  Note that the
//  following structure is a 32 bits long but USHORT aligned.
//

typedef struct _FAT_TIME {

    USHORT DoubleSeconds : 5;
    USHORT Minute        : 6;
    USHORT Hour          : 5;

} FAT_TIME;
typedef FAT_TIME *PFAT_TIME;

typedef struct _FAT_DATE {

    USHORT Day           : 5;
    USHORT Month         : 4;
    USHORT Year          : 7; // Relative to 1980

} FAT_DATE;
typedef FAT_DATE *PFAT_DATE;

typedef struct _FAT_TIME_STAMP {

    FAT_TIME Time;
    FAT_DATE Date;

} FAT_TIME_STAMP;
typedef FAT_TIME_STAMP *PFAT_TIME_STAMP;

//
//  Fat files have 8 character file names and 3 character extensions
//

typedef UCHAR FAT8DOT3[11];
typedef FAT8DOT3 *PFAT8DOT3;


//
//  The directory entry record exists for every file/directory on the
//  disk except for the root directory.
//

typedef struct _PACKED_DIRENT {
    FAT8DOT3       FileName;                         //  offset =  0
    UCHAR          Attributes;                       //  offset = 11
    UCHAR          NtByte;                           //  offset = 12
    UCHAR          CreationMSec;                     //  offset = 13
    FAT_TIME_STAMP CreationTime;                     //  offset = 14
    FAT_DATE       LastAccessDate;                   //  offset = 18
    union {
        USHORT     ExtendedAttributes;               //  offset = 20
        USHORT     FirstClusterOfFileHi;             //  offset = 20
    };
    FAT_TIME_STAMP LastWriteTime;                    //  offset = 22
    USHORT         FirstClusterOfFile;               //  offset = 26
    ULONG32        FileSize;                         //  offset = 28
} PACKED_DIRENT;                                     //  sizeof = 32
typedef PACKED_DIRENT *PPACKED_DIRENT;

//
//  A packed dirent is already quadword aligned so simply declare a dirent as a
//  packed dirent
//

typedef PACKED_DIRENT DIRENT;
typedef DIRENT *PDIRENT;

//
//  The first byte of a dirent describes the dirent.  There is also a routine
//  to help in deciding how to interpret the dirent.
//

#define FAT_DIRENT_NEVER_USED            0x00
#define FAT_DIRENT_REALLY_0E5            0x05
#define FAT_DIRENT_DIRECTORY_ALIAS       0x2e
#define FAT_DIRENT_DELETED               0xe5

//
//  Define the NtByte bits.
//

#define FAT_DIRENT_NT_BYTE_8_LOWER_CASE  0x08
#define FAT_DIRENT_NT_BYTE_3_LOWER_CASE  0x10

//
//  Define the various dirent attributes
//

#define FAT_DIRENT_ATTR_READ_ONLY        0x01
#define FAT_DIRENT_ATTR_HIDDEN           0x02
#define FAT_DIRENT_ATTR_SYSTEM           0x04
#define FAT_DIRENT_ATTR_VOLUME_ID        0x08
#define FAT_DIRENT_ATTR_DIRECTORY        0x10
#define FAT_DIRENT_ATTR_ARCHIVE          0x20
#define FAT_DIRENT_ATTR_DEVICE           0x40
#define FAT_DIRENT_ATTR_LFN              (FAT_DIRENT_ATTR_READ_ONLY | \
                                          FAT_DIRENT_ATTR_HIDDEN |    \
                                          FAT_DIRENT_ATTR_SYSTEM |    \
                                          FAT_DIRENT_ATTR_VOLUME_ID)


//
//  These macros convert a number of fields in the Bpb to bytes from sectors
//
//      ULONG
//      FatBytesPerCluster (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//
//      ULONG
//      FatBytesPerFat (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//
//      ULONG
//      FatReservedBytes (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//

#define FatBytesPerCluster(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerCluster))

#define FatBytesPerFat(B) (IsBpbFat32(B)?                           \
    ((ULONG)((B)->BytesPerSector * (B)->LargeSectorsPerFat)) :      \
    ((ULONG)((B)->BytesPerSector * (B)->SectorsPerFat)))

#define FatReservedBytes(B) ((ULONG)((B)->BytesPerSector * (B)->ReservedSectors))

//
//  This macro returns the size of the root directory dirent area in bytes
//  For Fat32, the root directory is variable in length.  This macro returns
//  0 because it is also used to determine the location of cluster 2.
//
//      ULONG
//      FatRootDirectorySize (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define FatRootDirectorySize(B) ((ULONG)((B)->RootEntries * sizeof(DIRENT)))


//
//  This macro returns the first Lbo (zero based) of the root directory on
//  the device.  This area is after the reserved and fats.
//
//  For Fat32, the root directory is moveable.  This macro returns the LBO
//  for cluster 2 because it is used to determine the location of cluster 2.
//  FatRootDirectoryLbo32() returns the actual LBO of the beginning of the
//  actual root directory.
//
//      LBO
//      FatRootDirectoryLbo (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define FatRootDirectoryLbo(B) (FatReservedBytes(B) + ((B)->Fats * FatBytesPerFat(B)))
#define FatRootDirectoryLbo32(B) (FatFileAreaLbo(B)+((B)->RootDirFirstCluster-2)*FatBytesPerCluster(B))

//
//  This macro returns the first Lbo (zero based) of the file area on the
//  the device.  This area is after the reserved, fats, and root directory.
//
//      LBO
//      FatFirstFileAreaLbo (
//          IN PBIOS_PARAMTER_BLOCK Bios
//          );
//

#define FatFileAreaLbo(B) (FatRootDirectoryLbo(B) + FatRootDirectorySize(B))

//
//  This macro returns the number of clusters on the disk.  This value is
//  computed by taking the total sectors on the disk subtracting up to the
//  first file area sector and then dividing by the sectors per cluster count.
//  Note that I don't use any of the above macros since far too much
//  superfluous sector/byte conversion would take place.
//
//      ULONG
//      FatNumberOfClusters (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

//
// for prior to MS-DOS Version 3.2
//
// After DOS 4.0, at least one of these, Sectors or LargeSectors, will be zero.
// but DOS version 3.2 case, both of these value might contains some value,
// because, before 3.2, we don't have Large Sector entry, some disk might have
// unexpected value in the field, we will use LargeSectors if Sectors eqaul to zero.
//

#define FatNumberOfClusters(B) (                                         \
                                                                         \
  IsBpbFat32(B) ?                                                        \
                                                                         \
    ((((B)->Sectors ? (B)->Sectors : (B)->LargeSectors)                  \
                                                                         \
        -   ((B)->ReservedSectors +                                      \
             (B)->Fats * (B)->LargeSectorsPerFat ))                      \
                                                                         \
                                    /                                    \
                                                                         \
                        (B)->SectorsPerCluster)                          \
  :                                                                      \
    ((((B)->Sectors ? (B)->Sectors : (B)->LargeSectors)                  \
                                                                         \
        -   ((B)->ReservedSectors +                                      \
             (B)->Fats * (B)->SectorsPerFat +                            \
             (B)->RootEntries * sizeof(DIRENT) / (B)->BytesPerSector ) ) \
                                                                         \
                                    /                                    \
                                                                         \
                        (B)->SectorsPerCluster)                          \
)

//
//  This macro returns the fat table bit size (i.e., 12 or 16 bits)
//
//      ULONG
//      FatIndexBitSize (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define FatIndexBitSize(B)  \
    ((UCHAR)(IsBpbFat32(B) ? 32 : (FatNumberOfClusters(B) < 4087 ? 12 : 16)))

//
//  This macro raises STATUS_FILE_CORRUPT and marks the Fcb bad if an
//  index value is not within the proper range.
//  Note that the first two index values are invalid (0, 1), so we must
//  add two from the top end to make sure the everything is within range
//
//      VOID
//      FatVerifyIndexIsValid (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN ULONG Index
//          );
//

#define FatVerifyIndexIsValid(IC,V,I) {                                       \
    if (((I) < 2) || ((I) > ((V)->AllocationSupport.NumberOfClusters + 1))) { \
        FatRaiseStatus(IC,STATUS_FILE_CORRUPT_ERROR);                         \
    }                                                                         \
}

//
//  These two macros are used to translate between Logical Byte Offsets,
//  and fat entry indexes.  Note the use of variables stored in the Vcb.
//  These two macros are used at a higher level than the other macros
//  above.
//
//  Note, these indexes are true cluster numbers.
//
//  LBO
//  GetLboFromFatIndex (
//      IN FAT_ENTRY Fat_Index,
//      IN PVCB Vcb
//      );
//
//  FAT_ENTRY
//  GetFatIndexFromLbo (
//      IN LBO Lbo,
//      IN PVCB Vcb
//      );
//

#define FatGetLboFromIndex(VCB,FAT_INDEX) (                                       \
    ( (LBO)                                                                       \
        (VCB)->AllocationSupport.FileAreaLbo +                                    \
        (((LBO)((FAT_INDEX) - 2)) << (VCB)->AllocationSupport.LogOfBytesPerCluster) \
    )                                                                             \
)

#define FatGetIndexFromLbo(VCB,LBO) (                      \
    (ULONG) (                                              \
        (((LBO) - (VCB)->AllocationSupport.FileAreaLbo) >> \
        (VCB)->AllocationSupport.LogOfBytesPerCluster) + 2 \
    )                                                      \
)

//
//  The following macro does the shifting and such to lookup an entry
//
//  VOID
//  FatLookup12BitEntry(
//      IN PVOID Fat,
//      IN FAT_ENTRY Index,
//      OUT PFAT_ENTRY Entry
//      );
//

#define FatLookup12BitEntry(FAT,INDEX,ENTRY) {                              \
                                                                            \
    CopyUchar2((PUCHAR)(ENTRY), (PUCHAR)(FAT) + (INDEX) * 3 / 2);           \
                                                                            \
    *ENTRY = (FAT_ENTRY)(0xfff & (((INDEX) & 1) ? (*(ENTRY) >> 4) :         \
                                                   *(ENTRY)));              \
}

//
//  The following macro does the tmp shifting and such to store an entry
//
//  VOID
//  FatSet12BitEntry(
//      IN PVOID Fat,
//      IN FAT_ENTRY Index,
//      IN FAT_ENTRY Entry
//      );
//

#define FatSet12BitEntry(FAT,INDEX,ENTRY) {                            \
                                                                       \
    FAT_ENTRY TmpFatEntry;                                             \
                                                                       \
    CopyUchar2((PUCHAR)&TmpFatEntry, (PUCHAR)(FAT) + (INDEX) * 3 / 2); \
                                                                       \
    TmpFatEntry = (FAT_ENTRY)                                          \
                (((INDEX) & 1) ? ((ENTRY) << 4) | (TmpFatEntry & 0xf)  \
                               : (ENTRY) | (TmpFatEntry & 0xf000));    \
                                                                       \
    *((UNALIGNED UCHAR2 *)((PUCHAR)(FAT) + (INDEX) * 3 / 2)) = *((UNALIGNED UCHAR2 *)(&TmpFatEntry)); \
}

//
//  The following macro compares two FAT_TIME_STAMPs
//

#define FatAreTimesEqual(TIME1,TIME2) (                     \
    RtlEqualMemory((TIME1),(TIME2), sizeof(FAT_TIME_STAMP)) \
)


#define EA_FILE_SIGNATURE                (0x4445) // "ED"
#define EA_SET_SIGNATURE                 (0x4145) // "EA"

//
//  If the volume contains any ea data then there is one EA file called
//  "EA DATA. SF" located in the root directory as Hidden, System and
//  ReadOnly.
//

typedef struct _EA_FILE_HEADER {
    USHORT Signature;           // offset = 0
    USHORT FormatType;          // offset = 2
    USHORT LogType;             // offset = 4
    USHORT Cluster1;            // offset = 6
    USHORT NewCValue1;          // offset = 8
    USHORT Cluster2;            // offset = 10
    USHORT NewCValue2;          // offset = 12
    USHORT Cluster3;            // offset = 14
    USHORT NewCValue3;          // offset = 16
    USHORT Handle;              // offset = 18
    USHORT NewHOffset;          // offset = 20
    UCHAR  Reserved[10];        // offset = 22
    USHORT EaBaseTable[240];    // offset = 32
} EA_FILE_HEADER;               // sizeof = 512

typedef EA_FILE_HEADER *PEA_FILE_HEADER;

typedef USHORT EA_OFF_TABLE[128];

typedef EA_OFF_TABLE *PEA_OFF_TABLE;

//
//  Every file with an extended attribute contains in its dirent an index
//  into the EaMapTable.  The map table contains an offset within the ea
//  file (cluster aligned) of the ea data for the file.  The individual
//  ea data for each file is prefaced with an Ea Data Header.
//

typedef struct _EA_SET_HEADER {
    USHORT Signature;           // offset = 0
    USHORT OwnEaHandle;         // offset = 2
    ULONG32  NeedEaCount;         // offset = 4
    UCHAR  OwnerFileName[14];   // offset = 8
    UCHAR  Reserved[4];         // offset = 22
    UCHAR  cbList[4];           // offset = 26
    UCHAR  PackedEas[1];        // offset = 30
} EA_SET_HEADER;                // sizeof = 30
typedef EA_SET_HEADER *PEA_SET_HEADER;

#define SIZE_OF_EA_SET_HEADER       30

#define MAXIMUM_EA_SIZE             0x0000ffff

#define GetcbList(EASET) (((EASET)->cbList[0] <<  0) + \
                          ((EASET)->cbList[1] <<  8) + \
                          ((EASET)->cbList[2] << 16) + \
                          ((EASET)->cbList[3] << 24))

#define SetcbList(EASET,CB) {                \
    (EASET)->cbList[0] = (CB >>  0) & 0x0ff; \
    (EASET)->cbList[1] = (CB >>  8) & 0x0ff; \
    (EASET)->cbList[2] = (CB >> 16) & 0x0ff; \
    (EASET)->cbList[3] = (CB >> 24) & 0x0ff; \
}

//
//  Every individual ea in an ea set is declared the following packed ea
//

typedef struct _PACKED_EA {
    UCHAR Flags;
    UCHAR EaNameLength;
    UCHAR EaValueLength[2];
    CHAR  EaName[1];
} PACKED_EA;
typedef PACKED_EA *PPACKED_EA;

//
//  The following two macros are used to get and set the ea value length
//  field of a packed ea
//
//      VOID
//      GetEaValueLength (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT ValueLength
//          );
//
//      VOID
//      SetEaValueLength (
//          IN PPACKED_EA Ea,
//          IN USHORT ValueLength
//          );
//

#define GetEaValueLength(EA,LEN) {               \
    *(LEN) = 0;                                  \
    CopyUchar2( (LEN), (EA)->EaValueLength );    \
}

#define SetEaValueLength(EA,LEN) {               \
    CopyUchar2( &((EA)->EaValueLength), (LEN) ); \
}

//
//  The following macro is used to get the size of a packed ea
//
//      VOID
//      SizeOfPackedEa (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT EaSize
//          );
//

#define SizeOfPackedEa(EA,SIZE) {          \
    ULONG _NL,_DL; _NL = 0; _DL = 0;       \
    CopyUchar1(&_NL, &(EA)->EaNameLength); \
    GetEaValueLength(EA, &_DL);            \
    *(SIZE) = 1 + 1 + 2 + _NL + 1 + _DL;   \
}

#define EA_NEED_EA_FLAG                 0x80
#define MIN_EA_HANDLE                   1
#define MAX_EA_HANDLE                   30719
#define UNUSED_EA_HANDLE                0xffff
#define EA_CBLIST_OFFSET                0x1a
#define MAX_EA_BASE_INDEX               240
#define MAX_EA_OFFSET_INDEX             128


#endif // _FAT_

