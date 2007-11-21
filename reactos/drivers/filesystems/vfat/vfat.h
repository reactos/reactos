#include <debug.h>
#include <ntifs.h>
#include <ntdddisk.h>
#include <reactos/helper.h>

#ifdef __GNUC__
#include <ccros.h>

#define USE_ROS_CC_AND_FS
#else
#define KEBUGCHECK KeBugCheck
#define KEBUGCHECKEX KeBugCheckEx
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))
#define ROUND_UP(N, S) ROUND_DOWN((N) + (S) - 1, (S))
#endif

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

#define VFAT_CASE_LOWER_BASE	8  		// base is lower case
#define VFAT_CASE_LOWER_EXT 	16 		// extension is lower case

#define LONGNAME_MAX_LENGTH 	256		// max length for a long filename

#define ENTRY_DELETED(DeviceExt, DirEntry) ((DeviceExt)->Flags & VCB_IS_FATX ? FATX_ENTRY_DELETED(&((DirEntry)->FatX)) : FAT_ENTRY_DELETED(&((DirEntry)->Fat)))
#define ENTRY_VOLUME(DeviceExt, DirEntry)  ((DeviceExt)->Flags & VCB_IS_FATX ? FATX_ENTRY_VOLUME(&((DirEntry)->FatX)) : FAT_ENTRY_VOLUME(&((DirEntry)->Fat)))
#define ENTRY_END(DeviceExt, DirEntry)     ((DeviceExt)->Flags & VCB_IS_FATX ? FATX_ENTRY_END(&((DirEntry)->FatX)) : FAT_ENTRY_END(&((DirEntry)->Fat)))

#define FAT_ENTRY_DELETED(DirEntry)  ((DirEntry)->Filename[0] == 0xe5)
#define FAT_ENTRY_END(DirEntry)      ((DirEntry)->Filename[0] == 0)
#define FAT_ENTRY_LONG(DirEntry)     (((DirEntry)->Attrib & 0x3f) == 0x0f)
#define FAT_ENTRY_VOLUME(DirEntry)   (((DirEntry)->Attrib & 0x1f) == 0x08)

#define FATX_ENTRY_DELETED(DirEntry) ((DirEntry)->FilenameLength == 0xe5)
#define FATX_ENTRY_END(DirEntry)     ((DirEntry)->FilenameLength == 0xff)
#define FATX_ENTRY_LONG(DirEntry)    (FALSE)
#define FATX_ENTRY_VOLUME(DirEntry)  (((DirEntry)->Attrib & 0x1f) == 0x08)

#define FAT_ENTRIES_PER_PAGE   (PAGE_SIZE / sizeof (FAT_DIR_ENTRY))
#define FATX_ENTRIES_PER_PAGE  (PAGE_SIZE / sizeof (FATX_DIR_ENTRY))

typedef struct _FATXDirEntry FATX_DIR_ENTRY, *PFATX_DIR_ENTRY;

union _DIR_ENTRY
{
   FAT_DIR_ENTRY Fat;
   FATX_DIR_ENTRY FatX;
};

typedef union _DIR_ENTRY DIR_ENTRY, *PDIR_ENTRY;

#define BLOCKSIZE 512

#define FAT16  (1)
#define FAT12  (2)
#define FAT32  (3)
#define FATX16 (4)
#define FATX32 (5)

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

typedef struct _HASHENTRY
{
  ULONG Hash;
  struct _VFATFCB* self;
  struct _HASHENTRY* next;
}
HASHENTRY;

#define FCB_HASH_TABLE_SIZE 65536

typedef struct DEVICE_EXTENSION *PDEVICE_EXTENSION;

typedef NTSTATUS (*PGET_NEXT_CLUSTER)(PDEVICE_EXTENSION,ULONG,PULONG);
typedef NTSTATUS (*PFIND_AND_MARK_AVAILABLE_CLUSTER)(PDEVICE_EXTENSION,PULONG);
typedef NTSTATUS (*PWRITE_CLUSTER)(PDEVICE_EXTENSION,ULONG,ULONG,PULONG);

