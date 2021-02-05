#ifndef NTFS_H
#define NTFS_H

#include <ntifs.h>
#include <pseh/pseh2.h>
#include <section_attribs.h>

#define CACHEPAGESIZE(pDeviceExt) \
	((pDeviceExt)->NtfsInfo.UCHARsPerCluster > PAGE_SIZE ? \
	 (pDeviceExt)->NtfsInfo.UCHARsPerCluster : PAGE_SIZE)

#define TAG_NTFS '0ftN'
#define TAG_CCB 'CftN'
#define TAG_FCB 'FftN'
#define TAG_IRP_CTXT 'iftN'
#define TAG_ATT_CTXT 'aftN'
#define TAG_FILE_REC 'rftN'

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
#define NTFS_TYPE_VCB         '50SF'
#define NTFS_TYPE_IRP_CONTEXT '60SF'
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

    NPAGED_LOOKASIDE_LIST FileRecLookasideList;

    ULONG MftDataOffset;
    ULONG Flags;
    ULONG OpenHandleCount;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION, NTFS_VCB, *PNTFS_VCB;

#define VCB_VOLUME_LOCKED       0x0001

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
    NPAGED_LOOKASIDE_LIST AttrCtxtLookasideList;
    BOOLEAN EnableWriteSupport;
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

// FILE_RECORD_END seems to follow AttributeEnd in every file record starting with $Quota.
// No clue what data is being represented here.
#define FILE_RECORD_END              0x11477982 

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
#define NTFS_FILE_FIRST_USER_FILE   16

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

#define INDEX_NODE_SMALL 0x0
#define INDEX_NODE_LARGE 0x1

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

/* Indexed Flag in Resident attributes - still somewhat speculative */
#define RA_INDEXED    0x01

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
    USHORT Padding;               /* Align to 4 UCHAR boundary (XP) */
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
    ULONG Type;
    USHORT Length;
    UCHAR NameLength;
    UCHAR NameOffset;
    ULONGLONG StartingVCN;
    ULONGLONG MFTIndex;
    USHORT Instance;
} NTFS_ATTRIBUTE_LIST_ITEM, *PNTFS_ATTRIBUTE_LIST_ITEM;

// The beginning and length of an attribute record are always aligned to an 8-byte boundary,
// relative to the beginning of the file record.
#define ATTR_RECORD_ALIGNMENT 8

// Data runs are aligned to a 4-byte boundary, relative to the start of the attribute record
#define DATA_RUN_ALIGNMENT  4

// Value offset is aligned to a 4-byte boundary, relative to the start of the attribute record
#define VALUE_OFFSET_ALIGNMENT  4

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

struct _B_TREE_FILENAME_NODE;
typedef struct _B_TREE_FILENAME_NODE B_TREE_FILENAME_NODE;

// Keys are arranged in nodes as an ordered, linked list
typedef struct _B_TREE_KEY
{
    struct _B_TREE_KEY *NextKey;
    B_TREE_FILENAME_NODE *LesserChild;  // Child-Node. All the keys in this node will be sorted before IndexEntry
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;  // must be last member for FIELD_OFFSET
}B_TREE_KEY, *PB_TREE_KEY;

// Every Node is just an ordered list of keys.
// Sub-nodes can be found attached to a key (if they exist).
// A key's sub-node precedes that key in the ordered list.
typedef struct _B_TREE_FILENAME_NODE
{
    ULONG KeyCount;
    BOOLEAN HasValidVCN;
    BOOLEAN DiskNeedsUpdating;
    ULONGLONG VCN;
    PB_TREE_KEY FirstKey;
} B_TREE_FILENAME_NODE, *PB_TREE_FILENAME_NODE;

