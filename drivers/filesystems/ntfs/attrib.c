/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/filesystems/ntfs/attrib.c
 * PURPOSE:     NTFS filesystem driver
 * COPYRIGHT:   Copyright (C) 2002,2003 ReactOS Team
 *              See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Eric Kohl
 *              Valentin Verkhovsky
 *              Hervé Poussineau (hpoussin@reactos.org)
 *              Pierre Schweitzer (pierre@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"
#include <ntintsafe.h>

#define NDEBUG
#include <debug.h>

/* Set to 1 to dump each data run list as it is encoded. Uses DbgPrint, so it
 * prints whatever NDEBUG is set to. */
#define NTFS_DUMP_DATA_RUNS 0

/* FUNCTIONS ****************************************************************/

/**
* @name AddBitmap
* @implemented
*
* Adds a $BITMAP attribute to a given FileRecord.
*
* @param Vcb
* Pointer to an NTFS_VCB for the destination volume.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $INDEX_ALLOCATION attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param Name
* Pointer to a string of 16-bit Unicode characters naming the attribute. Most often L"$I30".
*
* @param NameLength
* The number of wide-characters in the name. L"$I30" Would use 4 here.
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record, or if the file record isn't large enough for the attribute.
*
* @remarks
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* This could be improved by adding an $ATTRIBUTE_LIST to the file record if there's not enough space.
*
*/
NTSTATUS
AddBitmap(PNTFS_VCB Vcb,
          PFILE_RECORD_HEADER FileRecord,
          PNTFS_ATTR_RECORD AttributeAddress,
          PCWSTR Name,
          USHORT NameLength)
{
    ULONG AttributeLength;
    // Calculate the header length
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    ULONG FileRecordEnd = AttributeAddress->Length;
    ULONG NameOffset;
    ULONG ValueOffset;
    // We'll start out with 8 bytes of bitmap data
    ULONG ValueLength = 8;
    ULONG BytesAvailable;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $BITMAP attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    NameOffset = ResidentHeaderLength;

    // Calculate ValueOffset, which will be aligned to a 4-byte boundary
    ValueOffset = ALIGN_UP_BY(NameOffset + (sizeof(WCHAR) * NameLength), VALUE_OFFSET_ALIGNMENT);

    // Calculate length of attribute
    AttributeLength = ValueOffset + ValueLength;
    AttributeLength = ALIGN_UP_BY(AttributeLength, ATTR_RECORD_ALIGNMENT);

    // Make sure the file record is large enough for the new attribute
    BytesAvailable = Vcb->NtfsInfo.BytesPerFileRecord - FileRecord->BytesInUse;
    if (BytesAvailable < AttributeLength)
    {
        DPRINT1("FIXME: Not enough room in file record for index allocation attribute!\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    // Set Attribute fields
    RtlZeroMemory(AttributeAddress, AttributeLength);

    AttributeAddress->Type = AttributeBitmap;
    AttributeAddress->Length = AttributeLength;
    AttributeAddress->NameLength = NameLength;
    AttributeAddress->NameOffset = NameOffset;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    AttributeAddress->Resident.ValueLength = ValueLength;
    AttributeAddress->Resident.ValueOffset = ValueOffset;

    // Set the name
    RtlCopyMemory((PCHAR)((ULONG_PTR)AttributeAddress + NameOffset), Name, NameLength * sizeof(WCHAR));

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

/**
* @name AddData
* @implemented
*
* Adds a $DATA attribute to a given FileRecord.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to. Caller is responsible for
* ensuring FileRecord is large enough to contain $DATA.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $DATA attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record.
*
* @remarks
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* As it's implemented, this function is only intended to assist in creating new file records. It
* could be made more general-purpose by considering file records with an $ATTRIBUTE_LIST.
* It's the caller's responsibility to ensure the given file record has enough memory allocated
* for the attribute.
*/
NTSTATUS
AddData(PFILE_RECORD_HEADER FileRecord,
        PNTFS_ATTR_RECORD AttributeAddress)
{
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    ULONG FileRecordEnd = AttributeAddress->Length;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $DATA attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    AttributeAddress->Type = AttributeData;
    AttributeAddress->Length = ResidentHeaderLength;
    AttributeAddress->Length = ALIGN_UP_BY(AttributeAddress->Length, ATTR_RECORD_ALIGNMENT);
    AttributeAddress->Resident.ValueLength = 0;
    AttributeAddress->Resident.ValueOffset = ResidentHeaderLength;

    // for unnamed $DATA attributes, NameOffset equals header length
    AttributeAddress->NameOffset = ResidentHeaderLength;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

/**
* @name AddFileName
* @implemented
*
* Adds a $FILE_NAME attribute to a given FileRecord.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to. Caller is responsible for
* ensuring FileRecord is large enough to contain $FILE_NAME.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $FILE_NAME attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION.
*
* @param FileObject
* Pointer to the FILE_OBJECT which represents the new name.
* This parameter is used to determine the filename and parent directory.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application opened the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @param ParentMftIndex
* Pointer to a ULONGLONG which will receive the index of the parent directory.
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record.
*
* @remarks
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* As it's implemented, this function is only intended to assist in creating new file records. It
* could be made more general-purpose by considering file records with an $ATTRIBUTE_LIST.
* It's the caller's responsibility to ensure the given file record has enough memory allocated
* for the attribute.
*/
NTSTATUS
AddFileName(PFILE_RECORD_HEADER FileRecord,
            PNTFS_ATTR_RECORD AttributeAddress,
            PDEVICE_EXTENSION DeviceExt,
            PFILE_OBJECT FileObject,
            BOOLEAN CaseSensitive,
            PULONGLONG ParentMftIndex)
{
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    PFILENAME_ATTRIBUTE FileNameAttribute;
    LARGE_INTEGER SystemTime;
    ULONG FileRecordEnd = AttributeAddress->Length;
    ULONGLONG CurrentMFTIndex = NTFS_FILE_ROOT;
    UNICODE_STRING Current, Remaining, FilenameNoPath;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG FirstEntry;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $FILE_NAME attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    AttributeAddress->Type = AttributeFileName;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    FileNameAttribute = (PFILENAME_ATTRIBUTE)((LONG_PTR)AttributeAddress + ResidentHeaderLength);

    /* KeQuerySystemTime writes SystemTime. Zero it so a failed query cannot
     * leave the file name with an unspecified stamp. */
    SystemTime.QuadPart = 0;
    KeQuerySystemTime(&SystemTime);
    FileNameAttribute->CreationTime = SystemTime.QuadPart;
    FileNameAttribute->ChangeTime = SystemTime.QuadPart;
    FileNameAttribute->LastWriteTime = SystemTime.QuadPart;
    FileNameAttribute->LastAccessTime = SystemTime.QuadPart;

    // Is this a directory?
    if(FileRecord->Flags & FRH_DIRECTORY)
        FileNameAttribute->FileAttributes = NTFS_FILE_TYPE_DIRECTORY;
    else
        FileNameAttribute->FileAttributes = NTFS_FILE_TYPE_ARCHIVE;

    // we need to extract the filename from the path
    DPRINT("Pathname: %wZ\n", &FileObject->FileName);

    FsRtlDissectName(FileObject->FileName, &Current, &Remaining);

    while (Remaining.Length != 0)
    {
        DPRINT("Current: %wZ\n", &Current);

        FirstEntry = 0;
        Status = NtfsFindMftRecord(DeviceExt,
                                   CurrentMFTIndex,
                                   &Current,
                                   &FirstEntry,
                                   FALSE,
                                   CaseSensitive,
                                   &CurrentMFTIndex);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Path component '%wZ' of '%wZ' not found (Status %lx)\n",
                    &Current, &FileObject->FileName, Status);
            return Status;
        }

        FsRtlDissectName(Remaining, &Current, &Remaining);
    }

    /* Whatever is left over is the file's own name */
    FilenameNoPath = Current;

    DPRINT("MFT Index of parent: %I64u\n", CurrentMFTIndex);

    // set reference to parent directory
    FileNameAttribute->DirectoryFileReferenceNumber = CurrentMFTIndex;
    *ParentMftIndex = CurrentMFTIndex;

    /* The high 16 bits are the parent's sequence number, including the root. */
    {
        PFILE_RECORD_HEADER ParentRecord;
        NTSTATUS ParentStatus;

        ParentRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
        if (ParentRecord == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        ParentStatus = ReadFileRecord(DeviceExt, CurrentMFTIndex, ParentRecord);
        if (!NT_SUCCESS(ParentStatus))
        {
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentRecord);
            return ParentStatus;
        }

        FileNameAttribute->DirectoryFileReferenceNumber |=
            (ULONGLONG)ParentRecord->SequenceNumber << 48;
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentRecord);
    }

    DPRINT("FileNameAttribute->DirectoryFileReferenceNumber: 0x%016I64x\n", FileNameAttribute->DirectoryFileReferenceNumber);

    FileNameAttribute->NameLength = FilenameNoPath.Length / sizeof(WCHAR);
    RtlCopyMemory(FileNameAttribute->Name, FilenameNoPath.Buffer, FilenameNoPath.Length);

    /* With 8.3 generation off, Windows stores the one name as POSIX and
     * adds no DOS alias. A legal 8.3 name is otherwise one attribute in both
     * namespaces. A longer name is Win32, and a DOS alias is added later. */
    if (!DeviceExt->NtfsInfo.ShortNameCreation || CaseSensitive)
        FileNameAttribute->NameType = NTFS_FILE_NAME_POSIX;
    else if (RtlIsNameLegalDOS8Dot3(&FilenameNoPath, NULL, NULL))
        FileNameAttribute->NameType = NTFS_FILE_NAME_WIN32_AND_DOS;
    else
        FileNameAttribute->NameType = NTFS_FILE_NAME_WIN32;

    FileRecord->LinkCount++;

    AttributeAddress->Length = ResidentHeaderLength +
        FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + FilenameNoPath.Length;
    AttributeAddress->Length = ALIGN_UP_BY(AttributeAddress->Length, ATTR_RECORD_ALIGNMENT);

    AttributeAddress->Resident.ValueLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + FilenameNoPath.Length;
    AttributeAddress->Resident.ValueOffset = ResidentHeaderLength;
    AttributeAddress->Resident.Flags = RA_INDEXED;

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return Status;
}

/* Skip characters that cannot appear in an 8.3 name. The dot is the separator
 * and is handled by the caller. */
static
BOOLEAN
NtfsDosChar(WCHAR Char)
{
    if (Char < 32 || Char == L' ' || Char == L'.')
        return FALSE;

    switch (Char)
    {
        case L'"':
        case L'*':
        case L'+':
        case L',':
        case L'/':
        case L':':
        case L';':
        case L'<':
        case L'=':
        case L'>':
        case L'?':
        case L'\\':
        case L'|':
            return FALSE;

        default:
            break;
    }

    return TRUE;
}

/* Checksum of the long name. The first four tails use six characters plus
 * ~1..~4. Later tails use two characters, four hex digits of this value,
 * and ~N, which is how Windows keeps going after those four collide. */
static
USHORT
NtfsShortNameHash(PCUNICODE_STRING LongName)
{
    ULONG Hash = 0;
    ULONG Index;
    ULONG Chars = LongName->Length / sizeof(WCHAR);

    for (Index = 0; Index < Chars; Index++)
    {
        WCHAR Upper = RtlUpcaseUnicodeChar(LongName->Buffer[Index]);
        Hash = ((Hash << 5) + Hash) + Upper;
    }

    return (USHORT)(Hash ^ (Hash >> 16));
}

/* Build an 8.3 alias. Generation 1..4 is BASE(6)~N. Generation 5 and up is
 * the hash tail, and N continues from 1 inside that form. */
static
VOID
NtfsBuildShortName(PCUNICODE_STRING LongName,
                   USHORT Generation,
                   PWCHAR ShortName,
                   PUSHORT ShortChars)
{
    WCHAR Base[6];
    WCHAR Ext[3];
    ULONG BaseChars = 0;
    ULONG ExtChars = 0;
    ULONG Chars;
    ULONG Dot;
    ULONG Index;
    ULONG Out;
    ULONG Take;
    USHORT Tail;
    USHORT Hash;
    PCWSTR Source;
    static CONST WCHAR Hex[] = L"0123456789ABCDEF";

    Chars = LongName->Length / sizeof(WCHAR);
    Source = LongName->Buffer;
    Dot = Chars;

    for (Index = 0; Index < Chars; Index++)
    {
        if (Source[Index] == L'.')
            Dot = Index;
    }

    for (Index = 0; Index < Dot && BaseChars < 6; Index++)
    {
        WCHAR Upper = RtlUpcaseUnicodeChar(Source[Index]);
        if (NtfsDosChar(Upper))
            Base[BaseChars++] = Upper;
    }

    if (BaseChars == 0)
    {
        Base[0] = L'F';
        BaseChars = 1;
    }

    if (Dot < Chars)
    {
        for (Index = Dot + 1; Index < Chars && ExtChars < 3; Index++)
        {
            WCHAR Upper = RtlUpcaseUnicodeChar(Source[Index]);
            if (NtfsDosChar(Upper))
                Ext[ExtChars++] = Upper;
        }
    }

    Out = 0;
    if (Generation <= 4)
    {
        Take = min(BaseChars, 6);
        Tail = Generation;
    }
    else
    {
        Take = min(BaseChars, 2);
        Hash = NtfsShortNameHash(LongName);
        Tail = (USHORT)(Generation - 4);
        if (Tail > 9)
            Tail = 9;
    }

    for (Index = 0; Index < Take; Index++)
        ShortName[Out++] = Base[Index];

    if (Generation > 4)
    {
        ShortName[Out++] = Hex[(Hash >> 12) & 0xF];
        ShortName[Out++] = Hex[(Hash >> 8) & 0xF];
        ShortName[Out++] = Hex[(Hash >> 4) & 0xF];
        ShortName[Out++] = Hex[Hash & 0xF];
    }

    ShortName[Out++] = L'~';
    ShortName[Out++] = (WCHAR)(L'0' + Tail);

    if (ExtChars != 0)
    {
        ShortName[Out++] = L'.';
        for (Index = 0; Index < ExtChars; Index++)
            ShortName[Out++] = Ext[Index];
    }

    *ShortChars = (USHORT)Out;
}

/**
* @name AddDosFileName
*
* Stores a DOS $FILE_NAME beside a long name that is not itself a legal 8.3
* name. The short name is not another hard link, so LinkCount stays as it is.
* AttributeAddress must be the end marker, which is where a new file's
* $FILE_NAME sits before $DATA.
*/
NTSTATUS
AddDosFileName(PFILE_RECORD_HEADER FileRecord,
               PNTFS_ATTR_RECORD AttributeAddress,
               PDEVICE_EXTENSION DeviceExt,
               PFILENAME_ATTRIBUTE LongName,
               ULONGLONG ParentMftIndex,
               BOOLEAN CaseSensitive)
{
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    PFILENAME_ATTRIBUTE ShortAttribute;
    UNICODE_STRING LongString;
    UNICODE_STRING ShortString;
    WCHAR ShortName[12];
    USHORT ShortChars;
    USHORT Generation;
    ULONG FileRecordEnd;
    ULONG ValueLength;
    ULONG AttributeLength;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add a DOS name at the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    LongString.Buffer = LongName->Name;
    LongString.Length = LongName->NameLength * sizeof(WCHAR);
    LongString.MaximumLength = LongString.Length;

    /* ~1 through ~4, then hash tails. Stop only when those are exhausted. */
    for (Generation = 1; Generation <= 13; Generation++)
    {
        ULONG FirstEntry = 0;
        ULONGLONG Existing;
        NTSTATUS Status;

        NtfsBuildShortName(&LongString, Generation, ShortName, &ShortChars);
        ShortString.Buffer = ShortName;
        ShortString.Length = ShortChars * sizeof(WCHAR);
        ShortString.MaximumLength = sizeof(ShortName);

        Status = NtfsFindMftRecord(DeviceExt,
                                   ParentMftIndex,
                                   &ShortString,
                                   &FirstEntry,
                                   FALSE,
                                   CaseSensitive,
                                   &Existing);
        if (!NT_SUCCESS(Status))
            break;
    }

    if (Generation > 13)
    {
        DPRINT1("No free 8.3 name for '%.*S'\n", LongName->NameLength, LongName->Name);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    ValueLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + (ShortChars * sizeof(WCHAR));
    AttributeLength = ALIGN_UP_BY(ResidentHeaderLength + ValueLength, ATTR_RECORD_ALIGNMENT);

    if (FileRecord->BytesInUse + AttributeLength > FileRecord->BytesAllocated)
    {
        DPRINT1("No room for a DOS name in the file record.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FileRecordEnd = AttributeAddress->Length;
    RtlZeroMemory(AttributeAddress, AttributeLength);

    AttributeAddress->Type = AttributeFileName;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;
    AttributeAddress->Length = AttributeLength;
    AttributeAddress->Resident.ValueLength = ValueLength;
    AttributeAddress->Resident.ValueOffset = (USHORT)ResidentHeaderLength;
    AttributeAddress->Resident.Flags = RA_INDEXED;

    ShortAttribute = (PFILENAME_ATTRIBUTE)((ULONG_PTR)AttributeAddress + ResidentHeaderLength);
    ShortAttribute->DirectoryFileReferenceNumber = LongName->DirectoryFileReferenceNumber;
    ShortAttribute->CreationTime = LongName->CreationTime;
    ShortAttribute->LastWriteTime = LongName->LastWriteTime;
    ShortAttribute->ChangeTime = LongName->ChangeTime;
    ShortAttribute->LastAccessTime = LongName->LastAccessTime;
    ShortAttribute->AllocatedSize = LongName->AllocatedSize;
    ShortAttribute->DataSize = LongName->DataSize;
    ShortAttribute->FileAttributes = LongName->FileAttributes;
    ShortAttribute->NameLength = (UCHAR)ShortChars;
    ShortAttribute->NameType = NTFS_FILE_NAME_DOS;
    RtlCopyMemory(ShortAttribute->Name, ShortName, ShortChars * sizeof(WCHAR));

    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeLength);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

/**
* @name AddIndexAllocation
* @implemented
*
* Adds an $INDEX_ALLOCATION attribute to a given FileRecord.
*
* @param Vcb
* Pointer to an NTFS_VCB for the destination volume.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $INDEX_ALLOCATION attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param Name
* Pointer to a string of 16-bit Unicode characters naming the attribute. Most often, this will be L"$I30".
*
* @param NameLength
* The number of wide-characters in the name. L"$I30" Would use 4 here.
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record, or if the file record isn't large enough for the attribute.
*
* @remarks
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* This could be improved by adding an $ATTRIBUTE_LIST to the file record if there's not enough space.
*
*/
NTSTATUS
AddIndexAllocation(PNTFS_VCB Vcb,
                   PFILE_RECORD_HEADER FileRecord,
                   PNTFS_ATTR_RECORD AttributeAddress,
                   PCWSTR Name,
                   USHORT NameLength)
{
    ULONG RecordLength;
    ULONG FileRecordEnd;
    ULONG NameOffset;
    ULONG DataRunOffset;
    ULONG BytesAvailable;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $INDEX_ALLOCATION attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    // Calculate the name offset
    NameOffset = FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.CompressedSize);

    // Calculate the offset to the first data run
    DataRunOffset = (sizeof(WCHAR) * NameLength) + NameOffset;
    // The data run offset must be aligned to a 4-byte boundary
    DataRunOffset = ALIGN_UP_BY(DataRunOffset, DATA_RUN_ALIGNMENT);

    // Calculate the length of the new attribute; the empty data run will consist of a single byte
    RecordLength = DataRunOffset + 1;

    // The size of the attribute itself must be aligned to an 8 - byte boundary
    RecordLength = ALIGN_UP_BY(RecordLength, ATTR_RECORD_ALIGNMENT);

    // Back up the last 4-bytes of the file record (even though this value doesn't matter)
    FileRecordEnd = AttributeAddress->Length;

    // Make sure the file record can contain the new attribute
    BytesAvailable = Vcb->NtfsInfo.BytesPerFileRecord - FileRecord->BytesInUse;
    if (BytesAvailable < RecordLength)
    {
        DPRINT1("FIXME: Not enough room in file record for index allocation attribute!\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    // Set fields of attribute header
    RtlZeroMemory(AttributeAddress, RecordLength);

    AttributeAddress->Type = AttributeIndexAllocation;
    AttributeAddress->Length = RecordLength;
    AttributeAddress->IsNonResident = TRUE;
    AttributeAddress->NameLength = NameLength;
    AttributeAddress->NameOffset = NameOffset;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    AttributeAddress->NonResident.MappingPairsOffset = DataRunOffset;
    AttributeAddress->NonResident.HighestVCN = (LONGLONG)-1;

    // Set the name
    RtlCopyMemory((PCHAR)((ULONG_PTR)AttributeAddress + NameOffset), Name, NameLength * sizeof(WCHAR));

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

/**
* @name AddIndexRoot
* @implemented
*
* Adds an $INDEX_ROOT attribute to a given FileRecord.
*
* @param Vcb
* Pointer to an NTFS_VCB for the destination volume.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to. Caller is responsible for
* ensuring FileRecord is large enough to contain $INDEX_ROOT.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $INDEX_ROOT attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param NewIndexRoot
* Pointer to an INDEX_ROOT_ATTRIBUTE containing the index root that will be copied to the new attribute.
*
* @param RootLength
* The length of NewIndexRoot, in bytes.
*
* @param Name
* Pointer to a string of 16-bit Unicode characters naming the attribute. Most often, this will be L"$I30".
*
* @param NameLength
* The number of wide-characters in the name. L"$I30" Would use 4 here.
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record.
*
* @remarks
* This function is intended to assist in creating new folders.
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* It's the caller's responsibility to ensure the given file record has enough memory allocated
* for the attribute, and this memory must have been zeroed.
*/
NTSTATUS
AddIndexRoot(PNTFS_VCB Vcb,
             PFILE_RECORD_HEADER FileRecord,
             PNTFS_ATTR_RECORD AttributeAddress,
             PINDEX_ROOT_ATTRIBUTE NewIndexRoot,
             ULONG RootLength,
             PCWSTR Name,
             USHORT NameLength)
{
    ULONG AttributeLength;
    // Calculate the header length
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    // Back up the file record's final ULONG (even though it doesn't matter)
    ULONG FileRecordEnd = AttributeAddress->Length;
    ULONG NameOffset;
    ULONG ValueOffset;
    ULONG BytesAvailable;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $DATA attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    NameOffset = ResidentHeaderLength;

    // Calculate ValueOffset, which will be aligned to a 4-byte boundary
    ValueOffset = ALIGN_UP_BY(NameOffset + (sizeof(WCHAR) * NameLength), VALUE_OFFSET_ALIGNMENT);

    // Calculate length of attribute
    AttributeLength = ValueOffset + RootLength;
    AttributeLength = ALIGN_UP_BY(AttributeLength, ATTR_RECORD_ALIGNMENT);

    // Make sure the file record is large enough for the new attribute
    BytesAvailable = Vcb->NtfsInfo.BytesPerFileRecord - FileRecord->BytesInUse;
    if (BytesAvailable < AttributeLength)
    {
        DPRINT1("FIXME: Not enough room in file record for index allocation attribute!\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    // Set Attribute fields
    RtlZeroMemory(AttributeAddress, AttributeLength);

    AttributeAddress->Type = AttributeIndexRoot;
    AttributeAddress->Length = AttributeLength;
    AttributeAddress->NameLength = NameLength;
    AttributeAddress->NameOffset = NameOffset;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    AttributeAddress->Resident.ValueLength = RootLength;
    AttributeAddress->Resident.ValueOffset = ValueOffset;

    // Set the name
    RtlCopyMemory((PCHAR)((ULONG_PTR)AttributeAddress + NameOffset), Name, NameLength * sizeof(WCHAR));

    // Copy the index root attribute
    RtlCopyMemory((PCHAR)((ULONG_PTR)AttributeAddress + ValueOffset), NewIndexRoot, RootLength);

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

static
ULONG
NtfsWriteListItem(PUCHAR Dest,
                  ULONG Type,
                  USHORT Instance,
                  ULONGLONG FileReference,
                  ULONGLONG StartingVcn,
                  PCWSTR Name,
                  UCHAR NameLength);

static
NTSTATUS
NtfsInsertAttributeList(PFILE_RECORD_HEADER FileRecord);

static
NTSTATUS
NtfsAppendListEntry(PDEVICE_EXTENSION Vcb,
                    PFILE_RECORD_HEADER FileRecord,
                    ULONG Type,
                    USHORT Instance,
                    ULONGLONG FileReference,
                    ULONGLONG StartingVcn,
                    PCWSTR Name,
                    UCHAR NameLength)
{
    PNTFS_ATTR_RECORD Attribute;
    PUCHAR Value;
    ULONG ValueLength;
    ULONG ItemLength;
    PUCHAR Item;
    NTSTATUS Status;

    Status = NtfsInsertAttributeList(FileRecord);
    if (!NT_SUCCESS(Status))
        return Status;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute->Type != AttributeEnd && Attribute->Length != 0 &&
           Attribute->Type != AttributeAttributeList)
    {
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }
    if (Attribute->Type != AttributeAttributeList || Attribute->IsNonResident)
        return STATUS_NOT_IMPLEMENTED;

    Value = (PUCHAR)Attribute + Attribute->Resident.ValueOffset;
    ValueLength = Attribute->Resident.ValueLength;
    if (ValueLength >= sizeof(ULONGLONG) && *(PULONGLONG)(Value + ValueLength - sizeof(ULONGLONG)) == 0)
        ValueLength -= sizeof(ULONGLONG);

    ItemLength = ALIGN_UP_BY(26 + NameLength * sizeof(WCHAR), ATTR_RECORD_ALIGNMENT);
    if (Attribute->Resident.ValueOffset + ValueLength + ItemLength + sizeof(ULONGLONG) > Attribute->Length)
    {
        ULONG Need = ValueLength + ItemLength + sizeof(ULONGLONG) - Attribute->Resident.ValueLength;
        ULONG Spare = FileRecord->BytesAllocated - FileRecord->BytesInUse;
        PNTFS_ATTR_RECORD Next;

        Need = ALIGN_UP_BY(Need, ATTR_RECORD_ALIGNMENT);
        if (Need > Spare)
            return STATUS_DISK_FULL;

        Next = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
        RtlMoveMemory((PUCHAR)Next + Need, Next, FileRecord->BytesInUse - ((ULONG_PTR)Next - (ULONG_PTR)FileRecord));
        Attribute->Length += Need;
        FileRecord->BytesInUse += Need;
    }

    Item = Value + ValueLength;
    NtfsWriteListItem(Item, Type, Instance, FileReference, StartingVcn, Name, NameLength);
    *(PULONGLONG)(Item + ItemLength) = 0;
    Attribute->Resident.ValueLength = ValueLength + ItemLength + sizeof(ULONGLONG);
    return UpdateFileRecord(Vcb, FileRecord->MFTRecordNumber, FileRecord);
}

static
NTSTATUS
NtfsAppendRunSegment(PDEVICE_EXTENSION Vcb,
                     PNTFS_ATTR_CONTEXT AttrContext,
                     PFILE_RECORD_HEADER FileRecord,
                     ULONGLONG NextAssignedCluster,
                     ULONG RunLength,
                     ULONGLONG NextVbn)
{
    PFILE_RECORD_HEADER Extension;
    PFILE_RECORD_HEADER Base;
    PNTFS_ATTR_RECORD Source;
    PNTFS_ATTR_RECORD Dest;
    ULONGLONG ExtensionIndex;
    ULONGLONG BaseIndex;
    ULONGLONG BaseReference;
    USHORT Instance;
    ULONG NameBytes;
    ULONG Header;
    ULONG Pairs;
    UCHAR Pair[16];
    UCHAR LengthSize;
    UCHAR OffsetSize;
    NTSTATUS Status;
    BOOLEAN FreeBase = FALSE;

    Source = AttrContext->pRecord;
    Extension = NtfsCreateEmptyFileRecord(Vcb);
    if (Extension == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    BaseIndex = FileRecord->BaseFileRecord & NTFS_MFT_MASK;
    if (FileRecord->BaseFileRecord == 0)
    {
        Base = FileRecord;
        BaseIndex = FileRecord->MFTRecordNumber;
    }
    else
    {
        Base = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
        if (Base == NULL)
        {
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Extension);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Status = ReadFileRecord(Vcb, BaseIndex, Base);
        if (!NT_SUCCESS(Status))
        {
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Base);
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Extension);
            return Status;
        }
        FreeBase = TRUE;
    }

    BaseReference = BaseIndex | ((ULONGLONG)Base->SequenceNumber << 48);
    Instance = Base->NextAttributeNumber++;
    Extension->BaseFileRecord = BaseReference;
    Extension->Flags = FRH_IN_USE;

    Dest = (PNTFS_ATTR_RECORD)((ULONG_PTR)Extension + Extension->AttributeOffset);
    NameBytes = Source->NameLength * sizeof(WCHAR);
    Header = FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.Reserved);
    if (Source->Flags & (NTFS_ATTR_COMPRESSION_MASK | NTFS_ATTR_SPARSE))
        Header = FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.CompressedSize) + sizeof(LONGLONG);
    Header = ALIGN_UP_BY(Header + NameBytes, DATA_RUN_ALIGNMENT);

    LengthSize = GetPackedByteCount(RunLength, TRUE);
    OffsetSize = GetPackedByteCount(NextAssignedCluster, TRUE);
    Pairs = 1 + LengthSize + OffsetSize + 1;
    if (Extension->AttributeOffset + ALIGN_UP_BY(Header + Pairs, ATTR_RECORD_ALIGNMENT) + sizeof(ULONG) * 2 >
        Extension->BytesAllocated)
    {
        if (FreeBase)
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Base);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Extension);
        return STATUS_NOT_IMPLEMENTED;
    }

    RtlZeroMemory(Dest, Header + Pairs);
    Dest->Type = Source->Type;
    Dest->IsNonResident = 1;
    Dest->NameLength = Source->NameLength;
    Dest->NameOffset = (USHORT)(Header - NameBytes);
    Dest->Flags = Source->Flags;
    Dest->Instance = Instance;
    Dest->NonResident.LowestVCN = NextVbn;
    Dest->NonResident.HighestVCN = NextVbn + RunLength - 1;
    Dest->NonResident.MappingPairsOffset = (USHORT)Header;
    Dest->NonResident.CompressionUnit = Source->NonResident.CompressionUnit;
    if (NameBytes != 0)
        RtlCopyMemory((PUCHAR)Dest + Dest->NameOffset, (PUCHAR)Source + Source->NameOffset, NameBytes);

    Pair[0] = (UCHAR)((OffsetSize << 4) | LengthSize);
    RtlCopyMemory(Pair + 1, &RunLength, LengthSize);
    RtlCopyMemory(Pair + 1 + LengthSize, &NextAssignedCluster, OffsetSize);
    Pair[1 + LengthSize + OffsetSize] = 0;
    RtlCopyMemory((PUCHAR)Dest + Header, Pair, Pairs);
    Dest->Length = ALIGN_UP_BY(Header + Pairs, ATTR_RECORD_ALIGNMENT);

    {
        PNTFS_ATTR_RECORD End = (PNTFS_ATTR_RECORD)((ULONG_PTR)Dest + Dest->Length);
        SetFileRecordEnd(Extension, End, FILE_RECORD_END);
    }

    Status = AddNewMftEntry(Extension, Vcb, &ExtensionIndex, TRUE);
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Extension);
    if (!NT_SUCCESS(Status))
    {
        if (FreeBase)
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Base);
        return Status;
    }

    Status = NtfsAppendListEntry(Vcb,
                                 Base,
                                 Source->Type,
                                 Instance,
                                 ExtensionIndex | (1ULL << 48),
                                 NextVbn,
                                 Source->NameLength ? (PCWSTR)((PUCHAR)Source + Source->NameOffset) : NULL,
                                 Source->NameLength);
    if (FreeBase)
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, Base);
    return Status;
}

/**
* @name AddRun
* @implemented
*
* Adds a run of allocated clusters to a non-resident attribute.
*
* @param Vcb
* Pointer to an NTFS_VCB for the destination volume.
*
* @param AttrContext
* Pointer to an NTFS_ATTR_CONTEXT describing the destination attribute.
*
* @param AttrOffset
* Byte offset of the destination attribute relative to its file record.
*
* @param FileRecord
* Pointer to a complete copy of the file record containing the destination attribute. Must be at least
* Vcb->NtfsInfo.BytesPerFileRecord bytes long.
*
* @param NextAssignedCluster
* Logical cluster number of the start of the data run being added.
*
* @param RunLength
* How many clusters are in the data run being added. Can't be 0.
*
* @return
* STATUS_SUCCESS on success. STATUS_INVALID_PARAMETER if AttrContext describes a resident attribute.
* STATUS_INSUFFICIENT_RESOURCES if ConvertDataRunsToLargeMCB() fails or if we fail to allocate a
* buffer for the new data runs.
* STATUS_INSUFFICIENT_RESOURCES or STATUS_UNSUCCESSFUL if FsRtlAddLargeMcbEntry() fails.
* STATUS_NOT_IMPLEMENTED if we need to migrate the attribute to an attribute list (TODO).
* Any other failure from ConvertLargeMCBToDataRuns() is returned as-is.
*
* @remarks
* Clusters should have been allocated previously with NtfsAllocateClusters().
*
*
*/
NTSTATUS
AddRun(PNTFS_VCB Vcb,
       PNTFS_ATTR_CONTEXT AttrContext,
       ULONG AttrOffset,
       PFILE_RECORD_HEADER FileRecord,
       ULONGLONG NextAssignedCluster,
       ULONG RunLength)
{
    NTSTATUS Status;
    int DataRunMaxLength;
    PNTFS_ATTR_RECORD DestinationAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
    ULONG NextAttributeOffset = AttrOffset + AttrContext->pRecord->Length;
    ULONGLONG NextVBN = 0;
    ULONGLONG HighestVcn;

    PUCHAR RunBuffer;
    ULONG RunBufferSize;

    if (!AttrContext->pRecord->IsNonResident)
        return STATUS_INVALID_PARAMETER;

    if (AttrContext->pRecord->NonResident.AllocatedSize != 0)
        NextVBN = AttrContext->pRecord->NonResident.HighestVCN + 1;

    /* The run goes on the end, so it holds the new last VCN */
    HighestVcn = NextVBN + RunLength - 1;

    /* A contiguous tail grows the last pair in place when this record holds it. */
    {
        LONGLONG LastVbn = 0;
        LONGLONG LastLbn = -1;
        BOOLEAN Tail = TRUE;
        BOOLEAN HaveLast = FALSE;

        /* An empty stream, including a sparse $J with HighestVCN -1, has no
         * tail to extend. Looking one up walks an MCB that may hold no runs. */
        if (AttrContext->pRecord->NonResident.AllocatedSize != 0)
            HaveLast = FsRtlLookupLastLargeMcbEntry(&AttrContext->DataRunsMCB, &LastVbn, &LastLbn);
        if (HaveLast &&
            AttrContext->pRecord->NonResident.HighestVCN != (ULONGLONG)-1 &&
            (ULONGLONG)(LastVbn + 1) > AttrContext->pRecord->NonResident.HighestVCN + 1)
            Tail = FALSE;

        if (Tail && HaveLast && LastLbn != -1 && (ULONGLONG)LastLbn + 1 == NextAssignedCluster)
        {
            PUCHAR Run = (PUCHAR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset;
            PUCHAR LastPair = NULL;
            UCHAR LastLenSize = 0;
            ULONGLONG Length = 0;
            BOOLEAN Sparse = FALSE;

            while (*Run != 0)
            {
                UCHAR Header = *Run;
                UCHAR LenSize = Header & 0x0F;
                UCHAR OffSize = Header >> 4;
                LONGLONG Off = 0;

                LastPair = Run;
                LastLenSize = LenSize;
                Run++;
                Length = 0;
                RtlCopyMemory(&Length, Run, LenSize);
                Run += LenSize;
                if (OffSize != 0)
                {
                    RtlCopyMemory(&Off, Run, OffSize);
                    if (Run[OffSize - 1] & 0x80)
                        Off |= ~0LL << (OffSize * 8);
                    Run += OffSize;
                    Sparse = FALSE;
                }
                else
                    Sparse = TRUE;
            }

            if (LastPair != NULL && !Sparse &&
                GetPackedByteCount(Length + RunLength, TRUE) == LastLenSize)
            {
                Length += RunLength;
                RtlCopyMemory(LastPair + 1, &Length, LastLenSize);
                DestinationAttribute->NonResident.HighestVCN = HighestVcn;
                AttrContext->pRecord->NonResident.HighestVCN = HighestVcn;
                FsRtlAddLargeMcbEntry(&AttrContext->DataRunsMCB, NextVBN, NextAssignedCluster, RunLength);
                return UpdateFileRecord(Vcb, FileRecord->MFTRecordNumber, FileRecord);
            }
        }
    }

    // Add newly-assigned clusters to mcb
    _SEH2_TRY
    {
        if (!FsRtlAddLargeMcbEntry(&AttrContext->DataRunsMCB,
                                   NextVBN,
                                   NextAssignedCluster,
                                   RunLength))
        {
            ExRaiseStatus(STATUS_UNSUCCESSFUL);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Failed to add LargeMcb Entry!\n");
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    RunBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (!RunBuffer)
    {
        DPRINT1("ERROR: Couldn't allocate memory for data runs!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Convert the map control block back to encoded data runs.
    Status = ConvertLargeMCBToDataRuns(&AttrContext->DataRunsMCB, HighestVcn + 1, RunBuffer, Vcb->NtfsInfo.BytesPerFileRecord, &RunBufferSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ExFreePoolWithTag(RunBuffer, TAG_NTFS);
        return NtfsAppendRunSegment(Vcb, AttrContext, FileRecord, NextAssignedCluster, RunLength, NextVBN);
    }
    else if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to convert data runs! (Status %lx)\n", Status);
        ExFreePoolWithTag(RunBuffer, TAG_NTFS);
        return Status;
    }

    // Get the amount of free space between the start of the of the first data run and the attribute end
    DataRunMaxLength = AttrContext->pRecord->Length - AttrContext->pRecord->NonResident.MappingPairsOffset;

    // Do we need to extend the attribute (or convert to attribute list)?
    if (DataRunMaxLength < RunBufferSize)
    {
        PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + NextAttributeOffset);
        PNTFS_ATTR_RECORD NewRecord;

        // Add free space at the end of the file record to DataRunMaxLength
        DataRunMaxLength += Vcb->NtfsInfo.BytesPerFileRecord - FileRecord->BytesInUse;

        // Can we resize the attribute?
        if (DataRunMaxLength < RunBufferSize)
        {
            ExFreePoolWithTag(RunBuffer, TAG_NTFS);
            return NtfsAppendRunSegment(Vcb, AttrContext, FileRecord, NextAssignedCluster, RunLength, NextVBN);
        }

        // Are there more attributes after the one we're resizing?
        if (NextAttribute->Type != AttributeEnd)
        {
            PNTFS_ATTR_RECORD FinalAttribute;

            // Calculate where to move the trailing attributes
            ULONG_PTR MoveTo = (ULONG_PTR)DestinationAttribute + AttrContext->pRecord->NonResident.MappingPairsOffset + RunBufferSize;
            MoveTo = ALIGN_UP_BY(MoveTo, ATTR_RECORD_ALIGNMENT);

            DPRINT("Moving attribute(s) after this one starting with type 0x%lx\n", NextAttribute->Type);

            // Move the trailing attributes; FinalAttribute will point to the end marker
            FinalAttribute = MoveAttributes(Vcb, NextAttribute, NextAttributeOffset, MoveTo);

            // set the file record end
            SetFileRecordEnd(FileRecord, FinalAttribute, FILE_RECORD_END);
        }

        // calculate position of end markers
        NextAttributeOffset = AttrOffset + AttrContext->pRecord->NonResident.MappingPairsOffset + RunBufferSize;
        NextAttributeOffset = ALIGN_UP_BY(NextAttributeOffset, ATTR_RECORD_ALIGNMENT);

        // Update the length of the destination attribute
        DestinationAttribute->Length = NextAttributeOffset - AttrOffset;

        // Create a new copy of the attribute record
        NewRecord = ExAllocatePoolWithTag(NonPagedPool, DestinationAttribute->Length, TAG_NTFS);
        if (NewRecord == NULL)
        {
            DPRINT1("ERROR: Couldn't allocate attribute record!\n");
            ExFreePoolWithTag(RunBuffer, TAG_NTFS);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(NewRecord, AttrContext->pRecord, AttrContext->pRecord->Length);
        NewRecord->Length = DestinationAttribute->Length;

        // Free the old copy of the attribute record, which won't be large enough
        ExFreePoolWithTag(AttrContext->pRecord, TAG_NTFS);

        // Set the attribute context's record to the new copy
        AttrContext->pRecord = NewRecord;

        // if NextAttribute is the AttributeEnd marker
        if (NextAttribute->Type == AttributeEnd)
        {
            // End the file record
            NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + NextAttributeOffset);
            SetFileRecordEnd(FileRecord, NextAttribute, FILE_RECORD_END);
        }
    }

    // Update HighestVCN
    DestinationAttribute->NonResident.HighestVCN =
    AttrContext->pRecord->NonResident.HighestVCN = HighestVcn;

    /* Sparse and compressed streams also count the clusters they really own */
    if (AttrContext->pRecord->Flags & (NTFS_ATTR_COMPRESSION_MASK | NTFS_ATTR_SPARSE))
    {
        AttrContext->pRecord->NonResident.CompressedSize += (ULONGLONG)RunLength * Vcb->NtfsInfo.BytesPerCluster;
        DestinationAttribute->NonResident.CompressedSize = AttrContext->pRecord->NonResident.CompressedSize;
    }

    // Write data runs to destination attribute
    RtlCopyMemory((PVOID)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset),
                  RunBuffer,
                  RunBufferSize);

    // Update the attribute record in the attribute context
    RtlCopyMemory((PVOID)((ULONG_PTR)AttrContext->pRecord + AttrContext->pRecord->NonResident.MappingPairsOffset),
                  RunBuffer,
                  RunBufferSize);

    // Update the file record
    Status = UpdateFileRecord(Vcb, AttrContext->FileMFTIndex, FileRecord);

    ExFreePoolWithTag(RunBuffer, TAG_NTFS);

#if NTFS_DUMP_DATA_RUNS
    NtfsDumpDataRuns((PUCHAR)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset), 0);
#endif

    return Status;
}

/**
* @name AddStandardInformation
* @implemented
*
* Adds a $STANDARD_INFORMATION attribute to a given FileRecord.
*
* @param Vcb
* Volume the file will live on. Major version 3 or newer gets the 72-byte value.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to. Caller is responsible for
* ensuring FileRecord is large enough to contain $STANDARD_INFORMATION.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $STANDARD_INFORMATION attribute.
* This address must reside within FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param SecurityId
* Security id stored in the 72-byte tail. Ignored on a volume older than version 3.
*
* @return
* STATUS_SUCCESS on success. STATUS_NOT_IMPLEMENTED if target address isn't at the end
* of the given file record.
*
* @remarks
* Only adding the attribute to the end of the file record is supported; AttributeAddress must
* be of type AttributeEnd.
* As it's implemented, this function is only intended to assist in creating new file records. It
* could be made more general-purpose by considering file records with an $ATTRIBUTE_LIST.
* It's the caller's responsibility to ensure the given file record has enough memory allocated
* for the attribute.
*/
NTSTATUS
AddStandardInformation(PDEVICE_EXTENSION Vcb,
                       PFILE_RECORD_HEADER FileRecord,
                       PNTFS_ATTR_RECORD AttributeAddress,
                       ULONG SecurityId)
{
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    PSTANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER SystemTime;
    ULONG FileRecordEnd = AttributeAddress->Length;
    ULONG ValueLength;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $STANDARD_INFORMATION attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (Vcb->NtfsInfo.MajorVersion >= 3)
        ValueLength = NTFS_STD_INFO_V31_SIZE;
    else
        ValueLength = NTFS_STD_INFO_V12_SIZE;

    StandardInfo = (PSTANDARD_INFORMATION)((LONG_PTR)AttributeAddress + ResidentHeaderLength);

    AttributeAddress->Type = AttributeStandardInformation;
    AttributeAddress->Length = ValueLength + ResidentHeaderLength;
    AttributeAddress->Length = ALIGN_UP_BY(AttributeAddress->Length, ATTR_RECORD_ALIGNMENT);
    AttributeAddress->Resident.ValueLength = ValueLength;
    AttributeAddress->Resident.ValueOffset = ResidentHeaderLength;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    RtlZeroMemory(StandardInfo, ValueLength);

    /* Times and the file attribute live in both the 48-byte and 72-byte values. */
    SystemTime.QuadPart = 0;
    KeQuerySystemTime(&SystemTime);
    StandardInfo->CreationTime = SystemTime.QuadPart;
    StandardInfo->ChangeTime = SystemTime.QuadPart;
    StandardInfo->LastWriteTime = SystemTime.QuadPart;
    StandardInfo->LastAccessTime = SystemTime.QuadPart;
    StandardInfo->FileAttribute = NTFS_FILE_TYPE_ARCHIVE;

    /* The tail stays zero apart from the security id. Do not write it into a v1.2 value. */
    if (ValueLength >= NTFS_STD_INFO_V31_SIZE)
        StandardInfo->SecurityId = SecurityId;

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
}

/**
* @name NtfsCopyParentSecurityId
* @implemented
*
* Copies the parent's security id onto a new file when both attributes are the
* 72-byte NTFS 3.x form. A short attribute keeps the id it was created with.
* The value length is left alone, so a 48-byte attribute is not widened and a
* 72-byte tail is not rewritten except for SecurityId.
*/
VOID
NtfsCopyParentSecurityId(PDEVICE_EXTENSION Vcb,
                         PFILE_RECORD_HEADER FileRecord,
                         ULONGLONG ParentMftIndex)
{
    PSTANDARD_INFORMATION ChildInfo;
    PSTANDARD_INFORMATION ParentInfo;
    PFILE_RECORD_HEADER ParentRecord;
    ULONG ChildLength;
    ULONG ParentLength;
    NTSTATUS Status;

    ChildInfo = NtfsFindStandardInformation(Vcb, FileRecord, &ChildLength);
    if (ChildInfo == NULL || ChildLength < NTFS_STD_INFO_V31_SIZE)
        return;

    ParentRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (ParentRecord == NULL)
        return;

    Status = ReadFileRecord(Vcb, ParentMftIndex, ParentRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, ParentRecord);
        return;
    }

    ParentInfo = NtfsFindStandardInformation(Vcb, ParentRecord, &ParentLength);
    if (ParentInfo != NULL && ParentLength >= NTFS_STD_INFO_V31_SIZE)
        ChildInfo->SecurityId = ParentInfo->SecurityId;

    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, ParentRecord);
}

