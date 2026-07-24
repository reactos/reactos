/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/files.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include "ntfslib.h"
#include "data.h"

#define NDEBUG
#include <debug.h>


/* STRUCTURES ****************************************************************/

typedef struct
{
    ULONG  Number;
    WCHAR* Name;
    PFILE_RECORD_HEADER (*Constructor)();
    NTSTATUS (*AdditionalDataWriter)();
} METAFILE, *PMETAFILE;


/* PROTOTYPES ****************************************************************/

static
VOID
ApplyFixups(IN OUT PFILE_RECORD_HEADER FileRecord);

static
NTSTATUS
WriteZerosToClusters(IN  ULONGLONG           Address,
                     IN  ULONG               ClustersCount);

static
NTSTATUS
WritePatternToClusters(IN ULONGLONG Address,
                       IN ULONG     ClustersCount,
                       IN BYTE      Pattern);

static
NTSTATUS
WriteMetafile(IN  PFILE_RECORD_HEADER      FileRecord,
              OUT PIO_STATUS_BLOCK         IoStatusBlock);

static
NTSTATUS
WriteMetafileMirror(IN  PFILE_RECORD_HEADER      FileRecord,
                    OUT PIO_STATUS_BLOCK         IoStatusBlock);

static
PFILE_RECORD_HEADER
CreateMetafileRecord(IN DWORD32 MftRecordNumber, OUT PATTR_RECORD* Attribute);

static
PFILE_RECORD_HEADER
NtfsCreateBlankFileRecord(IN  DWORD32       MftRecordNumber,
                          OUT PATTR_RECORD* NextAttribute);

static
PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(IN DWORD32 MftRecordNumber);

static
PFILE_RECORD_HEADER
CreateDirectoryRecord(IN DWORD32 MftRecordNumber, IN LPCWSTR Name);

static PFILE_RECORD_HEADER CreateMft();
static PFILE_RECORD_HEADER CreateMftMirr();
static PFILE_RECORD_HEADER CreateLogFile();
static PFILE_RECORD_HEADER CreateVolume();
static PFILE_RECORD_HEADER CreateAttrDef();
static PFILE_RECORD_HEADER CreateRoot();
static PFILE_RECORD_HEADER CreateBitmap();
static PFILE_RECORD_HEADER CreateBoot();
static PFILE_RECORD_HEADER CreateBadClus();
static PFILE_RECORD_HEADER CreateSecure();
static PFILE_RECORD_HEADER CreateUpCase();
static PFILE_RECORD_HEADER CreateExtend();
static PFILE_RECORD_HEADER CreateStub(IN DWORD32 MftRecordNumber);

static NTSTATUS WriteMftBitmap();
static NTSTATUS WriteMftMirr();
static NTSTATUS WriteLogFile();
static NTSTATUS WriteAttributesTable();
static NTSTATUS WriteVolumeBitmap();
static NTSTATUS WriteUpCaseTable();
static NTSTATUS WriteRootIndex();
static NTSTATUS WriteSecureSds();


/* CONSTS ********************************************************************/

static const METAFILE METAFILES[] =
{
    { METAFILE_MFT    , L"$MFT"    , CreateMft    , WriteMftBitmap        },
    { METAFILE_MFTMIRR, L"$MFTMirr", CreateMftMirr, WriteMftMirr          },
    { METAFILE_LOGFILE, L"$LogFile", CreateLogFile, WriteLogFile          },
    { METAFILE_VOLUME , L"$Volume" , CreateVolume , NULL                  },
    { METAFILE_ATTRDEF, L"$AttrDef", CreateAttrDef, WriteAttributesTable  },
    { METAFILE_ROOT   , L"."       , CreateRoot   , WriteRootIndex        },
    { METAFILE_BITMAP , L"$Bitmap" , CreateBitmap , WriteVolumeBitmap     },
    { METAFILE_BOOT   , L"$Boot"   , CreateBoot   , NULL                  },
    { METAFILE_BADCLUS, L"$BadClus", CreateBadClus, NULL                  },
    { METAFILE_SECURE , L"$Secure" , CreateSecure , WriteSecureSds        },
    { METAFILE_UPCASE , L"$UpCase" , CreateUpCase , WriteUpCaseTable      },
    { METAFILE_EXTEND , L"$Extend" , CreateExtend , NULL                  },
    { 12              , L""        , NULL         , NULL                  },  // Reserved
    { 13              , L""        , NULL         , NULL                  },  // Reserved
    { 14              , L""        , NULL         , NULL                  },  // Reserved
    { 15              , L""        , NULL         , NULL                  },  // Reserved
};


/* FUNCTIONS *****************************************************************/

