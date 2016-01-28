////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef _OSTA_MISC_H_
#define _OSTA_MISC_H_

/* based on ECMA 167 structure definitions */
#include "ecma_167.h"

#pragma pack(push, 1)

/* -------- Basic types and constants ----------- */
/* UDF character set (UDF 1.50 2.1.2) */
#define UDF_CHAR_SET_TYPE   0
#define UDF_CHAR_SET_INFO   "OSTA Compressed Unicode"

#define UDF_ID_DEVELOPER    ("*WINNT " VER_STR_PRODUCT_NAME " UDF")

#define UDF_ID_DEVELOPER_ADAPTEC  "*Adaptec DirectCD"
 
/* UDF 1.02 2.2.6.4 */
struct LogicalVolIntegrityDescImpUse
{
    EntityID    impIdent;
    uint32      numFiles;
    uint32      numDirs;
    uint16      minUDFReadRev;
    uint16      minUDFWriteRev;
    uint16      maxUDFWriteRev;
};

/* UDF 1.02 2.2.7.2 */
/* LVInformation may be present in ImpUseVolDesc.impUse */
struct ImpUseVolDescImpUse
{
    charspec    LVICharset;
    dstring     logicalVolIdent[128];
    dstring     LVInfo1[36];
    dstring     LVInfo2[36];
    dstring     LVInfo3[36];
    EntityID    impIdent;
    uint8       impUse[128];
};

struct UdfPartitionMap2
{
        uint8           partitionMapType;
        uint8           partitionMapLength;
        uint8           reserved1[2];
        EntityID        partIdent;
        uint16          volSeqNum;
        uint16          partitionNum;
        uint8           reserved2[24];
};

/* UDF 1.5 2.2.8 */
struct VirtualPartitionMap
{
    uint8       partitionMapType;   /* 2 */
    uint8       partitionMapLength; /* 64 */
    uint8       reserved1[2];       /* #00 */
    EntityID    partIdent;
    uint16      volSeqNum;
    uint16      partitionNum;
    uint8       reserved2[24];      /* #00 */
};

#define UDF_VAT_FREE_ENTRY      0xffffffff

/* UDF 1.5 2.2.9 */
typedef struct _SPARABLE_PARTITION_MAP
{
    uint8       partitionMapType;   /* 2 */
    uint8       partitionMapLength; /* 64 */
    uint8       reserved1[2];       /* #00 */
    EntityID    partIdent;      /* Flags = 0 */
                        /* Id = UDF_ID_SPARABLE */
                        /* IdSuf = 2.1.5.3 */
    uint16      volSeqNum;
    uint16      partitionNum;
    uint16      packetLength;       /* 32 */
    uint8       numSparingTables;
    uint8       reserved2[1];       /* #00 */
    uint32      sizeSparingTable;
//  uint32      locSparingTable[0];
//  uint8       pad[0];
} SPARABLE_PARTITION_MAP, *PSPARABLE_PARTITION_MAP;

#define UDF_TYPE1_MAP15         0x1511U
#define UDF_VIRTUAL_MAP15       0x1512U
#define UDF_VIRTUAL_MAP20       0x2012U
#define UDF_SPARABLE_MAP15      0x1522U
#define UDF_METADATA_MAP25      0x2522U

#ifndef PACKETSIZE_UDF
#define PACKETSIZE_UDF              32
#endif //PACKETSIZE_UDF
 
/* UDF 2.5 */
typedef struct _METADATA_PARTITION_MAP
{
    uint8       partitionMapType;   /* 2 */
    uint8       partitionMapLength; /* 64 */
    uint8       reserved1[2];       /* #00 */
    EntityID    partIdent;      /* Flags = 0 */
                        /* Id = UDF_ID_METADATA */
                        /* IdSuf = 2.1.5.3 */
    uint16      volSeqNum;
    uint16      partitionNum;
    uint32      metadataFELocation;
    uint32      metadataMirrorFELocation;
    uint32      metadataBitmapFELocation;
    uint32      allocationUnit; /* blocks */
    uint16      alignmentUnit; /* blocks */
    uint8       flags;
    uint8       pad[5];
} METADATA_PARTITION_MAP, *PMETADATA_PARTITION_MAP;

#define UDF_METADATA_DUPLICATED 0x01U

/* DVD Copyright Management Info, see UDF 1.02 3.3.4.5.1.2 */
/* when ImpUseExtendedAttr.impIdent= "*UDF DVD CGMS Info" */
struct DVDCopyrightImpUse {
    uint16 headerChecksum;
    uint8  CGMSInfo;
    uint8  dataType;
    uint8  protectionSystemInfo[4];
};

