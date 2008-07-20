/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Cd.h

Abstract:

    This module defines the on-disk structure of the Cdfs file system.


--*/

#ifndef _CDFS_
#define _CDFS_

//
//  Sector size on Cdrom disks is hard-coded to 2048
//

#ifndef SECTOR_SIZE
#define SECTOR_SIZE                 (2048)
#endif

#define RAW_SECTOR_SIZE             (2352)
#define SECTOR_MASK                 (SECTOR_SIZE - 1)
#define INVERSE_SECTOR_MASK         ~(SECTOR_SIZE - 1)

#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT                (11)
#endif

#define XA_SECTOR_SIZE              (2352)

//
//  Cdfs file id is a large integer.
//

typedef LARGE_INTEGER               FILE_ID;
typedef FILE_ID                     *PFILE_ID;

//
//  The following constants are values from the disk.
//

#define FIRST_VD_SECTOR             (16)

#define VOL_ID_LEN                  (5)
#define ESC_SEQ_LEN                 (3)

#define VERSION_1                   (1)

#define VD_TERMINATOR               (255)
#define VD_PRIMARY                  (1)
#define VD_SECONDARY                (2)

#define VOLUME_ID_LENGTH            (32)

//
//  Leave the following so that CdfsBoot.c will compile
//

#define CD_SECTOR_SIZE              (2048)

#define ISO_VOL_ID                  "CD001"
#define HSG_VOL_ID                  "CDROM"

#define ISO_ATTR_MULTI              0x0080
#define ISO_ATTR_DIRECTORY          0x0002

#define MIN_DIR_REC_SIZE        (sizeof( RAW_DIR_REC ) - MAX_FILE_ID_LENGTH)

#define RVD_STD_ID( r, i )      (i ?    r->StandardId       : \
                                        ((PRAW_HSG_VD) r)->StandardId )

#define RVD_DESC_TYPE( r, i )   (i ?    r->DescType         : \
                                        ((PRAW_HSG_VD) r)->DescType )

#define RVD_VERSION( r, i )     (i ?    r->Version          : \
                                        ((PRAW_HSG_VD) r)->Version )

#define RVD_LB_SIZE( r, i )     (i ?    r->LogicalBlkSzI    : \
                                        ((PRAW_HSG_VD) r)->LogicalBlkSzI )

#define RVD_VOL_SIZE( r, i )    (i ?    r->VolSpaceI      : \
                                        ((PRAW_HSG_VD) r)->VolSpaceI )

#define RVD_ROOT_DE( r, i )     (i ?    r->RootDe           : \
                                        ((PRAW_HSG_VD) r)->RootDe )

#define DE_FILE_FLAGS( iso, de ) (iso ? de->FlagsISO : de->FlagsHSG)

//
//  Data track flag for track entries in TOC
//

#define TOC_DATA_TRACK              (0x04)
#define TOC_LAST_TRACK              (0xaa)


//
//  There is considerable rearrangement of the volume descriptors for
//  ISO and HSG.  However, within each standard the same structure can
//  be used for both the primary and secondary descriptors.
//
//  Both of these structures are aligned correctly so that no
//  special macros will be needed to unpack them.
//

//
//  Declaration of length of root directory entry in volume descriptor
//

#define LEN_ROOT_DE                 (34)

//
//  Maximum length of file ID on the disk.  We allow file size beyond the ISO 9660
//  standard.
//

#define MAX_FILE_ID_LENGTH          (255)


