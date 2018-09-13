/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    UdfStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    parts of the Udfs file system.

    In-Memory structures:

        The global data structures with the UdfDataRecord.  It contains a pointer
        to a File System Device object and a queue of Vcb's.  There is a Vcb for
        every currently or previously mounted volumes.  We may be in the process
        of tearing down the Vcb's which have been dismounted.  The Vcb's are
        allocated as an extension to a volume device object.

            +---------+
            | UdfData |     +--------+
            |         | --> |FilSysDo|
            |         |     |        |
            |         | <+  +--------+
            +---------+  |
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
        the volume.  Each directory Fcb contains a queue of child Lcb's for
        its children.  Each Lcb is queued onto both its parent and child Fcb.
        There can also be detached subtrees due to open operations by Id where
        the Fcb's are not connected to the root.

        The following diagram shows the root structure.

            +--------+     +--------+
            |  Vcb   |---->| Fcb    |-------------------------------------------------------------------+
            |        |     |  Table |----------------------------------------------------------------+  |
            |        |--+  |        |-------------------------------------------------------------+  |  |
            +--------+  |  +--------+                                                             |  |  |
                        |    |  |  |                                                              |  |  |
                        |    |  |  +---------------------------------------------+                |  |  |
                        |    V  +-----------------------+                        |                |  |  |
                        |  +--------+                   |                        |                |  |  |
                        |  |RootFcb |                   V                        V                |  |  |
                        +->|        |    +-----+    +--------+               +--------+           |  |  |
                           |        |<-->| Lcb |<-->|Child   |    +-----+    |Child   |           |  |  |
                           +--------+    +-----+    | Fcb    |<-->| Lcb |<-->| Fcb    |<--> ...   |  |  |
                                                    |        |    +-----+    |        |           |  |  |
                                                    +--------+               +--------+           |  |  |
                                                                                                  |  |  |
                          (Freestanding sub-tree)                                                 |  |  |
                          +--------+                                                              |  |  |
                          |OpenById|<-------------------------------------------------------------+  |  |
                          | Dir    |    +--------+                                                   |  |
                          |        |--->|OpenById|<--------------------------------------------------+  |
                          +--------+    | Child  |    +--------+                                        |
                                        |  Dir   |--->|OpenById|<---------------------------------------+
                                        +--------+    | Child  |
                                                      |  File  |
                                                      +--------+

        Attached to each Directory Fcb is an prefix table containing the
        Lcbs pointing to children of this directory for which there is an Fcb.

            +--------+      +--------+
            |  Dir   |      | Prefix |
            |   Fcb  |----->|  Table |--------------------+
            |        |      |        |-------+            |
            +--------+      +--------+       |            |
                ^              |             |            |
                |              |             |            |
                |              V             V            V
                |           +--------+    +--------+    +--------+
                |           |  Lcb   |    |  Lcb   |    |  Lcb   |
                +---------->|        |<-->|        |<-->|        |
                            +--------+    +--------+    +--------+

        Each file object open on a UDF volume contains two context pointers.  The
        first will point back to the Fcb for the file object.  The second, if present,
        points to a Ccb (ContextControlBlock) which contains the per-handle information.
        This includes the state of any directory enumeration and the Lcb used to open
        this file object.

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

        1. A resource in the UdfData synchronizes access to the Vcb queue.  This
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
            counts in all Fcbs/Lcbs.  This mutex cannot be acquired
            exclusely and is an end resource.

        6. A fast mutex in the Fcb will synchronize access to all Fcb fields
            which aren't synchronized in some other way.  A thread may acquire
            mutexes for multiple Fcb's as long as it works it way toward the
            root of the tree.  This mutex can also be acquired recursively.

        7. Normal locking order is UdfData/Vcb/Fcb starting at any point in this
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

Author:

    Dan Lovinger    [DanLo]   31-May-1996

Revision History:

--*/

#ifndef _UDFSTRUC_
#define _UDFSTRUC_

typedef PVOID PBCB;


//
//  The following structure is used to encapsulate the converted timestamps for
//  straightforward referencing.
//

