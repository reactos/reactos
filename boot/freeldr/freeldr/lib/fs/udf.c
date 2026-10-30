/*
 * PROJECT: FreeLoader
 * LICENSE: GPL-2.0-or-later
 * PURPOSE: Read-only UDF 2.50 metadata-partition filesystem access
 *
 * This reader handles the recorded short/long allocation descriptors used
 * by the Windows UDF 2.50 hard-disk formatter and by ReactOS UDFS. It is
 * intentionally read-only and rejects unsupported partition maps and sparse
 * extents rather than guessing their location.
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(FILESYSTEM);

#define UDF_BLOCK_SIZE 512
#define UDF_MAX_EXTENTS 256
#define UDF_MAX_DIRECTORY (1024 * 1024)
#define TAG_UDF_VOLUME 'VfdU'
#define TAG_UDF_FILE 'FfdU'
#define TAG_UDF_DIRECTORY 'DfdU'

typedef struct _UDF_EXTENT
{
    ULONG Length;
    ULONG Block;
    USHORT PartitionRef;
} UDF_EXTENT;

typedef struct _UDF_VOLUME
{
    ULONG DeviceId;
    ULONG PartitionStart;
    ULONG PartitionBlocks;
    ULONG MetadataCount;
    UDF_EXTENT Metadata[UDF_MAX_EXTENTS];
    UDF_EXTENT RootIcb;
} UDF_VOLUME;

typedef struct _UDF_FILE
{
    ULONGLONG Size;
    ULONGLONG Position;
    ULONG Attributes;
    ULONG ExtentCount;
    UDF_EXTENT Extents[UDF_MAX_EXTENTS];
    ULONG InlineLength;
    UCHAR Inline[UDF_BLOCK_SIZE];
    CHAR Name[256];
} UDF_FILE;

static UDF_VOLUME* UdfVolumes[MAX_FDS];

static USHORT
Udf16(const UCHAR* Buffer)
{
    return (USHORT)(Buffer[0] | ((USHORT)Buffer[1] << 8));
}

static ULONG
Udf32(const UCHAR* Buffer)
{
    return (ULONG)Udf16(Buffer) | ((ULONG)Udf16(Buffer + 2) << 16);
}

static ULONGLONG
Udf64(const UCHAR* Buffer)
{
    return (ULONGLONG)Udf32(Buffer) | ((ULONGLONG)Udf32(Buffer + 4) << 32);
}

static BOOLEAN
UdfReadPhysicalBlock(UDF_VOLUME* Volume, ULONG Block, UCHAR* Buffer)
{
    LARGE_INTEGER Position;
    ULONG Count;

    Position.QuadPart = (ULONGLONG)Block * UDF_BLOCK_SIZE;
    if (ArcSeek(Volume->DeviceId, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    return (ArcRead(Volume->DeviceId, Buffer, UDF_BLOCK_SIZE, &Count) == ESUCCESS &&
            Count == UDF_BLOCK_SIZE);
}

static BOOLEAN
UdfMapBlock(UDF_VOLUME* Volume, USHORT PartitionRef, ULONG Block, ULONG* Physical)
{
    ULONG Index;
    ULONGLONG Offset = Block;

    if (PartitionRef == 0)
    {
        if (Block >= Volume->PartitionBlocks)
            return FALSE;
        *Physical = Volume->PartitionStart + Block;
        return TRUE;
    }
    if (PartitionRef != 1)
        return FALSE;

    for (Index = 0; Index < Volume->MetadataCount; ++Index)
    {
        UDF_EXTENT* Extent = &Volume->Metadata[Index];
        ULONG Blocks = Extent->Length / UDF_BLOCK_SIZE;
        if (Offset < Blocks)
        {
            if (Extent->PartitionRef != 0 || Extent->Block + Offset >= Volume->PartitionBlocks)
                return FALSE;
            *Physical = Volume->PartitionStart + Extent->Block + (ULONG)Offset;
            return TRUE;
        }
        Offset -= Blocks;
    }
    return FALSE;
}

static BOOLEAN
UdfReadLogicalBlock(UDF_VOLUME* Volume, USHORT PartitionRef, ULONG Block, UCHAR* Buffer)
{
    ULONG Physical;
    return (UdfMapBlock(Volume, PartitionRef, Block, &Physical) &&
            UdfReadPhysicalBlock(Volume, Physical, Buffer));
}

static BOOLEAN
UdfGetLongAd(const UCHAR* Buffer, UDF_EXTENT* Extent)
{
    ULONG Length = Udf32(Buffer);
    if (Length >> 30)
        return FALSE;
    Extent->Length = Length;
    Extent->Block = Udf32(Buffer + 4);
    Extent->PartitionRef = Udf16(Buffer + 8);
    return TRUE;
}

static BOOLEAN
UdfParseAds(UDF_VOLUME* Volume, UDF_FILE* File, const UCHAR* Buffer,
            ULONG Start, ULONG Length, ULONG Mode, USHORT ParentRef, ULONG Depth)
{
    ULONG Offset, Step = (Mode == 0) ? 8 : 16, Tail;
    BOOLEAN Continued = FALSE;

    if (Depth >= 32 || Start > UDF_BLOCK_SIZE || Length > UDF_BLOCK_SIZE - Start)
        return FALSE;

    /* Earlier UDFS builds counted the zero padding in front of a
     * continuation descriptor in L_AD (for example 296 bytes after an
     * Extended File Entry). Accept only a zero tail shorter than one
     * descriptor, and only when the block continues elsewhere. */
    Tail = Length % Step;
    if (Tail)
    {
        for (Offset = Start + Length - Tail; Offset < Start + Length; ++Offset)
        {
            if (Buffer[Offset])
                return FALSE;
        }
        Length -= Tail;
    }

    for (Offset = Start; Offset < Start + Length; Offset += Step)
    {
        UDF_EXTENT Extent;
        ULONG Value = Udf32(Buffer + Offset);
        ULONG Kind = Value >> 30;

        Extent.Length = Value & 0x3fffffff;
        Extent.Block = Udf32(Buffer + Offset + 4);
        Extent.PartitionRef = (Mode == 0) ? ParentRef : Udf16(Buffer + Offset + 8);
        if (Extent.Length == 0)
            continue;

        if (Kind == 0)
        {
            if (File->ExtentCount >= UDF_MAX_EXTENTS)
                return FALSE;
            File->Extents[File->ExtentCount++] = Extent;
        }
        else if (Kind == 3)
        {
            UCHAR Continuation[UDF_BLOCK_SIZE];
            ULONG NextLength;
            if (!UdfReadLogicalBlock(Volume, Extent.PartitionRef, Extent.Block, Continuation) ||
                Udf16(Continuation) != 258)
                return FALSE;
            NextLength = Udf32(Continuation + 20);
            if (!UdfParseAds(Volume, File, Continuation, 24, NextLength,
                             Mode, ParentRef, Depth + 1))
                return FALSE;
            Continued = TRUE;
        }
        else
        {
            /* Unrecorded and sparse extents are outside the boot profile. */
            return FALSE;
        }
    }
    return (Tail == 0 || Continued);
}

