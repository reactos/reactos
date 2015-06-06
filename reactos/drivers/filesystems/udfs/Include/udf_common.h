////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
#ifndef __UDF_COMMON_STRUCT__H__
#define __UDF_COMMON_STRUCT__H__

typedef enum _UDFFSD_MEDIA_TYPE {
    MediaUnknown = 0,
    MediaHdd,
    MediaCdr,
    MediaCdrw,
    MediaCdrom,
    MediaZip,
    MediaFloppy,
    MediaDvdr,
    MediaDvdrw
} UDFFSD_MEDIA_TYPE;

#define MAX_UDFFSD_MEDIA_TYPE ((ULONG)MediaFloppy)

typedef struct _UDF_KEY_LIST {
    int32 d[4];
} UDF_KEY_LIST, *PUDF_KEY_LIST;

struct UDF_MEDIA_CLASS_NAMES
{
    UDFFSD_MEDIA_TYPE MediaClass;
    PWCHAR ClassName;
};

extern struct UDF_MEDIA_CLASS_NAMES UDFMediaClassName[];

#define MAX_ANCHOR_LOCATIONS 11
#define MAX_SPARING_TABLE_LOCATIONS 32

typedef struct _UDFVolumeControlBlock {

#ifdef _UDF_STRUCTURES_H_

    //---------------
    // Kernel-only data:
    //---------------

    //---------------
    // Fcb data
    //---------------

    union {
        struct {
            UDFIdentifier                       NodeIdentifier;
            //  compatibility field
            PtrUDFNTRequiredFCB                 NTRequiredFCB;
            // UDF related data (compatibility field)
            PVOID                               Reserved0;
            // this FCB belongs to some mounted logical volume
            // (compatibility field - pointer to itself)
            struct _UDFVolumeControlBlock*      Vcb;
            // a global list of all FCB structures associated with the VCB
            LIST_ENTRY                          NextFCB;
            // some state information for the FCB is maintained using the
            //  flags field
            uint32                              FCBFlags;
            // all CCB's for this particular FCB are linked off the following
            //  list head.
            // For volume open operations, we do not create a FCB (we use the VCB
            //  directly instead). Therefore, all CCB structures for the volume
            //  open operation are linked directly off the VCB
            LIST_ENTRY                          VolumeOpenListHead; // CCB
            // A count of the number of open files/directories
            //  As long as the count is != 0, the volume cannot
            //  be dismounted or locked.
            uint32                              VolumeOpenCount;
            uint32                              Reserved2;
            uint32                              Reserved3;

            PVOID                               Reserved4;
            ERESOURCE                           CcbListResource;

            struct _UDFFileControlBlock*        ParentFcb;
            //  Pointer to IrpContextLite in delayed queue.
            struct _UDFIrpContextLite*          IrpContextLite;
            uint32                              CcbCount;
        };
        UDFFCB VcbAsFcb;
    };

    //---------------
    // Vcb-only data
    //---------------

    uint32                              VCBOpenCount;
    uint32                              VCBOpenCountRO;
    uint32                              VCBHandleCount;
    // a resource to protect the fields contained within the VCB
    ERESOURCE                           FcbListResource;
    ERESOURCE                           FlushResource;
    // each VCB is accessible off a global linked list
    LIST_ENTRY                          NextVCB;
    // each VCB points to a VPB structure created by the NT I/O Manager
    PVPB                                Vpb;
    // we will maintain a global list of IRP's that are pending
    //  because of a directory notify request.
    LIST_ENTRY                          NextNotifyIRP;
    // the above list is protected only by the mutex declared below
    PNOTIFY_SYNC                        NotifyIRPMutex;
    // for each mounted volume, we create a device object. Here then
    //  is a back pointer to that device object
    PDEVICE_OBJECT                      VCBDeviceObject;
    BOOLEAN                             ShutdownRegistered;
    // We also retain a pointer to the physical device object on which we
    // have mounted ourselves. The I/O Manager passes us a pointer to this
    // device object when requesting a mount operation.
    PDEVICE_OBJECT                      TargetDeviceObject;
    UNICODE_STRING                      TargetDevName;
    PWSTR                               DefaultRegName;
    // the volume structure contains a pointer to the root directory FCB
    PtrUDFFCB                           RootDirFCB;
    // the complete name of the user visible drive letter we serve
    PUCHAR                              PtrVolumePath;
    // Pointer to a stream file object created for the volume information
    // to be more easily read from secondary storage (with the support of
    // the NT Cache Manager).
/*    PFILE_OBJECT                        PtrStreamFileObject;
    // Required to use the Cache Manager.
*/
    struct _FILE_SYSTEM_STATISTICS*     Statistics;
    // Volume lock file object - used in Lock/Unlock routines
    ULONG                               VolumeLockPID;
    PFILE_OBJECT                        VolumeLockFileObject;
    DEVICE_TYPE                         FsDeviceType;

    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    ULONG PostedRequestCount;
    ULONG ThreadsPerCpu;
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    ULONG OverflowQueueCount;
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    LIST_ENTRY OverflowQueue;
    //  The following spinlock protects access to all the above fields.
    KSPIN_LOCK OverflowQueueSpinLock;
    ULONG StopOverflowQueue;
    ULONG SystemCacheGran;

    //---------------
    // 
    //---------------

    // Eject Request waiter
    struct _UDFEjectWaitContext*  EjectWaiter;
    KEVENT          WaiterStopped;
    ULONG           SoftEjectReq;

    ULONG           BM_FlushTime;
    ULONG           BM_FlushPriod;
    ULONG           Tree_FlushTime;
    ULONG           Tree_FlushPriod;
    ULONG           SkipCountLimit;
    ULONG           SkipEjectCountLimit;

/*    // XP CD Burner related data
    UNICODE_STRING  CDBurnerVolume;
    BOOLEAN         CDBurnerVolumeValid;
*/
    // Background writes counter
    LONG            BGWriters;
    // File Id cache
    struct _UDFFileIDCacheItem* FileIdCache;
    ULONG           FileIdCount;
    //
    ULONG           MediaLockCount;

    BOOLEAN         IsVolumeJustMounted;
#else

    //---------------
    // Win32-only data:
    //---------------

  #ifdef _BROWSE_UDF_
    PUDF_FILE_INFO  RootFileInfo;
  #endif


  #ifdef UDF_FORMAT_MEDIA
    struct _UDFFmtState* fms;
  #endif //UDF_FORMAT_MEDIA


    PDEVICE_OBJECT  TargetDeviceObject;
    ULONG           FsDeviceType;
    ULONG           PhDeviceType;

#endif //_UDF_STRUCTURES_H_

    // FS size cache
    LONGLONG        TotalAllocUnits;
    LONGLONG        FreeAllocUnits;
    LONGLONG        EstimatedFreeUnits;

    // a resource to protect the fields contained within the VCB
    ERESOURCE                           VCBResource;
    ERESOURCE                           BitMapResource1;
    ERESOURCE                           FileIdResource;
    ERESOURCE                           DlocResource;
    ERESOURCE                           DlocResource2;
    ERESOURCE                           PreallocResource;
    ERESOURCE                           IoResource;

    //---------------
    // Physical media parameters
    //---------------

    ULONG           BlockSize;
    ULONG           BlockSizeBits;
    ULONG           WriteBlockSize;
    ULONG           LBlockSize;
    ULONG           LBlockSizeBits;
    ULONG           LB2B_Bits;
    // Number of last session
    ULONG           LastSession;
    ULONG           FirstTrackNum;
    ULONG           FirstTrackNumLastSes;
    ULONG           LastTrackNum;
    // First & Last LBA of the last session
    ULONG           FirstLBA;
    ULONG           FirstLBALastSes;
    ULONG           LastLBA;
    // Last writable LBA
    ULONG           LastPossibleLBA;
    // First writable LBA
    ULONG           NWA;
    // sector type map
    struct _UDFTrackMap*  TrackMap;
    ULONG           LastModifiedTrack;
    ULONG           LastReadTrack;
    ULONG           CdrwBufferSize;
    ULONG           CdrwBufferSizeCounter;
    uint32          SavedFeatures;
    // OPC info
//    PCHAR           OPC_buffer;
    UCHAR           OPCNum;
    BOOLEAN         OPCDone;
    UCHAR           MediaType;
    UCHAR           MediaClassEx;

    UCHAR           DiscStat;
    UCHAR           PhErasable;
    UCHAR           PhDiskType;
    UCHAR           PhMediaCapFlags;

    UCHAR           MRWStatus;
    UCHAR           UseEvent;
    BOOLEAN         BlankCD;
    UCHAR           Reserved;

    ULONG           PhSerialNumber;

    // Speed control
    SET_CD_SPEED_EX_USER_IN SpeedBuf;
    ULONG           MaxWriteSpeed;
    ULONG           MaxReadSpeed;
    ULONG           CurSpeed;

    BOOLEAN         CDR_Mode;
    BOOLEAN         DVD_Mode;
    BOOLEAN         WriteParamsReq;

#define SYNC_CACHE_RECOVERY_NONE     0
#define SYNC_CACHE_RECOVERY_ATTEMPT  1
#define SYNC_CACHE_RECOVERY_RETRY    2

    UCHAR           SyncCacheState;

    // W-cache
    W_CACHE         FastCache;
    ULONG           WCacheMaxFrames;
    ULONG           WCacheMaxBlocks;
    ULONG           WCacheBlocksPerFrameSh;
    ULONG           WCacheFramesToKeepFree;

    PCHAR           ZBuffer;
    PCHAR           fZBuffer;
    ULONG           fZBufferSize;
    PSEND_OPC_INFO_HEADER_USER_IN OPCh;
    PGET_WRITE_MODE_USER_OUT WParams;
    PGET_LAST_ERROR_USER_OUT Error;

    ULONG           IoErrorCounter;
    // Media change count (equal to the same field in CDFS VCB)
    ULONG           MediaChangeCount;

#define INCREMENTAL_SEEK_NONE        0
#define INCREMENTAL_SEEK_WORKAROUND  1
#define INCREMENTAL_SEEK_DONE        2

    UCHAR           IncrementalSeekState;

    BOOLEAN         VerifyOnWrite;
    BOOLEAN         DoNotCompareBeforeWrite;
    BOOLEAN         CacheChainedIo;

    ULONG           MountPhErrorCount;

    // a set of flags that might mean something useful
    uint32          VCBFlags;
    BOOLEAN         FP_disc;

    //---------------
    // UDF related data
    //---------------

#ifdef _BROWSE_UDF_

    // Anchors LBA
    ULONG           Anchor[MAX_ANCHOR_LOCATIONS];
    ULONG           BadSeqLoc[MAX_ANCHOR_LOCATIONS*2];
    OSSTATUS        BadSeqStatus[MAX_ANCHOR_LOCATIONS*2];
    ULONG           BadSeqLocIndex;
    // Volume label
    UNICODE_STRING  VolIdent;
    // Volume creation time
    int64           VolCreationTime;
    // Root & SystemStream lb_addr
    lb_addr         RootLbAddr;
    lb_addr         SysStreamLbAddr;
    // Number of partition
    ULONG           PartitionMaps;
    // Pointer to partition structures
    PUDFPartMap     Partitions;
    LogicalVolIntegrityDesc *LVid;
    uint32          IntegrityType;
    uint32          origIntegrityType;
    extent_ad       LVid_loc;
    ULONG           SerialNumber;
    // on-disk structure version control
    uint16          CurrentUDFRev;
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
    // sparing table
    ULONG           SparingCountFree;
    ULONG           SparingCount;
    ULONG           SparingBlockSize;
    struct _SparingEntry*    SparingTable;
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
#ifdef UDF_TRACK_FS_STRUCTURES
    PEXTENT_MAP     FsStructMap;
    PULONG          FE_link_counts; // 0 - free
#endif //UDF_TRACK_FS_STRUCTURES
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS

    PCHAR           FSBM_OldBitmap;  // 0 - free, 1 - used
    ULONG           BitmapModified;

    PCHAR           ZSBM_Bitmap;     // 0 - data, 1 - zero-filleld
#endif //_BROWSE_UDF_

    PCHAR           BSBM_Bitmap;     // 0 - normal, 1 - bad-block

#ifdef _BROWSE_UDF_
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
    // FE location cache
    PUDF_DATALOC_INDEX DlocList;
    ULONG           DlocCount;
    // FS compatibility
    USHORT          DefaultAllocMode; // Default alloc mode (from registry)
    BOOLEAN         UseExtendedFE;
    BOOLEAN         LowFreeSpace;
    UDFFSD_MEDIA_TYPE MediaTypeEx;
    ULONG           DefaultUID;
    ULONG           DefaultGID;
    ULONG           DefaultAttr;      // Default file attributes (NT-style)


    UCHAR           PartitialDamagedVolumeAction;
    BOOLEAN         NoFreeRelocationSpaceVolumeAction;
    BOOLEAN         WriteSecurity;
    BOOLEAN         FlushMedia;
    BOOLEAN         ForgetVolume;
    UCHAR           Reserved5[3];

    //
    ULONG           FECharge;
    ULONG           FEChargeSDir;
    ULONG           PackDirThreshold;
    ULONG           SparseThreshold;  // in blocks

    PUDF_ALLOCATION_CACHE_ITEM    FEChargeCache;
    ULONG                         FEChargeCacheMaxSize;

    PUDF_ALLOCATION_CACHE_ITEM    PreallocCache;
    ULONG                         PreallocCacheMaxSize;

    UDF_VERIFY_CTX  VerifyCtx;

    PUCHAR          Cfg;
    ULONG           CfgLength;
    ULONG           CfgVersion;

#endif //_BROWSE_UDF_
    uint32          CompatFlags;
    uint32          UserFSFlags;
    UCHAR           ShowBlankCd;

} VCB, *PVCB;


