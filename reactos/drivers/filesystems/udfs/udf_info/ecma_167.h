////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
    Module name:

   ecma_167.h

    Abstract:

   This file contains ECMA-167 definitions

*/

#ifndef __ECMA_167_H__
#define __ECMA_167_H__

typedef uint8   dstring;

#define UDF_COMP_ID_8    0x08
#define UDF_COMP_ID_16   0x10

/* make sure all structures are packed! */
#pragma pack(push, 1)

/* CS0 Charspec (ECMA 167 1/7.2.1) */
typedef struct {
    uint8 charSetType;
    uint8 charSetInfo[63];
} charspec;

/* Timestamp (ECMA 167 1/7.3) */
typedef struct {
    uint16 typeAndTimezone;
    uint16 year;
    uint8 month;
    uint8 day;
    uint8 hour;
    uint8 minute;
    uint8 second;
    uint8 centiseconds;
    uint8 hundredsOfMicroseconds;
    uint8 microseconds;
} timestamp;

typedef timestamp       UDF_TIME_STAMP;
typedef UDF_TIME_STAMP* PUDF_TIME_STAMP;

/* Timestamp types (ECMA 167 1/7.3.1) */
#define TIMESTAMP_TYPE_CUT          0x0000U
#define TIMESTAMP_TYPE_LOCAL        0x0001U
#define TIMESTAMP_TYPE_AGREEMENT    0x0002U
#define TIMESTAMP_OFFSET_MASK       0x0FFFU
#define TIMESTAMP_NO_OFFSET         0x0800U

/* Entity Identifier (ECMA 167 1/7.4) */
typedef struct {
    uint8 flags;
    uint8 ident[23];
    uint8 identSuffix[8];
} EntityID;
#define regid EntityID

/* Entity identifier flags (ECMA 167 1/7.4.1) */
#define ENTITYID_FLAGS_DIRTY        0x01U
#define ENTITYID_FLAGS_PROTECTED    0x02U

/* Volume Structure Descriptor (ECMA 167 2/9.1) */
#define STD_ID_LEN  5
struct VolStructDesc {
    uint8 structType;
    uint8 stdIdent[STD_ID_LEN];
    uint8 structVersion;
    uint8 structData[2041];
};

/* Std structure identifiers (ECMA 167 2/9.1.2) */
#define STD_ID_BEA01    "BEA01"
#define STD_ID_BOOT2    "BOOT2"
#define STD_ID_CD001    "CD001"
#define STD_ID_CDW02    "CDW02"
#define STD_ID_NSR02    "NSR02"
#define STD_ID_NSR03    "NSR03"
#define STD_ID_TEA01    "TEA01"

/* Beginning Extended Area Descriptor (ECMA 167 2/9.2) */
struct BeginningExtendedAreaDesc {
    uint8 structType;
    uint8 stdIdent[STD_ID_LEN];
    uint8 structVersion;
    uint8 structData[2041];
};

/* Terminating Extended Area Descriptor (ECMA 167 2/9.3) */
struct TerminatingExtendedAreaDesc {
    uint8 structType;
    uint8 stdIdent[STD_ID_LEN];
    uint8 structVersion;
    uint8 structData[2041];
};

/* Boot Descriptor (ECMA 167 2/9.4) */
typedef struct _BootDesc {
    uint8 structType;
    uint8 stdIdent[STD_ID_LEN];
    uint8 structVersion;
    uint8 reserved1;
    EntityID architectureType;
    EntityID bootIdent;
    uint32 bootExtLocation;
    uint32 bootExtLength;
    uint64 loadAddress;
    uint64 startAddress;
    timestamp descCreationDateAndTime;
    uint16 flags;
    uint8 reserved2[32];
    uint8 bootUse[1906];
} BootDesc, *PBootDesc;

/* Boot flags (ECMA 167 2/9.4.12) */
#define BOOT_FLAGS_ERASE    1

/* Extent Descriptor (ECMA 167 3/7.1) */