typedef struct _RAW_ISO_VD {

    UCHAR       DescType;           // volume type: 1 = standard, 2 = coded
    UCHAR       StandardId[5];      // volume structure standard id = CD001
    UCHAR       Version;            // volume structure version number = 1
    UCHAR       VolumeFlags;        // volume flags
    UCHAR       SystemId[32];       // system identifier
    UCHAR       VolumeId[32];       // volume identifier
    UCHAR       Reserved[8];        // reserved 8 = 0
    ULONG       VolSpaceI;          // size of the volume in LBN's Intel
    ULONG       VolSpaceM;          // size of the volume in LBN's Motorola
    UCHAR       CharSet[32];        // character set bytes 0 = ASCII
    USHORT      VolSetSizeI;        // volume set size Intel
    USHORT      VolSetSizeM;        // volume set size Motorola
    USHORT      VolSeqNumI;         // volume set sequence number Intel
    USHORT      VolSeqNumM;         // volume set sequence number Motorola
    USHORT      LogicalBlkSzI;      // logical block size Intel
    USHORT      LogicalBlkSzM;      // logical block size Motorola
    ULONG       PathTableSzI;       // path table size in bytes Intel
    ULONG       PathTableSzM;       // path table size in bytes Motorola
    ULONG       PathTabLocI[2];     // LBN of 2 path tables Intel
    ULONG       PathTabLocM[2];     // LBN of 2 path tables Motorola
    UCHAR       RootDe[LEN_ROOT_DE];// dir entry of the root directory
    UCHAR       VolSetId[128];      // volume set identifier
    UCHAR       PublId[128];        // publisher identifier
    UCHAR       PreparerId[128];    // data preparer identifier
    UCHAR       AppId[128];         // application identifier
    UCHAR       Copyright[37];      // file name of copyright notice
    UCHAR       Abstract[37];       // file name of abstract
    UCHAR       Bibliograph[37];    // file name of bibliography
    UCHAR       CreateDate[17];     // volume creation date and time
    UCHAR       ModDate[17];        // volume modification date and time
    UCHAR       ExpireDate[17];     // volume expiration date and time
    UCHAR       EffectDate[17];     // volume effective date and time
    UCHAR       FileStructVer;      // file structure version number = 1
    UCHAR       Reserved3;          // reserved
    UCHAR       ResApp[512];        // reserved for application
    UCHAR       Reserved4[653];     // remainder of 2048 bytes reserved

} RAW_ISO_VD;
typedef RAW_ISO_VD *PRAW_ISO_VD;


typedef struct _RAW_HSG_VD {

    ULONG       BlkNumI;            // logical block number Intel
    ULONG       BlkNumM;            // logical block number Motorola
    UCHAR       DescType;           // volume type: 1 = standard, 2 = coded
    UCHAR       StandardId[5];      // volume structure standard id = CDROM
    UCHAR       Version;            // volume structure version number = 1
    UCHAR       VolumeFlags;        // volume flags
    UCHAR       SystemId[32];       // system identifier
    UCHAR       VolumeId[32];       // volume identifier
    UCHAR       Reserved[8];        // reserved 8 = 0
    ULONG       VolSpaceI;          // size of the volume in LBN's Intel
    ULONG       VolSpaceM;          // size of the volume in LBN's Motorola
    UCHAR       CharSet[32];        // character set bytes 0 = ASCII
    USHORT      VolSetSizeI;        // volume set size Intel
    USHORT      VolSetSizeM;        // volume set size Motorola
    USHORT      VolSeqNumI;         // volume set sequence number Intel
    USHORT      VolSeqNumM;         // volume set sequence number Motorola
    USHORT      LogicalBlkSzI;      // logical block size Intel
    USHORT      LogicalBlkSzM;      // logical block size Motorola
    ULONG       PathTableSzI;       // path table size in bytes Intel
    ULONG       PathTableSzM;       // path table size in bytes Motorola
    ULONG       PathTabLocI[4];     // LBN of 4 path tables Intel
    ULONG       PathTabLocM[4];     // LBN of 4 path tables Motorola
    UCHAR       RootDe[LEN_ROOT_DE];// dir entry of the root directory
    UCHAR       VolSetId[128];      // volume set identifier
    UCHAR       PublId[128];        // publisher identifier
    UCHAR       PreparerId[128];    // data preparer identifier
    UCHAR       AppId[128];         // application identifier
    UCHAR       Copyright[32];      // file name of copyright notice
    UCHAR       Abstract[32];       // file name of abstract
    UCHAR       CreateDate[16];     // volume creation date and time
    UCHAR       ModDate[16];        // volume modification date and time
    UCHAR       ExpireDate[16];     // volume expiration date and time
    UCHAR       EffectDate[16];     // volume effective date and time
    UCHAR       FileStructVer;      // file structure version number
    UCHAR       Reserved3;          // reserved
    UCHAR       ResApp[512];        // reserved for application
    UCHAR       Reserved4[680];     // remainder of 2048 bytes reserved

} RAW_HSG_VD;
typedef RAW_HSG_VD *PRAW_HSG_VD;