// some valid flags for the VCB
#define         UDF_VCB_FLAGS_VOLUME_MOUNTED        (0x00000001)
#define         UDF_VCB_FLAGS_VOLUME_LOCKED         (0x00000002)
#define         UDF_VCB_FLAGS_BEING_DISMOUNTED      (0x00000004)
#define         UDF_VCB_FLAGS_SHUTDOWN              (0x00000008)
#define         UDF_VCB_FLAGS_VOLUME_READ_ONLY      (0x00000010)

#define         UDF_VCB_FLAGS_VCB_INITIALIZED       (0x00000020)
#define         UDF_VCB_FLAGS_OUR_DEVICE_DRIVER     (0x00000040)
#define         UDF_VCB_FLAGS_NO_SYNC_CACHE         (0x00000080)
#define         UDF_VCB_FLAGS_REMOVABLE_MEDIA       (0x00000100)
#define         UDF_VCB_FLAGS_MEDIA_LOCKED          (0x00000200)
#define         UDF_VCB_SKIP_EJECT_CHECK            (0x00000400)
//#define         UDF_VCB_FS_BITMAP_MODIFIED          (0x00000800)  // moved to separate flag
#define         UDF_VCB_LAST_WRITE                  (0x00001000)
#define         UDF_VCB_FLAGS_TRACKMAP              (0x00002000)
#define         UDF_VCB_ASSUME_ALL_USED             (0x00004000)

