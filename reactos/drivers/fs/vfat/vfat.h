


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
  unsigned char  x[31];
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

// Put the rest in struct.h
/*
typedef unsigned int uint32;

typedef struct _SFsdIdentifier {
  uint32 NodeType;
  uint32 NodeSize;
} SFsdIdentifier, *PtrSFsdIdentifier;

typedef struct _SFsdNTRequiredFCB {
  FSRTL_COMMON_FCB_HEADER CommonFCBHeader;
  SECTION_OBJECT_POINTERS SectionObject;
  ERESOURCE               MainResource;
  ERESOURCE               PagingIoResource;
} SFsdNTRequiredFCB, *PtrSFsdNTRequiredFCB;

typedef struct _SFsdFileControlBlock {
  SFsdIdentifier        NodeIdentifier;
  SFsdNTRequiredFCB     NTRequiredFCB;
  SFsdDiskDependentFCB  DiskDependentFCB;
  struct _SFsdVolumeControlBlock   *PtrVCB;
  LIST_ENTRY   NextFCB;
  uint32       FCBFlags;
  LIST_ENTRY   NextCCB;
  SHARE_ACCESS FCBShareAccess;
  uint32       LazyWriterThreadID;
  uint32       ReferenceCount;
  uint32       OpenHandleCount;
  PtrSFsdObjectName FCBName;
  LARGE_INTEGER     CreationTime;
  LARGE_INTEGER     LastAccessTime;
  LARGE_INTEGER     LastWriteTime;
  SFsdFileLockAnchorFCB ByteRangeLock;
  OPLOCK  FCBOplock;
} SFsdFCB, *PtrSFsdFCB;

*/
#define FAT16 (1)
#define FAT12 (2)
#define FAT32 (3)

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
   BootSector *Boot;
   int rootDirectorySectors, FATStart, rootStart, dataStart;
   int FATEntriesPerSector, FATUnit;
   ULONG BytesPerCluster;
   ULONG FatType;
   unsigned char* FAT;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct
{
   FATDirEntry entry;
   WCHAR ObjectName[251];// filename has 250 characters max
   ULONG StartSector;
   ULONG StartEntry;//for DirectoryControl
} FCB, *PFCB;


#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))



// functions called by i/o manager :
NTSTATUS DriverEntry(PDRIVER_OBJECT _DriverObject,PUNICODE_STRING RegistryPath);
NTSTATUS FsdDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS FsdQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);


// internal functions in blockdev.c
BOOLEAN VFATReadSectors(IN PDEVICE_OBJECT pDeviceObject,
            IN ULONG   DiskSector,
                        IN ULONG       SectorCount,
			IN UCHAR*	Buffer);

BOOLEAN VFATWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
             IN ULONG   DiskSector,
                         IN ULONG        SectorCount,
			 IN UCHAR*	Buffer);

//internal functions in iface.c :
NTSTATUS FsdGetStandardInformation(PFCB FCB, PDEVICE_OBJECT DeviceObject,
                                   PFILE_STANDARD_INFORMATION StandardInfo);
NTSTATUS FindFile(PDEVICE_EXTENSION DeviceExt, PFCB Fcb,
          PFCB Parent, PWSTR FileToFind,ULONG *StartSector,ULONG *Entry);
wchar_t * vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount);