NTSTATUS
WriteMetafiles(VOID)
{
    BYTE MetafileIndex;
    METAFILE Metafile;
    PFILE_RECORD_HEADER FileRecord;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    // Clear the $MFT area
    Status = WriteZerosToClusters(LAYOUT.MftLcn,
                                  LAYOUT.MftClusters);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to clear sectors. NtWriteFile() failed (Status %lx)\n", Status);
        return Status;
    }

    // Write metafiles
    for (
        MetafileIndex = 0;
        MetafileIndex < ARR_SIZE(METAFILES);
        MetafileIndex++
    )
    {
        Metafile = METAFILES[MetafileIndex];

        // Create metafile record or stub, if metafile is not implemented
        if (!Metafile.Constructor)
        {
            FileRecord = CreateStub(MetafileIndex);
        }
        else
        {
            FileRecord = Metafile.Constructor();
        }

        // Check file record
        if (!FileRecord)
        {
            DPRINT1(
                "ERROR: Unable to allocate memory for file record #%d!\n",
                MetafileIndex
            );

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Write metafile to disk
        Status = WriteMetafile(FileRecord, &IoStatusBlock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1(
                "ERROR: Unable to write metafile #%d to disk. NtWriteFile() failed (Status %lx)\n",
                MetafileIndex,
                Status
            );

            FREE(FileRecord);
            break;
        }

        // Write additional data to disk
        if (Metafile.AdditionalDataWriter)
        {
            Status = Metafile.AdditionalDataWriter();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1(
                    "ERROR: Unable to write additional data for metafile #%d to disk. Status %lx\n",
                    MetafileIndex,
                    Status
                );

                FREE(FileRecord);
                break;
            }
        }

        // Free memory
        FREE(FileRecord);
    }

    return Status;
}


/* FIXUPS *******************************************************************/

//
// Applies the Update Sequence Array (USA) fixup to a record before it is
// written to disk: a check value (USN) is stamped into the last two bytes of
// every sector the record spans, and the original bytes are saved into the USA.
// A correct NTFS driver validates and restores these on read; without this the
// record is not a valid NTFS record.
//
static
VOID
ApplyFixups(IN OUT PFILE_RECORD_HEADER FileRecord)
{
    PRECORD_HEADER Header = &FileRecord->Header;
    ULONG   BytesPerSector = BYTES_PER_SECTOR;
    PUSHORT Usa = (PUSHORT)((PBYTE)FileRecord + Header->UsaOffset);
    USHORT  Usn = 1;  // Any value except 0x0000 / 0xFFFF
    ULONG   Sectors;
    ULONG   i;

    if (Header->UsaCount < 2)
        return;

    Sectors = Header->UsaCount - 1;

    Usa[0] = Usn;

    for (i = 0; i < Sectors; i++)
    {
        PUSHORT Fixup = (PUSHORT)((PBYTE)FileRecord + (i + 1) * BytesPerSector - sizeof(USHORT));

        Usa[i + 1] = *Fixup;  // Save the original last two bytes of this sector
        *Fixup     = Usn;     // Stamp the USN
    }
}


/* DISK FUNCTIONS ***********************************************************/

static
NTSTATUS
WriteZerosToClusters(IN  ULONGLONG           Address,
                     IN  ULONG               ClustersCount)
{
    return WritePatternToClusters(Address, ClustersCount, 0x00);
}

//
// Fills a contiguous run of clusters with a byte pattern, in bounded-size
// chunks so we never allocate a huge buffer for e.g. the $LogFile.
//
static
NTSTATUS
WritePatternToClusters(IN ULONGLONG Address,
                       IN ULONG     ClustersCount,
                       IN BYTE      Pattern)
{
    PBYTE           Buffer;
    ULONG           ChunkClusters;
    ULONG           ChunkBytes;
    ULONG           Remaining;
    ULONGLONG       Lcn;
    NTSTATUS        Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    if (ClustersCount == 0)
        return STATUS_SUCCESS;

    // Cap each chunk to ~1 MB worth of clusters.
    ChunkClusters = (1024 * 1024) / BYTES_PER_CLUSTER;
    if (ChunkClusters == 0)
        ChunkClusters = 1;
    if (ChunkClusters > ClustersCount)
        ChunkClusters = ClustersCount;

    ChunkBytes = ChunkClusters * BYTES_PER_CLUSTER;

    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, ChunkBytes);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlFillMemory(Buffer, ChunkBytes, Pattern);

    Lcn = Address;
    Remaining = ClustersCount;

    while (Remaining > 0)
    {
        LARGE_INTEGER Offset;
        ULONG         ThisClusters = (Remaining < ChunkClusters) ? Remaining : ChunkClusters;
        ULONG         ThisBytes    = ThisClusters * BYTES_PER_CLUSTER;

        Offset.QuadPart = (LONGLONG)Lcn * BYTES_PER_CLUSTER;

        Status = NtWriteFile(DISK_HANDLE,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             ThisBytes,
                             &Offset,
                             NULL);
        if (!NT_SUCCESS(Status))
            break;

        Lcn       += ThisClusters;
        Remaining -= ThisClusters;
    }

    FREE(Buffer);

    return Status;
}

