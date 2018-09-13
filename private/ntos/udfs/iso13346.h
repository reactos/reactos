
/***    ISO13346.H - ISO 13346 File System Disk Format
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1996
 *      All Rights Reserved
 *
 *      This file defines the ISO 13346 Data Structures.
 *
 *      The UDF file system uses these data structures to interpret the
 *      media's contents.
 *
 */

//
//  All 13346 structures are aligned on natural boundaries even though it will
//  not be obvious to the compiler.  Disable compiler smarts for the duration
//  of the ISO definitions.
//
//  As an example, the LONGAD definition is {ULONG {ULONG USHORT} UCHAR[6]} and
//  normal packing will pad out the internal NSRLBA, nevermind that the UCHAR
//  reserved field is doing exactly that.
//

#pragma pack(1)

/***    ISO 13346 Part 1: General
 *
 *
 */

/***    charspec - Character Set Specification (1/7.2.1)
 *
 */

typedef struct  CHARSPEC {
    UCHAR       Type;                   // Character Set Type (CHARSPEC_T_...)
    UCHAR       Info[63];               // Character Set Information
} CHARSPEC, *PCHARSPEC;

//  CHARSPEC_T_... - Values for charspec_Type Character Set Types (1/7.2.1.1)

#define CHARSPEC_T_CS0  0               // By Agreement
#define CHARSPEC_T_CS1  1               // Unicode (according to ISO 2022)
#define CHARSPEC_T_CS2  2               // 38 Glyphs
#define CHARSPEC_T_CS3  3               // 65 Glyphs
#define CHARSPEC_T_CS4  4               // 95 Glyphs
#define CHARSPEC_T_CS5  5               // 191 Glyphs
#define CHARSPEC_T_CS6  6               // Unicode or ISO 2022
#define CHARSPEC_T_CS7  7               // Unicode or ISO 2022
#define CHARSPEC_T_CS8  8               // 53 Glyphs

/***    timestamp - Timestamp Structure (1/7.3)
 *
 */

typedef struct  TIMESTAMP {
    SHORT       Zone:12;                // Time Zone (+-1440 minutes from CUT)
    USHORT      Type:4;                 // Timestamp Type (TIMESTAMP_T_...)
    USHORT      Year;                   // Year (1..9999)
    UCHAR       Month;                  // Month (1..12)
    UCHAR       Day;                    // Day (1..31)
    UCHAR       Hour;                   // Hour (0..23)
    UCHAR       Minute;                 // Minute (0..59)
    UCHAR       Second;                 // Second (0..59)
    UCHAR       CentiSecond;            // Centiseconds (0..99)
    UCHAR       Usec100;                // Hundreds of microseconds (0..99)
    UCHAR       Usec;                   // microseconds (0..99)
} TIMESTAMP, *PTIMESTAMP;

//  TIMESTAMP_T_... - Values for timestamp_Type (1/7.3.1)

#define TIMESTAMP_T_CUT         0       // Coordinated Universal Time
#define TIMESTAMP_T_LOCAL       1       // Local Time
#define TIMESTAMP_T_AGREEMENT   2       // Time format by agreement

//  TIMESTAMP_Z_... Values for timestamp_Zone

#define TIMESTAMP_Z_MIN         (-1440) // Minimum timezone offset (minutes)
#define TIMESTAMP_Z_MAX         ( 1440) // Maximum timezone offset (minutes)
#define TIMESTAMP_Z_NONE        (-2047) // No timezone in timestamp_Zone


/****   regid - Entity Identifier (1/7.4)
 *
 */

typedef struct  REGID {
    UCHAR       Flags;                  // Flags (REGID_F_...)
    UCHAR       Identifier[23];         // Identifier
    UCHAR       Suffix[8];              // Identifier Suffix
} REGID, *PREGID;

//  REGID_F_... - Definitions for regid_Flags bits

#define REGID_F_DIRTY           (0x01)  // Information Modified
#define REGID_F_PROTECTED       (0x02)  // Changes Locked Out

//  REGID_LENGTH_... - regid field lengths

#define REGID_LENGTH_IDENT      23      // Length of regid_Identifier (bytes)
#define REGID_LENGTH_SUFFIX     8       // Length of regid_Suffix (bytes)

//  REGID_ID_... - Values for regid_Identifier[0]

#define REGID_ID_ISO13346       (0x2B)  // regid_Identifier within ISO 13346
#define REGID_ID_NOTREGISTERED  (0x2D)  // regid_Identifier is not registered


/***    Various Structures from Parts 3 and 4 moved here for compilation.
 *
 */


/***    extentad - Extent Address Descriptor (3/7.1)
 *
 */

typedef struct  EXTENTAD {
    ULONG       Len;                    // Extent Length in Bytes
    ULONG       Lsn;                    // Extent Logical Sector Number
} EXTENTAD, *PEXTENTAD;


/***    nsr_lba - Logical Block Address (4/7.1) (lb_addr)
 *
 */

typedef struct  NSRLBA {
    ULONG       Lbn;                    // Logical Block Number
    USHORT      Partition;              // Partition Reference Number
} NSRLBA, *PNSRLBA;


/***    nsr_length - Format of a NSR allocation descriptor length field (4/14.14.1.1)
 *
 *          This is hands-down one of the most stupid things in 13346
 */

typedef struct NSRLENGTH {
    ULONG       Length:30;
    ULONG       Type:2;
} NSRLENGTH, *PNSRLENGTH;

#define NSRLENGTH_TYPE_RECORDED         0
#define NSRLENGTH_TYPE_UNRECORDED       1
#define NSRLENGTH_TYPE_UNALLOCATED      2
#define NSRLENGTH_TYPE_CONTINUATION     3


/***    Short Allocation Descriptor (4/14.14.1)
 *
 *      Note that a SHORTAD precisely overlaps a LONGAD.  Use this by defining
 *      a generic allocation descriptor structure.
 */

typedef struct  SHORTAD {
    NSRLENGTH   Length;                 // Extent Length
    ULONG       Start;                  // Extent Logical Block Number
} SHORTAD, *PSHORTAD;

typedef SHORTAD AD_GENERIC, *PAD_GENERIC;


/***    Long Allocation Descriptor (4/14.14.2)
 *
 */

typedef struct  LONGAD {
    NSRLENGTH   Length;                 // Extent Length
    NSRLBA      Start;                  // Extent Location
    UCHAR       ImpUse[6];              // Implementation Use
} LONGAD, *PLONGAD;