typedef struct _RAW_JOLIET_VD {

    UCHAR       DescType;           // volume type: 2 = coded
    UCHAR       StandardId[5];      // volume structure standard id = CD001
    UCHAR       Version;            // volume structure version number = 1
    UCHAR       VolumeFlags;        // volume flags
    UCHAR       SystemId[32];       // system identifier
    UCHAR       VolumeId[32];       // volume identifier
    UCHAR       Reserved[8];        // reserved 8 = 0
    ULONG       VolSpaceI;          // size of the volume in LBN's Intel
    ULONG       VolSpaceM;          // size of the volume in LBN's Motorola
    UCHAR       CharSet[32];        // character set bytes 0 = ASCII, Joliett Seq here
    USHORT      VolSetSizeI;        // volume set size Intel
    USHORT      VolSetSizeM;        // volume set size Motorola
    USHORT      VolSeqNumI;         // volume set sequence number Intel
    USHORT      VolSeqNumM;         // volume set sequence number Motorola
    USHORT      LogicalBlkSzI;      // logical block size Intel
    USHORT      LogicalBlkSzM;      // logical block size Motorola
    ULONG       PathTableSzI;       // path table size in bytes Intel
    ULONG       PathTableSzM;       // path table size in bytes Motorola
    ULONG       PathTabLocI[2];     // LBN of 2 path tables Intel
    ULONG       PathTabLocM[2];     // LBN of 2 path tables Motorola
    UCHAR       RootDe[LEN_ROOT_DE];// dir entry of the root directory
    UCHAR       VolSetId[128];      // volume set identifier
    UCHAR       PublId[128];        // publisher identifier
    UCHAR       PreparerId[128];    // data preparer identifier
    UCHAR       AppId[128];         // application identifier
    UCHAR       Copyright[37];      // file name of copyright notice
    UCHAR       Abstract[37];       // file name of abstract
    UCHAR       Bibliograph[37];    // file name of bibliography
    UCHAR       CreateDate[17];     // volume creation date and time
    UCHAR       ModDate[17];        // volume modification date and time
    UCHAR       ExpireDate[17];     // volume expiration date and time
    UCHAR       EffectDate[17];     // volume effective date and time
    UCHAR       FileStructVer;      // file structure version number = 1
    UCHAR       Reserved3;          // reserved
    UCHAR       ResApp[512];        // reserved for application
    UCHAR       Reserved4[653];     // remainder of 2048 bytes reserved

} RAW_JOLIET_VD;
typedef RAW_JOLIET_VD *PRAW_JOLIET_VD;

//
//  Macros to access the different volume descriptors.
//

#define CdRvdId(R,F) (                  \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->StandardId :   \
    ((PRAW_ISO_VD) (R))->StandardId     \
)

#define CdRvdVersion(R,F) (             \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->Version :      \
    ((PRAW_ISO_VD) (R))->Version        \
)

#define CdRvdDescType(R,F) (            \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->DescType :     \
    ((PRAW_ISO_VD) (R))->DescType       \
)

#define CdRvdEsc(R,F) (                 \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->CharSet :      \
    ((PRAW_ISO_VD) (R))->CharSet        \
)

#define CdRvdVolId(R,F) (               \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->VolumeId :     \
    ((PRAW_ISO_VD) (R))->VolumeId       \
)

#define CdRvdBlkSz(R,F) (               \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->LogicalBlkSzI :\
    ((PRAW_ISO_VD) (R))->LogicalBlkSzI  \
)

#define CdRvdPtLoc(R,F) (               \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->PathTabLocI[0]:\
    ((PRAW_ISO_VD) (R))->PathTabLocI[0] \
)

#define CdRvdPtSz(R,F) (                \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->PathTableSzI : \
    ((PRAW_ISO_VD) (R))->PathTableSzI   \
)

