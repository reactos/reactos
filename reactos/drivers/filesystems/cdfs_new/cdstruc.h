/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    CdStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Cdfs file system.

    In-Memory structures:

        The global data structures with the CdDataRecord.  It contains a pointer
        to a File System Device object and a queue of Vcb's.  There is a Vcb for
        every currently or previously mounted volumes.  We may be in the process
        of tearing down the Vcb's which have been dismounted.  The Vcb's are
        allocated as an extension to a volume device object.

            +--------+
            | CdData |     +--------+
            |        | --> |FilSysDo|
            |        |     |        |
            |        | <+  +--------+
            +--------+  |
                        |
                        |  +--------+     +--------+
                        |  |VolDo   |     |VolDo   |
                        |  |        |     |        |
                        |  +--------+     +--------+
                        +> |Vcb     | <-> |Vcb     | <-> ...
                           |        |     |        |
                           +--------+     +--------+


        Each Vcb contains a table of all the Fcbs for the volume indexed by
        their FileId.  Each Vcb contains a pointer to the root directory of
        the volume.  Each directory Fcb contains a queue of child Fcb's for
        its children.  There can also be detached subtrees due to open operations
        by Id where the Fcb's are not connected to the root.

        The following diagram shows the root structure.

            +--------+     +--------+
            |  Vcb   |---->| Fcb    |-----------------------------------------------+
            |        |     |  Table |--------------------------------------------+  |                                   |
            |        |--+  |        |-----------------------------------------+  |  |                                   |
            +--------+  |  +--------+                                         |  |  |
                        |    |  |  |                                          |  |  |
                        |    |  |  +--------------------+                     |  |  |
                        |    V  +---------+             |                     |  |  |
                        |  +--------+     |             |                     |  |  |
                        |  |RootFcb |     V             V                     |  |  |
                        +->|        |   +--------+    +--------+              |  |  |
                           |        |-->|Child   |    |Child   |              |  |  |
                           +--------+   | Fcb    |<-->| Fcb    |<--> ...      |  |  |
                                        |        |    |        |              |  |  |
                                        +--------+    +--------+              |  |  |
                                                                              |  |  |
                          (Freestanding sub-tree)                             |  |  |
                          +--------+                                          |  |  |
                          |OpenById|<-----------------------------------------+  |  |
                          | Dir    |    +--------+                               |  |
                          |        |--->|OpenById|<------------------------------+  |
                          +--------+    | Child  |    +--------+                    |
                                        |  Dir   |--->|OpenById|<-------------------+
                                        +--------+    | Child  |
                                                      |  File  |
                                                      +--------+

        Attached to each Directory Fcb is a prefix table containing the names
        of children of this directory for which there is an Fcb.  Not all Fcb's
        will necessarily have an entry in this table.

            +--------+      +--------+
            |  Dir   |      | Prefix |
            |   Fcb  |----->|  Table |--------------------+
            |        |      |        |-------+            |
            +--------+      +--------+       |            |
                |              |             |            |
                |              |             |            |
                |              V             V            V
                |           +--------+    +--------+    +--------+    +--------+
                |           |  Fcb   |    |  Fcb   |    |  Fcb   |    |  Fcb   |
                +---------->|        |<-->|        |<-->|        |<-->|        |
                            |        |    |        |    |        |    |        |
                            +--------+    +--------+    +--------+    +--------+


        Each file object open on a CDROM volume contains two context pointers.  The
        first will point back to the Fcb for the file object.  The second, if present,
        points to a Ccb (ContextControlBlock) which contains the per-handle information.
        This includes the state of any directory enumeration.

          +--------+       +--------+    +--------+
          |  Fcb   |<------| File   |    |  Ccb   |
          |        |       |  Object|--->|        |
          |        |       |        |    |        |
          +--------+       +--------+    +--------+
            ^    ^
            |    |         +--------+    +--------+
            |    |         | File   |    |  Ccb   |
            |    +---------|  Object|--->|        |
            |              |        |    |        |
            |              +--------+    +--------+
            |
            |              +--------+
            |              |Stream  |
            +--------------| File   |
                           |  Object|
                           +--------+


    Synchronization:

        1. A resource in the CdData synchronizes access to the Vcb queue.  This
            is used during mount/verify/dismount operations.

        2. A resource in the Vcb is used to synchronize access to Vcb for
            open/close operations.  Typically acquired shared, it
            is acquired exclusively to lock out these operations.

        3. A second resource in the Vcb is used to synchronize all file operations.
            Typically acquired shared, it is acquired exclusively to lock
            out all file operations.  Acquiring both Vcb resources will lock
            the entire volume.

        4. A resource in the nonpaged Fcb will synchronize open/close operations
            on an Fcb.

        5. A fast mutex in the Vcb will protect access to the Fcb table and
            the open counts in the Vcb.  It is also used to modify the reference
            counts in all Fcbs.  This mutex cannot be acquired
            exclusely and is an end resource.

        6. A fast mutex in the Fcb will synchronize access to all Fcb fields
            which aren't synchronized in some other way.  A thread may acquire
            mutexes for multiple Fcb's as long as it works it way toward the
            root of the tree.  This mutex can also be acquired recursively.

        7. Normal locking order is CdData/Vcb/Fcb starting at any point in this
            chain.  The Vcb is required prior to acquiring resources for multiple
            files.  Shared ownership of the Vcb is sufficient in this case.

        8. Normal locking order when acquiring multiple Fcb's is from some
            starting Fcb and walking towards the root of tree.  Create typically
            walks down the tree.  In this case we will attempt to acquire the
            next node optimistically and if that fails we will reference
            the current node in the tree, release it and acquire the next node.
            At that point it will be safe to reacquire the parent node.

        9. Locking order for the Fcb (via the fast mutex) will be from leaf of
            tree back towards the root.  No other resource may be acquired
            after locking the Vcb (other than in-page reads).

       10. Cleanup operations only lock the Vcb and Fcb long enough to change the
            critical counts and share access fields.  No reason to synchronize
            otherwise.  None of the structures can go away from beneath us
            in this case.