typedef NTSTATUS (*PGET_NEXT_DIR_ENTRY)(PVOID*,PVOID*,struct _VFATFCB*,struct _VFAT_DIRENTRY_CONTEXT*,BOOLEAN);

typedef struct DEVICE_EXTENSION
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

  /* Pointers to functions for manipulating FAT. */
  PGET_NEXT_CLUSTER GetNextCluster;
  PFIND_AND_MARK_AVAILABLE_CLUSTER FindAndMarkAvailableCluster;
  PWRITE_CLUSTER WriteCluster;
  ULONG CleanShutBitMask;

  /* Pointers to functions for manipulating directory entries. */
  PGET_NEXT_DIR_ENTRY GetNextDirEntry;

  ULONG BaseDateYear;

  LIST_ENTRY VolumeListEntry;
} DEVICE_EXTENSION, VCB, *PVCB;

typedef struct
{
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT DeviceObject;
  ULONG Flags;
  ERESOURCE VolumeListLock;
  LIST_ENTRY VolumeListHead;
  NPAGED_LOOKASIDE_LIST FcbLookasideList;
  NPAGED_LOOKASIDE_LIST CcbLookasideList;
  NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
  FAST_IO_DISPATCH FastIoDispatch;
  CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
} VFAT_GLOBAL_DATA, *PVFAT_GLOBAL_DATA;

extern PVFAT_GLOBAL_DATA VfatGlobalData;

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

  /* directory entry for this file or directory */
  DIR_ENTRY entry;

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

  /* Entry into the hash table for the path + long name */
  HASHENTRY Hash;

  /* Entry into the hash table for the path + short name */
  HASHENTRY ShortHash;

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

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#define TAG_CCB TAG('V', 'C', 'C', 'B')
#define TAG_FCB TAG('V', 'F', 'C', 'B')
#define TAG_IRP TAG('V', 'I', 'R', 'P')
#define TAG_VFAT TAG('V', 'F', 'A', 'T')

#define ENTRIES_PER_SECTOR (BLOCKSIZE / sizeof(FATDirEntry))

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

#define IRPCONTEXT_CANWAIT	    0x0001
#define IRPCONTEXT_PENDINGRETURNED  0x0002

typedef struct
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PDEVICE_EXTENSION DeviceExt;
   ULONG Flags;
   WORK_QUEUE_ITEM WorkQueueItem;
   PIO_STACK_LOCATION Stack;
   UCHAR MajorFunction;
   UCHAR MinorFunction;
   PFILE_OBJECT FileObject;
   ULONG RefCount;
   KEVENT Event;
} VFAT_IRP_CONTEXT, *PVFAT_IRP_CONTEXT;

typedef struct _VFAT_DIRENTRY_CONTEXT
{
  ULONG StartIndex;
  ULONG DirIndex;
  DIR_ENTRY DirEntry;
  UNICODE_STRING LongNameU;
  UNICODE_STRING ShortNameU;
} VFAT_DIRENTRY_CONTEXT, *PVFAT_DIRENTRY_CONTEXT;


/*  ------------------------------------------------------  shutdown.c  */

DRIVER_DISPATCH VfatShutdown;
NTSTATUS NTAPI VfatShutdown (PDEVICE_OBJECT DeviceObject,
                               PIRP Irp);

/*  --------------------------------------------------------  volume.c  */

