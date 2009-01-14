#include <ntifs.h>
#include <ntdddisk.h>
#include <reactos/helper.h>
#include <debug.h>

/* FAT on-disk data structures */
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


/* File system types */
#define FAT16  (1)
#define FAT12  (2)
#define FAT32  (3)
#define FATX16 (4)
#define FATX32 (5)

/* VCB Flags */
#define VCB_VOLUME_LOCKED       0x0001
#define VCB_DISMOUNT_PENDING    0x0002
#define VCB_IS_FATX             0x0004
#define VCB_IS_DIRTY            0x4000 /* Volume is dirty */
#define VCB_CLEAR_DIRTY         0x8000 /* Clean dirty flag at shutdown */

typedef struct
{
  ULONG VolumeID;
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
} FATINFO, *PFATINFO;

struct _VFATFCB;
struct _VFAT_DIRENTRY_CONTEXT;

typedef struct DEVICE_EXTENSION *PDEVICE_EXTENSION;

typedef struct _DEVICE_EXTENSION
{
  ERESOURCE DirResource;
  ERESOURCE FatResource;

  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;
  ULONG HashTableSize;
  struct _HASHENTRY** FcbHashTable;

  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT FATFileObject;
  FATINFO FatInfo;
  ULONG LastAvailableCluster;
  ULONG AvailableClusters;
  BOOLEAN AvailableClustersValid;
  ULONG Flags;
  struct _VFATFCB * VolumeFcb;

  ULONG BaseDateYear;

  LIST_ENTRY VolumeListEntry;
} DEVICE_EXTENSION;

typedef struct _FAT_GLOBAL_DATA
{
    ERESOURCE Resource;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DiskDeviceObject;
    NPAGED_LOOKASIDE_LIST NonPagedFcbList;
    NPAGED_LOOKASIDE_LIST ResourceList;
    NPAGED_LOOKASIDE_LIST IrpContextList;
    FAST_IO_DISPATCH FastIoDispatch;
    CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
    CACHE_MANAGER_CALLBACKS CacheMgrNoopCallbacks;
} FAT_GLOBAL_DATA, *VFAT_GLOBAL_DATA;

extern VFAT_GLOBAL_DATA VfatGlobalData;

/* FCB flags */
#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_DELETE_PENDING      0x0002
#define FCB_IS_FAT              0x0004
#define FCB_IS_PAGE_FILE        0x0008
#define FCB_IS_VOLUME           0x0010
#define FCB_IS_DIRTY            0x0020
#define FCB_IS_FATX_ENTRY       0x0040

typedef struct _VFATFCB
{
  /* FCB header required by ROS/NT */
  FSRTL_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;
  ERESOURCE MainResource;
  ERESOURCE PagingIoResource;
  /* end FCB header required by ROS/NT */

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

  /* pointer to the parent fcb */
  struct _VFATFCB* parentFcb;

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

  /* List of byte-range locks for this file */
  FILE_LOCK FileLock;

  /*
   * Optimalization: caching of last read/write cluster+offset pair. Can't
   * be in VFATCCB because it must be reset everytime the allocated clusters
   * change.
   */
  FAST_MUTEX LastMutex;
  ULONG LastCluster;
  ULONG LastOffset;
} VFATFCB, *PVFATFCB;

typedef struct _VFATCCB
{
  LARGE_INTEGER  CurrentByteOffset;
  /* for DirectoryControl */
  ULONG Entry;
  /* for DirectoryControl */
  UNICODE_STRING SearchPattern;
} VFATCCB, *PVFATCCB;

/* Volume Control Block */
typedef struct _VCB
{
    FSRTL_ADVANCED_FCB_HEADER VolumeFileHeader;
} VCB, *PVCB;

/* Volume Device Object */
typedef struct _VOLUME_DEVICE_OBJECT
{
    DEVICE_OBJECT DeviceObject;
    FSRTL_COMMON_FCB_HEADER VolumeHeader;
    VCB Vcb; /* Must be the last entry! */
} VOLUME_DEVICE_OBJECT, *PVOLUME_DEVICE_OBJECT;


#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#define TAG_CCB TAG('V', 'C', 'C', 'B')
#define TAG_FCB TAG('V', 'F', 'C', 'B')
#define TAG_IRP TAG('V', 'I', 'R', 'P')
#define TAG_VFAT TAG('V', 'F', 'A', 'T')

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
   USHORT Year:5;
}
DOSDATE, *PDOSDATE;

#define IRPCONTEXT_CANWAIT          0x0001
#define IRPCONTEXT_PENDINGRETURNED  0x0002
#define IRPCONTEXT_STACK_IO_CONTEXT 0x0004

typedef struct _FAT_IRP_CONTEXT
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   UCHAR MajorFunction;
   UCHAR MinorFunction;
   PFILE_OBJECT FileObject;
   ULONG Flags;
   PVCB Vcb;
   ULONG PinCount;
   struct _FAT_IO_CONTEXT *FatIoContext;

   PDEVICE_EXTENSION DeviceExt;
   WORK_QUEUE_ITEM WorkQueueItem;
   PIO_STACK_LOCATION Stack;
   KEVENT Event;
} FAT_IRP_CONTEXT, *PFAT_IRP_CONTEXT;

typedef struct _FAT_IO_CONTEXT
{
    PMDL ZeroMdl;
} _FAT_IO_CONTEXT, *PFAT_IO_CONTEXT;

/*  ------------------------------------------------------  shutdown.c  */

DRIVER_DISPATCH FatShutdown;
NTSTATUS NTAPI
FatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  --------------------------------------------------------  volume.c  */

NTSTATUS NTAPI
FatQueryVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatSetVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------  blockdev.c  */

/*  -----------------------------------------------------------  dir.c  */

NTSTATUS NTAPI
FatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  --------------------------------------------------------  create.c  */

NTSTATUS NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  close.c  */

NTSTATUS NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  -------------------------------------------------------  cleanup.c  */

NTSTATUS NTAPI
FatCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fastio.c  */

VOID
FatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch);

BOOLEAN NTAPI
FatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromLazyWrite(IN PVOID Context);

BOOLEAN NTAPI
FatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromReadAhead(IN PVOID Context);

BOOLEAN NTAPI
FatNoopAcquire(IN PVOID Context,
               IN BOOLEAN Wait);

VOID NTAPI
FatNoopRelease(IN PVOID Context);

/* ---------------------------------------------------------  fastfat.c */

PFAT_IRP_CONTEXT NTAPI
FatBuildIrpContext(PIRP Irp, BOOLEAN CanWait);

VOID NTAPI
FatDestroyIrpContext(PFAT_IRP_CONTEXT IrpContext);

VOID NTAPI
FatCompleteRequest(PFAT_IRP_CONTEXT IrpContext OPTIONAL,
                   PIRP Irp OPTIONAL,
                   NTSTATUS Status);


/* ---------------------------------------------------------  lock.c */

NTSTATUS NTAPI
FatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fsctl.c  */

NTSTATUS NTAPI
FatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  finfo.c  */

NTSTATUS NTAPI FatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  iface.c  */

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

/*  -----------------------------------------------------------  fat.c  */

/*  ------------------------------------------------------  device.c  */

NTSTATUS NTAPI
FatDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------  direntry.c  */

/*  -----------------------------------------------------------  fcb.c  */

/*  ------------------------------------------------------------  rw.c  */

NTSTATUS NTAPI
FatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------------- flush.c  */

NTSTATUS NTAPI
FatFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);


/* EOF */
