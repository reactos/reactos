////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: struct.h
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   This file contains structure definitions for the UDF file system
*   driver. Note that all structures are prefixed with the letters
*   "UDF". The structures are all aligned using normal alignment
*   used by the compiler (typically quad-word aligned).
*
*************************************************************************/

#ifndef _UDF_STRUCTURES_H_
#define _UDF_STRUCTURES_H_


/**************************************************************************
    some useful definitions
**************************************************************************/

#include "Include/platform.h"


/**************************************************************************
 include udf related structures *here* (because we need definition of Fcb)
**************************************************************************/
#include "udf_info/udf_rel.h"

struct IRP_CONTEXT_LITE;
struct IRP_CONTEXT;

/**************************************************************************
    An array of these structures is used to describe a set of disk runs
    for non-cached I/O.  Each entry maps a contiguous range of bytes
    on disk to a transfer buffer (which may be the user buffer itself
    or a temporary scratch buffer for unaligned transfers).
**************************************************************************/

#define UDF_MAX_PARALLEL_IOS    5

struct UDF_IO_RUN {

    //  Disk byte offset and transfer size (both sector-aligned).

    LONGLONG DiskOffset;
    ULONG DiskByteCount;

    //  Final destination in the user buffer for this portion of the transfer.

    PVOID UserBuffer;

    //  Buffer where data is actually read into.  May point to UserBuffer
    //  (aligned case), an earlier scratch area in the user buffer, or
    //  a separately allocated NonPagedPool buffer.
    //
    //  TransferByteCount != 0 means post-I/O copy is required.
    //  TransferBufferOffset is the byte offset within TransferBuffer
    //  where the useful data starts.

    PVOID TransferBuffer;
    ULONG TransferByteCount;
    ULONG TransferBufferOffset;

    //  MDL for the transfer.  Points to Irp->MdlAddress for aligned
    //  transfers or to a separately allocated MDL for scratch buffers.

    PMDL TransferMdl;

    //  Virtual address passed to IoBuildPartialMdl.  For user buffer
    //  transfers this is Irp->UserBuffer + offset (original user VA).

    PVOID TransferVirtualAddress;

    //  Associated IRP created by IoMakeAssociatedIrp (multi-run case).

    PIRP SavedIrp;
};
typedef struct UDF_IO_RUN* PIO_RUN;

/**************************************************************************
    I/O context used to synchronize non-cached I/O completion.
    For synchronous requests the SyncEvent member is used;
    for asynchronous requests the Resource/ResourceThreadId members
    allow the completion routine to release the FCB resource.
**************************************************************************/

struct UDF_IO_CONTEXT {

    //  Count of outstanding IRPs (multi-run case).

    volatile LONG IrpCount;
    PIRP MasterIrp;
    volatile NTSTATUS Status;
    BOOLEAN AllocatedContext;
    /* Physical transfer length; independent of the logical async result. */
    ULONG ExpectedByteCount;

    union {

        //  Asynchronous non-cached I/O.

        struct {
            PERESOURCE Resource;
            ERESOURCE_THREAD ResourceThreadId;
            ULONG RequestedByteCount;
        };

        //  Synchronous non-cached I/O.

        KEVENT SyncEvent;
    };
};
typedef struct UDF_IO_CONTEXT* PUDF_IO_CONTEXT;

//  Keep the old name for IRP_CONTEXT compatibility.
typedef struct UDF_IO_CONTEXT IO_CONTEXT;
typedef IO_CONTEXT* PIO_CONTEXT;

struct LCB;

/**************************************************************************
    every structure has a node type, and a node size associated with it.
    The node type serves as a signature field. The size is used for
    consistency checking ...
**************************************************************************/
struct UDFIdentifier {
    NODE_TYPE_CODE NodeTypeCode;           // a 16 bit identifier for the structure
    NODE_BYTE_SIZE NodeByteSize;           // computed as sizeof(structure)
};

C_ASSERT(sizeof(struct UDFIdentifier) == offsetof(FSRTL_ADVANCED_FCB_HEADER, Flags));

/**************************************************************************
    Every open on-disk object must have a name associated with it
    This name has two components:
    (a) the path-name (prefix) that leads to this on-disk object
    (b) the name of the object itself
    Note that with multiply linked objects, a single object might be
    associated with more than one name structure.
    This UDF FSD does not correctly support multiply linked objects.

    This structure must be quad-word aligned because it is zone allocated.
**************************************************************************/
struct UDFObjectName {
    struct UDFIdentifier                       NodeIdentifier;
    uint32                              ObjectNameFlags;
    // an absolute pathname of the object is stored below
    UNICODE_STRING                      ObjectName;
};
typedef struct UDFObjectName* PtrUDFObjectName;

/**************************************************************************
    Each file open instance is represented by a context control block.
    For each successful create/open request; a file object and a CCB will
    be created.
    For open operations performed internally by the FSD, there may not
    exist file objects; but a CCB will definitely be created.

    This structure must be quad-word aligned because it is zone allocated.
**************************************************************************/
struct CCB {
    struct UDFIdentifier                       NodeIdentifier;

    // Fcb for the file being opened.

    struct FCB* Fcb;

    // Lcb for the file being opened.

    struct LCB* Lcb;

    // each CCB is associated with a file object
    PFILE_OBJECT                        FileObject;

    // Flags. Indicates flags to apply for the current open.

    ULONG Flags;

    // current index in directory is required sometimes

    ULONG                               CurrentIndex;

    // if this CCB represents a directory object open, we may
    //  need to maintain a search pattern

    UNICODE_STRING                      SearchExpression;
    HASH_ENTRY                          hashes;
};
typedef struct CCB* PCCB;