static
NTSTATUS
WriteMetafile(IN  PFILE_RECORD_HEADER      FileRecord,
              OUT PIO_STATUS_BLOCK         IoStatusBlock)
{
    LARGE_INTEGER Offset;

    // Apply the USA fixup right before writing
    ApplyFixups(FileRecord);

    // Offset to $MFT + offset to record
    Offset.QuadPart =
        ((LONGLONG)LAYOUT.MftLcn * BYTES_PER_CLUSTER) +
        (LONGLONG)(FileRecord->MFTRecordNumber * MFT_RECORD_SIZE);

    return NtWriteFile(DISK_HANDLE,
                       NULL,
                       NULL,
                       NULL,
                       IoStatusBlock,
                       FileRecord,
                       MFT_RECORD_SIZE,
                       &Offset,
                       NULL);
}

static
NTSTATUS
WriteMetafileMirror(IN  PFILE_RECORD_HEADER      FileRecord,
                    OUT PIO_STATUS_BLOCK         IoStatusBlock)
{
    LARGE_INTEGER Offset;

    // Apply the USA fixup right before writing
    ApplyFixups(FileRecord);

    // Offset to $MFTMirr + offset to record
    Offset.QuadPart =
        ((LONGLONG)LAYOUT.MftMirrLcn * BYTES_PER_CLUSTER) +
        (LONGLONG)(FileRecord->MFTRecordNumber * MFT_RECORD_SIZE);

    return NtWriteFile(DISK_HANDLE,
                       NULL,
                       NULL,
                       NULL,
                       IoStatusBlock,
                       FileRecord,
                       MFT_RECORD_SIZE,
                       &Offset,
                       NULL);
}


/* METAFILES FUNCTIONS ******************************************************/

static
PFILE_RECORD_HEADER
CreateMetafileRecord(IN  DWORD32       MftRecordNumber,
                     OUT PATTR_RECORD* Attribute)
{
    PFILE_RECORD_HEADER FileRecord;

    FileRecord = NtfsCreateBlankFileRecord(MftRecordNumber, Attribute);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    return FileRecord;
}

static
PFILE_RECORD_HEADER
NtfsCreateBlankFileRecord(IN  DWORD32       MftRecordNumber,
                          OUT PATTR_RECORD* NextAttribute)
{
    PFILE_RECORD_HEADER FileRecord;
    LPCWSTR             Name = METAFILES[MftRecordNumber].Name;

    // Create empty file record
    FileRecord = NtfsCreateEmptyFileRecord(MftRecordNumber);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    // $STANDARD_INFORMATION
    (*NextAttribute) = FIRST_ATTRIBUTE(FileRecord);
    AddStandardInformationAttribute(FileRecord, *NextAttribute);

    // $FILE_NAME (all system files live in the root directory). Reserved
    // records (12..15) have no name and get no $FILE_NAME.
    (*NextAttribute) = NEXT_ATTRIBUTE(*NextAttribute);
    if (Name && Name[0])
    {
        AddFileNameAttribute(FileRecord, *NextAttribute, Name, METAFILE_ROOT);
        (*NextAttribute) = NEXT_ATTRIBUTE(*NextAttribute);
    }

    return FileRecord;
}

static
PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(IN DWORD32 MftRecordNumber)
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD NextAttribute;

    // Allocate memory for file record
    FileRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, MFT_RECORD_SIZE);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    RtlZeroMemory(FileRecord, MFT_RECORD_SIZE);

    FileRecord->Header.Magic    = FILE_RECORD_MAGIC;
    FileRecord->MFTRecordNumber = MftRecordNumber;

    // Calculate USA offset and count
    FileRecord->Header.UsaOffset = FIELD_OFFSET(FILE_RECORD_HEADER, MFTRecordNumber) + sizeof(ULONG);

    // Size of USA (in ULONG's) will be 1 (for USA number) + 1 for every sector the file record uses
    FileRecord->BytesAllocated = MFT_RECORD_SIZE;
    FileRecord->Header.UsaCount = (FileRecord->BytesAllocated / BYTES_PER_SECTOR) + 1;

    // Setup other file record fields.
    // Windows sets a metadata file's sequence number equal to its record number
    // (record 0 is the exception: sequence 0 means "unused", so it uses 1).
    // chkdsk flags records whose sequence number doesn't follow this.
    FileRecord->SequenceNumber  = (MftRecordNumber == 0) ? 1 : (USHORT)MftRecordNumber;
    FileRecord->FirstAttributeOffset = FileRecord->Header.UsaOffset + (2 * FileRecord->Header.UsaCount);
    FileRecord->FirstAttributeOffset = ALIGN_UP_BY(FileRecord->FirstAttributeOffset, ATTR_RECORD_ALIGNMENT);
    FileRecord->Flags      = MFT_RECORD_IN_USE;
    FileRecord->BytesInUse = FileRecord->FirstAttributeOffset + sizeof(ULONG) * 2;

    // Find where the first attribute will be added
    NextAttribute = (PATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->FirstAttributeOffset);

    // Temporary mark the end of the file-record
    NextAttribute->Type   = AttributeEnd;
    NextAttribute->Length = FILE_RECORD_END;

    return FileRecord;
}

