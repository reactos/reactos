#ifndef __FAT_H__
#define __FAT_H__

//
//  Might be a good idea to have this as a shared
//  header with FS Recognizer.
//
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

#define CopyUchar1(Dst,Src) {                          \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
}

#define CopyUchar2(Dst,Src) {                          \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
}

#define CopyUchar4(Dst,Src) {                          \
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
//  Packed versions of the BPB and Boot Sector
//
typedef struct _PACKED_BIOS_PARAMETER_BLOCK
{
    UCHAR	BytesPerSector[2];
    UCHAR	SectorsPerCluster[1];
    UCHAR	ReservedSectors[2];
    UCHAR	Fats[1];
    UCHAR	RootEntries[2];
    UCHAR	Sectors[2];
    UCHAR	Media[1];
    UCHAR	SectorsPerFat[2];
    UCHAR	SectorsPerTrack[2];
    UCHAR	Heads[2];
    UCHAR	HiddenSectors[4];
    UCHAR	LargeSectors[4];
} PACKED_BIOS_PARAMETER_BLOCK, *PPACKED_BIOS_PARAMETER_BLOCK;
//  sizeof = 0x019

typedef struct _PACKED_BIOS_PARAMETER_BLOCK_EX
{
	PACKED_BIOS_PARAMETER_BLOCK Block;
	UCHAR	LargeSectorsPerFat[4];
	UCHAR	ExtendedFlags[2];
	UCHAR	FsVersion[2];
	UCHAR	RootDirFirstCluster[4];
	UCHAR	FsInfoSector[2];
	UCHAR	BackupBootSector[2];
	UCHAR	Reserved[12];
} PACKED_BIOS_PARAMETER_BLOCK_EX, *PPACKED_BIOS_PARAMETER_BLOCK_EX;
//  sizeof = 0x035 53