typedef struct
{
    PB_TREE_FILENAME_NODE RootNode;
} B_TREE, *PB_TREE;

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
    LARGE_MCB           DataRunsMCB;
    ULONGLONG           FileMFTIndex;
    ULONGLONG           FileOwnerMFTIndex; /* If attribute list attribute, reference the original file */
    PNTFS_ATTR_RECORD    pRecord;
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

    WCHAR Stream[MAX_PATH];
    WCHAR *ObjectName;		/* point on filename (250 chars max) in PathName */
    WCHAR PathName[MAX_PATH];	/* path+filename 260 max */

    ERESOURCE PagingIoResource;
    ERESOURCE MainResource;

    LIST_ENTRY FcbListEntry;
    struct _FCB* ParentFcb;

    ULONG DirIndex;

    LONG RefCount;
    ULONG Flags;
    ULONG OpenHandleCount;

    ULONGLONG MFTIndex;
    USHORT LinkCount;

    FILENAME_ATTRIBUTE Entry;

} NTFS_FCB, *PNTFS_FCB;

typedef struct _FIND_ATTR_CONTXT
{
    PDEVICE_EXTENSION Vcb;
    BOOLEAN OnlyResident;
    PNTFS_ATTR_RECORD FirstAttr;
    PNTFS_ATTR_RECORD CurrAttr;
    PNTFS_ATTR_RECORD LastAttr;
    PNTFS_ATTRIBUTE_LIST_ITEM NonResidentStart;
    PNTFS_ATTRIBUTE_LIST_ITEM NonResidentEnd;
    PNTFS_ATTRIBUTE_LIST_ITEM NonResidentCur;
    ULONG Offset;
} FIND_ATTR_CONTXT, *PFIND_ATTR_CONTXT;

typedef struct
{
    USHORT USN;
    USHORT Array[];
} FIXUP_ARRAY, *PFIXUP_ARRAY;

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

NTSTATUS
AddBitmap(PNTFS_VCB Vcb,
          PFILE_RECORD_HEADER FileRecord,
          PNTFS_ATTR_RECORD AttributeAddress,
          PCWSTR Name,
          USHORT NameLength);

NTSTATUS
AddData(PFILE_RECORD_HEADER FileRecord,
        PNTFS_ATTR_RECORD AttributeAddress);

NTSTATUS
AddRun(PNTFS_VCB Vcb,
       PNTFS_ATTR_CONTEXT AttrContext,
       ULONG AttrOffset,
       PFILE_RECORD_HEADER FileRecord,
       ULONGLONG NextAssignedCluster,
       ULONG RunLength);

NTSTATUS
AddIndexAllocation(PNTFS_VCB Vcb,
                   PFILE_RECORD_HEADER FileRecord,
                   PNTFS_ATTR_RECORD AttributeAddress,
                   PCWSTR Name,
                   USHORT NameLength);

NTSTATUS
AddIndexRoot(PNTFS_VCB Vcb,
             PFILE_RECORD_HEADER FileRecord,
             PNTFS_ATTR_RECORD AttributeAddress,
             PINDEX_ROOT_ATTRIBUTE NewIndexRoot,
             ULONG RootLength,
             PCWSTR Name,
             USHORT NameLength);

NTSTATUS
AddFileName(PFILE_RECORD_HEADER FileRecord,
            PNTFS_ATTR_RECORD AttributeAddress,
            PDEVICE_EXTENSION DeviceExt,
            PFILE_OBJECT FileObject,
            BOOLEAN CaseSensitive,
            PULONGLONG ParentMftIndex);

NTSTATUS
AddStandardInformation(PFILE_RECORD_HEADER FileRecord,
                       PNTFS_ATTR_RECORD AttributeAddress);

NTSTATUS
ConvertDataRunsToLargeMCB(PUCHAR DataRun,
                          PLARGE_MCB DataRunsMCB,
                          PULONGLONG pNextVBN);

NTSTATUS
ConvertLargeMCBToDataRuns(PLARGE_MCB DataRunsMCB,
                          PUCHAR RunBuffer,
                          ULONG MaxBufferSize,
                          PULONG UsedBufferSize);

PUCHAR
DecodeRun(PUCHAR DataRun,
          LONGLONG *DataRunOffset,
          ULONGLONG *DataRunLength);