--*/

#ifndef _CDSTRUC_
#define _CDSTRUC_

typedef PVOID PBCB;     //**** Bcb's are now part of the cache module

#define BYTE_COUNT_EMBEDDED_NAME        (32)


//
//  The CD_MCB is used to store the mapping of logical file offset to
//  logical disk offset.  NOTE - This package only deals with the
//  logical 2048 sectors.  Translating to 'raw' sectors happens in
//  software.  We will embed a single MCB_ENTRY in the Fcb since this
//  will be the typical case.
//

typedef struct _CD_MCB {

    //
    //  Size and current count of the Mcb entries.
    //

    ULONG MaximumEntryCount;
    ULONG CurrentEntryCount;

    //
    //  Pointer to the start of the Mcb entries.
    //

    struct _CD_MCB_ENTRY *McbArray;

} CD_MCB;
typedef CD_MCB *PCD_MCB;

typedef struct _CD_MCB_ENTRY {

    //
    //  Starting offset and number of bytes described by this entry.
    //  The Byte count is rounded to a logical block boundary if this is
    //  the last block.
    //

    LONGLONG DiskOffset;
    LONGLONG ByteCount;

    //
    //  Starting offset in the file of mapping described by this dirent.
    //

    LONGLONG FileOffset;

    //
    //  Data length and block length.  Data length is the length of each
    //  data block.  Total length is the length of each data block and
    //  the skip size.
    //

    LONGLONG DataBlockByteCount;
    LONGLONG TotalBlockByteCount;

} CD_MCB_ENTRY;
typedef CD_MCB_ENTRY *PCD_MCB_ENTRY;


//
//  Cd name structure.  The following structure is used to represent the
//  full Cdrom name.  This name can be stored in either Unicode or ANSI
//  format.
//

typedef struct _CD_NAME {

    //
    //  String containing name without the version number.
    //  The maximum length field for filename indicates the
    //  size of the buffer allocated for the two parts of the name.
    //

    UNICODE_STRING FileName;

    //
    //  String containging the version number.
    //

    UNICODE_STRING VersionString;

} CD_NAME;
typedef CD_NAME *PCD_NAME;

//
//  Following is the splay link structure for the prefix lookup.
//  The names can be in either Unicode string or Ansi string format.
//

typedef struct _NAME_LINK {

    RTL_SPLAY_LINKS Links;
    UNICODE_STRING FileName;

} NAME_LINK;
typedef NAME_LINK *PNAME_LINK;


//
//  Prefix entry.  There is one of these for each name in the prefix table.
//  An Fcb will have one of these embedded for the long name and an optional
//  pointer to the short name entry.
//

typedef struct _PREFIX_ENTRY {

    //
    //  Pointer to the Fcb for this entry.
    //

    struct _FCB *Fcb;

    //
    //  Flags field.  Used to indicate if the name is in the prefix table.
    //

    ULONG PrefixFlags;

    //
    //  Exact case name match.
    //

    NAME_LINK ExactCaseName;

    //
    //  Case-insensitive name link.
    //

    NAME_LINK IgnoreCaseName;

    WCHAR FileNameBuffer[ BYTE_COUNT_EMBEDDED_NAME ];

} PREFIX_ENTRY;
typedef PREFIX_ENTRY *PPREFIX_ENTRY;

#define PREFIX_FLAG_EXACT_CASE_IN_TREE              (0x00000001)
#define PREFIX_FLAG_IGNORE_CASE_IN_TREE             (0x00000002)


//
//  The CD_DATA record is the top record in the CDROM file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _CD_DATA {

    //
    //  The type and size of this record (must be CDFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  Vcb queue.
    //

    LIST_ENTRY VcbQueue;

    //
    //  The following fields are used to allocate IRP context structures
    //  using a lookaside list, and other fixed sized structures from a
    //  small cache.  We use the CdData mutex to protext these structures.
    //

    ULONG IrpContextDepth;
    ULONG IrpContextMaxDepth;
    SINGLE_LIST_ENTRY IrpContextList;

    //
    //  Filesystem device object for CDFS.
    //

    PDEVICE_OBJECT FileSystemDeviceObject;

    //
    //  Following are used to manage the async and delayed close queue.
    //
    //  FspCloseActive - Indicates whether there is a thread processing the
    //      two close queues.
    //  ReduceDelayedClose - Indicates that we have hit the upper threshold
    //      for the delayed close queue and need to reduce it to lower threshold.
    //
    //  AsyncCloseQueue - Queue of IrpContext waiting for async close operation.
    //  AsyncCloseCount - Number of entries on the async close queue.
    //
    //  DelayedCloseQueue - Queue of IrpContextLite waiting for delayed close
    //      operation.
    //  MaxDelayedCloseCount - Trigger delay close work at this threshold.
    //  MinDelayedCloseCount - Turn off delay close work at this threshold.
    //  DelayedCloseCount - Number of entries on the delayted close queue.
    //
    //  CloseItem - Workqueue item used to start FspClose thread.
    //

    LIST_ENTRY AsyncCloseQueue;
    ULONG AsyncCloseCount;
    BOOLEAN FspCloseActive;
    BOOLEAN ReduceDelayedClose;
    USHORT PadUshort;

    //
    //  The following fields describe the deferred close file objects.
    //

    LIST_ENTRY DelayedCloseQueue;
    ULONG DelayedCloseCount;
    ULONG MaxDelayedCloseCount;
    ULONG MinDelayedCloseCount;

    //
    //  Fast mutex used to lock the fields of this structure.
    //

    PVOID CdDataLockThread;
    FAST_MUTEX CdDataMutex;

    //
    //  A resource variable to control access to the global CDFS data record
    //

    ERESOURCE DataResource;

    //
    //  Cache manager call back structure, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerVolumeCallbacks;

    //
    //  This is the ExWorkerItem that does both kinds of deferred closes.
    //

    PIO_WORKITEM CloseItem;

} CD_DATA;
typedef CD_DATA *PCD_DATA;