/**
* @name ConvertDataRunsToLargeMCB
* @implemented
*
* Converts binary data runs to a map control block.
*
* @param DataRun
* Pointer to the run data
*
* @param DataRunsMCB
* Pointer to an unitialized LARGE_MCB structure.
*
* @return
* STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES or STATUS_UNSUCCESSFUL if we fail to
* initialize the mcb or add an entry.
*
* @remarks
* Initializes the LARGE_MCB pointed to by DataRunsMCB. If this function succeeds, you
* need to call FsRtlUninitializeLargeMcb() when you're done with DataRunsMCB. This
* function will ensure the LargeMCB has been unitialized in case of failure.
*
*/
NTSTATUS
ConvertDataRunsToLargeMCB(PUCHAR DataRun,
                          PLARGE_MCB DataRunsMCB,
                          PULONGLONG pNextVBN)
{
    LONGLONG  DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG  DataRunStartLCN;
    ULONGLONG LastLCN = 0;

    // Initialize the MCB, potentially catch an exception
    _SEH2_TRY{
        FsRtlInitializeLargeMcb(DataRunsMCB, NonPagedPool);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    } _SEH2_END;

    while (*DataRun != 0)
    {
        DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);

        if (DataRunOffset != -1)
        {
            // Normal data run.
            DataRunStartLCN = LastLCN + DataRunOffset;
            LastLCN = DataRunStartLCN;

            _SEH2_TRY{
                if (!FsRtlAddLargeMcbEntry(DataRunsMCB,
                                           *pNextVBN,
                                           DataRunStartLCN,
                                           DataRunLength))
                {
                    ExRaiseStatus(STATUS_UNSUCCESSFUL);
                }
            } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                FsRtlUninitializeLargeMcb(DataRunsMCB);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            } _SEH2_END;

        }

        *pNextVBN += DataRunLength;
    }

    return STATUS_SUCCESS;
}