typedef struct _EXTENT_AD {
    uint32 extLength;
    uint32 extLocation;   // Lba
} EXTENT_AD, *PEXTENT_AD;

typedef EXTENT_AD   extent_ad;

typedef EXTENT_AD   EXTENT_MAP;
typedef PEXTENT_AD  PEXTENT_MAP;

/* Descriptor Tag (ECMA 167 3/7.2) */
typedef struct {
    uint16 tagIdent;
    uint16 descVersion;
    uint8 tagChecksum;
    uint8 reserved;
    uint16 tagSerialNum;
    uint16 descCRC;
    uint16 descCRCLength;
    uint32 tagLocation;
} tag;

typedef tag       DESC_TAG;
typedef DESC_TAG* PDESC_TAG;

/* Tag Identifiers (ECMA 167 3/7.2.1) */
#define TID_UNUSED_DESC                 0x0000U
#define TID_PRIMARY_VOL_DESC            0x0001U
#define TID_ANCHOR_VOL_DESC_PTR         0x0002U
#define TID_VOL_DESC_PTR                0x0003U
#define TID_IMP_USE_VOL_DESC            0x0004U
#define TID_PARTITION_DESC              0x0005U
#define TID_LOGICAL_VOL_DESC            0x0006U
#define TID_UNALLOC_SPACE_DESC          0x0007U
#define TID_TERMINATING_DESC            0x0008U
#define TID_LOGICAL_VOL_INTEGRITY_DESC  0x0009U

/* Tag Identifiers (ECMA 167 4/7.2.1) */
#define TID_FILE_SET_DESC               0x0100U
#define TID_FILE_IDENT_DESC             0x0101U
#define TID_ALLOC_EXTENT_DESC           0x0102U
#define TID_INDIRECT_ENTRY              0x0103U
#define TID_TERMINAL_ENTRY              0x0104U
#define TID_FILE_ENTRY                  0x0105U
#define TID_EXTENDED_ATTRE_HEADER_DESC  0x0106U
#define TID_UNALLOCATED_SPACE_ENTRY     0x0107U
#define TID_SPACE_BITMAP_DESC           0x0108U
#define TID_PARTITION_INTEGRITY_ENTRY   0x0109U
#define TID_EXTENDED_FILE_ENTRY         0x010AU

/* NSR Descriptor (ECMA 167 3/9.1) */
struct NSRDesc {
    uint8 structType;
    uint8 stdIdent[STD_ID_LEN];
    uint8 structVersion;
    uint8 reserved;
    uint8 structData[2040];
};
    
/* Primary Volume Descriptor (ECMA 167 3/10.1) */
struct PrimaryVolDesc {
    tag descTag;
    uint32 volDescSeqNum;
    uint32 primaryVolDescNum;
    dstring volIdent[32];
    uint16 volSeqNum;
    uint16 maxVolSeqNum;
    uint16 interchangeLvl;
    uint16 maxInterchangeLvl;
    uint32 charSetList;
    uint32 maxCharSetList;
    dstring volSetIdent[128];
    charspec descCharSet;
    charspec explanatoryCharSet;
    extent_ad volAbstract;
    extent_ad volCopyright;
    EntityID appIdent;
    timestamp recordingDateAndTime;
    EntityID impIdent;
    uint8 impUse[64];
    uint32 predecessorVolDescSeqLocation;
    uint16 flags;
    uint8 reserved[22];
};

/* Primary volume descriptor flags (ECMA 167 3/10.1.21) */
#define VOL_SET_IDENT   1

/* Anchor Volume Descriptor Pointer (ECMA 167 3/10.2) */
struct AnchorVolDescPtr {
    tag descTag;
    extent_ad mainVolDescSeqExt;
    extent_ad reserveVolDescSeqExt;
    uint8 reserved[480];
};

/* Volume Descriptor Pointer (ECMA 167 3/10.3) */
struct VolDescPtr {
    tag descTag;
    uint32 volDescSeqNum;
    extent_ad nextVolDescSeqExt;
    uint8 reserved[484];
};