/* the impUse of long_ad used in AllocDescs  - UDF 1.02 2.3.10.1 */
struct ADImpUse
{
    uint16 flags;
    uint8  impUse[4];
};

/* the impUse of long_ad used in AllocDescs  - UDF 1.02 2.3.10.1 */
struct FidADImpUse
{
    uint8  reserved[2];
    uint32 uniqueID;
};

/* UDF 1.02 2.3.10.1 */
#define UDF_EXTENT_LENGTH_MASK      0x3FFFFFFF
#define UDF_EXTENT_FLAG_MASK        0xc0000000
#define UDF_EXTENT_FLAG_ERASED      0x40000000

/* 
 * Important!  VirtualAllocationTables are 
 * very different between 1.5 and 2.0!
 */

/* ----------- 1.5 ------------- */
/* UDF 1.5 2.2.10 */
#define UDF_FILE_TYPE_VAT15     0x0U

/* UDF 1.5 2.2.10 - VAT layout: */
struct VirtualAllocationTable15 {
/*  uint32 VirtualSector[0];*/
    EntityID    ident;
    uint32  previousVATICB;
   };  
/* where number of VirtualSector's is (VATSize-36)/4 */

/* ----------- 2.0 ------------- */
/* UDF 2.0 2.2.10 */
#define UDF_FILE_TYPE_VAT20     0xf8U

/* UDF 2.0 2.2.10 (different from 1.5!) */
struct VirtualAllocationTable20 {
    uint16 lengthHeader;
    uint16 lengthImpUse;
    dstring logicalVolIdent[128];
    uint32  previousVatICBLoc;
    uint32  numFIDSFiles;
    uint32  numFIDSDirectories; /* non-parent */
    uint16  minReadRevision;
    uint16  minWriteRevision;
    uint16  maxWriteRevision;
    uint16  reserved;
/*  uint8   impUse[0];*/
/*  uint32  vatEntry[0];*/
};

/* Sparing maps, see UDF 1.5 2.2.11 */
typedef struct _SparingEntry {
    uint32  origLocation;
    uint32  mappedLocation;
} SparingEntry;

#define SPARING_LOC_AVAILABLE  0xffffffff
#define SPARING_LOC_CORRUPTED  0xfffffff0

typedef SparingEntry   SPARING_ENTRY;
typedef SPARING_ENTRY* PSPARING_ENTRY;

typedef SPARING_ENTRY  SPARING_MAP;
typedef PSPARING_ENTRY PSPARING_MAP;

/* sparing maps, see UDF 2.0 2.2.11 */
typedef struct _SPARING_TABLE {
    tag     descTag;
    EntityID sparingIdent; /* *UDF Sparing Table */
    uint16   reallocationTableLen;
    uint16   reserved;  /* #00 */
    uint32   sequenceNum;
//  SparingEntry mapEntry[0];
} SPARING_TABLE, *PSPARING_TABLE;

/* Identifier Suffixes, see EntityID */

typedef struct {
    uint16 currentRev;
    uint8  flags;
    uint8  reserved[5];
} domainIdentSuffix;

#define ENTITYID_FLAGS_HARD_RO      0x01U
#define ENTITYID_FLAGS_SOFT_RO      0x02U

typedef struct {
    uint16 currentRev;
    uint8  OSClass;
    uint8  OSIdent;
    uint8  reserved[4];
} UDFIdentSuffix;

typedef struct {
    uint8  OSClass;
    uint8  OSIdent;
    uint8  reserved[6];
} impIdentSuffix;

/* Unique ID maps, see UDF 2.0 2.2.11 */

typedef struct {
    uint32   uniqueID;
    uint32   parentLogicalBlock;
    uint32   objectLogicalBlock;
    uint16   parentPartitionReferenceNum;
    uint16   objectPartitionReferenceNum;
} UniqueIDEntry;

typedef UniqueIDEntry        UID_MAPPING_ENTRY;
typedef UID_MAPPING_ENTRY*   PUID_MAPPING_ENTRY;

typedef struct {
    EntityID ident;
    uint32   flags;
    uint32   entryCount;
    uint8    reserved[8];
//    UniqueIDEntry mapEntry[0];
} UniqueIDMappingData;

typedef UniqueIDMappingData  UID_MAPPING_TABLE;
typedef UID_MAPPING_TABLE*   PUID_MAPPING_TABLE;