ULONG GetFileNameAttributeLength(PFILENAME_ATTRIBUTE FileNameAttribute);

VOID
NtfsDumpDataRuns(PVOID StartOfRun,
                 ULONGLONG CurrentLCN);

VOID
NtfsDumpFileAttributes(PDEVICE_EXTENSION Vcb,
                       PFILE_RECORD_HEADER FileRecord);

PSTANDARD_INFORMATION
GetStandardInformationFromRecord(PDEVICE_EXTENSION Vcb,
                                 PFILE_RECORD_HEADER FileRecord);

PFILENAME_ATTRIBUTE
GetFileNameFromRecord(PDEVICE_EXTENSION Vcb,
                      PFILE_RECORD_HEADER FileRecord,
                      UCHAR NameType);

UCHAR
GetPackedByteCount(LONGLONG NumberToPack,
                   BOOLEAN IsSigned);

NTSTATUS
GetLastClusterInDataRun(PDEVICE_EXTENSION Vcb,
                        PNTFS_ATTR_RECORD Attribute,
                        PULONGLONG LastCluster);

PFILENAME_ATTRIBUTE
GetBestFileNameFromRecord(PDEVICE_EXTENSION Vcb,
                          PFILE_RECORD_HEADER FileRecord);

NTSTATUS
FindFirstAttributeListItem(PFIND_ATTR_CONTXT Context,
                           PNTFS_ATTRIBUTE_LIST_ITEM *Item);

NTSTATUS
FindNextAttributeListItem(PFIND_ATTR_CONTXT Context,
                          PNTFS_ATTRIBUTE_LIST_ITEM *Item);

NTSTATUS
FindFirstAttribute(PFIND_ATTR_CONTXT Context,
                   PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER FileRecord,
                   BOOLEAN OnlyResident,
                   PNTFS_ATTR_RECORD * Attribute);

NTSTATUS
FindNextAttribute(PFIND_ATTR_CONTXT Context,
                  PNTFS_ATTR_RECORD * Attribute);

VOID
FindCloseAttribute(PFIND_ATTR_CONTXT Context);

NTSTATUS
FreeClusters(PNTFS_VCB Vcb,
             PNTFS_ATTR_CONTEXT AttrContext,
             ULONG AttrOffset,
             PFILE_RECORD_HEADER FileRecord,
             ULONG ClustersToFree);

/* blockdev.c */

NTSTATUS
NtfsReadDisk(IN PDEVICE_OBJECT DeviceObject,
             IN LONGLONG StartingOffset,
             IN ULONG Length,
             IN ULONG SectorSize,
             IN OUT PUCHAR Buffer,
             IN BOOLEAN Override);

NTSTATUS
NtfsWriteDisk(IN PDEVICE_OBJECT DeviceObject,
              IN LONGLONG StartingOffset,
              IN ULONG Length,
              IN ULONG SectorSize,
              IN const PUCHAR Buffer);

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


/* btree.c */

LONG
CompareTreeKeys(PB_TREE_KEY Key1,
                PB_TREE_KEY Key2,
                BOOLEAN CaseSensitive);

NTSTATUS
CreateBTreeFromIndex(PDEVICE_EXTENSION Vcb,
                     PFILE_RECORD_HEADER FileRecordWithIndex,
                     /*PCWSTR IndexName,*/
                     PNTFS_ATTR_CONTEXT IndexRootContext,
                     PINDEX_ROOT_ATTRIBUTE IndexRoot,
                     PB_TREE *NewTree);

NTSTATUS
CreateIndexRootFromBTree(PDEVICE_EXTENSION DeviceExt,
                         PB_TREE Tree,
                         ULONG MaxIndexSize,
                         PINDEX_ROOT_ATTRIBUTE *IndexRoot,
                         ULONG *Length);

NTSTATUS
DemoteBTreeRoot(PB_TREE Tree);

VOID
DestroyBTree(PB_TREE Tree);

VOID
DestroyBTreeNode(PB_TREE_FILENAME_NODE Node);

VOID
DumpBTree(PB_TREE Tree);

