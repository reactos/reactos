#ifndef _VFATFS_PCH_
#define _VFATFS_PCH_

#include <ntifs.h>
#include <ntdddisk.h>
#include <dos.h>
#include <pseh/pseh2.h>
#include <section_attribs.h>
#ifdef KDBG
#include <ndk/kdfuncs.h>
#include <reactos/kdros.h>
#endif


#define USE_ROS_CC_AND_FS
#define ENABLE_SWAPOUT

/* FIXME: because volume is not cached, we have to perform direct IOs
 * The day this is fixed, just comment out that line, and check
 * it still works (and delete old code ;-))
 */
#define VOLUME_IS_NOT_CACHED_WORK_AROUND_IT


#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#define ROUND_DOWN_64(n, align) \
    (((ULONGLONG)n) & ~((align) - 1LL))

#define ROUND_UP_64(n, align) \
    ROUND_DOWN_64(((ULONGLONG)n) + (align) - 1LL, (align))

#include <pshpack1.h>
struct _BootSector
{
    unsigned char  magic0, res0, magic1;
    unsigned char  OEMName[8];
    unsigned short BytesPerSector;
    unsigned char  SectorsPerCluster;
    unsigned short ReservedSectors;
    unsigned char  FATCount;
    unsigned short RootEntries, Sectors;
    unsigned char  Media;
    unsigned short FATSectors, SectorsPerTrack, Heads;
    unsigned long  HiddenSectors, SectorsHuge;
    unsigned char  Drive, Res1, Sig;
    unsigned long  VolumeID;
    unsigned char  VolumeLabel[11], SysType[8];
    unsigned char  Res2[448];
    unsigned short Signatur1;
};

struct _BootSector32
{
    unsigned char  magic0, res0, magic1;			// 0
    unsigned char  OEMName[8];				// 3
    unsigned short BytesPerSector;			// 11
    unsigned char  SectorsPerCluster;			// 13
    unsigned short ReservedSectors;			// 14
    unsigned char  FATCount;				// 16
    unsigned short RootEntries, Sectors;			// 17
    unsigned char  Media;					// 21
    unsigned short FATSectors, SectorsPerTrack, Heads;	// 22
    unsigned long  HiddenSectors, SectorsHuge;		// 28
    unsigned long  FATSectors32;				// 36
    unsigned short ExtFlag;				// 40
    unsigned short FSVersion;				// 42
    unsigned long  RootCluster;				// 44
    unsigned short FSInfoSector;				// 48
    unsigned short BootBackup;				// 50
    unsigned char  Res3[12];				// 52
    unsigned char  Drive;					// 64
    unsigned char  Res4;					// 65
    unsigned char  ExtBootSignature;			// 66
    unsigned long  VolumeID;				// 67
    unsigned char  VolumeLabel[11], SysType[8];		// 71
    unsigned char  Res2[420];				// 90
    unsigned short Signature1;				// 510
};

#define FAT_DIRTY_BIT 0x01

struct _BootSectorFatX
{
    unsigned char SysType[4];        // 0
    unsigned long VolumeID;          // 4
    unsigned long SectorsPerCluster; // 8
    unsigned short FATCount;         // 12
    unsigned long Unknown;           // 14
    unsigned char Unused[4078];      // 18
};

struct _FsInfoSector
{
    unsigned long  ExtBootSignature2;			// 0
    unsigned char  Res6[480];				// 4
    unsigned long  FSINFOSignature;			// 484
    unsigned long  FreeCluster;				// 488
    unsigned long  NextCluster;				// 492
    unsigned char  Res7[12];				// 496
    unsigned long  Signatur2;				// 508
};

typedef struct _BootSector BootSector;

struct _FATDirEntry
{
    union
    {
        struct { unsigned char Filename[8], Ext[3]; };
        unsigned char ShortName[11];
    };
    unsigned char  Attrib;
    unsigned char  lCase;
    unsigned char  CreationTimeMs;
    unsigned short CreationTime,CreationDate,AccessDate;
    union
    {
        unsigned short FirstClusterHigh; // FAT32
        unsigned short ExtendedAttributes; // FAT12/FAT16
    };
    unsigned short UpdateTime;                            //time create/update
    unsigned short UpdateDate;                            //date create/update
    unsigned short FirstCluster;
    unsigned long  FileSize;
};

#define FAT_EAFILE  "EA DATA. SF"

typedef struct _EAFileHeader FAT_EA_FILE_HEADER, *PFAT_EA_FILE_HEADER;