/***    Extended Allocation Descriptor (4/14.14.3)
 *
 */

typedef struct  EXTAD {
    NSRLENGTH   ExtentLen;              // Extent Length
    NSRLENGTH   RecordedLen;            // Recorded Length
    ULONG       InfoLen;                // Information Length
    NSRLBA      Start;                  // Extent Location
    UCHAR       ImpUse[2];              // Implementation Use
} EXTAD, *PEXTAD;

/***    ISO 13346 Part 2: Volume and Boot Block Recognition
 *
 *
 */


/***    vsd_generic - Generic Volume Structure Descriptor (2/9.1)
 *
 */

typedef struct  VSD_GENERIC {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier
    UCHAR       Version;                // Standard Version
    UCHAR       Data[2041];             // Structure Data
} VSD_GENERIC, *PVSD_GENERIC;

//  VSD_LENGTH_... - vsd field lengths

#define VSD_LENGTH_IDENT        5       // Length of regid_Identifier (bytes)

//  VSD_IDENT_... - Values for vsd_generic_Ident

#define VSD_IDENT_BEA01     "BEA01"     // Begin Extended Area
#define VSD_IDENT_TEA01     "TEA01"     // Terminate Extended Area
#define VSD_IDENT_CDROM     "CDROM"     // High Sierra Group (pre-ISO 9660)
#define VSD_IDENT_CD001     "CD001"     // ISO 9660
#define VSD_IDENT_CDW01     "CDW01"     // ECMA 168
#define VSD_IDENT_CDW02     "CDW02"     // ISO 13490
#define VSD_IDENT_NSR01     "NSR01"     // ECMA 167
#define VSD_IDENT_NSR02     "NSR02"     // ISO 13346
#define VSD_IDENT_BOOT2     "BOOT2"     // Boot Descriptor

typedef enum _VSD_IDENT {
    VsdIdentBad = 0,
    VsdIdentBEA01,
    VsdIdentTEA01,
    VsdIdentCDROM,
    VsdIdentCD001,
    VsdIdentCDW01,
    VsdIdentCDW02,
    VsdIdentNSR01,
    VsdIdentNSR02,
    VsdIdentBOOT2
} VSD_IDENT, *PVSD_IDENT;

/***    vsd_bea01 - Begin Extended Area Descriptor (2/9.2)
 *
 */

typedef struct  VSD_BEA01 {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier ('BEA01')
    UCHAR       Version;                // Standard Version
    UCHAR       Data[2041];             // Structure Data
} VSD_BEA01, *PVSD_BEA01;


/***    vsd_tea01 - Terminate Extended Area Descriptor (2/9.3)
 *
 */

typedef struct  VSD_TEA01 {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier ('TEA01')
    UCHAR       Version;                // Standard Version
    UCHAR       Data[2041];             // Structure Data
} VSD_TEA01, *PVSD_TEA01;


/***    vsd_boot2 - Boot Descriptor (2/9.4)
 *
 */

typedef struct  VSD_BOOT2 {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier ('BOOT2')
    UCHAR       Version;                // Standard Version
    UCHAR       Res8;                   // Reserved Zero
    REGID       Architecture;           // Architecture Type
    REGID       BootIdent;              // Boot Identifier
    ULONG       BootExt;                // Boot Extent Start
    ULONG       BootExtLen;             // Boot Extent Length
    ULONG       LoadAddr[2];            // Load Address
    ULONG       StartAddr[2];           // Start Address
    TIMESTAMP   Timestamp;              // Creation Time
    USHORT      Flags;                  // Flags (VSD_BOOT2_F_...)
    UCHAR       Res110[32];             // Reserved Zeros
    UCHAR       BootUse[1906];          // Boot Use
} VSD_BOOT2, *PVSD_BOOT2;

//  VSD_BOOT2_F_... - Definitions for vsd_boot2_Flags bits

#define VSD_BOOT2_F_ERASE   (0x0001)    // Ignore previous similar BOOT2 vsds

//
//  Aligning this byte offset to a sector boundary by rounding up will
//  yield the starting offset of the Volume Recognition Area (2/8.3)
//

#define VRA_BOUNDARY_LOCATION (32767 + 1)

/***    ISO 13346 Part 3: Volume Structure
 *
 *
 */

/***    destag - Descriptor Tag (3/7.1 and 4/7.2)
 *
 *      destag_Checksum = Byte sum of bytes 0-3 and 5-15 of destag.
 *
 *      destag_CRC = CRC (X**16 + X**12 + X**5 + 1)
 *
 */

typedef struct  DESTAG {
    USHORT      Ident;                  // Tag Identifier
    USHORT      Version;                // Descriptor Version
    UCHAR       Checksum;               // Tag Checksum
    UCHAR       Res5;                   // Reserved
    USHORT      Serial;                 // Tag Serial Number
    USHORT      CRC;                    // Descriptor CRC
    USHORT      CRCLen;                 // Descriptor CRC Length
    ULONG       Lbn;                    // Tag Location (Logical Block Number)
} DESTAG, *PDESTAG;

//  DESTAG_ID_... - Values for destag_Ident
//  Descriptor Tag Values from NSR Part 3 (3/7.2.1)

#define DESTAG_ID_NOTSPEC           0   // Format Not Specified
#define DESTAG_ID_NSR_PVD           1   // (3/10.1) Primary Volume Descriptor
#define DESTAG_ID_NSR_ANCHOR        2   // (3/10.2) Anchor Volume Desc Pointer
#define DESTAG_ID_NSR_VDP           3   // (3/10.3) Volume Descriptor Pointer
#define DESTAG_ID_NSR_IMPUSE        4   // (3/10.4) Implementation Use Vol Desc
#define DESTAG_ID_NSR_PART          5   // (3/10.5) Partition Descriptor
#define DESTAG_ID_NSR_LVOL          6   // (3/10.6) Logical Volume Descriptor
#define DESTAG_ID_NSR_UASD          7   // (3/10.8) Unallocated Space Desc
#define DESTAG_ID_NSR_TERM          8   // (3/10.9) Terminating Descriptor
#define DESTAG_ID_NSR_LVINTEG       9   // (3/10.10) Logical Vol Integrity Desc

#define DESTAG_ID_MINIMUM_PART3     1   // The lowest legal DESTAG in Part 3
#define DESTAG_ID_MAXIMUM_PART3     9   // The highest legal DESTAG in Part 3