typedef struct _TIMESTAMP_BUNDLE {

    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   AccessTime;
    LARGE_INTEGER   ModificationTime;

} TIMESTAMP_BUNDLE, *PTIMESTAMP_BUNDLE;


//
//  The UDF_DATA record is the top record in the UDF file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

#define NUMBER_OF_FS_OBJECTS    2

typedef struct _UDF_DATA {

    //
    //  The type and size of this record (must be UDFS_NTC_DATA_HEADER)
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
    //  Filesystem device objects for UDFS.
    //

    PDEVICE_OBJECT FileSystemDeviceObjects[NUMBER_OF_FS_OBJECTS];

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

    PVOID UdfDataLockThread;
    FAST_MUTEX UdfDataMutex;

    //
    //  A resource variable to control access to the global UDFS data record
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

    WORK_QUEUE_ITEM CloseItem;

} UDF_DATA, *PUDF_DATA;


//
//  A PARTITION will record the VSN/LSN -> PSN retrieval information for a
//  partition reference.  Since we do not support multi-volume 13346/UDF,
//  we will omit noting the volume sequence number that would tell us which
//  piece of media contained the partition.
//
//  There are currently three types of partitions used during operation: physical,
//  sparable and virtual.  However, since sparing merely adds another last layer
//  of quick indirection, we consider them as a minor extension of a physical
//  partition.
//

typedef enum _PARTITION_TYPE {
    Uninitialized,
    Physical,
    Virtual
} PARTITION_TYPE, *PPARTITION_TYPE;

//
//  A Physical partition corresponds to a single extent of the volume.
//

typedef struct _PARTITION_PHYSICAL {

    //
    //  Starting Psn and length in sectors
    //

    ULONG Start;
    ULONG Length;

    //
    //  The partition number is specified by the LVD, and refers to
    //  a specific partition descriptor on the media.  We use this
    //  in the second pass of partition discovery.
    //

    ULONG PartitionNumber;
    PNSR_PART PartitionDescriptor;

    //
    //  Spared partition map, saved temporarily between
    //  logical volume descriptor analysis and partition
    //  descriptor discover/pcb completion.
    //

    PPARTMAP_SPARABLE SparingMap;

} PARTITION_PHYSICAL, *PPARTITION_PHYSICAL;

//
//  A Virtual partition is a remapping from VSN to LSN on a given Physical
//  partition.  The remapping is done through the VAT FCB.
//

typedef struct _PARTITION_VIRTUAL{

    //
    //  The maximum Vbn in the virtual partition.
    //

    ULONG Length;
    
    //
    //  A virtual partition refers to its "host" physical partition by partition
    //  number, which we translate to a partition reference during the second pass
    //  of partition discovery.
    //
    //  Example: if the virtual partition is reference 1, hosted on partition 156
    //  (which is reference 0 for this logical volume), then NSRLBA 100/1 would
    //  refer to the block on partition ref 0 as mapped in the VAT at entry 100.
    //

    USHORT RelatedReference;

} PARTITION_VIRTUAL, *PPARTITION_VIRTUAL;
        
//
//  There is exactly one PARTITION per partition.  It is responsible for mapping
//  from some form of logical sector to a physical sector.
//

typedef struct _PARTITION {

    //
    //  This is the type of partition.
    //

    PARTITION_TYPE Type;

    union {

        PARTITION_PHYSICAL Physical;
        PARTITION_VIRTUAL Virtual;
    };

} PARTITION, *PPARTITION;

//
//  The Pcb (Partition control block) record corresponds to the partitions
//  which collectively form the mounted volume.  Exactly one of these is
//  linked off of the Vcb.
//

typedef struct _PCB {

    //
    //  The type and size of this record (must be UDFS_NTC_PCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  This is the number of partitions in the map
    //

    USHORT Partitions;

    //
    //  A bitmask of flags.
    //

    USHORT Flags;

    //
    //  Sparing Mcb, if this volume has sparing.
    //

    PLARGE_MCB SparingMcb;

    //
    //  This is the mapping table.  A PCB will be dynamically sized
    //  according to the number of partitions forming the volume.
    //

    PARTITION Partition[0];

} PCB, *PPCB;

