////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
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
    some empty typedefs defined here so we can reference them easily
**************************************************************************/
struct _UDFIdentifier;
struct _UDFObjectName;
struct _UDFContextControlBlock;
struct _UDFNTRequiredFCB;
struct _UDFDiskDependentFCB;
struct _UDFFileControlBlock;
struct _UDFVolumeControlBlock;
struct _UDFIrpContext;
struct _UDFIrpContextLite;
struct _UDF_FILE_INFO;
struct _UDFData;
struct _UDFEjectWaitContext;
struct _UDFFileIDCacheItem;
struct _SparingEntry;
struct _UDFTrackMap;

/**************************************************************************
 include udf related structures *here* (because we need definition of Fcb)
**************************************************************************/
#include "udf_info/udf_rel.h"

/**************************************************************************
    each structure has a unique "node type" or signature associated with it
**************************************************************************/
#define UDF_NODE_TYPE_NT_REQ_FCB            ((CSHORT)(0xfcb0))
#define UDF_NODE_TYPE_OBJECT_NAME           (0xfdecba01)
#define UDF_NODE_TYPE_CCB                   (0xfdecba02)
#define UDF_NODE_TYPE_FCB                   (0xfdecba03)
#define UDF_NODE_TYPE_VCB                   (0xfdecba04)
#define UDF_NODE_TYPE_IRP_CONTEXT           (0xfdecba05)
#define UDF_NODE_TYPE_GLOBAL_DATA           (0xfdecba06)
#define UDF_NODE_TYPE_FILTER_DEVOBJ         (0xfdecba07)
#define UDF_NODE_TYPE_UDFFS_DEVOBJ          (0xfdecba08)
#define UDF_NODE_TYPE_IRP_CONTEXT_LITE      (0xfdecba09)
#define UDF_NODE_TYPE_UDFFS_DRVOBJ          (0xfdecba0a)

/**************************************************************************
    every structure has a node type, and a node size associated with it.
    The node type serves as a signature field. The size is used for
    consistency checking ...
**************************************************************************/
typedef struct _UDFIdentifier {
    uint32      NodeType;           // a 32 bit identifier for the structure
    uint32      NodeSize;           // computed as sizeof(structure)
} UDFIdentifier, *PtrUDFIdentifier;

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
typedef struct _UDFObjectName {
    UDFIdentifier                       NodeIdentifier;
    uint32                              ObjectNameFlags;
    // an absolute pathname of the object is stored below
    UNICODE_STRING                      ObjectName;
} UDFObjectName, *PtrUDFObjectName;

#define     UDF_OBJ_NAME_NOT_FROM_ZONE               (0x80000000)

/**************************************************************************
    Each file open instance is represented by a context control block.
    For each successful create/open request; a file object and a CCB will
    be created.
    For open operations performed internally by the FSD, there may not
    exist file objects; but a CCB will definitely be created.

    This structure must be quad-word aligned because it is zone allocated.
**************************************************************************/
typedef struct _UDFContextControlBlock {
    UDFIdentifier                       NodeIdentifier;
    // ptr to the associated FCB
    struct _UDFFileControlBlock *Fcb;
    // all CCB structures for a FCB are linked together
    LIST_ENTRY                          NextCCB;
    // each CCB is associated with a file object
    PFILE_OBJECT                        FileObject;
    // flags (see below) associated with this CCB
    uint32                              CCBFlags;
    // current index in directory is required sometimes
    ULONG                               CurrentIndex;
    // if this CCB represents a directory object open, we may
    //  need to maintain a search pattern
    PUNICODE_STRING                     DirectorySearchPattern;
    HASH_ENTRY                          hashes;
    ULONG                               TreeLength;
    // Acces rights previously granted to caller's thread
    ACCESS_MASK                         PreviouslyGrantedAccess;
} UDFCCB, *PtrUDFCCB;


/**************************************************************************
    the following CCBFlags values are relevant. These flag
    values are bit fields; therefore we can test whether
    a bit position is set (1) or not set (0).
**************************************************************************/

// some on-disk file/directories are opened by UDF itself
//  as opposed to being opened on behalf of a user process
#define UDF_CCB_OPENED_BY_UDF                   (0x00000001)
// the file object specified synchronous access at create/open time.
//  this implies that UDF must maintain the current byte offset
#define UDF_CCB_OPENED_FOR_SYNC_ACCESS          (0x00000002)
// file object specified sequential access for this file
#define UDF_CCB_OPENED_FOR_SEQ_ACCESS           (0x00000004)
// the CCB has had an IRP_MJ_CLEANUP issued on it. we must
//  no longer allow the file object / CCB to be used in I/O requests.
#define UDF_CCB_CLEANED                         (0x00000008)
// if we were invoked via the fast i/o path to perform file i/o;
//  we should set the CCB access/modification time at cleanup
#define UDF_CCB_ACCESSED                        (0x00000010)
#define UDF_CCB_MODIFIED                        (0x00000020)
// if an application process set the file date time, we must
//  honor that request and *not* overwrite the values at cleanup
#define UDF_CCB_ACCESS_TIME_SET                 (0x00000040)
#define UDF_CCB_MODIFY_TIME_SET                 (0x00000080)
#define UDF_CCB_CREATE_TIME_SET                 (0x00000100)
#define UDF_CCB_WRITE_TIME_SET                  (0x00000200)
#define UDF_CCB_ATTRIBUTES_SET                  (0x00020000)