#define CCB_FLAG_IGNORE_CASE                    (0x00000004)
// the CCB has had an IRP_MJ_CLEANUP issued on it.
#define UDF_CCB_CLEANED                         (0x00000008)
#define CCB_FLAG_ALLOW_EXTENDED_DASD_IO         (0x00000010)
// the file object was opened relative to another (RelatedFileObject was
// supplied at create): its FileName holds only the final path component,
// not an absolute path. Notifications must build the full path from the
// LCB chain rather than trusting FileName.
#define CCB_FLAG_HAS_RELATED_CCB                (0x00000020)
// if an application process set the file date time, we must
//  honor that request and *not* overwrite the values at cleanup
#define UDF_CCB_ACCESS_TIME_SET                 (0x00000040)
#define UDF_CCB_MODIFY_TIME_SET                 (0x00000080)
#define UDF_CCB_CREATE_TIME_SET                 (0x00000100)
#define UDF_CCB_WRITE_TIME_SET                  (0x00000200)
#define UDF_CCB_DELETE_ON_CLOSE                 (0x00000800)
#define UDF_CCB_MATCH_ALL                       (0x00002000)
#define UDF_CCB_WILDCARD_PRESENT                (0x00004000)
#define UDF_CCB_CAN_BE_8_DOT_3                  (0x00008000)
#define UDF_CCB_ENUM_RETURN_NEXT                (0x00010000)
#define UDF_CCB_ENUM_INITIALIZED                (0x00100000)
#define UDF_CCB_ATTRIBUTES_SET                  (0x00020000)
#define CCB_FLAG_DISMOUNT_ON_CLOSE              (0x00040000)
#define CCB_FLAG_OPEN_BY_ID                     (0x01000000)
#define CCB_FLAG_OPEN_RELATIVE_BY_ID            (0x02000000)
#define CCB_FLAG_SENT_FORMAT_UNIT               (0x10000000)

struct FCB_NONPAGED {

    //  Type and size of this record must be UDF_NODE_TYPE_FCB_NONPAGED

    _Field_range_(==, UDF_NODE_TYPE_FCB_NONPAGED) NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to
    //  point to this field

    SECTION_OBJECT_POINTERS SegmentObject;

    // This is the resource structure for this Fcb.

    ERESOURCE FcbResource;

    ERESOURCE FcbPagingIoResource;

    // This is the FastMutex for this Fcb.

    FAST_MUTEX FcbMutex;

    // This is the mutex that is inserted into the FCB_ADVANCED_HEADER
    // FastMutex field

    FAST_MUTEX AdvancedFcbHeaderMutex;

    // Fast mutex for file name cache synchronization
    // Protects LCB->FileName when used as full path cache
    FAST_MUTEX FcbFastMutex;

};
typedef struct FCB_NONPAGED* PFCB_NONPAGED;

/**************************************************************************
    each open file/directory/volume is represented by a file control block.

    Each FCB can logically be divided into two:
    (a) a structure that must have a field of type FSRTL_COMMON_FCB_HEADER
         as the first field in the structure.
         This portion should also contain other structures/resources required
         by the NT Cache Manager
         We will call this structure the "NT Required" FCB. Note that this
         portion of the FCB must be allocated from non-paged pool.
    (b) the remainder of the FCB is dependent upon the particular FSD
         requirements.
         This portion of the FCB could possibly be allocated from paged
         memory, though in the UDF FSD, it will always be allocated
         from non-paged pool.

    FCB structures are protected by the MainResource as well as the
    PagingIoResource. Of course, if the FSD implementation requires
    it, we can associate other syncronization structures with the
    FCB.

    These structures must be quad-word aligned because they are zone-allocated.
**************************************************************************/

#define     UDF_NTREQ_FCB_DELETED       (0x00000004)
#define     UDF_NTREQ_FCB_MODIFIED      (0x00000008)
#define     UDF_NTREQ_FCB_VALID         (0x40000000)

/**************************************************************************/

/***************************************************/
/*****************  W A R N I N G  *****************/
/***************************************************/

/***************************************************/
/*      DO NOT FORGET TO UPDATE VCB's HEADER !     */
/***************************************************/

struct FCB_DATA {
    ULONG Reserved;
};

struct FCB_INDEX {
    ULONG Reserved;
};

struct FCB {

    union {

        struct UDFIdentifier NodeIdentifier;
        FSRTL_ADVANCED_FCB_HEADER Header;
    };

    LIST_ENTRY EofListHead;
    ULONG NtReqFCBFlags;

    // UDF related data
    PUDF_FILE_INFO                      FileInfo;
    // this FCB belongs to some mounted logical volume
    struct VCB*      Vcb;

    // FileId for this file.

    FILE_ID FileId;

    // whenever a file stream has a create/open operation performed,
    //  the Reference count below is incremented AND the OpenHandle count
    //  below is also incremented.
    //  When an IRP_MJ_CLEANUP is received, the OpenHandle count below
    //  is decremented.
    //  When an IRP_MJ_CLOSE is received, the Reference count below is
    //  decremented.
    //  When the Reference count goes down to zero, the FCB can be de-allocated.
    //  Note that a zero Reference count implies a zero OpenHandle count.
    //  But when we have mapped data, we can receive no IRP_MJ_CLOSE
    //  In this case OpenHandleCount may reach zero, but ReferenceCount may
    //  be non-zero.
    ULONG                              FcbReference;
    ULONG                              FcbCleanup;
    ULONG                              CachedOpenHandleCount;
    ULONG FcbUserReference;

    // State flags for this Fcb.

    ULONG FcbState;

    ULONG FileAttributes;

    // File name cache synchronization
    // FcbLockThread - owner of the file name cache mutex
    // FcbLockCount - reference count for cached names in LCBs
    PVOID FcbLockThread;                // Thread owning the cache mutex
    ULONG FcbLockCount;                 // Reference count for cache

    // NOTE: FCBName field REMOVED - all names are now stored in LCB.
    // Full paths are built dynamically from LCB chain via UDFBuildFullPathFromLcb().

    // Pointer to the Fcb non-paged structures.

    PFCB_NONPAGED FcbNonpaged;


