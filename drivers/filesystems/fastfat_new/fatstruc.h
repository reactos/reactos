#pragma once

typedef struct _FAT_SCAN_CONTEXT *PFAT_SCAN_CONTEXT;
typedef struct _FAT_IO_CONTEXT *PFAT_IO_CONTEXT;
typedef struct _FAT_IRP_CONTEXT *PFAT_IRP_CONTEXT;
typedef PVOID PBCB;

typedef NTSTATUS (*PFAT_OPERATION_HANDLER) (PFAT_IRP_CONTEXT);

/* Node type stuff */
typedef CSHORT FAT_NODE_TYPE;
typedef FAT_NODE_TYPE *PFAT_NODE_TYPE;

#define FatNodeType(Ptr) (*((PFAT_NODE_TYPE)(Ptr)))

/* Node type codes */
#define FAT_NTC_VCB      (CSHORT) '00VF'
#define FAT_NTC_FCB      (CSHORT)   'CF'
#define FAT_NTC_DCB      (CSHORT)   'DF'
#define FAT_NTC_ROOT_DCB (CSHORT)  'RFD'
#define FAT_NTC_CCB      (CSHORT)  'BCC'

typedef struct _FAT_GLOBAL_DATA
{
    ERESOURCE Resource;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DiskDeviceObject;
    LIST_ENTRY VcbListHead;
    NPAGED_LOOKASIDE_LIST NonPagedFcbList;
    NPAGED_LOOKASIDE_LIST ResourceList;
    NPAGED_LOOKASIDE_LIST IrpContextList;
    FAST_IO_DISPATCH FastIoDispatch;
    CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
    CACHE_MANAGER_CALLBACKS CacheMgrNoopCallbacks;
    BOOLEAN Win31FileSystem;
    /* Jan 1, 1980 System Time */
    LARGE_INTEGER DefaultFileTime;

    /* FullFAT integration */
    FF_IOMAN *Ioman;
    FF_ERROR FF_Error;
} FAT_GLOBAL_DATA;

typedef struct _FAT_PAGE_CONTEXT
{
    PFILE_OBJECT FileObject;
    LARGE_INTEGER EndOfData;
    LARGE_INTEGER Offset;
    LARGE_INTEGER EndOfPage;
    SIZE_T ValidLength;
    PVOID Buffer;
    PBCB Bcb;
    BOOLEAN CanWait;
} FAT_PAGE_CONTEXT, *PFAT_PAGE_CONTEXT;

#define FatPinSetupContext(xContext, xFcb, CanWait)     \
{                                                       \
    (xContext)->FileObject = (xFcb)->StreamFileObject;  \
    (xContext)->EndOfData = (xFcb)->Header.FileSize;    \
    (xContext)->Offset.QuadPart = -1LL;                 \
    (xContext)->Bcb = NULL;                             \
    (xContext)->CanWait = CanWait;                      \
}

#define FatPinCleanupContext(xContext)                  \
    if ((xContext)->Bcb != NULL) {                      \
        CcUnpinData((xContext)->Bcb);                   \
        (xContext)->Bcb = NULL;                         \
    }                                                   \
    
#define FatPinEndOfPage(xContext, xType) \
    Add2Ptr((xContext)->Buffer, (xContext)->ValidLength, xType)

#define FatPinIsLastPage(xContext) \
    ((xContext)->ValidLength != PAGE_SIZE)

#define IRPCONTEXT_CANWAIT          0x0001
#define IRPCONTEXT_PENDINGRETURNED  0x0002
#define IRPCONTEXT_STACK_IO_CONTEXT 0x0004
#define IRPCONTEXT_WRITETHROUGH		0x0008
#define IRPCONTEXT_TOPLEVEL			0x0010

typedef struct _FAT_IRP_CONTEXT
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    PFILE_OBJECT FileObject;
    ULONG Flags;
    struct _VCB *Vcb;
    ULONG PinCount;
    FAT_PAGE_CONTEXT Page;
    struct _FAT_IO_CONTEXT *FatIoContext;
    WORK_QUEUE_ITEM WorkQueueItem;
    PFAT_OPERATION_HANDLER QueuedOperationHandler;
    PIO_STACK_LOCATION Stack;
    KEVENT Event;
} FAT_IRP_CONTEXT;

typedef struct _FAT_IO_CONTEXT
{
    PMDL ZeroMdl;
    PIRP Irp;
    LONG RunCount;
    SIZE_T Length;
    LONGLONG Offset;
    PFILE_OBJECT FileObject;

    union
    {
        struct
        {
            PERESOURCE Resource;
            PERESOURCE PagingIoResource;
            ERESOURCE_THREAD ResourceThreadId;
        } Async;
        KEVENT SyncEvent;
    } Wait;
    PIRP AssociatedIrp[0];
} FAT_IO_CONTEXT;