/* Entity Identifiers (UDF 1.50 6.1) */
#define UDF_ID_COMPLIANT    "*OSTA UDF Compliant"
#define UDF_ID_LV_INFO      "*UDF LV Info"
#define UDF_ID_FREE_EA      "*UDF FreeEASpace"
#define UDF_ID_FREE_APP_EA  "*UDF FreeAppEASpace"
#define UDF_ID_DVD_CGMS     "*UDF DVD CGMS Info"
#define UDF_ID_OS2_EA       "*UDF OS/2 EA"
#define UDF_ID_OS2_EA_LENGTH    "*UDF OS/2 EALength"
#define UDF_ID_OS400_DIRINFO    "*UDF OS/400 DirInfo"
#define UDF_ID_MAC_VOLUME   "*UDF Mac VolumeInfo"
#define UDF_ID_MAC_FINDER   "*UDF Mac FinderInfo"
#define UDF_ID_MAC_UNIQUE   "*UDF Mac UniqueIDTable"
#define UDF_ID_MAC_RESOURCE "*UDF Mac ResourceFork"
#define UDF_ID_VIRTUAL      "*UDF Virtual Partition"
#define UDF_ID_SPARABLE     "*UDF Sparable Partition"
#define UDF_ID_METADATA     "*UDF Metadata Partition"
#define UDF_ID_ALLOC        "*UDF Virtual Alloc Tbl"
#define UDF_ID_SPARING      "*UDF Sparing Table"

/* Operating System Identifiers (UDF 1.50 6.3) */
#define UDF_OS_CLASS_UNDEF  0x00U
#define UDF_OS_CLASS_DOS    0x01U
#define UDF_OS_CLASS_OS2    0x02U
#define UDF_OS_CLASS_MAC    0x03U
#define UDF_OS_CLASS_UNIX   0x04U
#define UDF_OS_CLASS_WIN95  0x05U
#define UDF_OS_CLASS_WINNT  0x06U
#define UDF_OS_CLASS_OS400  0x07U
#define UDF_OS_CLASS_BEOS   0x08U
#define UDF_OS_CLASS_WINCE  0x09U

#define UDF_OS_ID_GENERIC   0x00U
#define UDF_OS_ID_UNDEF     0x00U
#define UDF_OS_ID_DOS       0x00U
#define UDF_OS_ID_OS2       0x00U
#define UDF_OS_ID_MAC       0x00U
#define UDF_OS_ID_UNIX      0x00U
#define UDF_OS_ID_WIN95     0x00U
#define UDF_OS_ID_WINNT     0x00U
#define UDF_OS_ID_OS400     0x00U
#define UDF_OS_ID_BEOS      0x00U

#define UDF_OS_ID_AIX       0x01U
#define UDF_OS_ID_SOLARIS   0x02U
#define UDF_OS_ID_HPUX      0x03U
#define UDF_OS_ID_IRIX      0x04U
#define UDF_OS_ID_LINUX     0x05U
#define UDF_OS_ID_MKLINUX   0x06U
#define UDF_OS_ID_FREEBSD   0x07U
#define UDF_OS_ID_NETBSD    0x08U

#define UDF_NAME_PAD      4
#define UDF_NAME_LEN      255
#define UDF_EXT_SIZE      5 // ???
#define UDF_PATH_LEN      1023
#define UDF_VOL_LABEL_LEN 32

/* Reserved file names */

#define UDF_FN_NON_ALLOCATABLE      L"Non-Allocatable Space"
#define UDF_FN_NON_ALLOCATABLE_2    L"Non-Allocatable List"

#define UDF_FN_NON_ALLOCATABLE_USER      "Non-Allocatable Space"
#define UDF_FN_NON_ALLOCATABLE_2_USER    "Non-Allocatable List"

/* Reserved system stream names */
/* METADATA bit shall be set to 1 */

#define UDF_SN_UID_MAPPING          L"*UDF Unique ID Mapping Data"
#define UDF_SN_NON_ALLOCATABLE      L"*UDF Non-Allocatable Space"
#define UDF_SN_POWER_CAL_TABLE      L"*UDF Power Cal Table"
#define UDF_SN_BACKUP               L"*UDF Backup"

/* Reserved non-system stream names */
/* METADATA bit shall be set to 0 */

#define UDF_SN_MAC_RESOURCE_FORK    L"*UDF Macintosh Resource Fork"
#define UDF_SN_OS2_EA               L"*UDF OS/2 EA"
#define UDF_SN_NT_ACL               L"*UDF NT ACL"
#define UDF_SN_UNIX_ACL             L"*UDF UNIX ACL"

#define UDF_RESERVED_NAME_HDR       L"*UDF "

/* ----------- 2.01 ------------ */
/* UDF 2.0 2.2.10 */
#define UDF_FILE_TYPE_REALTIME    0xf9U

#define TID_ADAPTEC_LOGICAL_VOL_DESC      0x9999U

#pragma pack(pop)

#endif /* _OSTA_MISC_H */