NTSTATUS VfatQueryVolumeInformation (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS VfatSetVolumeInformation (PVFAT_IRP_CONTEXT IrpContext);

/*  ------------------------------------------------------  blockdev.c  */

NTSTATUS VfatReadDisk(IN PDEVICE_OBJECT pDeviceObject,
                      IN PLARGE_INTEGER ReadOffset,
                      IN ULONG ReadLength,
                      IN PUCHAR Buffer,
                      IN BOOLEAN Override);

NTSTATUS VfatReadDiskPartial (IN PVFAT_IRP_CONTEXT IrpContext,
			      IN PLARGE_INTEGER ReadOffset,
			      IN ULONG ReadLength,
			      IN ULONG BufferOffset,
			      IN BOOLEAN Wait);

NTSTATUS VfatWriteDiskPartial(IN PVFAT_IRP_CONTEXT IrpContext,
			      IN PLARGE_INTEGER WriteOffset,
			      IN ULONG WriteLength,
			      IN ULONG BufferOffset,
			      IN BOOLEAN Wait);

NTSTATUS VfatBlockDeviceIoControl (IN PDEVICE_OBJECT DeviceObject,
				   IN ULONG CtlCode,
				   IN PVOID InputBuffer,
				   IN ULONG InputBufferSize,
				   IN OUT PVOID OutputBuffer,
				   IN OUT PULONG pOutputBufferSize,
				   IN BOOLEAN Override);

/*  -----------------------------------------------------------  dir.c  */

NTSTATUS VfatDirectoryControl (PVFAT_IRP_CONTEXT);

BOOLEAN FsdDosDateTimeToSystemTime (PDEVICE_EXTENSION DeviceExt,
                                    USHORT DosDate,
                                    USHORT DosTime,
                                    PLARGE_INTEGER SystemTime);

BOOLEAN FsdSystemTimeToDosDateTime (PDEVICE_EXTENSION DeviceExt,
                                    PLARGE_INTEGER SystemTime,
                                    USHORT *pDosDate,
                                    USHORT *pDosTime);

/*  --------------------------------------------------------  create.c  */

NTSTATUS VfatCreate (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS FindFile (PDEVICE_EXTENSION DeviceExt,
                   PVFATFCB Parent,
                   PUNICODE_STRING FileToFindU,
		   PVFAT_DIRENTRY_CONTEXT DirContext,
		   BOOLEAN First);

VOID vfat8Dot3ToString (PFAT_DIR_ENTRY pEntry,
                        PUNICODE_STRING NameU);

NTSTATUS ReadVolumeLabel(PDEVICE_EXTENSION DeviceExt,
                         PVPB Vpb);

/*  ---------------------------------------------------------  close.c  */

NTSTATUS VfatClose (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS VfatCloseFile(PDEVICE_EXTENSION DeviceExt,
                       PFILE_OBJECT FileObject);

/*  -------------------------------------------------------  cleanup.c  */

NTSTATUS VfatCleanup (PVFAT_IRP_CONTEXT IrpContext);

/*  ---------------------------------------------------------  fastio.c  */

VOID
VfatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch);

BOOLEAN NTAPI
VfatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
VfatReleaseFromLazyWrite(IN PVOID Context);

BOOLEAN NTAPI
VfatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
VfatReleaseFromReadAhead(IN PVOID Context);

/*  ---------------------------------------------------------  fsctl.c  */

NTSTATUS VfatFileSystemControl (PVFAT_IRP_CONTEXT IrpContext);

/*  ---------------------------------------------------------  finfo.c  */

NTSTATUS VfatQueryInformation (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS VfatSetInformation (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS
VfatSetAllocationSizeInformation(PFILE_OBJECT FileObject,
				 PVFATFCB Fcb,
				 PDEVICE_EXTENSION DeviceExt,
				 PLARGE_INTEGER AllocationSize);

/*  ---------------------------------------------------------  iface.c  */

NTSTATUS NTAPI DriverEntry (PDRIVER_OBJECT DriverObject,
                              PUNICODE_STRING RegistryPath);

/*  ---------------------------------------------------------  dirwr.c  */

NTSTATUS VfatAddEntry (PDEVICE_EXTENSION DeviceExt,
		       PUNICODE_STRING PathNameU,
		       PVFATFCB* Fcb,
		       PVFATFCB ParentFcb,
		       ULONG RequestedOptions,
		       UCHAR ReqAttr);

NTSTATUS VfatUpdateEntry (PVFATFCB pFcb);

NTSTATUS VfatDelEntry(PDEVICE_EXTENSION, PVFATFCB);

BOOLEAN
vfatFindDirSpace(PDEVICE_EXTENSION DeviceExt,
                 PVFATFCB pDirFcb,
                 ULONG nbSlots,
                 PULONG start);

/*  --------------------------------------------------------  string.c  */

VOID
vfatSplitPathName(PUNICODE_STRING PathNameU,
		  PUNICODE_STRING DirNameU,
		  PUNICODE_STRING FileNameU);

BOOLEAN vfatIsLongIllegal(WCHAR c);

BOOLEAN wstrcmpjoki (PWSTR s1,
                     PWSTR s2);

/*  -----------------------------------------------------------  fat.c  */

NTSTATUS FAT12GetNextCluster(PDEVICE_EXTENSION DeviceExt,
                             ULONG CurrentCluster,
                             PULONG NextCluster);

NTSTATUS FAT12FindAndMarkAvailableCluster(PDEVICE_EXTENSION DeviceExt,
                                          PULONG Cluster);

NTSTATUS FAT12WriteCluster(PDEVICE_EXTENSION DeviceExt,
                           ULONG ClusterToWrite,
                           ULONG NewValue,
                           PULONG OldValue);

NTSTATUS FAT16GetNextCluster(PDEVICE_EXTENSION DeviceExt,
                             ULONG CurrentCluster,
                             PULONG NextCluster);

NTSTATUS FAT16FindAndMarkAvailableCluster(PDEVICE_EXTENSION DeviceExt,
                                          PULONG Cluster);

NTSTATUS FAT16WriteCluster(PDEVICE_EXTENSION DeviceExt,
                           ULONG ClusterToWrite,
                           ULONG NewValue,
                           PULONG OldValue);

NTSTATUS FAT32GetNextCluster(PDEVICE_EXTENSION DeviceExt,
                             ULONG CurrentCluster,
                             PULONG NextCluster);

NTSTATUS FAT32FindAndMarkAvailableCluster(PDEVICE_EXTENSION DeviceExt,
                                          PULONG Cluster);

NTSTATUS FAT32WriteCluster(PDEVICE_EXTENSION DeviceExt,
                           ULONG ClusterToWrite,
                           ULONG NewValue,
                           PULONG OldValue);

NTSTATUS OffsetToCluster (PDEVICE_EXTENSION DeviceExt,
                          ULONG FirstCluster,
                          ULONG FileOffset,
                          PULONG Cluster,
                          BOOLEAN Extend);

ULONGLONG ClusterToSector (PDEVICE_EXTENSION DeviceExt,
			   ULONG Cluster);

NTSTATUS GetNextCluster (PDEVICE_EXTENSION DeviceExt,
                         ULONG CurrentCluster,
                         PULONG NextCluster);

NTSTATUS GetNextClusterExtend (PDEVICE_EXTENSION DeviceExt,
	                       ULONG CurrentCluster,
	                       PULONG NextCluster);

NTSTATUS CountAvailableClusters (PDEVICE_EXTENSION DeviceExt,
                                 PLARGE_INTEGER Clusters);

NTSTATUS
WriteCluster(PDEVICE_EXTENSION DeviceExt,
	     ULONG ClusterToWrite,
	     ULONG NewValue);

/*  ------------------------------------------------------  direntry.c  */

ULONG  vfatDirEntryGetFirstCluster (PDEVICE_EXTENSION  pDeviceExt,
                                    PDIR_ENTRY  pDirEntry);

BOOLEAN VfatIsDirectoryEmpty(PVFATFCB Fcb);

NTSTATUS FATGetNextDirEntry(PVOID * pContext,
			     PVOID * pPage,
			     IN PVFATFCB pDirFcb,
			     IN PVFAT_DIRENTRY_CONTEXT DirContext,
			     BOOLEAN First);

NTSTATUS FATXGetNextDirEntry(PVOID * pContext,
			     PVOID * pPage,
			     IN PVFATFCB pDirFcb,
			     IN PVFAT_DIRENTRY_CONTEXT DirContext,
			     BOOLEAN First);

/*  -----------------------------------------------------------  fcb.c  */

PVFATFCB vfatNewFCB (PDEVICE_EXTENSION  pVCB,
                        PUNICODE_STRING pFileNameU);

VOID vfatDestroyFCB (PVFATFCB  pFCB);

VOID vfatDestroyCCB(PVFATCCB pCcb);

VOID vfatGrabFCB (PDEVICE_EXTENSION  pVCB,
                  PVFATFCB  pFCB);

VOID vfatReleaseFCB (PDEVICE_EXTENSION  pVCB,
                     PVFATFCB  pFCB);

VOID vfatAddFCBToTable (PDEVICE_EXTENSION  pVCB,
                        PVFATFCB  pFCB);

PVFATFCB vfatGrabFCBFromTable (PDEVICE_EXTENSION  pDeviceExt,
                               PUNICODE_STRING  pFileNameU);

PVFATFCB vfatMakeRootFCB (PDEVICE_EXTENSION  pVCB);

PVFATFCB vfatOpenRootFCB (PDEVICE_EXTENSION  pVCB);

BOOLEAN vfatFCBIsDirectory (PVFATFCB FCB);

BOOLEAN vfatFCBIsRoot(PVFATFCB FCB);

NTSTATUS vfatAttachFCBToFileObject (PDEVICE_EXTENSION  vcb,
                                    PVFATFCB  fcb,
                                    PFILE_OBJECT  fileObject);

NTSTATUS vfatDirFindFile (PDEVICE_EXTENSION  pVCB,
                          PVFATFCB  parentFCB,
                          PUNICODE_STRING FileToFindU,
                          PVFATFCB * fileFCB);

NTSTATUS vfatGetFCBForFile (PDEVICE_EXTENSION  pVCB,
                            PVFATFCB  *pParentFCB,
                            PVFATFCB  *pFCB,
                            PUNICODE_STRING pFileNameU);

NTSTATUS vfatMakeFCBFromDirEntry (PVCB  vcb,
                                  PVFATFCB  directoryFCB,
				  PVFAT_DIRENTRY_CONTEXT DirContext,
                                  PVFATFCB * fileFCB);

/*  ------------------------------------------------------------  rw.c  */

NTSTATUS VfatRead (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS VfatWrite (PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS NextCluster(PDEVICE_EXTENSION DeviceExt,
                     ULONG FirstCluster,
                     PULONG CurrentCluster,
                     BOOLEAN Extend);

/*  -----------------------------------------------------------  misc.c  */

NTSTATUS VfatQueueRequest(PVFAT_IRP_CONTEXT IrpContext);

PVFAT_IRP_CONTEXT VfatAllocateIrpContext(PDEVICE_OBJECT DeviceObject,
                                         PIRP Irp);

VOID VfatFreeIrpContext(PVFAT_IRP_CONTEXT IrpContext);

DRIVER_DISPATCH VfatBuildRequest;
NTSTATUS NTAPI VfatBuildRequest (PDEVICE_OBJECT DeviceObject,
                                   PIRP Irp);

PVOID VfatGetUserBuffer(IN PIRP);

NTSTATUS VfatLockUserBuffer(IN PIRP, IN ULONG,
                            IN LOCK_OPERATION);

NTSTATUS
VfatSetExtendedAttributes(PFILE_OBJECT FileObject,
			  PVOID Ea,
			  ULONG EaLength);
/*  ------------------------------------------------------------- flush.c  */

NTSTATUS VfatFlush(PVFAT_IRP_CONTEXT IrpContext);

NTSTATUS VfatFlushVolume(PDEVICE_EXTENSION DeviceExt, PVFATFCB VolumeFcb);


/* EOF */