struct _EAFileHeader
{
    unsigned short Signature; // ED
    unsigned short Unknown[15];
    unsigned short EASetTable[240];
};

typedef struct _EASetHeader FAT_EA_SET_HEADER, *PFAT_EA_SET_HEADER;

struct _EASetHeader
{
    unsigned short Signature; // EA
    unsigned short Offset; // relative offset, same value as in the EASetTable
    unsigned short Unknown1[2];
    char TargetFileName[12];
    unsigned short Unknown2[3];
    unsigned int EALength;
    // EA Header
};

typedef struct _EAHeader FAT_EA_HEADER, *PFAT_EA_HEADER;

struct _EAHeader
{
    unsigned char Unknown;
    unsigned char EANameLength;
    unsigned short EAValueLength;
    // Name Data
    // Value Data
};

typedef struct _FATDirEntry FAT_DIR_ENTRY, *PFAT_DIR_ENTRY;

struct _FATXDirEntry
{
    unsigned char FilenameLength; // 0
    unsigned char Attrib;         // 1
    unsigned char Filename[42];   // 2
    unsigned long FirstCluster;   // 44
    unsigned long FileSize;       // 48
    unsigned short UpdateTime;    // 52
    unsigned short UpdateDate;    // 54
    unsigned short CreationTime;  // 56
    unsigned short CreationDate;  // 58
    unsigned short AccessTime;    // 60
    unsigned short AccessDate;    // 62
};

struct _slot
{
    unsigned char id;               // sequence number for slot
    WCHAR  name0_4[5];              // first 5 characters in name
    unsigned char attr;             // attribute byte
    unsigned char reserved;         // always 0
    unsigned char alias_checksum;   // checksum for 8.3 alias
    WCHAR  name5_10[6];             // 6 more characters in name
    unsigned char start[2];         // starting cluster number
    WCHAR  name11_12[2];            // last 2 characters in name
};

typedef struct _slot slot;

#include <poppack.h>

#define VFAT_CASE_LOWER_BASE	8  		// base is lower case
#define VFAT_CASE_LOWER_EXT 	16 		// extension is lower case

#define LONGNAME_MAX_LENGTH 	256		// max length for a long filename

#define ENTRY_DELETED(IsFatX, DirEntry) (IsFatX ? FATX_ENTRY_DELETED(&((DirEntry)->FatX)) : FAT_ENTRY_DELETED(&((DirEntry)->Fat)))
#define ENTRY_VOLUME(IsFatX, DirEntry) (IsFatX ? FATX_ENTRY_VOLUME(&((DirEntry)->FatX)) : FAT_ENTRY_VOLUME(&((DirEntry)->Fat)))
#define ENTRY_END(IsFatX, DirEntry) (IsFatX ? FATX_ENTRY_END(&((DirEntry)->FatX)) : FAT_ENTRY_END(&((DirEntry)->Fat)))

#define FAT_ENTRY_DELETED(DirEntry)  ((DirEntry)->Filename[0] == 0xe5)
#define FAT_ENTRY_END(DirEntry)      ((DirEntry)->Filename[0] == 0)
#define FAT_ENTRY_LONG(DirEntry)     (((DirEntry)->Attrib & 0x3f) == 0x0f)
#define FAT_ENTRY_VOLUME(DirEntry)   (((DirEntry)->Attrib & 0x1f) == 0x08)

#define FATX_ENTRY_DELETED(DirEntry) ((DirEntry)->FilenameLength == 0xe5)
#define FATX_ENTRY_END(DirEntry)     ((DirEntry)->FilenameLength == 0xff)
#define FATX_ENTRY_LONG(DirEntry)    (FALSE)
#define FATX_ENTRY_VOLUME(DirEntry)  (((DirEntry)->Attrib & 0x1f) == 0x08)

#define FAT_ENTRIES_PER_PAGE   (PAGE_SIZE / sizeof (FAT_DIR_ENTRY))
#define FATX_ENTRIES_PER_PAGE  (PAGE_SIZE / sizeof (FATX_DIR_ENTRY))

typedef struct _FATXDirEntry FATX_DIR_ENTRY, *PFATX_DIR_ENTRY;

union _DIR_ENTRY
{
    FAT_DIR_ENTRY Fat;
    FATX_DIR_ENTRY FatX;
};

typedef union _DIR_ENTRY DIR_ENTRY, *PDIR_ENTRY;

#define BLOCKSIZE 512