static BOOLEAN
UdfLoadFile(UDF_VOLUME* Volume, const UDF_EXTENT* Icb, UDF_FILE* File)
{
    UCHAR Entry[UDF_BLOCK_SIZE];
    ULONG Tag, AttrOffset, AllocOffset, DataOffset, Start, Length, Mode;

    if (Icb->Length < UDF_BLOCK_SIZE ||
        !UdfReadLogicalBlock(Volume, Icb->PartitionRef, Icb->Block, Entry))
        return FALSE;
    Tag = Udf16(Entry);
    if (Tag == 266)
    {
        AttrOffset = 208;
        AllocOffset = 212;
        DataOffset = 216;
    }
    else if (Tag == 261)
    {
        AttrOffset = 168;
        AllocOffset = 172;
        DataOffset = 176;
    }
    else
        return FALSE;

    RtlZeroMemory(File, sizeof(*File));
    File->Size = Udf64(Entry + 56);
    File->Attributes = ReadOnlyFile;
    if (Entry[27] == 4)
        File->Attributes |= DirectoryFile;
    else if (Entry[27] != 5 && Entry[27] != 250 && Entry[27] != 251)
        return FALSE;

    Mode = Udf16(Entry + 34) & 7;
    Length = Udf32(Entry + AllocOffset);
    Start = DataOffset + Udf32(Entry + AttrOffset);
    if (Start > UDF_BLOCK_SIZE || Length > UDF_BLOCK_SIZE - Start)
        return FALSE;

    if (Mode == 3)
    {
        File->InlineLength = Length;
        RtlCopyMemory(File->Inline, Entry + Start, Length);
        return (File->Size <= Length);
    }
    if (Mode != 0 && Mode != 1)
        return FALSE;
    return UdfParseAds(Volume, File, Entry, Start, Length, Mode,
                       Icb->PartitionRef, 0);
}