//
// Builds an empty directory file record: $STANDARD_INFORMATION, $FILE_NAME
// (marked as a directory) and an empty $INDEX_ROOT ("$I30").
//
static
PFILE_RECORD_HEADER
CreateDirectoryRecord(IN DWORD32 MftRecordNumber, IN LPCWSTR Name)
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute;

    FileRecord = NtfsCreateEmptyFileRecord(MftRecordNumber);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for directory record #%d!\n", MftRecordNumber);
        return NULL;
    }

    // Mark the record as a directory *before* adding $FILE_NAME so that the
    // FILE_ATTRIBUTE_DIRECTORY flag is reflected there.
    FileRecord->Flags |= MFT_RECORD_IS_DIRECTORY;

    // $STANDARD_INFORMATION
    Attribute = FIRST_ATTRIBUTE(FileRecord);
    AddStandardInformationAttribute(FileRecord, Attribute);

    // $FILE_NAME
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddFileNameAttribute(FileRecord, Attribute, Name, METAFILE_ROOT);

    // $INDEX_ROOT [$I30] - empty index
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddIndexRoot(FileRecord, Attribute);

    return FileRecord;
}


/* METAFILES CONSTRUCTORS ***************************************************/

static
PFILE_RECORD_HEADER
CreateMft()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_MFT, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.MftLcn,
                                LAYOUT.MftClusters,
                                0);

    // $BITMAP
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddMftBitmapAttribute(FileRecord, Attribute);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateMftMirr()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_MFTMIRR, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.MftMirrLcn,
                                LAYOUT.MftMirrClusters,
                                0);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateLogFile()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_LOGFILE, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.LogFileLcn,
                                LAYOUT.LogFileClusters,
                                0);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateVolume()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_VOLUME, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $VOLUME_NAME (from the requested label, if any)
    AddVolumeNameAttribute(FileRecord, Attribute, LABEL);

    // $VOLUME_INFORMATION
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddVolumeInformationAttribute(FileRecord,
                                  Attribute,
                                  NTFS_MAJOR_VERSION,
                                  NTFS_MINOR_VERSION);

    // $DATA
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddEmptyDataAttribute(FileRecord, Attribute);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateAttrDef()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_ATTRDEF, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.AttrDefLcn,
                                LAYOUT.AttrDefClusters,
                                sizeof(ATTRIBUTES_TABLE));

    return FileRecord;
}

//
// The root directory. Unlike a small directory, root uses a "large" $I30
// index: an $INDEX_ROOT that points at an $INDEX_ALLOCATION (one INDX block,
// written by WriteRootIndex) which holds the entries for the system files.
//
static
PFILE_RECORD_HEADER
CreateRoot()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute;
    BYTE                IndexBitmap[8];

    FileRecord = NtfsCreateEmptyFileRecord(METAFILE_ROOT);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for root file record!\n");
        return NULL;
    }

    FileRecord->Flags |= MFT_RECORD_IS_DIRECTORY;

    // $STANDARD_INFORMATION
    Attribute = FIRST_ATTRIBUTE(FileRecord);
    AddStandardInformationAttribute(FileRecord, Attribute);

    // $FILE_NAME "."
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddFileNameAttribute(FileRecord, Attribute, L".", METAFILE_ROOT);

    // $INDEX_ROOT [$I30] - large (points at the $INDEX_ALLOCATION)
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddIndexRootLarge(FileRecord, Attribute);

    // $INDEX_ALLOCATION [$I30] - one INDX block at RootIdxLcn
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddIndexAllocationAttribute(FileRecord, Attribute, LAYOUT.RootIdxLcn, LAYOUT.RootIdxClusters);

    // $BITMAP [$I30] - INDX VCN 0 is in use
    Attribute = NEXT_ATTRIBUTE(Attribute);
    RtlZeroMemory(IndexBitmap, sizeof(IndexBitmap));
    IndexBitmap[0] = 0x01;
    AddIndexBitmapAttribute(FileRecord, Attribute, L"$I30", IndexBitmap, sizeof(IndexBitmap));

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateBitmap()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // One bit per cluster of the whole volume, rounded up to bytes.
    ULONGLONG BitmapBytes = (LAYOUT.ClusterCount + 7) / 8;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_BITMAP, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.BitmapLcn,
                                LAYOUT.BitmapClusters,
                                BitmapBytes);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateBoot()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_BOOT, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA (the boot sector data itself is written by WriteBootSector)
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.BootLcn,
                                LAYOUT.BootClusters,
                                0);

    return FileRecord;
}