#define         UDF_VCB_FLAGS_RAW_DISK              (0x00040000)
#define         UDF_VCB_FLAGS_USE_STD               (0x00080000)

#define         UDF_VCB_FLAGS_STOP_WAITER_EVENT     (0x00100000)
#define         UDF_VCB_FLAGS_NO_DELAYED_CLOSE      (0x00200000)
#define         UDF_VCB_FLAGS_MEDIA_READ_ONLY       (0x00400000)

#define         UDF_VCB_FLAGS_FLUSH_BREAK_REQ       (0x01000000)
#define         UDF_VCB_FLAGS_EJECT_REQ             (0x02000000)
#define         UDF_VCB_FLAGS_FORCE_SYNC_CACHE      (0x04000000)

#define         UDF_VCB_FLAGS_USE_CAV               (0x08000000)
#define         UDF_VCB_FLAGS_UNSAFE_IOCTL          (0x10000000)
#define         UDF_VCB_FLAGS_DEAD                  (0x20000000)  // device unexpectedly disappeared


// flags for FS Interface Compatibility
#define         UDF_VCB_IC_UPDATE_ACCESS_TIME          (0x00000001)
#define         UDF_VCB_IC_UPDATE_MODIFY_TIME          (0x00000002)
#define         UDF_VCB_IC_UPDATE_ATTR_TIME            (0x00000004)
#define         UDF_VCB_IC_UPDATE_ARCH_BIT             (0x00000008)
#define         UDF_VCB_IC_UPDATE_DIR_WRITE            (0x00000010)
#define         UDF_VCB_IC_UPDATE_DIR_READ             (0x00000020)
#define         UDF_VCB_IC_WRITE_IN_RO_DIR             (0x00000040)
#define         UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME (0x00000080)
#define         UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS      (0x00000100)
#define         UDF_VCB_IC_HW_RO                       (0x00000200)
#define         UDF_VCB_IC_OS_NATIVE_DOS_NAME          (0x00000400)
#define         UDF_VCB_IC_FORCE_WRITE_THROUGH         (0x00000800)
#define         UDF_VCB_IC_FORCE_HW_RO                 (0x00001000)
#define         UDF_VCB_IC_IGNORE_SEQUENTIAL_IO        (0x00002000)
#define         UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE    (0x00004000)
#define         UDF_VCB_IC_BAD_RW_SEEK                 (0x00008000)
#define         UDF_VCB_IC_FP_ADDR_PROBLEM             (0x00010000)
#define         UDF_VCB_IC_MRW_ADDR_PROBLEM            (0x00020000)
#define         UDF_VCB_IC_BAD_DVD_LAST_LBA            (0x00040000)
#define         UDF_VCB_IC_SYNCCACHE_BEFORE_READ       (0x00080000)
#define         UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS  (0x00100000)
#define         UDF_VCB_IC_SOFT_RO                     (0x00200000)

