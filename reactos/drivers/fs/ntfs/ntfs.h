#ifndef NTFS_H
#define NTFS_H

#include <ddk/ntifs.h>


#define CACHEPAGESIZE(pDeviceExt) \
	((pDeviceExt)->NtfsInfo.BytesPerCluster > PAGE_SIZE ? \
	 (pDeviceExt)->NtfsInfo.BytesPerCluster : PAGE_SIZE)

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
  ULARGE_INTEGER MftStart;
  ULARGE_INTEGER MftMirrStart;
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


typedef enum
{
  AttributeStandardInformation = 0x10,
  AttributeAttributeList = 0x20,
  AttributeFileName = 0x30,
  AttributeObjectId = 0x40,
  AttributeSecurityDescriptor = 0x50,
  AttributeVolumeName = 0x60,
  AttributeVolumeInformation = 0x70,
  AttributeData = 0x80,
  AttributeIndexRoot = 0x90,
  AttributeIndexAllocation = 0xA0,
  AttributeBitmap = 0xB0,
  AttributeReparsePoint = 0xC0,
  AttributeEAInformation = 0xD0,
  AttributeEA = 0xE0,
  AttributePropertySet = 0xF0,
  AttributeLoggedUtilityStream = 0x100
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;


typedef struct
{
  ULONG Type;
  USHORT UsnOffset;
  USHORT UsnSize;
  ULONGLONG Usn;
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

typedef struct
{
  NTFS_RECORD_HEADER Ntfs;
  USHORT SequenceNumber;
  USHORT LinkCount;
  USHORT AttributeOffset;
  USHORT Flags;
  ULONG BytesInUse;
  ULONG BytesAllocated;
  ULONGLONG BaseFileRecord;
  USHORT NextAttributeNumber;
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

typedef struct
{
  ATTRIBUTE_TYPE AttributeType;
  ULONG Length;
  BOOLEAN Nonresident;
  UCHAR NameLength;
  USHORT NameOffset;
  USHORT Flags;
  USHORT AttributeNumber;
} ATTRIBUTE, *PATTRIBUTE;

typedef struct
{
  ATTRIBUTE Attribute;
  ULONG ValueLength;
  USHORT ValueOffset;
  UCHAR Flags;
//  UCHAR Padding0;
} RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE;

typedef struct
{
  ATTRIBUTE Attribute;
  ULONGLONG StartVcn; // LowVcn
  ULONGLONG LastVcn; // HighVcn
  USHORT RunArrayOffset;
  USHORT CompressionUnit;
  ULONG  Padding0;
  UCHAR  IndexedFlag;
  ULONGLONG AllocatedSize;
  ULONGLONG DataSize;
  ULONGLONG InitializedSize;
  ULONGLONG CompressedSize;
} NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;


typedef struct
{
  ULONGLONG CreationTime;
  ULONGLONG ChangeTime;
  ULONGLONG LastWriteTime;
  ULONGLONG LastAccessTime;
  ULONG FileAttribute;
  ULONG AlignmentOrReserved[3];
#if 0
  ULONG QuotaId;
  ULONG SecurityId;
  ULONGLONG QuotaCharge;
  USN Usn;
#endif
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;


typedef struct
{
  ATTRIBUTE_TYPE AttributeType;
  USHORT Length;
  UCHAR NameLength;
  UCHAR NameOffset;
  ULONGLONG StartVcn; // LowVcn
  ULONGLONG FileReferenceNumber;
  USHORT AttributeNumber;
  USHORT AlignmentOrReserved[3];
} ATTRIBUTE_LIST, *PATTRIBUTE_LIST;


typedef struct
{
  ULONGLONG DirectoryFileReferenceNumber;
  ULONGLONG CreationTime;
  ULONGLONG ChangeTime;
  ULONGLONG LastWriteTime;
  ULONGLONG LastAccessTime;
  ULONGLONG AllocatedSize;
  ULONGLONG DataSize;
  ULONG FileAttributes;
  ULONG AlignmentOrReserved;
  UCHAR NameLength;
  UCHAR NameType;
  WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;



extern PNTFS_GLOBAL_DATA NtfsGlobalData;

//int CdfsStrcmpi( wchar_t *str1, wchar_t *str2 );
//void CdfsWstrcpy( wchar_t *str1, wchar_t *str2, int max );


/* attrib.c */

VOID
NtfsDumpAttribute(PATTRIBUTE Attribute);


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


/* mft.c */
NTSTATUS
NtfsOpenMft(PDEVICE_OBJECT DeviceObject,
	    PDEVICE_EXTENSION Vcb);


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