    // Share access structure
    SHARE_ACCESS ShareAccess;

    // we will maintain some time information here to make our life easier
    LARGE_INTEGER                       CreationTime;
    LARGE_INTEGER                       LastAccessTime;
    LARGE_INTEGER                       LastWriteTime;
    LARGE_INTEGER                       ChangeTime;

    PVOID LazyWriteThread;

    struct FCB* ParentFcb;

    // LCB queues for parent-child relationships
    // ParentLcbQueue - LCBs linking this FCB to parent directories (for hardlinks, usually 1)
    LIST_ENTRY ParentLcbQueue;
    // ChildLcbQueue - LCBs linking child files to this directory FCB
    LIST_ENTRY ChildLcbQueue;

    // Splay tree roots for fast child LCB lookup by name.
    // Protected by this FCB's FcbResource (exclusive for insert/remove,
    // shared is NOT safe — splay rebalances on lookup).
    PRTL_SPLAY_LINKS ExactCaseRoot;
    PRTL_SPLAY_LINKS IgnoreCaseRoot;
    PRTL_SPLAY_LINKS ShortNameRoot;

    // Pointer to IrpContextLite in delayed queue.
    struct IRP_CONTEXT_LITE* IrpContextLite;

    // Actual delayed-close membership, published/removed with interlocked IO.
    LONG MetadataCloseQueued;

    //  The following field is used by the filelock module
    //  to maintain current byte range locking information.
    //  A file lock is allocated as needed.

    PFILE_LOCK FileLock;

    union{

        ULONG FcbType;
        struct FCB_DATA FcbData;
        struct FCB_INDEX FcbIndex;
    };
};
typedef struct FCB* PFCB;

#define SIZEOF_FCB_DATA     \
    (FIELD_OFFSET(struct FCB, FcbType) + sizeof(struct FCB_DATA))

#define SIZEOF_FCB_INDEX    \
    (FIELD_OFFSET(struct FCB, FcbType) + sizeof(struct FCB_INDEX))

/**************************************************************************
    the following FCBFlags values are relevant. These flag
    values are bit fields; therefore we can test whether
    a bit position is set (1) or not set (0).
**************************************************************************/
// File data is embedded in ICB (IN_ICB allocation mode)
// Requires exclusive lock for writes since data shares sector with metadata
#define     UDF_FCB_EMBEDDED_DATA                       (0x00000001)
#define     UDF_FCB_VALID                               (0x00000002)
#define     UDF_FCB_DIRECTORY                           (0x00000008)
#define     UDF_FCB_ROOT_DIRECTORY                      (0x00000010)
#define     UDF_FCB_DELETE_ON_CLOSE                     (0x00000200)
#define     UDF_FCB_MODIFIED                            (0x00000400)
#define     UDF_FCB_ACCESSED                            (0x00000800)
#define     UDF_FCB_READ_ONLY                           (0x00001000)
#define     UDF_FCB_DELAY_CLOSE                         (0x00002000)
#define     UDF_FCB_DELETED                             (0x00004000)
// Verification failed — FID/ICB data on disk does not match FCB
#define     UDF_FCB_NEEDS_VERIFICATION                  (0x00008000)
// Object not found on media during verification — stale FCB
#define     UDF_FCB_NOT_FOUND_ON_MEDIA                  (0x00010000)
#define     FCB_STATE_INITIALIZED                       (0x00020000)
#define     FCB_STATE_IN_FCB_TABLE                      (0x00040000)

#define     UDF_FCB_DELETE_PARENT                       (0x10000000)
#define     FCB_STATE_TEMPORARY                         (0x80000000)

/**************************************************************************
    A logical volume is represented with the following structure.
    This structure is allocated as part of the device extension
    for a device object that this FSD will create, to represent
    the mounted logical volume.

**************************************************************************/

enum UDFFSD_MEDIA_TYPE {
    MediaUnknown = 0,
    MediaHdd,
    MediaCdr,
    MediaCdrw,
    MediaCdrom,
    MediaZip,
    MediaFloppy,
    MediaDvdr,
    MediaDvdrw
};

//***************************************************************************
//                      LCB (Link Control Block)
//***************************************************************************

/**
    Link Control Block (LCB) - links parent directory to child file.

    Used to defer directory linkage until create operation completes successfully.
    This prevents partial creates from being visible in directory lookups.
*/
struct LCB {
    struct UDFIdentifier NodeIdentifier;      // +0x00 Node type = UDFS_NTC_LCB

    UCHAR Reserved1[4];                // +0x04 Padding for alignment

    // Links in parent FCB's ChildLcbQueue
    LIST_ENTRY ParentFcbLinks;         // +0x08 Links in parent FCB list

    // Parent directory FCB
    PFCB ParentFcb;                    // +0x18 Pointer to parent FCB

    // Links in child FCB's ParentLcbQueue
    LIST_ENTRY ChildFcbLinks;          // +0x20 Links in child FCB list

    // Child file FCB
    PFCB ChildFcb;                     // +0x30 Pointer to child FCB

    // Initial directory offset (for hard links and directory entries)
    ULONGLONG InitialOffset;           // +0x38 Initial offset

    // Reference count (incremented by CCB, decremented on cleanup)
    ULONG Reference;                   // +0x40 Reference count

    // LCB flags
    ULONG Flags;                       // +0x44 LCB flags

    // File attributes from directory entry
    ULONG FileAttributes;              // +0x48 File attributes

    UCHAR Reserved2[4];                // +0x4C Padding

    // ANSI name (for compatibility)
    STRING Name;                       // +0x50 ANSI name (size 0x10)

    UCHAR Reserved3[8];                // +0x60 Padding

    // Splay tree links for exact case name matching
    RTL_SPLAY_LINKS ExactCaseLinks;    // +0x68 Exact case splay links

    // Exact case name (preserves original case from disk)
    UNICODE_STRING ExactCaseLinkName;  // +0x80 Exact case link name