/**
* @name ConvertLargeMCBToDataRuns
* @implemented
*
* Converts a map control block to a series of encoded data runs (used by non-resident attributes).
*
* @param DataRunsMCB
* Pointer to a LARGE_MCB structure describing the data runs.
*
* @param VcnCount
* Number of VCNs the runs must describe, HighestVCN + 1. A hole the MCB doesn't
* reach, at the end of a sparse stream, is encoded up to this point.
*
* @param RunBuffer
* Pointer to the buffer that will receive the encoded data runs.
*
* @param MaxBufferSize
* Size of RunBuffer, in bytes.
*
* @param UsedBufferSize
* Pointer to a ULONG that will receive the size of the data runs in bytes. Can't be NULL.
*
* @return
* STATUS_SUCCESS on success, STATUS_BUFFER_TOO_SMALL if RunBuffer is too small to contain the
* complete output.
*
*/
NTSTATUS
ConvertLargeMCBToDataRuns(PLARGE_MCB DataRunsMCB,
                          ULONGLONG VcnCount,
                          PUCHAR RunBuffer,
                          ULONG MaxBufferSize,
                          PULONG UsedBufferSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RunBufferOffset = 0;
    LONGLONG  DataRunOffset = 0;
    ULONGLONG LastLCN = 0;
    ULONGLONG NextVcn = 0;
    LONGLONG Vbn, Lbn, Count;
    ULONG i;
    BOOLEAN Tail = FALSE;


    DPRINT("\t[Vbn, Lbn, Count]\n");

    // convert each mcb entry to a data run, then the trailing hole if there is one
    for (i = 0; ; i++)
    {
        UCHAR DataRunOffsetSize = 0;
        UCHAR DataRunLengthSize = 0;
        UCHAR ControlByte = 0;

        if (!FsRtlGetNextLargeMcbEntry(DataRunsMCB, i, &Vbn, &Lbn, &Count))
        {
            if (NextVcn >= VcnCount)
                break;

            /* The MCB only knows mapped clusters, so a hole at the end isn't in it */
            Vbn = NextVcn;
            Lbn = -1;
            Count = VcnCount - NextVcn;
            Tail = TRUE;
        }

        // [vbn, lbn, count]
        DPRINT("\t[%I64d, %I64d,%I64d]\n", Vbn, Lbn, Count);

        // determine how to represent DataRunLengthSize with the minimum number of bytes
        DPRINT("Determining how many bytes needed to represent %I64x\n", Count);
        DataRunLengthSize = GetPackedByteCount(Count, TRUE);
        DPRINT("%d bytes needed.\n", DataRunLengthSize);

        /* A sparse run has no offset, and the next real run is relative to the last real one */
        if (Lbn != -1)
        {
            DataRunOffset = Lbn - LastLCN;
            LastLCN = Lbn;

            // now we need to determine how to represent DataRunOffset with the minimum number of bytes
            DPRINT("Determining how many bytes needed to represent %I64x\n", DataRunOffset);
            DataRunOffsetSize = GetPackedByteCount(DataRunOffset, TRUE);
            DPRINT("%d bytes needed.\n", DataRunOffsetSize);
        }

        // ensure the next data run + end marker would be <= Max buffer size
        if (RunBufferOffset + 2 + DataRunLengthSize + DataRunOffsetSize > MaxBufferSize)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            DPRINT1("FIXME: Ran out of room in buffer for data runs!\n");
            break;
        }

        // pack and copy the control byte
        ControlByte = (DataRunOffsetSize << 4) + DataRunLengthSize;
        RunBuffer[RunBufferOffset++] = ControlByte;

        // copy DataRunLength
        RtlCopyMemory(RunBuffer + RunBufferOffset, &Count, DataRunLengthSize);
        RunBufferOffset += DataRunLengthSize;

        // copy DataRunOffset
        RtlCopyMemory(RunBuffer + RunBufferOffset, &DataRunOffset, DataRunOffsetSize);
        RunBufferOffset += DataRunOffsetSize;

        NextVcn = Vbn + Count;

        if (Tail)
            break;
    }

    // End of data runs
    RunBuffer[RunBufferOffset++] = 0;

    *UsedBufferSize = RunBufferOffset;
    DPRINT("New Size of DataRuns: %ld\n", *UsedBufferSize);

    return Status;
}