#define FAT16  (1)
#define FAT12  (2)
#define FAT32  (3)
#define FATX16 (4)
#define FATX32 (5)

#define VCB_VOLUME_LOCKED       0x0001
#define VCB_DISMOUNT_PENDING    0x0002
#define VCB_IS_FATX             0x0004
#define VCB_IS_SYS_OR_HAS_PAGE  0x0008
#define VCB_IS_DIRTY            0x4000 /* Volume is dirty */
#define VCB_CLEAR_DIRTY         0x8000 /* Clean dirty flag at shutdown */
/* VCB condition state */
#define VCB_GOOD                0x0010 /* If not set, the VCB is improper for usage */

typedef struct
{
    ULONG VolumeID;
    CHAR VolumeLabel[11];
    ULONG FATStart;
    ULONG FATCount;
    ULONG FATSectors;
    ULONG rootDirectorySectors;
    ULONG rootStart;
    ULONG dataStart;
    ULONG RootCluster;
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG NumberOfClusters;
    ULONG FatType;
    ULONG Sectors;
    BOOLEAN FixedMedia;
    ULONG FSInfoSector;
} FATINFO, *PFATINFO;

struct _VFATFCB;
struct _VFAT_DIRENTRY_CONTEXT;
struct _VFAT_MOVE_CONTEXT;
struct _VFAT_CLOSE_CONTEXT;

typedef struct _HASHENTRY
{
    ULONG Hash;
    struct _VFATFCB* self;
    struct _HASHENTRY* next;
}
HASHENTRY;

typedef struct DEVICE_EXTENSION *PDEVICE_EXTENSION;

typedef NTSTATUS (*PGET_NEXT_CLUSTER)(PDEVICE_EXTENSION,ULONG,PULONG);
typedef NTSTATUS (*PFIND_AND_MARK_AVAILABLE_CLUSTER)(PDEVICE_EXTENSION,PULONG);
typedef NTSTATUS (*PWRITE_CLUSTER)(PDEVICE_EXTENSION,ULONG,ULONG,PULONG);

typedef BOOLEAN (*PIS_DIRECTORY_EMPTY)(PDEVICE_EXTENSION,struct _VFATFCB*);
typedef NTSTATUS (*PADD_ENTRY)(PDEVICE_EXTENSION,PUNICODE_STRING,struct _VFATFCB**,struct _VFATFCB*,ULONG,UCHAR,struct _VFAT_MOVE_CONTEXT*);
typedef NTSTATUS (*PDEL_ENTRY)(PDEVICE_EXTENSION,struct _VFATFCB*,struct _VFAT_MOVE_CONTEXT*);
typedef NTSTATUS (*PGET_NEXT_DIR_ENTRY)(PVOID*,PVOID*,struct _VFATFCB*,struct _VFAT_DIRENTRY_CONTEXT*,BOOLEAN);
typedef NTSTATUS (*PGET_DIRTY_STATUS)(PDEVICE_EXTENSION,PBOOLEAN);
typedef NTSTATUS (*PSET_DIRTY_STATUS)(PDEVICE_EXTENSION,BOOLEAN);

typedef struct _VFAT_DISPATCH
{
    PIS_DIRECTORY_EMPTY IsDirectoryEmpty;
    PADD_ENTRY AddEntry;
    PDEL_ENTRY DelEntry;
    PGET_NEXT_DIR_ENTRY GetNextDirEntry;
} VFAT_DISPATCH, *PVFAT_DISPATCH;

#define STATISTICS_SIZE_NO_PAD (sizeof(FILESYSTEM_STATISTICS) + sizeof(FAT_STATISTICS))
typedef struct _STATISTICS {
    FILESYSTEM_STATISTICS Base;
    FAT_STATISTICS Fat;
    UCHAR Pad[((STATISTICS_SIZE_NO_PAD + 0x3f) & ~0x3f) - STATISTICS_SIZE_NO_PAD];
} STATISTICS, *PSTATISTICS;