//  DESTAG_ID_... - Values for destag_Ident, continued...
//  Descriptor Tag Values from NSR Part 4 (4/7.2.1)

#define DESTAG_ID_NSR_FSD           256 // (4/14.1) File Set Descriptor
#define DESTAG_ID_NSR_FID           257 // (4/14.4) File Identifier Descriptor
#define DESTAG_ID_NSR_ALLOC         258 // (4/14.5) Allocation Extent Desc
#define DESTAG_ID_NSR_ICBIND        259 // (4/14.7) ICB Indirect Entry
#define DESTAG_ID_NSR_ICBTRM        260 // (4/14.8) ICB Terminal Entry
#define DESTAG_ID_NSR_FILE          261 // (4/14.9) File Entry
#define DESTAG_ID_NSR_EA            262 // (4/14.10) Extended Attribute Header
#define DESTAG_ID_NSR_UASE          263 // (4/14.11) Unallocated Space Entry
#define DESTAG_ID_NSR_SBP           264 // (4/14.12) Space Bitmap Descriptor
#define DESTAG_ID_NSR_PINTEG        265 // (4/14.13) Partition Integrity

#define DESTAG_ID_MINIMUM_PART4     256 // The lowest legal DESTAG in Part 4
#define DESTAG_ID_MAXIMUM_PART4     265 // The highest legal DESTAG in Part 4

//  DESTAG_VER_... - Values for destag_Version (3/7.2.2)

#define DESTAG_VER_CURRENT          2   // Current Descriptor Tag Version

//  DESTAG_SERIAL_... - Values for destag_Serial (3/7.2.5)

#define DESTAG_SERIAL_NONE          0   // No Serial Number specified


/***    Anchor Points (3/8.4.2.1)
 *
 */

#define ANCHOR_SECTOR   256


/***    vsd_nsr02 - NSR02 Volume Structure Descriptor (3/9.1)
 *
 */

typedef struct  VSD_NSR02 {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier ('NSR02')
    UCHAR       Version;                // Standard Version
    UCHAR       Res7;                   // Reserved 0 Byte
    UCHAR       Data[2040];             // Structure Data
} VSD_NSR02, *PVSD_NSR02;


//  Values for vsd_nsr02_Type

#define VSD_NSR02_TYPE_0        0       // Reserved 0

//  Values for vsd_nsr02_Version

#define VSD_NSR02_VER           1       // Standard Version 1


/***    nsr_vd_generic - Generic Volume Descriptor of 512 bytes
 *
 */

typedef struct  NSR_VD_GENERIC {
    DESTAG      Destag;                 // Descriptor Tag
    ULONG       Sequence;               // Volume Descriptor Sequence Number
    UCHAR       Data20[492];            // Descriptor Data
} NSR_VD_GENERIC, *PNSR_VD_GENERIC;


/***    nsr_pvd - NSR Primary Volume Descriptor (3/10.1)
 *
 *      nsr_pvd_destag.destag_Ident = DESTAG_ID_NSR_PVD
 *
 */

typedef struct  NSR_PVD {
    DESTAG      Destag;                 // Descriptor Tag (NSR_PVD)
    ULONG       VolDescSeqNum;          // Volume Descriptor Sequence Number
    ULONG       Number;                 // Primary Volume Descriptor Number
    UCHAR       VolumeID[32];           // Volume Identifier
    USHORT      VolSetSeq;              // Volume Set Sequence Number
    USHORT      VolSetSeqMax;           // Maximum Volume Set Sequence Number
    USHORT      Level;                  // Interchange Level
    USHORT      LevelMax;               // Maximum Interchange Level
    ULONG       CharSetList;            // Character Set List (See 1/7.2.11)
    ULONG       CharSetListMax;         // Maximum Character Set List
    UCHAR       VolSetID[128];          // Volume Set Identifier
    CHARSPEC    CharsetDesc;            // Descriptor Character Set
    CHARSPEC    CharsetExplan;          // Explanatory Character Set
    EXTENTAD    Abstract;               // Volume Abstract Location
    EXTENTAD    Copyright;              // Volume Copyright Notice Location
    REGID       Application;            // Application Identifier
    TIMESTAMP   RecordTime;             // Recording Time
    REGID       ImpUseID;               // Implementation Identifier
    UCHAR       ImpUse[64];             // Implementation Use
    ULONG       Predecessor;            // Predecessor Vol Desc Seq Location
    USHORT      Flags;                  // Flags
    UCHAR       Res490[22];             // Reserved Zeros
} NSR_PVD, *PNSR_PVD;

//  NSRPVD_F_... - Definitions for nsr_pvd_Flags

#define NSRPVD_F_COMMON_VOLID   (0x0001)// Volume ID is common across Vol Set


/***    nsr_anchor - Anchor Volume Descriptor Pointer (3/10.2)
 *
 *      nsr_anchor_destag.destag_Ident = DESTAG_ID_NSR_ANCHOR
 *
 */

typedef struct  NSR_ANCHOR {
    DESTAG      Destag;                 // Descriptor Tag (NSR_ANCHOR)
    EXTENTAD    Main;                   // Main Vol Desc Sequence Location
    EXTENTAD    Reserve;                // Reserve Vol Desc Sequence Location
    UCHAR       Res32[480];             // Reserved Zeros
} NSR_ANCHOR, *PNSR_ANCHOR;


/***    nsr_vdp - Volume Descriptor Pointer (3/10.3)
 *
 *      nsr_vdp_destag.destag_Ident = DESTAG_ID_NSR_VDP
 *
 */

typedef struct  NSR_VDP {
    DESTAG      Destag;                 // Descriptor Tag (NSR_VDP)
    ULONG       VolDescSeqNum;          // Vol Desc Sequence Number
    EXTENTAD    Next;                   // Next Vol Desc Sequence Location
    UCHAR       Res28[484];             // Reserved Zeros
} NSR_VDP, *PNSR_VDP;


/***    nsr_impuse - Implementation Use Volume Descriptor (3/10.4)
 *
 *      nsr_impuse_destag.destag_Ident = DESTAG_ID_NSR_IMPUSE
 *
 */

typedef struct  NSR_IMPUSE {
    DESTAG      Destag;                 // Descriptor Tag (NSR_IMPUSE)
    ULONG       VolDescSeqNum;          // Vol Desc Sequence Number
    REGID       ImpUseID;               // Implementation Identifier
    UCHAR       ImpUse[460];            // Implementation Use
} NSR_IMPUSE, *PNSR_IMPUSE;