#define MAX_VDS_PARTS  32

/* Implementation Use Volume Descriptor (ECMA 167 3/10.4) */
struct ImpUseVolDesc {
    tag descTag;
    uint32 volDescSeqNum;
    EntityID impIdent;
    uint8 impUse[460];
};

/* Partition Descriptor (ECMA 167 3/10.5) */
struct PartitionDesc {
    tag descTag;
    uint32 volDescSeqNum;
    uint16 partitionFlags;
    uint16 partitionNumber;
    EntityID partitionContents;
    uint8 partitionContentsUse[128];
    uint32 accessType;
    uint32 partitionStartingLocation;
    uint32 partitionLength;
    EntityID impIdent;
    uint8 impUse[128];
    uint8 reserved[156];
};

/* Partition Flags (ECMA 167 3/10.5.3) */
#define PARTITION_FLAGS_ALLOC   1

/* Partition Contents (ECMA 167 3/10.5.5) */
#define PARTITION_CONTENTS_FDC01    "+FDC01"
#define PARTITION_CONTENTS_CD001    "+CD001"
#define PARTITION_CONTENTS_CDW02    "+CDW02"
#define PARTITION_CONTENTS_NSR02    "+NSR02"
#define PARTITION_CONTENTS_NSR03    "+NSR03"

/* Partition Access Types (ECMA 167 3/10.5.7) */
#define PARTITION_ACCESS_NONE       0
#define PARTITION_ACCESS_R          1
#define PARTITION_ACCESS_WO         2
#define PARTITION_ACCESS_RW         3
#define PARTITION_ACCESS_OW         4
#define PARTITION_ACCESS_MAX_KNOWN  PARTITION_ACCESS_OW

/* Logical Volume Descriptor (ECMA 167 3/10.6) */
struct LogicalVolDesc {
    tag descTag;
    uint32 volDescSeqNum;
    charspec descCharSet;
    dstring logicalVolIdent[128];
    uint32 logicalBlockSize;
    EntityID domainIdent;
    uint8 logicalVolContentsUse[16]; /* used to find fileset */
    uint32 mapTableLength;
    uint32 numPartitionMaps;
    EntityID impIdent;
    uint8 impUse[128];
    extent_ad integritySeqExt;
//  uint8 partitionMaps[0];
};

/* Generic Partition Map (ECMA 167 3/10.7.1) */
struct GenericPartitionMap {
    uint8 partitionMapType;
    uint8 partitionMapLength;
//  uint8 partitionMapping[0];
};

/* Partition Map Type (ECMA 167 3/10.7.1.1) */
#define PARTITION_MAP_TYPE_NONE     0
#define PARTITION_MAP_TYPE_1        1
#define PARTITION_MAP_TYPE_2        2

/* Type 1 Partition Map (ECMA 167 3/10.7.2) */
struct GenericPartitionMap1 {
    uint8 partitionMapType;
    uint8 partitionMapLength;
    uint16 volSeqNum;
    uint16 partitionNum;
};

/* Type 2 Partition Map (ECMA 167 3/10.7.3) */
struct GenericPartitionMap2 {
    uint8 partitionMapType; /* 2 */
    uint8 partitionMapLength; 
    uint8 partitionIdent[62];
};

/* Unallocated Space Descriptor (ECMA 167 3/10.8) */
typedef struct _UNALLOC_SPACE_DESC {
    tag descTag;
    uint32 volDescSeqNum;
    uint32 numAllocDescs;
//  extent_ad allocDescs[0];
} UNALLOC_SPACE_DESC, *PUNALLOC_SPACE_DESC;

typedef UNALLOC_SPACE_DESC UnallocatedSpaceDesc;

/* Terminating Descriptor (ECMA 3/10.9) */
struct TerminatingDesc {
    tag descTag;
    uint8 reserved[496];
};

struct GenericDesc
{
        tag descTag;
        uint32 volDescSeqNum;
};

/* Logical Volume Integrity Descriptor (ECMA 167 3/10.10) */

