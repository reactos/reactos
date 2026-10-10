/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.h
 * PURPOSE:     NTFS lib definitions
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

#ifndef NTFSLIB_H
#define NTFSLIB_H


/* INCLUDES ******************************************************************/

#include <stdlib.h>

#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umtypes.h>

#include <fmifs/fmifs.h>


/* OTHER MACROSES ************************************************************/

#define KeQuerySystemTime(t)  NtfsGetSystemTimeAsFileTime((LPFILETIME)(t));

#define FREE(p) if (p) RtlFreeHeap(RtlGetProcessHeap(), 0, p);

#define FIRST_ATTRIBUTE(fr) ((PATTR_RECORD)((ULONG_PTR)fr + fr->FirstAttributeOffset))
#define NEXT_ATTRIBUTE(attr) ((PATTR_RECORD)((ULONG_PTR)(attr) + (attr)->Length))

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))


/* BYTES MACROSES ************************************************************/

#define MB_TO_B(x) (x * 1024)

#define GET_BYTE(val, n) (((val) >> (8*(n))) & 0xFF)

#define GET_BYTE_FROM_END(val, n) (((val) >> (8*(sizeof((val)) - 1 - (n)))) & 0xFF)

#define BSWAP16(val) \
 ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

#define BSWAP32(val) \
 ( (((val) >> 24) & 0x000000FF) | (((val) >>  8) & 0x0000FF00) | \
   (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )

#define BSWAP64(val) \
 ( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
   (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | \
   (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
   (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )


/* DISK MACROSES *************************************************************/

#define DISK_HANDLE (NtfsFormatData.DiskHandle)
#define DISK_GEO    (NtfsFormatData.DiskGeometry)
#define DISK_LEN    (NtfsFormatData.LengthInformation)
#define LABEL       (NtfsFormatData.Label)
#define LAYOUT      (NtfsFormatData.Layout)

#define BYTES_PER_SECTOR    ((ULONG)DISK_GEO->BytesPerSector)
#define SECTORS_PER_TRACK   ((ULONG)DISK_GEO->SectorsPerTrack)

// The following are computed once by ComputeLayout() and cached in LAYOUT.
#define SECTORS_PER_CLUSTER (LAYOUT.SectorsPerCluster)
#define BYTES_PER_CLUSTER   (LAYOUT.BytesPerCluster)
#define CLUSTER_COUNT       (LAYOUT.ClusterCount)
#define TOTAL_SECTORS       (LAYOUT.TotalSectors)

#define IS_HARD_DRIVE (DISK_GEO->MediaType == FixedMedia)


/* DISK DEFINES **************************************************************/

#define DISK_HEADS  0xFF


/* BOOT SECTOR DEFINES *******************************************************/

#define OEM_ID           BSWAP64(0x4E54465320202020)  // "NTFS    "
#define EBPB_HEADER      BSWAP32(0x80008000)
#define BOOT_SECTOR_END  BSWAP16(0x55AA)


/* MFT / RECORD DEFINES ******************************************************/

#define MFT_RECORD_SIZE    1024
#define INDEX_RECORD_SIZE  4096

// Count of MFT records materialized during format (the 16 reserved system records).
#define MFT_RESERVED_RECORDS  16

// Count of $MFTMirr records: the first four system records
// ($MFT, $MFTMirr, $LogFile, $Volume).
#define MFT_MIRR_COUNT  4


/* OTHER DEFINES *************************************************************/

#define NTFS_MAJOR_VERSION 3
#define NTFS_MINOR_VERSION 1

// The shared security id stamped on every file (its descriptor lives in $Secure).
#define NTFS_SECURITY_ID 0x100

#define FILE_RECORD_MAGIC  BSWAP32(0x46494C45)  // 'FILE'
#define INDX_RECORD_MAGIC  BSWAP32(0x494E4458)  // 'INDX'

// The beginning and length of an attribute record are always aligned to an 8-byte boundary,
// relative to the beginning of the file record.
#define ATTR_RECORD_ALIGNMENT  8

// A resident attribute's value offset is aligned to a 4-byte boundary,
// relative to the start of the attribute record.
#define VALUE_OFFSET_ALIGNMENT  4

// Data runs are aligned to a 4-byte boundary, relative to the start of the
// attribute record.
#define DATA_RUN_ALIGNMENT  4

// FILE_RECORD_END seems to follow AttributeEnd in every file record starting with $Quota.
// No clue what data is being represented here.
#define FILE_RECORD_END  0x11477982

#define FILE_NAME_POSIX          0
#define FILE_NAME_WIN32          1
#define FILE_NAME_DOS            2
#define FILE_NAME_WIN32_AND_DOS  3

#define FILE_TYPE_READ_ONLY  0x1
#define FILE_TYPE_HIDDEN     0x2
#define FILE_TYPE_SYSTEM     0x4
#define FILE_TYPE_ARCHIVE    0x20
#define FILE_TYPE_REPARSE    0x400
#define FILE_TYPE_COMPRESSED 0x800
#define FILE_TYPE_DIRECTORY  0x10000000

// Indexed Flag in Resident attributes - still somewhat speculative
#define RA_INDEXED  0x01

#define RA_METAFILES_ATTRIBUTES  (FILE_TYPE_SYSTEM | FILE_TYPE_HIDDEN)
#define RA_HEADER_LENGTH         (FIELD_OFFSET(ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR))

// Collation rules (used by $INDEX_ROOT)
#define COLLATION_BINARY               0x00
#define COLLATION_FILE_NAME            0x01
#define COLLATION_NTOFS_ULONG          0x10  // $Secure:$SII (by security id)
#define COLLATION_NTOFS_SECURITY_HASH  0x12  // $Secure:$SDH (by descriptor hash)

// Non-resident attribute header size. The extended form (with the trailing
// "compressed/allocated size" field at 0x40) is used for sparse/compressed
// attributes such as $BadClus:$Bad.
#define NONRES_HEADER_SIZE          0x40
#define NONRES_HEADER_SIZE_SPARSE   0x48

// Index entry flags
#define INDEX_ENTRY_NODE  0x01   // Entry points at a sub-node (has a child VCN)
#define INDEX_ENTRY_END   0x02   // Last entry in the node
// Index node header flags
#define INDEX_NODE_LARGE  0x01   // Node has an $INDEX_ALLOCATION

// A metadata record's sequence number. Windows uses sequence == record number
// only for the reserved system range 1..15 (record 0 uses 1, since sequence 0
// means "unused"); every other record - including the $Extend children at 24+ -
// is a normal first allocation with sequence 1. chkdsk rejects references to
// records >= 16 whose sequence isn't 1 (it reads as an impossibly-reused record).
#define RECORD_SEQUENCE(rec)  ((USHORT)(((rec) >= 1 && (rec) <= 15) ? (rec) : 1))

// Build an MFT file reference: record number in the low 48 bits, sequence
// number in the high 16 bits.
#define MFT_REFERENCE(rec)  (((ULONGLONG)(rec)) | ((ULONGLONG)RECORD_SEQUENCE(rec) << 48))

// Metafiles
#define METAFILE_MFT              0
#define METAFILE_MFTMIRR          1
#define METAFILE_LOGFILE          2
#define METAFILE_VOLUME           3
#define METAFILE_ATTRDEF          4
#define METAFILE_ROOT             5
#define METAFILE_BITMAP           6
#define METAFILE_BOOT             7
#define METAFILE_BADCLUS          8
#define METAFILE_SECURE           9
#define METAFILE_UPCASE           10
#define METAFILE_EXTEND           11
#define METAFILE_FIRST_USER_FILE  16

#define RUN_LIST_ENTRY_HEADER_SIZE 1
#define RUN_LIST_ENTRY_SIZE        8


/* GLOBAL DATA ***************************************************************/

//
// The whole metadata is laid out contiguously from the start of the volume:
//
//   [ $Boot | $MFT | $MFTMirr | $LogFile | $Bitmap | $UpCase | $AttrDef | $MFT bitmap | ... free ... ]
//
// A correct NTFS driver locates $MFT / $MFTMirr through the boot sector, so the
// exact placement does not matter for mountability - only that everything fits
// inside the volume and the cluster bitmap marks the used clusters.
//
typedef struct _NTFS_LAYOUT
{
    ULONG      SectorsPerCluster;
    ULONG      BytesPerCluster;
    ULONGLONG  TotalSectors;      // Value recorded in the boot sector (volume sectors - 1)
    ULONGLONG  ClusterCount;

    CHAR       ClustersPerMftRecord;    // Signed, boot-sector encoding
    CHAR       ClustersPerIndexRecord;  // Signed, boot-sector encoding

    ULONGLONG  SerialNumber;

    // Cluster placement of the metadata
    ULONGLONG  BootLcn;        ULONG BootClusters;
    ULONGLONG  MftLcn;         ULONG MftClusters;   ULONG MftAllocRecords;
    ULONGLONG  MftMirrLcn;     ULONG MftMirrClusters;
    ULONGLONG  LogFileLcn;     ULONG LogFileClusters;
    ULONGLONG  BitmapLcn;      ULONG BitmapClusters;
    ULONGLONG  UpCaseLcn;      ULONG UpCaseClusters;
    ULONGLONG  AttrDefLcn;     ULONG AttrDefClusters;
    ULONGLONG  MftBitmapLcn;   ULONG MftBitmapClusters;
    ULONGLONG  RootIdxLcn;     ULONG RootIdxClusters;   // One INDX block for the root $I30 index
    ULONGLONG  SdsLcn;         ULONG SdsClusters;       // $Secure:$SDS (8 descriptors + 256 KiB mirror)
    ULONGLONG  SdhIdxLcn;      ULONG SdhIdxClusters;    // $Secure:$SDH INDX block (large view index)

    // $Extend / Transactional-NTFS (TxF) payload streams (zero-initialized).
    ULONGLONG  TopsTLcn;       ULONG TopsTClusters;     // $RmMetadata/$TxfLog/$Tops:$T (1 MiB)
    ULONGLONG  BlfLcn;         ULONG BlfClusters;       // $TxfLog/$TxfLog.blf (64 KiB)
    ULONGLONG  Cont1Lcn;       ULONG Cont1Clusters;     // $TxfLogContainer...0001 (2 MiB)
    ULONGLONG  Cont2Lcn;       ULONG Cont2Clusters;     // $TxfLogContainer...0002 (2 MiB)
    ULONGLONG  DeletedIdxLcn;  ULONG DeletedIdxClusters;// $Extend/$Deleted:$I30 allocation (64 KiB)

    ULONGLONG  FirstFreeLcn;   // Count of clusters used by metadata == first free cluster
} NTFS_LAYOUT, *PNTFS_LAYOUT;

// The $SDS descriptor mirror is stored NTFS_SDS_MIRROR bytes after the primary.
#define NTFS_SDS_MIRROR  0x40000

// Shared across all ntfslib translation units. The single definition lives in
// ntfslib.c; every other file sees it through this extern declaration.
typedef struct _NTFS_FORMAT_DATA
{
    HANDLE                  DiskHandle;
    GET_LENGTH_INFORMATION* LengthInformation;
    PDISK_GEOMETRY          DiskGeometry;
    PUNICODE_STRING         Label;
    ULONGLONG               FormatTime;   // Single timestamp stamped on every record
    ULONG                   HiddenSectors;// Partition start LBA (BPB field; the VBR needs it to boot)
    NTFS_LAYOUT             Layout;
} NTFS_FORMAT_DATA;

extern NTFS_FORMAT_DATA NtfsFormatData;

#define HIDDEN_SECTORS (NtfsFormatData.HiddenSectors)

#define FORMAT_TIME (NtfsFormatData.FormatTime)


/* BOOT SECTOR STRUCTURES ****************************************************/

#include <pshpack1.h>

typedef struct _BIOS_PARAMETERS_BLOCK
{
    USHORT   BytesPerSector;       // 0x0B
    BYTE     SectorsPerCluster;    // 0x0D
    BYTE     Unused0[7];           // 0x0E
    BYTE     MediaId;              // 0x15
    USHORT   Unused1;              // 0x16
    USHORT   SectorsPerTrack;      // 0x18
    USHORT   Heads;                // 0x1A
    DWORD32  HiddenSectorsCount;   // 0x1C
    DWORD32  Unused2;              // 0x20
} BIOS_PARAMETERS_BLOCK, *PBIOS_PARAMETERS_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETERS_BLOCK
{
    DWORD32    Header;                  // 0x24
    ULONGLONG  SectorCount;             // 0x28
    ULONGLONG  MftLocation;             // 0x30
    ULONGLONG  MftMirrLocation;         // 0x38
    CHAR       ClustersPerMftRecord;    // 0x40
    BYTE       Unused0[3];              // 0x41
    CHAR       ClustersPerIndexRecord;  // 0x44
    BYTE       Unused1[3];              // 0x45
    ULONGLONG  SerialNumber;            // 0x48
    DWORD32    Checksum;                // 0x50, unused
} EXTENDED_BIOS_PARAMETERS_BLOCK, *PEXTENDED_BIOS_PARAMETERS_BLOCK;

typedef struct _BOOT_SECTOR
{
    BYTE                            Jump[3];         // 0x00
    ULARGE_INTEGER                  OEMID;           // 0x03
    BIOS_PARAMETERS_BLOCK           BPB;             // 0x0B
    EXTENDED_BIOS_PARAMETERS_BLOCK  EBPB;            // 0x24
    BYTE                            BootStrap[426];  // 0x54
    USHORT                          EndSector;       // 0x1FE
} BOOT_SECTOR, * PBOOT_SECTOR;

#include <poppack.h>


/* FILES DATA ****************************************************************/

typedef struct _RECORD_HEADER
{
    ULONG      Magic;        // 0x00, magic 'FILE'
    USHORT     UsaOffset;    // 0x04, offset to the update sequence
    USHORT     UsaCount;     // 0x06, size in words of Update Sequence Number & Array (S)
    ULONGLONG  Lsn;          // 0x08, $LogFile Sequence Number
} RECORD_HEADER, *PRECORD_HEADER;

typedef enum _MFT_RECORD_FLAGS
{
    MFT_RECORD_NOT_USED     = 0x0000,
    MFT_RECORD_IN_USE       = 0x0001,
    MFT_RECORD_IS_DIRECTORY = 0x0002,
    MFT_RECORD_UNKNOWN1     = 0x0004,   // Set on $Extend children by Windows
    MFT_RECORD_VIEW_INDEX   = 0x0008    // File carries a view index (e.g. $Secure)
} MFT_RECORD_FLAGS, *PMFT_RECORD_FLAGS;

typedef struct _FILE_RECORD_HEADER
{
    RECORD_HEADER  Header;            // 0x00
    USHORT     SequenceNumber;        // 0x10
    USHORT     HardLinkCount;         // 0x12
    USHORT     FirstAttributeOffset;  // 0x14
    USHORT     Flags;                 // 0x16, flags (see MFT_RECORD_FLAGS)
    ULONG      BytesInUse;            // 0x18, real size of the FILE record
    ULONG      BytesAllocated;        // 0x1C, allocated size of the FILE record
    ULONGLONG  BaseFileRecord;        // 0x20, file reference to the base FILE record
    USHORT     NextAttributeNumber;   // 0x28
    USHORT     Padding;               // 0x2A
    ULONG      MFTRecordNumber;       // 0x2C
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;


/* ATTRIBUTES COMMON *********************************************************/

typedef enum _ATTR_FLAGS
{
    ATTR_IS_COMPRESSED = 0x1,
    ATTR_IS_ENCRYPTED  = 0x4000,
    ATTR_IS_SPARSE     = 0x8000
} ATTR_FLAGS, *PATTR_FLAGS;

typedef enum _ATTRIBUTE_TYPE
{
    AttributeStandardInformation = 0x10,
    AttributeAttributeList       = 0x20,
    AttributeFileName            = 0x30,
    AttributeObjectId            = 0x40,
    AttributeSecurityDescriptor  = 0x50,
    AttributeVolumeName          = 0x60,
    AttributeVolumeInformation   = 0x70,
    AttributeData                = 0x80,
    AttributeIndexRoot           = 0x90,
    AttributeIndexAllocation     = 0xA0,
    AttributeBitmap              = 0xB0,
    AttributeReparsePoint        = 0xC0,
    AttributeEAInformation       = 0xD0,
    AttributeEA                  = 0xE0,
    AttributePropertySet         = 0xF0,
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd                 = 0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct _ATTR_RECORD
{
    ULONG   Type;           // 0x00
    ULONG   Length;         // 0x04
    UCHAR   IsNonResident;  // 0x08
    UCHAR   NameLength;     // 0x09
    USHORT  NameOffset;     // 0x0A
    USHORT  Flags;          // 0x0C
    USHORT  Instance;       // 0x0E
    union
    {
        struct
        {
            ULONG   ValueLength;  // 0x10
            USHORT  ValueOffset;  // 0x14
            BYTE    Flags;        // 0x16
            BYTE    Reserved;     // 0x17
        } Resident;

        struct
        {
            ULONGLONG  LowestVCN;           // 0x10
            ULONGLONG  HighestVCN;          // 0x18
            USHORT     DataRunsOffset;      // 0x20
            USHORT     CompressionUnit;     // 0x22
            BYTE       Reserved[4];         // 0x24
            LONGLONG   AllocatedSize;       // 0x28
            LONGLONG   DataSize;            // 0x30
            LONGLONG   CompressedSize;     // 0x38
        } NonResident;
    };
} ATTR_RECORD, *PATTR_RECORD;


/* ATTRIBUTES STRUCTURES *****************************************************/

typedef struct _STANDARD_INFORMATION
{
    ULONGLONG CreationTime;      // 0x00
    ULONGLONG ChangeTime;        // 0x08
    ULONGLONG LastWriteTime;     // 0x10
    ULONGLONG LastAccessTime;    // 0x18
    ULONG     FileAttribute;     // 0x20
    ULONG     MaximumVersions;   // 0x24
    ULONG     VersionNumber;     // 0x28
    ULONG     ClassId;           // 0x2C
    ULONG     OwnerId;           // 0x30
    ULONG     SecurityId;        // 0x34
    ULONGLONG QuotaCharge;       // 0x38
    ULONGLONG Usn;               // 0x40
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;   // 72 bytes (NTFS 3.x)

typedef struct _FILENAME_ATTRIBUTE
{
    ULONGLONG DirectoryFileReferenceNumber;
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONG     FileAttributes;
    union
    {
        struct
        {
            USHORT PackedEaSize;
            USHORT AlignmentOrReserved;
        } EaInfo;

        ULONG ReparseTag;
    } Extended;
    BYTE  NameLength;
    BYTE  NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct _VOLUME_INFORMATION_ATTRIBUTE
{
    BYTE   Unused[8];
    BYTE   MajorVersion;
    BYTE   MinorVersion;
    USHORT Flags;
} VOLUME_INFORMATION_ATTRIBUTE, *PVOLUME_INFORMATION_ATTRIBUTE;


/* INDEX STRUCTURES **********************************************************/

typedef struct _INDEX_NODE_HEADER
{
    ULONG FirstEntryOffset;    // Offset to the first index entry, relative to this header
    ULONG TotalSizeOfEntries;  // Total size of the index entries, incl. this header
    ULONG AllocatedSize;       // Allocated size of the node
    BYTE  Flags;               // 0 = small (fits in $INDEX_ROOT), 1 = large ($INDEX_ALLOCATION)
    BYTE  Padding[3];
} INDEX_NODE_HEADER, *PINDEX_NODE_HEADER;

typedef struct _INDEX_ROOT_ATTRIBUTE
{
    ULONG             AttributeType;           // Type of the indexed attribute ($FILE_NAME = 0x30)
    ULONG             CollationRule;
    ULONG             SizeOfEntry;             // Bytes per index record
    BYTE              ClustersPerIndexRecord;
    BYTE              Padding[3];
    INDEX_NODE_HEADER Header;
} INDEX_ROOT_ATTRIBUTE, *PINDEX_ROOT_ATTRIBUTE;

typedef struct _INDEX_ENTRY_HEADER
{
    ULONGLONG FileReference;  // MFT reference of the indexed file (0 for the end marker)
    USHORT    Length;         // Length of this index entry
    USHORT    KeyLength;      // Length of the key ($FILE_NAME) that follows
    USHORT    Flags;          // See INDEX_ENTRY_*
    USHORT    Reserved;
} INDEX_ENTRY_HEADER, *PINDEX_ENTRY_HEADER;


/* PROTOTYPES ****************************************************************/

ULONG
NTAPI NtGetTickCount(VOID); 

// ntfslib.c

VOID
NtfsGetSystemTimeAsFileTime(OUT PFILETIME lpFileTime);

BYTE GetSectorsPerCluster(VOID);

// Computes the on-disk layout (cluster size, metadata placement, serial number)
// and stores it in LAYOUT. ClusterSize is the caller-requested cluster size in
// bytes (0 = auto-select). Returns STATUS_SUCCESS, or an error if the volume is
// too small to hold the NTFS metadata.
NTSTATUS
ComputeLayout(IN ULONG ClusterSize);

// bootsect.c

NTSTATUS
WriteBootSector(VOID);

// files.c

NTSTATUS
WriteMetafiles(VOID);

#endif
