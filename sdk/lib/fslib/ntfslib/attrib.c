/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/attrib.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include <ntfslib.h>

#define NDEBUG
#include <debug.h>


/* MACROSES ******************************************************************/

// Get resident attribute data address
#define RESIDENT_DATA(attr, type) ((type)((LONG_PTR)attr + RA_HEADER_LENGTH))


/* RUN-LIST ENCODING *********************************************************/

//
// Number of bytes needed for the minimal *signed* encoding of Value. NTFS
// mapping-pairs store both the run length and the LCN delta as signed values,
// so a value whose natural top bit is set must be widened by a byte.
//
static
ULONG
NtfsSignedByteCount(IN LONGLONG Value)
{
    ULONG    Bytes = 0;
    LONGLONG V = Value;

    if (Value == 0)
        return 0;

    do
    {
        Bytes++;
        V >>= 8;
    }
    while (!((V ==  0) && (((Value >> (Bytes * 8 - 1)) & 1) == 0)) &&
           !((V == -1) && (((Value >> (Bytes * 8 - 1)) & 1) == 1)));

    return Bytes;
}

//
// Encode a single data run (or sparse hole) into Out as NTFS mapping pairs,
// followed by the 0 terminator. The run is the first/only one, so the LCN is
// encoded as an absolute (delta from 0) value. Returns the number of bytes.
//
static
ULONG
NtfsEncodeSingleRun(OUT PBYTE     Out,
                    IN  ULONGLONG Lcn,
                    IN  ULONGLONG Length,
                    IN  BOOLEAN   IsHole)
{
    ULONG    LenBytes;
    ULONG    OffBytes = 0;
    ULONG    Pos = 0;
    ULONG    i;
    LONGLONG Delta = 0;

    // Length field (unsigned, but stored with a guaranteed-clear top bit).
    LenBytes = NtfsSignedByteCount((LONGLONG)Length);
    if (LenBytes == 0)
        LenBytes = 1;
    if ((Length >> (LenBytes * 8 - 1)) & 1)
        LenBytes++;

    // Offset field (signed LCN delta; 0 bytes for a hole).
    if (!IsHole)
    {
        Delta    = (LONGLONG)Lcn;   // Previous LCN is 0 for the first run
        OffBytes = NtfsSignedByteCount(Delta);
        if (OffBytes == 0)
            OffBytes = 1;
    }

    Out[Pos++] = (BYTE)((OffBytes << 4) | (LenBytes & 0x0F));
    for (i = 0; i < LenBytes; i++)
        Out[Pos++] = (BYTE)(Length >> (i * 8));
    for (i = 0; i < OffBytes; i++)
        Out[Pos++] = (BYTE)(((ULONGLONG)Delta) >> (i * 8));

    Out[Pos++] = 0;   // Run-list terminator

    return Pos;
}


/* FUNCTIONS *****************************************************************/