#define UDF_CCB_CASE_SENSETIVE                  (0x00000400)

#ifndef UDF_READ_ONLY_BUILD
#define UDF_CCB_DELETE_ON_CLOSE                 (0x00000800)
#endif //UDF_READ_ONLY_BUILD

// this CCB was allocated for a "volume open" operation
#define UDF_CCB_VOLUME_OPEN                     (0x00001000)
#define UDF_CCB_MATCH_ALL                       (0x00002000)
#define UDF_CCB_WILDCARD_PRESENT                (0x00004000)
#define UDF_CCB_CAN_BE_8_DOT_3                  (0x00008000)
#define UDF_CCB_READ_ONLY                       (0x00010000)
//#define UDF_CCB_ATTRIBUTES_SET                (0x00020000) // see above

#define UDF_CCB_FLUSHED                         (0x20000000)
#define UDF_CCB_VALID                           (0x40000000)
#define UDF_CCB_NOT_FROM_ZONE                   (0x80000000)


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

typedef struct _UDFNTRequiredFCB {

    FSRTL_COMMON_FCB_HEADER             CommonFCBHeader;
    SECTION_OBJECT_POINTERS             SectionObject;
    FILE_LOCK                           FileLock;
    ERESOURCE                           MainResource;
    ERESOURCE                           PagingIoResource;
    // we will maintain some time information here to make our life easier
    LARGE_INTEGER                       CreationTime;
    LARGE_INTEGER                       LastAccessTime;
    LARGE_INTEGER                       LastWriteTime;
    LARGE_INTEGER                       ChangeTime;
    // NT requires that a file system maintain and honor the various
    //  SHARE_ACCESS modes ...
    SHARE_ACCESS                        FCBShareAccess;
    // This counter is used to prevent unexpected structure releases
    ULONG                               CommonRefCount;
    PSECURITY_DESCRIPTOR                SecurityDesc;
    ULONG                               NtReqFCBFlags;
    // to identify the lazy writer thread(s) we will grab and store
    //  the thread id here when a request to acquire resource(s)
    //  arrives ..
    uint32                              LazyWriterThreadID;
    UCHAR                               AcqSectionCount;
    UCHAR                               AcqFlushCount;
#ifdef DBG
    PFILE_OBJECT                        FileObject;
#endif //DBG
    PETHREAD                            CloseThread;
} UDFNTRequiredFCB, *PtrUDFNTRequiredFCB;

#define     UDF_NTREQ_FCB_SD_MODIFIED   (0x00000001)
#define     UDF_NTREQ_FCB_INLIST        (0x00000002)
#define     UDF_NTREQ_FCB_DELETED       (0x00000004)
#define     UDF_NTREQ_FCB_MODIFIED      (0x00000008)
#define     UDF_NTREQ_FCB_VALID         (0x40000000)

/**************************************************************************/

#define UDF_FCB_MT NonPagedPool

/***************************************************/
/*****************  W A R N I N G  *****************/
/***************************************************/

/***************************************************/
/*      DO NOT FORGET TO UPDATE VCB's HEADER !     */
/***************************************************/

typedef struct _UDFFileControlBlock {
    UDFIdentifier                       NodeIdentifier;
    // we will not embed the "NT Required FCB" here, 'cause we dislike
    // troubles with Hard(&Symbolic) Links
    PtrUDFNTRequiredFCB                 NTRequiredFCB;
    // UDF related data
    PUDF_FILE_INFO                      FileInfo;
    // this FCB belongs to some mounted logical volume
    struct _UDFVolumeControlBlock*      Vcb;
    // to be able to access all open file(s) for a volume, we will
    //  link all FCB structures for a logical volume together
    LIST_ENTRY                          NextFCB;
    // some state information for the FCB is maintained using the
    //  flags field
    uint32                              FCBFlags;
    // all CCB's for this particular FCB are linked off the following
    //  list head.
    LIST_ENTRY                          NextCCB;
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
    uint32                              ReferenceCount;
    uint32                              OpenHandleCount;
    uint32                              CachedOpenHandleCount;
    // for the UDF fsd, there exists a 1-1 correspondence between a
    //  full object pathname and a FCB
    PtrUDFObjectName                    FCBName;
    ERESOURCE                           CcbListResource;

    struct _UDFFileControlBlock*        ParentFcb;
    // Pointer to IrpContextLite in delayed queue.
    struct _UDFIrpContextLite*          IrpContextLite;
    uint32                              CcbCount;
} UDFFCB, *PtrUDFFCB;