//
//  Indicate what kinds of partitions are contained for quick checks.
//

#define PCB_FLAG_PHYSICAL_PARTITION     0x0001
#define PCB_FLAG_VIRTUAL_PARTITION      0x0002
#define PCB_FLAG_SPARABLE_PARTITION     0x0004


//
//  The Vmcb structure is a double mapped structure for mapping
//  between VBNs and LBNs using the MCB structures.  The whole structure
//  is also protected by a private mutex.  This record must be allocated
//  from non-paged pool.
//

//
//  We use an #if to snip out historical code in the Vmcb package that
//  dealt with write issues, leaving it for the future.
//

#define VMCB_WRITE_SUPPORT 0

typedef struct _VMCB {

    KMUTEX Mutex;

    MCB VbnIndexed;     // maps VBNs to LBNs
    MCB LbnIndexed;     // maps LBNs to VBNs

    ULONG MaximumLbn;

    ULONG SectorSize;

#if VMCB_WRITE_SUPPORT

    RTL_GENERIC_TABLE DirtyTable;

#endif // VMCB_WRITE_SUPPORT

} VMCB, *PVMCB;

//
//  The Vcb (Volume control block) record corresponds to every
//  volume mounted by the file system.  They are ordered in a queue off
//  of UdfData.VcbQueue.
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
    //  The type and size of this record (must be UDFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Vpb for this volume.
    //

    PVPB Vpb;

    //
    //  Pcb for this volume.
    //

    PPCB Pcb;

    //
    //  Device object for the driver below us.
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  Link into queue of Vcb's in the UdfData structure.  We will create a union with
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
    //  File object used to lock the volume.
    //

    PFILE_OBJECT VolumeLockFileObject;

    //
    //  Media change count from device driver for bulletproof detection
    //  of media movement
    //

    ULONG MediaChangeCount;

    //
    //  Logical block size for this volume.
    //

    ULONG SectorSize;

    //
    //  Associated shift size
    //

    ULONG SectorShift;

    //
    //  LSN of the bounds that CD-UDF defines.
    //
    //  S - start of the session that contains the AVD @ +256
    //  N - end of the disc, another chance to find AVD @ -256,
    //      and discovery of the VAT ICB.
    //
    //  N may be unset until late in the mount sequence for a volume, since
    //  the device may not respond to CD-style TOC requests, and only then
    //  be a guess based on the partitons we find. S will be zero except in
    //  the case of CD-UDF.  In a mounted system, S will correspond to where
    //  we started finding the volume descriptors that let us proceed.
    //

    ULONG BoundS;
    ULONG BoundN;

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
    //  These are the number of times a mounted Vcb will be referenced on behalf
    //  of the system.  See commentary in udfdata.h.
    //

    ULONG VcbResidualReference;
    ULONG VcbResidualUserReference;

    //
    //  Fcb for the Volume Dasd file, root directory and the Vmcb-mapped Metadata stream.
    //  The VAT Fcb is only created on CD UDF media, for the Virtual Allocation Table.
    //

    struct _FCB *VolumeDasdFcb;
    struct _FCB *RootIndexFcb;
    struct _FCB *MetadataFcb;
    struct _FCB *VatFcb;

    //
    //  Vmcb for the metadata stream
    //

    VMCB Vmcb;
    
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
    //  Fcb table.  Synchronized with the Vcb fast mutex.
    //

    RTL_GENERIC_TABLE FcbTable;

} VCB, *PVCB;

#define VCB_STATE_LOCKED                            (0x00000001)
#define VCB_STATE_REMOVABLE_MEDIA                   (0x00000002)
#define VCB_STATE_NOTIFY_REMOUNT                    (0x00000004)
#define VCB_STATE_METHOD_2_FIXUP                    (0x00000008)


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

} VOLUME_DEVICE_OBJECT, *PVOLUME_DEVICE_OBJECT;


//
//  Udfs file id is a large integer. This corresponds to the FileInternalInformation
//  query type and is used for internal FCB indexing.
//

typedef LARGE_INTEGER                   FILE_ID, *PFILE_ID;