typedef struct DEVICE_EXTENSION
{
    ERESOURCE DirResource;
    ERESOURCE FatResource;

    KSPIN_LOCK FcbListLock;
    LIST_ENTRY FcbListHead;
    ULONG HashTableSize;
    struct _HASHENTRY **FcbHashTable;

    PDEVICE_OBJECT VolumeDevice;
    PDEVICE_OBJECT StorageDevice;
    PFILE_OBJECT FATFileObject;
    FATINFO FatInfo;
    ULONG LastAvailableCluster;
    ULONG AvailableClusters;
    BOOLEAN AvailableClustersValid;
    ULONG Flags;
    struct _VFATFCB *VolumeFcb;
    struct _VFATFCB *RootFcb;
    PSTATISTICS Statistics;

    /* Overflow request queue */
    KSPIN_LOCK OverflowQueueSpinLock;
    LIST_ENTRY OverflowQueue;
    ULONG OverflowQueueCount;
    ULONG PostedRequestCount;

    /* Pointers to functions for manipulating FAT. */
    PGET_NEXT_CLUSTER GetNextCluster;
    PFIND_AND_MARK_AVAILABLE_CLUSTER FindAndMarkAvailableCluster;
    PWRITE_CLUSTER WriteCluster;
    PGET_DIRTY_STATUS GetDirtyStatus;
    PSET_DIRTY_STATUS SetDirtyStatus;

    ULONG BaseDateYear;

    LIST_ENTRY VolumeListEntry;

    /* Notifications */
    LIST_ENTRY NotifyList;
    PNOTIFY_SYNC NotifySync;

    /* Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE */
    ULONG OpenHandleCount;

    /* VPBs for dismount */
    PVPB IoVPB;
    PVPB SpareVPB;

    /* Pointers to functions for manipulating directory entries. */
    VFAT_DISPATCH Dispatch;
} DEVICE_EXTENSION, VCB, *PVCB;

FORCEINLINE
BOOLEAN
VfatIsDirectoryEmpty(PDEVICE_EXTENSION DeviceExt,
                     struct _VFATFCB* Fcb)
{
    return DeviceExt->Dispatch.IsDirectoryEmpty(DeviceExt, Fcb);
}

FORCEINLINE
NTSTATUS
VfatAddEntry(PDEVICE_EXTENSION DeviceExt,
             PUNICODE_STRING NameU,
             struct _VFATFCB** Fcb,
             struct _VFATFCB* ParentFcb,
             ULONG RequestedOptions,
             UCHAR ReqAttr,
             struct _VFAT_MOVE_CONTEXT* MoveContext)
{
    return DeviceExt->Dispatch.AddEntry(DeviceExt, NameU, Fcb, ParentFcb, RequestedOptions, ReqAttr, MoveContext);
}

FORCEINLINE
NTSTATUS
VfatDelEntry(PDEVICE_EXTENSION DeviceExt,
             struct _VFATFCB* Fcb,
             struct _VFAT_MOVE_CONTEXT* MoveContext)
{
    return DeviceExt->Dispatch.DelEntry(DeviceExt, Fcb, MoveContext);
}

FORCEINLINE
NTSTATUS
VfatGetNextDirEntry(PDEVICE_EXTENSION DeviceExt,
                    PVOID *pContext,
                    PVOID *pPage,
                    struct _VFATFCB* pDirFcb,
                    struct _VFAT_DIRENTRY_CONTEXT* DirContext,
                    BOOLEAN First)
{
    return DeviceExt->Dispatch.GetNextDirEntry(pContext, pPage, pDirFcb, DirContext, First);
}

#define VFAT_BREAK_ON_CORRUPTION 1

typedef struct
{
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;
    ULONG NumberProcessors;
    ERESOURCE VolumeListLock;
    LIST_ENTRY VolumeListHead;
    NPAGED_LOOKASIDE_LIST FcbLookasideList;
    NPAGED_LOOKASIDE_LIST CcbLookasideList;
    NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
    PAGED_LOOKASIDE_LIST CloseContextLookasideList;
    FAST_IO_DISPATCH FastIoDispatch;
    CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
    FAST_MUTEX CloseMutex;
    ULONG CloseCount;
    LIST_ENTRY CloseListHead;
    BOOLEAN CloseWorkerRunning;
    PIO_WORKITEM CloseWorkItem;
    BOOLEAN ShutdownStarted;
} VFAT_GLOBAL_DATA, *PVFAT_GLOBAL_DATA;

extern PVFAT_GLOBAL_DATA VfatGlobalData;

#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_DELETE_PENDING      0x0002
#define FCB_IS_FAT              0x0004
#define FCB_IS_PAGE_FILE        0x0008
#define FCB_IS_VOLUME           0x0010
#define FCB_IS_DIRTY            0x0020
#define FCB_DELAYED_CLOSE       0x0040
#ifdef KDBG
#define FCB_CLEANED_UP          0x0080
#define FCB_CLOSED              0x0100
#endif