//
// $BadClus: an empty unnamed $DATA plus the sparse $DATA:$Bad stream that
// spans the whole volume (no bad clusters on a fresh format).
//
static
PFILE_RECORD_HEADER
CreateBadClus()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_BADCLUS, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // Unnamed empty $DATA (the default stream)
    AddEmptyDataAttribute(FileRecord, Attribute);

    // $DATA:$Bad - sparse, whole-volume
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddBadClusterDataAttribute(FileRecord, Attribute);

    return FileRecord;
}

/* SECURITY *****************************************************************/

#define SD_LENGTH        72
#define SDS_HEADER_LEN   0x14
#define SDS_ENTRY_LEN    (SDS_HEADER_LEN + SD_LENGTH)   // 0x5C

//
// Builds a 72-byte self-relative security descriptor granting Everyone full
// control, owner/group = Local System. Returns the length.
//
static
ULONG
BuildSecurityDescriptor(OUT PBYTE Out)
{
    // S-1-5-18 (Local System)
    static const BYTE SidSystem[12] =
        { 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00 };
    // S-1-1-0 (Everyone)
    static const BYTE SidEveryone[12] =
        { 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

    const USHORT OwnerOff = 0x14;
    const USHORT GroupOff = 0x20;
    const USHORT DaclOff  = 0x2C;
    PBYTE P;

    RtlZeroMemory(Out, SD_LENGTH);

    // SECURITY_DESCRIPTOR_RELATIVE header
    Out[0x00] = 1;                          // Revision
    Out[0x01] = 0;                          // Sbz1
    *(PUSHORT)(Out + 0x02) = 0x8004;        // SELF_RELATIVE | DACL_PRESENT
    *(PULONG)(Out + 0x04)  = OwnerOff;
    *(PULONG)(Out + 0x08)  = GroupOff;
    *(PULONG)(Out + 0x0C)  = 0;             // No SACL
    *(PULONG)(Out + 0x10)  = DaclOff;

    RtlCopyMemory(Out + OwnerOff, SidSystem, 12);
    RtlCopyMemory(Out + GroupOff, SidSystem, 12);

    // DACL: ACL header (8) + one ACCESS_ALLOWED_ACE (20)
    P = Out + DaclOff;
    P[0x00] = 2;                            // ACL revision
    P[0x01] = 0;
    *(PUSHORT)(P + 0x02) = 0x1C;            // ACL size = 8 + 20
    *(PUSHORT)(P + 0x04) = 1;               // ACE count
    *(PUSHORT)(P + 0x06) = 0;

    P[0x08] = 0x00;                         // ACCESS_ALLOWED_ACE_TYPE
    P[0x09] = 0x00;                         // Flags
    *(PUSHORT)(P + 0x0A) = 0x14;            // ACE size = 8 + 12
    *(PULONG)(P + 0x0C)  = 0x1F01FF;        // Full control
    RtlCopyMemory(P + 0x10, SidEveryone, 12);

    return SD_LENGTH;
}

//
// NTFS security-descriptor hash: rol32(hash,3) + dword over the descriptor.
//
static
ULONG
SecurityHash(IN PBYTE Sd, IN ULONG Len)
{
    ULONG Hash = 0;
    ULONG i;

    for (i = 0; i + 4 <= Len; i += 4)
    {
        ULONG Dword = *(PULONG)(Sd + i);
        Hash = Dword + ((Hash << 3) | (Hash >> 29));
    }

    return Hash;
}

// Builds the 20-byte $SDS entry header (also the $SDH/$SII index data).
static
VOID
BuildSdsHeader(OUT PBYTE Out, IN ULONG Hash, IN ULONGLONG Offset, IN ULONG EntryLen)
{
    *(PULONG)(Out + 0x00)     = Hash;
    *(PULONG)(Out + 0x04)     = NTFS_SECURITY_ID;
    *(PULONGLONG)(Out + 0x08) = Offset;      // Offset of this entry within $SDS
    *(PULONG)(Out + 0x10)     = EntryLen;
}

//
// $Secure: the real security-descriptor store. Holds one descriptor (Everyone:
// full control) in $SDS (plus its 256 KiB mirror), referenced by every file
// through SecurityId 0x100 and indexed by $SDH (hash) and $SII (id). The record
// carries the view-index flag. NTFS orders same-type attributes by name, so
// $SDH precedes $SII.
//
static
PFILE_RECORD_HEADER
CreateSecure()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute;
    BYTE                Sd[SD_LENGTH];
    BYTE                SdsHeader[SDS_HEADER_LEN];
    BYTE                SdhKey[8];
    BYTE                SiiKey[4];
    ULONG               SdLen;
    ULONG               Hash;
    ULONGLONG           SdsDataSize;

    SdLen = BuildSecurityDescriptor(Sd);
    Hash  = SecurityHash(Sd, SdLen);
    BuildSdsHeader(SdsHeader, Hash, 0, SDS_HEADER_LEN + SdLen);

    *(PULONG)(SdhKey + 0) = Hash;
    *(PULONG)(SdhKey + 4) = NTFS_SECURITY_ID;
    *(PULONG)(SiiKey + 0) = NTFS_SECURITY_ID;

    // Logical $SDS size: the primary entry plus the mirror 256 KiB later.
    SdsDataSize = (ULONGLONG)NTFS_SDS_MIRROR + SDS_HEADER_LEN + SdLen;

    FileRecord = NtfsCreateEmptyFileRecord(METAFILE_SECURE);
    if (!FileRecord)
    {
        return NULL;
    }

    // $Secure carries view indexes ($SDH/$SII).
    FileRecord->Flags |= MFT_RECORD_VIEW_INDEX;

    // $STANDARD_INFORMATION
    Attribute = FIRST_ATTRIBUTE(FileRecord);
    AddStandardInformationAttribute(FileRecord, Attribute);

    // $FILE_NAME
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddFileNameAttribute(FileRecord, Attribute, L"$Secure", METAFILE_ROOT);

    // $DATA:$SDS
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddNamedNonResidentDataAttribute(FileRecord, Attribute, L"$SDS",
                                     LAYOUT.SdsLcn, LAYOUT.SdsClusters, SdsDataSize);

    // $INDEX_ROOT:$SDH (by descriptor hash)
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddViewIndexRoot(FileRecord, Attribute, L"$SDH", COLLATION_NTOFS_SECURITY_HASH,
                     SdhKey, sizeof(SdhKey), SdsHeader, SDS_HEADER_LEN);

    // $INDEX_ROOT:$SII (by security id)
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddViewIndexRoot(FileRecord, Attribute, L"$SII", COLLATION_NTOFS_ULONG,
                     SiiKey, sizeof(SiiKey), SdsHeader, SDS_HEADER_LEN);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateUpCase()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_UPCASE, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LAYOUT.UpCaseLcn,
                                LAYOUT.UpCaseClusters,
                                sizeof(UPCASE_TABLE));

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateExtend()
{
    return CreateDirectoryRecord(METAFILE_EXTEND, L"$Extend");
}