    // Splay tree links for case-insensitive name matching
    RTL_SPLAY_LINKS IgnoreCaseLinks;   // +0x90 Ignore case splay links

    // Case-insensitive name (uppercased for fast comparison)
    UNICODE_STRING IgnoreCaseLinkName; // +0xA8 Ignore case link name

    // Splay tree links for short (8.3) name matching
    RTL_SPLAY_LINKS ShortNameLinks;    // +0xB8 Short name splay links

    // Short (8.3) name for DOS compatibility
    UNICODE_STRING ShortName;          // +0xD0 Short name

    // Buffer pointer for dynamically allocated name data
    // Points to memory immediately after LCB structure in most cases
    PVOID NameBuffer;                  // +0xE0 Buffer pointer

    // Full file name (component name, not full path)
    UNICODE_STRING FileName;           // +0xE8 File name
};

typedef struct LCB* PLCB;

// LCB Flags
#define UDF_LCB_FLAG_POOL_ALLOCATED         0x00000001  // Allocated from pool (not lookaside)
#define UDF_LCB_FLAG_LINK_DELETED           0x00000002  // Link has been deleted
#define UDF_LCB_FLAG_HAS_NAME_BUFFER        0x00000004  // Name buffer needs freeing (separately allocated)
#define UDF_LCB_FLAG_DELETE_ON_CLEANUP      0x00000008  // Delete file on cleanup
#define UDF_LCB_FLAG_EXACT_CASE_IN_TREE     0x00000010  // In exact case splay tree
#define UDF_LCB_FLAG_IGNORE_CASE_IN_TREE    0x00000020  // In ignore case splay tree
#define UDF_LCB_FLAG_SHORT_NAME_IN_TREE     0x00000040  // In short name splay tree
#define UDF_LCB_FLAG_SHORT_NAME_CREATED     0x00000200  // Short name was created (bit 9)

// LCB size constants
#define UDF_LCB_BASE_SIZE               ((ULONG)sizeof(struct LCB)) 
#define UDF_LCB_LOOKASIDE_SIZE          0x158   // 344 bytes - max size for lookaside allocation
C_ASSERT(UDF_LCB_LOOKASIDE_SIZE > sizeof(struct LCB));

// LCB lookaside size - fits LCB + 16 WCHARs for short names
// NOTE: SIZEOF_LOOKASIDE_LCB must be at least UDF_LCB_LOOKASIDE_SIZE (0x158)
#define SIZEOF_LOOKASIDE_LCB            UDF_LCB_LOOKASIDE_SIZE

//***************************************************************************
//                      VCB (Volume Control Block)
//***************************************************************************

enum VCB_CONDITION {

    VcbNotMounted = 0,
    VcbMountInProgress,
    VcbMounted,
    VcbInvalid,
    VcbDismountInProgress
};

struct VCB {

    struct UDFIdentifier NodeIdentifier;

    // Condition flag for the Vcb.
    enum VCB_CONDITION VcbCondition;

    ULONG                               VcbCleanup;
    ULONG                               VcbReference;
    ULONG                               VcbUserReference;
    ULONG                               VcbResidualReference;
    ULONG                               VcbResidualUserReference;
    ERESOURCE                           FlushResource;

    // Link into queue of Vcb's in the CdData structure.  We will create a union with
    // a LONGLONG to force the Vcb to be quad-aligned.

    union {

        LIST_ENTRY VcbLinks;
        LONGLONG Alignment;
    };

    // each VCB points to a VPB structure created by the NT I/O Manager
    PVPB                                Vpb;
    // we will maintain a global list of IRP's that are pending
    //  because of a directory notify request.
    LIST_ENTRY                          NextNotifyIRP;
    // the above list is protected only by the mutex declared below
    PNOTIFY_SYNC                        NotifySync;

    // We also retain a pointer to the physical device object on which we
    // have mounted ourselves. The I/O Manager passes us a pointer to this
    // device object when requesting a mount operation.
    PDEVICE_OBJECT                      TargetDeviceObject;

    // the volume structure contains a pointer to the root directory FCB
    struct FCB* RootIndexFcb;
    struct FCB* VolumeDasdFcb;
    // the complete name of the user visible drive letter we serve
    PUCHAR                              PtrVolumePath;
    // Pointer to a stream file object created for the volume information
    // to be more easily read from secondary storage (with the support of
    // the NT Cache Manager).
/*    PFILE_OBJECT                        PtrStreamFileObject;
    // Required to use the Cache Manager.
*/
    // Volume lock file object - used in Lock/Unlock routines
    PFILE_OBJECT                        VolumeLockFileObject;
    DEVICE_TYPE                         FsDeviceType;

    //---------------
    //
    //---------------

    ULONG           BM_FlushTime;
    ULONG           BM_FlushPriod;
    ULONG           Tree_FlushTime;
    ULONG           Tree_FlushPriod;
    ULONG           SkipCountLimit;

    // File Id cache
    struct _UDFFileIDCacheItem* FileIdCache;
    ULONG           FileIdCount;

    // FS size cache
    LONGLONG        TotalAllocUnits;
    LONGLONG        FreeAllocUnits;
    LONGLONG        EstimatedFreeUnits;

    // a resource to protect the fields contained within the VCB
    ERESOURCE                           VcbResource;
    ERESOURCE                           BitMapResource1;
    // serializes growth of the UDF 2.50 metadata file
    ERESOURCE                           MetadataGrowResource;

    ERESOURCE                           DlocResource;
    ERESOURCE                           DlocResource2;
    ERESOURCE                           PreallocResource;

    // Vcb fast mutex.  This is used to synchronize the fields in the Vcb
    // when modified when the Vcb is not held exclusively.  Included here
    // are the count fields.

    // We also use this to synchronize changes to the Fcb reference field.

    FAST_MUTEX VcbMutex;
    PVOID VcbLockThread;