#define NODE_TYPE_FCB ((CSHORT)0x0502)

typedef struct _VFATFCB
{
    /* FCB header required by ROS/NT */
    FSRTL_COMMON_FCB_HEADER RFCB;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ERESOURCE MainResource;
    ERESOURCE PagingIoResource;
    /* end FCB header required by ROS/NT */

    /* directory entry for this file or directory */
    DIR_ENTRY entry;

    /* Pointer to attributes in entry */
    PUCHAR Attributes;

    /* long file name, points into PathNameBuffer */
    UNICODE_STRING LongNameU;

    /* short file name */
    UNICODE_STRING ShortNameU;

    /* directory name, points into PathNameBuffer */
    UNICODE_STRING DirNameU;

    /* path + long file name 260 max*/
    UNICODE_STRING PathNameU;

    /* buffer for PathNameU */
    PWCHAR PathNameBuffer;

    /* buffer for ShortNameU */
    WCHAR ShortNameBuffer[13];

    /* */
    LONG RefCount;

    /* List of FCB's for this volume */
    LIST_ENTRY FcbListEntry;

    /* List of FCB's for the parent */
    LIST_ENTRY ParentListEntry;

    /* pointer to the parent fcb */
    struct _VFATFCB *parentFcb;

    /* List for the children */
    LIST_ENTRY ParentListHead;

    /* Flags for the fcb */
    ULONG Flags;

    /* pointer to the file object which has initialized the fcb */
    PFILE_OBJECT FileObject;

    /* Directory index for the short name entry */
    ULONG dirIndex;

    /* Directory index where the long name starts */
    ULONG startIndex;

    /* Share access for the file object */
    SHARE_ACCESS FCBShareAccess;

    /* Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP */
    ULONG OpenHandleCount;

    /* Entry into the hash table for the path + long name */
    HASHENTRY Hash;

    /* Entry into the hash table for the path + short name */
    HASHENTRY ShortHash;

    /* List of byte-range locks for this file */
    FILE_LOCK FileLock;

    /*
     * Optimization: caching of last read/write cluster+offset pair. Can't
     * be in VFATCCB because it must be reset everytime the allocated clusters
     * change.
     */
    FAST_MUTEX LastMutex;
    ULONG LastCluster;
    ULONG LastOffset;

    struct _VFAT_CLOSE_CONTEXT * CloseContext;
} VFATFCB, *PVFATFCB;

#define CCB_DELETE_ON_CLOSE     0x0001

typedef struct _VFATCCB
{
    LARGE_INTEGER  CurrentByteOffset;
    ULONG Flags;
    /* for DirectoryControl */
    ULONG Entry;
    /* for DirectoryControl */
    UNICODE_STRING SearchPattern;
} VFATCCB, *PVFATCCB;

#define TAG_CCB  'CtaF'
#define TAG_FCB  'FtaF'
#define TAG_IRP  'ItaF'
#define TAG_CLOSE 'xtaF'
#define TAG_STATS 'VtaF'
#define TAG_BUFFER 'OtaF'
#define TAG_VPB 'vtaF'
#define TAG_NAME 'ntaF'
#define TAG_SEARCH 'LtaF'
#define TAG_DIRENT 'DtaF'

#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))

typedef struct __DOSTIME
{
    USHORT Second:5;
    USHORT Minute:6;
    USHORT Hour:5;
}
DOSTIME, *PDOSTIME;

typedef struct __DOSDATE
{
    USHORT Day:5;
    USHORT Month:4;
    USHORT Year:7;
}
DOSDATE, *PDOSDATE;

#define IRPCONTEXT_CANWAIT          0x0001
#define IRPCONTEXT_COMPLETE         0x0002
#define IRPCONTEXT_QUEUE            0x0004
#define IRPCONTEXT_PENDINGRETURNED  0x0008
#define IRPCONTEXT_DEFERRED_WRITE   0x0010

typedef struct
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExt;
    ULONG Flags;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIO_STACK_LOCATION Stack;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    PFILE_OBJECT FileObject;
    ULONG RefCount;
    KEVENT Event;
    CCHAR PriorityBoost;
} VFAT_IRP_CONTEXT, *PVFAT_IRP_CONTEXT;

typedef struct _VFAT_DIRENTRY_CONTEXT
{
    ULONG StartIndex;
    ULONG DirIndex;
    DIR_ENTRY DirEntry;
    UNICODE_STRING LongNameU;
    UNICODE_STRING ShortNameU;
    PDEVICE_EXTENSION DeviceExt;
} VFAT_DIRENTRY_CONTEXT, *PVFAT_DIRENTRY_CONTEXT;