PUCHAR
DecodeRun(PUCHAR DataRun,
          LONGLONG *DataRunOffset,
          ULONGLONG *DataRunLength)
{
    UCHAR DataRunOffsetSize;
    UCHAR DataRunLengthSize;
    CHAR i;

    DataRunOffsetSize = (*DataRun >> 4) & 0xF;
    DataRunLengthSize = *DataRun & 0xF;
    *DataRunOffset = 0;
    *DataRunLength = 0;
    DataRun++;
    for (i = 0; i < DataRunLengthSize; i++)
    {
        *DataRunLength += ((ULONG64)*DataRun) << (i * 8);
        DataRun++;
    }

    /* NTFS 3+ sparse files */
    if (DataRunOffsetSize == 0)
    {
        *DataRunOffset = -1;
    }
    else
    {
        for (i = 0; i < DataRunOffsetSize - 1; i++)
        {
            *DataRunOffset += ((ULONG64)*DataRun) << (i * 8);
            DataRun++;
        }
        /* The last byte contains sign so we must process it different way. */
        *DataRunOffset = ((LONG64)(CHAR)(*(DataRun++)) << (i * 8)) + *DataRunOffset;
    }

    DPRINT("DataRunOffsetSize: %x\n", DataRunOffsetSize);
    DPRINT("DataRunLengthSize: %x\n", DataRunLengthSize);
    DPRINT("DataRunOffset: %x\n", *DataRunOffset);
    DPRINT("DataRunLength: %x\n", *DataRunLength);

    return DataRun;
}