static
VOID
SetFileRecordEnd(OUT PFILE_RECORD_HEADER FileRecord,
                 OUT PATTR_RECORD        AttrEnd,
                 IN  ULONG               EndMarker)
{
    // Ensure AttrEnd is aligned on an 8-byte boundary, relative to FileRecord
    ASSERT(((ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord) % ATTR_RECORD_ALIGNMENT == 0);

    // Mark the end of attributes
    AttrEnd->Type = AttributeEnd;

    // Restore the "file-record-end marker." The value is never checked but this behavior is consistent with Win2k3.
    AttrEnd->Length = EndMarker;

    // Recalculate bytes in use
    FileRecord->BytesInUse = (ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord + sizeof(ULONG) * 2;
}

VOID
AddStandardInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                                OUT PATTR_RECORD        Attribute)
{
    PSTANDARD_INFORMATION StandardInfo;
    BOOLEAN               IsDirectory = (FileRecord->Flags & MFT_RECORD_IS_DIRECTORY) != 0;

    StandardInfo = RESIDENT_DATA(Attribute, PSTANDARD_INFORMATION);

    Attribute->Type     = AttributeStandardInformation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // Default setup for resident attribute
    Attribute->Length = sizeof(STANDARD_INFORMATION) + RA_HEADER_LENGTH;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = sizeof(STANDARD_INFORMATION);
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    // Set dates and times (a single timestamp captured at format start)
    StandardInfo->CreationTime   = FORMAT_TIME;
    StandardInfo->ChangeTime     = FORMAT_TIME;
    StandardInfo->LastWriteTime  = FORMAT_TIME;
    StandardInfo->LastAccessTime = FORMAT_TIME;
    StandardInfo->FileAttribute  = METAFILE_FILE_ATTRIBUTES(IsDirectory);
    StandardInfo->SecurityId     = NTFS_SECURITY_ID;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Builds a $FILE_NAME value at Out. Shared by the file record's own $FILE_NAME
// attribute and by the parent directory's index entries, so both stay
// byte-identical (which chkdsk requires). Returns the value length.
//
ULONG
BuildFileNameValue(OUT PBYTE     Out,
                   IN  LPCWSTR   Name,
                   IN  DWORD32   ParentRecordNumber,
                   IN  ULONG     FileAttributes,
                   IN  ULONGLONG AllocatedSize,
                   IN  ULONGLONG RealSize)
{
    PFILENAME_ATTRIBUTE FileNameAttribute = (PFILENAME_ATTRIBUTE)Out;
    ULONG NameLength = (ULONG)wcslen(Name);
    ULONG NameSize   = NameLength * sizeof(WCHAR);
    ULONG ValueLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + NameSize;

    RtlZeroMemory(Out, ValueLength);

    FileNameAttribute->DirectoryFileReferenceNumber = MFT_REFERENCE(ParentRecordNumber);

    FileNameAttribute->CreationTime   = FORMAT_TIME;
    FileNameAttribute->ChangeTime     = FORMAT_TIME;
    FileNameAttribute->LastWriteTime  = FORMAT_TIME;
    FileNameAttribute->LastAccessTime = FORMAT_TIME;

    FileNameAttribute->AllocatedSize  = AllocatedSize;
    FileNameAttribute->DataSize       = RealSize;
    FileNameAttribute->FileAttributes = FileAttributes;

    FileNameAttribute->NameLength = (BYTE)NameLength;
    // System names fit 8.3 and serve as both the Win32 and DOS name.
    FileNameAttribute->NameType   = FILE_NAME_WIN32_AND_DOS;
    RtlCopyMemory(FileNameAttribute->Name, Name, NameSize);

    return ValueLength;
}

VOID
AddFileNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                     OUT PATTR_RECORD        Attribute,
                     IN  LPCWSTR             FileName,
                     IN  DWORD32             ParentMftRecordNumber)
{
    BOOLEAN IsDirectory = (FileRecord->Flags & MFT_RECORD_IS_DIRECTORY) != 0;
    ULONG   ValueLength;

    Attribute->Type     = AttributeFileName;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    ValueLength = BuildFileNameValue(RESIDENT_DATA(Attribute, PBYTE),
                                     FileName,
                                     ParentMftRecordNumber,
                                     METAFILE_FILE_ATTRIBUTES(IsDirectory),
                                     0, 0);

    FileRecord->HardLinkCount++;

    // Setup for resident attribute
    Attribute->Length = RA_HEADER_LENGTH + ValueLength;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = ValueLength;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    Attribute->Resident.Flags = RA_INDEXED;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddEmptyDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                      OUT PATTR_RECORD        Attribute)
{
    Attribute->Type     = AttributeData;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->Length = RA_HEADER_LENGTH;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = 0;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    // For unnamed $DATA attributes, NameOffset equals header length
    Attribute->NameOffset = RA_HEADER_LENGTH;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

static
VOID
AddNonResidentAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                        OUT PATTR_RECORD            Attribute,
                        IN  ULONG                   AttributeType,
                        IN  ULONGLONG               Lcn,
                        IN  ULONG                   ClustersCount,
                        OPTIONAL IN ULONGLONG       DataSize)
{
    PBYTE Runs;
    ULONG RunsOffset = NONRES_HEADER_SIZE;   // 0x40 (unnamed, non-sparse)
    ULONG RunsLength;

    // Setup attribute
    Attribute->Type     = AttributeType;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->IsNonResident = 1;
    Attribute->Flags         = 0;

    // Unnamed attribute: name offset is 0 (Windows/chkdsk expect this).
    Attribute->NameLength = 0;
    Attribute->NameOffset = 0;

    Attribute->NonResident.LowestVCN       = 0;
    Attribute->NonResident.HighestVCN      = (ClustersCount - 1);
    Attribute->NonResident.DataRunsOffset  = (USHORT)RunsOffset;
    Attribute->NonResident.CompressionUnit = 0;

    Attribute->NonResident.AllocatedSize   = (LONGLONG)ClustersCount * BYTES_PER_CLUSTER;
    Attribute->NonResident.DataSize        = !DataSize ? Attribute->NonResident.AllocatedSize : (LONGLONG)DataSize;
    // NOTE: the "CompressedSize" struct field is the InitializedSize slot (0x38).
    Attribute->NonResident.CompressedSize  = !DataSize ? Attribute->NonResident.AllocatedSize : (LONGLONG)DataSize;

    // Encode the single (contiguous) data run.
    Runs = (PBYTE)((ULONG_PTR)Attribute + RunsOffset);
    RunsLength = NtfsEncodeSingleRun(Runs, Lcn, ClustersCount, FALSE);

    Attribute->Length = ALIGN_UP_BY(RunsOffset + RunsLength, ATTR_RECORD_ALIGNMENT);

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddNonResidentDataAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                            OUT PATTR_RECORD            Attribute,
                            IN  ULONGLONG               Lcn,
                            IN  ULONG                   ClustersCount,
                            OPTIONAL IN ULONGLONG       DataSize)
{
    AddNonResidentAttribute(FileRecord,
                            Attribute,
                            AttributeData,
                            Lcn,
                            ClustersCount,
                            DataSize);
}

VOID
AddMftBitmapAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                      OUT PATTR_RECORD            Attribute)
{
    // One bit per reserved MFT record, rounded up to bytes.
    ULONGLONG BitmapBytes = (LAYOUT.MftAllocRecords + 7) / 8;

    AddNonResidentAttribute(FileRecord,
                            Attribute,
                            AttributeBitmap,
                            LAYOUT.MftBitmapLcn,
                            LAYOUT.MftBitmapClusters,
                            BitmapBytes);
}