typedef struct _VFAT_MOVE_CONTEXT
{
    ULONG FirstCluster;
    ULONG FileSize;
    USHORT CreationDate;
    USHORT CreationTime;
    BOOLEAN InPlace;
} VFAT_MOVE_CONTEXT, *PVFAT_MOVE_CONTEXT;

typedef struct _VFAT_CLOSE_CONTEXT
{
    PDEVICE_EXTENSION Vcb;
    PVFATFCB Fcb;
    LIST_ENTRY CloseListEntry;
} VFAT_CLOSE_CONTEXT, *PVFAT_CLOSE_CONTEXT;

FORCEINLINE
NTSTATUS
VfatMarkIrpContextForQueue(PVFAT_IRP_CONTEXT IrpContext)
{
    PULONG Flags = &IrpContext->Flags;

    *Flags &= ~IRPCONTEXT_COMPLETE;
    *Flags |= IRPCONTEXT_QUEUE;

    return STATUS_PENDING;
}

FORCEINLINE
BOOLEAN
vfatFCBIsDirectory(PVFATFCB FCB)
{
    return BooleanFlagOn(*FCB->Attributes, FILE_ATTRIBUTE_DIRECTORY);
}

FORCEINLINE
BOOLEAN
vfatFCBIsReadOnly(PVFATFCB FCB)
{
    return BooleanFlagOn(*FCB->Attributes, FILE_ATTRIBUTE_READONLY);
}

FORCEINLINE
BOOLEAN
vfatVolumeIsFatX(PDEVICE_EXTENSION DeviceExt)
{
    return BooleanFlagOn(DeviceExt->Flags, VCB_IS_FATX);
}

FORCEINLINE
VOID
vfatReportChange(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB Fcb,
    IN ULONG FilterMatch,
    IN ULONG Action)
{
    FsRtlNotifyFullReportChange(DeviceExt->NotifySync,
                                &(DeviceExt->NotifyList),
                                (PSTRING)&Fcb->PathNameU,
                                Fcb->PathNameU.Length - Fcb->LongNameU.Length,
                                NULL, NULL, FilterMatch, Action, NULL);
}

#define vfatAddToStat(Vcb, Stat, Inc)                                                                         \
{                                                                                                             \
    PSTATISTICS Stats = &(Vcb)->Statistics[KeGetCurrentProcessorNumber() % VfatGlobalData->NumberProcessors]; \
    Stats->Stat += Inc;                                                                                       \
}

/* blockdev.c */

NTSTATUS
VfatReadDisk(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PLARGE_INTEGER ReadOffset,
    IN ULONG ReadLength,
    IN PUCHAR Buffer,
    IN BOOLEAN Override);

NTSTATUS
VfatReadDiskPartial(
    IN PVFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER ReadOffset,
    IN ULONG ReadLength,
    IN ULONG BufferOffset,
    IN BOOLEAN Wait);

NTSTATUS
VfatWriteDisk(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PLARGE_INTEGER WriteOffset,
    IN ULONG WriteLength,
    IN OUT PUCHAR Buffer,
    IN BOOLEAN Override);

NTSTATUS
VfatWriteDiskPartial(
    IN PVFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER WriteOffset,
    IN ULONG WriteLength,
    IN ULONG BufferOffset,
    IN BOOLEAN Wait);

NTSTATUS
VfatBlockDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CtlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    IN OUT PVOID OutputBuffer,
    IN OUT PULONG pOutputBufferSize,
    IN BOOLEAN Override);

/* cleanup.c */

NTSTATUS
VfatCleanup(
    PVFAT_IRP_CONTEXT IrpContext);

/* close.c */