/***    nsr_part - Partition Descriptor (3/10.5)
 *
 *      nsr_part_destag.destag_Ident = DESTAG_ID_NSR_PART
 *
 */

typedef struct  NSR_PART {
    DESTAG      Destag;                 // Descriptor Tag (NSR_PART)
    ULONG       VolDescSeqNum;          // Vol Desc Sequence Number
    USHORT      Flags;                  // Partition Flags (NSR_PART_F_...)
    USHORT      Number;                 // Partition Number
    REGID       ContentsID;             // Partition Contents ID
    UCHAR       ContentsUse[128];       // Partition Contents Use
    ULONG       AccessType;             // Access Type
    ULONG       Start;                  // Partition Starting Location
    ULONG       Length;                 // Partition Length (sector count)
    REGID       ImpUseID;               // Implementation Identifier
    UCHAR       ImpUse[128];            // Implementation Use
    UCHAR       Res356[156];            // Reserved Zeros
} NSR_PART, *PNSR_PART;


//  NSR_PART_F_... - Definitions for nsr_part_Flags

#define NSR_PART_F_ALLOCATION   (0x0001)    // Volume Space Allocated

//  Values for nsr_part_ContentsID.regid_Identifier

#define NSR_PART_CONTID_FDC01   "+FDC01"    // ISO 9293-1987
#define NSR_PART_CONTID_CD001   "+CD001"    // ISO 9660
#define NSR_PART_CONTID_CDW01   "+CDW01"    // ECMA 168
#define NSR_PART_CONTID_CDW02   "+CDW02"    // ISO 13490
#define NSR_PART_CONTID_NSR01   "+NSR01"    // ECMA 167
#define NSR_PART_CONTID_NSR02   "+NSR02"    // ISO 13346

typedef enum NSR_PART_CONTID {
    NsrPartContIdBad = 0,
    NsrPartContIdFDC01,
    NsrPartContIdCD001,
    NsrPartContIdCDW01,
    NsrPartContIdCDW02,
    NsrPartContIdNSR01,
    NsrPartContIdNSR02
} NSR_PART_CONTID, *PNSR_PART_CONTID;

//  Values for nsr_part_AccessType

#define NSR_PART_ACCESS_NOSPEC  0       // Partition Access Unspecified
#define NSR_PART_ACCESS_RO      1       // Read Only Access
#define NSR_PART_ACCESS_WO      2       // Write-Once Access
#define NSR_PART_ACCESS_RW_PRE  3       // Read/Write with preparation
#define NSR_PART_ACCESS_RW_OVER 4       // Read/Write, fully overwritable


/***    nsr_lvol - Logical Volume Descriptor (3/10.6)
 *
 *      nsr_lvol_destag.destag_Ident = DESTAG_ID_NSR_LVOL
 *
 *      The Logical Volume Contents Use field is specified here as a
 *      File Set Descriptor Sequence (FSD) address.  See (4/3.1).
 *
 */

typedef struct  NSR_LVOL {
    DESTAG      Destag;                 // Descriptor Tag (NSR_LVOL)
    ULONG       VolDescSeqNum;          // Vol Desc Sequence Number
    CHARSPEC    Charset;                // Descriptor Character Set
    UCHAR       VolumeID[128];          // Logical Volume ID
    ULONG       BlockSize;              // Logical Block Size (in bytes)
    REGID       DomainID;               // Domain Identifier
    LONGAD      FSD;                    // Logical Volume Contents Use
    ULONG       MapTableLength;         // Map Table Length (bytes)
    ULONG       MapTableCount;          // Map Table Partition Maps Count
    REGID       ImpUseID;               // Implementaion Identifier
    UCHAR       ImpUse[128];            // Implementation Use
    EXTENTAD    Integrity;              // Integrity Sequence Extent
    UCHAR       MapTable[0];            // Partition Map Table (variant!)

//  The true length of this structure may vary!

} NSR_LVOL, *PNSR_LVOL;

#define ISONsrLvolConstantSize (FIELD_OFFSET( NSR_LVOL, MapTable ))
#define ISONsrLvolSize( L ) (QuadAlign( ISONsrLvolConstantSize + (L)->MapTableLength ))

/***    partmap_generic - Generic Partition Map (3/10.7.1)
 *
 */

typedef struct  PARTMAP_GENERIC {
    UCHAR       Type;                   // Partition Map Type
    UCHAR       Length;                 // Partition Map Length
    UCHAR       Map[0];                 // Partion Mapping (variant!)

//  The true length of this structure may vary!

} PARTMAP_GENERIC, *PPARTMAP_GENERIC;

//  Values for partmap_g_Type

#define PARTMAP_TYPE_NOTSPEC        0   // Partition Map Format Not Specified
#define PARTMAP_TYPE_PHYSICAL       1   // Partition Map in Volume Set (Type 1)
#define PARTMAP_TYPE_PROXY          2   // Partition Map by identifier (Type 2)


/***    partmap_physical - Normal (Type 1) Partition Map (3/10.7.2)
 *
 *      A Normal Partion Map specifies a partition number on a volume
 *      within the same volume set.
 *
 */

typedef struct  PARTMAP_PHYSICAL {
    UCHAR       Type;                   // Partition Map Type = 1
    UCHAR       Length;                 // Partition Map Length = 6
    USHORT      VolSetSeq;              // Partition Volume Set Sequence Number
    USHORT      Partition;              // Partition Number
} PARTMAP_PHYSICAL, *PPARTMAP_PHYSICAL;


/***    partmap_proxy - Proxy (Type 2) Partition Map (3/10.7.3)
 *
 *      A Proxy Partition Map is commonly not interchangeable.
 *
 */

typedef struct  PARTMAP_PROXY {
    UCHAR       Type;                   // Partition Map Type = 2
    UCHAR       Length;                 // Partition Map Length = 64
    UCHAR       PartID[62];             // Partition Identifier (Proxy)
} PARTMAP_PROXY, *PPARTMAP_PROXY;


/***    nsr_uasd - Unallocated Space Descriptor (3/10.8)
 *
 *      nsr_uasd_destag.destag_Ident = DESTAG_ID_NSR_UASD
 *
 *      The true length of nsr_uasd_Extents is (nsr_uasd_ExtentCount * 8), and
 *      the last logical sector of nsr_uasd_Extents is zero padded.
 *
 */

