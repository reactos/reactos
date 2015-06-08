////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 Module Name: udf_rel.h

 Abstract:

    Contains udf related structures.

 Environment:

    Both kernel and user mode

*/

#ifndef _UDF_REL_H_
#define _UDF_REL_H_

#include "Include/platform.h"
#include "ecma_167.h"

#ifdef UDF_LIMIT_DIR_SIZE
typedef uint8 uint_di;
#else //UDF_LIMIT_DIR_SIZE
typedef uint32 uint_di;
#endif //UDF_LIMIT_DIR_SIZE

typedef struct _UDFTrackMap {
    uint32 FirstLba;
    uint32 LastLba;
    uint32 NWA;
    uint32 PacketSize;
    uint32 Session;
    uint8  TrackParam;
    uint8  DataParam;
    uint8  NWA_V;

    uint8  Flags;
#define     TrackMap_AllowCopyBit_variated     0x01
#define     TrackMap_CopyBit_variated          0x02
#define     TrackMap_Try_variation             0x04
#define     TrackMap_Use_variation             0x08
#define     TrackMap_FixFPAddressing           0x10
#define     TrackMap_FixMRWAddressing          0x20

    // are used only if FixFPAddressing is enabled
    uint32 TrackFPOffset;
    uint32 PacketFPOffset;

} UDFTrackMap, *PUDFTrackMap;

typedef struct _UDFSparingData
{
    uint32     SparingLocation;
    uint16     SparingPLength;
} UDFSparingData, *PUDFSparingData;

#define PACK_MAPPING_THRESHOLD      (sizeof(EXTENT_MAP)*8)

typedef struct _EXTENT_INFO {
    uint32      Offset;
    PEXTENT_MAP Mapping;
    int64       Length;   // user data
    BOOLEAN     Modified; // mapping
    UCHAR       Flags;
/*
    UCHAR       Reserved[2];
    PVOID       Cache;
*/
} EXTENT_INFO, *PEXTENT_INFO;

#define EXTENT_FLAG_ALLOC_STD             0x00
#define EXTENT_FLAG_ALLOC_SEQUENTIAL      0x01
#define EXTENT_FLAG_ALLOC_MASK            0x03
#define EXTENT_FLAG_PREALLOCATED          0x80
#define EXTENT_FLAG_CUT_PREALLOCATED      0x40
#define EXTENT_FLAG_VERIFY                0x20
#define EXTENT_FLAG_2K_COMPAT             0x10

typedef struct _UDFPartMap
{
    uint32  UspaceBitmap; // Lba
    uint32  FspaceBitmap; // Lba
    uint32  AccessType;
    uint32  PartitionRoot;
    uint32  PartitionLen;
    uint16  PartitionType;
    uint16  PartitionNum;
    uint16  VolumeSeqNum;
} UDFPartMap, *PUDFPartMap;


#define VDS_POS_PRIMARY_VOL_DESC    0
#define VDS_POS_UNALLOC_SPACE_DESC  1
#define VDS_POS_LOGICAL_VOL_DESC    2
#define VDS_POS_PARTITION_DESC      3
#define VDS_POS_IMP_USE_VOL_DESC    4
#define VDS_POS_VOL_DESC_PTR        5
#define VDS_POS_TERMINATING_DESC    6
#define VDS_POS_RECURSION_COUNTER   7
#define VDS_POS_LENGTH              8

typedef struct _UDF_VDS_RECORD {
    uint32 block;
    uint32 volDescSeqNum;
} UDF_VDS_RECORD, *PUDF_VDS_RECORD;

#define VRS_NSR02_FOUND         0x0001
#define VRS_NSR03_FOUND         0x0002
#define VRS_ISO9660_FOUND       0x0004

#define EXTENT_MAP_GRAN         (8*sizeof(LONG_AD))
#define DIR_INDEX_MAP_GRAN      (8*sizeof(DIR_INDEX))
#define SHORT_AD_GRAN           (8*sizeof(SHORT_AD))
#define RELOC_MAP_GRAN          (8*sizeof(EXT_RELOCATION_ENTRY))
#define ALLOC_DESC_MAX_RECURSE  256

struct _UDF_FILE_INFO;
struct _DIR_INDEX_ITEM;

#define UDF_DIR_INDEX_MT PagedPool
#define UDF_FILENAME_MT PagedPool

typedef struct _HASH_ENTRY {
    uint32 hDos;                       // hash for Dos-name
    uint32 hLfn;                       // hash for Upcased Lfn
    uint32 hPosix;                     // hash for Posix Lfn
} HASH_ENTRY, *PHASH_ENTRY;