VOID
AddVolumeNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                       OUT PATTR_RECORD        Attribute,
                       OPTIONAL IN PUNICODE_STRING Label)
{
    ULONG ValueLength = 0;

    Attribute->Type     = AttributeVolumeName;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    if (Label && Label->Buffer && Label->Length > 0)
    {
        ValueLength = Label->Length;  // In bytes

        // NTFS volume labels are limited to 128 bytes (64 characters).
        if (ValueLength > 128)
            ValueLength = 128;

        RtlCopyMemory(RESIDENT_DATA(Attribute, PVOID), Label->Buffer, ValueLength);
    }

    Attribute->Length = RA_HEADER_LENGTH + ValueLength;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = ValueLength;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    Attribute->NameOffset = RA_HEADER_LENGTH;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddVolumeInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                              OUT PATTR_RECORD        Attribute,
                              IN  BYTE                MajorVersion,
                              IN  BYTE                MinorVersion)
{
    PVOLUME_INFORMATION_ATTRIBUTE VolumeInfo;

    Attribute->Type     = AttributeVolumeInformation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    VolumeInfo = RESIDENT_DATA(Attribute, PVOLUME_INFORMATION_ATTRIBUTE);

    VolumeInfo->MajorVersion = MajorVersion;
    VolumeInfo->MinorVersion = MinorVersion;
    VolumeInfo->Flags = 0;

    Attribute->Length = RA_HEADER_LENGTH + sizeof(VOLUME_INFORMATION_ATTRIBUTE);
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = sizeof(VOLUME_INFORMATION_ATTRIBUTE);
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Builds an empty index. The named $INDEX_ROOT attribute contains a single
// index node holding only the end-of-index marker, so the index is valid but
// empty (no $INDEX_ALLOCATION needed).
//
VOID
AddEmptyIndexRoot(OUT PFILE_RECORD_HEADER FileRecord,
                  OUT PATTR_RECORD        Attribute,
                  IN  LPCWSTR             Name,
                  IN  ULONG               IndexedType,
                  IN  ULONG               CollationRule)
{
    ULONG NameLength = (ULONG)wcslen(Name);
    ULONG NameSize   = NameLength * sizeof(WCHAR);

    PWCHAR                 NamePtr;
    ULONG                  NameOffset;
    ULONG                  ValueOffset;
    ULONG                  ValueLength;
    PINDEX_ROOT_ATTRIBUTE  IndexRoot;
    PINDEX_ENTRY_HEADER    EndEntry;
    ULONG                  ClustersPerIndexRecord;

    Attribute->Type     = AttributeIndexRoot;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // Named attribute: the name follows the resident header, value follows the name.
    NameOffset  = RA_HEADER_LENGTH;
    ValueOffset = ALIGN_UP_BY(NameOffset + NameSize, VALUE_OFFSET_ALIGNMENT);
    ValueLength = sizeof(INDEX_ROOT_ATTRIBUTE) + sizeof(INDEX_ENTRY_HEADER);

    Attribute->NameLength         = (BYTE)NameLength;
    Attribute->NameOffset         = (USHORT)NameOffset;
    Attribute->Resident.ValueLength = ValueLength;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;

    Attribute->Length = ALIGN_UP_BY(ValueOffset + ValueLength, ATTR_RECORD_ALIGNMENT);

    // Copy the attribute name
    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, Name, NameSize);

    // Fill the $INDEX_ROOT
    IndexRoot = (PINDEX_ROOT_ATTRIBUTE)((ULONG_PTR)Attribute + ValueOffset);

    if (BYTES_PER_CLUSTER <= INDEX_RECORD_SIZE)
        ClustersPerIndexRecord = INDEX_RECORD_SIZE / BYTES_PER_CLUSTER;
    else
        ClustersPerIndexRecord = 1;

    IndexRoot->AttributeType          = IndexedType;
    IndexRoot->CollationRule          = CollationRule;
    IndexRoot->SizeOfEntry            = INDEX_RECORD_SIZE;
    IndexRoot->ClustersPerIndexRecord = (BYTE)ClustersPerIndexRecord;

    // Index node header. Offsets are relative to the start of the header.
    IndexRoot->Header.FirstEntryOffset   = sizeof(INDEX_NODE_HEADER);
    IndexRoot->Header.TotalSizeOfEntries = sizeof(INDEX_NODE_HEADER) + sizeof(INDEX_ENTRY_HEADER);
    IndexRoot->Header.AllocatedSize      = sizeof(INDEX_NODE_HEADER) + sizeof(INDEX_ENTRY_HEADER);
    IndexRoot->Header.Flags              = 0;  // Small index (no $INDEX_ALLOCATION)

    // The single index entry: the end-of-index marker.
    EndEntry = (PINDEX_ENTRY_HEADER)((ULONG_PTR)&IndexRoot->Header +
                                     IndexRoot->Header.FirstEntryOffset);
    EndEntry->FileReference = 0;
    EndEntry->Length        = sizeof(INDEX_ENTRY_HEADER);
    EndEntry->KeyLength     = 0;
    EndEntry->Flags         = INDEX_ENTRY_END;
    EndEntry->Reserved      = 0;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddIndexRoot(OUT PFILE_RECORD_HEADER FileRecord,
             OUT PATTR_RECORD        Attribute)
{
    // Directory index: keyed on $FILE_NAME, collated as file names.
    AddEmptyIndexRoot(FileRecord, Attribute, L"$I30", AttributeFileName, COLLATION_FILE_NAME);
}

//
// A "large" directory $INDEX_ROOT ("$I30"): the node has no inline entries,
// just an end marker flagged LAST|HAS_SUBNODE that points at INDX VCN 0. The
// actual entries live in the $INDEX_ALLOCATION.
//
VOID
AddIndexRootLarge(OUT PFILE_RECORD_HEADER FileRecord,
                  OUT PATTR_RECORD        Attribute)
{
    static const WCHAR I30Name[] = L"$I30";
    const ULONG NameLength = 4;
    const ULONG NameSize   = NameLength * sizeof(WCHAR);

    // Value = INDEX_ROOT header (0x20) + an end entry with an 8-byte subnode VCN (0x18).
    const ULONG EndEntrySize = sizeof(INDEX_ENTRY_HEADER) + sizeof(ULONGLONG);   // 0x18
    ULONG ValueLength = sizeof(INDEX_ROOT_ATTRIBUTE) + EndEntrySize;             // 0x38

    ULONG NameOffset  = RA_HEADER_LENGTH;
    ULONG ValueOffset = ALIGN_UP_BY(NameOffset + NameSize, ATTR_RECORD_ALIGNMENT);
    ULONG ClustersPerIndexRecord;
    PWCHAR NamePtr;
    PINDEX_ROOT_ATTRIBUTE IndexRoot;
    PINDEX_ENTRY_HEADER   EndEntry;

    Attribute->Type     = AttributeIndexRoot;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->NameLength           = (BYTE)NameLength;
    Attribute->NameOffset           = (USHORT)NameOffset;
    Attribute->Resident.ValueLength = ValueLength;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;

    Attribute->Length = ALIGN_UP_BY(ValueOffset + ValueLength, ATTR_RECORD_ALIGNMENT);

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, I30Name, NameSize);

    IndexRoot = (PINDEX_ROOT_ATTRIBUTE)((ULONG_PTR)Attribute + ValueOffset);

    if (BYTES_PER_CLUSTER <= INDEX_RECORD_SIZE)
        ClustersPerIndexRecord = INDEX_RECORD_SIZE / BYTES_PER_CLUSTER;
    else
        ClustersPerIndexRecord = 1;

    IndexRoot->AttributeType          = AttributeFileName;
    IndexRoot->CollationRule          = COLLATION_FILE_NAME;
    IndexRoot->SizeOfEntry            = INDEX_RECORD_SIZE;
    IndexRoot->ClustersPerIndexRecord = (BYTE)ClustersPerIndexRecord;

    IndexRoot->Header.FirstEntryOffset   = sizeof(INDEX_NODE_HEADER);
    IndexRoot->Header.TotalSizeOfEntries = sizeof(INDEX_NODE_HEADER) + EndEntrySize;
    IndexRoot->Header.AllocatedSize      = sizeof(INDEX_NODE_HEADER) + EndEntrySize;
    IndexRoot->Header.Flags              = INDEX_NODE_LARGE;

    // End marker: LAST | HAS_SUBNODE, with an 8-byte subnode VCN (= 0) appended.
    EndEntry = (PINDEX_ENTRY_HEADER)((ULONG_PTR)&IndexRoot->Header +
                                     IndexRoot->Header.FirstEntryOffset);
    EndEntry->FileReference = 0;
    EndEntry->Length        = (USHORT)EndEntrySize;
    EndEntry->KeyLength     = 0;
    EndEntry->Flags         = INDEX_ENTRY_END | INDEX_ENTRY_NODE;
    EndEntry->Reserved      = 0;
    *(PULONGLONG)((ULONG_PTR)EndEntry + sizeof(INDEX_ENTRY_HEADER)) = 0;   // Subnode VCN

    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Non-resident $INDEX_ALLOCATION named "$I30".
//
VOID
AddIndexAllocationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                            OUT PATTR_RECORD        Attribute,
                            IN  ULONGLONG           Lcn,
                            IN  ULONG               Clusters)
{
    static const WCHAR I30Name[] = L"$I30";
    const ULONG NameLength = 4;
    const ULONG NameSize   = NameLength * sizeof(WCHAR);

    ULONG     NameOffset = NONRES_HEADER_SIZE;   // 0x40 (non-sparse)
    ULONG     RunsOffset = ALIGN_UP_BY(NameOffset + NameSize, ATTR_RECORD_ALIGNMENT);
    ULONGLONG Size       = (ULONGLONG)Clusters * BYTES_PER_CLUSTER;
    ULONG     RunsLength;
    PWCHAR    NamePtr;
    PBYTE     Runs;

    Attribute->Type     = AttributeIndexAllocation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->IsNonResident = 1;
    Attribute->Flags         = 0;
    Attribute->NameLength    = (BYTE)NameLength;
    Attribute->NameOffset    = (USHORT)NameOffset;

    Attribute->NonResident.LowestVCN       = 0;
    Attribute->NonResident.HighestVCN      = Clusters - 1;
    Attribute->NonResident.DataRunsOffset  = (USHORT)RunsOffset;
    Attribute->NonResident.CompressionUnit = 0;
    Attribute->NonResident.AllocatedSize   = (LONGLONG)Size;
    Attribute->NonResident.DataSize        = (LONGLONG)Size;
    Attribute->NonResident.CompressedSize  = (LONGLONG)Size;   // InitializedSize slot (0x38)

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, I30Name, NameSize);

    Runs = (PBYTE)((ULONG_PTR)Attribute + RunsOffset);
    RunsLength = NtfsEncodeSingleRun(Runs, Lcn, Clusters, FALSE);

    Attribute->Length = ALIGN_UP_BY(RunsOffset + RunsLength, ATTR_RECORD_ALIGNMENT);

    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Non-resident named $DATA stream (e.g. $Secure:$SDS).
