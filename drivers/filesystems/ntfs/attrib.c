/*
 *  ReactOS kernel
 *  Copyright (C) 2002,2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/attrib.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Valentin Verkhovsky
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"
#include <ntintsafe.h>

#define NDEBUG
#include <debug.h>

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

    // set timestamps
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
    DPRINT1("Pathname: %wZ\n", &FileObject->FileName);

    FsRtlDissectName(FileObject->FileName, &Current, &Remaining);

    FilenameNoPath.Buffer = Current.Buffer;
    FilenameNoPath.MaximumLength = FilenameNoPath.Length = Current.Length;

    while (Current.Length != 0)
    {
        DPRINT1("Current: %wZ\n", &Current);

        if (Remaining.Length != 0)
        {
            FilenameNoPath.Buffer = Remaining.Buffer;
            FilenameNoPath.Length = FilenameNoPath.MaximumLength = Remaining.Length;
        }

        FirstEntry = 0;
        Status = NtfsFindMftRecord(DeviceExt,
                                   CurrentMFTIndex,
                                   &Current,
                                   &FirstEntry,
                                   FALSE,
                                   CaseSensitive,
                                   &CurrentMFTIndex);
        if (!NT_SUCCESS(Status))
            break;

        if (Remaining.Length == 0 )
        {
            if (Current.Length != 0)
            {
                FilenameNoPath.Buffer = Current.Buffer;
                FilenameNoPath.Length = FilenameNoPath.MaximumLength = Current.Length;
            }
            break;
        }

        FsRtlDissectName(Remaining, &Current, &Remaining);
    }

    DPRINT1("MFT Index of parent: %I64u\n", CurrentMFTIndex);

    // set reference to parent directory
    FileNameAttribute->DirectoryFileReferenceNumber = CurrentMFTIndex;
    *ParentMftIndex = CurrentMFTIndex;

    DPRINT1("SequenceNumber: 0x%02x\n", FileRecord->SequenceNumber);

    // The highest 2 bytes should be the sequence number, unless the parent happens to be root
    if (CurrentMFTIndex == NTFS_FILE_ROOT)
        FileNameAttribute->DirectoryFileReferenceNumber |= (ULONGLONG)NTFS_FILE_ROOT << 48;
    else
        FileNameAttribute->DirectoryFileReferenceNumber |= (ULONGLONG)FileRecord->SequenceNumber << 48;

    DPRINT1("FileNameAttribute->DirectoryFileReferenceNumber: 0x%016I64x\n", FileNameAttribute->DirectoryFileReferenceNumber);

    FileNameAttribute->NameLength = FilenameNoPath.Length / sizeof(WCHAR);
    RtlCopyMemory(FileNameAttribute->Name, FilenameNoPath.Buffer, FilenameNoPath.Length);

    // For now, we're emulating the way Windows behaves when 8.3 name generation is disabled
    // TODO: add DOS Filename as needed
    if (!CaseSensitive && RtlIsNameLegalDOS8Dot3(&FilenameNoPath, NULL, NULL))
        FileNameAttribute->NameType = NTFS_FILE_NAME_WIN32_AND_DOS;
    else
        FileNameAttribute->NameType = NTFS_FILE_NAME_POSIX;

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
* STATUS_BUFFER_TOO_SMALL if ConvertLargeMCBToDataRuns() fails.
* STATUS_NOT_IMPLEMENTED if we need to migrate the attribute to an attribute list (TODO).
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

    PUCHAR RunBuffer;
    ULONG RunBufferSize;

    if (!AttrContext->pRecord->IsNonResident)
        return STATUS_INVALID_PARAMETER;

    if (AttrContext->pRecord->NonResident.AllocatedSize != 0)
        NextVBN = AttrContext->pRecord->NonResident.HighestVCN + 1;

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

    // Convert the map control block back to encoded data runs
    ConvertLargeMCBToDataRuns(&AttrContext->DataRunsMCB, RunBuffer, Vcb->NtfsInfo.BytesPerCluster, &RunBufferSize);

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
            DPRINT1("FIXME: Need to create attribute list! Max Data Run Length available: %d, RunBufferSize: %d\n", DataRunMaxLength, RunBufferSize);
            ExFreePoolWithTag(RunBuffer, TAG_NTFS);
            return STATUS_NOT_IMPLEMENTED;
        }

        // Are there more attributes after the one we're resizing?
        if (NextAttribute->Type != AttributeEnd)
        {
            PNTFS_ATTR_RECORD FinalAttribute;

            // Calculate where to move the trailing attributes
            ULONG_PTR MoveTo = (ULONG_PTR)DestinationAttribute + AttrContext->pRecord->NonResident.MappingPairsOffset + RunBufferSize;
            MoveTo = ALIGN_UP_BY(MoveTo, ATTR_RECORD_ALIGNMENT);

            DPRINT1("Moving attribute(s) after this one starting with type 0x%lx\n", NextAttribute->Type);

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
    AttrContext->pRecord->NonResident.HighestVCN = max(NextVBN - 1 + RunLength,
                                                     AttrContext->pRecord->NonResident.HighestVCN);

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

    NtfsDumpDataRuns((PUCHAR)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset), 0);

    return Status;
}

/**
* @name AddStandardInformation
* @implemented
*
* Adds a $STANDARD_INFORMATION attribute to a given FileRecord.
*
* @param FileRecord
* Pointer to a complete file record to add the attribute to. Caller is responsible for
* ensuring FileRecord is large enough to contain $STANDARD_INFORMATION.
*
* @param AttributeAddress
* Pointer to the region of memory that will receive the $STANDARD_INFORMATION attribute.
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
AddStandardInformation(PFILE_RECORD_HEADER FileRecord,
                       PNTFS_ATTR_RECORD AttributeAddress)
{
    ULONG ResidentHeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    PSTANDARD_INFORMATION StandardInfo = (PSTANDARD_INFORMATION)((LONG_PTR)AttributeAddress + ResidentHeaderLength);
    LARGE_INTEGER SystemTime;
    ULONG FileRecordEnd = AttributeAddress->Length;

    if (AttributeAddress->Type != AttributeEnd)
    {
        DPRINT1("FIXME: Can only add $STANDARD_INFORMATION attribute to the end of a file record.\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    AttributeAddress->Type = AttributeStandardInformation;
    AttributeAddress->Length = sizeof(STANDARD_INFORMATION) + ResidentHeaderLength;
    AttributeAddress->Length = ALIGN_UP_BY(AttributeAddress->Length, ATTR_RECORD_ALIGNMENT);
    AttributeAddress->Resident.ValueLength = sizeof(STANDARD_INFORMATION);
    AttributeAddress->Resident.ValueOffset = ResidentHeaderLength;
    AttributeAddress->Instance = FileRecord->NextAttributeNumber++;

    // set dates and times
    KeQuerySystemTime(&SystemTime);
    StandardInfo->CreationTime = SystemTime.QuadPart;
    StandardInfo->ChangeTime = SystemTime.QuadPart;
    StandardInfo->LastWriteTime = SystemTime.QuadPart;
    StandardInfo->LastAccessTime = SystemTime.QuadPart;
    StandardInfo->FileAttribute = NTFS_FILE_TYPE_ARCHIVE;

    // move the attribute-end and file-record-end markers to the end of the file record
    AttributeAddress = (PNTFS_ATTR_RECORD)((ULONG_PTR)AttributeAddress + AttributeAddress->Length);
    SetFileRecordEnd(FileRecord, AttributeAddress, FileRecordEnd);

    return STATUS_SUCCESS;
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
                          PUCHAR RunBuffer,
                          ULONG MaxBufferSize,
                          PULONG UsedBufferSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RunBufferOffset = 0;
    LONGLONG  DataRunOffset;
    ULONGLONG LastLCN = 0;
    LONGLONG Vbn, Lbn, Count;
    ULONG i;


    DPRINT("\t[Vbn, Lbn, Count]\n");

    // convert each mcb entry to a data run
    for (i = 0; FsRtlGetNextLargeMcbEntry(DataRunsMCB, i, &Vbn, &Lbn, &Count); i++)
    {
        UCHAR DataRunOffsetSize = 0;
        UCHAR DataRunLengthSize = 0;
        UCHAR ControlByte = 0;

        // [vbn, lbn, count]
        DPRINT("\t[%I64d, %I64d,%I64d]\n", Vbn, Lbn, Count);

        // TODO: check for holes and convert to sparse runs
        DataRunOffset = Lbn - LastLCN;
        LastLCN = Lbn;

        // now we need to determine how to represent DataRunOffset with the minimum number of bytes
        DPRINT("Determining how many bytes needed to represent %I64x\n", DataRunOffset);
        DataRunOffsetSize = GetPackedByteCount(DataRunOffset, TRUE);
        DPRINT("%d bytes needed.\n", DataRunOffsetSize);

        // determine how to represent DataRunLengthSize with the minimum number of bytes
        DPRINT("Determining how many bytes needed to represent %I64x\n", Count);
        DataRunLengthSize = GetPackedByteCount(Count, TRUE);
        DPRINT("%d bytes needed.\n", DataRunLengthSize);

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

    PFILE_RECORD_HEADER BitmapRecord;
    PNTFS_ATTR_CONTEXT DataContext;
    ULONGLONG BitmapDataSize;
    PUCHAR BitmapData;
    RTL_BITMAP Bitmap;
    ULONG LengthWritten;

    if (!AttrContext->pRecord->IsNonResident)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Read the $Bitmap file
    BitmapRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (BitmapRecord == NULL)
    {
        DPRINT1("Error: Unable to allocate memory for bitmap file record!\n");
        return STATUS_NO_MEMORY;
    }

    Status = ReadFileRecord(Vcb, NTFS_FILE_BITMAP, BitmapRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Unable to read file record for bitmap!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BitmapRecord);
        return 0;
    }

    Status = FindAttribute(Vcb, BitmapRecord, AttributeData, L"", 0, &DataContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Unable to find data attribute for bitmap file!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BitmapRecord);
        return 0;
    }

    BitmapDataSize = AttributeDataLength(DataContext->pRecord);
    BitmapDataSize = min(BitmapDataSize, ULONG_MAX);
    ASSERT((BitmapDataSize * 8) >= Vcb->NtfsInfo.ClusterCount);
    BitmapData = ExAllocatePoolWithTag(NonPagedPool, ROUND_UP(BitmapDataSize, Vcb->NtfsInfo.BytesPerSector), TAG_NTFS);
    if (BitmapData == NULL)
    {
        DPRINT1("Error: Unable to allocate memory for bitmap file data!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BitmapRecord);
        return 0;
    }

    ReadAttribute(Vcb, DataContext, 0, (PCHAR)BitmapData, (ULONG)BitmapDataSize);

    RtlInitializeBitMap(&Bitmap, (PULONG)BitmapData, Vcb->NtfsInfo.ClusterCount);

    // free clusters in $BITMAP file
    while (ClustersLeftToFree > 0)
    {
        LONGLONG LargeVbn, LargeLbn;

        if (!FsRtlLookupLastLargeMcbEntry(&AttrContext->DataRunsMCB, &LargeVbn, &LargeLbn))
        {
            Status = STATUS_INVALID_PARAMETER;
            DPRINT1("DRIVER ERROR: FreeClusters called to free %lu clusters, which is %lu more clusters than are assigned to attribute!",
                    ClustersToFree,
                    ClustersLeftToFree);
            break;
        }

        if (LargeLbn != -1)
        {
            // deallocate this cluster
            RtlClearBits(&Bitmap, LargeLbn, 1);
        }
        FsRtlTruncateLargeMcb(&AttrContext->DataRunsMCB, AttrContext->pRecord->NonResident.HighestVCN);

        // decrement HighestVCN, but don't let it go below 0
        AttrContext->pRecord->NonResident.HighestVCN = min(AttrContext->pRecord->NonResident.HighestVCN, AttrContext->pRecord->NonResident.HighestVCN - 1);
        ClustersLeftToFree--;
    }

    // update $BITMAP file on disk
    Status = WriteAttribute(Vcb, DataContext, 0, BitmapData, (ULONG)BitmapDataSize, &LengthWritten, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        ReleaseAttributeContext(DataContext);
        ExFreePoolWithTag(BitmapData, TAG_NTFS);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BitmapRecord);
        return Status;
    }

    ReleaseAttributeContext(DataContext);
    ExFreePoolWithTag(BitmapData, TAG_NTFS);
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BitmapRecord);

    // Save updated data runs to file record

    // Allocate some memory for a new RunBuffer
    RunBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (!RunBuffer)
    {
        DPRINT1("ERROR: Couldn't allocate memory for data runs!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Convert the map control block back to encoded data runs
    ConvertLargeMCBToDataRuns(&AttrContext->DataRunsMCB, RunBuffer, Vcb->NtfsInfo.BytesPerCluster, &RunBufferSize);

    // Update HighestVCN
    DestinationAttribute->NonResident.HighestVCN = AttrContext->pRecord->NonResident.HighestVCN;

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

    NtfsDumpDataRuns((PUCHAR)((ULONG_PTR)DestinationAttribute + DestinationAttribute->NonResident.MappingPairsOffset), 0);

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
        DPRINT1("Dumping data runs.\n\tData:\n\t\t");
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
GetStandardInformationFromRecord(PDEVICE_EXTENSION Vcb,
                                 PFILE_RECORD_HEADER FileRecord)
{
    NTSTATUS Status;
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;
    PSTANDARD_INFORMATION StdInfo;

    Status = FindFirstAttribute(&Context, Vcb, FileRecord, FALSE, &Attribute);
    while (NT_SUCCESS(Status))
    {
        if (Attribute->Type == AttributeStandardInformation)
        {
            StdInfo = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
            FindCloseAttribute(&Context);
            return StdInfo;
        }

        Status = FindNextAttribute(&Context, &Attribute);
    }

    FindCloseAttribute(&Context);
    return NULL;
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

/* EOF */