static BOOLEAN
UdfReadFileRange(UDF_VOLUME* Volume, UDF_FILE* File,
                 ULONGLONG Offset, ULONG Length, UCHAR* Output)
{
    ULONG Index;
    UCHAR Sector[UDF_BLOCK_SIZE];

    if (Offset > File->Size || Length > File->Size - Offset)
        return FALSE;
    if (File->InlineLength)
    {
        if (Offset + Length > File->InlineLength)
            return FALSE;
        RtlCopyMemory(Output, File->Inline + (ULONG)Offset, Length);
        return TRUE;
    }

    for (Index = 0; Index < File->ExtentCount && Length; ++Index)
    {
        UDF_EXTENT* Extent = &File->Extents[Index];
        if (Offset >= Extent->Length)
        {
            Offset -= Extent->Length;
            continue;
        }
        while (Length && Offset < Extent->Length)
        {
            ULONG InBlock = (ULONG)(Offset % UDF_BLOCK_SIZE);
            ULONG Transfer = min(Length, UDF_BLOCK_SIZE - InBlock);
            Transfer = min(Transfer, Extent->Length - (ULONG)Offset);
            if (!UdfReadLogicalBlock(Volume, Extent->PartitionRef,
                                     Extent->Block + (ULONG)(Offset / UDF_BLOCK_SIZE),
                                     Sector))
                return FALSE;
            RtlCopyMemory(Output, Sector + InBlock, Transfer);
            Output += Transfer;
            Length -= Transfer;
            Offset += Transfer;
        }
        Offset = 0;
    }
    return Length == 0;
}

static BOOLEAN
UdfDecodeName(const UCHAR* Source, ULONG Length, CHAR* Target, ULONG Capacity)
{
    ULONG Index, Count;
    if (!Length || !Capacity)
        return FALSE;
    if (Source[0] == 8)
    {
        Count = Length - 1;
        if (Count >= Capacity)
            return FALSE;
        for (Index = 0; Index < Count; ++Index)
            Target[Index] = (CHAR)Source[Index + 1];
    }
    else if (Source[0] == 16)
    {
        if ((Length - 1) % 2)
            return FALSE;
        Count = (Length - 1) / 2;
        if (Count >= Capacity)
            return FALSE;
        for (Index = 0; Index < Count; ++Index)
        {
            if (Source[1 + Index * 2] != 0)
                return FALSE;
            Target[Index] = (CHAR)Source[2 + Index * 2];
        }
    }
    else
        return FALSE;
    Target[Count] = ANSI_NULL;
    return TRUE;
}

static BOOLEAN
UdfFindChild(UDF_VOLUME* Volume, const UDF_EXTENT* DirectoryIcb,
             PCSTR Name, UDF_EXTENT* Found)
{
    UDF_FILE* Directory;
    UCHAR* Data;
    ULONG Size, Offset;
    BOOLEAN Result = FALSE;

    Directory = FrLdrTempAlloc(sizeof(*Directory), TAG_UDF_FILE);
    if (!Directory)
        return FALSE;
    if (!UdfLoadFile(Volume, DirectoryIcb, Directory) ||
        !(Directory->Attributes & DirectoryFile) ||
        Directory->Size > UDF_MAX_DIRECTORY)
        goto FinishFile;

    Size = (ULONG)Directory->Size;
    Data = FrLdrTempAlloc(max(Size, 1), TAG_UDF_DIRECTORY);
    if (!Data)
        goto FinishFile;
    if (!UdfReadFileRange(Volume, Directory, 0, Size, Data))
        goto FinishData;

    for (Offset = 0; Offset < Size; )
    {
        UCHAR* Fid = Data + Offset;
        ULONG ImpLength, NameLength, RecordLength;
        CHAR Decoded[256];

        if (Size - Offset < 38 || Udf16(Fid) != 257)
            break;
        ImpLength = Udf16(Fid + 36);
        NameLength = Fid[19];
        RecordLength = (38 + ImpLength + NameLength + 3) & ~3;
        if (!RecordLength || RecordLength > Size - Offset)
            break;
        if (!(Fid[18] & (4 | 8)) &&
            UdfDecodeName(Fid + 38 + ImpLength, NameLength,
                          Decoded, sizeof(Decoded)) &&
            !_stricmp(Decoded, Name))
        {
            Result = UdfGetLongAd(Fid + 20, Found);
            break;
        }
        Offset += RecordLength;
    }

FinishData:
    FrLdrTempFree(Data, TAG_UDF_DIRECTORY);
FinishFile:
    FrLdrTempFree(Directory, TAG_UDF_FILE);
    return Result;
}