//
// Reserved MFT records (12..15). Windows formats these as in-use stubs holding
// $STANDARD_INFORMATION and an empty resident unnamed $DATA - no $FILE_NAME,
// zero hard links, and their bits set in the $MFT bitmap. chkdsk repairs any
// other shape.
//
static
PFILE_RECORD_HEADER
CreateStub(IN DWORD32 MftRecordNumber)
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        NextAttribute;

    // In-use record with $STANDARD_INFORMATION; no name, so no $FILE_NAME is
    // added and HardLinkCount stays 0.
    FileRecord = NtfsCreateBlankFileRecord(MftRecordNumber, &NextAttribute);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for stub #%d file record!\n", MftRecordNumber);
        return NULL;
    }

    // Empty resident unnamed $DATA
    AddEmptyDataAttribute(FileRecord, NextAttribute);

    return FileRecord;
}


/* METAFILES ADDITIONAL DATA WRITERS ****************************************/

static NTSTATUS WriteMftBitmap()
{
    PBYTE Data = NULL;
    LARGE_INTEGER Offset;
    ULONG Size;
    ULONG i;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Size = LAYOUT.MftBitmapClusters * BYTES_PER_CLUSTER;

    // Allocate memory for the $MFT bitmap
    Data = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Data)
    {
        DPRINT1("ERROR: Unable to allocate memory for $MFT bitmap!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Data, Size);

    // Mark all system records (0..15) as allocated, including the reserved
    // stubs 12..15, which Windows keeps in-use.
    for (i = 0; i <= 15; i++)
    {
        Data[i / 8] |= (1 << (i % 8));
    }

    // Calculate offset
    Offset.QuadPart = (LONGLONG)LAYOUT.MftBitmapLcn * BYTES_PER_CLUSTER;

    // Write file
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Data,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write $MFT bitmap! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Data);
    return Status;
}

static NTSTATUS WriteMftMirr()
{
    BYTE MetafileIndex;
    METAFILE Metafile;
    PFILE_RECORD_HEADER FileRecord;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    // Clear the $MFTMirr area
    Status = WriteZerosToClusters(LAYOUT.MftMirrLcn,
                                  LAYOUT.MftMirrClusters);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to clear sectors for $MFTMirr! NtWriteFile() failed (Status %lx)\n", Status);
        return Status;
    }

    // Write the first MFT_MIRR_COUNT records to the mirror
    for (
        MetafileIndex = 0;
        MetafileIndex < MFT_MIRR_COUNT;
        MetafileIndex++
        )
    {
        Metafile = METAFILES[MetafileIndex];

        // Create metafile record or stub, if metafile is not implemented
        if (!Metafile.Constructor)
        {
            FileRecord = CreateStub(MetafileIndex);
        }
        else
        {
            FileRecord = Metafile.Constructor();
        }

        // Check file record
        if (!FileRecord)
        {
            DPRINT1(
                "ERROR: Unable to allocate memory for file record #%d!\n",
                MetafileIndex
            );

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Write metafile to disk
        Status = WriteMetafileMirror(FileRecord, &IoStatusBlock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1(
                "ERROR: Unable to write metafile mirror #%d to disk. NtWriteFile() failed (Status %lx)\n",
                MetafileIndex,
                Status
            );

            FREE(FileRecord);
            break;
        }

        // Free memory
        FREE(FileRecord);
    }

    return Status;
}