//
//  Lcb (Link Control Block), which corresponds to a link from a directory (or in
//  the future, other container objects) to a file (UDF File Identifier).  There is
//  one of these for each name tuple in a prefix table.
//

typedef struct _LCB {

    //
    //  Type and size of this record (must be UDFS_NTC_LCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Pointer to the Parent Fcb for this entry and queue for Parent to
    //  find all referencing Lcbs.  Corresponds to Fcb->ChildLcbQueue.
    //

    LIST_ENTRY ParentFcbLinks;
    struct _FCB *ParentFcb;

    //
    //  Pointer to Child (referenced) Fcb for this entry and queue for Child
    //  to find all referencing Lcbs.  Corresponds to Fcb->ParentLcbQueue.
    //

    LIST_ENTRY ChildFcbLinks;
    struct _FCB *ChildFcb;

    //
    //  Number of extra realtime references made to this Lcb.
    //

    ULONG Reference;

    //
    //  Flags indicating the state of this Lcb.
    //

    ULONG Flags;

    //
    //  File attributes to be merged with the child Fcb.  UDF seperates interesting
    //  information into the FID and FE so, properly, the name link (corresponding to
    //  a FID) must record some extra information.
    //

    ULONG FileAttributes;

    //
    //  Splay links in the prefix tree.
    //
    
    RTL_SPLAY_LINKS Links;

    //
    //  The name of this link.
    //

    UNICODE_STRING FileName;

} LCB, *PLCB;

#define LCB_FLAG_IGNORE_CASE        0x00000001
#define LCB_FLAG_SHORT_NAME         0x00000002
#define LCB_FLAG_POOL_ALLOCATED     0x00000004

//
//  We build a lookaside of Lcb capable of holding a reasonably sized name.
//

#define SIZEOF_LOOKASIDE_LCB        ( sizeof( LCB ) + ( sizeof( WCHAR ) * 16 ))


//
//  The following two structures are the separate union structures for
//  data and index Fcb's.
//

typedef enum _FCB_CONDITION {
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

typedef struct _FCB_NONPAGED {

    //
    //  Type and size of this record must be UDFS_NTC_FCB_NONPAGED
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

} FCB_NONPAGED;
typedef FCB_NONPAGED *PFCB_NONPAGED;

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

} FCB_DATA, *PFCB_DATA;

typedef struct _FCB_INDEX {

    //
    //  Internal stream file for the directory.
    //

    PFILE_OBJECT FileObject;

    //
    //  Root of splay trees for exact and ignore case prefix trees.
    //

    PRTL_SPLAY_LINKS ExactCaseRoot;
    PRTL_SPLAY_LINKS IgnoreCaseRoot;

} FCB_INDEX, *PFCB_INDEX;

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
    //      UDFS_NTC_FCB_INDEX
    //      UDFS_NTC_FCB_DATA
    //

    //
    //  Common Fsrtl Header.  The named header is for the fieldoff.c output.  We
    //  use the unnamed header internally.
    //

    union {

        FSRTL_COMMON_FCB_HEADER Header;
        FSRTL_COMMON_FCB_HEADER;
    };

    //
    //  Vcb for this Fcb.
    //

    PVCB Vcb;

    //
    //  Queues of Lcbs that are on this Fcb: Parent - edges that lead in
    //                                       Child  - edges that lead out
    //
    //  We anticipate supporting the streaming extension to UDF 2.0, so we
    //  leave the ChildLcbQueue here which in the case of a stream-rich file
    //  will contain a solitary Lcb leading to the stream directory.
    //

    LIST_ENTRY ParentLcbQueue;
    LIST_ENTRY ChildLcbQueue;

    //
    //  Length of Root ICB Extent for this object.  Coupled with the information
    //  in the FileId, this will allow discovery of the active File Entry for this
    //  Fcb at any time.
    //

    ULONG RootExtentLength;

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
    //  FcbMutex.
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
    //  This is the thread and count for the thread which has locked this
    //  Fcb.
    //

    PVOID FcbLockThread;
    ULONG FcbLockCount;

    //
    //  Information for Lsn->Psn mapping.  If the file data is embedded, we have a
    //  lookup into the metadata stream for the single logical block and an offset
    //  of the data within that block.  If the file data is is external, we have a
    //  regular Mapping Control Block.
    //
    //  Metadata structures are mapped through the volume-level Metadata Fcb which
    //  uses the volume's VMCB.
    //

    union {
        
        LARGE_MCB Mcb;

        struct EMBEDDED_MAPPING {
            
            ULONG EmbeddedVsn;
            ULONG EmbeddedOffset;
        };
    };

    //
    //  This is the nonpaged data for the Fcb
    //

    PFCB_NONPAGED FcbNonpaged;

    //
    //  Share access structure.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  We cache a few fields from the FE so that various operations do not have to
    //  hit the disk (query, etc.).
    //
    
    //
    //  Time stamps for this file.
    //

    TIMESTAMP_BUNDLE Timestamps;

    //
    //  Link count on this file.
    //

    USHORT LinkCount;

    union {

        ULONG FcbType;
        FCB_INDEX;
        FCB_DATA;
    };

} FCB, *PFCB;