//
VOID
AddNamedNonResidentDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                                 OUT PATTR_RECORD        Attribute,
                                 IN  LPCWSTR             Name,
                                 IN  ULONGLONG           Lcn,
                                 IN  ULONG               Clusters,
                                 IN  ULONGLONG           DataSize)
{
    ULONG     NameLength = (ULONG)wcslen(Name);
    ULONG     NameSize   = NameLength * sizeof(WCHAR);
    ULONG     NameOffset = NONRES_HEADER_SIZE;   // 0x40 (non-sparse)
    ULONG     RunsOffset = ALIGN_UP_BY(NameOffset + NameSize, ATTR_RECORD_ALIGNMENT);
    ULONGLONG Alloc      = (ULONGLONG)Clusters * BYTES_PER_CLUSTER;
    ULONG     RunsLength;
    PWCHAR    NamePtr;
    PBYTE     Runs;

    Attribute->Type     = AttributeData;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->IsNonResident = 1;
    Attribute->Flags         = 0;
    Attribute->NameLength    = (BYTE)NameLength;
    Attribute->NameOffset    = (USHORT)NameOffset;

    Attribute->NonResident.LowestVCN       = 0;
    Attribute->NonResident.HighestVCN      = Clusters - 1;
    Attribute->NonResident.DataRunsOffset  = (USHORT)RunsOffset;
    Attribute->NonResident.CompressionUnit = 0;
    Attribute->NonResident.AllocatedSize   = (LONGLONG)Alloc;
    Attribute->NonResident.DataSize        = (LONGLONG)DataSize;
    Attribute->NonResident.CompressedSize  = (LONGLONG)DataSize;   // InitializedSize slot (0x38)

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, Name, NameSize);

    Runs = (PBYTE)((ULONG_PTR)Attribute + RunsOffset);
    RunsLength = NtfsEncodeSingleRun(Runs, Lcn, Clusters, FALSE);

    Attribute->Length = ALIGN_UP_BY(RunsOffset + RunsLength, ATTR_RECORD_ALIGNMENT);

    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Resident named view-index $INDEX_ROOT holding one entry plus the end marker.
