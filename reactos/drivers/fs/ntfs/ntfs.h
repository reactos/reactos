#ifndef NTFS_H
#define NTFS_H

#include <ddk/ntifs.h>


#define CACHEPAGESIZE(pDeviceExt) \
	((pDeviceExt)->NtfsInfo.BytesPerCluster > PAGESIZE ? \
	 (pDeviceExt)->NtfsInfo.BytesPerCluster : PAGESIZE)

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


typedef struct _BOOT_SECTOR
{
  UCHAR     Magic[3];				// 0x00
  UCHAR     OemName[8];				// 0x03
  USHORT    BytesPerSector;			// 0x0B
  UCHAR     SectorsPerCluster;			// 0x0D
  UCHAR     Unused0[7];				// 0x0E
  UCHAR     MediaId;				// 0x15
  UCHAR     Unused1[2];				// 0x16
  USHORT    SectorsPerTrack;
  USHORT    Heads;
  UCHAR     Unused2[8];
  UCHAR     Unknown0[4]; /* always 80 00 80 00 */
  ULONGLONG SectorCount;
  ULONGLONG MftLocation;
  ULONGLONG MftMirrLocation;
  ULONG     ClustersPerMftRecord;
  ULONG     ClustersPerIndexRecord;
  ULONGLONG SerialNumber;			// 0x48
  UCHAR     BootCode[432];			// 0x50
} __attribute__((packed)) BOOT_SECTOR, *PBOOT_SECTOR;

//typedef struct _BootSector BootSector;





typedef struct _NTFS_INFO
{
  ULONG BytesPerSector;
  ULONG SectorsPerCluster;
  ULONG BytesPerCluster;
  ULONGLONG SectorCount;
  ULONGLONG MftStart;
  ULONGLONG MftMirrStart;
  ULONGLONG SerialNumber;

} NTFS_INFO, *PNTFS_INFO;


typedef struct
{
  ERESOURCE DirResource;
//  ERESOURCE FatResource;

  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;

  PVPB Vpb;
  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT StreamFileObject;

  NTFS_INFO NtfsInfo;


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

  ERESOURCE PagingIoResource;
  ERESOURCE MainResource;

  LIST_ENTRY FcbListEntry;
  struct _FCB* ParentFcb;

  ULONG DirIndex;

  LONG RefCount;
  ULONG Flags;

//  DIR_RECORD Entry;


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
} NTFS_GLOBAL_DATA, *PNTFS_GLOBAL_DATA;

extern PNTFS_GLOBAL_DATA NtfsGlobalData;




//int CdfsStrcmpi( wchar_t *str1, wchar_t *str2 );
//void CdfsWstrcpy( wchar_t *str1, wchar_t *str2, int max );


/* blockdev.c */

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN ULONG SectorSize,
		IN OUT PUCHAR Buffer);

NTSTATUS
NtfsReadRawSectors(IN PDEVICE_OBJECT DeviceObject,
		   IN ULONG DiskSector,
		   IN ULONG SectorCount,
		   IN ULONG SectorSize,
		   IN OUT PUCHAR Buffer);

NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
		    IN ULONG ControlCode,
		    IN PVOID InputBuffer,
		    IN ULONG InputBufferSize,
		    IN OUT PVOID OutputBuffer,
		    IN OUT PULONG OutputBufferSize);

#if 0
/* close.c */

NTSTATUS STDCALL
CdfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp);
#endif

/* create.c */

NTSTATUS STDCALL
NtfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp);


/* dirctl.c */

NTSTATUS STDCALL
NtfsDirectoryControl(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp);


/* fcb.c */

PFCB
NtfsCreateFCB(PWCHAR FileName);

VOID
NtfsDestroyFCB(PFCB Fcb);

BOOLEAN
NtfsFCBIsDirectory(PFCB Fcb);

BOOLEAN
NtfsFCBIsRoot(PFCB Fcb);

VOID
NtfsGrabFCB(PDEVICE_EXTENSION Vcb,
	    PFCB Fcb);

VOID
NtfsReleaseFCB(PDEVICE_EXTENSION Vcb,
	       PFCB Fcb);

VOID
NtfsAddFCBToTable(PDEVICE_EXTENSION Vcb,
		  PFCB Fcb);

PFCB
NtfsGrabFCBFromTable(PDEVICE_EXTENSION Vcb,
		     PWSTR FileName);

NTSTATUS
NtfsFCBInitializeCache(PVCB Vcb,
		       PFCB Fcb);

PFCB
NtfsMakeRootFCB(PDEVICE_EXTENSION Vcb);

PFCB
NtfsOpenRootFCB(PDEVICE_EXTENSION Vcb);

NTSTATUS
NtfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
			  PFCB Fcb,
			  PFILE_OBJECT FileObject);

NTSTATUS
NtfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
		  PFCB *pParentFCB,
		  PFCB *pFCB,
		  const PWSTR pFileName);


/* finfo.c */

NTSTATUS STDCALL
NtfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp);


/* fsctl.c */

NTSTATUS STDCALL
NtfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp);

#if 0
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
#endif


/* volinfo.c */

NTSTATUS STDCALL
NtfsQueryVolumeInformation(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp);

NTSTATUS STDCALL
NtfsSetVolumeInformation(PDEVICE_OBJECT DeviceObject,
			 PIRP Irp);

#endif /* NTFS_H */