/**************************************************************************
    the following FCBFlags values are relevant. These flag
    values are bit fields; therefore we can test whether
    a bit position is set (1) or not set (0).
**************************************************************************/
#define     UDF_FCB_VALID                               (0x00000002)

#define     UDF_FCB_PAGE_FILE                           (0x00000004)
#define     UDF_FCB_DIRECTORY                           (0x00000008)
#define     UDF_FCB_ROOT_DIRECTORY                      (0x00000010)
#define     UDF_FCB_WRITE_THROUGH                       (0x00000020)
#define     UDF_FCB_MAPPED                              (0x00000040)
#define     UDF_FCB_FAST_IO_READ_IN_PROGESS             (0x00000080)
#define     UDF_FCB_FAST_IO_WRITE_IN_PROGESS            (0x00000100)
#define     UDF_FCB_DELETE_ON_CLOSE                     (0x00000200)
#define     UDF_FCB_MODIFIED                            (0x00000400)
#define     UDF_FCB_ACCESSED                            (0x00000800)
#define     UDF_FCB_READ_ONLY                           (0x00001000)
#define     UDF_FCB_DELAY_CLOSE                         (0x00002000)
#define     UDF_FCB_DELETED                             (0x00004000)

#define     UDF_FCB_INITIALIZED_CCB_LIST_RESOURCE       (0x00008000)
#define     UDF_FCB_POSTED_RENAME                       (0x00010000)

#define     UDF_FCB_DELETE_PARENT                       (0x10000000)
#define     UDF_FCB_NOT_FROM_ZONE                       (0x80000000)

/**************************************************************************
    A logical volume is represented with the following structure.
    This structure is allocated as part of the device extension
    for a device object that this FSD will create, to represent
    the mounted logical volume.

**************************************************************************/

#define _BROWSE_UDF_

// Common UDF-related definitions
#include "Include/udf_common.h"

// One for root
#define         UDF_RESIDUAL_REFERENCE              (2)

// input flush flags
#define         UDF_FLUSH_FLAGS_BREAKABLE           (0x00000001)
// see also udf_rel.h
#define         UDF_FLUSH_FLAGS_LITE                (0x80000000)
// output flush flags
#define         UDF_FLUSH_FLAGS_INTERRUPTED         (0x00000001)

#define         UDF_MAX_BG_WRITERS                  16

typedef struct _FILTER_DEV_EXTENSION {
    UDFIdentifier   NodeIdentifier;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  lowerFSDeviceObject;
} FILTER_DEV_EXTENSION, *PFILTER_DEV_EXTENSION;

typedef struct _UDFFS_DEV_EXTENSION {
    UDFIdentifier   NodeIdentifier;
} UDFFS_DEV_EXTENSION, *PUDFFS_DEV_EXTENSION;
/**************************************************************************
    The IRP context encapsulates the current request. This structure is
    used in the "common" dispatch routines invoked either directly in
    the context of the original requestor, or indirectly in the context
    of a system worker thread.
**************************************************************************/
typedef struct _UDFIrpContext {
    UDFIdentifier                   NodeIdentifier;
    uint32                          IrpContextFlags;
    // copied from the IRP
    uint8                           MajorFunction;
    // copied from the IRP
    uint8                           MinorFunction;
    // to queue this IRP for asynchronous processing
    WORK_QUEUE_ITEM                 WorkQueueItem;
    // the IRP for which this context structure was created
    PIRP                            Irp;
    // the target of the request (obtained from the IRP)
    PDEVICE_OBJECT                  TargetDeviceObject;
    // if an exception occurs, we will store the code here
    NTSTATUS                        SavedExceptionCode;
    // For queued close operation we save Fcb
    _UDFFileControlBlock            *Fcb;
    ULONG                           TreeLength;
    PMDL                            PtrMdl;
    PCHAR                           TransitionBuffer;
    // support for delayed close
} UDFIrpContext, *PtrUDFIrpContext;