//
//  The Vcb (Volume control block) record corresponds to every
//  volume mounted by the file system.  They are ordered in a queue off
//  of CdData.VcbQueue.
//
//  The Vcb will be in several conditions during its lifespan.
//
//      NotMounted - Disk is not currently mounted (i.e. removed
//          from system) but cleanup and close operations are
//          supported.
//
//      MountInProgress - State of the Vcb from the time it is
//          created until it is successfully mounted or the mount
//          fails.
//
//      Mounted - Volume is currently in the mounted state.
//
//      Invalid - User has invalidated the volume.  Only legal operations
//          are cleanup and close.
//
//      DismountInProgress - We have begun the process of tearing down the
//          Vcb.  It can be deleted when all the references to it
//          have gone away.
//

typedef enum _VCB_CONDITION {

    VcbNotMounted = 0,
    VcbMountInProgress,
    VcbMounted,
    VcbInvalid,
    VcbDismountInProgress

} VCB_CONDITION;

typedef struct _VCB {

    //
    //  The type and size of this record (must be CDFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Vpb for this volume.
    //

    PVPB Vpb;

    //
    //  Device object for the driver below us.
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  File object used to lock the volume.
    //

    PFILE_OBJECT VolumeLockFileObject;

    //
    //  Link into queue of Vcb's in the CdData structure.  We will create a union with
    //  a LONGLONG to force the Vcb to be quad-aligned.
    //

    union {

        LIST_ENTRY VcbLinks;
        LONGLONG Alignment;
    };

    //
    //  State flags and condition for the Vcb.
    //

    ULONG VcbState;
    VCB_CONDITION VcbCondition;

    //
    //  Various counts for this Vcb.
    //
    //      VcbCleanup - Open handles left on this system.
    //      VcbReference - Number of reasons this Vcb is still present.
    //      VcbUserReference - Number of user file objects still present.
    //

    ULONG VcbCleanup;
    ULONG VcbReference;
    ULONG VcbUserReference;

    //
    //  Fcb for the Volume Dasd file, root directory and the Path Table.
    //

    struct _FCB *VolumeDasdFcb;
    struct _FCB *RootIndexFcb;
    struct _FCB *PathTableFcb;

    //
    //  Location of current session and offset of volume descriptors.
    //

    ULONG BaseSector;
    ULONG VdSectorOffset;
    ULONG PrimaryVdSectorOffset;

    //
    //  Following is a sector from the last non-cached read of an XA file.
    //  Also the cooked offset on the disk.
    //

    PVOID XASector;
    LONGLONG XADiskOffset;

    //
    //  Vcb resource.  This is used to synchronize open/cleanup/close operations.
    //

    ERESOURCE VcbResource;

    //
    //  File resource.  This is used to synchronize all file operations except
    //  open/cleanup/close.
    //

    ERESOURCE FileResource;

    //
    //  Vcb fast mutex.  This is used to synchronize the fields in the Vcb
    //  when modified when the Vcb is not held exclusively.  Included here
    //  are the count fields and Fcb table.
    //
    //  We also use this to synchronize changes to the Fcb reference field.
    //

    FAST_MUTEX VcbMutex;
    PVOID VcbLockThread;

    //
    //  The following is used to synchronize the dir notify package.
    //

    PNOTIFY_SYNC NotifySync;

    //
    //  The following is the head of a list of notify Irps.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  Logical block size for this volume as well constant values
    //  associated with the block size.
    //

    ULONG BlockSize;
    ULONG BlockToSectorShift;
    ULONG BlockToByteShift;
    ULONG BlocksPerSector;
    ULONG BlockMask;
    ULONG BlockInverseMask;

    //
    //  Fcb table.  Synchronized with the Vcb fast mutex.
    //

    RTL_GENERIC_TABLE FcbTable;

    //
    //  Volume TOC.  Cache this information for quick lookup.
    //

    PCDROM_TOC CdromToc;
    ULONG TocLength;
    ULONG TrackCount;
    ULONG DiskFlags;

    //
    //  Block factor to determine last session information.
    //

    ULONG BlockFactor;

    //
    //  Media change count from device driver for bulletproof detection
    //  of media movement
    //

    ULONG MediaChangeCount;

    //
    //  For raw reads, CDFS must obey the port maximum transfer restrictions.
    //

    ULONG MaximumTransferRawSectors;
    ULONG MaximumPhysicalPages;

    //
    //  Preallocated VPB for swapout, so we are not forced to consider
    //  must succeed pool.
    //

    PVPB SwapVpb;

} VCB;
typedef VCB *PVCB;