typedef struct _DIR_INDEX_HDR {
    uint_di     FirstFree;
    uint_di     LastUsed;
    uint_di     FrameCount;
    uint_di     LastFrameCount;  // in items
    uint_di     DelCount;
    EXTENT_INFO FECharge;        // file entry charge
    EXTENT_INFO FEChargeSDir;    // file entry charge for streams
    ULONG       DIFlags;
//    struct _DIR_INDEX_ITEM* FrameList[0];
} DIR_INDEX_HDR, *PDIR_INDEX_HDR;

// Initial location of directory data extent in IN_ICB
#define UDF_DI_FLAG_INIT_IN_ICB  (0x01)

/**
    This is an entry of file list. Each directory has such a list
    for fast search & modification. This structure was introduced
    because on-disk equivalents  have  variable  sizes  &  record
    formats.
*/
typedef struct _DIR_INDEX_ITEM {
    // FSD-specific data
/**
    Specifies the position of corresponding FileIdent inside  the
    directory (on-disk).
*/
    uint32 Offset;                      // File Ident offset in extent
/**
    Specifies on-disk size of FileIdent. 0 value  of  this  field
    means that given entry has no on-disk representation  (ex.  -
    pointer to the directory itself). In this case #Offset
    value must be also 0.
*/
    uint32 Length;                      // Its length
/**
    Points to file name in UnicodeString format.  NULL  value  of
    this field is treated as list terminator.
*/
    UNICODE_STRING FName;              // Filename
/**
    Specifies on-disk location of FileEntry associated  with  the
    given file. This value should be used for unopened files.
*/
    lb_addr FileEntryLoc;              // pointer to FileEntry
/**
    Cached value from FileIdent (see #FILE_IDENT_DESC)
*/
    uint8 FileCharacteristics;
/**
    Set of flags. This is intended for internal use.
    Valid flags:
    - #UDF_FI_FLAG_FI_MODIFIED\n
    Presence of this bit means that given FileIdent was  modified
    & should be flushed.
    - #UDF_FI_FLAG_SYS_ATTR\n
    Presence of this bit means that  given  entry  of  file  list
    contains valid file attributes & times in NT-specific format.
    - #UDF_FI_FLAG_FI_INTERNAL\n
    Presence of this bit means that given  entry  represents  the
    file used for internal FS purposes & must be invisible.
    - #UDF_FI_FLAG_LINKED\n
    Presence of this bit means that related  FileEntry  has  more
    than one FileIdent. It happends when we use HardLinks.
*/
    uint8 FI_Flags;                    // FileIdent-related flags
/**
    Points to FileInfo structure for  opened  files.  This  field
    must be NULL if the file is not opened.
*/
    struct _UDF_FILE_INFO* FileInfo;   // associated FileInfo (if opened)
    // search hashes
    HASH_ENTRY hashes;
    // attributes (System-specific format)
    uint32 SysAttr;
    int64 CreationTime;
    int64 LastWriteTime;
    int64 LastAccessTime;
    int64 ChangeTime;
    int64 FileSize;
    int64 AllocationSize;
} DIR_INDEX_ITEM, *PDIR_INDEX_ITEM;
/// FileIdent was  modified & should be flushed.
#define UDF_FI_FLAG_FI_MODIFIED  (0x01)
/// Given entry of file list contains valid file attributes & times in NT-specific format.
#define UDF_FI_FLAG_SYS_ATTR     (0x02)// cached flags in system-specific format
/// Given  entry  represents the file used for internal FS purposes & must be invisible
#define UDF_FI_FLAG_FI_INTERNAL  (0x04)
/// Related  FileEntry  has more than one FileIdent. It happends when we use HardLinks.
#define UDF_FI_FLAG_LINKED       (0x08)

#define UDF_FI_FLAG_DOS          (0x10)// Lfn-style name is equal to DOS-style (case insensetive)
#define UDF_FI_FLAG_KEEP_NAME    (0x20)

#define UDF_DATALOC_INFO_MT PagedPool

/**
    This structure describes  actual  data  location.  It  is  an
    analogue (and a pair in this implementation) of NTRequiredFcb
    structure.

    UDF FSD keeps list of all Dloc  structures  &  their  on-disk
    representations. Before allocating new Dloc for newly  opened
    file the FSD checks if the Dloc with the same Lba is  already
    in memory. If it is  so  the  initiator  receive  pointer  to
    existing structure instead of allocating new. This allows  to
    handle HardLiks properly. When all references to  given  Dloc
    has gone it  is  released.  In  case  of  file  deletion  the
    association between Dloc & Lba is discarded, but Dloc is  not
    released untill UDFCleanUpFile__() is called.  This  prevents
    interference between deleted files & newly  created  ones  in
    case of equal Lba of deleted & created FileEntries.

*/