#define         UDF_IRP_CONTEXT_CAN_BLOCK           (0x00000001)
#define         UDF_IRP_CONTEXT_WRITE_THROUGH       (0x00000002)
#define         UDF_IRP_CONTEXT_EXCEPTION           (0x00000004)
#define         UDF_IRP_CONTEXT_DEFERRED_WRITE      (0x00000008)
#define         UDF_IRP_CONTEXT_ASYNC_PROCESSING    (0x00000010)
#define         UDF_IRP_CONTEXT_NOT_TOP_LEVEL       (0x00000020)
#define         UDF_IRP_CONTEXT_FLAG_DISABLE_POPUPS (0x00000040)
#define         UDF_IRP_CONTEXT_FLUSH_REQUIRED      (0x00000080)
#define         UDF_IRP_CONTEXT_FLUSH2_REQUIRED     (0x00000100)
#define         UDF_IRP_CONTEXT_READ_ONLY           (0x00010000)
#define         UDF_IRP_CONTEXT_RES1_ACQ            (0x01000000)
#define         UDF_IRP_CONTEXT_RES2_ACQ            (0x02000000)
#define         UDF_IRP_CONTEXT_FORCED_POST         (0x20000000)
#define         UDF_IRP_CONTEXT_BUFFER_LOCKED       (0x40000000)
#define         UDF_IRP_CONTEXT_NOT_FROM_ZONE       (0x80000000)

/**************************************************************************
    Following structure is used to queue a request to the delayed close queue.
    This structure should be the minimum block allocation size.
**************************************************************************/
typedef struct _UDFIrpContextLite {
    UDFIdentifier                   NodeIdentifier;
    //  Fcb for the file object being closed.
    _UDFFileControlBlock            *Fcb;
    //  List entry to attach to delayed close queue.
    LIST_ENTRY                      DelayedCloseLinks;
    //  User reference count for the file object being closed.
    //ULONG                           UserReference;
    //  Real device object.  This represents the physical device closest to the media.
    PDEVICE_OBJECT                  RealDevice;
    ULONG                           TreeLength;
    uint32                          IrpContextFlags;
} UDFIrpContextLite, *PtrUDFIrpContextLite;




// a default size of the number of pages of non-paged pool allocated
//  for each of the zones ...

//  Note that the values are absolutely arbitrary, the only information
//  worth using from the values themselves is that they increase for
//  larger systems (i.e. systems with more memory)
#define     UDF_DEFAULT_ZONE_SIZE_SMALL_SYSTEM          (0x4)
#define     UDF_DEFAULT_ZONE_SIZE_MEDIUM_SYSTEM         (0x8)
#define     UDF_DEFAULT_ZONE_SIZE_LARGE_SYSTEM          (0xc)

// another simplistic (brain dead ? :-) method used is to simply double
//  the values for a "server" machine

//  So, for all you guys who "modified" the registry ;-) to change the
//  wkstation into a server, tough luck !
#define     UDF_NTAS_MULTIPLE                               (0x2)

typedef struct _UDFEjectWaitContext {
    PVCB    Vcb;

    BOOLEAN SoftEjectReq;
    UCHAR   Padding0[3];

    KEVENT  StopReq;
    PKEVENT WaiterStopped;
    WORK_QUEUE_ITEM EjectReqWorkQueueItem;
    
    GET_EVENT_USER_OUT EjectReqBuffer;
    UCHAR   PaddingEvt[(0x40 - sizeof(GET_EVENT_USER_OUT)) & 0x0f];

    GET_CAPABILITIES_USER_OUT DevCap;
    UCHAR   PaddingDevCap[(0x40 - sizeof(GET_CAPABILITIES_USER_OUT)) & 0x0f];

    GET_LAST_ERROR_USER_OUT Error;
    UCHAR   PaddingError[(0x40 - sizeof(GET_LAST_ERROR_USER_OUT)) & 0x0f];

    ULONG   Zero;
} UDFEjectWaitContext, *PUDFEjectWaitContext;

typedef struct _UDFBGWriteContext {
    PVCB  Vcb;
    PVOID Buffer;     // Target buffer
    ULONG Length;
    ULONG Lba;
    ULONG WrittenBytes;
    BOOLEAN FreeBuffer;
    WORK_QUEUE_ITEM WorkQueueItem;
} UDFBGWriteContext, *PUDFBGWriteContext;

//  Define the file system statistics struct.  Vcb->Statistics points to an
//  array of these (one per processor) and they must be 64 byte aligned to
//  prevent cache line tearing.
typedef struct _FILE_SYSTEM_STATISTICS {
    //  This contains the actual data.
    FILESYSTEM_STATISTICS Common;
    FAT_STATISTICS Fat;
    //  Pad this structure to a multiple of 64 bytes.
    UCHAR Pad[64-(sizeof(FILESYSTEM_STATISTICS)+sizeof(FAT_STATISTICS))%64];
} FILE_SYSTEM_STATISTICS, *PFILE_SYSTEM_STATISTICS;

//
typedef struct _UDFFileIDCacheItem {
    LONGLONG Id;
    UNICODE_STRING FullName;
    BOOLEAN CaseSens;
} UDFFileIDCacheItem, *PUDFFileIDCacheItem;

#define DIRTY_PAGE_LIMIT   32

#endif /* _UDF_STRUCTURES_H_ */ // has this file been included?