typedef ULONG (*PFAT_SCANFAT_FOR_CONTINOUS_RUN_ROUTINE) (PFAT_PAGE_CONTEXT, PULONG, BOOLEAN);
typedef ULONG (*PFAT_SETFAT_CONTINOUS_RUN_ROUTINE) (PFAT_PAGE_CONTEXT, ULONG, ULONG, BOOLEAN);
typedef ULONG (*PFAT_SCANFAT_FOR_VALUE_RUN_ROUTINE) (PFAT_PAGE_CONTEXT, PULONG, ULONG, BOOLEAN);
typedef ULONG (*PFAT_SETFAT_VALUE_RUN_ROUTINE) (PFAT_PAGE_CONTEXT, ULONG, ULONG, ULONG, BOOLEAN);

typedef struct _FAT_METHODS {
    PFAT_SCANFAT_FOR_CONTINOUS_RUN_ROUTINE ScanContinousRun;
    PFAT_SETFAT_CONTINOUS_RUN_ROUTINE SetContinousRun;
    PFAT_SCANFAT_FOR_VALUE_RUN_ROUTINE ScanValueRun;
    PFAT_SETFAT_VALUE_RUN_ROUTINE SetValueRun;
} FAT_METHODS, *PFAT_METHODS;

#define VCB_STATE_FLAG_LOCKED        0x01
#define VCB_STATE_FLAG_DIRTY         0x02
#define VCB_STATE_MOUNTED_DIRTY      0x04
#define VCB_STATE_CREATE_IN_PROGRESS 0x08

typedef enum _VCB_CONDITION
{
    VcbGood,
    VcbNotMounted,
    VcbBad
} VCB_CONDITION;

/* Volume Control Block */
typedef struct _VCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    FAST_MUTEX HeaderMutex;
    SECTION_OBJECT_POINTERS SectionObjectPointers;

    PFILE_OBJECT StreamFileObject;
    PDEVICE_OBJECT TargetDeviceObject;
    LIST_ENTRY VcbLinks;
    PVPB Vpb;
    ULONG State;
    VCB_CONDITION Condition;
    ERESOURCE Resource;

    /* Direct volume access */
    ULONG DirectOpenCount;
    SHARE_ACCESS ShareAccess;

    /* Notifications support */
    PNOTIFY_SYNC NotifySync;
    LIST_ENTRY NotifyList;

    /*  Volume Characteristics: */
    ULONG SerialNumber;
    BIOS_PARAMETER_BLOCK Bpb;
    ULONG BytesPerClusterLog;
    ULONG BytesPerCluster;
    ULONG SectorsPerFat;
    ULONG DataArea;
    ULONG Sectors;
    ULONG Clusters;
    ULONG IndexDepth;
    ULONG RootDirent;
    ULONG RootDirentSectors;
    LONGLONG BeyondLastClusterInFat;
    FAT_METHODS Methods;

    /*  Root Directory Control block */
    struct _FCB *RootDcb;

    /* Counters */
    ULONG MediaChangeCount;
    ULONG OpenFileCount;

    /* FullFAT integration */
    FF_IOMAN *Ioman;
} VCB, *PVCB;

#define VcbToVolumeDeviceObject(xVcb) \
    CONTAINING_RECORD((xVcb), VOLUME_DEVICE_OBJECT, Vcb))

#define VcbToDeviceObject(xVcb) \
    &(VcbToVolumeDeviceObject(xVcb)->DeviceObject)


#define SectorsToBytes(xVcb, xSectrors) \
	((xVcb)->Bpb.BytesPerSector * (xSectrors))

#define BytesToSectors(xVcb, xBytes) \
	((xBytes + (xVcb)->Bpb.BytesPerSector - 1) / (xVcb)->Bpb.BytesPerSector)

#define SectorsToClusters(xVcb, xSectors) \
	((xSectors + (xVcb)->Bpb.SectorsPerCluster - 1) / (xVcb)->Bpb.SectorsPerCluster)

#define VCB_FAT_BITMAP_SIZE				0x10000
#define VcbFatBitmapIndex(xCluster)		((xCluster)/VCB_FAT_BITMAP_SIZE)

/* Volume Device Object */
typedef struct _VOLUME_DEVICE_OBJECT
{
    DEVICE_OBJECT DeviceObject;
    union {
        FSRTL_COMMON_FCB_HEADER VolumeHeader;
        VCB Vcb; /* Must be the last entry! */
    };
} VOLUME_DEVICE_OBJECT, *PVOLUME_DEVICE_OBJECT;
//
//  Short name always exists in FAT
//
enum _FCB_NAME_TYPE {
    FcbShortName = 0x0,
    FcbLongName
} FCB_NAME_TYPE;

typedef struct _FCB_NAME_LINK {
    struct _FCB *Fcb;
    RTL_SPLAY_LINKS Links;
    union
    {
        OEM_STRING Ansi;
        UNICODE_STRING String;
    } Name;
    BOOLEAN IsDosName;
} FCB_NAME_LINK, *PFCB_NAME_LINK;