typedef struct _UDF_DATALOC_INFO {

/**
    NT-specific field. As soon as NT supports HardLink concept it
    has own structure describing the file's actual data.
*/
    struct _UDFNTRequiredFCB* CommonFcb; // pointer to corresponding NtReqFcb
/**
    Describes on-disk location of  user  data.  If  the  file  is
    recorded using IN_ICB method this  structure  points  to  the
    same LBA as #FELoc, but with non-zero offset  inside  logical
    block
*/
    EXTENT_INFO DataLoc;               // user data
/**
    Describes on-disk location of  allocation  descriptors.  They
    are part of metadata  associated  with  given  file  If  this
    structure is not initialized UDF assumes that  no  allocation
    descriptors recorded for this file.  Usually  this  structure
    points to the same LBA as #FELoc, but  with  non-zero  offset
    inside  logical  block.  The  only  exception  is  for  files
    recorded using IN_ICB method (see above). In such a case this
    structure must be zero-filled
*/

    EXTENT_INFO AllocLoc;              // allocation descriptors (if any)
/**
    Describes on-disk location the  FileEntry.  This  is  on-disk
    metadata block describing file location.
*/
    EXTENT_INFO FELoc;                 // file entry location
/**
    Pointer to cached FileEntry. This field mush be valid  untill
    all file instances are cleaned up (see UDFCleanUpFile__() and
    #LinkRefCount).
*/
    tag*        FileEntry;             // file entry data
    uint32      FileEntryLen;
/**
    Set of flags. This is intended for internal use.
    Valid flags:
    - #UDF_FE_FLAG_FE_MODIFIED\n
        Presence of this bit means that given FileEntry was modified &
        should be flushed
    - #UDF_FE_FLAG_HAS_SDIR\n
        Presence of this bit means that given FileEntry has an associated
        Stream Dir.
    - #UDF_FE_FLAG_IS_SDIR\n
        Presence of this bit means that given FileEntry represents a Stream Dir.
*/
    uint32      FE_Flags;              // FileEntry flags
/**
    Counter of currently opened directory tree instances  (files)
    pointing to given data. It is introduced because UDF supports
    HardLink concept. UDF_DATALOC_INFO structure  should  not  be
    released untill this field reaches zero.
*/
    uint32      LinkRefCount;
/**
    Points to the list of files referenced  by  the  given  file.
    This field is used for directories only. Otherwise its  value
    should be NULL.
*/
    PDIR_INDEX_HDR DirIndex;           // for Directory objects only
    struct _UDF_FILE_INFO* LinkedFileInfo;
/**
    Points  to  the  FileInfo  describing   the   StreamDirectory
    associated with the given file. If the file has no associated
    StreamDirectory this field must bu NULL.
*/
    struct _UDF_FILE_INFO* SDirInfo;
} UDF_DATALOC_INFO, *PUDF_DATALOC_INFO;

/// Was modified & should be flushed
#define UDF_FE_FLAG_FE_MODIFIED  (0x01)
/// File contains Stream Dir
#define UDF_FE_FLAG_HAS_SDIR     (0x02)
/// File is a StreamDir
#define UDF_FE_FLAG_IS_SDIR      (0x04)
/// Dir was modified & should be packed
#define UDF_FE_FLAG_DIR_MODIFIED (0x08)
/// File contains pointer to Deleted Stream Dir
#define UDF_FE_FLAG_HAS_DEL_SDIR (0x10)
/// File is Deleted Stream Dir                                       
#define UDF_FE_FLAG_IS_DEL_SDIR  (0x20)
/// Dloc is being initialized, don't touch it now
#define UDF_FE_FLAG_UNDER_INIT   (0x40)
                                       

#define UDF_FILE_INFO_MT PagedPool