#define VCB_STATE_HSG                               (0x00000001)
#define VCB_STATE_ISO                               (0x00000002)
#define VCB_STATE_JOLIET                            (0x00000004)
#define VCB_STATE_LOCKED                            (0x00000010)
#define VCB_STATE_REMOVABLE_MEDIA                   (0x00000020)
#define VCB_STATE_CDXA                              (0x00000040)
#define VCB_STATE_AUDIO_DISK                        (0x00000080)
#define VCB_STATE_NOTIFY_REMOUNT                    (0x00000100)
#define VCB_STATE_VPB_NOT_ON_DEVICE                 (0x00000200)


//
//  The Volume Device Object is an I/O system device object with a
//  workqueue and an VCB record appended to the end.  There are multiple
//  of these records, one for every mounted volume, and are created during
//  a volume mount operation.  The work queue is for handling an overload
//  of work requests to the volume.
//

typedef struct _VOLUME_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    ULONG PostedRequestCount;

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    ULONG OverflowQueueCount;

    //
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue;

    //
    //  The following spinlock protects access to all the above fields.
    //

    KSPIN_LOCK OverflowQueueSpinLock;

    //
    //  This is the file system specific volume control block.
    //

    VCB Vcb;

} VOLUME_DEVICE_OBJECT;
typedef VOLUME_DEVICE_OBJECT *PVOLUME_DEVICE_OBJECT;


//
//  The following two structures are the separate union structures for
//  data and index Fcb's.  The path table is actually the same structure
//  as the index Fcb since it uses the first few fields.
//

typedef enum _FCB_CONDITION {
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

typedef struct _FCB_DATA {

    //
    //  The following field is used by the oplock module
    //  to maintain current oplock information.
    //

    OPLOCK Oplock;

    //
    //  The following field is used by the filelock module
    //  to maintain current byte range locking information.
    //  A file lock is allocated as needed.
    //

    PFILE_LOCK FileLock;

} FCB_DATA;
typedef FCB_DATA *PFCB_DATA;

typedef struct _FCB_INDEX {

    //
    //  Internal stream file.
    //

    PFILE_OBJECT FileObject;

    //
    //  Offset of first entry in stream.  This is for case where directory
    //  or path table does not begin on a sector boundary.  This value is
    //  added to all offset values to determine the real offset.
    //

    ULONG StreamOffset;

    //
    //  List of child fcbs.
    //

    LIST_ENTRY FcbQueue;

    //
    //  Ordinal number for this directory.  Combine this with the path table offset
    //  in the FileId and you have a starting point in the path table.
    //

    ULONG Ordinal;

    //
    //  Children path table start.  This is the offset in the path table
    //  for the first child of the directory.  A value of zero indicates
    //  that we haven't found the first child yet.  If there are no child
    //  directories we will position at a point in the path table so that
    //  subsequent searches will fail quickly.
    //

    ULONG ChildPathTableOffset;
    ULONG ChildOrdinal;

    //
    //  Root of splay trees for exact and ignore case prefix trees.
    //

    PRTL_SPLAY_LINKS ExactCaseRoot;
    PRTL_SPLAY_LINKS IgnoreCaseRoot;

} FCB_INDEX;
typedef FCB_INDEX *PFCB_INDEX;

typedef struct _FCB_NONPAGED {

    //
    //  Type and size of this record must be CDFS_NTC_FCB_NONPAGED
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to
    //  point to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  This is the resource structure for this Fcb.
    //

    ERESOURCE FcbResource;

    //
    //  This is the FastMutex for this Fcb.
    //

    FAST_MUTEX FcbMutex;

    //
    //  This is the mutex that is inserted into the FCB_ADVANCED_HEADER
    //  FastMutex field
    //

    FAST_MUTEX AdvancedFcbHeaderMutex;

} FCB_NONPAGED;
typedef FCB_NONPAGED *PFCB_NONPAGED;

//
//  The Fcb/Dcb record corresponds to every open file and directory, and to
//  every directory on an opened path.
//

typedef struct _FCB {

    //
    //  The following field is used for fast I/O.  It contains the node
    //  type code and size, indicates if fast I/O is possible, contains
    //  allocation, file, and valid data size, a resource, and call back
    //  pointers for FastIoRead and FastMdlRead.
    //
    //
    //  Node type codes for the Fcb must be one of the following.
    //
    //      CDFS_NTC_FCB_PATH_TABLE
    //      CDFS_NTC_FCB_INDEX
    //      CDFS_NTC_FCB_DATA
    //

    //
    //  Common Fsrtl Header.  The named header is for the fieldoff.c output.  We
    //  use the unnamed header internally.
    //

    union{

        FSRTL_ADVANCED_FCB_HEADER Header;
        FSRTL_ADVANCED_FCB_HEADER;
    };

    //
    //  Vcb for this Fcb.
    //

    PVCB Vcb;

    //
    //  Parent Fcb for this Fcb.  This may be NULL if this file was opened
    //  by ID, also for the root Fcb.
    //

    struct _FCB *ParentFcb;

    //
    //  Links to the queue of Fcb's in the parent.
    //

    LIST_ENTRY FcbLinks;

    //
    //  FileId for this file.
    //

    FILE_ID FileId;

    //
    //  Counts on this Fcb.  Cleanup count represents the number of open handles
    //  on this Fcb.  Reference count represents the number of reasons this Fcb
    //  is still present.  It includes file objects, children Fcb and anyone
    //  who wants to prevent this Fcb from going away.  Cleanup count is synchronized
    //  with the FcbResource.  The reference count is synchronized with the
    //  VcbMutex.
    //

    ULONG FcbCleanup;
    ULONG FcbReference;
    ULONG FcbUserReference;

    //
    //  State flags for this Fcb.
    //

    ULONG FcbState;

    //
    //  NT style attributes for the Fcb.
    //

    ULONG FileAttributes;

    //
    //  CDXA attributes for this file.
    //

    USHORT XAAttributes;

    //
    //  File number from the system use area.
    //

    UCHAR XAFileNumber;

    //
    //  This is the thread and count for the thread which has locked this
    //  Fcb.
    //

    PVOID FcbLockThread;
    ULONG FcbLockCount;

    //
    //  Pointer to the Fcb non-paged structures.
    //

    PFCB_NONPAGED FcbNonpaged;

    //
    //  Share access structure.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  Mcb for the on disk mapping and a single map entry.
    //

    CD_MCB_ENTRY McbEntry;
    CD_MCB Mcb;

    //
    //  Embed the prefix entry for the longname.  Store an optional pointer
    //  to a prefix structure for the short name.
    //

    PPREFIX_ENTRY ShortNamePrefix;
    PREFIX_ENTRY FileNamePrefix;

    //
    //  Time stamp for this file.
    //

    LONGLONG CreationTime;

    union{

        ULONG FcbType;
        FCB_DATA;
        FCB_INDEX;
    };

} FCB;
typedef FCB *PFCB;

#define FCB_STATE_INITIALIZED                   (0x00000001)
#define FCB_STATE_IN_FCB_TABLE                  (0x00000002)
#define FCB_STATE_MODE2FORM2_FILE               (0x00000004)
#define FCB_STATE_MODE2_FILE                    (0x00000008)
#define FCB_STATE_DA_FILE                       (0x00000010)

//
//  These file types are read as raw 2352 byte sectors
//

#define FCB_STATE_RAWSECTOR_MASK                ( FCB_STATE_MODE2FORM2_FILE | \
                                                  FCB_STATE_MODE2_FILE      | \
                                                  FCB_STATE_DA_FILE )