VOID
DumpBTreeKey(PB_TREE Tree,
             PB_TREE_KEY Key,
             ULONG Number,
             ULONG Depth);

VOID
DumpBTreeNode(PB_TREE Tree,
              PB_TREE_FILENAME_NODE Node,
              ULONG Number,
              ULONG Depth);

NTSTATUS
CreateEmptyBTree(PB_TREE *NewTree);

ULONGLONG
GetAllocationOffsetFromVCN(PDEVICE_EXTENSION DeviceExt,
                           ULONG IndexBufferSize,
                           ULONGLONG Vcn);

ULONGLONG
GetIndexEntryVCN(PINDEX_ENTRY_ATTRIBUTE IndexEntry);

ULONG
GetSizeOfIndexEntries(PB_TREE_FILENAME_NODE Node);

NTSTATUS
NtfsInsertKey(PB_TREE Tree,
              ULONGLONG FileReference,
              PFILENAME_ATTRIBUTE FileNameAttribute,
              PB_TREE_FILENAME_NODE Node,
              BOOLEAN CaseSensitive,
              ULONG MaxIndexRootSize,
              ULONG IndexRecordSize,
              PB_TREE_KEY *MedianKey,
              PB_TREE_FILENAME_NODE *NewRightHandSibling);

NTSTATUS
SplitBTreeNode(PB_TREE Tree,
               PB_TREE_FILENAME_NODE Node,
               PB_TREE_KEY *MedianKey,
               PB_TREE_FILENAME_NODE *NewRightHandSibling,
               BOOLEAN CaseSensitive);

NTSTATUS
UpdateIndexAllocation(PDEVICE_EXTENSION DeviceExt,
                      PB_TREE Tree,
                      ULONG IndexBufferSize,
                      PFILE_RECORD_HEADER FileRecord);

NTSTATUS
UpdateIndexNode(PDEVICE_EXTENSION DeviceExt,
                PFILE_RECORD_HEADER FileRecord,
                PB_TREE_FILENAME_NODE Node,
                ULONG IndexBufferSize,
                PNTFS_ATTR_CONTEXT IndexAllocationContext,
                ULONG IndexAllocationOffset);

/* close.c */

NTSTATUS
NtfsCleanup(PNTFS_IRP_CONTEXT IrpContext);


/* close.c */

NTSTATUS
NtfsCloseFile(PDEVICE_EXTENSION DeviceExt,
              PFILE_OBJECT FileObject);

NTSTATUS
NtfsClose(PNTFS_IRP_CONTEXT IrpContext);


/* create.c */

NTSTATUS
NtfsCreate(PNTFS_IRP_CONTEXT IrpContext);

NTSTATUS
NtfsCreateDirectory(PDEVICE_EXTENSION DeviceExt,
                    PFILE_OBJECT FileObject,
                    BOOLEAN CaseSensitive,
                    BOOLEAN CanWait);

PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(PDEVICE_EXTENSION DeviceExt);

NTSTATUS
NtfsCreateFileRecord(PDEVICE_EXTENSION DeviceExt,
                     PFILE_OBJECT FileObject,
                     BOOLEAN CaseSensitive,
                     BOOLEAN CanWait);

/* devctl.c */

NTSTATUS
NtfsDeviceControl(PNTFS_IRP_CONTEXT IrpContext);


/* dirctl.c */

ULONGLONG
NtfsGetFileSize(PDEVICE_EXTENSION DeviceExt,
                PFILE_RECORD_HEADER FileRecord,
                PCWSTR Stream,
                ULONG StreamLength,
                PULONGLONG AllocatedSize);

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
              PCWSTR Stream,
              PNTFS_VCB Vcb);

VOID
NtfsDestroyFCB(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsDirectory(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsReparsePoint(PNTFS_FCB Fcb);

BOOLEAN
NtfsFCBIsCompressed(PNTFS_FCB Fcb);

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
                  PCWSTR pFileName,
                  BOOLEAN CaseSensitive);

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
                        PCWSTR Stream,
                        PFILE_RECORD_HEADER Record,
                        ULONGLONG MFTIndex,
                        PNTFS_FCB * fileFCB);