// A view-index entry differs from a filename entry: it starts with a data
// offset/length pair, and the data follows the key immediately (no padding).
//
VOID
AddViewIndexRoot(OUT PFILE_RECORD_HEADER FileRecord,
                 OUT PATTR_RECORD        Attribute,
                 IN  LPCWSTR             Name,
                 IN  ULONG               CollationRule,
                 IN  PVOID               Key,
                 IN  USHORT              KeyLength,
                 IN  PVOID               Data,
                 IN  USHORT              DataLength)
{
    ULONG  NameLength = (ULONG)wcslen(Name);
    ULONG  NameSize   = NameLength * sizeof(WCHAR);
    ULONG  NameOffset  = RA_HEADER_LENGTH;
    ULONG  ValueOffset = ALIGN_UP_BY(NameOffset + NameSize, ATTR_RECORD_ALIGNMENT);

    USHORT DataOffset = (USHORT)(0x10 + KeyLength);            // header(0x10) + key
    USHORT EntryLen   = (USHORT)ALIGN_UP_BY(DataOffset + DataLength, ATTR_RECORD_ALIGNMENT);
    ULONG  IndexLen   = 0x10 + (ULONG)EntryLen + 0x10;        // header + entry + end
    ULONG  ValueLength = 0x20 + (ULONG)EntryLen + 0x10;
    ULONG  ClustersPerIndexRecord;
    PWCHAR NamePtr;
    PBYTE  V;
    PBYTE  E;

    Attribute->Type     = AttributeIndexRoot;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->NameLength           = (BYTE)NameLength;
    Attribute->NameOffset           = (USHORT)NameOffset;
    Attribute->Resident.ValueLength = ValueLength;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;

    Attribute->Length = ALIGN_UP_BY(ValueOffset + ValueLength, ATTR_RECORD_ALIGNMENT);

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, Name, NameSize);

    if (BYTES_PER_CLUSTER <= INDEX_RECORD_SIZE)
        ClustersPerIndexRecord = INDEX_RECORD_SIZE / BYTES_PER_CLUSTER;
    else
        ClustersPerIndexRecord = 1;

    V = (PBYTE)((ULONG_PTR)Attribute + ValueOffset);
    RtlZeroMemory(V, ValueLength);

    // INDEX_ROOT header (view index: indexed attribute type 0)
    *(PULONG)(V + 0x00) = 0;
    *(PULONG)(V + 0x04) = CollationRule;
    *(PULONG)(V + 0x08) = INDEX_RECORD_SIZE;
    V[0x0C] = (BYTE)ClustersPerIndexRecord;

    // INDEX_HEADER at 0x10
    *(PULONG)(V + 0x10) = 0x10;         // FirstEntryOffset
    *(PULONG)(V + 0x14) = IndexLen;
    *(PULONG)(V + 0x18) = IndexLen;
    V[0x1C] = 0;                        // Small index

    // Entry 0 (view index entry) at 0x20
    E = V + 0x20;
    *(PUSHORT)(E + 0x00) = DataOffset;
    *(PUSHORT)(E + 0x02) = DataLength;
    *(PULONG)(E + 0x04)  = 0;
    *(PUSHORT)(E + 0x08) = EntryLen;
    *(PUSHORT)(E + 0x0A) = KeyLength;
    *(PUSHORT)(E + 0x0C) = 0;           // Flags
    *(PUSHORT)(E + 0x0E) = 0;
    RtlCopyMemory(E + 0x10, Key, KeyLength);
    RtlCopyMemory(E + DataOffset, Data, DataLength);

    // End marker
    E = V + 0x20 + EntryLen;
    *(PUSHORT)(E + 0x08) = 0x10;
    *(PUSHORT)(E + 0x0A) = 0;
    *(PUSHORT)(E + 0x0C) = INDEX_ENTRY_END;

    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// Resident named $BITMAP attribute holding the supplied bytes (e.g. the
