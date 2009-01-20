#include <ntifs.h>
#include <ntdddisk.h>
#include <reactos/helper.h>
#include <debug.h>

/* FAT on-disk data structures */
#include <fat.h>

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

extern FAT_GLOBAL_DATA FatGlobalData;

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