/* finfo.c */

NTSTATUS
NtfsQueryInformation(PNTFS_IRP_CONTEXT IrpContext);

NTSTATUS
NtfsSetEndOfFile(PNTFS_FCB Fcb,
                 PFILE_OBJECT FileObject,
                 PDEVICE_EXTENSION DeviceExt,
                 ULONG IrpFlags,
                 BOOLEAN CaseSensitive,
                 PLARGE_INTEGER NewFileSize);

NTSTATUS
NtfsSetInformation(PNTFS_IRP_CONTEXT IrpContext);

/* fsctl.c */

NTSTATUS
NtfsFileSystemControl(PNTFS_IRP_CONTEXT IrpContext);


/* mft.c */
NTSTATUS
NtfsAddFilenameToDirectory(PDEVICE_EXTENSION DeviceExt,
                           ULONGLONG DirectoryMftIndex,
                           ULONGLONG FileReferenceNumber,
                           PFILENAME_ATTRIBUTE FilenameAttribute,
                           BOOLEAN CaseSensitive);

NTSTATUS
AddNewMftEntry(PFILE_RECORD_HEADER FileRecord,
               PDEVICE_EXTENSION DeviceExt,
               PULONGLONG DestinationIndex,
               BOOLEAN CanWait);

VOID
NtfsDumpData(ULONG_PTR Buffer, ULONG Length);

PNTFS_ATTR_CONTEXT
PrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord);

VOID
ReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context);

ULONG
ReadAttribute(PDEVICE_EXTENSION Vcb,
              PNTFS_ATTR_CONTEXT Context,
              ULONGLONG Offset,
              PCHAR Buffer,
              ULONG Length);

NTSTATUS
WriteAttribute(PDEVICE_EXTENSION Vcb,
               PNTFS_ATTR_CONTEXT Context,
               ULONGLONG Offset,
               const PUCHAR Buffer,
               ULONG Length,
               PULONG LengthWritten,
               PFILE_RECORD_HEADER FileRecord);

ULONGLONG
AttributeDataLength(PNTFS_ATTR_RECORD AttrRecord);

NTSTATUS
InternalSetResidentAttributeLength(PDEVICE_EXTENSION DeviceExt,
                                   PNTFS_ATTR_CONTEXT AttrContext,
                                   PFILE_RECORD_HEADER FileRecord,
                                   ULONG AttrOffset,
                                   ULONG DataSize);

PNTFS_ATTR_RECORD
MoveAttributes(PDEVICE_EXTENSION DeviceExt,
               PNTFS_ATTR_RECORD FirstAttributeToMove,
               ULONG FirstAttributeOffset,
               ULONG_PTR MoveTo);

NTSTATUS
SetAttributeDataLength(PFILE_OBJECT FileObject,
                       PNTFS_FCB Fcb,
                       PNTFS_ATTR_CONTEXT AttrContext,
                       ULONG AttrOffset,
                       PFILE_RECORD_HEADER FileRecord,
                       PLARGE_INTEGER DataSize);

VOID
SetFileRecordEnd(PFILE_RECORD_HEADER FileRecord,
                 PNTFS_ATTR_RECORD AttrEnd,
                 ULONG EndMarker);

NTSTATUS
SetNonResidentAttributeDataLength(PDEVICE_EXTENSION Vcb,
                                  PNTFS_ATTR_CONTEXT AttrContext,
                                  ULONG AttrOffset,
                                  PFILE_RECORD_HEADER FileRecord,
                                  PLARGE_INTEGER DataSize);

NTSTATUS
SetResidentAttributeDataLength(PDEVICE_EXTENSION Vcb,
                               PNTFS_ATTR_CONTEXT AttrContext,
                               ULONG AttrOffset,
                               PFILE_RECORD_HEADER FileRecord,
                               PLARGE_INTEGER DataSize);