#define FCB_STATE_INITIALIZED                   (0x00000001)
#define FCB_STATE_IN_FCB_TABLE                  (0x00000002)
#define FCB_STATE_VMCB_MAPPING                  (0x00000004)
#define FCB_STATE_EMBEDDED_DATA                 (0x00000008)
#define FCB_STATE_MCB_INITIALIZED               (0x00000010)

#define SIZEOF_FCB_DATA              \
    (FIELD_OFFSET( FCB, FcbType ) + sizeof( FCB_DATA ))

#define SIZEOF_FCB_INDEX             \
    (FIELD_OFFSET( FCB, FcbType ) + sizeof( FCB_INDEX ))


//
//  The Ccb record is allocated for every user file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be UDFS_NTC_CCB)
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
    //  Lcb for the file being opened.
    //

    PLCB Lcb;

    //
    //  We store state information in the Ccb for a directory
    //  enumeration on this handle.
    //

    //
    //  Offset in the virtual directory stream to base the next enumeration.
    //
    //  A small number (in fact, possibly one) of file indices are reserved for
    //  synthesized directory entries (like '.'). Past that point, CurrentFileIndex -
    //  UDF_MAX_SYNTHESIZED_FILEINDEX is a byte offset in the stream.
    //

    LONGLONG CurrentFileIndex;
    UNICODE_STRING SearchExpression;

    //
    //  Highest ULONG-representable FileIndex so far found in the directory stream.
    //  This corresponds to the highest FileIndex returnable in a query structure.
    //

    ULONG HighestReturnableFileIndex;

} CCB, *PCCB;

#define CCB_FLAG_OPEN_BY_ID                     (0x00000001)
#define CCB_FLAG_OPEN_RELATIVE_BY_ID            (0x00000002)
#define CCB_FLAG_IGNORE_CASE                    (0x00000004)

//
//  Following flags refer to index enumeration.
//

#define CCB_FLAG_ENUM_NAME_EXP_HAS_WILD         (0x00010000)
#define CCB_FLAG_ENUM_MATCH_ALL                 (0x00020000)
#define CCB_FLAG_ENUM_RETURN_NEXT               (0x00040000)
#define CCB_FLAG_ENUM_INITIALIZED               (0x00080000)
#define CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY    (0x00100000)


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the UdfComplete
//  request routine
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be UDFS_NTC_IRP_CONTEXT)
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

        struct _UDF_IO_CONTEXT *IoContext;
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

} IRP_CONTEXT, *PIRP_CONTEXT;

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
    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS     |   \
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
    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP                 \
)


//
//  Following structure is used to queue a request to the delayed close queue.
//  This structure should be the minimum block allocation size.
//

typedef struct _IRP_CONTEXT_LITE {

    //
    //  Type and size of this record (must be UDFS_NTC_IRP_CONTEXT_LITE)
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

} IRP_CONTEXT_LITE, *PIRP_CONTEXT_LITE;


//
//  Context structure for asynchronous I/O calls.  Most of these fields
//  are actually only required for the ReadMultiple routines, but
//  the caller must allocate one as a local variable anyway before knowing
//  whether there are multiple requests are not.  Therefore, a single
//  structure is used for simplicity.
//