BOOLEAN
FindRun(PNTFS_ATTR_RECORD NresAttr,
        ULONGLONG vcn,
        PULONGLONG lcn,
        PULONGLONG count)
{
    if (vcn < NresAttr->NonResident.LowestVCN || vcn > NresAttr->NonResident.HighestVCN)
        return FALSE;

    DecodeRun((PUCHAR)((ULONG_PTR)NresAttr + NresAttr->NonResident.MappingPairsOffset), (PLONGLONG)lcn, count);

    return TRUE;
}

/**
* @name FreeClusters
* @implemented
*
* Shrinks the allocation size of a non-resident attribute by a given number of clusters.
* Frees the clusters from the volume's $BITMAP file as well as the attribute's data runs.
*
* @param Vcb
* Pointer to an NTFS_VCB for the destination volume.
*
* @param AttrContext
* Pointer to an NTFS_ATTR_CONTEXT describing the attribute from which the clusters will be freed.
*
* @param AttrOffset
* Byte offset of the destination attribute relative to its file record.
*
* @param FileRecord
* Pointer to a complete copy of the file record containing the attribute. Must be at least
* Vcb->NtfsInfo.BytesPerFileRecord bytes long.
*
* @param ClustersToFree
* Number of clusters that should be freed from the end of the data stream. Must be no more
* Than the number of clusters assigned to the attribute (HighestVCN + 1).
*
* @return
* STATUS_SUCCESS on success. STATUS_INVALID_PARAMETER if AttrContext describes a resident attribute,
* or if the caller requested more clusters be freed than the attribute has been allocated.
* STATUS_INSUFFICIENT_RESOURCES if allocating a buffer for the data runs fails or
* if ConvertDataRunsToLargeMCB() fails.
* STATUS_BUFFER_TOO_SMALL if ConvertLargeMCBToDataRuns() fails.
*
*
*/
NTSTATUS
FreeClusters(PNTFS_VCB Vcb,
             PNTFS_ATTR_CONTEXT AttrContext,
             ULONG AttrOffset,
             PFILE_RECORD_HEADER FileRecord,
             ULONG ClustersToFree)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ClustersLeftToFree = ClustersToFree;

    PNTFS_ATTR_RECORD DestinationAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
    ULONG NextAttributeOffset = AttrOffset + AttrContext->pRecord->Length;
    PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + NextAttributeOffset);

    PUCHAR RunBuffer;
    ULONG RunBufferSize = 0;

    ULONG LowestFreed = MAXULONG;
    ULONG HighestFreed = 0;
    ULONG Freed = 0;

    if (!AttrContext->pRecord->IsNonResident)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Same lock NtfsAllocateClusters() holds over the bitmap copy */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&Vcb->BitmapResource, TRUE);

    // free clusters in $BITMAP file
    while (ClustersLeftToFree > 0)
    {
        ULONGLONG LastVcn = AttrContext->pRecord->NonResident.HighestVCN;
        LONGLONG LargeLbn;

        /* An empty stream has HighestVCN -1 */
        if (LastVcn == (ULONGLONG)-1)
        {
            Status = STATUS_INVALID_PARAMETER;
            DPRINT1("DRIVER ERROR: FreeClusters called to free %lu clusters, which is %lu more clusters than are assigned to attribute!",
                    ClustersToFree,
                    ClustersLeftToFree);
            break;
        }

        /* The last VCN of a sparse stream can sit in a hole, which owns no cluster */
        if (FsRtlLookupLargeMcbEntry(&AttrContext->DataRunsMCB, LastVcn, &LargeLbn, NULL, NULL, NULL, NULL) &&
            LargeLbn != -1)
        {
            // deallocate this cluster
            if (NT_SUCCESS(NtfsBringClusterWindow(Vcb, (ULONGLONG)LargeLbn)))
            {
                ULONG BaseBit = (ULONG)(Vcb->ClusterWindowBase * 8);
                RtlClearBits(&Vcb->ClusterBitmap, (ULONG)LargeLbn - BaseBit, 1);
                Vcb->ClusterWindowDirty = TRUE;
            }
            LowestFreed = min(LowestFreed, (ULONG)LargeLbn);
            HighestFreed = max(HighestFreed, (ULONG)LargeLbn);
            Freed++;
        }
        FsRtlTruncateLargeMcb(&AttrContext->DataRunsMCB, LastVcn);

        /* Freeing the last cluster leaves the -1 of an empty stream */
        AttrContext->pRecord->NonResident.HighestVCN = LastVcn - 1;
        ClustersLeftToFree--;
    }

    if (AttrContext->pRecord->Flags & (NTFS_ATTR_COMPRESSION_MASK | NTFS_ATTR_SPARSE))
        AttrContext->pRecord->NonResident.CompressedSize -= (ULONGLONG)Freed * Vcb->NtfsInfo.BytesPerCluster;

    if (Freed != 0)
    {
        NTSTATUS WriteStatus;

        Vcb->FreeClusterCount += Freed;

        // update $BITMAP file on disk
        WriteStatus = NtfsWriteVolumeBitmap(Vcb, LowestFreed, HighestFreed - LowestFreed + 1);
        if (NT_SUCCESS(Status))
            Status = WriteStatus;
    }

    ExReleaseResourceLite(&Vcb->BitmapResource);
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(Status))
        return Status;

    // Save updated data runs to file record

    // Allocate some memory for a new RunBuffer
    RunBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (!RunBuffer)
    {
        DPRINT1("ERROR: Couldn't allocate memory for data runs!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Convert the map control block back to encoded data runs.
    // RunBuffer is one file record long (not one cluster) - see AddRun().
    ConvertLargeMCBToDataRuns(&AttrContext->DataRunsMCB,
                              AttrContext->pRecord->NonResident.HighestVCN + 1,
                              RunBuffer,
                              Vcb->NtfsInfo.BytesPerFileRecord,
                              &RunBufferSize);

    // Update HighestVCN
    DestinationAttribute->NonResident.HighestVCN = AttrContext->pRecord->NonResident.HighestVCN;

    /* Only these headers have the field; in others its offset is the name or the runs */
    if (AttrContext->pRecord->Flags & (NTFS_ATTR_COMPRESSION_MASK | NTFS_ATTR_SPARSE))
        DestinationAttribute->NonResident.CompressedSize = AttrContext->pRecord->NonResident.CompressedSize;

    // Write data runs to destination attribute
    RtlCopyMemory((PVOID)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset),
                  RunBuffer,
                  RunBufferSize);

    // Is DestinationAttribute the last attribute in the file record?
    if (NextAttribute->Type == AttributeEnd)
    {
        // update attribute length
        DestinationAttribute->Length = ALIGN_UP_BY(AttrContext->pRecord->NonResident.MappingPairsOffset + RunBufferSize,
                                                 ATTR_RECORD_ALIGNMENT);

        ASSERT(DestinationAttribute->Length <= AttrContext->pRecord->Length);

        AttrContext->pRecord->Length = DestinationAttribute->Length;

        // write end markers
        NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)DestinationAttribute + DestinationAttribute->Length);
        SetFileRecordEnd(FileRecord, NextAttribute, FILE_RECORD_END);
    }

    // Update the file record
    Status = UpdateFileRecord(Vcb, AttrContext->FileMFTIndex, FileRecord);

    ExFreePoolWithTag(RunBuffer, TAG_NTFS);

