/* $Id: vfat.h,v 1.35 2001/08/14 20:47:30 hbirr Exp $ */

#include <ddk/ntifs.h>

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
  unsigned char  Res2[450];
} __attribute__((packed));

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
  unsigned long  BootBackup;				// 50
  unsigned char  Res3[10];				// 54
  unsigned char  Drive;					// 64
  unsigned char  Res4;					// 65
  unsigned char  ExtBootSignature;			// 66
  unsigned long  VolumeID;				// 67
  unsigned char  VolumeLabel[11], SysType[8];		// 71
  unsigned char  Res2[418];				// 90
  unsigned long  Signature1;				// 508
} __attribute__((packed));

struct _BootBackupSector
{
  unsigned long  ExtBootSignature2;			// 0
  unsigned char  Res6[480];				// 4
  unsigned long  FSINFOSignature;			// 484
  unsigned long  FreeCluster;				// 488
  unsigned long  NextCluster;				// 492
  unsigned char  Res7[12];				// 496
  unsigned long  Signatur2;				// 508
} __attribute__((packed));

typedef struct _BootSector BootSector;

struct _FATDirEntry {
  unsigned char  Filename[8], Ext[3], Attrib, Res[2];
  unsigned short CreationTime,CreationDate,AccessDate;
  unsigned short FirstClusterHigh;// higher
  unsigned short UpdateTime;//time create/update
  unsigned short UpdateDate;//date create/update
  unsigned short FirstCluster;
  unsigned long  FileSize;
} __attribute__((packed));

typedef struct _FATDirEntry FATDirEntry, FAT_DIR_ENTRY, *PFAT_DIR_ENTRY;

struct _slot
{
  unsigned char id;               // sequence number for slot
  WCHAR  name0_4[5];      // first 5 characters in name
  unsigned char attr;             // attribute byte
  unsigned char reserved;         // always 0
  unsigned char alias_checksum;   // checksum for 8.3 alias
  WCHAR  name5_10[6];     // 6 more characters in name
  unsigned char start[2];         // starting cluster number
  WCHAR  name11_12[2];     // last 2 characters in name
} __attribute__((packed));


typedef struct _slot slot;

#define BLOCKSIZE 512

#define FAT16 (1)
#define FAT12 (2)
#define FAT32 (3)

typedef struct
{
  ERESOURCE DirResource;
  ERESOURCE FatResource;
  
  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;
  
  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT StreamStorageDevice;
  PBCB StorageBcb;
  PFILE_OBJECT Fat12StorageDevice;
  PBCB Fat12StorageBcb;
  BootSector *Boot;
  int rootDirectorySectors, FATStart, rootStart, dataStart;
  int BytesPerSector;
  int FATEntriesPerSector, FATUnit;
  ULONG BytesPerCluster;
  ULONG FatType;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION, VCB, *PVCB;

#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_DELETE_PENDING      0x0002

typedef struct _VFATFCB
{
  REACTOS_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;
  FATDirEntry entry;
  /* point on filename (250 chars max) in PathName */
  WCHAR *ObjectName;
  /* path+filename 260 max */
  WCHAR PathName[MAX_PATH];
  LONG RefCount;
  PDEVICE_EXTENSION pDevExt;
  LIST_ENTRY FcbListEntry;
  struct _VFATFCB* parentFcb;
  ULONG Flags;
} VFATFCB, *PVFATFCB;

typedef struct _VFATCCB
{
  VFATFCB *   pFcb;
  LIST_ENTRY     NextCCB;
  PFILE_OBJECT   PtrFileObject;
  LARGE_INTEGER  CurrentByteOffset;
  /* for DirectoryControl */
  ULONG StartSector; 
  /* for DirectoryControl */
  ULONG StartEntry;  
  //    PSTRING DirectorySearchPattern;// for DirectoryControl ?
} VFATCCB, *PVFATCCB;

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

#define TAG_CCB TAG('V', 'C', 'C', 'B')

#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))

typedef struct __DOSTIME
{
   WORD	Second:5; 
   WORD	Minute:6;
   WORD Hour:5;
} DOSTIME, *PDOSTIME;

typedef struct __DOSDATE
{
   WORD	Day:5; 
   WORD	Month:4;
   WORD Year:5;
} DOSDATE, *PDOSDATE;

/* functions called by i/o manager : */
NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT _DriverObject,PUNICODE_STRING RegistryPath);
NTSTATUS STDCALL
VfatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatCleanup (PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL
VfatSetVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS
NextCluster(PDEVICE_EXTENSION DeviceExt,
	    ULONG FirstCluster,
	    PULONG CurrentCluster,
	    BOOLEAN Extend);

/* internal functions in blockdev.c */
NTSTATUS
VfatReadSectors(IN PDEVICE_OBJECT pDeviceObject,
		IN ULONG   DiskSector,
		IN ULONG       SectorCount,
		IN UCHAR*	Buffer);

NTSTATUS
VfatWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
		 IN ULONG   DiskSector,
		 IN ULONG        SectorCount,
		 IN UCHAR*	Buffer);

