BOOLEAN VFATReadSectors(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
                        IN ULONG        SectorCount,
			IN UCHAR*	Buffer);

BOOLEAN VFATWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
			 IN ULONG	DiskSector,
                         IN ULONG        SectorCount,
			 IN UCHAR*	Buffer);

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
  unsigned char  Filename[8], Ext[3], Attrib, Res[8];
  unsigned short FirstClusterHigh;// higher
  unsigned char Res2[4];
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