#if NTFS_DUMP_DATA_RUNS
    NtfsDumpDataRuns((PUCHAR)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset), 0);
#endif

    return Status;
}

static
NTSTATUS
InternalReadNonResidentAttributes(PFIND_ATTR_CONTXT Context)
{
    ULONGLONG ListSize;
    PNTFS_ATTR_RECORD Attribute;
    PNTFS_ATTR_CONTEXT ListContext;

    DPRINT("InternalReadNonResidentAttributes(%p)\n", Context);

    Attribute = Context->CurrAttr;
    ASSERT(Attribute->Type == AttributeAttributeList);

    if (Context->OnlyResident)
    {
        Context->NonResidentStart = NULL;
        Context->NonResidentEnd = NULL;
        return STATUS_SUCCESS;
    }

    if (Context->NonResidentStart != NULL)
    {
        return STATUS_FILE_CORRUPT_ERROR;
    }

    ListContext = PrepareAttributeContext(Attribute);
    ListSize = AttributeDataLength(ListContext->pRecord);
    if (ListSize > 0xFFFFFFFF)
    {
        ReleaseAttributeContext(ListContext);
        return STATUS_BUFFER_OVERFLOW;
    }

    Context->NonResidentStart = ExAllocatePoolWithTag(NonPagedPool, (ULONG)ListSize, TAG_NTFS);
    if (Context->NonResidentStart == NULL)
    {
        ReleaseAttributeContext(ListContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ReadAttribute(Context->Vcb, ListContext, 0, (PCHAR)Context->NonResidentStart, (ULONG)ListSize) != ListSize)
    {
        ExFreePoolWithTag(Context->NonResidentStart, TAG_NTFS);
        Context->NonResidentStart = NULL;
        ReleaseAttributeContext(ListContext);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    ReleaseAttributeContext(ListContext);
    Context->NonResidentEnd = (PNTFS_ATTRIBUTE_LIST_ITEM)((PCHAR)Context->NonResidentStart + ListSize);
    return STATUS_SUCCESS;
}

static
PNTFS_ATTRIBUTE_LIST_ITEM
InternalGetNextAttributeListItem(PFIND_ATTR_CONTXT Context)
{
    PNTFS_ATTRIBUTE_LIST_ITEM NextItem;

    if (Context->NonResidentCur == (PVOID)-1)
    {
        return NULL;
    }

    if (Context->NonResidentCur == NULL || Context->NonResidentCur->Type == AttributeEnd)
    {
        Context->NonResidentCur = (PVOID)-1;
        return NULL;
    }

    if (Context->NonResidentCur->Length == 0)
    {
        DPRINT1("Broken length list entry length !");
        Context->NonResidentCur = (PVOID)-1;
        return NULL;
    }

    NextItem = (PNTFS_ATTRIBUTE_LIST_ITEM)((PCHAR)Context->NonResidentCur + Context->NonResidentCur->Length);
    if (NextItem->Length == 0 || NextItem->Type == AttributeEnd)
    {
        Context->NonResidentCur = (PVOID)-1;
        return NULL;
    }

    if (NextItem < Context->NonResidentStart || NextItem > Context->NonResidentEnd)
    {
        Context->NonResidentCur = (PVOID)-1;
        return NULL;
    }

    Context->NonResidentCur = NextItem;
    return NextItem;
}

NTSTATUS
FindFirstAttributeListItem(PFIND_ATTR_CONTXT Context,
                           PNTFS_ATTRIBUTE_LIST_ITEM *Item)
{
    if (Context->NonResidentStart == NULL || Context->NonResidentStart->Type == AttributeEnd)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Context->NonResidentCur = Context->NonResidentStart;
    *Item = Context->NonResidentCur;
    return STATUS_SUCCESS;
}

NTSTATUS
FindNextAttributeListItem(PFIND_ATTR_CONTXT Context,
                          PNTFS_ATTRIBUTE_LIST_ITEM *Item)
{
    *Item = InternalGetNextAttributeListItem(Context);
    if (*Item == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static
PNTFS_ATTR_RECORD
InternalGetNextAttribute(PFIND_ATTR_CONTXT Context)
{
    PNTFS_ATTR_RECORD NextAttribute;

    if (Context->CurrAttr == (PVOID)-1)
    {
        return NULL;
    }

    if (Context->CurrAttr >= Context->FirstAttr &&
        Context->CurrAttr < Context->LastAttr)
    {
        if (Context->CurrAttr->Length == 0)
        {
            DPRINT1("Broken length!\n");
            Context->CurrAttr = (PVOID)-1;
            return NULL;
        }

        NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Context->CurrAttr + Context->CurrAttr->Length);

        if (NextAttribute > Context->LastAttr || NextAttribute < Context->FirstAttr)
        {
            DPRINT1("Broken length: 0x%lx!\n", Context->CurrAttr->Length);
            Context->CurrAttr = (PVOID)-1;
            return NULL;
        }

        Context->Offset += ((ULONG_PTR)NextAttribute - (ULONG_PTR)Context->CurrAttr);
        Context->CurrAttr = NextAttribute;

        if (Context->CurrAttr < Context->LastAttr &&
            Context->CurrAttr->Type != AttributeEnd)
        {
            return Context->CurrAttr;
        }
    }

    if (Context->NonResidentStart == NULL)
    {
        Context->CurrAttr = (PVOID)-1;
        return NULL;
    }

    Context->CurrAttr = (PVOID)-1;
    return NULL;
}

NTSTATUS
FindFirstAttribute(PFIND_ATTR_CONTXT Context,
                   PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER FileRecord,
                   BOOLEAN OnlyResident,
                   PNTFS_ATTR_RECORD * Attribute)
{
    NTSTATUS Status;

    DPRINT("FindFistAttribute(%p, %p, %p, %p, %u, %p)\n", Context, Vcb, FileRecord, OnlyResident, Attribute);

    Context->Vcb = Vcb;
    Context->OnlyResident = OnlyResident;
    Context->FirstAttr = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    Context->CurrAttr = Context->FirstAttr;
    Context->LastAttr = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse);
    Context->NonResidentStart = NULL;
    Context->NonResidentEnd = NULL;
    Context->Offset = FileRecord->AttributeOffset;

    if (Context->FirstAttr->Type == AttributeEnd)
    {
        Context->CurrAttr = (PVOID)-1;
        return STATUS_END_OF_FILE;
    }
    else if (Context->FirstAttr->Type == AttributeAttributeList)
    {
        Status = InternalReadNonResidentAttributes(Context);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        *Attribute = InternalGetNextAttribute(Context);
        if (*Attribute == NULL)
        {
            return STATUS_END_OF_FILE;
        }
    }
    else
    {
        *Attribute = Context->CurrAttr;
        Context->Offset = (UCHAR*)Context->CurrAttr - (UCHAR*)FileRecord;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FindNextAttribute(PFIND_ATTR_CONTXT Context,
                  PNTFS_ATTR_RECORD * Attribute)
{
    NTSTATUS Status;

    DPRINT("FindNextAttribute(%p, %p)\n", Context, Attribute);

    *Attribute = InternalGetNextAttribute(Context);
    if (*Attribute == NULL)
    {
        return STATUS_END_OF_FILE;
    }

    if (Context->CurrAttr->Type != AttributeAttributeList)
    {
        return STATUS_SUCCESS;
    }

    Status = InternalReadNonResidentAttributes(Context);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *Attribute = InternalGetNextAttribute(Context);
    if (*Attribute == NULL)
    {
        return STATUS_END_OF_FILE;
    }

    return STATUS_SUCCESS;
}

VOID
FindCloseAttribute(PFIND_ATTR_CONTXT Context)
{
    if (Context->NonResidentStart != NULL)
    {
        ExFreePoolWithTag(Context->NonResidentStart, TAG_NTFS);
        Context->NonResidentStart = NULL;
    }
}

static
VOID
NtfsDumpFileNameAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PFILENAME_ATTRIBUTE FileNameAttr;

    DbgPrint("  $FILE_NAME ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    FileNameAttr = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" (%x) '%.*S' ", FileNameAttr->NameType, FileNameAttr->NameLength, FileNameAttr->Name);
    DbgPrint(" '%x' \n", FileNameAttr->FileAttributes);
    DbgPrint(" AllocatedSize: %I64u\nDataSize: %I64u\n", FileNameAttr->AllocatedSize, FileNameAttr->DataSize);
    DbgPrint(" File reference: 0x%016I64x\n", FileNameAttr->DirectoryFileReferenceNumber);
}


static
VOID
NtfsDumpStandardInformationAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PSTANDARD_INFORMATION StandardInfoAttr;

    DbgPrint("  $STANDARD_INFORMATION ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    StandardInfoAttr = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" '%x' ", StandardInfoAttr->FileAttribute);
}


static
VOID
NtfsDumpVolumeNameAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PWCHAR VolumeName;

    DbgPrint("  $VOLUME_NAME ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    VolumeName = (PWCHAR)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" '%.*S' ", Attribute->Resident.ValueLength / sizeof(WCHAR), VolumeName);
}


static
VOID
NtfsDumpVolumeInformationAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PVOLINFO_ATTRIBUTE VolInfoAttr;

    DbgPrint("  $VOLUME_INFORMATION ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    VolInfoAttr = (PVOLINFO_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" NTFS Version %u.%u  Flags 0x%04hx ",
             VolInfoAttr->MajorVersion,
             VolInfoAttr->MinorVersion,
             VolInfoAttr->Flags);
}


static
VOID
NtfsDumpIndexRootAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PINDEX_ROOT_ATTRIBUTE IndexRootAttr;
    ULONG CurrentOffset;
    ULONG CurrentNode;

    IndexRootAttr = (PINDEX_ROOT_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);

    if (IndexRootAttr->AttributeType == AttributeFileName)
        ASSERT(IndexRootAttr->CollationRule == COLLATION_FILE_NAME);

    DbgPrint("  $INDEX_ROOT (%u bytes per index record, %u clusters) ", IndexRootAttr->SizeOfEntry, IndexRootAttr->ClustersPerIndexRecord);

    if (IndexRootAttr->Header.Flags == INDEX_ROOT_SMALL)
    {
        DbgPrint(" (small)\n");
    }
    else
    {
        ASSERT(IndexRootAttr->Header.Flags == INDEX_ROOT_LARGE);
        DbgPrint(" (large)\n");
    }

    DbgPrint("   Offset to first index: 0x%lx\n   Total size of index entries: 0x%lx\n   Allocated size of node: 0x%lx\n",
             IndexRootAttr->Header.FirstEntryOffset,
             IndexRootAttr->Header.TotalSizeOfEntries,
             IndexRootAttr->Header.AllocatedSize);
    CurrentOffset = IndexRootAttr->Header.FirstEntryOffset;
    CurrentNode = 0;
    // print details of every node in the index
    while (CurrentOffset < IndexRootAttr->Header.TotalSizeOfEntries)
    {
        PINDEX_ENTRY_ATTRIBUTE currentIndexExtry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexRootAttr + 0x10 + CurrentOffset);
        DbgPrint("   Index Node Entry %lu", CurrentNode++);
        if (BooleanFlagOn(currentIndexExtry->Flags, NTFS_INDEX_ENTRY_NODE))
            DbgPrint(" (Branch)");
        else
            DbgPrint(" (Leaf)");
        if (BooleanFlagOn(currentIndexExtry->Flags, NTFS_INDEX_ENTRY_END))
        {
            DbgPrint(" (Dummy Key)");
        }
        DbgPrint("\n    File Reference: 0x%016I64x\n", currentIndexExtry->Data.Directory.IndexedFile);
        DbgPrint("    Index Entry Length: 0x%x\n", currentIndexExtry->Length);
        DbgPrint("    Index Key Length: 0x%x\n", currentIndexExtry->KeyLength);

        // if this isn't the final (dummy) node, print info about the key (Filename attribute)
        if (!(currentIndexExtry->Flags & NTFS_INDEX_ENTRY_END))
        {
            UNICODE_STRING Name;
            DbgPrint("     Parent File Reference: 0x%016I64x\n", currentIndexExtry->FileName.DirectoryFileReferenceNumber);
            DbgPrint("     $FILENAME indexed: ");
            Name.Length = currentIndexExtry->FileName.NameLength * sizeof(WCHAR);
            Name.MaximumLength = Name.Length;
            Name.Buffer = currentIndexExtry->FileName.Name;
            DbgPrint("'%wZ'\n", &Name);
        }

        // if this node has a sub-node beneath it
        if (currentIndexExtry->Flags & NTFS_INDEX_ENTRY_NODE)
        {
            // Print the VCN of the sub-node
            PULONGLONG SubNodeVCN = (PULONGLONG)((ULONG_PTR)currentIndexExtry + currentIndexExtry->Length - sizeof(ULONGLONG));
            DbgPrint("    VCN of sub-node: 0x%llx\n", *SubNodeVCN);
        }

        CurrentOffset += currentIndexExtry->Length;
        ASSERT(currentIndexExtry->Length);
    }

}


static
VOID
NtfsDumpAttribute(PDEVICE_EXTENSION Vcb,
                  PNTFS_ATTR_RECORD Attribute)
{
    UNICODE_STRING Name;

    ULONGLONG lcn = 0;
    ULONGLONG runcount = 0;

    switch (Attribute->Type)
    {
        case AttributeFileName:
            NtfsDumpFileNameAttribute(Attribute);
            break;

        case AttributeStandardInformation:
            NtfsDumpStandardInformationAttribute(Attribute);
            break;

        case AttributeObjectId:
            DbgPrint("  $OBJECT_ID ");
            break;

        case AttributeSecurityDescriptor:
            DbgPrint("  $SECURITY_DESCRIPTOR ");
            break;

        case AttributeVolumeName:
            NtfsDumpVolumeNameAttribute(Attribute);
            break;

        case AttributeVolumeInformation:
            NtfsDumpVolumeInformationAttribute(Attribute);
            break;

        case AttributeData:
            DbgPrint("  $DATA ");
            //DataBuf = ExAllocatePool(NonPagedPool,AttributeLengthAllocated(Attribute));
            break;

        case AttributeIndexRoot:
            NtfsDumpIndexRootAttribute(Attribute);
            break;

        case AttributeIndexAllocation:
            DbgPrint("  $INDEX_ALLOCATION ");
            break;

        case AttributeBitmap:
            DbgPrint("  $BITMAP ");
            break;

        case AttributeReparsePoint:
            DbgPrint("  $REPARSE_POINT ");
            break;

        case AttributeEAInformation:
            DbgPrint("  $EA_INFORMATION ");
            break;

        case AttributeEA:
            DbgPrint("  $EA ");
            break;

        case AttributePropertySet:
            DbgPrint("  $PROPERTY_SET ");
            break;

        case AttributeLoggedUtilityStream:
            DbgPrint("  $LOGGED_UTILITY_STREAM ");
            break;

        default:
            DbgPrint("  Attribute %lx ",
                     Attribute->Type);
            break;
    }

    if (Attribute->Type != AttributeAttributeList)
    {
        if (Attribute->NameLength != 0)
        {
            Name.Length = Attribute->NameLength * sizeof(WCHAR);
            Name.MaximumLength = Name.Length;
            Name.Buffer = (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset);

            DbgPrint("'%wZ' ", &Name);
        }

        DbgPrint("(%s)\n",
                 Attribute->IsNonResident ? "non-resident" : "resident");

        if (Attribute->IsNonResident)
        {
            FindRun(Attribute,0,&lcn, &runcount);

            DbgPrint("  AllocatedSize %I64u  DataSize %I64u InitilizedSize %I64u\n",
                     Attribute->NonResident.AllocatedSize, Attribute->NonResident.DataSize, Attribute->NonResident.InitializedSize);
            DbgPrint("  logical clusters: %I64u - %I64u\n",
                     lcn, lcn + runcount - 1);
        }
        else
            DbgPrint("    %u bytes of data\n", Attribute->Resident.ValueLength);
    }
}


VOID NtfsDumpDataRunData(PUCHAR DataRun)
{
    UCHAR DataRunOffsetSize;
    UCHAR DataRunLengthSize;
    CHAR i;

    DbgPrint("%02x ", *DataRun);

    if (*DataRun == 0)
        return;

    DataRunOffsetSize = (*DataRun >> 4) & 0xF;
    DataRunLengthSize = *DataRun & 0xF;

    DataRun++;
    for (i = 0; i < DataRunLengthSize; i++)
    {
        DbgPrint("%02x ", *DataRun);
        DataRun++;
    }

    for (i = 0; i < DataRunOffsetSize; i++)
    {
        DbgPrint("%02x ", *DataRun);
        DataRun++;
    }

    NtfsDumpDataRunData(DataRun);
}


VOID
NtfsDumpDataRuns(PVOID StartOfRun,
                 ULONGLONG CurrentLCN)
{
    PUCHAR DataRun = StartOfRun;
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;

    if (CurrentLCN == 0)
    {
        DPRINT("Dumping data runs.\n\tData:\n\t\t");
        NtfsDumpDataRunData(StartOfRun);
        DbgPrint("\n\tRuns:\n\t\tOff\t\tLCN\t\tLength\n");
    }

    DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);

    if (DataRunOffset != -1)
        CurrentLCN += DataRunOffset;

    DbgPrint("\t\t%I64d\t", DataRunOffset);
    if (DataRunOffset < 99999)
        DbgPrint("\t");
    DbgPrint("%I64u\t", CurrentLCN);
    if (CurrentLCN < 99999)
        DbgPrint("\t");
    DbgPrint("%I64u\n", DataRunLength);

    if (*DataRun == 0)
        DbgPrint("\t\t00\n");
    else
        NtfsDumpDataRuns(DataRun, CurrentLCN);
}