/**
    This structure  describes  file  location  in  the  directory
    tree.. It is an analogue (and a pair in this  implementation)
    of Fcb structure.
*/
typedef struct _UDF_FILE_INFO {
#ifdef VALIDATE_STRUCTURES
/**
    Used for debug purposes. Each valid  FileInfo  structure  has
    IntegrityTag set to zero. If the  routine  receives  FileInfo
    with invalid IntegrityTag value the break occures.
*/
    uint32       IntegrityTag;
#endif
/**
    Points to the NT structure describing file  instance  in  the
    directory tree. Each file opened by NT has  Fcb  structure  &
    associated FileInfo. If the file is opened  by  UDF  FSD  for
    internal use this field may be NULL.  Each  Fcb  has  a  back
    pointer to FileInfo, so both structures are accessable.
*/
    struct _UDFFileControlBlock* Fcb;  // pointer to corresponding Fcb (null if absent)
/**
    Points to the structure describing  actual  data  location  &
    file attributes. See #UDF_DATALOC_INFO for more information.
*/
    PUDF_DATALOC_INFO Dloc;            // actual data location descriptor
/**
    Pointer to cached FileIdent. This field mush be valid  untill
    the file is cleaned up (see UDFCleanUpFile__()).
*/
    PFILE_IDENT_DESC FileIdent;        // file ident data
/**
    Length of the buffer allocated for FileIdent.
*/
    uint32       FileIdentLen;
/**
    Points to FileInfo structure of the Parent Directory. If  the
    file has no parent directory this field must be NULL.
*/
    struct _UDF_FILE_INFO* ParentFile; // parent (directory) if any
/**
    Number of entry in the DirIndex of the parent  directory.  It
    is used  for  fast  access  &  modification  of  the  parent.
    FileInfo  with  index  equal  to  0  usually  describes   the
    directory itself (file name is '.'  &  the  parent  is  given
    directory itself). FileInfo with index  equal  to  1  usually
    describes the parent (file name is '..'). Otherwise  FileInfo
    describes a plain file or  directory.  If  the  file  has  no
    parent this field must be 0.
*/
    uint_di      Index;                // index in parent directory
/**
    Counter of open operations. Each  routine  opening  the  file
    increments this counter, each  routine  closing  the  file  -
    decrements. The FileInfo structure can't be  released  untill
    this counter reachs zero.
*/
    uint32       RefCount;             // number of references
/**
    Counter of open operations performed  for  subsequent  files.
    Each routine opening the  file  increments  this  counter  in
    parent FileInfo structure, each routine closing  the  file  -
    decrements. The FileInfo structure can't be  released  untill
    this counter reachs zero.
*/
    uint32       OpenCount;            // number of opened files in Dir
    struct _UDF_FILE_INFO* NextLinkedFile; //
    struct _UDF_FILE_INFO* PrevLinkedFile; //

    struct _FE_LIST_ENTRY*  ListPtr;
} UDF_FILE_INFO, *PUDF_FILE_INFO;

typedef struct _FE_LIST_ENTRY {
    PUDF_FILE_INFO FileInfo;
    ULONG EntryRefCount;
} FE_LIST_ENTRY, *PFE_LIST_ENTRY;

#define DOS_NAME_LEN 8
#define DOS_EXT_LEN 3
#define ILLEGAL_CHAR_MARK 0x005F
#define UNICODE_CRC_MARK  0x0023
#define UNICODE_PERIOD    0x002E
#define UNICODE_SPACE     0x0020

#define LBA_OUT_OF_EXTENT       ((LONG)(-1))
#define LBA_NOT_ALLOCATED       ((LONG)(-2))

typedef struct _EXT_RELOCATION_ENTRY {
    uint32 extLength;
    uint32 extLocation;
    uint32 extRedir;
} EXT_RELOCATION_ENTRY, *PEXT_RELOCATION_ENTRY;

typedef struct _UDF_DATALOC_INDEX {
    uint32 Lba;
    PUDF_DATALOC_INFO Dloc;
} UDF_DATALOC_INDEX, *PUDF_DATALOC_INDEX;

typedef struct _UDF_DIR_SCAN_CONTEXT {
    PUDF_FILE_INFO DirInfo;
    PDIR_INDEX_HDR hDirNdx;
    PDIR_INDEX_ITEM DirNdx;
    uint32  frame;
    uint_di j;
    uint32  d;
    uint_di i;
} UDF_DIR_SCAN_CONTEXT, *PUDF_DIR_SCAN_CONTEXT;

typedef EXT_RELOCATION_ENTRY  EXT_RELOC_MAP;
typedef PEXT_RELOCATION_ENTRY PEXT_RELOC_MAP;

typedef struct _UDF_ALLOCATION_CACHE_ITEM {
    lba_t       ParentLocation;
    uint32      Items;
    EXTENT_INFO Ext;
} UDF_ALLOCATION_CACHE_ITEM, *PUDF_ALLOCATION_CACHE_ITEM;