typedef struct _UDF_IO_CONTEXT {

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

} UDF_IO_CONTEXT, *PUDF_IO_CONTEXT;


//
//  Following structure is used to track the top level request.  Each Udfs
//  Fsd and Fsp entry point will examine the top level irp location in the
//  thread local storage to determine if this request is top level and/or
//  top level Udfs.  The top level Udfs request will remember the previous
//  value and update that location with a stack location.  This location
//  can be accessed by recursive Udfs entry points.
//

typedef struct _THREAD_CONTEXT {

    //
    //  UDFS signature.  Used to confirm structure on stack is valid.
    //

    ULONG Udfs;

    //
    //  Previous value in top-level thread location.  We restore this
    //  when done.
    //

    PIRP SavedTopLevelIrp;

    //
    //  Top level Udfs IrpContext.  Initial Udfs entry point on stack
    //  will store the IrpContext for the request in this stack location.
    //

    PIRP_CONTEXT TopLevelIrpContext;

} THREAD_CONTEXT, *PTHREAD_CONTEXT;


//
//  Following structure is used to build up static data for parse tables
//

typedef struct _PARSE_KEYVALUE {
    PCHAR Key;
    ULONG Value;
} PARSE_KEYVALUE, *PPARSE_KEYVALUE;


//
//  Some macros for supporting the use of a Generic Table
//  containing all the FCB and indexed by their FileId.
//
//  The ISO 13346 lb_addr of the ICB hierarchy of the object
//
//      { ULONG BlockNo; USHORT PartitionId }
//
//  is encoded in the LowPart (BlockNo) and low 16 bits of the
//  HighPart (PartitionId). The top 16 bits are reserved and are
//  currently used to indicate the type of the object being referenced
//  (file or directory).
//
//  NOTE: this FileId prevents us from being able crack the name of
//  object since an ICB hierarchy's contained direct File Entrys do
//  not (and cannot) contain backpointers to the containing directory.
//  In order to be able to crack paths, we need to be able to do a
//  directory/dirent offset, which cannot fit in 64bits of FileId.
//  A FileId must be 64bits since we export this in the FileInternalInforation
//  query.
//
//  Also, even through we are restricted to a single partition in this
//  implementation, getting those "spare" 16bits isn't good enough to let us
//  point directly into a directory's File Identifier. Files and by extension
//  directories can exceed 2^32 bytes/entries.  Once we have pointed at the
//  parent dir, we are out of bits.
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

#define FID_DIR_MASK  0x80000000        // high order bit means directory.

#define UdfSetFidFromLbAddr(I, LBA)     { (I).LowPart = (LBA).Lbn; \
                                          (I).HighPart = (ULONG) (LBA).Partition; }

#define UdfGetFidLbn(I)                 ((I).LowPart)
#define UdfGetFidPartition(I)           ((USHORT) (((I).HighPart & ~FID_DIR_MASK) & MAXUSHORT))
#define UdfGetFidReservedZero(I)        ((I).HighPart & ~(FID_DIR_MASK|MAXUSHORT))

#define UdfSetFidFile(I)                ClearFlag( (I).HighPart, FID_DIR_MASK )
#define UdfSetFidDirectory(I)           SetFlag( (I).HighPart, FID_DIR_MASK )

#define UdfIsFidFile(I)                 BooleanFlagOff( (I).HighPart, FID_DIR_MASK )
#define UdfIsFidDirectory(I)            BooleanFlagOn( (I).HighPart, FID_DIR_MASK )


//
//  Context structures for browsing through structures
//

//
//  A mapped view is a useful bundle to hold information about a physical
//  view of the disk.
//

typedef struct _MAPPED_PVIEW {

    //
    //  A mapped extent and CC control block
    //

    PVOID View;
    PBCB Bcb;

    //
    //  Extent location
    //

    USHORT Partition;
    ULONG Lbn;
    ULONG Length;

} MAPPED_PVIEW, *PMAPPED_PVIEW;


//
//  Enumeration contexts for various operations.
//

//
//  The following is used for crawling ICB hierarchies searching
//  for some notion of an active entry.
//

