#ifndef NTFS_H
#define NTFS_H

#include <ntifs.h>
#include <pseh/pseh2.h>

#define CACHEPAGESIZE(pDeviceExt) \
	((pDeviceExt)->NtfsInfo.UCHARsPerCluster > PAGE_SIZE ? \
	 (pDeviceExt)->NtfsInfo.UCHARsPerCluster : PAGE_SIZE)

#define TAG_NTFS 'SFTN'

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

#define DEVICE_NAME L"\\Ntfs"

#include <pshpack1.h>
typedef struct _BIOS_PARAMETERS_BLOCK
{
    USHORT    BytesPerSector;			// 0x0B
    UCHAR     SectorsPerCluster;		// 0x0D
    UCHAR     Unused0[7];				// 0x0E, checked when volume is mounted
    UCHAR     MediaId;				// 0x15
    UCHAR     Unused1[2];				// 0x16
    USHORT    SectorsPerTrack;		// 0x18
    USHORT    Heads;					// 0x1A
    UCHAR     Unused2[4];				// 0x1C
    UCHAR     Unused3[4];				// 0x20, checked when volume is mounted
} BIOS_PARAMETERS_BLOCK, *PBIOS_PARAMETERS_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETERS_BLOCK
{
    USHORT    Unknown[2];				// 0x24, always 80 00 80 00
    ULONGLONG SectorCount;			// 0x28
    ULONGLONG MftLocation;			// 0x30
    ULONGLONG MftMirrLocation;		// 0x38
    CHAR      ClustersPerMftRecord;	// 0x40
    UCHAR     Unused4[3];				// 0x41
    CHAR      ClustersPerIndexRecord; // 0x44
    UCHAR     Unused5[3];				// 0x45
    ULONGLONG SerialNumber;			// 0x48
    UCHAR     Checksum[4];			// 0x50
} EXTENDED_BIOS_PARAMETERS_BLOCK, *PEXTENDED_BIOS_PARAMETERS_BLOCK;

typedef struct _BOOT_SECTOR
{
    UCHAR     Jump[3];				// 0x00
    UCHAR     OEMID[8];				// 0x03
    BIOS_PARAMETERS_BLOCK BPB;
    EXTENDED_BIOS_PARAMETERS_BLOCK EBPB;
    UCHAR     BootStrap[426];			// 0x54
    USHORT    EndSector;				// 0x1FE
} BOOT_SECTOR, *PBOOT_SECTOR;
#include <poppack.h>

//typedef struct _BootSector BootSector;

typedef struct _NTFS_INFO
{
    ULONG BytesPerSector;
    ULONG SectorsPerCluster;
    ULONG BytesPerCluster;
    ULONGLONG SectorCount;
    ULONGLONG ClusterCount;
    ULARGE_INTEGER MftStart;
    ULARGE_INTEGER MftMirrStart;
    ULONG BytesPerFileRecord;
    ULONG BytesPerIndexRecord;

    ULONGLONG SerialNumber;
    USHORT VolumeLabelLength;
    WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH];
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT Flags;

    ULONG MftZoneReservation;
} NTFS_INFO, *PNTFS_INFO;

#define NTFS_TYPE_CCB         '20SF'
#define NTFS_TYPE_FCB         '30SF'
#define	NTFS_TYPE_VCB         '50SF'
#define NTFS_TYPE_IRP_CONTEST '60SF'
#define NTFS_TYPE_GLOBAL_DATA '70SF'

typedef struct
{
    ULONG Type;
    ULONG Size;
} NTFSIDENTIFIER, *PNTFSIDENTIFIER;