struct LogicalVolIntegrityDesc {
    tag descTag;
    timestamp recordingDateAndTime;
    uint32 integrityType;
    extent_ad nextIntegrityExt;
    uint8 logicalVolContentsUse[32];
    uint32 numOfPartitions;
    uint32 lengthOfImpUse;
//  uint32 freeSpaceTable[0];
//  uint32 sizeTable[0];
//  uint8 impUse[0];
};

/* Integrity Types (ECMA 167 3/10.10.3) */
#define INTEGRITY_TYPE_OPEN     0
#define INTEGRITY_TYPE_CLOSE    1

/* Recorded Address (ECMA 167 4/7.1) */
typedef struct {
    uint32 logicalBlockNum;
    uint16 partitionReferenceNum;
} lb_addr;

/* Extent interpretation (ECMA 167 4/14.14.1.1) */
#define EXTENT_RECORDED_ALLOCATED               0x00
#define EXTENT_NOT_RECORDED_ALLOCATED           0x01
#define EXTENT_NOT_RECORDED_NOT_ALLOCATED       0x02
#define EXTENT_NEXT_EXTENT_ALLOCDESC            0x03

/* Long Allocation Descriptor (ECMA 167 4/14.14.2) */
typedef struct {
    uint32 extLength;
    lb_addr extLocation;
    uint8 impUse[6];
} long_ad;
    /* upper 2 bits of extLength indicate type */
typedef long_ad  LONG_AD;
typedef LONG_AD* PLONG_AD;

/* File Set Descriptor (ECMA 167 4/14.1) */
typedef struct _FILE_SET_DESC {
    tag descTag;
    timestamp recordingDateAndTime;
    uint16 interchangeLvl;
    uint16 maxInterchangeLvl;
    uint32 charSetList;
    uint32 maxCharSetList;
    uint32 fileSetNum;
    uint32 fileSetDescNum;
    charspec logicalVolIdentCharSet;
    dstring logicalVolIdent[128];
    charspec fileSetCharSet;
    dstring fileSetIdent[32];
    dstring copyrightFileIdent[32];
    dstring abstractFileIdent[32];
    long_ad rootDirectoryICB;        //points to Allocation Ext Descriptor
    EntityID domainIdent;
    long_ad nextExt;
    long_ad streamDirectoryICB;
    uint8 reserved[32];
} FILE_SET_DESC, *PFILE_SET_DESC;

/* Short Allocation Descriptor (ECMA 167 4/14.14.1) */
typedef struct _SHORT_AD {
    uint32 extLength;
    uint32 extPosition;
} SHORT_AD, *PSHORT_AD;

typedef SHORT_AD short_ad;

/* Partition Header Descriptor (ECMA 167 4/14.3) */
typedef struct _PARTITION_HEADER_DESC {
    short_ad unallocatedSpaceTable;
    short_ad unallocatedSpaceBitmap;             // 0 - allocated, 1 - free
    short_ad partitionIntegrityTable;
    short_ad freedSpaceTable;
    short_ad freedSpaceBitmap;                   // 0 - ????       1 - freed
    uint8 reserved[88];
} PARTITION_HEADER_DESC, *PPARTITION_HEADER_DESC;

/* File Identifier Descriptor (ECMA 167 4/14.4) */

typedef struct _FILE_IDENT_DESC {
    tag descTag;
    uint16 fileVersionNum; 
    uint8 fileCharacteristics;
    uint8 lengthFileIdent;
    long_ad icb;
    uint16 lengthOfImpUse;
//  uint8 impUse[0];
//  uint8 fileIdent[0];
//  uint8 padding[0];
} FILE_IDENT_DESC, *PFILE_IDENT_DESC;

/* File Characteristics (ECMA 167 4/14.4.3) */
#define FILE_HIDDEN     0x01
#define FILE_DIRECTORY  0x02
#define FILE_DELETED    0x04
#define FILE_PARENT     0x08
#define FILE_METADATA   0x10 /* UDF 2.0 */