typedef struct _ICB_SEARCH_CONTEXT {

    //
    //  Vcb the search is occuring on.
    //

    PVCB Vcb;

    //
    //  Type of Icb being searched for.
    //

    USHORT IcbType;

    //
    //  The Active is most prevailing ICB so far found.
    //

    MAPPED_PVIEW Active;
    
    //
    //  The current logical block extent being read from the disk.
    //

    MAPPED_PVIEW Current;

} ICB_SEARCH_CONTEXT, *PICB_SEARCH_CONTEXT;

//
//  The following is used for crawling Extended Attributes extending off of
//  a direct ICB
//

typedef enum _EA_SEARCH_TYPE {

    EaEnumBad = 0,
    EaEnumISO,
    EaEnumImplementation,
    EaEnumApplication

} EA_SEARCH_TYPE, *PEA_SEARCH_TYPE;

typedef struct _EA_SEARCH_CONTEXT {

    //
    //  Reference to an elaborated ICB_SEARCH_CONTEXT which gives us a handle
    //  onto a direct ICB to crawl.
    //

    PICB_SEARCH_CONTEXT IcbContext;

    //
    //  The current Ea being looked at.
    //

    PVOID Ea;

    //
    //  Bytes remaining in the EA view
    //

    ULONG Remaining;

    //
    //  EA being searched for.  We only support looking for ISO at this time.
    //

    ULONG EAType;
    USHORT EASubType;

} EA_SEARCH_CONTEXT, *PEA_SEARCH_CONTEXT;

//
//  The following is used to crawl the list of allocation extent descriptors attached
//  to an ICB.
//

typedef struct _ALLOC_ENUM_CONTEXT {

    //
    //  Reference to an elaborated ICB_ENUM_CONTEXT which gives us a handle
    //  onto a direct ICB to crawl.
    //

    PICB_SEARCH_CONTEXT IcbContext;

    //
    //  The current allocation descriptor being looked at.
    //

    PVOID Alloc;

    //
    //  Type of allocation descriptors in this enumeration
    //

    ULONG AllocType;

    //
    //  Bytes remaining in this view.
    //

    ULONG Remaining;

} ALLOC_ENUM_CONTEXT, *PALLOC_ENUM_CONTEXT;

//
//  The following is used to crawl a logical directory.
//

typedef struct _DIR_ENUM_CONTEXT {

    //
    //  The current view in the enumeration.
    //

    PVOID View;
    PBCB Bcb;

    //
    //  Offset of the view from the beginning of the directory.
    //
    
    LARGE_INTEGER BaseOffset;

    //
    //  Length of the view which is valid and the current
    //  offset in it.
    //

    ULONG ViewLength;
    ULONG ViewOffset;

    //
    //  Pointer to the current FID.
    //

    PNSR_FID Fid;

    //
    //  Offset to the next fid from the beginning of the view.
    //
    
    ULONG NextFidOffset;

    //
    //  Flags indicating the state of the enumeration.
    //

    ULONG Flags;

    //
    //  Converted names from the FID. Case name is "case appropriate" for
    //  the operation.
    //

    UNICODE_STRING ObjectName;
    UNICODE_STRING CaseObjectName;

    //
    //  Real object name in pure form (not rendered to NT legal form)
    //

    UNICODE_STRING PureObjectName;

    //
    //  Short name for the object.
    //

    UNICODE_STRING ShortObjectName;

    //
    //  Currently allocated space for the name.  The previous strings are
    //  carved out of this single buffer.
    //

    PVOID NameBuffer;

    //
    //  Size of currently allocated name buffer for the lfn names.
    //

    USHORT AllocLength;

} DIR_ENUM_CONTEXT, *PDIR_ENUM_CONTEXT;

//
//  Flags for noting where in the enumeration we are.
//

#define DIR_CONTEXT_FLAG_SEEN_NONCONSTANT       0x0001
#define DIR_CONTEXT_FLAG_SEEN_PARENT            0x0002

//
//  Flag indicating current Fid was buffered into pool.
//

#define DIR_CONTEXT_FLAG_FID_BUFFERED           0x0004

#endif // _CDSTRUC_