typedef enum _FCB_CONDITION
{
    FcbGood,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

#define FCB_STATE_HAS_NAMES         0x01
#define FCB_STATE_HAS_UNICODE_NAME  0x02
#define FCB_STATE_PAGEFILE          0x04
#define FCB_STATE_DELAY_CLOSE       0x08
#define FCB_STATE_TRUNCATE_ON_CLOSE 0x10

typedef struct _FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
   /*
    * Later we might want to move the next four fields
    * into a separate structureif we decide to split
    * FCB into paged and non paged parts
    * (as it is done in MS implementation
    */
    FAST_MUTEX HeaderMutex; // nonpaged!
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ERESOURCE Resource; // nonpaged!
    ERESOURCE PagingIoResource; // nonpaged!

    /* First cluster in the fat allocation chain */
    ULONG FirstClusterOfFile;
    /* A list of all FCBs of that DCB */
    LIST_ENTRY ParentDcbLinks;
    /* Reference to the Parent Dcb*/
    struct _FCB *ParentFcb;
    /* Pointer to a Vcb */
    PVCB Vcb;
    /* Fcb state */
    ULONG State;
    /* Fcb condition */
    FCB_CONDITION Condition;
    /* Share access */
    SHARE_ACCESS ShareAccess;
    /* Mcb mapping Vbo->Lbo */
    LARGE_MCB Mcb;
    ULONG FirstCluster;
    /* Links into FCB Tree */
    FCB_NAME_LINK ShortName;
    FCB_NAME_LINK LongName;
    /* Buffer for the short name */
    CHAR ShortNameBuffer[0xc];
    /* Full file name */
    UNICODE_STRING FullFileName;
    /* Long name with exact case */
    UNICODE_STRING ExactCaseLongName;
    /* Hint for the filename length */
    ULONG FileNameLength;
    /* A copy of fat attribute byte */
    UCHAR DirentFatFlags;
    /* File basic info */
    FILE_BASIC_INFORMATION BasicInfo;
    /* FullFAT file handle */
    FF_FILE *FatHandle;
    /* The file has outstanding async writes */
    ULONG OutstandingAsyncWrites;
    /* The outstanding async writes sync event */
    PKEVENT OutstandingAsyncEvent;
    /* Counters */
    ULONG OpenCount;
    union
    {
        struct
        {
            /* File and Op locks */
            FILE_LOCK Lock;
            OPLOCK Oplock;
        } Fcb;

        struct
        {
            /* A list of all FCBs/DCBs opened under this DCB */
            LIST_ENTRY ParentDcbList;
            /* Directory data stream (just handy to have it). */
            PFILE_OBJECT StreamFileObject;
            /* Bitmap to search for free dirents. */
            RTL_BITMAP FreeBitmap;
            /* Names */
            PRTL_SPLAY_LINKS SplayLinksAnsi;
            PRTL_SPLAY_LINKS SplayLinksUnicode;
        } Dcb;
    };
} FCB, *PFCB;

typedef struct _FAT_ENUM_DIRENT_CONTEXT *PFAT_ENUM_DIRENT_CONTEXT;
typedef struct _FAT_ENUM_DIR_CONTEXT *PFAT_ENUM_DIR_CONTEXT;

typedef ULONG (*PFAT_COPY_DIRENT_ROUTINE) (PFAT_ENUM_DIR_CONTEXT, PDIR_ENTRY, PVOID);

typedef struct _FAT_ENUM_DIRENT_CONTEXT
{
    FAT_PAGE_CONTEXT Page;

    /* Copy dirent to dirinfo */
    PFAT_COPY_DIRENT_ROUTINE CopyDirent;
    LONGLONG BytesPerClusterMask;

    /* Info buffer characteristics */
    PVOID Buffer;
    SIZE_T Offset;
    SIZE_T Length;

    /* Criteria */
    PUNICODE_STRING FileName;
    UCHAR CcbFlags;

    /* Lfn buffer/length offsets */
    ULONG LengthOffset;
    ULONG NameOffset;
} FAT_ENUM_DIRENT_CONTEXT;

typedef struct _FAT_FIND_DIRENT_CONTEXT
{
    FAT_PAGE_CONTEXT Page;
    UNICODE_STRING ShortName;
    WCHAR ShortNameBuffer[0x18];
    /* Criteria */
    PUNICODE_STRING FileName;
    BOOLEAN Valid8dot3Name;
} FAT_FIND_DIRENT_CONTEXT, *PFAT_FIND_DIRENT_CONTEXT;

typedef struct _CCB
{
    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    LARGE_INTEGER CurrentByteOffset;
    ULONG Entry;
    UNICODE_STRING SearchPattern;
    UCHAR Flags;
} CCB, *PCCB;

typedef enum _TYPE_OF_OPEN
{
    UnopenedFileObject,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    VirtualVolumeFile,
    DirectoryFile,
    EaFile
} TYPE_OF_OPEN;

typedef enum _FILE_TIME_INDEX
{
    FileCreationTime = 0,
    FileLastAccessTime,
    FileLastWriteTime,
    FileChangeTime
} FILE_TIME_INDEX;

#define CCB_SEARCH_RETURN_SINGLE_ENTRY      0x01
#define CCB_SEARCH_PATTERN_LEGAL_8DOT3      0x02
#define CCB_SEARCH_PATTERN_HAS_WILD_CARD    0x04
#define CCB_DASD_IO                         0x10
extern FAT_GLOBAL_DATA FatGlobalData;