NTSTATUS
VfatClose(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatCloseFile(
    PDEVICE_EXTENSION DeviceExt,
    PFILE_OBJECT FileObject);

/* create.c */

NTSTATUS
VfatCreate(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
FindFile(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB Parent,
    PUNICODE_STRING FileToFindU,
    PVFAT_DIRENTRY_CONTEXT DirContext,
    BOOLEAN First);

VOID
vfat8Dot3ToString(
    PFAT_DIR_ENTRY pEntry,
    PUNICODE_STRING NameU);

/* dir.c */

NTSTATUS
VfatDirectoryControl(
    PVFAT_IRP_CONTEXT IrpContext);

BOOLEAN
FsdDosDateTimeToSystemTime(
    PDEVICE_EXTENSION DeviceExt,
    USHORT DosDate,
    USHORT DosTime,
    PLARGE_INTEGER SystemTime);

BOOLEAN
FsdSystemTimeToDosDateTime(
    PDEVICE_EXTENSION DeviceExt,
    PLARGE_INTEGER SystemTime,
    USHORT *pDosDate,
    USHORT *pDosTime);

/* direntry.c */

ULONG
vfatDirEntryGetFirstCluster(
    PDEVICE_EXTENSION pDeviceExt,
    PDIR_ENTRY pDirEntry);

/* dirwr.c */

NTSTATUS
vfatFCBInitializeCacheFromVolume(
    PVCB vcb,
    PVFATFCB fcb);

NTSTATUS
VfatUpdateEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    PVFATFCB pFcb);

BOOLEAN
vfatFindDirSpace(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB pDirFcb,
    ULONG nbSlots,
    PULONG start);

NTSTATUS
vfatRenameEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pFcb,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN CaseChangeOnly);

NTSTATUS
VfatMoveEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pFcb,
    IN PUNICODE_STRING FileName,
    IN PVFATFCB ParentFcb);

/* ea.h */

NTSTATUS
VfatSetExtendedAttributes(
    PFILE_OBJECT FileObject,
    PVOID Ea,
    ULONG EaLength);

/* fastio.c */

CODE_SEG("INIT")
VOID
VfatInitFastIoRoutines(
    PFAST_IO_DISPATCH FastIoDispatch);

BOOLEAN
NTAPI
VfatAcquireForLazyWrite(
    IN PVOID Context,
    IN BOOLEAN Wait);

VOID
NTAPI
VfatReleaseFromLazyWrite(
    IN PVOID Context);

/* fat.c */

NTSTATUS
FAT12GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster);

NTSTATUS
FAT12FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster);

NTSTATUS
FAT12WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue);

NTSTATUS
FAT16GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster);

NTSTATUS
FAT16FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster);

NTSTATUS
FAT16WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue);

NTSTATUS
FAT32GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster);

NTSTATUS
FAT32FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster);

NTSTATUS
FAT32WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue);

NTSTATUS
OffsetToCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG FirstCluster,
    ULONG FileOffset,
    PULONG Cluster,
    BOOLEAN Extend);

ULONGLONG
ClusterToSector(
    PDEVICE_EXTENSION DeviceExt,
    ULONG Cluster);

NTSTATUS
GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster);

NTSTATUS
GetNextClusterExtend(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster);

NTSTATUS
CountAvailableClusters(
    PDEVICE_EXTENSION DeviceExt,
    PLARGE_INTEGER Clusters);

NTSTATUS
WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue);

NTSTATUS
GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus);

NTSTATUS
FAT16GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus);

NTSTATUS
FAT32GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus);

NTSTATUS
SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus);

NTSTATUS
FAT16SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus);

NTSTATUS
FAT32SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus);

NTSTATUS
FAT32UpdateFreeClustersCount(
    PDEVICE_EXTENSION DeviceExt);

/* fcb.c */

PVFATFCB
vfatNewFCB(
    PDEVICE_EXTENSION pVCB,
    PUNICODE_STRING pFileNameU);

NTSTATUS
vfatSetFCBNewDirName(
    PDEVICE_EXTENSION pVCB,
    PVFATFCB Fcb,
    PVFATFCB ParentFcb);

NTSTATUS
vfatUpdateFCB(
    PDEVICE_EXTENSION pVCB,
    PVFATFCB Fcb,
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PVFATFCB ParentFcb);

VOID
vfatDestroyFCB(
    PVFATFCB pFCB);

VOID
vfatDestroyCCB(
    PVFATCCB pCcb);