    // FcbTable fast mutex.  Protects Vcb->FcbTable operations (lookup,
    // insert, delete) and ensures atomicity of FCB create + init sequence.
    // Lock ordering: FcbTableMutex (outer) before VcbMutex (inner).

    FAST_MUTEX FcbTableMutex;
    PVOID FcbTableLockThread;

    //---------------
    // Physical media parameters
    //---------------

    ULONG           SectorSize;
    ULONG           SectorShift;

    ULONG SessionStartLba;
    ULONG SessionEndLba;

    // Number of last session
    ULONG           LastSession;
    ULONG           FirstTrackNum;
    ULONG           FirstTrackNumLastSes;
    ULONG           LastTrackNum;
    // First writable LBA
    ULONG           NWA;
    // sector type map
    struct _UDFTrackMap* TrackMap;
    ULONG           LastModifiedTrack;
    ULONG           LastReadTrack;
    uint32          SavedFeatures;

    UCHAR           MediaType;
    UCHAR           MediaClassEx;

    UCHAR           MRWStatus;
    BOOLEAN         BlankCD;

    ULONG           PhSerialNumber;

    BOOLEAN         CDR_Mode;
    BOOLEAN         DVD_Mode;

    PCHAR           ZBuffer;
    PCHAR           fZBuffer;
    ULONG           fZBufferSize;

    ULONG           IoErrorCounter;
    // Media change count (equal to the same field in CDFS VCB)
    ULONG           MediaChangeCount;
    ULONG           MountPhErrorCount;

    // a set of flags that might mean something useful
    uint32          VcbState;

    //---------------
    // UDF related data
    //---------------

    // Anchors LBA
#define MAX_ANCHOR_LOCATIONS 11
    ULONG           Anchor[MAX_ANCHOR_LOCATIONS];
    // Volume label
    UNICODE_STRING  VolIdent;
    BOOLEAN         VolIdentChanged;
    // Volume creation time
    int64           VolCreationTime;
    // Root & SystemStream lb_addr
    lb_addr         RootLbAddr;
    lb_addr         SysStreamLbAddr;
    // Number of partition
    USHORT          PartitionMaps;
    // Pointer to partition structures
    PUDFPartMap     Partitions;
    struct LogicalVolIntegrityDesc* LVid;
    uint32          IntegrityType;
    uint32          origIntegrityType;
    /* First OPEN write can finish before its device barrier. Other writers
     * must wait for that barrier before using the already-OPEN fast path. */
    volatile LONG   IntegrityOpenPending;
    extent_ad       LVid_loc;
    // Optional two-slot journal in unused sectors of the LVID extent.
    uint32          LvidJournalSlot[2];
    uint32          LvidJournalTarget;
    uint64          LvidJournalGeneration;
    uint8           LvidJournalLatestSlot;
    uint8           LvidJournalState; // 0 unchecked, 1 enabled, 2 unavailable
    ULONG           SerialNumber;
    // on-disk structure version control
    uint16          UdfRevision;
    uint16          minUDFReadRev;
    uint16          minUDFWriteRev;
    uint16          maxUDFWriteRev;
    // file/dir counters for Mac OS
    uint32          numFiles;
    uint32          numDirs;
    // VAT
    uint32          InitVatCount;
    uint32          VatCount;
    uint32*         Vat;
    uint32          VatPartNdx;
    PUDF_FILE_INFO  VatFileInfo;
    uint32          MetadataRef;            
    uint32          MetadataPhysRef;        
    uint32          MetadataFELoc;         
    uint32          MetadataMirrorFELoc;
    uint32          MetadataBitmapFELoc;   
    uint32          MetadataAllocUnit;      
    uint32          MetadataAlignUnit;      
    uint8           MetadataFlags;          
    PUDF_FILE_INFO  MetadataFileInfo;
    PUDF_FILE_INFO  MetadataMirrorFileInfo;
    PUDF_FILE_INFO  MetadataBitmapFileInfo;
    PCHAR           MetadataBitmap;         
    PCHAR           MetadataPendingAlloc;  // bits allocated since last successful bitmap flush
    uint32          MetadataBitmapByteCount;
    uint32          MetadataBitCount;
    uint32          MetadataFreeCount;
    uint32          MetadataHint;
    BOOLEAN         MetadataBitmapModified;
    PUDF_DELETE_PREFREE DeletePrefreeList;
    ULONG           DeletePrefreeCount;
    // sparing table
    ULONG           SparingCountFree;
    ULONG           SparingCount;
    ULONG           SparingBlockSize;
    struct _SparingEntry* SparingTable;
#define MAX_SPARING_TABLE_LOCATIONS 32
    uint32          SparingTableLoc[MAX_SPARING_TABLE_LOCATIONS];
    uint32          SparingTableCount;
    uint32          SparingTableLength;
    uint32          SparingTableModified;
    // free space bitmap
    ULONG           FSBM_ByteCount;
    // the following 2 fields are equal to NTIFS's RTL_BITMAP structure
    ULONG           FSBM_BitCount;
    PCHAR           FSBM_Bitmap;     // 0 - free, 1 - used
#ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
    PULONG          FSBM_Bitmap_owners; // 0 - free
    // -1 - used by unknown
    // other - owner's FE location
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS

    PCHAR           FSBM_OldBitmap;  // 0 - free, 1 - used
    /* A physical shrink owns BitMapResource1 until its old FE is durable.
     * The original release calls collect ranges here without making them
     * allocatable or issuing TRIM.  Only that thread may append entries. */
    PETHREAD        TruncateTxnOwner;
    PEXTENT_MAP     TruncateTxnReleased;
    ULONG           TruncateTxnCount;
    ULONG           TruncateTxnCapacity;
    NTSTATUS        TruncateTxnError;
    ULONG           BitmapModified;
    LONG            DirectoryAppendBytes; // bounded pending-directory buffers
    LONG            DirectoryFECheckpointBytes; // bounded successful-FE proofs
    LONG            DirectoryFECheckpointDisabled; // raw volume writes invalidate all proofs
    // First unreported asynchronous new-metadata write failure.
    LONG MetadataWritebackError;
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
    LONG MetadataBatchDiag[64];
    LONG DirectoryAppendDiag[16]; // private lab counters, separate ABI
#endif
    PCHAR           BSBM_Bitmap;     // 0 - normal, 1 - bad-block