#define SIZEOF_FCB_DATA     \
    (FIELD_OFFSET( FCB, FcbType ) + sizeof( FCB_DATA ))

#define SIZEOF_FCB_INDEX    \
    (FIELD_OFFSET( FCB, FcbType ) + sizeof( FCB_INDEX ))


//
//  The Ccb record is allocated for every file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be CDFS_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Flags.  Indicates flags to apply for the current open.
    //

    ULONG Flags;

    //
    //  Fcb for the file being opened.
    //

    PFCB Fcb;

    //
    //  We store state information in the Ccb for a directory
    //  enumeration on this handle.
    //

    //
    //  Offset in the directory stream to base the next enumeration.
    //

    ULONG CurrentDirentOffset;
    CD_NAME SearchExpression;

} CCB;
typedef CCB *PCCB;

#define CCB_FLAG_OPEN_BY_ID                     (0x00000001)
#define CCB_FLAG_OPEN_RELATIVE_BY_ID            (0x00000002)
#define CCB_FLAG_IGNORE_CASE                    (0x00000004)
#define CCB_FLAG_OPEN_WITH_VERSION              (0x00000008)
#define CCB_FLAG_DISMOUNT_ON_CLOSE              (0x00000010)

//
//  Following flags refer to index enumeration.
//

#define CCB_FLAG_ENUM_NAME_EXP_HAS_WILD         (0x00010000)
#define CCB_FLAG_ENUM_VERSION_EXP_HAS_WILD      (0x00020000)
#define CCB_FLAG_ENUM_MATCH_ALL                 (0x00040000)
#define CCB_FLAG_ENUM_VERSION_MATCH_ALL         (0x00080000)
#define CCB_FLAG_ENUM_RETURN_NEXT               (0x00100000)
#define CCB_FLAG_ENUM_INITIALIZED               (0x00200000)
#define CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY    (0x00400000)


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the CdComplete
//  request routine
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be CDFS_NTC_IRP_CONTEXT)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Originating Irp for the request.
    //

    PIRP Irp;

    //
    //  Vcb for this operation.  When this is NULL it means we were called
    //  with our filesystem device object instead of a volume device object.
    //  (Mount will fill this in once the Vcb is created)
    //

    PVCB Vcb;

    //
    //  Exception encountered during the request.  Any error raised explicitly by
    //  the file system will be stored here.  Any other error raised by the system
    //  is stored here after normalizing it.
    //

    NTSTATUS ExceptionStatus;
    ULONG RaisedAtLineFile;

    //
    //  Flags for this request.
    //

    ULONG Flags;

    //
    //  Real device object.  This represents the physical device closest to the media.
    //

    PDEVICE_OBJECT RealDevice;

    //
    //  Io context for a read request.
    //  Address of Fcb for teardown oplock in create case.
    //

    union {

        struct _CD_IO_CONTEXT *IoContext;
        PFCB *TeardownFcb;
    };

    //
    //  Top level irp context for this thread.
    //

    struct _IRP_CONTEXT *TopLevel;

    //
    //  Major and minor function codes.
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  Pointer to the top-level context if this IrpContext is responsible
    //  for cleaning it up.
    //

    struct _THREAD_CONTEXT *ThreadContext;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