/* Allocation Ext Descriptor (ECMA 167 4/14.5) */
typedef struct _ALLOC_EXT_DESC {
    tag descTag;
    uint32 previousAllocExtLocation;
    uint32 lengthAllocDescs;
} ALLOC_EXT_DESC, *PALLOC_EXT_DESC;

/* ICB Tag (ECMA 167 4/14.6) */
typedef struct {
    uint32 priorRecordedNumDirectEntries;
    uint16 strategyType;
    uint16 strategyParameter;
    uint16 numEntries;
    uint8 reserved;
    uint8 fileType;
    lb_addr parentICBLocation;
    uint16 flags;
} icbtag;

/* ICB File Type (ECMA 167 4/14.6.6) */
#define UDF_FILE_TYPE_NONE      0x00U
#define UDF_FILE_TYPE_UNALLOC   0x01U
#define UDF_FILE_TYPE_INTEGRITY 0x02U
#define UDF_FILE_TYPE_INDIRECT  0x03U
#define UDF_FILE_TYPE_DIRECTORY 0x04U
#define UDF_FILE_TYPE_REGULAR   0x05U
#define UDF_FILE_TYPE_BLOCK     0x06U
#define UDF_FILE_TYPE_CHAR      0x07U
#define UDF_FILE_TYPE_EXTENDED  0x08U
#define UDF_FILE_TYPE_FIFO      0x09U
#define UDF_FILE_TYPE_SOCKET    0x0aU
#define UDF_FILE_TYPE_TERMINAL  0x0bU
#define UDF_FILE_TYPE_SYMLINK   0x0cU
#define UDF_FILE_TYPE_STREAMDIR 0x0dU /* ECMA 167 4/13 */

/* ICB Flags (ECMA 167 4/14.6.8) */
#define ICB_FLAG_ALLOC_MASK     0x0007U
#define ICB_FLAG_SORTED         0x0008U
#define ICB_FLAG_NONRELOCATABLE 0x0010U
#define ICB_FLAG_ARCHIVE        0x0020U
#define ICB_FLAG_SETUID         0x0040U
#define ICB_FLAG_SETGID         0x0080U
#define ICB_FLAG_STICKY         0x0100U
#define ICB_FLAG_CONTIGUOUS     0x0200U
#define ICB_FLAG_SYSTEM         0x0400U
#define ICB_FLAG_TRANSFORMED    0x0800U
#define ICB_FLAG_MULTIVERSIONS  0x1000U

/* ICB Flags Allocation type(ECMA 167 4/14.6.8) */
#define ICB_FLAG_AD_SHORT    0
#define ICB_FLAG_AD_LONG     1
#define ICB_FLAG_AD_EXTENDED 2
#define ICB_FLAG_AD_IN_ICB   3

/* Indirect Entry (ECMA 167 4/14.7) */
struct IndirectEntry {
    tag descTag;
    icbtag icbTag;
    long_ad indirectICB;
};

/* Terminal Entry (ECMA 167 4/14.8) */
struct TerminalEntry {
    tag descTag;
    icbtag icbTag;
};

/* File Entry (ECMA 167 4/14.9) */

typedef struct _FILE_ENTRY {
    tag         descTag;
    icbtag      icbTag;
    uint32      uid;
    uint32      gid;
    uint32      permissions;
    uint16      fileLinkCount;
    uint8       recordFormat;
    uint8       recordDisplayAttr;
    uint32      recordLength;
    uint64      informationLength;
    uint64      logicalBlocksRecorded;
    timestamp   accessTime;
    timestamp   modificationTime;
    timestamp   attrTime;
    uint32      checkpoint;
    long_ad     extendedAttrICB;
    EntityID    impIdent;
    uint64      uniqueID; /* 0= root, 16- (2^32-1) */

    uint32      lengthExtendedAttr;
    uint32      lengthAllocDescs;
//  uint8       extendedAttr[0];
//  uint8       allocDescs[0];
} FILE_ENTRY, *PFILE_ENTRY;