typedef struct  NSR_UASD {
    DESTAG      Destag;                 // Descriptor Tag (NSR_UASD)
    ULONG       VolDescSeqNum;          // Vol Desc Sequence Number
    ULONG       ExtentCount;            // Number of Allocation Descriptors
    EXTENTAD    Extents[0];             // Allocation Descriptors (variant!)

//  The true length of this structure may vary!
//  The true length of nsr_uasd_Extents is (nsr_uasd_ExtentCount * 8) bytes.
//  The last logical sector of nsr_uasd_Extents is zero padded.

} NSR_UASD, *PNSR_UASD;


/***    nsr_term - Terminating Descriptor (3/10.9 and 4/14.2)
 *
 *      nsr_term_destag.destag_Ident = DESTAG_ID_NSR_TERM
 *
 */

typedef struct  NSR_TERM {
    DESTAG      Destag;                 // Descriptor Tag (NSR_TERM)
    UCHAR       Res16[496];             // Reserved Zeros
} NSR_TERM, *PNSR_TERM;


/***    nsr_lvhd - Logical Volume Header Descriptor (4/14.15)
 *
 *      This descriptor is found in the Logical Volume Content Use
 *      field of a Logical Volume Integrity Descriptor.
 *
 *      This definition is moved to here to avoid forward reference.
 */

typedef struct  NSR_LVHD {
    ULONG       UniqueID[2];            // Unique ID
    UCHAR       Res8[24];               // Reserved Zeros
} NSR_LVHD, *PNSR_LVHD;


/***    nsr_integ - Logical Volume Integrity Descriptor (3/10.10)
 *
 *      nsr_integ_destag.destag_Ident = DESTAG_ID_NSR_LVINTEG
 *
 *      WARNING: WARNING: WARNING: nsr_integ is a multi-variant structure!
 *
 *      The starting address of nsr_integ_Size is not acurrate.
 *      Compensate for this nsr_integ_Size problem by adding the value of
 *      (nsr_integ_PartitionCount-1) to the ULONG ARRAY INDEX.
 *
 *      The starting address of nsr_integ_ImpUse[0] is not accurate.
 *      Compensate for this nsr_integ_ImpUse problem by adding the value of
 *      ((nsr_integ_PartitionCount-1)<<3) to the UCHAR ARRAY INDEX.
 *
 *      This descriptor is padded with zero bytes to the end of the last
 *      logical sector it occupies.
 *
 *      The Logical Volume Contents Use field is specified here as a
 *      Logical Volume Header Descriptor.  See (4/3.1) second last point.
 */

typedef struct  NSR_INTEG {
    DESTAG      Destag;                 // Descriptor Tag (NSR_LVINTEG)
    TIMESTAMP   Time;                   // Recording Date
    ULONG       Type;                   // Integrity Type (INTEG_T_...)
    EXTENTAD    Next;                   // Next Integrity Extent
    NSR_LVHD    LVHD;                   // Logical Volume Contents Use
    ULONG       PartitionCount;         // Number of Partitions
    ULONG       ImpUseLength;           // Length of Implementation Use
    ULONG       Free[1];                // Free Space Table

//  nsr_integ_Free has a variant length = (4*nsr_integ_PartitionCount)

    ULONG       Size[1];                // Size Table

//  nsr_integ_Size has a variant starting offset due to nsr_integ_Free
//  nsr_integ_Size has a variant length = (4*nsr_integ_PartitionCount)

    UCHAR       ImpUse[0];              // Implementation Use

//  nsr_integ_ImpUse has a variant starting offset due to nsr_integ_Free and
//  nsr_integ_Size.
//  nsr_integ_ImpUse has a variant length = (nsr_integ_ImpUseLength)

} NSR_INTEG, *PNSR_INTEG;

// Values for nsr_integ_Type

#define NSR_INTEG_T_OPEN        0           // Open Integrity Descriptor
#define NSR_INTEG_T_CLOSE       1           // Close Integrity Descriptor


/***    ISO 13346 Part 4: File Structure
 *
 *      See DESTAG structure in Part 3 for definitions found in (4/7.2).
 *
 */


/***    nsr_fsd - File Set Descriptor (4/14.1)
 *
 *      nsr_fsd_destag.destag_Ident = DESTAG_ID_NSR_FSD
 */

typedef struct  NSR_FSD {
    DESTAG      Destag;                     // Descriptor Tag (NSR_LVOL)
    TIMESTAMP   Time;                       // Recording Time
    USHORT      Level;                      // Interchange Level
    USHORT      LevelMax;                   // Maximum Interchange Level
    ULONG       CharSetList;                // Character Set List (See 1/7.2.11)
    ULONG       CharSetListMax;             // Maximum Character Set List
    ULONG       FileSet;                    // File Set Number
    ULONG       FileSetDesc;                // File Set Descriptor Number
    CHARSPEC    CharspecVolID;              // Volume ID Character Set
    UCHAR       VolID[128];                 // Volume ID
    CHARSPEC    CharspecFileSet;            // File Set Character Set
    UCHAR       FileSetID[32];              // File Set ID
    UCHAR       Copyright[32];              // Copyright File Name
    UCHAR       Abstract[32];               // Abstract File Name
    LONGAD      IcbRoot;                    // Root Directory ICB Address
    REGID       DomainID;                   // Domain Identifier
    LONGAD      NextExtent;                 // Next FSD Extent
    UCHAR       Res464[48];                 // Reserved Zeros
} NSR_FSD, *PNSR_FSD;


/***    nsr_part_h - Partition Header Descriptor (4/14.3)
 *
 *      No Descriptor Tag.
 *
 *      This descriptor is found in the nsr_part_ContentsUse field of
 *      an NSR02 Partition Descriptor.  See NSR_PART_CONTID_NSR02.
 *
 */

typedef struct  NSR_PART_H {
    SHORTAD     UASTable;                   // Unallocated Space Table
    SHORTAD     UASBitmap;                  // Unallocated Space Bitmap
    SHORTAD     IntegTable;                 // Integrity Table
    SHORTAD     FreedTable;                 // Freed Space Table
    SHORTAD     FreedBitmap;                // Freed Space Bitmap
    UCHAR       Res40[88];                  // Reserved Zeros
} NSR_PART_H, *PNSR_PART_H;