#define         UDF_VCB_IC_DIRTY_RO                    (0x04000000)
#define         UDF_VCB_IC_W2K_COMPAT_VLABEL           (0x08000000)
#define         UDF_VCB_IC_CACHE_BAD_VDS               (0x10000000)
#define         UDF_VCB_IC_WAIT_CD_SPINUP              (0x20000000)
#define         UDF_VCB_IC_SHOW_BLANK_CD               (0x40000000)
#define         UDF_VCB_IC_ADAPTEC_NONALLOC_COMPAT     (0x80000000)

// Some defines
#define UDFIsDvdMedia(Vcb)       (Vcb->DVD_Mode)
#define UDFIsWriteParamsReq(Vcb) (Vcb->WriteParamsReq && !Vcb->DVD_Mode)

/**************************************************************************
    we will store all of our global variables in one structure.
    Global variables are not specific to any mounted volume BUT
    by definition are required for successful operation of the
    FSD implementation.
**************************************************************************/
typedef struct _UDFData {

#ifdef _UDF_STRUCTURES_H_
    UDFIdentifier               NodeIdentifier;
    // the fields in this list are protected by the following resource
    ERESOURCE                   GlobalDataResource;
    // each driver has a driver object created for it by the NT I/O Mgr.
    //  we are no exception to this rule.
    PDRIVER_OBJECT              DriverObject;
    // we will create a device object for our FSD as well ...
    //  Although not really required, it helps if a helper application
    //  writen by us wishes to send us control information via
    //  IOCTL requests ...
    PDEVICE_OBJECT              UDFDeviceObject;
    PDEVICE_OBJECT              UDFDeviceObject_CD;
    PDEVICE_OBJECT              UDFDeviceObject_HDD; 
    PDEVICE_OBJECT              UDFDeviceObject_TAPE;
    PDEVICE_OBJECT              UDFDeviceObject_OTHER;
    PDEVICE_OBJECT              UDFFilterDeviceObject;
    // we will keep a list of all logical volumes for our UDF FSD
    LIST_ENTRY                  VCBQueue;
    // the NT Cache Manager, the I/O Manager and we will conspire
    //  to bypass IRP usage using the function pointers contained
    //  in the following structure
    FAST_IO_DISPATCH            UDFFastIoDispatch;
    // The NT Cache Manager uses the following call backs to ensure
    //  correct locking hierarchy is maintained
    CACHE_MANAGER_CALLBACKS     CacheMgrCallBacks;
    // structures allocated from a zone need some fields here. Note
    //  that under version 4.0, it might be better to use lookaside
    //  lists
    KSPIN_LOCK                  ZoneAllocationSpinLock;
    ZONE_HEADER                 ObjectNameZoneHeader;
    ZONE_HEADER                 CCBZoneHeader;
    ZONE_HEADER                 FCBZoneHeader;
    ZONE_HEADER                 IrpContextZoneHeader;
//    ZONE_HEADER                 FileInfoZoneHeader;
    VOID                        *ObjectNameZone;
    VOID                        *CCBZone;
    VOID                        *FCBZone;
    VOID                        *IrpContextZone;
//    VOID                        *FileInfoZone;
    // currently, there is a single default zone size value used for
    //  all zones. This should ideally be changed by you to be 1 per
    //  type of zone (e.g. a default size for the FCB zone might be
    //  different from the default size for the ByteLock zone).

#ifdef EVALUATION_TIME_LIMIT
    UDF_KEY_LIST                CurrentKeyHash;
#endif //EVALUATION_TIME_LIMIT
    //  Of course, you will need to use different values (min/max)
    //  for lookaside lists (if you decide to use them instead)
    uint32                      DefaultZoneSizeInNumStructs;
    // Handle returned by the MUP is stored here.
    HANDLE                      MupHandle;
    // IsSync Resource
//    ERESOURCE                   IsSyncResource;
    // Is operation synchronous flag
//    BOOLEAN                     IsSync;
#ifdef EVALUATION_TIME_LIMIT
    ULONG                       Saved_j;
#endif //EVALUATION_TIME_LIMIT

    // delayed close support
    ERESOURCE                   DelayedCloseResource;
    ULONG                       MaxDelayedCloseCount;
    ULONG                       DelayedCloseCount;
    ULONG                       MinDelayedCloseCount;
    ULONG                       MaxDirDelayedCloseCount;
    ULONG                       DirDelayedCloseCount;
    ULONG                       MinDirDelayedCloseCount;
    LIST_ENTRY                  DelayedCloseQueue;
    LIST_ENTRY                  DirDelayedCloseQueue;
    WORK_QUEUE_ITEM             CloseItem;
    WORK_QUEUE_ITEM             LicenseKeyItem;
    BOOLEAN                     LicenseKeyItemStarted;
    BOOLEAN                     FspCloseActive;
    BOOLEAN                     ReduceDelayedClose;
    BOOLEAN                     ReduceDirDelayedClose;

#ifdef EVALUATION_TIME_LIMIT
    LARGE_INTEGER               UDFCurrentTime;
#endif //EVALUATION_TIME_LIMIT
    ULONG                       CPU_Count;
    LARGE_INTEGER               UDFLargeZero;

    // mount event (for udf gui app)
    PKEVENT                     MountEvent;

#ifdef EVALUATION_TIME_LIMIT
    WCHAR                       LicenseKeyW[16+1];
    WCHAR                       LKPadding[1];
#endif //EVALUATION_TIME_LIMIT

#endif //_UDF_STRUCTURES_H_
    //HKEY                        hUdfRootKey;
    UNICODE_STRING              SavedRegPath;
    UNICODE_STRING              UnicodeStrRoot;
    UNICODE_STRING              UnicodeStrSDir;
    UNICODE_STRING              AclName;
//    WCHAR                       UnicodeStrRootBuffer[2];
#ifdef EVALUATION_TIME_LIMIT
    ULONG                       iTime;
    ULONG                       iVer;
    ULONG                       iTrial;
#endif //EVALUATION_TIME_LIMIT

    ULONG                       WCacheMaxFrames;
    ULONG                       WCacheMaxBlocks;
    ULONG                       WCacheBlocksPerFrameSh;
    ULONG                       WCacheFramesToKeepFree;

#ifdef EVALUATION_TIME_LIMIT
    UCHAR                       Page2Padding[PAGE_SIZE];
#endif //EVALUATION_TIME_LIMIT

    // some state information is maintained in the flags field
    uint32                      UDFFlags;

    PVOID                       AutoFormatCount;

} UDFData, *PtrUDFData;

// valid flag values for the global data structure
#define     UDF_DATA_FLAGS_RESOURCE_INITIALIZED     (0x00000001)
#define     UDF_DATA_FLAGS_ZONES_INITIALIZED        (0x00000002)
#define     UDF_DATA_FLAGS_BEING_UNLOADED           (0x00000004)
#define     UDF_DATA_FLAGS_UNREGISTERED             (0x00000008)

/**/

extern VOID UDFSetModified(
    IN PVCB        Vcb
    );

extern VOID UDFPreClrModified(
    IN PVCB        Vcb
    );

extern VOID UDFClrModified(
    IN PVCB        Vcb
    );


#define FILE_ID_CACHE_GRANULARITY 16
#define DLOC_LIST_GRANULARITY 16

typedef LONGLONG               FILE_ID;
typedef FILE_ID                *PFILE_ID;

#endif //__UDF_COMMON_STRUCT__H__