//
// The $LogFile content is filled with 0xFF. A driver treats an all-0xFF log as
// "not initialized" and rebuilds it on first mount (this is exactly what
// Windows format does).
//
static NTSTATUS WriteLogFile()
{
    return WritePatternToClusters(LAYOUT.LogFileLcn,
                                  LAYOUT.LogFileClusters,
                                  0xFF);
}

static NTSTATUS WriteAttributesTable()
{
    PBYTE Table;
    LARGE_INTEGER Offset;
    ULONG Size;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Offset.QuadPart = (LONGLONG)LAYOUT.AttrDefLcn * BYTES_PER_CLUSTER;
    Size = LAYOUT.AttrDefClusters * BYTES_PER_CLUSTER;

    // Allocate memory
    Table = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Table)
    {
        DPRINT1("ERROR: Unable to allocate memory for attributes table!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Clear memory
    RtlZeroMemory(Table, Size);

    // Copy table to memory
    RtlCopyBytes(Table, &ATTRIBUTES_TABLE, sizeof(ATTRIBUTES_TABLE));

    // Write table to disk
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Table,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write attributes table to disk! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Table);

    return Status;
}

//
// Writes the volume cluster allocation bitmap ($Bitmap): one bit per cluster,
// with the metadata clusters (and the non-existent trailing bits of the last
// byte) marked as used.
//
static NTSTATUS WriteVolumeBitmap()
{
    PBYTE Buffer;
    LARGE_INTEGER Offset;
    ULONG Size;
    ULONGLONG BitmapBits;
    ULONGLONG BitmapBytes;
    ULONGLONG c;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Size = LAYOUT.BitmapClusters * BYTES_PER_CLUSTER;
    BitmapBytes = (LAYOUT.ClusterCount + 7) / 8;
    BitmapBits  = BitmapBytes * 8;

    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Buffer)
    {
        DPRINT1("ERROR: Unable to allocate memory for volume bitmap!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Buffer, Size);

    // Mark the metadata clusters [0, FirstFreeLcn) as used.
    for (c = 0; c < LAYOUT.FirstFreeLcn; c++)
    {
        Buffer[c / 8] |= (1 << (c % 8));
    }

    // Mark the non-existent trailing bits in the final bitmap byte as used, so
    // they can never be allocated.
    for (c = LAYOUT.ClusterCount; c < BitmapBits; c++)
    {
        Buffer[c / 8] |= (1 << (c % 8));
    }

    Offset.QuadPart = (LONGLONG)LAYOUT.BitmapLcn * BYTES_PER_CLUSTER;

    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buffer,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write volume bitmap! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Buffer);

    return Status;
}

static NTSTATUS WriteUpCaseTable()
{
    PBYTE         Table;
    LARGE_INTEGER Offset;
    ULONG         Size;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Offset.QuadPart = (LONGLONG)LAYOUT.UpCaseLcn * BYTES_PER_CLUSTER;
    Size = LAYOUT.UpCaseClusters * BYTES_PER_CLUSTER;

    // Allocate memory
    Table = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Table)
    {
        DPRINT1("ERROR: Unable to allocate memory for upcase table!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Clear memory
    RtlZeroMemory(Table, Size);

    // Copy table to memory
    RtlCopyBytes(Table, &UPCASE_TABLE, sizeof(UPCASE_TABLE));

    // Write table to disk
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Table,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write upcase table to disk! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Table);

    return Status;
}