/***    nsr_fid - File Identifier Descriptor (4/14.4)
 *
 *      nsr_fid_destag.destag_Ident = DESTAG_ID_NSR_FID
 *
 *      WARNING: WARNING: WARNING: nsr_fid is a multi-variant structure!
 *
 *      The starting address of nsr_fid_FileID is not acurrate.
 *      Compensate for this nsr_fid_FileID problem by adding the value of
 *      (nsr_fid_ImpUseLen-1) to the UCHAR ARRAY INDEX.
 *
 *      The starting address of nsr_fid_Padding is not acurrate.
 *      Compensate for this nsr_fid_Padding problem by adding the value of
 *      (nsr_fid_ImpUseLen+nsr_fid_FileIDLen-2) to the UCHAR ARRAY INDEX.
 *
 *      The true total size of nsr_fid_s is
 *          ((38 + nsr_fid_FileIDLen + nsr_fid_ImpUseLen) + 3) & ~3)
 *
 */

typedef struct  NSR_FID {
    DESTAG      Destag;                     // Descriptor Tag (NSR_FID)
    USHORT      Version;                    // File Version Number
    UCHAR       Flags;                      // File Flags (NSR_FID_F_...)
    UCHAR       FileIDLen;                  // File ID Length
    LONGAD      Icb;                        // ICB (long) Address
    USHORT      ImpUseLen;                  // Implementation Use Length

    UCHAR       ImpUse[1];                  // Implementation Use Area

//  nsr_fid_ImpUse has a variant length = nsr_fid_ImpUseLen

    UCHAR       FileID[1];                  // File Identifier

//  nsr_fid_FileID has a variant starting offset due to nsr_fid_ImpUse
//  nsr_fid_FileID has a variant length = nsr_fid_FileIDLen

    UCHAR       Padding[1];                 // Padding

//  nsr_fid_Paddinghas a variant starting offset due to nsr_fid_ImpUse and
//  nsr_fid_FileID
//  nsr_fid_Padding has a variant length. Round up to the next ULONG boundary.

} NSR_FID, *PNSR_FID;

#define ISONsrFidConstantSize (ULONG)(FIELD_OFFSET( NSR_FID, ImpUse ))
#define ISONsrFidSize( F ) (LongAlign( ISONsrFidConstantSize + (F)->FileIDLen + (F)->ImpUseLen ))

//  NSR_FID_F_... - Definitions for nsr_fid_Flags (Characteristics, 4/14.4.3)

#define NSR_FID_F_HIDDEN        (0x01)  // Hidden Bit
#define NSR_FID_F_DIRECTORY     (0x02)  // Directory Bit
#define NSR_FID_F_DELETED       (0x04)  // Deleted Bit
#define NSR_FID_F_PARENT        (0x08)  // Parent Directory Bit

#define NSR_FID_OFFSET_FILEID   38      // Field Offset of nsr_fid_FileID[];


/***    nsr_alloc - Allocation Extent Descriptor (4/14.5)
 *
 *      nsr_alloc_destag.destag_Ident = DESTAG_ID_NSR_ALLOC
 *
 *      This descriptor is immediately followed by AllocLen bytes
 *      of allocation descriptors, which is not part of this
 *      descriptor (so CRC calculation doesn't include it).
 *
 */

typedef struct  NSR_ALLOC {
    DESTAG      Destag;                 // Descriptor Tag (NSR_ALLOC)
    ULONG       Prev;                   // Previous Allocation Descriptor
    ULONG       AllocLen;               // Length of Allocation Descriptors
} NSR_ALLOC, *PNSR_ALLOC;


/***    icbtag - Information Control Block Tag (4/14.6)
 *
 *      An ICBTAG is commonly preceeded by a Descriptor Tag (DESTAG).
 *
 */

typedef struct  ICBTAG {
    ULONG       PriorDirectCount;// Prior Direct Entry Count
    USHORT      StratType;       // Strategy Type (ICBTAG_STRAT_...)
    USHORT      StratParm;       // Strategy Parameter (2 bytes)
    USHORT      MaxEntries;      // Maximum Number of Entries in ICB
    UCHAR       Res10;           // Reserved Zero
    UCHAR       FileType;        // File Type (ICBTAG_FILE_T_...)
    NSRLBA      IcbParent;       // Parent ICB Location
    USHORT      Flags;           // ICB Flags (ICBTAG_F_...)
} ICBTAG, *PICBTAG;


//  ICBTAG_STRAT_T_... - ICB Strategy Types
//  BUGBUG: rickdew 7/31/95.  Weird strategies!  I'm guessing on names here.

#define ICBTAG_STRAT_NOTSPEC    0       // ICB Strategy Not Specified
#define ICBTAG_STRAT_TREE       1       // Strategy 1 (4/A.2) (Plain Tree)
#define ICBTAG_STRAT_MASTER     2       // Strategy 2 (4/A.3) (Master ICB)
#define ICBTAG_STRAT_BAL_TREE   3       // Strategy 3 (4/A.4) (Balanced Tree)
#define ICBTAG_STRAT_DIRECT     4       // Strategy 4 (4/A.5) (One Direct)

//  ICBTAG_FILE_T_... - Values for icbtag_FileType

#define ICBTAG_FILE_T_NOTSPEC    0      // Not Specified
#define ICBTAG_FILE_T_UASE       1      // Unallocated Space Entry
#define ICBTAG_FILE_T_PINTEG     2      // Partition Integrity Entry
#define ICBTAG_FILE_T_INDIRECT   3      // Indirect Entry
#define ICBTAG_FILE_T_DIRECTORY  4      // Directory
#define ICBTAG_FILE_T_FILE       5      // Ordinary File
#define ICBTAG_FILE_T_BLOCK_DEV  6      // Block Special Device
#define ICBTAG_FILE_T_CHAR_DEV   7      // Character Special Device
#define ICBTAG_FILE_T_XA         8      // Extended Attributes
#define ICBTAG_FILE_T_FIFO       9      // FIFO file
#define ICBTAG_FILE_T_C_ISSOCK  10      // Socket
#define ICBTAG_FILE_T_TERMINAL  11      // Terminal Entry
#define ICBTAG_FILE_T_PATHLINK  12      // Symbolic Link with a pathname

//  ICBTAG_F_... - Values for icbtag_Flags

#define ICBTAG_F_ALLOC_MASK     (0x0007)// Mask for Allocation Descriptor Info
#define ICBTAG_F_ALLOC_SHORT          0 // Short Allocation Descriptors Used
#define ICBTAG_F_ALLOC_LONG           1 // Long Allocation Descriptors Used
#define ICBTAG_F_ALLOC_EXTENDED       2 // Extended Allocation Descriptors Used
#define ICBTAG_F_ALLOC_IMMEDIATE      3 // File Data Recorded Immediately