/* internal functions in dir.c : */
BOOL FsdDosDateTimeToFileTime(WORD wDosDate,WORD wDosTime, TIME *FileTime);
BOOL FsdFileTimeToDosDateTime(TIME *FileTime,WORD *pwDosDate,WORD *pwDosTime);

/* internal functions in iface.c : */
NTSTATUS 
FindFile(PDEVICE_EXTENSION DeviceExt, PVFATFCB Fcb,
	 PVFATFCB Parent, PWSTR FileToFind,ULONG *StartSector,ULONG *Entry);
NTSTATUS 
VfatCloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject);
NTSTATUS 
VfatOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
	    PWSTR FileName);
NTSTATUS 
VfatReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	     PVOID Buffer, ULONG Length, ULONG ReadOffset,
             PULONG LengthRead, ULONG NoCache);
NTSTATUS 
VfatWriteFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
              PVOID Buffer, ULONG Length, ULONG WriteOffset, ULONG NoCache);
NTSTATUS
GetNextWriteCluster(PDEVICE_EXTENSION DeviceExt, 
		    ULONG FirstCluster,
		    ULONG CurrentCluster,
		    PULONG NextCluster);
BOOLEAN 
IsDeletedEntry(PVOID Block, ULONG Offset);
BOOLEAN 
IsLastEntry(PVOID Block, ULONG Offset);
wchar_t* 
vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount);
NTSTATUS 
VfatRawWriteCluster(PDEVICE_EXTENSION DeviceExt, 
		    ULONG FirstCluster, PVOID Buffer, ULONG Cluster);

/* internal functions in dirwr.c */
NTSTATUS 
addEntry(PDEVICE_EXTENSION DeviceExt,
	 PFILE_OBJECT pFileObject,ULONG RequestedOptions,UCHAR ReqAttr);
NTSTATUS 
updEntry(PDEVICE_EXTENSION DeviceExt,PFILE_OBJECT pFileObject);
NTSTATUS
delEntry(PDEVICE_EXTENSION, PFILE_OBJECT);

/*
 * String functions
 */
VOID 
vfat_initstr(wchar_t *wstr, ULONG wsize);
wchar_t* 
vfat_wcsncat(wchar_t * dest, const wchar_t * src,size_t wstart, size_t wcount);
wchar_t* 
vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount);
wchar_t* 
vfat_movstr(wchar_t *src, ULONG dpos, ULONG spos, ULONG len);
BOOLEAN 
wstrcmpi(PWSTR s1, PWSTR s2);
BOOLEAN 
wstrcmpjoki(PWSTR s1, PWSTR s2);
PWCHAR  vfatGetNextPathElement (PWCHAR  pFileName);
void  vfatWSubString (PWCHAR pTarget, const PWCHAR pSource, size_t pLength);
BOOL  vfatIsFileNameValid (PWCHAR pFileName);

/*
 * functions from fat.c
 */
NTSTATUS
OffsetToCluster(PDEVICE_EXTENSION DeviceExt, 
		ULONG FirstCluster, 
		ULONG FileOffset,
		PULONG Cluster,
		BOOLEAN Extend);
ULONG
ClusterToSector(PDEVICE_EXTENSION DeviceExt,
		ULONG Cluster);
NTSTATUS
GetNextCluster(PDEVICE_EXTENSION DeviceExt,
	       ULONG CurrentCluster,
	       PULONG NextCluster,
	       BOOLEAN Extend);
NTSTATUS
GetNextSector(PDEVICE_EXTENSION DeviceExt,
	      ULONG CurrentSector,
	      PULONG NextSector,
	      BOOLEAN Extend);