ULONGLONG
AttributeAllocatedLength(PNTFS_ATTR_RECORD AttrRecord);

BOOLEAN
CompareFileName(PUNICODE_STRING FileName,
                PINDEX_ENTRY_ATTRIBUTE IndexEntry,
                BOOLEAN DirSearch,
                BOOLEAN CaseSensitive);

NTSTATUS
UpdateMftMirror(PNTFS_VCB Vcb);

NTSTATUS
ReadFileRecord(PDEVICE_EXTENSION Vcb,
               ULONGLONG index,
               PFILE_RECORD_HEADER file);

NTSTATUS
UpdateIndexEntryFileNameSize(PDEVICE_EXTENSION Vcb,
                             PFILE_RECORD_HEADER MftRecord,
                             PCHAR IndexRecord,
                             ULONG IndexBlockSize,
                             PINDEX_ENTRY_ATTRIBUTE FirstEntry,
                             PINDEX_ENTRY_ATTRIBUTE LastEntry,
                             PUNICODE_STRING FileName,
                             PULONG StartEntry,
                             PULONG CurrentEntry,
                             BOOLEAN DirSearch,
                             ULONGLONG NewDataSize,
                             ULONGLONG NewAllocatedSize,
                             BOOLEAN CaseSensitive);

NTSTATUS
UpdateFileNameRecord(PDEVICE_EXTENSION Vcb,
                     ULONGLONG ParentMFTIndex,
                     PUNICODE_STRING FileName,
                     BOOLEAN DirSearch,
                     ULONGLONG NewDataSize,
                     ULONGLONG NewAllocationSize,
                     BOOLEAN CaseSensitive);

NTSTATUS
UpdateFileRecord(PDEVICE_EXTENSION Vcb,
                 ULONGLONG MftIndex,
                 PFILE_RECORD_HEADER FileRecord);

NTSTATUS
FindAttribute(PDEVICE_EXTENSION Vcb,
              PFILE_RECORD_HEADER MftRecord,
              ULONG Type,
              PCWSTR Name,
              ULONG NameLength,
              PNTFS_ATTR_CONTEXT * AttrCtx,
              PULONG Offset);

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
AddFixupArray(PDEVICE_EXTENSION Vcb,
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
               BOOLEAN CaseSensitive,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex);

NTSTATUS
NtfsLookupFileAt(PDEVICE_EXTENSION Vcb,
                 PUNICODE_STRING PathName,
                 BOOLEAN CaseSensitive,
                 PFILE_RECORD_HEADER *FileRecord,
                 PULONGLONG MFTIndex,
                 ULONGLONG CurrentMFTIndex);

VOID
NtfsDumpFileRecord(PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER FileRecord);

NTSTATUS
NtfsFindFileAt(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING SearchPattern,
               PULONG FirstEntry,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex,
               ULONGLONG CurrentMFTIndex,
               BOOLEAN CaseSensitive);

NTSTATUS
NtfsFindMftRecord(PDEVICE_EXTENSION Vcb,
                  ULONGLONG MFTIndex,
                  PUNICODE_STRING FileName,
                  PULONG FirstEntry,
                  BOOLEAN DirSearch,
                  BOOLEAN CaseSensitive,
                  ULONGLONG *OutMFTIndex);

/* misc.c */

BOOLEAN
NtfsIsIrpTopLevel(PIRP Irp);

PNTFS_IRP_CONTEXT
NtfsAllocateIrpContext(PDEVICE_OBJECT DeviceObject,
                       PIRP Irp);

PVOID
NtfsGetUserBuffer(PIRP Irp,
                  BOOLEAN Paging);

NTSTATUS
NtfsLockUserBuffer(IN PIRP Irp,
                   IN ULONG Length,
                   IN LOCK_OPERATION Operation);

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

NTSTATUS
NtfsAllocateClusters(PDEVICE_EXTENSION DeviceExt,
                     ULONG FirstDesiredCluster,
                     ULONG DesiredClusters,
                     PULONG FirstAssignedCluster,
                     PULONG AssignedClusters);

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
