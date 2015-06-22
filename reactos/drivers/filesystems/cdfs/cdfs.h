#ifndef CDFS_H
#define CDFS_H

#include <ntifs.h>
#include <ntddcdrm.h>
#include <pseh/pseh2.h>

#define CDFS_BASIC_SECTOR 2048
#define CDFS_PRIMARY_DESCRIPTOR_LOCATION 16
#define BLOCKSIZE CDFS_BASIC_SECTOR
#define CDFS_MAX_NAME_LEN 256

/* Volume descriptor types (VdType) */
#define BOOT_VOLUME_DESCRIPTOR_TYPE		0
#define PRIMARY_VOLUME_DESCRIPTOR_TYPE		1
#define SUPPLEMENTARY_VOLUME_DESCRIPTOR_TYPE	2
#define VOLUME_PARTITION_DESCRIPTOR_TYPE	3
#define VOLUME_DESCRIPTOR_SET_TERMINATOR	255

#include <pshpack1.h>
struct _DIR_RECORD
{
  UCHAR  RecordLength;			// 1
  UCHAR  ExtAttrRecordLength;		// 2
  ULONG  ExtentLocationL;		// 3-6
  ULONG  ExtentLocationM;		// 7-10
  ULONG  DataLengthL;			// 11-14
  ULONG  DataLengthM;			// 15-18
  UCHAR  Year;				// 19
  UCHAR  Month;				// 20
  UCHAR  Day;				// 21
  UCHAR  Hour;				// 22
  UCHAR  Minute;			// 23
  UCHAR  Second;			// 24
  UCHAR  TimeZone;			// 25
  UCHAR  FileFlags;			// 26
  UCHAR  FileUnitSize;			// 27
  UCHAR  InterleaveGapSize;		// 28
  ULONG  VolumeSequenceNumber;		// 29-32
  UCHAR  FileIdLength;			// 33
  UCHAR  FileId[1];			// 34
};
#include <poppack.h>

typedef struct _DIR_RECORD DIR_RECORD, *PDIR_RECORD;

/* DIR_RECORD.FileFlags */
#define FILE_FLAG_HIDDEN    0x01
#define FILE_FLAG_DIRECTORY 0x02
#define FILE_FLAG_SYSTEM    0x04
#define FILE_FLAG_READONLY  0x10


/* Volume Descriptor header*/
#include <pshpack1.h>
struct _VD_HEADER
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
};

typedef struct _VD_HEADER VD_HEADER, *PVD_HEADER;



/* Primary Volume Descriptor */
struct _PVD
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
  UCHAR  unused0;			// 8
  UCHAR  SystemId[32];			// 9-40
  UCHAR  VolumeId[32];			// 41-72
  UCHAR  unused1[8];			// 73-80
  ULONG  VolumeSpaceSizeL;		// 81-84
  ULONG  VolumeSpaceSizeM;		// 85-88
  UCHAR  unused2[32];			// 89-120
  ULONG  VolumeSetSize;			// 121-124
  ULONG  VolumeSequenceNumber;		// 125-128
  ULONG  LogicalBlockSize;		// 129-132
  ULONG  PathTableSizeL;		// 133-136
  ULONG  PathTableSizeM;		// 137-140
  ULONG  LPathTablePos;			// 141-144
  ULONG  LOptPathTablePos;		// 145-148
  ULONG  MPathTablePos;			// 149-152
  ULONG  MOptPathTablePos;		// 153-156
  DIR_RECORD RootDirRecord;		// 157-190
  UCHAR  VolumeSetIdentifier[128];	// 191-318
  UCHAR  PublisherIdentifier[128];	// 319-446

  /* more data ... */

};
typedef struct _PVD PVD, *PPVD;


/* Supplementary Volume Descriptor */
struct _SVD
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
  UCHAR  VolumeFlags;			// 8
  UCHAR  SystemId[32];			// 9-40
  UCHAR  VolumeId[32];			// 41-72
  UCHAR  unused1[8];			// 73-80
  ULONG  VolumeSpaceSizeL;		// 81-84
  ULONG  VolumeSpaceSizeM;		// 85-88
  UCHAR  EscapeSequences[32];		// 89-120
  ULONG  VolumeSetSize;			// 121-124
  ULONG  VolumeSequenceNumber;		// 125-128
  ULONG  LogicalBlockSize;		// 129-132
  ULONG  PathTableSizeL;		// 133-136
  ULONG  PathTableSizeM;		// 137-140
  ULONG  LPathTablePos;			// 141-144
  ULONG  LOptPathTablePos;		// 145-148
  ULONG  MPathTablePos;			// 149-152
  ULONG  MOptPathTablePos;		// 153-156
  DIR_RECORD RootDirRecord;		// 157-190
  UCHAR  VolumeSetIdentifier[128];	// 191-318
  UCHAR  PublisherIdentifier[128];	// 319-446

 // more data ...
};
#include <poppack.h>