#define ISOAllocationDescriptorSize(T) ( (T) == ICBTAG_F_ALLOC_SHORT ? sizeof(SHORTAD) : \
                                         (T) == ICBTAG_F_ALLOC_LONG ? sizeof(LONGAD) :   \
                                         sizeof(EXTAD) )

#define ICBTAG_F_SORTED         (0x0008)// Directory is Sorted (4/8.6.1)
#define ICBTAG_F_NO_RELOCATE    (0x0010)// Data is not relocateable
#define ICBTAG_F_ARCHIVE        (0x0020)// Archive Bit
#define ICBTAG_F_SETUID         (0x0040)// S_ISUID Bit
#define ICBTAG_F_SETGID         (0x0080)// S_ISGID Bit
#define ICBTAG_F_STICKY         (0x0100)// C_ISVTX Bit
#define ICBTAG_F_CONTIGUOUS     (0x0200)// File Data is Contiguous
#define ICBTAG_F_SYSTEM         (0x0400)// System Bit
#define ICBTAG_F_TRANSFORMED    (0x0800)// Data Transformed
#define ICBTAG_F_MULTIVERSIONS  (0x1000)// Multi-version Files in Directory


/***    icbind - Indirect ICB Entry (4/14.7)
 *
 */

typedef struct  ICBIND {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_ICBIND)
    ICBTAG      Icbtag;                 // ICB Tag (ICBTAG_FILE_T_INDIRECT)
    LONGAD      Icb;                    // ICB Address
} ICBIND, *PICBIND;


/***    icbtrm - Terminal ICB Entry (4/14.8)
 *
 */

typedef struct  ICBTRM {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_ICBTRM)
    ICBTAG      Icbtag;                 // ICB Tag (ICBTAG_FILE_T_TERMINAL)
} ICBTRM, *PICBTRM;


/***    icbfile - File ICB Entry (4/14.9)
 *
 *      WARNING: WARNING: WARNING: icbfile is a multi-variant structure!
 *
 *      The starting address of icbfile_Allocs is not acurrate.
 *      Compensate for this icbfile_Allocs problem by adding the value of
 *      (icbfile_XALength-1) to the UCHAR ARRAY INDEX.
 *
 *      icbfile_XALength is a multiple of 4.
 *
 */

typedef struct  ICBFILE {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_FILE)
    ICBTAG      Icbtag;                 // ICB Tag (ICBTAG_FILE_T_FILE)
    ULONG       UID;                    // User ID of file's owner
    ULONG       GID;                    // Group ID of file's owner
    ULONG       Permissions;            // File Permissions
    USHORT      LinkCount;              // File hard-link count
    UCHAR       RecordFormat;           // Record Format
    UCHAR       RecordDisplay;          // Record Display Attributes
    ULONG       RecordLength;           // Record Length
    ULONGLONG   InfoLength;             // Information Length (file size)
    ULONGLONG   BlocksRecorded;         // Logical Blocks Recorded
    TIMESTAMP   AccessTime;             // Last-Accessed Time
    TIMESTAMP   ModifyTime;             // Last-Modification Time
    TIMESTAMP   AttributeTime;          // Last-Attribute-Change Time
    ULONG       Checkpoint;             // File Checkpoint
    LONGAD      IcbEA;                  // Extended Attribute ICB
    REGID       ImpUseID;               // Implementation Use Identifier
    ULONGLONG   UniqueID;               // Unique ID
    ULONG       EALength;               // Length of Extended Attributes
    ULONG       AllocLength;            // Length of Allocation Descriptors
    UCHAR       EAs[1];                 // Extended Attributes

//  icbfile_EAs has a variant length = icbfile_EALength

    UCHAR       Allocs[0];              // Allocation Descriptors.

//  icbfile_Allocs has a variant starting offset due to icbfile_EAs.
//  icbfile_Allocs has a variant length = icbfile_AllocLen.

} ICBFILE, *PICBFILE;


//  Definitions for icbfile_Permissions (4/14.9.6)

#define ICBFILE_PERM_OTH_X  (0x00000001)    // Other: Execute OK
#define ICBFILE_PERM_OTH_W  (0x00000002)    // Other: Write OK
#define ICBFILE_PERM_OTH_R  (0x00000004)    // Other: Read OK
#define ICBFILE_PERM_OTH_A  (0x00000008)    // Other: Set Attributes OK
#define ICBFILE_PERM_OTH_D  (0x00000010)    // Other: Delete OK
#define ICBFILE_PERM_GRP_X  (0x00000020)    // Group: Execute OK
#define ICBFILE_PERM_GRP_W  (0x00000040)    // Group: Write OK
#define ICBFILE_PERM_GRP_R  (0x00000080)    // Group: Read OK
#define ICBFILE_PERM_GRP_A  (0x00000100)    // Group: Set Attributes OK
#define ICBFILE_PERM_GRP_D  (0x00000200)    // Group: Delete OK
#define ICBFILE_PERM_OWN_X  (0x00000400)    // Owner: Execute OK
#define ICBFILE_PERM_OWN_W  (0x00000800)    // Owner: Write OK
#define ICBFILE_PERM_OWN_R  (0x00001000)    // Owner: Read OK
#define ICBFILE_PERM_OWN_A  (0x00002000)    // Owner: Set Attributes OK
#define ICBFILE_PERM_OWN_D  (0x00004000)    // Owner: Delete OK

//  (4/14.9.7) Record Format
//      Skipped

//  (4/14.9.8) Record Display Attributes
//      Skipped


/***    nsr_eah - Extended Attributes Header Descriptor (4/14.10.1)
 *
 */

typedef struct  NSR_EAH {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_XA)
    ULONG       EAImp;                  // Implementation Attributes Location
    ULONG       EAApp;                  // Application Attributes Location
} NSR_EAH, *PNSR_EAH;


/***    nsr_ea_g - Generic Extended Attributes Format (4/14.10.2)
 *
 */

typedef struct  NSR_EA_GENERIC {
    ULONG       EAType;                 // Extended Attribute Type
    UCHAR       EASubType;              // Extended Attribute Sub Type
    UCHAR       Res5[3];                // Reserved Zeros
    ULONG       EALength;               // Extended Attribute Length
    UCHAR       EAData[0];              // Extended Attribute Data (variant!)

} NSR_EA_GENERIC, *PNSR_EA_GENERIC;

//
//  Extended Attribute Types (14.4.10)
//

