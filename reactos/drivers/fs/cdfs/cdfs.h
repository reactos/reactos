#ifndef CDFS_H
#define CDFS_H

#include <ddk/ntifs.h>

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
} __attribute__((packed));

typedef struct _DIR_RECORD DIR_RECORD, *PDIR_RECORD;




/* Volume Descriptor header*/
struct _VD_HEADER
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
} __attribute__((packed));

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

} __attribute__((packed));

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
} __attribute__((packed));

typedef struct _SVD SVD, *PSVD;







typedef struct _CDINFO
{
  ULONG VolumeSpaceSize;
  ULONG JolietLevel;
  ULONG RootStart;
  ULONG RootSize;

} CDINFO, *PCDINFO;


typedef struct
{
  ERESOURCE DirResource;
//  ERESOURCE FatResource;

  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;

  PVPB Vpb;
  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT StreamFileObject;

  CDINFO CdInfo;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION, VCB, *PVCB;


#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_IS_VOLUME_STREAM    0x0002
#define FCB_IS_VOLUME           0x0004

typedef struct _FCB
{
  REACTOS_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;

  PFILE_OBJECT FileObject;
  PDEVICE_EXTENSION DevExt;

  WCHAR *ObjectName;		/* point on filename (250 chars max) in PathName */
  WCHAR PathName[MAX_PATH];	/* path+filename 260 max */

//  ERESOURCE PagingIoResource;
  ERESOURCE MainResource;

  LIST_ENTRY FcbListEntry;
  struct _FCB* ParentFcb;

  ULONG DirIndex;

  LONG RefCount;
  ULONG Flags;

  DIR_RECORD Entry;


} FCB, *PFCB;


typedef struct _CCB
{
  PFCB           Fcb;
  LIST_ENTRY     NextCCB;
  PFILE_OBJECT   PtrFileObject;
  LARGE_INTEGER  CurrentByteOffset;
  /* for DirectoryControl */
  ULONG Entry;
  /* for DirectoryControl */
  PWCHAR DirectorySearchPattern;
  ULONG LastCluster;
  ULONG LastOffset;
} CCB, *PCCB;

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

#define TAG_CCB TAG('I', 'C', 'C', 'B')



typedef struct
{
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT DeviceObject;
  ULONG Flags;
} CDFS_GLOBAL_DATA, *PCDFS_GLOBAL_DATA;

extern PCDFS_GLOBAL_DATA CdfsGlobalData;


NTSTATUS
CdfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN OUT PUCHAR Buffer);

int CdfsStrcmpi( wchar_t *str1, wchar_t *str2 );
void CdfsWstrcpy( wchar_t *str1, wchar_t *str2, int max );



/* close.c */

NTSTATUS STDCALL
CdfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp);

/* common.c */

NTSTATUS
CdfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN OUT PUCHAR Buffer);

NTSTATUS
CdfsReadRawSectors(IN PDEVICE_OBJECT DeviceObject,
		   IN ULONG DiskSector,
		   IN ULONG SectorCount,
		   IN OUT PUCHAR Buffer);

/* create.c */

NTSTATUS STDCALL
CdfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp);


/* dirctl.c */

NTSTATUS STDCALL
CdfsDirectoryControl(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp);


/* fcb.c */

PFCB
CdfsCreateFCB(PWCHAR FileName);

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
		     PWSTR FileName);

NTSTATUS
CdfsFCBInitializeCache(PVCB Vcb,
		       PFCB Fcb);

PFCB
CdfsMakeRootFCB(PDEVICE_EXTENSION Vcb);

PFCB
CdfsOpenRootFCB(PDEVICE_EXTENSION Vcb);



NTSTATUS
CdfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
			  PFCB Fcb,
			  PFILE_OBJECT FileObject);




NTSTATUS
CdfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
		  PFCB *pParentFCB,
		  PFCB *pFCB,
		  const PWSTR pFileName);


/* finfo.c */

NTSTATUS STDCALL
CdfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp);

/* fsctl.c */

NTSTATUS STDCALL
CdfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp);

/* misc.c */

BOOLEAN
wstrcmpjoki(PWSTR s1, PWSTR s2);

VOID
CdfsSwapString(PWCHAR Out,
	       PUCHAR In,
	       ULONG Count);

VOID
CdfsDateTimeToFileTime(PFCB Fcb,
		       TIME *FileTime);

VOID
CdfsFileFlagsToAttributes(PFCB Fcb,
			  PULONG FileAttributes);

/* rw.c */

NTSTATUS STDCALL
CdfsRead(PDEVICE_OBJECT DeviceObject,
	PIRP Irp);

NTSTATUS STDCALL
CdfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp);


/* volinfo.c */

NTSTATUS STDCALL
CdfsQueryVolumeInformation(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp);

NTSTATUS STDCALL
CdfsSetVolumeInformation(PDEVICE_OBJECT DeviceObject,
			 PIRP Irp);

#endif //CDFS_H