typedef struct _SVD SVD, *PSVD;







typedef struct _CDINFO
{
  ULONG VolumeOffset;
  ULONG VolumeSpaceSize;
  ULONG JolietLevel;
  ULONG RootStart;
  ULONG RootSize;
  WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
  USHORT VolumeLabelLength;
  ULONG SerialNumber;
} CDINFO, *PCDINFO;


typedef struct
{
  ERESOURCE VcbResource;
  ERESOURCE DirResource;

  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;

  PVPB Vpb;
  PDEVICE_OBJECT VolumeDevice;
  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT StreamFileObject;

  CDINFO CdInfo;

  /* Notifications */
  LIST_ENTRY NotifyList;
  PNOTIFY_SYNC NotifySync;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION, VCB, *PVCB;


#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_IS_VOLUME_STREAM    0x0002
#define FCB_IS_VOLUME           0x0004

#define MAX_PATH                260

typedef struct _CDFS_SHORT_NAME
{
    LIST_ENTRY Entry;
    LARGE_INTEGER StreamOffset;
    UNICODE_STRING Name;
    WCHAR NameBuffer[13];
} CDFS_SHORT_NAME, *PCDFS_SHORT_NAME;

typedef struct _FCB
{
  FSRTL_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;
  ERESOURCE MainResource;
  ERESOURCE PagingIoResource;

  PFILE_OBJECT FileObject;
  PDEVICE_EXTENSION DevExt;

  UNICODE_STRING ShortNameU;

  WCHAR *ObjectName;			/* point on filename (250 chars max) in PathName */
  UNICODE_STRING PathName;		/* path+filename 260 max */
  WCHAR PathNameBuffer[MAX_PATH];	/* Buffer for PathName */
  WCHAR ShortNameBuffer[13];

  LIST_ENTRY FcbListEntry;
  struct _FCB* ParentFcb;

  ULONG DirIndex;

  LARGE_INTEGER IndexNumber;		/* HighPart: Parent directory start sector */
					/* LowPart: Directory record offset in the parent directory file */

  LONG RefCount;
  ULONG Flags;

  DIR_RECORD Entry;

  ERESOURCE  NameListResource;
  LIST_ENTRY ShortNameList;
} FCB, *PFCB;


typedef struct _CCB
{
  LIST_ENTRY     NextCCB;
  PFILE_OBJECT   PtrFileObject;
  LARGE_INTEGER  CurrentByteOffset;
  /* for DirectoryControl */
  ULONG Entry;
  ULONG Offset;
  /* for DirectoryControl */
  UNICODE_STRING DirectorySearchPattern;
  ULONG LastCluster;
  ULONG LastOffset;
} CCB, *PCCB;

#define CDFS_TAG                'sfdC'
#define CDFS_CCB_TAG            'ccdC'
#define CDFS_NONPAGED_FCB_TAG   'nfdC'
#define CDFS_SHORT_NAME_TAG     'sgdC'
#define CDFS_SEARCH_PATTERN_TAG 'eedC'
#define CDFS_FILENAME_TAG       'nFdC'

typedef struct _CDFS_GLOBAL_DATA
{
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT DeviceObject;
  ULONG Flags;
  CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
  FAST_IO_DISPATCH FastIoDispatch;
    NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
} CDFS_GLOBAL_DATA, *PCDFS_GLOBAL_DATA;

#define IRPCONTEXT_CANWAIT 0x1
#define IRPCONTEXT_COMPLETE 0x2
#define IRPCONTEXT_QUEUE 0x4

typedef struct _CDFS_IRP_CONTEXT
{
//    NTFSIDENTIFIER Identifier;
    ULONG Flags;
    PIO_STACK_LOCATION Stack;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIRP Irp;
    BOOLEAN IsTopLevel;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    NTSTATUS SavedExceptionCode;
    CCHAR PriorityBoost;
} CDFS_IRP_CONTEXT, *PCDFS_IRP_CONTEXT;


extern PCDFS_GLOBAL_DATA CdfsGlobalData;

/* cdfs.c */

NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath);

/* cleanup.c */

NTSTATUS
NTAPI
CdfsCleanup(
    PCDFS_IRP_CONTEXT IrpContext);


/* close.c */

NTSTATUS
NTAPI
CdfsClose(
    PCDFS_IRP_CONTEXT IrpContext);

NTSTATUS
CdfsCloseFile(PDEVICE_EXTENSION DeviceExt,
	      PFILE_OBJECT FileObject);


/* common.c */

NTSTATUS
CdfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN OUT PUCHAR Buffer,
		IN BOOLEAN Override);

NTSTATUS
CdfsDeviceIoControl (IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG CtlCode,
		     IN PVOID InputBuffer,
		     IN ULONG InputBufferSize,
		     IN OUT PVOID OutputBuffer,
		     IN OUT PULONG pOutputBufferSize,
		     IN BOOLEAN Override);