NTSTATUS
VfatRawReadCluster(PDEVICE_EXTENSION DeviceExt,
		   ULONG FirstCluster,
		   PVOID Buffer,
		   ULONG Cluster);
NTSTATUS
VfatRawWriteCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG FirstCluster,
		    PVOID Buffer,
		    ULONG Cluster);
NTSTATUS
FAT12CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters);
NTSTATUS
FAT16CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters);
NTSTATUS
FAT32CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters);
NTSTATUS
WriteCluster(PDEVICE_EXTENSION DeviceExt,
	     ULONG ClusterToWrite,
	     ULONG NewValue);

/*  ---------------------------------------------------------  create.c  */

void  vfat8Dot3ToString (PCHAR pBasename, PCHAR pExtension, PWSTR pName);
NTSTATUS
ReadVolumeLabel(PDEVICE_EXTENSION DeviceExt, PVPB Vpb);
NTSTATUS
VfatOpenFile(PDEVICE_EXTENSION DeviceExt,
	     PFILE_OBJECT FileObject,
	     PWSTR FileName);

/*  -----------------------------------------------  DirEntry Functions  */

ULONG  vfatDirEntryGetFirstCluster (PDEVICE_EXTENSION  pDeviceExt,
                                    PFAT_DIR_ENTRY  pDirEntry);
BOOL  vfatIsDirEntryDeleted (FATDirEntry * pFatDirEntry);
BOOL  vfatIsDirEntryVolume (FATDirEntry * pFatDirEntry);
BOOL  vfatIsDirEntryEndMarker (FATDirEntry * pFatDirEntry);
void  vfatGetDirEntryName (PFAT_DIR_ENTRY pDirEntry,  PWSTR  pEntryName);
NTSTATUS  vfatGetNextDirEntry (PDEVICE_EXTENSION  pDeviceExt,
                               PVFATFCB  pDirectoryFCB,
                               ULONG * pDirectoryIndex,
                               PWSTR pLongFileName,
                               PFAT_DIR_ENTRY pDirEntry);

/*  -----------------------------------------------------  FCB Functions */

PVFATFCB  vfatNewFCB (PWCHAR pFileName);
void  vfatDestroyFCB (PVFATFCB  pFCB);
void  vfatGrabFCB (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB);
void  vfatReleaseFCB (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB);
void  vfatAddFCBToTable (PDEVICE_EXTENSION  pVCB,  
                         PVFATFCB  pFCB);
PVFATFCB  vfatGrabFCBFromTable (PDEVICE_EXTENSION  pDeviceExt, 
                                PWSTR  pFileName);
NTSTATUS  vfatRequestAndValidateRegion (PDEVICE_EXTENSION  pDeviceExt, 
                                        PVFATFCB  pFCB, 
                                        ULONG  pOffset,
                                        PVOID * pBuffer,
                                        PCACHE_SEGMENT * pCacheSegment,
                                        BOOL  pExtend);
NTSTATUS  vfatReleaseRegion (PDEVICE_EXTENSION  pDeviceExt,
                             PVFATFCB  pFCB,
                             PCACHE_SEGMENT  pCacheSegment);
PVFATFCB  vfatMakeRootFCB (PDEVICE_EXTENSION  pVCB);
PVFATFCB  vfatOpenRootFCB (PDEVICE_EXTENSION  pVCB);
BOOL  vfatFCBIsDirectory (PDEVICE_EXTENSION pVCB, PVFATFCB FCB);
NTSTATUS  vfatAttachFCBToFileObject (PDEVICE_EXTENSION  vcb, 
                                     PVFATFCB  fcb,
                                     PFILE_OBJECT  fileObject);
NTSTATUS  vfatDirFindFile (PDEVICE_EXTENSION  pVCB, 
                           PVFATFCB  parentFCB, 
                           PWSTR  elementName,
                           PVFATFCB * fileFCB);
NTSTATUS  vfatGetFCBForFile (PDEVICE_EXTENSION  pVCB, 
                             PVFATFCB  *pParentFCB, 
                             PVFATFCB  *pFCB, 
                             const PWSTR  pFileName);
NTSTATUS vfatMakeFCBFromDirEntry(PVCB  vcb,
			                     PVFATFCB  directoryFCB,
			                     PWSTR  longName,
			                     PFAT_DIR_ENTRY  dirEntry,
			                     PVFATFCB * fileFCB);