// directory index bitmap "$I30").
//
VOID
AddIndexBitmapAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                        OUT PATTR_RECORD        Attribute,
                        IN  LPCWSTR             Name,
                        IN  PVOID               Bits,
                        IN  ULONG               BitsLength)
{
    ULONG  NameLength = (ULONG)wcslen(Name);
    ULONG  NameSize   = NameLength * sizeof(WCHAR);
    ULONG  NameOffset = RA_HEADER_LENGTH;
    ULONG  ValueOffset = ALIGN_UP_BY(NameOffset + NameSize, ATTR_RECORD_ALIGNMENT);
    PWCHAR NamePtr;

    Attribute->Type     = AttributeBitmap;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->NameLength = (BYTE)NameLength;
    Attribute->NameOffset = (USHORT)NameOffset;

    Attribute->Resident.ValueLength = BitsLength;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;

    Attribute->Length = ALIGN_UP_BY(ValueOffset + BitsLength, ATTR_RECORD_ALIGNMENT);

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, Name, NameSize);

    RtlCopyMemory((PBYTE)((ULONG_PTR)Attribute + ValueOffset), Bits, BitsLength);

    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddNamedEmptyDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                           OUT PATTR_RECORD        Attribute,
                           IN  LPCWSTR             Name)
{
    ULONG  NameLength = (ULONG)wcslen(Name);
    ULONG  NameSize   = NameLength * sizeof(WCHAR);
    ULONG  NameOffset = RA_HEADER_LENGTH;
    ULONG  ValueOffset = ALIGN_UP_BY(NameOffset + NameSize, VALUE_OFFSET_ALIGNMENT);
    PWCHAR NamePtr;

    Attribute->Type     = AttributeData;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->NameLength = (BYTE)NameLength;
    Attribute->NameOffset = (USHORT)NameOffset;

    Attribute->Resident.ValueLength = 0;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;

    Attribute->Length = ALIGN_UP_BY(ValueOffset, ATTR_RECORD_ALIGNMENT);

    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, Name, NameSize);

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