/*
#define MEM_DIR_HDR_TAG     (ULONG)"DirHdr"
#define MEM_DIR_NDX_TAG     (ULONG)"DirNdx"
#define MEM_DLOC_NDX_TAG    (ULONG)"DlocNdx"
#define MEM_DLOC_INF_TAG    (ULONG)"DlocInf"
#define MEM_FNAME_TAG       (ULONG)"FName"
#define MEM_FNAME16_TAG     (ULONG)"FName16"
#define MEM_FNAMECPY_TAG    (ULONG)"FNameC"
#define MEM_FE_TAG          (ULONG)"FE"
#define MEM_XFE_TAG         (ULONG)"xFE"
#define MEM_FID_TAG         (ULONG)"FID"
#define MEM_FINF_TAG        (ULONG)"FInf"
#define MEM_VATFINF_TAG     (ULONG)"FInfVat"
#define MEM_SDFINF_TAG      (ULONG)"SDirFInf"
#define MEM_EXTMAP_TAG      (ULONG)"ExtMap"
#define MEM_ALLOCDESC_TAG   (ULONG)"AllocDesc"
#define MEM_SHAD_TAG        (ULONG)"SHAD"
#define MEM_LNGAD_TAG       (ULONG)"LNGAD"
*/

#define MEM_DIR_HDR_TAG     'DirH'
#define MEM_DIR_NDX_TAG     'DirN'
#define MEM_DLOC_NDX_TAG    'Dloc'
#define MEM_DLOC_INF_TAG    'Dloc'
#define MEM_FNAME_TAG       'FNam'
#define MEM_FNAME16_TAG     'FNam'
#define MEM_FNAMECPY_TAG    'FNam'
#define MEM_FE_TAG          'FE'
#define MEM_XFE_TAG         'xFE"'
#define MEM_FID_TAG         'FID'
#define MEM_FINF_TAG        'FInf'
#define MEM_VATFINF_TAG     'FInf'
#define MEM_SDFINF_TAG      'SDir'
#define MEM_EXTMAP_TAG      'ExtM'
#define MEM_ALLOCDESC_TAG   'Allo'
#define MEM_SHAD_TAG        'SHAD'
#define MEM_LNGAD_TAG       'LNGA'
#define MEM_ALLOC_CACHE_TAG 'hcCA'

#define UDF_DEFAULT_LAST_LBA_CD     276159
#define UDF_DEFAULT_LAST_LBA_DVD    0x23053f
#define UDF_DEFAULT_FE_CHARGE       128
#define UDF_DEFAULT_FE_CHARGE_SDIR  1
#define UDF_WRITE_MAX_RETRY         4
#define UDF_READ_MAX_RETRY          4
#define UDF_READY_MAX_RETRY         5

#define ICB_FLAG_AD_DEFAULT_ALLOC_MODE     (UCHAR)(0xff)

#define UDF_INVALID_LINK_COUNT      0xffff
#define UDF_MAX_LINK_COUNT          0x7fff

#define UDF_MAX_EXTENT_LENGTH       (UDF_EXTENT_LENGTH_MASK & ~(2048-1))

#define UDF_MAX_READ_REVISION       0x0260
#define UDF_MAX_WRITE_REVISION      0x0201

#define UDF_MAX_LVID_CHAIN_LENGTH   1024
#define UDF_LVID_TTL                1024

#define UDF_NO_EXTENT_MAP           ((PEXTENT_MAP)0xffffffff)

#define UDF_FLUSH_FLAGS_LITE        (0x80000000)

#if defined UDF_DBG || defined _CONSOLE
//#define UDF_CHECK_DISK_ALLOCATION

//#define UDF_TRACK_ONDISK_ALLOCATION

//#define UDF_TRACK_ONDISK_ALLOCATION_OWNERS
//#define UDF_TRACK_ONDISK_ALLOCATION_OWNERS_INTERNAL

//#define UDF_TRACK_EXTENT_TO_MAPPING

//#define UDF_TRACK_ALLOC_FREE_EXTENT

//#define UDF_CHECK_EXTENT_SIZE_ALIGNMENT

// dependences:

#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
  #define UDF_TRACK_EXTENT_TO_MAPPING
#endif //UDF_TRACK_ALLOC_FREE_EXTENT

#endif //UDF_DBG

typedef struct _UDF_VERIFY_CTX {
    uint8*     StoredBitMap;
    ULONG      ItemCount;
    LIST_ENTRY vrfList;
    ERESOURCE  VerifyLock;
    KEVENT     vrfEvent;
    uint32     WaiterCount;
    uint32     QueuedCount;
    BOOLEAN    VInited;
} UDF_VERIFY_CTX, *PUDF_VERIFY_CTX;

#endif /* _UDF_REL_H_ */
