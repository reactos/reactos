/* $Id: vfat.h,v 1.19 2000/12/29 23:17:12 dwelch Exp $ */

#include <ddk/ntifs.h>

struct _BootSector { 
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

struct _BootSector32 {
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
  unsigned long  FATSectors32;
  unsigned char  x[27];
  unsigned long  VolumeID;
  unsigned char  VolumeLabel[11], SysType[8];
  unsigned char  Res2[422];
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

typedef struct _FATDirEntry FATDirEntry;

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
   BootSector *Boot;
   int rootDirectorySectors, FATStart, rootStart, dataStart;
   int FATEntriesPerSector, FATUnit;
   ULONG BytesPerCluster;
   ULONG FatType;
   unsigned char* FAT;   
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _VFATFCB
{
  REACTOS_COMMON_FCB_HEADER RFCB;
  FATDirEntry entry;
  /* point on filename (250 chars max) in PathName */
  WCHAR *ObjectName; 
  /* path+filename 260 max */
  WCHAR PathName[MAX_PATH]; 
  LONG RefCount;
  PDEVICE_EXTENSION pDevExt;
  LIST_ENTRY FcbListEntry;
  struct _VFATFCB* parentFcb;
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


/* internal functions in blockdev.c */
BOOLEAN 
VFATReadSectors(IN PDEVICE_OBJECT pDeviceObject,
		IN ULONG   DiskSector,
		IN ULONG       SectorCount,
		IN UCHAR*	Buffer);

BOOLEAN 
VFATWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
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
VfatGetStandardInformation(PVFATFCB FCB, PDEVICE_OBJECT DeviceObject,
                                   PFILE_STANDARD_INFORMATION StandardInfo);
NTSTATUS 
VfatOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
	    PWSTR FileName);
NTSTATUS 
VfatReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	     PVOID Buffer, ULONG Length, ULONG ReadOffset,
             PULONG LengthRead);
NTSTATUS 
VfatWriteFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
              PVOID Buffer, ULONG Length, ULONG WriteOffset);
ULONG 
GetNextWriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster);
BOOLEAN 
IsDeletedEntry(PVOID Block, ULONG Offset);
BOOLEAN 
IsLastEntry(PVOID Block, ULONG Offset);
wchar_t* 
vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount);
VOID 
VFATWriteCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster);

/* internal functions in dirwr.c */
NTSTATUS 
addEntry(PDEVICE_EXTENSION DeviceExt,
	 PFILE_OBJECT pFileObject,ULONG RequestedOptions,UCHAR ReqAttr);
NTSTATUS 
updEntry(PDEVICE_EXTENSION DeviceExt,PFILE_OBJECT pFileObject);

/*
 * String functions
 */
VOID 
RtlAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length);
VOID 
RtlCatAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length);
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

/*
 * functions from fat.c
 */
ULONG 
ClusterToSector(PDEVICE_EXTENSION DeviceExt, ULONG Cluster);
ULONG 
GetNextCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster);
VOID 
VFATLoadCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster);
ULONG 
FAT12CountAvailableClusters(PDEVICE_EXTENSION DeviceExt);
ULONG 
FAT16CountAvailableClusters(PDEVICE_EXTENSION DeviceExt);
ULONG 
FAT32CountAvailableClusters(PDEVICE_EXTENSION DeviceExt);

/*
 * functions from volume.c
 */
NTSTATUS STDCALL 
VfatQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*
 * functions from finfo.c
 */
NTSTATUS STDCALL 
VfatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*
 * From create.c
 */
NTSTATUS 
ReadVolumeLabel(PDEVICE_EXTENSION DeviceExt, PVPB Vpb);

/*
 * functions from shutdown.c
 */
NTSTATUS STDCALL VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);