static BOOLEAN
UdfLookup(UDF_VOLUME* Volume, PCSTR Path, UDF_EXTENT* Found, CHAR* FinalName)
{
    UDF_EXTENT Current = Volume->RootIcb;
    CHAR Component[256];
    ULONG Length;

    while (*Path == '\\' || *Path == '/')
        ++Path;
    if (!*Path)
        return FALSE;

    while (*Path)
    {
        Length = 0;
        while (Path[Length] && Path[Length] != '\\' && Path[Length] != '/')
        {
            if (Length + 1 >= sizeof(Component))
                return FALSE;
            Component[Length] = Path[Length];
            ++Length;
        }
        Component[Length] = ANSI_NULL;
        if (!UdfFindChild(Volume, &Current, Component, &Current))
            return FALSE;
        RtlCopyMemory(FinalName, Component, Length + 1);
        Path += Length;
        while (*Path == '\\' || *Path == '/')
            ++Path;
    }
    *Found = Current;
    return TRUE;
}

static ARC_STATUS
UdfClose(ULONG FileId)
{
    FrLdrTempFree(FsGetDeviceSpecific(FileId), TAG_UDF_FILE);
    return ESUCCESS;
}

static ARC_STATUS
UdfGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    UDF_FILE* File = FsGetDeviceSpecific(FileId);
    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = File->Size;
    Information->CurrentAddress.QuadPart = File->Position;
    Information->Attributes = File->Attributes;
    Information->FileNameLength = min((ULONG)strlen(File->Name),
                                      sizeof(Information->FileName) - 1);
    RtlCopyMemory(Information->FileName, File->Name, Information->FileNameLength + 1);
    return ESUCCESS;
}

static ARC_STATUS
UdfOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    UDF_VOLUME* Volume = UdfVolumes[FsGetDeviceId(*FileId)];
    UDF_EXTENT Icb;
    UDF_FILE* File;
    CHAR Name[256];

    if (OpenMode != OpenReadOnly)
        return EACCES;
    if (!UdfLookup(Volume, Path, &Icb, Name))
        return ENOENT;
    File = FrLdrTempAlloc(sizeof(*File), TAG_UDF_FILE);
    if (!File)
        return ENOMEM;
    if (!UdfLoadFile(Volume, &Icb, File))
    {
        FrLdrTempFree(File, TAG_UDF_FILE);
        return EIO;
    }
    RtlCopyMemory(File->Name, Name, strlen(Name) + 1);
    FsSetDeviceSpecific(*FileId, File);
    return ESUCCESS;
}

static ARC_STATUS
UdfRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    UDF_FILE* File = FsGetDeviceSpecific(FileId);
    UDF_VOLUME* Volume = UdfVolumes[FsGetDeviceId(FileId)];

    *Count = 0;
    if (File->Position >= File->Size)
        return ESUCCESS;
    N = (ULONG)min((ULONGLONG)N, File->Size - File->Position);
    if (!UdfReadFileRange(Volume, File, File->Position, N, Buffer))
        return EIO;
    File->Position += N;
    *Count = N;
    return ESUCCESS;
}

static ARC_STATUS
UdfSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE Mode)
{
    UDF_FILE* File = FsGetDeviceSpecific(FileId);
    LONGLONG NewPosition = Position->QuadPart;

    if (Mode == SeekRelative)
        NewPosition += File->Position;
    else if (Mode != SeekAbsolute)
        return EINVAL;
    if (NewPosition < 0 || (ULONGLONG)NewPosition > File->Size)
        return EINVAL;
    File->Position = (ULONGLONG)NewPosition;
    return ESUCCESS;
}

ULONGLONG
UdfGetVolumeSize(ULONG DeviceId)
{
    UDF_VOLUME* Volume = UdfVolumes[DeviceId];
    ASSERT(Volume);
    return (ULONGLONG)Volume->PartitionBlocks * UDF_BLOCK_SIZE;
}

static const DEVVTBL UdfFuncTable =
{
    UdfClose,
    UdfGetFileInformation,
    UdfOpen,
    UdfRead,
    UdfSeek,
    L"udfs",
};