    // Bitmap cache stream (per-page CcPinRead)
    PFCB                BitmapFcb;              // FCB for bitmap internal stream
    PFILE_OBJECT        BitmapStreamFileObject;  // Internal FileObject for bitmap
    struct FCB_NONPAGED        BitmapNonpaged;          // Nonpaged data (inline in VCB)
    LARGE_MCB           BitmapMcb;               // VBN -> PSN mapping for bitmap extents
    PVOID               BitmapBcb;               // BCB of currently pinned page (NULL = none)
    PUCHAR              BitmapPinnedData;        // Pointer to raw data of pinned region
    ULONG               BitmapPinnedOffset;      // Stream byte offset of pinned region
    ULONG               BitmapPinnedLength;      // Length of pinned region in bytes
    ULONG               BitmapDataOffset;        // sizeof(SPACE_BITMAP_DESC) - bit data offset in stream
    RTL_BITMAP          BitmapRtl;               // RTL_BITMAP for current pinned page
    ULONG               BitmapPageStartLbn;      // First LBN covered by current RTL_BITMAP
    ULONG               BitmapPageBitCount;      // Number of valid bits in current RTL_BITMAP
    ULONG               SectorHint;              // First LBN for free space search
    ULONG               FeSectorHint;            // Separate UDF 2.01 FE search cursor

    // pointers to Volume Descriptor Sequences
    ULONG VDS1;
    ULONG VDS1_Len;
    ULONG VDS2;
    ULONG VDS2_Len;

    ULONG           Modified;

    // System Stream Dir
    PUDF_FILE_INFO  SysSDirFileInfo;
    // Non-alloc space
    PUDF_FILE_INFO  NonAllocFileInfo;
    // Unique ID Mapping
    PUDF_FILE_INFO  UniqueIDMapFileInfo;
    // Saved location of Primary Vol Descr (used for setting Label)
    UDF_VDS_RECORD  PVolDescAddr;
    UDF_VDS_RECORD  PVolDescAddr2;
    // NSR flags
    uint32          NSRDesc;
    // File Id cache
    ULONGLONG       NextUniqueId;
    // Exclusive upper bound durably recorded in the OPEN LVID journal.
    // Only the journal-enabled layout uses this crash reservation.
    ULONGLONG       UniqueIdReservedEnd;
    // FE location cache
    PUDF_DATALOC_INDEX DlocList;
    ULONG           DlocCount;
    ULONG           DlocFreeHint; // all earlier slots are occupied
    // FS compatibility
    BOOLEAN         LowFreeSpace;
    enum UDFFSD_MEDIA_TYPE MediaTypeEx;
    ULONG           DefaultAttr;      // Default file attributes (NT-style)

    BOOLEAN         NoFreeRelocationSpaceVolumeAction;

    //
    ULONG           SparseThreshold;  // in blocks

    PUDF_ALLOCATION_CACHE_ITEM    PreallocCache;
    ULONG                         PreallocCacheMaxSize;

    uint32          CompatFlags;

    // Fcb table.  Synchronized with FcbTableMutex.

    RTL_GENERIC_TABLE FcbTable;

    // Preallocated VPB for swapout, so we are not forced to consider
    // must succeed pool.
    PVPB SwapVpb;
};

typedef struct VCB* PVCB;

// One for root
#define UDFS_BASE_RESIDUAL_REFERENCE                (4)//(6)
#define UDFS_BASE_RESIDUAL_USER_REFERENCE           (1)//(3)

// input flush flags
#define         UDF_FLUSH_FLAGS_BREAKABLE           (0x00000001)
// see also udf_rel.h
#define         UDF_FLUSH_FLAGS_LITE                (0x80000000)
// output flush flags
#define         UDF_FLUSH_FLAGS_INTERRUPTED         (0x00000001)

#define         UDF_MAX_BG_WRITERS                  16

// The Volume Device Object is an I/O system device object with a
// workqueue and an VCB record appended to the end.  There are multiple
// of these records, one for every mounted volume, and are created during
// a volume mount operation.  The work queue is for handling an overload
// of work requests to the volume.

struct VOLUME_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    // The following field tells how many requests for this volume have
    // either been enqueued to ExWorker threads or are currently being
    // serviced by ExWorker threads.  If the number goes above
    // a certain threshold, put the request on the overflow queue to be
    // executed later.

    __volatile ULONG PostedRequestCount;

    // The following field indicates the number of IRP's waiting
    // to be serviced in the overflow queue.

    ULONG OverflowQueueCount;

    // The following field contains the queue header of the overflow queue.
    // The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    // field.

    LIST_ENTRY OverflowQueue;

    // The following spinlock protects access to all the above fields.

    KSPIN_LOCK OverflowQueueSpinLock;

    // This is the file system specific volume control block.

    struct VCB Vcb;
};
typedef struct VOLUME_DEVICE_OBJECT* PVOLUME_DEVICE_OBJECT;

//  Following structure is used to track the top level request.  Each Udfs
//  Fsd and Fsp entry point will examine the top level irp location in the
//  thread local storage to determine if this request is top level and/or
//  top level Udfs.  The top level Udfs request will remember the previous
//  value and update that location with a stack location.  This location
//  can be accessed by recursive Udfs entry points.

struct THREAD_CONTEXT {

    //  UDFS signature.  Used to confirm structure on stack is valid.
    ULONG Udfs;

    //  Previous value in top-level thread location.  We restore this
    //  when done.
    PIRP SavedTopLevelIrp;