#define CdRvdDirent(R,F) (              \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->RootDe :       \
    ((PRAW_ISO_VD) (R))->RootDe         \
)

#define CdRvdVolSz(R,F) (               \
    FlagOn( (F), VCB_STATE_HSG ) ?      \
    ((PRAW_HSG_VD) (R))->VolSpaceI :    \
    ((PRAW_ISO_VD) (R))->VolSpaceI      \
)


//
//  This structure is used to overlay a region of a disk sector
//  to retrieve a single directory entry.  There is a difference
//  in the file flags between the ISO and HSG version and a
//  additional byte in the ISO for the offset from Greenwich time.
//
//  The disk structure is aligned on a word boundary, so any 32
//  bit fields will be represented as an array of 16 bit fields.
//

typedef struct _RAW_DIRENT {

    UCHAR       DirLen;
    UCHAR       XarLen;
    UCHAR       FileLoc[4];
    UCHAR       FileLocMot[4];
    UCHAR       DataLen[4];
    UCHAR       DataLenMot[4];
    UCHAR       RecordTime[6];
    UCHAR       FlagsHSG;
    UCHAR       FlagsISO;
    UCHAR       IntLeaveSize;
    UCHAR       IntLeaveSkip;
    UCHAR       Vssn[2];
    UCHAR       VssnMot[2];
    UCHAR       FileIdLen;
    UCHAR       FileId[MAX_FILE_ID_LENGTH];

} RAW_DIRENT;
typedef RAW_DIRENT RAW_DIR_REC;
typedef RAW_DIRENT *PRAW_DIR_REC;
typedef RAW_DIRENT *PRAW_DIRENT;

#define CD_ATTRIBUTE_HIDDEN                         (0x01)
#define CD_ATTRIBUTE_DIRECTORY                      (0x02)
#define CD_ATTRIBUTE_ASSOC                          (0x04)
#define CD_ATTRIBUTE_MULTI                          (0x80)

#define CD_BASE_YEAR                                (1900)

#define MIN_RAW_DIRENT_LEN  (FIELD_OFFSET( RAW_DIRENT, FileId ) + 1)

#define BYTE_COUNT_8_DOT_3                          (24)

#define SHORT_NAME_SHIFT                            (5)

//
//  The following macro recovers the correct flag field.
//

#define CdRawDirentFlags(IC,RD) (                   \
    FlagOn( (IC)->Vcb->VcbState, VCB_STATE_HSG) ?   \
    (RD)->FlagsHSG :                                \
    (RD)->FlagsISO                                  \
)

//
//  The following macro converts from CD time to NT time.  On ISO
//  9660 media, we now pay attention to the GMT offset (integer
//  increments of 15 minutes offset from GMT).  HSG does not record
//  this field.
//
//  The restriction to the interval [-48, 52] comes from 9660 8.4.26.1
//
//  VOID
//  CdConvertCdTimeToNtTime (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCHAR CdTime,
//      OUT PLARGE_INTEGER NtTime
//      );
//

#define GMT_OFFSET_TO_NT ((LONGLONG) 15 * 60 * 1000 * 1000 * 10)

#define CdConvertCdTimeToNtTime(IC,CD,NT) {                     \
    TIME_FIELDS _TimeField;                                     \
    CHAR GmtOffset;                                             \
    _TimeField.Year = (CSHORT) *((PCHAR) CD) + CD_BASE_YEAR;    \
    _TimeField.Month = (CSHORT) *(Add2Ptr( CD, 1, PCHAR ));     \
    _TimeField .Day = (CSHORT) *(Add2Ptr( CD, 2, PCHAR ));      \
    _TimeField.Hour = (CSHORT) *(Add2Ptr( CD, 3, PCHAR ));      \
    _TimeField.Minute = (CSHORT) *(Add2Ptr( CD, 4, PCHAR ));    \
    _TimeField.Second = (CSHORT) *(Add2Ptr( CD, 5, PCHAR ));    \
    _TimeField.Milliseconds = (CSHORT) 0;                       \
    RtlTimeFieldsToTime( &_TimeField, NT );                     \
    if (!FlagOn((IC)->Vcb->VcbState, VCB_STATE_HSG) &&          \
        ((GmtOffset = *(Add2Ptr( CD, 6, PCHAR ))) != 0 ) &&     \
        (GmtOffset >= -48 && GmtOffset <= 52)) {                \
            (NT)->QuadPart += -GmtOffset * GMT_OFFSET_TO_NT;     \
        }                                                       \
}


