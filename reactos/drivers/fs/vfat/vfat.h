




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

#define FAT16 (1)
#define FAT12 (2)
#define FAT32 (3)

typedef struct
{
  ERESOURCE Resource;
   PDEVICE_OBJECT StorageDevice;
   BootSector *Boot;
   int rootDirectorySectors, FATStart, rootStart, dataStart;
   int FATEntriesPerSector, FATUnit;
   ULONG BytesPerCluster;
   ULONG FatType;
   unsigned char* FAT;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _FSRTL_COMMON_FCB_HEADER{
  char  IsFastIoPossible;//is char the realtype ?
  ERESOURCE Resource;
  ERESOURCE PagingIoResource;
  ULONG  Flags;// is long the real type ?
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  LARGE_INTEGER ValidDataLength;
  // other fields ??
} FSRTL_COMMON_FCB_HEADER;

typedef struct _SFsdNTRequiredFCB {
  FSRTL_COMMON_FCB_HEADER CommonFCBHeader;
  SECTION_OBJECT_POINTERS SectionObject;
  ERESOURCE               MainResource;
  ERESOURCE               PagingIoResource;
} SFsdNTRequiredFCB, *PtrSFsdNTRequiredFCB;

struct _VfatFCB;
typedef struct _VfatFCB
{
  SFsdNTRequiredFCB     NTRequiredFCB;
   FATDirEntry entry;
   WCHAR *ObjectName; // point on filename (250 chars max) in PathName
   WCHAR PathName[MAX_PATH];// path+filename 260 max
   long RefCount;
   PDEVICE_EXTENSION pDevExt;
   struct _VfatFCB * nextFcb, *prevFcb;
   struct _VfatFCB * parentFcb;
} VfatFCB, *PVfatFCB;

typedef struct
{
  VfatFCB *   pFcb;
  LIST_ENTRY     NextCCB;
  PFILE_OBJECT   PtrFileObject;
  LARGE_INTEGER  CurrentByteOffset;
  ULONG StartSector; // for DirectoryControl
  ULONG StartEntry;  //for DirectoryControl
//    PSTRING DirectorySearchPattern;// for DirectoryControl ?
} VfatCCB, *PVfatCCB;


#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))


extern PVfatFCB pFirstFcb;

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
NTSTATUS FindFile(PDEVICE_EXTENSION DeviceExt, PVfatFCB Fcb,
          PVfatFCB Parent, PWSTR FileToFind,ULONG *StartSector,ULONG *Entry);
NTSTATUS FsdCloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject);
NTSTATUS FsdGetStandardInformation(PVfatFCB FCB, PDEVICE_OBJECT DeviceObject,
                                   PFILE_STANDARD_INFORMATION StandardInfo);
NTSTATUS FsdOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
             PWSTR FileName);
NTSTATUS FsdReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		     PVOID Buffer, ULONG Length, ULONG ReadOffset,
             PULONG LengthRead);
NTSTATUS FsdWriteFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
              PVOID Buffer, ULONG Length, ULONG WriteOffset);
ULONG GetNextWriteCluster(PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster);
BOOLEAN IsDeletedEntry(PVOID Block, ULONG Offset);
BOOLEAN IsLastEntry(PVOID Block, ULONG Offset);
wchar_t * vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount);
void VFATWriteCluster(PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster);

//internal functions in dirwr.c
NTSTATUS addEntry(PDEVICE_EXTENSION DeviceExt
                  ,PFILE_OBJECT pFileObject,ULONG RequestedOptions,UCHAR ReqAttr);
NTSTATUS updEntry(PDEVICE_EXTENSION DeviceExt,PFILE_OBJECT pFileObject);


//FIXME : following defines must be removed
//FIXME   when this functions will work.
#define ExAcquireResourceExclusiveLite(x,y) {}
#define ExReleaseResourceForThreadLite(x,y) {}