/* create.c */

NTSTATUS
NTAPI
CdfsCreate(
    PCDFS_IRP_CONTEXT IrpContext);

/* devctrl.c */

NTSTATUS NTAPI
CdfsDeviceControl(
    PCDFS_IRP_CONTEXT IrpContext);

/* dirctl.c */

NTSTATUS
NTAPI
CdfsDirectoryControl(
    PCDFS_IRP_CONTEXT IrpContext);

/* dispatch.c */

DRIVER_DISPATCH CdfsFsdDispatch;
NTSTATUS
NTAPI
CdfsFsdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* fastio.c */

BOOLEAN
NTAPI
CdfsAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID
NTAPI
CdfsReleaseFromLazyWrite(IN PVOID Context);

FAST_IO_CHECK_IF_POSSIBLE CdfsFastIoCheckIfPossible;
FAST_IO_READ CdfsFastIoRead;
FAST_IO_WRITE CdfsFastIoWrite;

/* fcb.c */

PFCB
CdfsCreateFCB(PCWSTR FileName);

VOID
CdfsDestroyFCB(PFCB Fcb);

BOOLEAN
CdfsFCBIsDirectory(PFCB Fcb);

BOOLEAN
CdfsFCBIsRoot(PFCB Fcb);

VOID
CdfsGrabFCB(PDEVICE_EXTENSION Vcb,
	    PFCB Fcb);

VOID
CdfsReleaseFCB(PDEVICE_EXTENSION Vcb,
	       PFCB Fcb);

VOID
CdfsAddFCBToTable(PDEVICE_EXTENSION Vcb,
		  PFCB Fcb);

PFCB
CdfsGrabFCBFromTable(PDEVICE_EXTENSION Vcb,
		     PUNICODE_STRING FileName);

NTSTATUS
CdfsFCBInitializeCache(PVCB Vcb,
		       PFCB Fcb);

PFCB
CdfsMakeRootFCB(PDEVICE_EXTENSION Vcb);

PFCB
CdfsOpenRootFCB(PDEVICE_EXTENSION Vcb);

NTSTATUS
CdfsMakeFCBFromDirEntry(PVCB Vcb,
			PFCB DirectoryFCB,
			PWSTR LongName,
			PWSTR ShortName,
			PDIR_RECORD Record,
			ULONG DirectorySector,
			ULONG DirectoryOffset,
			PFCB * fileFCB);

NTSTATUS
CdfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
			  PFCB Fcb,
			  PFILE_OBJECT FileObject);

NTSTATUS
CdfsDirFindFile(PDEVICE_EXTENSION DeviceExt,
		PFCB DirectoryFcb,
		PUNICODE_STRING FileToFind,
		PFCB *FoundFCB);

NTSTATUS
CdfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
		  PFCB *pParentFCB,
		  PFCB *pFCB,
		  PUNICODE_STRING FileName);


/* finfo.c */

NTSTATUS
NTAPI
CdfsQueryInformation(
    PCDFS_IRP_CONTEXT IrpContext);

NTSTATUS
NTAPI
CdfsSetInformation(
    PCDFS_IRP_CONTEXT IrpContext);


/* fsctl.c */

NTSTATUS NTAPI
CdfsFileSystemControl(
    PCDFS_IRP_CONTEXT IrpContext);


/* misc.c */

BOOLEAN
CdfsIsIrpTopLevel(
    PIRP Irp);

PCDFS_IRP_CONTEXT
CdfsAllocateIrpContext(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

VOID
CdfsSwapString(PWCHAR Out,
	       PUCHAR In,
	       ULONG Count);

VOID
CdfsDateTimeToSystemTime(PFCB Fcb,
			 PLARGE_INTEGER SystemTime);

VOID
CdfsFileFlagsToAttributes(PFCB Fcb,
			  PULONG FileAttributes);

VOID
CdfsShortNameCacheGet
(PFCB DirectoryFcb,
 PLARGE_INTEGER StreamOffset,
 PUNICODE_STRING LongName,
 PUNICODE_STRING ShortName);

BOOLEAN
CdfsIsRecordValid(IN PDEVICE_EXTENSION DeviceExt,
                  IN PDIR_RECORD Record);

/* rw.c */

NTSTATUS
NTAPI
CdfsRead(
    PCDFS_IRP_CONTEXT IrpContext);

NTSTATUS
NTAPI
CdfsWrite(
    PCDFS_IRP_CONTEXT IrpContext);


/* volinfo.c */

NTSTATUS
NTAPI
CdfsQueryVolumeInformation(
    PCDFS_IRP_CONTEXT IrpContext);

NTSTATUS
NTAPI
CdfsSetVolumeInformation(
    PCDFS_IRP_CONTEXT IrpContext);

#endif /* CDFS_H */