//
//  The on-disk representation of a Path Table entry differs between
//  the ISO version and the HSG version.  The fields are the same
//  and the same size, but the positions are different.
//

typedef struct _RAW_PATH_ISO {

    UCHAR           DirIdLen;
    UCHAR           XarLen;
    USHORT          DirLoc[2];
    USHORT          ParentNum;
    UCHAR           DirId[MAX_FILE_ID_LENGTH];

} RAW_PATH_ISO;
typedef RAW_PATH_ISO *PRAW_PATH_ISO;
typedef RAW_PATH_ISO RAW_PATH_ENTRY;
typedef RAW_PATH_ISO *PRAW_PATH_ENTRY;

typedef struct _RAW_PATH_HSG {

    USHORT          DirLoc[2];
    UCHAR           XarLen;
    UCHAR           DirIdLen;
    USHORT          ParentNum;
    UCHAR           DirId[MAX_FILE_ID_LENGTH];

} RAW_PATH_HSG;
typedef RAW_PATH_HSG *PRAW_PATH_HSG;

#define MIN_RAW_PATH_ENTRY_LEN      (FIELD_OFFSET( RAW_PATH_ENTRY, DirId ) + 1)

//
//  The following macros are used to recover the different fields of the
//  Path Table entries.  The macro to recover the disk location of the
//  directory must copy it into a different variable for alignment reasons.
//
//      CdRawPathIdLen - Length of directory name in bytes
//      CdRawPathXar - Number of Xar blocks
//      CdRawPathLoc - Address of unaligned ulong for disk offset in blocks
//

#define CdRawPathIdLen(IC, RP) (                    \
    FlagOn( (IC)->Vcb->VcbState, VCB_STATE_HSG ) ?  \
    ((PRAW_PATH_HSG) (RP))->DirIdLen :              \
    (RP)->DirIdLen                                  \
)

#define CdRawPathXar(IC, RP) (                      \
    FlagOn( (IC)->Vcb->VcbState, VCB_STATE_HSG ) ?  \
    ((PRAW_PATH_HSG) (RP))->XarLen :                \
    (RP)->XarLen                                    \
)

#define CdRawPathLoc(IC, RP) (                      \
    FlagOn( (IC)->Vcb->VcbState, VCB_STATE_HSG ) ?  \
    ((PRAW_PATH_HSG) (RP))->DirLoc :                \
    (RP)->DirLoc                                    \
)


//
//  System use are for XA data.  The following is the system use area for
//  directory entries on XA data disks.
//

typedef struct _SYSTEM_USE_XA {

    //
    //  Owner ID.  Not used in this version.
    //

    UCHAR OwnerId[4];

    //
    //  Extent attributes.  Only interested if mode2 form2 or digital audio.
    //  This is stored big endian.  We will define the attribute flags so
    //  we can ignore this fact.
    //

    USHORT Attributes;

    //
    //  XA signature.  This value must be 'XA'.
    //

    USHORT Signature;

    //
    //  File Number.
    //

    UCHAR FileNumber;

    //
    //  Not used in this version.
    //

    UCHAR Reserved[5];

} SYSTEM_USE_XA;
typedef SYSTEM_USE_XA *PSYSTEM_USE_XA;

#define SYSTEM_USE_XA_FORM1             (0x0008)
#define SYSTEM_USE_XA_FORM2             (0x0010)
#define SYSTEM_USE_XA_DA                (0x0040)

#define SYSTEM_XA_SIGNATURE             (0x4158)

typedef enum _XA_EXTENT_TYPE {

    Form1Data = 0,
    Mode2Form2Data,
    CDAudio

} XA_EXTENT_TYPE;
typedef XA_EXTENT_TYPE *PXA_EXTENT_TYPE;

#endif // _CDFS_