#define IRP_CONTEXT_FLAG_ON_STACK               (0x00000001)
#define IRP_CONTEXT_FLAG_MORE_PROCESSING        (0x00000002)
#define IRP_CONTEXT_FLAG_WAIT                   (0x00000004)
#define IRP_CONTEXT_FLAG_FORCE_POST             (0x00000008)
#define IRP_CONTEXT_FLAG_TOP_LEVEL              (0x00000010)
#define IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS         (0x00000020)
#define IRP_CONTEXT_FLAG_IN_FSP                 (0x00000040)
#define IRP_CONTEXT_FLAG_IN_TEARDOWN            (0x00000080)
#define IRP_CONTEXT_FLAG_ALLOC_IO               (0x00000100)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS         (0x00000200)
#define IRP_CONTEXT_FLAG_FORCE_VERIFY           (0x00000400)

//
//  Flags used for create.
//

#define IRP_CONTEXT_FLAG_FULL_NAME              (0x10000000)
#define IRP_CONTEXT_FLAG_TRAIL_BACKSLASH        (0x20000000)

//
//  The following flags need to be cleared when a request is posted.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_POST (   \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_FORCE_POST         |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP             |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//
//  The following flags need to be cleared when a request is retried.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY (  \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//
//  The following flags are set each time through the Fsp loop.
//

#define IRP_CONTEXT_FSP_FLAGS (             \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP                 \
)


//
//  Following structure is used to queue a request to the delayed close queue.
//  This structure should be the minimum block allocation size.
//

typedef struct _IRP_CONTEXT_LITE {

    //
    //  Type and size of this record (must be CDFS_NTC_IRP_CONTEXT_LITE)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Fcb for the file object being closed.
    //

    PFCB Fcb;

    //
    //  List entry to attach to delayed close queue.
    //

    LIST_ENTRY DelayedCloseLinks;

    //
    //  User reference count for the file object being closed.
    //

    ULONG UserReference;

    //
    //  Real device object.  This represents the physical device closest to the media.
    //

    PDEVICE_OBJECT RealDevice;

} IRP_CONTEXT_LITE;
typedef IRP_CONTEXT_LITE *PIRP_CONTEXT_LITE;


//
//  Context structure for asynchronous I/O calls.  Most of these fields
//  are actually only required for the ReadMultiple routines, but
//  the caller must allocate one as a local variable anyway before knowing
//  whether there are multiple requests are not.  Therefore, a single
//  structure is used for simplicity.
//

typedef struct _CD_IO_CONTEXT {

    //
    //  These two fields are used for multiple run Io
    //

    LONG IrpCount;
    PIRP MasterIrp;
    NTSTATUS Status;
    BOOLEAN AllocatedContext;

    union {

        //
        //  This element handles the asynchronous non-cached Io
        //

        struct {

            PERESOURCE Resource;
            ERESOURCE_THREAD ResourceThreadId;
            ULONG RequestedByteCount;
        };

        //
        //  and this element handles the synchronous non-cached Io.
        //

        KEVENT SyncEvent;
    };

} CD_IO_CONTEXT;
typedef CD_IO_CONTEXT *PCD_IO_CONTEXT;


//
//  Following structure is used to track the top level request.  Each Cdfs
//  Fsd and Fsp entry point will examine the top level irp location in the
//  thread local storage to determine if this request is top level and/or
//  top level Cdfs.  The top level Cdfs request will remember the previous
//  value and update that location with a stack location.  This location
//  can be accessed by recursive Cdfs entry points.
//

typedef struct _THREAD_CONTEXT {

    //
    //  CDFS signature.  Used to confirm structure on stack is valid.
    //

    ULONG Cdfs;

    //
    //  Previous value in top-level thread location.  We restore this
    //  when done.
    //

    PIRP SavedTopLevelIrp;

    //
    //  Top level Cdfs IrpContext.  Initial Cdfs entry point on stack
    //  will store the IrpContext for the request in this stack location.
    //

    PIRP_CONTEXT TopLevelIrpContext;

} THREAD_CONTEXT;
typedef THREAD_CONTEXT *PTHREAD_CONTEXT;


//
//  The following structure is used for enumerating the entries in the
//  path table.  We will always map this two sectors at a time so we don't
//  have to worry about entries which span sectors.  We move through
//  one sector at a time though.  We will unpin and remap after
//  crossing a sector boundary.
//
//  The only special case is where we span a cache view.  In that case
//  we will allocate a buffer and read both pieces into it.
//
//  This strategy takes advantage of the CC enhancement which allows
//  overlapping ranges.
//

typedef struct _PATH_ENUM_CONTEXT {

    //
    //  Pointer to the current sector and the offset of this sector to
    //  the beginning of the path table.  The Data pointer may be
    //  a pool block in the case where we cross a cache view
    //  boundary.  Also the length of the data for this block.
    //

    PVOID Data;
    ULONG BaseOffset;
    ULONG DataLength;

    //
    //  Bcb for the sector.  (We may actually have pinned two sectors)
    //  This will be NULL for the case where we needed to allocate a
    //  buffer in the case where we span a cache view.
    //

    PBCB Bcb;

    //
    //  Offset to current entry within the current data block.
    //

    ULONG DataOffset;

    //
    //  Did we allocate the buffer for the entry.
    //

    BOOLEAN AllocatedData;

    //
    //  End of Path Table.  This tells us whether the current data
    //  block includes the end of the path table.  This is the
    //  only block where we need to do a careful check about whether
    //  the path table entry fits into the buffer.
    //
    //  Also once we have reached the end of the path table we don't
    //  need to remap the data as we move into the final sector.
    //  We always look at the last two sectors together.
    //

    BOOLEAN LastDataBlock;

} PATH_ENUM_CONTEXT;
typedef PATH_ENUM_CONTEXT *PPATH_ENUM_CONTEXT;