/* File Permissions (ECMA 167 4/14.9.5) */
#define PERM_O_EXEC     0x00000001U
#define PERM_O_WRITE    0x00000002U
#define PERM_O_READ     0x00000004U
#define PERM_O_CHATTR   0x00000008U
#define PERM_O_DELETE   0x00000010U
#define PERM_G_EXEC     0x00000020U
#define PERM_G_WRITE    0x00000040U
#define PERM_G_READ     0x00000080U
#define PERM_G_CHATTR   0x00000100U
#define PERM_G_DELETE   0x00000200U
#define PERM_U_EXEC     0x00000400U
#define PERM_U_WRITE    0x00000800U
#define PERM_U_READ     0x00001000U
#define PERM_U_CHATTR   0x00002000U
#define PERM_U_DELETE   0x00004000U

/* File Record Format (ECMA 167 4/14.9.7) */
#define RECORD_FMT_NONE             0
#define RECORD_FMT_FIXED_PAD        1
#define RECORD_FMT_FIXED            2
#define RECORD_FMT_VARIABLE8        3
#define RECORD_FMT_VARIABLE16       4
#define RECORD_FMT_VARIABLE16_MSB   5
#define RECORD_FMT_VARIABLE32       6
#define RECORD_FMT_PRINT            7
#define RECORD_FMT_LF               8
#define RECORD_FMT_CR               9
#define RECORD_FMT_CRLF             10
#define RECORD_FMT_LFCR             10

/* Extended Attribute Header Descriptor (ECMA 167 4/14.10.1) */
struct ExtendedAttrHeaderDesc {
    tag descTag;
    uint32 impAttrLocation;
    uint32 appAttrLocation;
};

/* Generic Attribute Format (ECMA 4/14.10.2) */
struct GenericAttrFormat {
    uint32 attrType;
    uint8 attrSubtype;
    uint8 reserved[3];
    uint32 attrLength;
//  uint8 attrData[0];
};