//
// Builds the root directory's single INDX block and writes it to RootIdxLcn.
// The block holds a filename index entry for each of the 12 in-use system
// files (records 0..11), in NTFS filename-collation order.
//
static NTSTATUS WriteRootIndex()
{
    // Collation (uppercased-filename) order. All "$" names (0x24) sort before
    // root's own "." (0x2E), which comes last. chkdsk expects root to be
    // indexed in its own directory too.
    static const BYTE RootOrder[] =
    {
        METAFILE_ATTRDEF, METAFILE_BADCLUS, METAFILE_BITMAP, METAFILE_BOOT,
        METAFILE_EXTEND,  METAFILE_LOGFILE, METAFILE_MFT,    METAFILE_MFTMIRR,
        METAFILE_SECURE,  METAFILE_UPCASE,  METAFILE_VOLUME, METAFILE_ROOT
    };

    PBYTE           Blk;
    ULONG           Size = LAYOUT.RootIdxClusters * BYTES_PER_CLUSTER;
    ULONG           P;
    ULONG           i;
    USHORT          UsaCount;
    USHORT          Usn;
    ULONG           Sector;
    LARGE_INTEGER   Offset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        Status;

    Blk = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Blk)
    {
        DPRINT1("ERROR: Unable to allocate memory for root INDX block!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Blk, Size);

    // INDX record header
    UsaCount = (USHORT)(Size / BYTES_PER_SECTOR + 1);
    RtlCopyMemory(Blk + 0x00, "INDX", 4);
    *(PUSHORT)(Blk + 0x04) = 0x28;       // USA offset
    *(PUSHORT)(Blk + 0x06) = UsaCount;   // USA count
    // 0x08 LSN = 0, 0x10 VCN of this INDX = 0 (already zeroed)

    // INDEX_HEADER at 0x18. Entries begin at 0x40 (after the USA).
    *(PULONG)(Blk + 0x18) = 0x28;          // FirstEntryOffset (relative to 0x18)
    *(PULONG)(Blk + 0x20) = Size - 0x18;   // AllocatedSize
    Blk[0x24] = 0;                         // Leaf node

    // Index entries
    P = 0x40;
    for (i = 0; i < ARR_SIZE(RootOrder); i++)
    {
        BYTE    Rec    = RootOrder[i];
        BOOLEAN IsDir  = (Rec == METAFILE_ROOT) || (Rec == METAFILE_EXTEND);
        ULONG   FnLen  = BuildFileNameValue(Blk + P + 0x10,
                                            METAFILES[Rec].Name,
                                            METAFILE_ROOT,
                                            METAFILE_FILE_ATTRIBUTES(IsDir),
                                            0, 0);
        ULONG   EntryLen = ALIGN_UP_BY(0x10 + FnLen, 8);

        *(PULONGLONG)(Blk + P + 0x00) = MFT_REFERENCE(Rec);
        *(PUSHORT)(Blk + P + 0x08)    = (USHORT)EntryLen;
        *(PUSHORT)(Blk + P + 0x0A)    = (USHORT)FnLen;
        *(PUSHORT)(Blk + P + 0x0C)    = 0;   // flags
        *(PUSHORT)(Blk + P + 0x0E)    = 0;

        P += EntryLen;
    }

    // End marker
    *(PUSHORT)(Blk + P + 0x08) = 0x10;
    *(PUSHORT)(Blk + P + 0x0A) = 0;
    *(PUSHORT)(Blk + P + 0x0C) = INDEX_ENTRY_END;
    P += 0x10;

    *(PULONG)(Blk + 0x1C) = P - 0x18;    // IndexLength

    // Apply the USA fixup (INDX blocks are fixed up just like FILE records).
    Usn = 1;
    *(PUSHORT)(Blk + 0x28) = Usn;
    for (Sector = 0; (Sector + 1) < UsaCount; Sector++)
    {
        PUSHORT Tail = (PUSHORT)(Blk + (Sector + 1) * BYTES_PER_SECTOR - sizeof(USHORT));
        *(PUSHORT)(Blk + 0x28 + (Sector + 1) * 2) = *Tail;
        *Tail = Usn;
    }

    // Write the INDX block
    Offset.QuadPart = (LONGLONG)LAYOUT.RootIdxLcn * BYTES_PER_CLUSTER;
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Blk,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write root INDX block! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Blk);

    return Status;
}

//
// Writes the $Secure:$SDS stream: the single security-descriptor entry at
// offset 0, plus a mirror copy 256 KiB later (whose header offset field points
// at the mirror location).
//
static NTSTATUS WriteSecureSds()
{
    BYTE            Sd[SD_LENGTH];
    ULONG           SdLen;
    ULONG           Hash;
    ULONG           EntryLen;
    PBYTE           Buf;
    ULONG           Size = LAYOUT.SdsClusters * BYTES_PER_CLUSTER;
    LARGE_INTEGER   Offset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        Status;

    SdLen    = BuildSecurityDescriptor(Sd);
    Hash     = SecurityHash(Sd, SdLen);
    EntryLen = SDS_HEADER_LEN + SdLen;

    Buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Buf)
    {
        DPRINT1("ERROR: Unable to allocate memory for $SDS!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Buf, Size);

    // Primary entry at offset 0
    BuildSdsHeader(Buf, Hash, 0, EntryLen);
    RtlCopyMemory(Buf + SDS_HEADER_LEN, Sd, SdLen);

    // Mirror copy 256 KiB later; its header offset field points at the mirror.
    RtlCopyMemory(Buf + NTFS_SDS_MIRROR, Buf, EntryLen);
    *(PULONGLONG)(Buf + NTFS_SDS_MIRROR + 0x08) = NTFS_SDS_MIRROR;

    Offset.QuadPart = (LONGLONG)LAYOUT.SdsLcn * BYTES_PER_CLUSTER;
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buf,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write $SDS! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Buf);

    return Status;
}