#define VACB_MAPPING_MASK               (VACB_MAPPING_GRANULARITY - 1)
#define LAST_VACB_SECTOR_OFFSET         (VACB_MAPPING_GRANULARITY - SECTOR_SIZE)


//
//  Path Entry.  This is our representation of the on disk data.
//

typedef struct _PATH_ENTRY {

    //
    //  Directory number and offset.  This is the ordinal and the offset from
    //  the beginning of the path table stream for this entry.
    //
    //

    ULONG Ordinal;
    ULONG PathTableOffset;

    //
    //  Logical block Offset on the disk for this entry.  We already bias
    //  this by any Xar blocks.
    //

    ULONG DiskOffset;

    //
    //  Length of on-disk path table entry.
    //

    ULONG PathEntryLength;

    //
    //  Parent number.
    //

    ULONG ParentOrdinal;

    //
    //  DirName length and Id.  Typically the pointer here points to the raw on-disk
    //  bytes.  We will point to a fixed self entry if this is the root directory.
    //

    ULONG DirNameLen;
    PCHAR DirName;

    //
    //  Following are the flags used to cleanup this structure.
    //

    ULONG Flags;

    //
    //  The following is the filename string and version number strings.  We embed a buffer
    //  large enough to hold two 8.3 names.  One for exact case and one for case insensitive.
    //

    CD_NAME CdDirName;
    CD_NAME CdCaseDirName;

    WCHAR NameBuffer[BYTE_COUNT_EMBEDDED_NAME / sizeof( WCHAR ) * 2];

} PATH_ENTRY;
typedef PATH_ENTRY *PPATH_ENTRY;

#define PATH_ENTRY_FLAG_ALLOC_BUFFER            (0x00000001)


//
//  Compound path entry.  This structure combines the on-disk entries
//  with the in-memory structures.
//

typedef struct _COMPOUND_PATH_ENTRY {

    PATH_ENUM_CONTEXT PathContext;
    PATH_ENTRY PathEntry;

} COMPOUND_PATH_ENTRY;
typedef COMPOUND_PATH_ENTRY *PCOMPOUND_PATH_ENTRY;


//
//  The following is used for enumerating through a directory via the
//  dirents.
//

typedef struct _DIRENT_ENUM_CONTEXT {

    //
    //  Pointer the current sector and the offset of this sector within
    //  the directory file.  Also the data length of this pinned block.
    //

    PVOID Sector;
    ULONG BaseOffset;
    ULONG DataLength;

    //
    //  Bcb for the sector.
    //

    PBCB Bcb;

    //
    //  Offset to the current dirent within this sector.
    //

    ULONG SectorOffset;

    //
    //  Length to next dirent.  A zero indicates to move to the next sector.
    //

    ULONG NextDirentOffset;

} DIRENT_ENUM_CONTEXT;
typedef DIRENT_ENUM_CONTEXT *PDIRENT_ENUM_CONTEXT;


//
//  Following structure is used to smooth out the differences in the HSG, ISO
//  and Joliett directory entries.
//

typedef struct _DIRENT {

    //
    //  Offset in the Directory of this entry.  Note this includes
    //  any bytes added to the beginning of the directory to pad
    //  down to a sector boundary.
    //

    ULONG DirentOffset;

    ULONG DirentLength;

    //
    //  Starting offset on the disk including any Xar blocks.
    //

    ULONG StartingOffset;

    //
    //  DataLength of the data.  If not the last block then this should
    //  be an integral number of logical blocks.
    //

    ULONG DataLength;

    //
    //  The following field is the time stamp out of the directory entry.
    //  Use a pointer into the dirent for this.
    //

    PCHAR CdTime;

    //
    //  The following field is the dirent file flags field.
    //

    UCHAR DirentFlags;

    //
    //  Following field is a Cdfs flag field used to clean up this structure.
    //

    UCHAR Flags;

    //
    //  The following fields indicate the file unit size and interleave gap
    //  for interleaved files.  Each of these are in logical blocks.
    //

    ULONG FileUnitSize;
    ULONG InterleaveGapSize;

    //
    //  System use offset.  Zero value indicates no system use area.
    //

    ULONG SystemUseOffset;

    //
    //  CDXA attributes and file number for this file.
    //

    USHORT XAAttributes;
    UCHAR XAFileNumber;

    //
    //  Filename length and ID.  We copy the length (in bytes) and keep
    //  a pointer to the start of the name.
    //

    ULONG FileNameLen;
    PCHAR FileName;

    //
    //  The following are the filenames stored by name and version numbers.
    //  The fixed buffer here can hold two Unicode 8.3 names.  This allows
    //  us to upcase the name into a fixed buffer.
    //

    CD_NAME CdFileName;
    CD_NAME CdCaseFileName;

    //
    //  Data stream type.  Indicates if this is audio, XA mode2 form2 or cooked sectors.
    //

    XA_EXTENT_TYPE ExtentType;

    WCHAR NameBuffer[BYTE_COUNT_EMBEDDED_NAME / sizeof( WCHAR ) * 2];

} DIRENT;
typedef DIRENT *PDIRENT;

#define DIRENT_FLAG_ALLOC_BUFFER                (0x01)
#define DIRENT_FLAG_CONSTANT_ENTRY              (0x02)

#define DIRENT_FLAG_NOT_PERSISTENT              (0)


//
//  Following structure combines the on-disk information with the normalized
//  structure.
//