VOID
NtfsDumpFileAttributes(PDEVICE_EXTENSION Vcb,
                       PFILE_RECORD_HEADER FileRecord)
{
    NTSTATUS Status;
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;

    Status = FindFirstAttribute(&Context, Vcb, FileRecord, FALSE, &Attribute);
    while (NT_SUCCESS(Status))
    {
        NtfsDumpAttribute(Vcb, Attribute);

        Status = FindNextAttribute(&Context, &Attribute);
    }

    FindCloseAttribute(&Context);
}

PFILENAME_ATTRIBUTE
GetFileNameFromRecord(PDEVICE_EXTENSION Vcb,
                      PFILE_RECORD_HEADER FileRecord,
                      UCHAR NameType)
{
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;
    PFILENAME_ATTRIBUTE Name;
    NTSTATUS Status;

    Status = FindFirstAttribute(&Context, Vcb, FileRecord, FALSE, &Attribute);
    while (NT_SUCCESS(Status))
    {
        if (Attribute->Type == AttributeFileName)
        {
            Name = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
            if (Name->NameType == NameType ||
                (Name->NameType == NTFS_FILE_NAME_WIN32_AND_DOS && NameType == NTFS_FILE_NAME_WIN32) ||
                (Name->NameType == NTFS_FILE_NAME_WIN32_AND_DOS && NameType == NTFS_FILE_NAME_DOS))
            {
                FindCloseAttribute(&Context);
                return Name;
            }
        }

        Status = FindNextAttribute(&Context, &Attribute);
    }

    FindCloseAttribute(&Context);
    return NULL;
}

/**
* GetPackedByteCount
* Returns the minimum number of bytes needed to represent the value of a
* 64-bit number. Used to encode data runs.
*/
UCHAR
GetPackedByteCount(LONGLONG NumberToPack,
                   BOOLEAN IsSigned)
{
    if (!IsSigned)
    {
        if (NumberToPack >= 0x0100000000000000)
            return 8;
        if (NumberToPack >= 0x0001000000000000)
            return 7;
        if (NumberToPack >= 0x0000010000000000)
            return 6;
        if (NumberToPack >= 0x0000000100000000)
            return 5;
        if (NumberToPack >= 0x0000000001000000)
            return 4;
        if (NumberToPack >= 0x0000000000010000)
            return 3;
        if (NumberToPack >= 0x0000000000000100)
            return 2;
        return 1;
    }

    if (NumberToPack > 0)
    {
        // we have to make sure the number that gets encoded won't be interpreted as negative
        if (NumberToPack >= 0x0080000000000000)
            return 8;
        if (NumberToPack >= 0x0000800000000000)
            return 7;
        if (NumberToPack >= 0x0000008000000000)
            return 6;
        if (NumberToPack >= 0x0000000080000000)
            return 5;
        if (NumberToPack >= 0x0000000000800000)
            return 4;
        if (NumberToPack >= 0x0000000000008000)
            return 3;
        if (NumberToPack >= 0x0000000000000080)
            return 2;
    }
    else
    {
        // negative number
        if (NumberToPack <= 0xff80000000000000)
            return 8;
        if (NumberToPack <= 0xffff800000000000)
            return 7;
        if (NumberToPack <= 0xffffff8000000000)
            return 6;
        if (NumberToPack <= 0xffffffff80000000)
            return 5;
        if (NumberToPack <= 0xffffffffff800000)
            return 4;
        if (NumberToPack <= 0xffffffffffff8000)
            return 3;
        if (NumberToPack <= 0xffffffffffffff80)
            return 2;
    }
    return 1;
}

NTSTATUS
GetLastClusterInDataRun(PDEVICE_EXTENSION Vcb, PNTFS_ATTR_RECORD Attribute, PULONGLONG LastCluster)
{
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG DataRunStartLCN;

    ULONGLONG LastLCN = 0;
    PUCHAR DataRun = (PUCHAR)Attribute + Attribute->NonResident.MappingPairsOffset;

    if (!Attribute->IsNonResident)
        return STATUS_INVALID_PARAMETER;

    while (1)
    {
        DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);

        if (DataRunOffset != -1)
        {
            // Normal data run.
            DataRunStartLCN = LastLCN + DataRunOffset;
            LastLCN = DataRunStartLCN;
            *LastCluster = LastLCN + DataRunLength - 1;
        }

        if (*DataRun == 0)
            break;
    }

    return STATUS_SUCCESS;
}

PSTANDARD_INFORMATION
NtfsFindStandardInformation(PDEVICE_EXTENSION Vcb,
                            PFILE_RECORD_HEADER FileRecord,
                            PULONG ValueLength)
{
    NTSTATUS Status;
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;

    if (ValueLength != NULL)
        *ValueLength = 0;

    Status = FindFirstAttribute(&Context, Vcb, FileRecord, FALSE, &Attribute);
    while (NT_SUCCESS(Status))
    {
        if (Attribute->Type == AttributeStandardInformation &&
            Attribute->IsNonResident == 0 &&
            Attribute->Resident.ValueLength >= NTFS_STD_INFO_V12_SIZE)
        {
            PSTANDARD_INFORMATION StdInfo;

            /* SecurityId and the rest of the tail are valid only at 72 bytes.
             * Callers that need them must test ValueLength first. */
            if (ValueLength != NULL)
                *ValueLength = Attribute->Resident.ValueLength;

            StdInfo = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute +
                                              Attribute->Resident.ValueOffset);
            FindCloseAttribute(&Context);
            return StdInfo;
        }

        Status = FindNextAttribute(&Context, &Attribute);
    }

    FindCloseAttribute(&Context);
    return NULL;
}