#define EA_TYPE_CHARSET     1
#define EA_TYPE_ALTPERM     3
#define EA_TYPE_FILETIMES   5
#define EA_TYPE_INFOTIMES   6
#define EA_TYPE_DEVICESPEC  12
#define EA_TYPE_IMPUSE      2048
#define EA_TYPE_APPUSE      65536

#define EA_SUBTYPE_BASE     1


//  (4/14.10.3) Character Set Information Extended Attribute Format
//      Skipped

//  (4/14.10.4) Alternate Permissions Extended Attribute Format
//      Skipped

//  (4/14.10.5) File Times Extended Attribute Format

typedef struct  NSR_EA_FILETIMES {
    ULONG       EAType;                 // Extended Attribute Type
    UCHAR       EASubType;              // Extended Attribute Sub Type
    UCHAR       Res5[3];                // Reserved Zeros
    ULONG       EALength;               // Extended Attribute Length
    ULONG       DataLength;             // EAData Length
    ULONG       Existence;              // Specifies which times are recorded
    TIMESTAMP   Stamps[0];              // Timestamps (variant!)

} NSR_EA_FILETIMES, *PNSR_EA_FILETIMES;


//  Definitions for nsr_ea_filetimes_Existence (4/14.10.5.6)

#define EA_FILETIMES_E_CREATION     (0x00000001)
#define EA_FILETIMES_E_DELETION     (0x00000004)
#define EA_FILETIMES_E_EFFECTIVE    (0x00000008)
#define EA_FILETIMES_E_LASTBACKUP   (0x00000020)


//  (4/14.10.6) Information Times Extended Attribute Format
//
//  Exactly the same as an NSR_EA_FILETIMES

//  Definitions for nsr_ea_infotimes_Existence (4/14.10.6.6)

#define EA_INFOTIMES_E_CREATION     (0x00000001)
#define EA_INFOTIMES_E_MODIFICATION (0x00000002)
#define EA_INFOTIMES_E_EXPIRATION   (0x00000004)
#define EA_INFOTIMES_E_EFFECTIVE    (0x00000008)


//  (4/14.10.7) Device Specification Extended Attribute Format
//      Skipped

//  (4/14.10.8) Implementation Use Extended Attribute Format
//      Skipped

//  (4/14.10.9) Application Use Extended Attribute Format
//      Skipped


/***    icbuase - Unallocated Space Entry (4/14.11)
 *
 *      icbuase_destag.destag_Ident = DESTAG_ID_NSR_UASE
 *      icbuase_icbtag.icbtag_FileType = ICBTAG_FILE_T_UASE
 *
 */

typedef struct  ICBUASE {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_ICBUASE)
    ICBTAG      Icbtag;                 // ICB Tag (ICBTAG_FILE_T_UASE)
    ULONG       AllocLen;               // Allocation Descriptors Length
    UCHAR       Allocs[0];              // Allocation Descriptors (variant!)

//  The true length of this structure may vary!
//  icbuase_Allocs has a variant length = icbuase_AllocLen;

} ICBUASE, *PICBUASE;


/***    nsr_sbd - Space Bitmap Descriptor (4/14.12)
 *
 *      nsr_sbd_destag.destag_Ident = DESTAG_ID_NSR_SBD
 *
 */

typedef struct  NSR_SBD {
    DESTAG      Destag;                 // Descriptor Tag (DESTAG_ID_NSR_SBD)
    ULONG       BitCount;               // Number of bits in Space Bitmap
    ULONG       ByteCount;              // Number of bytes in Space Bitmap
    UCHAR       Bits[0];                // Space Bitmap (variant!)

//  The true length of this structure may vary!
//  nsr_sbd_Bits has a variant length = nsr_sbd_ByteCount;

} NSR_SBD, *PNSR_SBD;


/***    icbpinteg - Partition Integrity ICB Entry (4/14.13)
 *
 */

typedef struct  ICBPINTEG {
    DESTAG      Destag;                 // Descriptor Tag (ID_NSR_PINTEG)
    ICBTAG      Icbtag;                 // ICB Tag (ICBTAG_FILE_T_PINTEG)
    TIMESTAMP   Recording;              // Recording Time
    UCHAR       IntegType;              // Integrity Type (ICBPINTEG_T_...)
    UCHAR       Res49[175];             // Reserved Zeros
    REGID       ImpUseID;               // Implemetation Use Identifier
    UCHAR       ImpUse[256];            // Implemetation Use Area
} ICBPINTEG, *PICBPINTEG;

//  ICBPINTEG_T_... - Values for icbpinteg_IntegType

#define ICBPINTEG_T_OPEN        0       // Open Partition Integrity Entry
#define ICBPINTEG_T_CLOSE       1       // Close Partition Integrity Entry
#define ICBPINTEG_T_STABLE      2       // Stable Partition Integrity Entry


/***    (4/14.14.1) Short Allocation Descriptor
 ***    (4/14.14.2) Long Allocation Descriptor
 ***    (4/14.14.3) Extended Allocation Descriptor
 *
 *      See SHORTAD, LONGAD, EXTAD, already defined above.
 *
 */


/***    nsr_lvhd - Logical Volume Header Descriptor (4/14.15)
 *
 *      The definition is moved to before Logical Volume Integrity
 *      Descriptor.
 *
 */


/***    nsr_path - Path Component (4/14.16)
 *
 */

typedef struct  NSR_PATH {
    UCHAR       Type;                   // Path Component Type (NSR_PATH_T_...)
    UCHAR       CompLen;                // Path Component Length
    UCHAR       CompVer;                // Path Component Version
    UCHAR       Comp[0];                // Path Component Identifier (variant!)

//  nsr_path_Comp has a variant length = nsr_path_CompLen

} NSR_PATH, *PNSR_PATH;

//  NSR_PATH_T_... - Values for nsr_path_Type

#define NSR_PATH_T_RESERVED     0       // Reserved Value
#define NSR_PATH_T_OTHER_ROOT   1       // Another root directory, by agreement
#define NSR_PATH_T_ROOTDIR      2       // Root Directory ('\')
#define NSR_PATH_T_PARENTDIR    3       // Parent Directory ('..')
#define NSR_PATH_T_CURDIR       4       // Current Directory ('.')
#define NSR_PATH_T_FILE         5       // File


/***    ISO 13346 Part 5: Record Structure
 *
 *      Skipped
 *
 */

//
//  Restore the standard structure packing.
//

#pragma pack()