/* Character Set Attribute Format (ECMA 4/14.10.3) */
struct CharSetAttrFormat {
    uint32 attrType;        /* 1 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 escapeSeqLength;
    uint8 charSetType;
//  uint8 escapeSeq[0];
};

/* Alternate Permissions (ECMA 167 4/14.10.4) */
struct AlternatePermissionsExtendedAttr {
    uint32 attrType;        /* 3 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint16 ownerIdent;
    uint16 groupIdent;
    uint16 permission;
};

/* File Times Extended Attribute (ECMA 167 4/14.10.5) */
struct FileTimesExtendedAttr {
    uint32 attrType;        /* 5 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 dataLength;
    uint32 fileTimeExistence;
//    timestamp fileTimes[0];
};

/* FileTimeExistence (ECMA 167 4/14.10.5.6) */
#define FTE_CREATION    0
#define FTE_DELETION    2
#define FTE_EFFECTIVE   3
#define FTE_BACKUP  5

/* Information Times Extended Attribute (ECMA 167 4/14.10.6) */
struct InfoTimesExtendedAttr {
    uint32 attrType;        /* 6 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 dataLength;
    uint32 infoTimeExistence;
//  uint8 infoTimes[0];
};

/* Device Specification Extended Attribute (ECMA 167 4/14.10.7) */
struct DeviceSpecificationExtendedAttr {
    uint32 attrType;        /* 12 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 impUseLength;
    uint32 majorDeviceIdent;
    uint32 minorDeviceIdent;
//  uint8 impUse[0];
};

/* Implementation Use Extended Attr (ECMA 167 4/14.10.8) */
struct ImpUseExtendedAttr {
    uint32 attrType;        /* 2048 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 impUseLength;
    EntityID impIdent;
//  uint8 impUse[0];
};

/* Application Use Extended Attribute (ECMA 167 4/14.10.9) */
struct AppUseExtendedAttr {
    uint32 attrType;        /* 65536 */
    uint8 attrSubtype;      /* 1 */
    uint8 reserved[3];
    uint32 attrLength;
    uint32 appUseLength;
    EntityID appIdent;
//  uint8 appUse[0];
};

#define EXTATTR_CHAR_SET    1
#define EXTATTR_ALT_PERMS   3
#define EXTATTR_FILE_TIMES  5
#define EXTATTR_INFO_TIMES  6
#define EXTATTR_DEV_SPEC    12
#define EXTATTR_IMP_USE     2048
#define EXTATTR_APP_USE     65536


/* Unallocated Space Entry (ECMA 167 4/14.11) */
struct UnallocatedSpaceEntry {
    tag descTag;
    icbtag icbTag;
    uint32 lengthAllocDescs;
//  uint8 allocDescs[0];
};

/* Space Bitmap Descriptor (ECMA 167 4/14.12) */
typedef struct _SPACE_BITMAP_DESC {
    tag descTag;
    uint32 numOfBits;
    uint32 numOfBytes;
//  uint8 bitmap[0]; // describes blocks from Lba=0 to Lba=LAST_LBA
} SPACE_BITMAP_DESC, *PSPACE_BITMAP_DESC;

typedef SPACE_BITMAP_DESC SpaceBitmapDesc;

/* Partition Integrity Entry (ECMA 167 4/14.13) */
struct PartitionIntegrityEntry {
    tag descTag;
    icbtag icbTag;
    timestamp recordingDateAndTime;
    uint8 integrityType;
    uint8 reserved[175];
    EntityID impIdent;
    uint8 impUse[256];
};

#define INTEGRITY_TYPE_STABLE    2

/* Extended Allocation Descriptor (ECMA 167 4/14.14.3) */
typedef struct _EXT_AD { /* ECMA 167 4/14.14.3 */
    uint32 extLength;
    uint32 recordedLength;
    uint32 informationLength;
    lb_addr extLocation;
} EXT_AD, *PEXT_AD;

typedef EXT_AD ext_ad;

/* Logical Volume Header Descriptor (ECMA 167 4/14.5) */
struct LogicalVolHeaderDesc {
    uint64 uniqueID;
    uint8 reserved[24];
};

/* Path Component (ECMA 167 4/14.16.1) */
struct PathComponent {
    uint8 componentType;
    uint8 lengthComponentIdent;
    uint16 componentFileVersionNum;
//  dstring componentIdent[0];
};

#define COMPONENT_TYPE_ROOT_X  0x01 /* originator & recipient know its value */
#define COMPONENT_TYPE_ROOT    0x02 /* root of the volume */ 
#define COMPONENT_TYPE_PARENT  0x03 /* predecessor's parent dir */
#define COMPONENT_TYPE_CURENT  0x04 /* same as predecessor's dir */
#define COMPONENT_TYPE_OBJECT  0x05 /* terminal entry */

/* File Entry (ECMA 167 4/14.17) */

typedef struct _EXTENDED_FILE_ENTRY {
    tag         descTag;
    icbtag      icbTag;
    uint32      uid;
    uint32      gid;
    uint32      permissions;
    uint16      fileLinkCount;
    uint8       recordFormat;
    uint8       recordDisplayAttr;
    uint32      recordLength;
    uint64      informationLength;
    uint64      objectSize;
    uint64      logicalBlocksRecorded;
    timestamp   accessTime;
    timestamp   modificationTime;
    timestamp   createTime;
    timestamp   attrTime;
    uint32      checkpoint;
    uint32      reserved;
    long_ad     extendedAttrICB;
    long_ad     streamDirectoryICB;
    EntityID    impIdent;
    uint64      uniqueID;
    uint32      lengthExtendedAttr;
    uint32      lengthAllocDescs;
//  uint8       extendedAttr[0];
//  uint8       allocDescs[0];
} EXTENDED_FILE_ENTRY, *PEXTENDED_FILE_ENTRY;

typedef EXTENDED_FILE_ENTRY ExtendedFileEntry;

#pragma pack(pop)

#endif /* __ECMA_167_H__ */