const DEVVTBL*
UdfMount(ULONG DeviceId)
{
    UDF_VOLUME* Volume;
    UDF_FILE* Metadata;
    UCHAR Block[UDF_BLOCK_SIZE], Lvd[UDF_BLOCK_SIZE], Fileset[UDF_BLOCK_SIZE];
    ULONG MainLength, MainStart, Index, MapOffset, MapLength;
    USHORT PartitionNumber = 0;
    BOOLEAN HavePartition = FALSE, HaveLvd = FALSE;
    UDF_EXTENT FilesetIcb, MetadataIcb;

    if (DeviceId >= MAX_FDS)
        return NULL;
    Volume = FrLdrTempAlloc(sizeof(*Volume), TAG_UDF_VOLUME);
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(*Volume));
    Volume->DeviceId = DeviceId;

    if (!UdfReadPhysicalBlock(Volume, 256, Block) || Udf16(Block) != 2)
        goto Failure;
    MainLength = Udf32(Block + 16);
    MainStart = Udf32(Block + 20);
    if (!MainLength || MainLength > 256 * UDF_BLOCK_SIZE)
        goto Failure;
    for (Index = 0; Index < (MainLength + UDF_BLOCK_SIZE - 1) / UDF_BLOCK_SIZE; ++Index)
    {
        if (!UdfReadPhysicalBlock(Volume, MainStart + Index, Block))
            goto Failure;
        if (Udf16(Block) == 5)
        {
            PartitionNumber = Udf16(Block + 22);
            Volume->PartitionStart = Udf32(Block + 188);
            Volume->PartitionBlocks = Udf32(Block + 192);
            HavePartition = TRUE;
        }
        else if (Udf16(Block) == 6)
        {
            RtlCopyMemory(Lvd, Block, UDF_BLOCK_SIZE);
            HaveLvd = TRUE;
        }
    }
    if (!HavePartition || !HaveLvd || !Volume->PartitionBlocks ||
        Udf32(Lvd + 212) != UDF_BLOCK_SIZE || Udf32(Lvd + 268) != 2)
        goto Failure;

    MapLength = Udf32(Lvd + 264);
    if (MapLength != 70 || Lvd[440] != 1 || Lvd[441] != 6 ||
        Udf16(Lvd + 444) != PartitionNumber)
        goto Failure;
    MapOffset = 446;
    if (Lvd[MapOffset] != 2 || Lvd[MapOffset + 1] != 64 ||
        Udf16(Lvd + MapOffset + 38) != PartitionNumber ||
        !RtlEqualMemory(Lvd + MapOffset + 5, "*UDF Metadata Partition", 23))
        goto Failure;

    Metadata = FrLdrTempAlloc(sizeof(*Metadata), TAG_UDF_FILE);
    if (!Metadata)
        goto Failure;
    if (!UdfReadPhysicalBlock(Volume,
                              Volume->PartitionStart + Udf32(Lvd + MapOffset + 40),
                              Block))
        goto FailureMetadata;
    MetadataIcb.Length = UDF_BLOCK_SIZE;
    MetadataIcb.Block = Udf32(Lvd + MapOffset + 40);
    MetadataIcb.PartitionRef = 0;
    if (Udf16(Block) != 266 || Block[27] != 250 ||
        !UdfLoadFile(Volume, &MetadataIcb, Metadata))
        goto FailureMetadata;
    if (!Metadata->ExtentCount || Metadata->ExtentCount > UDF_MAX_EXTENTS)
        goto FailureMetadata;
    for (Index = 0; Index < Metadata->ExtentCount; ++Index)
    {
        if (Metadata->Extents[Index].PartitionRef != 0 ||
            (Metadata->Extents[Index].Length % UDF_BLOCK_SIZE) != 0)
            goto FailureMetadata;
        Volume->Metadata[Index] = Metadata->Extents[Index];
    }
    Volume->MetadataCount = Metadata->ExtentCount;
    FrLdrTempFree(Metadata, TAG_UDF_FILE);

    if (!UdfGetLongAd(Lvd + 248, &FilesetIcb) ||
        FilesetIcb.Length < UDF_BLOCK_SIZE ||
        !UdfReadLogicalBlock(Volume, FilesetIcb.PartitionRef,
                             FilesetIcb.Block, Fileset) ||
        Udf16(Fileset) != 256 ||
        !UdfGetLongAd(Fileset + 400, &Volume->RootIcb))
        goto Failure;

    UdfVolumes[DeviceId] = Volume;
    TRACE("Mounted UDF 2.50 metadata partition on device %lu\n", DeviceId);
    return &UdfFuncTable;

FailureMetadata:
    FrLdrTempFree(Metadata, TAG_UDF_FILE);
Failure:
    FrLdrTempFree(Volume, TAG_UDF_VOLUME);
    return NULL;
}