    //  Top level Udfs IrpContext.  Initial Udfs entry point on stack
    //  will store the IrpContext for the request in this stack location.
    struct IRP_CONTEXT* TopLevelIrpContext;
};
typedef struct THREAD_CONTEXT* PTHREAD_CONTEXT;

/**************************************************************************
    The IRP context encapsulates the current request. This structure is
    used in the "common" dispatch routines invoked either directly in
    the context of the original requestor, or indirectly in the context
    of a system worker thread.
**************************************************************************/
struct IRP_CONTEXT {
    struct UDFIdentifier                   NodeIdentifier;
    ULONG                           Flags;
    // copied from the IRP
    UCHAR                           MajorFunction;
    // copied from the IRP
    UCHAR                           MinorFunction;
    // to queue this IRP for asynchronous processing
    WORK_QUEUE_ITEM                 WorkQueueItem;
    // the IRP for which this context structure was created
    PIRP                            Irp;
    // the target of the request (obtained from the IRP)
    PDEVICE_OBJECT                  RealDevice;
    // if an exception occurs, we will store the code here
    NTSTATUS                        ExceptionStatus;
    // For queued close operation we save Fcb
    PFCB                            Fcb;

    // Io context for a read request.
    // Address of Fcb for teardown oplock in create case.

    union {

        PIO_CONTEXT IoContext;
        PFCB* TeardownFcb;
    };

    // Top level irp context for this thread.
    struct IRP_CONTEXT* TopLevel;

    //  Pointer to the top-level context if this IrpContext is responsible
    //  for cleaning it up.
    PTHREAD_CONTEXT ThreadContext;

    PVCB      Vcb;
};
typedef struct IRP_CONTEXT* PIRP_CONTEXT;

#define IRP_CONTEXT_FLAG_ON_STACK               (0x00000001)
#define IRP_CONTEXT_FLAG_MORE_PROCESSING        (0x00000002)
#define IRP_CONTEXT_FLAG_WAIT                   (0x00000004)
#define IRP_CONTEXT_FLAG_FORCE_POST             (0x00000008)
#define IRP_CONTEXT_FLAG_TOP_LEVEL              (0x00000010)
#define IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS         (0x00000020)
#define IRP_CONTEXT_FLAG_IN_FSP                 (0x00000040)
#define IRP_CONTEXT_FLAG_IN_TEARDOWN            (0x00000080)
#define IRP_CONTEXT_FLAG_ALLOC_IO               (0x00000100)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS         (0x00000200)
#define IRP_CONTEXT_FLAG_DEFERRED_WRITE         (0x00000400)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH          (0x00020000)
#define IRP_CONTEXT_FLAG_FULL_NAME              (0x00040000)
#define IRP_CONTEXT_FLAG_TRAIL_BACKSLASH        (0x00080000)
#define IRP_CONTEXT_FLAG_METADATA_WRITER        (0x01000000)
#define IRP_CONTEXT_FLAG_METADATA_PUBLISH       (0x02000000)
#define UDF_IRP_CONTEXT_NOT_TOP_LEVEL           (0x10000000)
#define IRP_CONTEXT_FLAG_ALLOW_MEDIA_EJECT      (0x80000000)

//  The following flags need to be cleared when a request is posted.

#define IRP_CONTEXT_FLAGS_CLEAR_ON_POST (   \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_FORCE_POST         |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP             |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//  The following flags need to be cleared when a request is retried.

#define IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY (  \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//  The following flags are set each time through the Fsp loop.

#define IRP_CONTEXT_FSP_FLAGS (             \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP                 \
)

/**************************************************************************
    Following structure is used to queue a request to the delayed close queue.
    This structure should be the minimum block allocation size.
**************************************************************************/
struct IRP_CONTEXT_LITE {
    struct UDFIdentifier                   NodeIdentifier;
    //  Fcb for the file object being closed.
    PFCB                            Fcb;
    //  List entry to attach to delayed close queue.
    LIST_ENTRY                      DelayedCloseLinks;
    //  User reference count for the file object being closed.
    ULONG                           UserReference;
    //  Real device object.  This represents the physical device closest to the media.
    PDEVICE_OBJECT                  RealDevice;
};
typedef struct IRP_CONTEXT_LITE* PIRP_CONTEXT_LITE;

/**************************************************************************
    we will store all of our global variables in one structure.
    Global variables are not specific to any mounted volume BUT
    by definition are required for successful operation of the
    FSD implementation.
**************************************************************************/
typedef struct _UDFData {

    struct UDFIdentifier               NodeIdentifier;
    // the fields in this list are protected by the following resource
    ERESOURCE                   GlobalDataResource;
    // each driver has a driver object created for it by the NT I/O Mgr.
    //  we are no exception to this rule.
    PDRIVER_OBJECT              DriverObject;
    // we will create a device object for our FSD as well ...
    //  Although not really required, it helps if a helper application
    //  writen by us wishes to send us control information via
    //  IOCTL requests ...
    PDEVICE_OBJECT              UDFDeviceObject_CD;
    PDEVICE_OBJECT              UDFDeviceObject_HDD;

    // we will keep a list of all logical volumes for our UDF FSD
    LIST_ENTRY VcbQueue;

    // the NT Cache Manager, the I/O Manager and we will conspire
    //  to bypass IRP usage using the function pointers contained
    //  in the following structure
    FAST_IO_DISPATCH            UDFFastIoDispatch;
    // The NT Cache Manager uses the following call backs to ensure
    //  correct locking hierarchy is maintained
    CACHE_MANAGER_CALLBACKS     CacheMgrCallBacks;

    // Our lookaside lists.
    NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST ObjectNameLookasideList;
    NPAGED_LOOKASIDE_LIST NonPagedFcbLookasideList;
    NPAGED_LOOKASIDE_LIST UDFNonPagedFcbLookasideList;
    PAGED_LOOKASIDE_LIST UDFFcbIndexLookasideList;
    PAGED_LOOKASIDE_LIST UDFFcbDataLookasideList;

    PAGED_LOOKASIDE_LIST CcbLookasideList;
    PAGED_LOOKASIDE_LIST LcbLookasideList;

    LIST_ENTRY AsyncCloseQueue;
    ULONG AsyncCloseCount;
    BOOLEAN FspCloseActive;
    BOOLEAN ReduceDelayedClose;
    USHORT Flags;

    // The following fields describe the deferred close file objects.

    LIST_ENTRY DelayedCloseQueue;
    ULONG DelayedCloseCount;
    ULONG MaxDelayedCloseCount;
    ULONG MinDelayedCloseCount;
    WORK_QUEUE_ITEM CloseItem;

    // Fast mutex used to lock the fields of this structure.

    PVOID UdfDataLockThread;
    FAST_MUTEX UdfDataMutex;

    WORK_QUEUE_ITEM             LicenseKeyItem;
    BOOLEAN                     LicenseKeyItemStarted;

    LARGE_INTEGER               UDFLargeZero;

    UNICODE_STRING              SavedRegPath;
    UNICODE_STRING              UnicodeStrRoot;
    UNICODE_STRING              UnicodeStrSDir;
    UNICODE_STRING              AclName;

} UDFData, *PUDFData;