PSTANDARD_INFORMATION
GetStandardInformationFromRecord(PDEVICE_EXTENSION Vcb,
                                 PFILE_RECORD_HEADER FileRecord)
{
    return NtfsFindStandardInformation(Vcb, FileRecord, NULL);
}

/**
* @name GetFileNameAttributeLength
* @implemented
*
* Returns the size of a given FILENAME_ATTRIBUTE, in bytes.
*
* @param FileNameAttribute
* Pointer to a FILENAME_ATTRIBUTE to determine the size of.
*
* @remarks
* The length of a FILENAME_ATTRIBUTE is variable and is dependent on the length of the file name stored at the end.
* This function operates on the FILENAME_ATTRIBUTE proper, so don't try to pass it a PNTFS_ATTR_RECORD.
*/
ULONG GetFileNameAttributeLength(PFILENAME_ATTRIBUTE FileNameAttribute)
{
    ULONG Length = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + (FileNameAttribute->NameLength * sizeof(WCHAR));
    return Length;
}

PFILENAME_ATTRIBUTE
GetBestFileNameFromRecord(PDEVICE_EXTENSION Vcb,
                          PFILE_RECORD_HEADER FileRecord)
{
    PFILENAME_ATTRIBUTE FileName;

    FileName = GetFileNameFromRecord(Vcb, FileRecord, NTFS_FILE_NAME_POSIX);
    if (FileName == NULL)
    {
        FileName = GetFileNameFromRecord(Vcb, FileRecord, NTFS_FILE_NAME_WIN32);
        if (FileName == NULL)
        {
            FileName = GetFileNameFromRecord(Vcb, FileRecord, NTFS_FILE_NAME_DOS);
        }
    }

    return FileName;
}

C_ASSERT(FIELD_OFFSET(NTFS_ATTRIBUTE_LIST_ITEM, StartingVCN) == 8);
C_ASSERT(FIELD_OFFSET(NTFS_ATTRIBUTE_LIST_ITEM, MFTIndex) == 16);
C_ASSERT(FIELD_OFFSET(NTFS_ATTRIBUTE_LIST_ITEM, Instance) == 24);

/* One attribute-list entry. The name, if any, follows the 26-byte header and
 * the whole entry is padded to eight bytes. A trailing zero entry stops the
 * walk, which treats a length of zero as the end. */
static
ULONG
NtfsWriteListItem(PUCHAR Dest,
                  ULONG Type,
                  USHORT Instance,
                  ULONGLONG FileReference,
                  ULONGLONG StartingVcn,
                  PCWSTR Name,
                  UCHAR NameLength)
{
    ULONG NameBytes = NameLength * sizeof(WCHAR);
    ULONG Length = ALIGN_UP_BY(26 + NameBytes, ATTR_RECORD_ALIGNMENT);

    RtlZeroMemory(Dest, Length);
    *(PULONG)Dest = Type;
    *(PUSHORT)(Dest + 4) = (USHORT)Length;
    Dest[6] = NameLength;
    Dest[7] = 26;
    *(PULONGLONG)(Dest + 8) = StartingVcn;
    *(PULONGLONG)(Dest + 16) = FileReference;
    *(PUSHORT)(Dest + 24) = Instance;

    if (NameBytes != 0)
        RtlCopyMemory(Dest + 26, Name, NameBytes);

    return Length;
}

static
BOOLEAN
NtfsRecordHasAttributeList(PFILE_RECORD_HEADER FileRecord)
{
    PNTFS_ATTR_RECORD Attribute;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute->Type != AttributeEnd && Attribute->Length != 0)
    {
        if (Attribute->Type == AttributeAttributeList)
            return TRUE;

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    return FALSE;
}

/* Resident $ATTRIBUTE_LIST describing the attributes already in this record.
 * The list attribute itself is included. Attributes added afterwards stay in
 * this record and are found by the base-record walk. */
static
NTSTATUS
NtfsInsertAttributeList(PFILE_RECORD_HEADER FileRecord)
{
    PUCHAR ListBuffer;
    ULONG ListLength = 0;
    ULONG ListCapacity;
    ULONG ResidentHeader;
    ULONG ValueOffset;
    ULONG AttributeLength;
    USHORT ListInstance;
    ULONGLONG FileReference;
    PNTFS_ATTR_RECORD Attribute;
    PNTFS_ATTR_RECORD InsertAt;
    ULONG InsertOffset;
    ULONG Tail;

    if (NtfsRecordHasAttributeList(FileRecord))
        return STATUS_SUCCESS;

    ListCapacity = FileRecord->BytesAllocated;
    ListBuffer = ExAllocatePoolWithTag(NonPagedPool, ListCapacity, TAG_NTFS);
    if (ListBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    FileReference = ((ULONGLONG)FileRecord->MFTRecordNumber) |
                    ((ULONGLONG)FileRecord->SequenceNumber << 48);
    ListInstance = FileRecord->NextAttributeNumber;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    InsertAt = Attribute;
    while (Attribute->Type != AttributeEnd && Attribute->Length != 0 &&
           Attribute->Type < AttributeAttributeList)
    {
        PCWSTR Name = NULL;

        if (Attribute->NameLength != 0)
            Name = (PCWSTR)((ULONG_PTR)Attribute + Attribute->NameOffset);

        if (ListLength + 64 > ListCapacity)
        {
            ExFreePoolWithTag(ListBuffer, TAG_NTFS);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ListLength += NtfsWriteListItem(ListBuffer + ListLength,
                                        Attribute->Type,
                                        Attribute->Instance,
                                        FileReference,
                                        0,
                                        Name,
                                        Attribute->NameLength);
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
        InsertAt = Attribute;
    }

    if (ListLength + 64 > ListCapacity)
    {
        ExFreePoolWithTag(ListBuffer, TAG_NTFS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ListLength += NtfsWriteListItem(ListBuffer + ListLength,
                                    AttributeAttributeList,
                                    ListInstance,
                                    FileReference,
                                    0,
                                    NULL,
                                    0);

    Attribute = InsertAt;
    while (Attribute->Type != AttributeEnd && Attribute->Length != 0)
    {
        PCWSTR Name = NULL;

        if (Attribute->NameLength != 0)
            Name = (PCWSTR)((ULONG_PTR)Attribute + Attribute->NameOffset);

        if (ListLength + 64 > ListCapacity)
        {
            ExFreePoolWithTag(ListBuffer, TAG_NTFS);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ListLength += NtfsWriteListItem(ListBuffer + ListLength,
                                        Attribute->Type,
                                        Attribute->Instance,
                                        FileReference,
                                        0,
                                        Name,
                                        Attribute->NameLength);
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    RtlZeroMemory(ListBuffer + ListLength, sizeof(ULONGLONG));
    ListLength += sizeof(ULONGLONG);

    ResidentHeader = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    ValueOffset = ALIGN_UP_BY(ResidentHeader, VALUE_OFFSET_ALIGNMENT);
    AttributeLength = ALIGN_UP_BY(ValueOffset + ListLength, ATTR_RECORD_ALIGNMENT);

    if (FileRecord->BytesInUse + AttributeLength > FileRecord->BytesAllocated)
    {
        ExFreePoolWithTag(ListBuffer, TAG_NTFS);
        return STATUS_DISK_FULL;
    }

    InsertOffset = (ULONG)((ULONG_PTR)InsertAt - (ULONG_PTR)FileRecord);
    Tail = FileRecord->BytesInUse - InsertOffset;
    RtlMoveMemory((PUCHAR)FileRecord + InsertOffset + AttributeLength,
                  (PUCHAR)FileRecord + InsertOffset,
                  Tail);

    Attribute = (PNTFS_ATTR_RECORD)((PUCHAR)FileRecord + InsertOffset);
    RtlZeroMemory(Attribute, AttributeLength);
    Attribute->Type = AttributeAttributeList;
    Attribute->Length = AttributeLength;
    Attribute->NameLength = 0;
    Attribute->NameOffset = (USHORT)ResidentHeader;
    Attribute->Instance = FileRecord->NextAttributeNumber++;
    Attribute->Resident.ValueLength = ListLength;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;
    RtlCopyMemory((PUCHAR)Attribute + ValueOffset, ListBuffer, ListLength);

    FileRecord->BytesInUse += AttributeLength;

    ExFreePoolWithTag(ListBuffer, TAG_NTFS);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsAppendNamedData(PFILE_RECORD_HEADER FileRecord,
                    PCWSTR StreamName,
                    ULONG NameLength)
{
    PNTFS_ATTR_RECORD Attribute;
    ULONG ResidentHeader;
    ULONG ValueOffset;
    ULONG AttributeLength;
    ULONG EndMarker;

    ResidentHeader = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    ValueOffset = ALIGN_UP_BY(ResidentHeader + (NameLength * sizeof(WCHAR)), VALUE_OFFSET_ALIGNMENT);
    AttributeLength = ALIGN_UP_BY(ValueOffset, ATTR_RECORD_ALIGNMENT);

    if (FileRecord->BytesInUse + AttributeLength > FileRecord->BytesAllocated)
        return STATUS_DISK_FULL;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute->Type != AttributeEnd && Attribute->Length != 0)
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);

    if (Attribute->Type != AttributeEnd)
        return STATUS_FILE_CORRUPT_ERROR;

    EndMarker = Attribute->Length;
    RtlZeroMemory(Attribute, AttributeLength);
    Attribute->Type = AttributeData;
    Attribute->Length = AttributeLength;
    Attribute->NameLength = (UCHAR)NameLength;
    Attribute->NameOffset = (USHORT)ResidentHeader;
    Attribute->Instance = FileRecord->NextAttributeNumber++;
    Attribute->Resident.ValueLength = 0;
    Attribute->Resident.ValueOffset = (USHORT)ValueOffset;
    RtlCopyMemory((PUCHAR)Attribute + ResidentHeader, StreamName, NameLength * sizeof(WCHAR));

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + AttributeLength);
    SetFileRecordEnd(FileRecord, Attribute, EndMarker);
    return STATUS_SUCCESS;
}

NTSTATUS
NtfsAddNamedData(PDEVICE_EXTENSION DeviceExt,
                 ULONGLONG MftIndex,
                 PCWSTR StreamName)
{
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT Context = NULL;
    NTSTATUS Status;
    ULONG NameLength;
    BOOLEAN AddedList = FALSE;

    if (StreamName == NULL || StreamName[0] == UNICODE_NULL)
        return STATUS_OBJECT_NAME_INVALID;

    NameLength = (ULONG)wcslen(StreamName);
    if (NameLength == 0 || NameLength > 255)
        return STATUS_OBJECT_NAME_INVALID;

    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ReadFileRecord(DeviceExt, MftIndex & NTFS_MFT_MASK, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    Status = FindAttribute(DeviceExt,
                           FileRecord,
                           AttributeData,
                           StreamName,
                           NameLength,
                           &Context,
                           NULL);
    if (NT_SUCCESS(Status))
    {
        ReleaseAttributeContext(Context);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return STATUS_SUCCESS;
    }

    if (!NtfsRecordHasAttributeList(FileRecord))
    {
        Status = NtfsInsertAttributeList(FileRecord);
        if (!NT_SUCCESS(Status))
        {
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            return Status;
        }
        AddedList = TRUE;
    }

    Status = NtfsAppendNamedData(FileRecord, StreamName, NameLength);
    if (NT_SUCCESS(Status) || AddedList)
    {
        NTSTATUS Written;

        Written = UpdateFileRecord(DeviceExt, MftIndex & NTFS_MFT_MASK, FileRecord);
        if (!NT_SUCCESS(Written))
            Status = Written;
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
    return Status;
}

/* EOF */