//
//  Unpacked version of the BPB
//
typedef struct BIOS_PARAMETER_BLOCK
{
    USHORT	BytesPerSector;
    UCHAR	SectorsPerCluster;
    USHORT	ReservedSectors;
    UCHAR	Fats;
    USHORT	RootEntries;
    USHORT	Sectors;
    UCHAR	Media;
    USHORT	SectorsPerFat;
    USHORT	SectorsPerTrack;
    USHORT	Heads;
    ULONG	HiddenSectors;
    ULONG	LargeSectors;
    ULONG	LargeSectorsPerFat;
    union
    {
        USHORT	ExtendedFlags;
        struct
        {
            ULONG	ActiveFat:4;
            ULONG	Reserved0:3;
            ULONG	MirrorDisabled:1;
            ULONG	Reserved1:8;
        };
    };
    USHORT	FsVersion;
    ULONG	RootDirFirstCluster;
    USHORT	FsInfoSector;
    USHORT	BackupBootSector;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

#define FatValidBytesPerSector(xBytes) \
	(!((xBytes) & ((xBytes)-1)) && (xBytes)>=0x80 && (xBytes)<=0x1000)

#define FatValidSectorsPerCluster(xSectors) \
	(!((xSectors) & ((xSectors)-1)) && (xSectors)>=0 && (xSectors)<=0x80)

typedef struct _PACKED_BOOT_SECTOR
{
    UCHAR	Jump[3];
    UCHAR	Oem[8];
    PACKED_BIOS_PARAMETER_BLOCK	PackedBpb;
    UCHAR	PhysicalDriveNumber;
    UCHAR	CurrentHead;
    UCHAR	Signature;
    UCHAR	Id[4];
    UCHAR	VolumeLabel[11];
    UCHAR	SystemId[8];
} PACKED_BOOT_SECTOR, *PPACKED_BOOT_SECTOR;
//  sizeof = 0x03E

typedef struct _PACKED_BOOT_SECTOR_EX
{
	UCHAR	Jump[3];
	UCHAR	Oem[8];
	PACKED_BIOS_PARAMETER_BLOCK_EX PackedBpb;
	UCHAR	PhysicalDriveNumber;
	UCHAR	CurrentHead;
	UCHAR	Signature;
	UCHAR	Id[4];
	UCHAR	VolumeLabel[11];
	UCHAR	SystemId[8];
} PACKED_BOOT_SECTOR_EX, *PPACKED_BOOT_SECTOR_EX;
//  sizeof = 0x060

#define FatBootSectorJumpValid(xMagic) \
	((xMagic)[0] == 0xe9 || (xMagic)[0] == 0xeb || (xMagic)[0] == 0x49)

typedef struct _FSINFO_SECTOR
{
	ULONG	SectorBeginSignature;
	UCHAR	Reserved[480];
	ULONG	FsInfoSignature;
	ULONG	FreeClusterCount;
	ULONG	NextFreeCluster;
	UCHAR	Reserved0[12];
	ULONG	SectorEndSignature;
} FSINFO_SECTOR, *PFSINFO_SECTOR;
//  sizeof = 0x200
#define FSINFO_SECTOR_BEGIN_SIGNATURE   0x41615252
#define FSINFO_SECTOR_END_SIGNATURE     0xaa550000
#define FSINFO_SIGNATURE                0x61417272
//
//  Cluster Markers:
//
//
#define FAT_CLUSTER_AVAILABLE			0x00000000
#define FAT_CLUSTER_RESERVED			0x0ffffff0
#define FAT_CLUSTER_BAD					0x0ffffff7
#define FAT_CLUSTER_LAST				0x0fffffff
//
//  Directory Structure:
//
typedef struct _FAT_TIME
{
    union {
        struct {
            USHORT DoubleSeconds : 5;
            USHORT Minute : 6;
            USHORT Hour : 5;
        };
        USHORT Value;
    };
} FAT_TIME, *PFAT_TIME;
//
//
//
typedef struct _FAT_DATE {
    union {
        struct {
            USHORT Day : 5;
            USHORT Month : 4;
            /* Relative to 1980 */
            USHORT Year : 7;
        };
        USHORT Value;
    };
} FAT_DATE, *PFAT_DATE;
//
//
//
typedef struct _FAT_DATETIME {
    union {
        struct {
            FAT_TIME    Time;
            FAT_DATE    Date;
        };
        ULONG Value;
    };
} FAT_DATETIME, *PFAT_DATETIME;
//
//
//
typedef struct _DIR_ENTRY
{
	UCHAR FileName[11];
	UCHAR Attributes;
	UCHAR Case;
	UCHAR CreationTimeTenMs;
    FAT_DATETIME CreationDateTime;
    FAT_DATE LastAccessDate;
	union {
		USHORT	ExtendedAttributes;
		USHORT	FirstClusterOfFileHi;
	};
    FAT_DATETIME LastWriteDateTime;
	USHORT		FirstCluster;
	ULONG		FileSize;
} DIR_ENTRY, *PDIR_ENTRY;
//  sizeof = 0x020

typedef struct _LONG_FILE_NAME_ENTRY {
    UCHAR       Index;
    UCHAR       NameA[10];
    UCHAR       Attributes;
    UCHAR       Type;
    UCHAR       Checksum;
    USHORT      NameB[6];
    USHORT      Reserved;
    USHORT      NameC[2];
} LONG_FILE_NAME_ENTRY, *PLONG_FILE_NAME_ENTRY;
//  sizeof = 0x020

#define FAT_FN_DIR_ENTRY_TERM_INDEX 0x40

#define FAT_BYTES_PER_DIRENT        0x20
#define FAT_BYTES_PER_DIRENT_LOG    0x05
#define FAT_DIRENT_NEVER_USED       0x00
#define FAT_DIRENT_REALLY_0E5       0x05
#define FAT_DIRENT_DIRECTORY_ALIAS  0x2e
#define FAT_DIRENT_DELETED          0xe5

#define FAT_CASE_LOWER_BASE	        0x08
#define FAT_CASE_LOWER_EXT 	        0x10

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

typedef struct _PACKED_LFN_DIRENT {
    UCHAR     Ordinal;    //  offset =  0
    UCHAR     Name1[10];  //  offset =  1 (Really 5 chars, but not WCHAR aligned)
    UCHAR     Attributes; //  offset = 11
    UCHAR     Type;       //  offset = 12
    UCHAR     Checksum;   //  offset = 13
    WCHAR     Name2[6];   //  offset = 14
    USHORT    MustBeZero; //  offset = 26
    WCHAR     Name3[2];   //  offset = 28
} PACKED_LFN_DIRENT;      //  sizeof = 32

#endif//__FAT_H__