VOID
#ifndef KDBG
vfatGrabFCB(
#else
_vfatGrabFCB(
#endif
    PDEVICE_EXTENSION pVCB,
    PVFATFCB pFCB
#ifdef KDBG
    ,
    PCSTR File,
    ULONG Line,
    PCSTR Func
#endif
    );

VOID
#ifndef KDBG
vfatReleaseFCB(
#else
_vfatReleaseFCB(
#endif
    PDEVICE_EXTENSION pVCB,
    PVFATFCB pFCB
#ifdef KDBG
    ,
    PCSTR File,
    ULONG Line,
    PCSTR Func
#endif
    );

#ifdef KDBG
#define vfatGrabFCB(v, f) _vfatGrabFCB(v, f, __FILE__, __LINE__, __FUNCTION__)
#define vfatReleaseFCB(v, f) _vfatReleaseFCB(v, f, __FILE__, __LINE__, __FUNCTION__)
#endif

PVFATFCB
vfatGrabFCBFromTable(
    PDEVICE_EXTENSION pDeviceExt,
    PUNICODE_STRING pFileNameU);

PVFATFCB
vfatMakeRootFCB(
    PDEVICE_EXTENSION pVCB);

PVFATFCB
vfatOpenRootFCB(
    PDEVICE_EXTENSION pVCB);

BOOLEAN
vfatFCBIsDirectory(
    PVFATFCB FCB);

BOOLEAN
vfatFCBIsRoot(
    PVFATFCB FCB);

NTSTATUS
vfatAttachFCBToFileObject(
    PDEVICE_EXTENSION vcb,
    PVFATFCB fcb,
    PFILE_OBJECT fileObject);

NTSTATUS
vfatDirFindFile(
    PDEVICE_EXTENSION pVCB,
    PVFATFCB parentFCB,
    PUNICODE_STRING FileToFindU,
    PVFATFCB *fileFCB);

NTSTATUS
vfatGetFCBForFile(
    PDEVICE_EXTENSION pVCB,
    PVFATFCB *pParentFCB,
    PVFATFCB *pFCB,
    PUNICODE_STRING pFileNameU);

NTSTATUS
vfatMakeFCBFromDirEntry(
    PVCB vcb,
    PVFATFCB directoryFCB,
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PVFATFCB *fileFCB);

/* finfo.c */

NTSTATUS
VfatGetStandardInformation(
    PVFATFCB FCB,
    PFILE_STANDARD_INFORMATION StandardInfo,
    PULONG BufferLength);

NTSTATUS
VfatGetBasicInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_BASIC_INFORMATION BasicInfo,
    PULONG BufferLength);

NTSTATUS
VfatQueryInformation(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatSetInformation(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatSetAllocationSizeInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PLARGE_INTEGER AllocationSize);

/* flush.c */

NTSTATUS
VfatFlush(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatFlushVolume(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB VolumeFcb);

/* fsctl.c */

NTSTATUS
VfatFileSystemControl(
    PVFAT_IRP_CONTEXT IrpContext);

/* iface.c */

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath);

#ifdef KDBG
/* kdbg.c */
KDBG_CLI_ROUTINE vfatKdbgHandler;
#endif

/* misc.c */

DRIVER_DISPATCH
VfatBuildRequest;

NTSTATUS
NTAPI
VfatBuildRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

PVOID
VfatGetUserBuffer(
    IN PIRP Irp,
    IN BOOLEAN Paging);

NTSTATUS
VfatLockUserBuffer(
    IN PIRP Irp,
    IN ULONG Length,
    IN LOCK_OPERATION Operation);

BOOLEAN
VfatCheckForDismount(
    IN PDEVICE_EXTENSION DeviceExt,
    IN BOOLEAN Create);

VOID
vfatReportChange(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB Fcb,
    IN ULONG FilterMatch,
    IN ULONG Action);

VOID
NTAPI
VfatHandleDeferredWrite(
    IN PVOID IrpContext,
    IN PVOID Unused);

/* pnp.c */

NTSTATUS
VfatPnp(
    PVFAT_IRP_CONTEXT IrpContext);

/* rw.c */

NTSTATUS
VfatRead(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatWrite(
    PVFAT_IRP_CONTEXT *pIrpContext);

NTSTATUS
NextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG FirstCluster,
    PULONG CurrentCluster,
    BOOLEAN Extend);

/* shutdown.c */

DRIVER_DISPATCH
VfatShutdown;

NTSTATUS
NTAPI
VfatShutdown(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* string.c */

VOID
vfatSplitPathName(
    PUNICODE_STRING PathNameU,
    PUNICODE_STRING DirNameU,
    PUNICODE_STRING FileNameU);

BOOLEAN
vfatIsLongIllegal(
    WCHAR c);

BOOLEAN
IsDotOrDotDot(
    PCUNICODE_STRING Name);

/* volume.c */

NTSTATUS
VfatQueryVolumeInformation(
    PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatSetVolumeInformation(
    PVFAT_IRP_CONTEXT IrpContext);

#endif /* _VFATFS_PCH_ */