//
// $BadClus:$Bad - a sparse $DATA stream that logically spans the whole volume
// but has no clusters allocated (a single hole run). This is how a freshly
// formatted volume says "there are no bad clusters".
//
VOID
AddBadClusterDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                           OUT PATTR_RECORD        Attribute)
{
    static const WCHAR BadName[] = L"$Bad";
    const ULONG NameLength = 4;
    const ULONG NameSize   = NameLength * sizeof(WCHAR);

    ULONGLONG ClusterCount = LAYOUT.ClusterCount;
    ULONGLONG DataSize     = ClusterCount * BYTES_PER_CLUSTER;
    ULONG     NameOffset   = NONRES_HEADER_SIZE;                        // 0x40 (non-sparse)
    ULONG     RunsOffset   = ALIGN_UP_BY(NameOffset + NameSize, DATA_RUN_ALIGNMENT);
    ULONG     RunsLength;
    PWCHAR    NamePtr;
    PBYTE     Run;

    Attribute->Type     = AttributeData;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // Windows encodes $BadClus:$Bad as a plain (non-sparse) non-resident
    // attribute whose single mapping-pair is a hole. It does NOT set the SPARSE
    // flag; allocated and real size are the whole volume, initialized size is 0.
    Attribute->IsNonResident = 1;
    Attribute->Flags         = 0;
    Attribute->NameLength    = (BYTE)NameLength;
    Attribute->NameOffset    = (USHORT)NameOffset;

    Attribute->NonResident.LowestVCN       = 0;
    Attribute->NonResident.HighestVCN      = ClusterCount - 1;
    Attribute->NonResident.DataRunsOffset  = (USHORT)RunsOffset;
    Attribute->NonResident.CompressionUnit = 0;
    Attribute->NonResident.AllocatedSize   = (LONGLONG)DataSize;
    Attribute->NonResident.DataSize        = (LONGLONG)DataSize;
    Attribute->NonResident.CompressedSize  = 0;   // This field is InitializedSize (0x38)

    // Copy the attribute name ("$Bad")
    NamePtr = (PWCHAR)((ULONG_PTR)Attribute + NameOffset);
    RtlCopyMemory(NamePtr, BadName, NameSize);

    // A single hole data run covering the whole volume (offset field absent).
    Run = (PBYTE)((ULONG_PTR)Attribute + RunsOffset);
    RunsLength = NtfsEncodeSingleRun(Run, 0, ClusterCount, TRUE);

    Attribute->Length = ALIGN_UP_BY(RunsOffset + RunsLength, ATTR_RECORD_ALIGNMENT);

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}