#define     UDF_DATA_FLAGS_SHUTDOWN                 (0x00000001)

#define TAG_IRP_CONTEXT         'cidU'
#define TAG_IRP_CONTEXT_LITE    'lidU'
#define TAG_OBJECT_NAME         'nodU'
#define TAG_FCB_NONPAGED        'nfdU'
#define TAG_FCB                 'pfdU'
#define TAG_CCB                 'ccdU'
#define TAG_LCB                 'lcdU'
#define TAG_VPB                 'pvdU'
#define TAG_FCB_TABLE           'tfdU'
#define TAG_FILE_NAME           'nFdU'

// some valid flags for the VCB
#define         VCB_STATE_LOCKED                    (0x00000001)
#define         VCB_STATE_DISMOUNT_IN_PROGRESS      (0x00000002)
#define         VCB_STATE_MOUNTED_DIRTY             (0x00000004)
#define         VCB_STATE_SHUTDOWN                  (0x00000008)
#define         VCB_STATE_VOLUME_READ_ONLY          (0x00000010)
#define         VCB_STATE_PACKET_RUNOUT_FIXUP       (0x00000020)
#define         VCB_STATE_VPB_NOT_ON_DEVICE         (0x00000040)
#define         VCB_STATE_MEDIA_WRITE_PROTECT       (0x00000080)
#define         VCB_STATE_REMOVABLE_MEDIA           (0x00000100)
#define         UDF_VCB_FLAGS_MEDIA_LOCKED          (0x00000200)
#define         UDF_VCB_LAST_WRITE                  (0x00001000)
#define         UDF_VCB_FLAGS_TRACKMAP              (0x00002000)
#define         UDF_VCB_ASSUME_ALL_USED             (0x00004000)
#define         VCB_STATE_RMW_INITIALIZED           (0x00008000)
#define         VCB_STATE_SEQUENCE_CACHE            (0x00010000)
#define         VCB_STATE_PNP_NOTIFICATION          (0x00020000)
#define         UDF_VCB_FLAGS_RAW_DISK              (0x00040000)

#define         UDF_VCB_FLAGS_FLUSH_BREAK_REQ       (0x01000000)
#define         UDF_VCB_FLAGS_EJECT_REQ             (0x02000000)
#define         UDF_VCB_FLAGS_FORCE_SYNC_CACHE      (0x04000000)
#define         UDF_VCB_FLAGS_DEAD                  (0x20000000)  // device unexpectedly disappeared


// flags for FS Interface Compatibility
#define         UDF_VCB_IC_UPDATE_ACCESS_TIME          (0x00000001)
#define         UDF_VCB_IC_UPDATE_MODIFY_TIME          (0x00000002)
#define         UDF_VCB_IC_UPDATE_ATTR_TIME            (0x00000004)
#define         UDF_VCB_IC_UPDATE_ARCH_BIT             (0x00000008)
#define         UDF_VCB_IC_UPDATE_DIR_WRITE            (0x00000010)
#define         UDF_VCB_IC_UPDATE_DIR_READ             (0x00000020)
#define         UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME (0x00000080)
#define         UDF_VCB_IC_IGNORE_SEQUENTIAL_IO        (0x00002000)
#define         UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE    (0x00004000)
#define         UDF_VCB_IC_BAD_RW_SEEK                 (0x00008000)
#define         UDF_VCB_IC_BAD_DVD_LAST_LBA            (0x00040000)
#define         UDF_VCB_IC_SYNCCACHE_BEFORE_READ       (0x00080000)
#define         UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS  (0x00100000)

#define         UDF_VCB_IC_DIRTY_RO                    (0x04000000)

#define FILE_ID_CACHE_GRANULARITY 16
#define DLOC_LIST_GRANULARITY 16

// Some defines
#define UDFIsDvdMedia(Vcb)       (Vcb->DVD_Mode)

typedef struct _UDFFileIDCacheItem {
    FILE_ID Id;
    UNICODE_STRING FullName;
    BOOLEAN IgnoreCase;
} UDFFileIDCacheItem, *PUDFFileIDCacheItem;

#define DIRTY_PAGE_LIMIT   32

#define UDFS_FILE_SYSTEM                 ((ULONG)0x0000009BL)

#define MAXIMUM_NUMBER_TRACKS_LARGE 0xAA

typedef struct _CDROM_TOC_LARGE {

    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstTrack;
    UCHAR LastTrack;

    //
    // Track data
    //

    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS_LARGE];

} CDROM_TOC_LARGE;

#endif /* _UDF_STRUCTURES_H_ */ // has this file been included?