typedef struct _COMPOUND_DIRENT {

    DIRENT_ENUM_CONTEXT DirContext;
    DIRENT Dirent;

} COMPOUND_DIRENT;
typedef COMPOUND_DIRENT *PCOMPOUND_DIRENT;


//
//  The following structure is used to enumerate the files in a directory.
//  It contains three DirContext/Dirent pairs and then self pointers to
//  know which of these is begin used how.
//

typedef struct _FILE_ENUM_CONTEXT {

    //
    //  Pointers to the current compound dirents below.
    //
    //      PriorDirent - Initial dirent for the last file encountered.
    //      InitialDirent - Initial dirent for the current file.
    //      CurrentDirent - Second or later dirent for the current file.
    //

    PCOMPOUND_DIRENT PriorDirent;
    PCOMPOUND_DIRENT InitialDirent;
    PCOMPOUND_DIRENT CurrentDirent;

    //
    //  Flags indicating the state of the search.
    //

    ULONG Flags;

    //
    //  This is an accumulation of the file sizes of the different extents
    //  of a single file.
    //

    LONGLONG FileSize;

    //
    //  Short name for this file.
    //

    CD_NAME ShortName;
    WCHAR ShortNameBuffer[ BYTE_COUNT_8_DOT_3 / sizeof( WCHAR ) ];

    //
    //  Array of compound dirents.
    //

    COMPOUND_DIRENT Dirents[3];

} FILE_ENUM_CONTEXT;
typedef FILE_ENUM_CONTEXT *PFILE_ENUM_CONTEXT;

#define FILE_CONTEXT_MULTIPLE_DIRENTS       (0x00000001)


//
//  RIFF header.  Prepended to the data of a file containing XA sectors.
//  This is a hard-coded structure except that we bias the 'ChunkSize' and
//  'RawSectors' fields with the file size.  We also copy the attributes flag
//  from the system use area in the dirent.  We always initialize this
//  structure by copying the XAFileHeader.
//

typedef struct _RIFF_HEADER {

    ULONG ChunkId;
    LONG ChunkSize;
    ULONG SignatureCDXA;
    ULONG SignatureFMT;
    ULONG XAChunkSize;
    ULONG OwnerId;
    USHORT Attributes;
    USHORT SignatureXA;
    UCHAR FileNumber;
    UCHAR Reserved[7];
    ULONG SignatureData;
    ULONG RawSectors;

} RIFF_HEADER;
typedef RIFF_HEADER *PRIFF_HEADER;

//
//  Audio play header for CDDA tracks.
//

typedef struct _AUDIO_PLAY_HEADER {

    ULONG Chunk;
    ULONG ChunkSize;
    ULONG SignatureCDDA;
    ULONG SignatureFMT;
    ULONG FMTChunkSize;
    USHORT FormatTag;
    USHORT TrackNumber;
    ULONG DiskID;
    ULONG StartingSector;
    ULONG SectorCount;
    UCHAR TrackAddress[4];
    UCHAR TrackLength[4];

} AUDIO_PLAY_HEADER;
typedef AUDIO_PLAY_HEADER *PAUDIO_PLAY_HEADER;


//
//  Some macros for supporting the use of a Generic Table
//  containing all the FCB/DCBs and indexed by their FileId.
//
//  For directories:
//
//      The HighPart contains the path table offset of this directory in the
//      path table.
//
//      The LowPart contains zero except for the upper bit which is
//      set to indicate that this is a directory.
//
//  For files:
//
//      The HighPart contains the path table offset of the parent directory
//      in the path table.
//
//      The LowPart contains the byte offset of the dirent in the parent
//      directory file.
//
//  A directory is always entered into the Fcb Table as if it's
//  dirent offset was zero.  This enables any child to look in the FcbTable
//  for it's parent by searching with the same HighPart but with zero
//  as the value for LowPart.
//
//  The Id field is a LARGE_INTEGER where the High and Low parts can be
//  accessed separately.
//
//  The following macros are used to access the Fid fields.
//
//      CdQueryFidDirentOffset      - Accesses the Dirent offset field
//      CdQueryFidPathTableNumber   - Accesses the PathTable offset field
//      CdSetFidDirentOffset        - Sets the Dirent offset field
//      CdSetFidPathTableNumber     - Sets the PathTable ordinal field
//      CdFidIsDirectory            - Queries if directory bit is set
//      CdFidSetDirectory           - Sets directory bit
//

#define FID_DIR_MASK  0x80000000            // high order bit means directory.

#define CdQueryFidDirentOffset(I)           ((I).LowPart & ~FID_DIR_MASK)
#define CdQueryFidPathTableOffset(I)        ((I).HighPart)
#define CdSetFidDirentOffset(I,D)           ((I).LowPart = D)
#define CdSetFidPathTableOffset(I,P)        ((I).HighPart = P)
#define CdFidIsDirectory(I)                 FlagOn( (I).LowPart, FID_DIR_MASK )
#define CdFidSetDirectory(I)                SetFlag( (I).LowPart, FID_DIR_MASK )

#define CdSetFidFromParentAndDirent(I,F,D)  {                                           \
        CdSetFidPathTableOffset( (I), CdQueryFidPathTableOffset( (F)->FileId ));        \
        CdSetFidDirentOffset( (I), (D)->DirentOffset );                                 \
        if (FlagOn( (D)->DirentFlags, CD_ATTRIBUTE_DIRECTORY )) {                       \
            CdFidSetDirectory((I));                                                     \
        }                                                                               \
}

#endif // _CDSTRUC_