typedef struct
{
    NTFSIDENTIFIER Identifier;

    ERESOURCE DirResource;
//    ERESOURCE FatResource;

    KSPIN_LOCK FcbListLock;
    LIST_ENTRY FcbListHead;

    PVPB Vpb;
    PDEVICE_OBJECT StorageDevice;
    PFILE_OBJECT StreamFileObject;

    struct _NTFS_ATTR_CONTEXT* MFTContext;
    struct _FILE_RECORD_HEADER* MasterFileTable;
    struct _FCB *VolumeFcb;

    NTFS_INFO NtfsInfo;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION, NTFS_VCB, *PNTFS_VCB;

typedef struct
{
    NTFSIDENTIFIER Identifier;
    LIST_ENTRY     NextCCB;
    PFILE_OBJECT   PtrFileObject;
    LARGE_INTEGER  CurrentByteOffset;
    /* for DirectoryControl */
    ULONG Entry;
    /* for DirectoryControl */
    PWCHAR DirectorySearchPattern;
    ULONG LastCluster;
    ULONG LastOffset;
} NTFS_CCB, *PNTFS_CCB;

#define TAG_CCB 'BCCI'
#define TAG_FCB 'BCFI'

typedef struct
{
    NTFSIDENTIFIER Identifier;
    ERESOURCE Resource;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    CACHE_MANAGER_CALLBACKS CacheMgrCallbacks;
    ULONG Flags;
    FAST_IO_DISPATCH FastIoDispatch;
    NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST FcbLookasideList;
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
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd = 0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

#define NTFS_FILE_MFT                0
#define NTFS_FILE_MFTMIRR            1
#define NTFS_FILE_LOGFILE            2
#define NTFS_FILE_VOLUME            3
#define NTFS_FILE_ATTRDEF            4
#define NTFS_FILE_ROOT                5
#define NTFS_FILE_BITMAP            6
#define NTFS_FILE_BOOT                7
#define NTFS_FILE_BADCLUS            8
#define NTFS_FILE_QUOTA                9
#define NTFS_FILE_UPCASE            10
#define NTFS_FILE_EXTEND            11

#define NTFS_MFT_MASK 0x0000FFFFFFFFFFFFULL

#define COLLATION_BINARY              0x00
#define COLLATION_FILE_NAME           0x01
#define COLLATION_UNICODE_STRING      0x02
#define COLLATION_NTOFS_ULONG         0x10
#define COLLATION_NTOFS_SID           0x11
#define COLLATION_NTOFS_SECURITY_HASH 0x12
#define COLLATION_NTOFS_ULONGS        0x13

#define INDEX_ROOT_SMALL 0x0
#define INDEX_ROOT_LARGE 0x1

#define NTFS_INDEX_ENTRY_NODE            1
#define NTFS_INDEX_ENTRY_END            2

#define NTFS_FILE_NAME_POSIX            0
#define NTFS_FILE_NAME_WIN32            1
#define NTFS_FILE_NAME_DOS            2
#define NTFS_FILE_NAME_WIN32_AND_DOS        3

#define NTFS_FILE_TYPE_READ_ONLY  0x1
#define NTFS_FILE_TYPE_HIDDEN     0x2
#define NTFS_FILE_TYPE_SYSTEM     0x4
#define NTFS_FILE_TYPE_ARCHIVE    0x20
#define NTFS_FILE_TYPE_REPARSE    0x400
#define NTFS_FILE_TYPE_COMPRESSED 0x800
#define NTFS_FILE_TYPE_DIRECTORY  0x10000000

typedef struct
{
    ULONG Type;             /* Magic number 'FILE' */
    USHORT UsaOffset;       /* Offset to the update sequence */
    USHORT UsaCount;        /* Size in words of Update Sequence Number & Array (S) */
    ULONGLONG Lsn;          /* $LogFile Sequence Number (LSN) */
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

/* NTFS_RECORD_HEADER.Type */
#define NRH_FILE_TYPE  0x454C4946  /* 'FILE' */
#define NRH_INDX_TYPE  0x58444E49  /* 'INDX' */


typedef struct _FILE_RECORD_HEADER
{
    NTFS_RECORD_HEADER Ntfs;
    USHORT SequenceNumber;        /* Sequence number */
    USHORT LinkCount;             /* Hard link count */
    USHORT AttributeOffset;       /* Offset to the first Attribute */
    USHORT Flags;                 /* Flags */
    ULONG BytesInUse;             /* Real size of the FILE record */
    ULONG BytesAllocated;         /* Allocated size of the FILE record */
    ULONGLONG BaseFileRecord;     /* File reference to the base FILE record */
    USHORT NextAttributeNumber;   /* Next Attribute Id */
    USHORT Pading;                /* Align to 4 UCHAR boundary (XP) */
    ULONG MFTRecordNumber;        /* Number of this MFT Record (XP) */
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

/* Flags in FILE_RECORD_HEADER */

#define FRH_IN_USE    0x0001    /* Record is in use */
#define FRH_DIRECTORY 0x0002    /* Record is a directory */
#define FRH_UNKNOWN1  0x0004    /* Don't know */
#define FRH_UNKNOWN2  0x0008    /* Don't know */

typedef struct
{
    ULONG        Type;
    ULONG        Length;
    UCHAR        IsNonResident;
    UCHAR        NameLength;
    USHORT        NameOffset;
    USHORT        Flags;
    USHORT        Instance;
    union
    {
        // Resident attributes
        struct
        {
            ULONG        ValueLength;
            USHORT        ValueOffset;
            UCHAR        Flags;
            UCHAR        Reserved;
        } Resident;
        // Non-resident attributes
        struct
        {
            ULONGLONG        LowestVCN;
            ULONGLONG        HighestVCN;
            USHORT        MappingPairsOffset;
            USHORT        CompressionUnit;
            UCHAR        Reserved[4];
            LONGLONG        AllocatedSize;
            LONGLONG        DataSize;
            LONGLONG        InitializedSize;
            LONGLONG        CompressedSize;
        } NonResident;
    };
} NTFS_ATTR_RECORD, *PNTFS_ATTR_RECORD;

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
    union
    {
        struct
        {
            USHORT PackedEaSize;
            USHORT AlignmentOrReserved;
        } EaInfo;
        ULONG ReparseTag;
    } Extended;
    UCHAR NameLength;
    UCHAR NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct
{
    ULONG FirstEntryOffset;
    ULONG TotalSizeOfEntries;
    ULONG AllocatedSize;
    UCHAR Flags;
    UCHAR Padding[3];
} INDEX_HEADER_ATTRIBUTE, *PINDEX_HEADER_ATTRIBUTE;

typedef struct
{
    ULONG AttributeType;
    ULONG CollationRule;
    ULONG SizeOfEntry;
    UCHAR ClustersPerIndexRecord;
    UCHAR Padding[3];
    INDEX_HEADER_ATTRIBUTE Header;
} INDEX_ROOT_ATTRIBUTE, *PINDEX_ROOT_ATTRIBUTE;

typedef struct
{
    NTFS_RECORD_HEADER Ntfs;
    ULONGLONG VCN;
    INDEX_HEADER_ATTRIBUTE Header;
} INDEX_BUFFER, *PINDEX_BUFFER;

typedef struct
{
    union
    {
        struct
        {
            ULONGLONG    IndexedFile;
        } Directory;
        struct
        {
            USHORT    DataOffset;
            USHORT    DataLength;
            ULONG    Reserved;
        } ViewIndex;
    } Data;
    USHORT            Length;
    USHORT            KeyLength;
    USHORT            Flags;
    USHORT            Reserved;
    FILENAME_ATTRIBUTE    FileName;
} INDEX_ENTRY_ATTRIBUTE, *PINDEX_ENTRY_ATTRIBUTE;

typedef struct
{
    ULONGLONG Unknown1;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT Flags;
    ULONG Unknown2;
} VOLINFO_ATTRIBUTE, *PVOLINFO_ATTRIBUTE;

typedef struct {
    ULONG ReparseTag;
    USHORT DataLength;
    USHORT Reserved;
    UCHAR Data[1];
} REPARSE_POINT_ATTRIBUTE, *PREPARSE_POINT_ATTRIBUTE;

#define IRPCONTEXT_CANWAIT 0x1
#define IRPCONTEXT_COMPLETE 0x2
#define IRPCONTEXT_QUEUE 0x4

typedef struct
{
    NTFSIDENTIFIER Identifier;
    ULONG Flags;
    PIO_STACK_LOCATION Stack;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIRP Irp;
    BOOLEAN IsTopLevel;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    NTSTATUS SavedExceptionCode;
    CCHAR PriorityBoost;
} NTFS_IRP_CONTEXT, *PNTFS_IRP_CONTEXT;

typedef struct _NTFS_ATTR_CONTEXT
{
    PUCHAR            CacheRun;
    ULONGLONG            CacheRunOffset;
    LONGLONG            CacheRunStartLCN;
    ULONGLONG            CacheRunLength;
    LONGLONG            CacheRunLastLCN;
    ULONGLONG            CacheRunCurrentOffset;
    NTFS_ATTR_RECORD    Record;
} NTFS_ATTR_CONTEXT, *PNTFS_ATTR_CONTEXT;

#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_IS_VOLUME_STREAM    0x0002
#define FCB_IS_VOLUME           0x0004
#define MAX_PATH                260

typedef struct _FCB
{
    NTFSIDENTIFIER Identifier;

    FSRTL_COMMON_FCB_HEADER RFCB;
    SECTION_OBJECT_POINTERS SectionObjectPointers;

    PFILE_OBJECT FileObject;
    PNTFS_VCB Vcb;

    WCHAR *ObjectName;		/* point on filename (250 chars max) in PathName */
    WCHAR PathName[MAX_PATH];	/* path+filename 260 max */

    ERESOURCE PagingIoResource;
    ERESOURCE MainResource;

    LIST_ENTRY FcbListEntry;
    struct _FCB* ParentFcb;

    ULONG DirIndex;

    LONG RefCount;
    ULONG Flags;

    ULONGLONG MFTIndex;
    USHORT LinkCount;

    FILENAME_ATTRIBUTE Entry;

} NTFS_FCB, *PNTFS_FCB;

extern PNTFS_GLOBAL_DATA NtfsGlobalData;

FORCEINLINE
NTSTATUS
NtfsMarkIrpContextForQueue(PNTFS_IRP_CONTEXT IrpContext)
{
    PULONG Flags = &IrpContext->Flags;

    *Flags &= ~IRPCONTEXT_COMPLETE;
    *Flags |= IRPCONTEXT_QUEUE;

    return STATUS_PENDING;
}

/* attrib.c */

//VOID
//NtfsDumpAttribute(PATTRIBUTE Attribute);

PUCHAR
DecodeRun(PUCHAR DataRun,
          LONGLONG *DataRunOffset,
          ULONGLONG *DataRunLength);

VOID
NtfsDumpFileAttributes(PFILE_RECORD_HEADER FileRecord);

PSTANDARD_INFORMATION
GetStandardInformationFromRecord(PFILE_RECORD_HEADER FileRecord);

PFILENAME_ATTRIBUTE
GetFileNameFromRecord(PFILE_RECORD_HEADER FileRecord, UCHAR NameType);

PFILENAME_ATTRIBUTE
GetBestFileNameFromRecord(PFILE_RECORD_HEADER FileRecord);

/* blockdev.c */

NTSTATUS
NtfsReadDisk(IN PDEVICE_OBJECT DeviceObject,
             IN LONGLONG StartingOffset,
             IN ULONG Length,
             IN ULONG SectorSize,
             IN OUT PUCHAR Buffer,
             IN BOOLEAN Override);

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG DiskSector,
                IN ULONG SectorCount,
                IN ULONG SectorSize,
                IN OUT PUCHAR Buffer,
                IN BOOLEAN Override);

NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
                    IN ULONG ControlCode,
                    IN PVOID InputBuffer,
                    IN ULONG InputBufferSize,
                    IN OUT PVOID OutputBuffer,
                    IN OUT PULONG OutputBufferSize,
                    IN BOOLEAN Override);


/* close.c */

NTSTATUS
NtfsCloseFile(PDEVICE_EXTENSION DeviceExt,
              PFILE_OBJECT FileObject);

NTSTATUS
NtfsClose(PNTFS_IRP_CONTEXT IrpContext);


/* create.c */

NTSTATUS
NtfsCreate(PNTFS_IRP_CONTEXT IrpContext);


/* devctl.c */

NTSTATUS
NtfsDeviceControl(PNTFS_IRP_CONTEXT IrpContext);


/* dirctl.c */

NTSTATUS
NtfsDirectoryControl(PNTFS_IRP_CONTEXT IrpContext);


/* dispatch.c */

DRIVER_DISPATCH NtfsFsdDispatch;
NTSTATUS NTAPI
NtfsFsdDispatch(PDEVICE_OBJECT DeviceObject,
                PIRP Irp);


/* fastio.c */

BOOLEAN NTAPI
NtfsAcqLazyWrite(PVOID Context,
                 BOOLEAN Wait);

VOID NTAPI
NtfsRelLazyWrite(PVOID Context);

BOOLEAN NTAPI
NtfsAcqReadAhead(PVOID Context,
                 BOOLEAN Wait);

VOID NTAPI
NtfsRelReadAhead(PVOID Context);

FAST_IO_CHECK_IF_POSSIBLE NtfsFastIoCheckIfPossible;
FAST_IO_READ NtfsFastIoRead;
FAST_IO_WRITE NtfsFastIoWrite;


/* fcb.c */

PNTFS_FCB
NtfsCreateFCB(PCWSTR FileName,
              PNTFS_VCB Vcb);

VOID
NtfsDestroyFCB(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsDirectory(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsReparsePoint(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsRoot(PNTFS_FCB Fcb);

VOID
NtfsGrabFCB(PNTFS_VCB Vcb,
            PNTFS_FCB Fcb);

VOID
NtfsReleaseFCB(PNTFS_VCB Vcb,
               PNTFS_FCB Fcb);

VOID
NtfsAddFCBToTable(PNTFS_VCB Vcb,
                  PNTFS_FCB Fcb);

PNTFS_FCB
NtfsGrabFCBFromTable(PNTFS_VCB Vcb,
                     PCWSTR FileName);

NTSTATUS
NtfsFCBInitializeCache(PNTFS_VCB Vcb,
                       PNTFS_FCB Fcb);

PNTFS_FCB
NtfsMakeRootFCB(PNTFS_VCB Vcb);

PNTFS_FCB
NtfsOpenRootFCB(PNTFS_VCB Vcb);

NTSTATUS
NtfsAttachFCBToFileObject(PNTFS_VCB Vcb,
                          PNTFS_FCB Fcb,
                          PFILE_OBJECT FileObject);

NTSTATUS
NtfsGetFCBForFile(PNTFS_VCB Vcb,
                  PNTFS_FCB *pParentFCB,
                  PNTFS_FCB *pFCB,
                  const PWSTR pFileName);

NTSTATUS
NtfsReadFCBAttribute(PNTFS_VCB Vcb,
                     PNTFS_FCB pFCB,
                     ULONG Type, 
                     PCWSTR Name,
                     ULONG NameLength,
                     PVOID * Data);

NTSTATUS
NtfsMakeFCBFromDirEntry(PNTFS_VCB Vcb,
                        PNTFS_FCB DirectoryFCB,
                        PUNICODE_STRING Name,
                        PFILE_RECORD_HEADER Record,
                        ULONGLONG MFTIndex,
                        PNTFS_FCB * fileFCB);


/* finfo.c */

NTSTATUS
NtfsQueryInformation(PNTFS_IRP_CONTEXT IrpContext);


/* fsctl.c */

NTSTATUS
NtfsFileSystemControl(PNTFS_IRP_CONTEXT IrpContext);


/* mft.c */
VOID
ReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context);

ULONG
ReadAttribute(PDEVICE_EXTENSION Vcb,
              PNTFS_ATTR_CONTEXT Context,
              ULONGLONG Offset,
              PCHAR Buffer,
              ULONG Length);

ULONGLONG
AttributeDataLength(PNTFS_ATTR_RECORD AttrRecord);

ULONG
AttributeAllocatedLength(PNTFS_ATTR_RECORD AttrRecord);

NTSTATUS
ReadFileRecord(PDEVICE_EXTENSION Vcb,
               ULONGLONG index,
               PFILE_RECORD_HEADER file);

NTSTATUS
FindAttribute(PDEVICE_EXTENSION Vcb,
              PFILE_RECORD_HEADER MftRecord,
              ULONG Type,
              PCWSTR Name,
              ULONG NameLength,
              PNTFS_ATTR_CONTEXT * AttrCtx);

VOID
ReadVCN(PDEVICE_EXTENSION Vcb,
        PFILE_RECORD_HEADER file,
        ATTRIBUTE_TYPE type,
        ULONGLONG vcn,
        ULONG count,
        PVOID buffer);

NTSTATUS 
FixupUpdateSequenceArray(PDEVICE_EXTENSION Vcb,
                         PNTFS_RECORD_HEADER Record);

NTSTATUS
ReadLCN(PDEVICE_EXTENSION Vcb,
        ULONGLONG lcn,
        ULONG count,
        PVOID buffer);

VOID
EnumerAttribute(PFILE_RECORD_HEADER file,
                PDEVICE_EXTENSION Vcb,
                PDEVICE_OBJECT DeviceObject);

NTSTATUS
NtfsLookupFile(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING PathName,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex);

NTSTATUS
NtfsLookupFileAt(PDEVICE_EXTENSION Vcb,
                 PUNICODE_STRING PathName,
                 PFILE_RECORD_HEADER *FileRecord,
                 PULONGLONG MFTIndex,
                 ULONGLONG CurrentMFTIndex);

NTSTATUS
NtfsFindFileAt(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING SearchPattern,
               PULONG FirstEntry,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex,
               ULONGLONG CurrentMFTIndex);

/* misc.c */

BOOLEAN
NtfsIsIrpTopLevel(PIRP Irp);

PNTFS_IRP_CONTEXT
NtfsAllocateIrpContext(PDEVICE_OBJECT DeviceObject,
                       PIRP Irp);

PVOID
NtfsGetUserBuffer(PIRP Irp,
                  BOOLEAN Paging);

#if 0
BOOLEAN
wstrcmpjoki(PWSTR s1, PWSTR s2);

VOID
CdfsSwapString(PWCHAR Out,
	       PUCHAR In,
	       ULONG Count);
#endif

VOID
NtfsFileFlagsToAttributes(ULONG NtfsAttributes,
                          PULONG FileAttributes);


/* rw.c */

NTSTATUS
NtfsRead(PNTFS_IRP_CONTEXT IrpContext);

NTSTATUS
NtfsWrite(PNTFS_IRP_CONTEXT IrpContext);


/* volinfo.c */

ULONGLONG
NtfsGetFreeClusters(PDEVICE_EXTENSION DeviceExt);

NTSTATUS
NtfsQueryVolumeInformation(PNTFS_IRP_CONTEXT IrpContext);

NTSTATUS
NtfsSetVolumeInformation(PNTFS_IRP_CONTEXT IrpContext);


/* ntfs.c */

DRIVER_INITIALIZE DriverEntry;

VOID
NTAPI
NtfsInitializeFunctionPointers(PDRIVER_OBJECT DriverObject);

#endif /* NTFS_H */
