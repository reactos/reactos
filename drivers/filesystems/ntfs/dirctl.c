/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003, 2014 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/dirctl.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Pierre Schweitzer (pierre@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/**
* @name NtfsAddFilenameToDirectory
* @implemented
*
* Adds a $FILE_NAME attribute to a given directory index.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION.
*
* @param DirectoryMftIndex
* Mft index of the parent directory which will receive the file.
*
* @param FileReferenceNumber
* File reference of the file to be added to the directory. This is a combination of the 
* Mft index and sequence number.
*
* @param FilenameAttribute
* Pointer to the FILENAME_ATTRIBUTE of the file being added to the directory.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_NOT_IMPLEMENTED if target address isn't at the end of the given file record.
*
* @remarks
* WIP - Can only support an empty directory.
* One FILENAME_ATTRIBUTE is added to the directory's index for each link to that file. So, each
* file which contains one FILENAME_ATTRIBUTE for a long name and another for the 8.3 name, will
* get both attributes added to its parent directory.
*/
NTSTATUS
NtfsAddFilenameToDirectory(PDEVICE_EXTENSION DeviceExt,
                           ULONGLONG DirectoryMftIndex,
                           ULONGLONG FileReferenceNumber,
                           PFILENAME_ATTRIBUTE FilenameAttribute)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_RECORD_HEADER ParentFileRecord;
    PNTFS_ATTR_CONTEXT DirectoryContext;
    PINDEX_ROOT_ATTRIBUTE I30IndexRoot;
    ULONG IndexRootOffset;
    ULONGLONG I30IndexRootLength;
    PINDEX_ENTRY_ATTRIBUTE IndexNodeEntry;
    ULONG LengthWritten;
    PNTFS_ATTR_RECORD DestinationAttribute;
    PINDEX_ROOT_ATTRIBUTE NewIndexRoot;
    ULONG AttributeLength;
    PNTFS_ATTR_RECORD NextAttribute;

    // Allocate memory for the parent directory
    ParentFileRecord = ExAllocatePoolWithTag(NonPagedPool,
                                             DeviceExt->NtfsInfo.BytesPerFileRecord,
                                             TAG_NTFS);
    if (!ParentFileRecord)
    {
        DPRINT1("ERROR: Couldn't allocate memory for file record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Open the parent directory
    Status = ReadFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        DPRINT1("ERROR: Couldn't read parent directory with index %I64u\n",
                DirectoryMftIndex);
        return Status;
    }

    DPRINT1("Dumping old parent file record:\n");
    NtfsDumpFileRecord(DeviceExt, ParentFileRecord);

    // Find the index root attribute for the directory
    Status = FindAttribute(DeviceExt,
                           ParentFileRecord,
                           AttributeIndexRoot,
                           L"$I30",
                           4,
                           &DirectoryContext,
                           &IndexRootOffset);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $I30 $INDEX_ROOT attribute for parent directory with MFT #: %I64u!\n",
                DirectoryMftIndex);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return Status;
    }

    I30IndexRootLength = AttributeDataLength(&DirectoryContext->Record);

    // Allocate memory for the index root data
    I30IndexRoot = (PINDEX_ROOT_ATTRIBUTE)ExAllocatePoolWithTag(NonPagedPool, I30IndexRootLength, TAG_NTFS);
    if (!I30IndexRoot)
    {
        DPRINT1("ERROR: Couldn't allocate memory for index root attribute!\n");
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
    }

    // Read the Index Root
    Status = ReadAttribute(DeviceExt, DirectoryContext, 0, (PCHAR)I30IndexRoot, I30IndexRootLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couln't read index root attribute for Mft index #%I64u\n", DirectoryMftIndex);
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return Status;
    }

    // Make sure it's empty (temporarily)
    IndexNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)I30IndexRoot + I30IndexRoot->Header.FirstEntryOffset + 0x10);
    if (IndexNodeEntry->Data.Directory.IndexedFile != 0 || IndexNodeEntry->Flags != 2)
    {
        DPRINT1("FIXME: File-creation is only supported in empty directories right now! Be patient! :)\n");
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return STATUS_NOT_IMPLEMENTED;
    }
    
    // Now we need to setup a new index root attribute to replace the old one
    NewIndexRoot = ExAllocatePoolWithTag(NonPagedPool, DeviceExt->NtfsInfo.BytesPerIndexRecord, TAG_NTFS);
    if (!NewIndexRoot)
    {
        DPRINT1("ERROR: Unable to allocate memory for new index root attribute!\n");
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    // Setup the new index record
    RtlZeroMemory(NewIndexRoot, DeviceExt->NtfsInfo.BytesPerIndexRecord);   // shouldn't be necessary but aids in debugging

    NewIndexRoot->AttributeType = AttributeFileName;
    NewIndexRoot->CollationRule = COLLATION_FILE_NAME;
    NewIndexRoot->SizeOfEntry = DeviceExt->NtfsInfo.BytesPerIndexRecord;
    // If Bytes per index record is less than cluster size, clusters per index record becomes sectors per index
    if(NewIndexRoot->SizeOfEntry < DeviceExt->NtfsInfo.BytesPerCluster)
        NewIndexRoot->ClustersPerIndexRecord = NewIndexRoot->SizeOfEntry / DeviceExt->NtfsInfo.BytesPerSector;
    else    
        NewIndexRoot->ClustersPerIndexRecord = NewIndexRoot->SizeOfEntry / DeviceExt->NtfsInfo.BytesPerCluster;

    // Setup the Index node header
    NewIndexRoot->Header.FirstEntryOffset = 0x10;
    NewIndexRoot->Header.Flags = INDEX_ROOT_SMALL;
    // still need to calculate sizes

    // The first index node entry will be for the filename we're adding
    IndexNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)NewIndexRoot + NewIndexRoot->Header.FirstEntryOffset + 0x10);
    IndexNodeEntry->Data.Directory.IndexedFile = FileReferenceNumber;
    IndexNodeEntry->Flags = INDEX_ROOT_SMALL;
    IndexNodeEntry->KeyLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + (2 * FilenameAttribute->NameLength);
    IndexNodeEntry->Length = ALIGN_UP_BY(IndexNodeEntry->KeyLength, 8) + FIELD_OFFSET(INDEX_ENTRY_ATTRIBUTE, FileName);

    // Now we can calculate the Node length (temp logic)
    NewIndexRoot->Header.TotalSizeOfEntries = NewIndexRoot->Header.FirstEntryOffset + IndexNodeEntry->Length + 0x10;
    NewIndexRoot->Header.AllocatedSize = NewIndexRoot->Header.TotalSizeOfEntries;

    DPRINT1("New Index Node Entry Stream Length: %u\nNew Inde Node Entry Length: %u\n",
            IndexNodeEntry->KeyLength,
            IndexNodeEntry->Length);

    // copy over the attribute proper
    RtlCopyMemory(&IndexNodeEntry->FileName, FilenameAttribute, IndexNodeEntry->KeyLength);

    // Now setup the dummy key
    IndexNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexNodeEntry + IndexNodeEntry->Length);

    IndexNodeEntry->Data.Directory.IndexedFile = 0;
    IndexNodeEntry->Length = 0x10;
    IndexNodeEntry->KeyLength = 0;
    IndexNodeEntry->Flags = NTFS_INDEX_ENTRY_END;

    // This is when we'd normally setup the length (already done above)

    // Write back the new index root attribute to the parent directory file record

    // First, we need to make sure the attribute is large enough.
    // We can't set the size as we normally would, because if we extend past the file record, 
    // we must create an index allocation and index bitmap (TODO). Also TODO: support file records with
    // $ATTRIBUTE_LIST's.
    AttributeLength = NewIndexRoot->Header.AllocatedSize + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header);
    DestinationAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)ParentFileRecord + IndexRootOffset);

    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)DestinationAttribute + DestinationAttribute->Length);
    if (NextAttribute->Type != AttributeEnd)
    {
        DPRINT1("FIXME: For now, only resizing index root at the end of a file record is supported!\n");
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return STATUS_NOT_IMPLEMENTED;
    }
    
    // Update the length of the attribute in the file record of the parent directory
    InternalSetResidentAttributeLength(DirectoryContext,
                                       ParentFileRecord,
                                       IndexRootOffset,
                                       AttributeLength);

    Status = UpdateFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to update file record of directory with index: %llx\n", DirectoryMftIndex);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        return Status;
    }

    // Update the parent directory with the new index root
    Status = WriteAttribute(DeviceExt,
                            DirectoryContext,
                            0,
                            (PUCHAR)NewIndexRoot,
                            AttributeLength,
                            &LengthWritten);
    if (!NT_SUCCESS(Status) )
    {
        DPRINT1("ERROR: Unable to write new index root attribute to parent directory!\n");
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ReleaseAttributeContext(DirectoryContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);
        return Status;
    }
    
    // re-read the parent file record, so we can dump it
    Status = ReadFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't read parent directory after messing with it!\n");
    }
    else
    {
        DPRINT1("Dumping new parent file record:\n");
        NtfsDumpFileRecord(DeviceExt, ParentFileRecord);
    }

    // Cleanup
    ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
    ReleaseAttributeContext(DirectoryContext);
    ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
    ExFreePoolWithTag(ParentFileRecord, TAG_NTFS);

    return Status;
}

ULONGLONG
NtfsGetFileSize(PDEVICE_EXTENSION DeviceExt,
                PFILE_RECORD_HEADER FileRecord,
                PCWSTR Stream,
                ULONG StreamLength,
                PULONGLONG AllocatedSize)
{
    ULONGLONG Size = 0ULL;
    ULONGLONG Allocated = 0ULL;
    NTSTATUS Status;
    PNTFS_ATTR_CONTEXT DataContext;

    Status = FindAttribute(DeviceExt, FileRecord, AttributeData, Stream, StreamLength, &DataContext, NULL);
    if (NT_SUCCESS(Status))
    {
        Size = AttributeDataLength(&DataContext->Record);
        Allocated = AttributeAllocatedLength(&DataContext->Record);
        ReleaseAttributeContext(DataContext);
    }

    if (AllocatedSize != NULL) *AllocatedSize = Allocated;

    return Size;
}


static NTSTATUS
NtfsGetNameInformation(PDEVICE_EXTENSION DeviceExt,
                       PFILE_RECORD_HEADER FileRecord,
                       ULONGLONG MFTIndex,
                       PFILE_NAMES_INFORMATION Info,
                       ULONG BufferLength)
{
    ULONG Length;
    PFILENAME_ATTRIBUTE FileName;

    DPRINT("NtfsGetNameInformation() called\n");

    FileName = GetBestFileNameFromRecord(DeviceExt, FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(DeviceExt, FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    Length = FileName->NameLength * sizeof (WCHAR);
    if ((sizeof(FILE_NAMES_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_NAMES_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, FileName->Name, Length);

    return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsGetDirectoryInformation(PDEVICE_EXTENSION DeviceExt,
                            PFILE_RECORD_HEADER FileRecord,
                            ULONGLONG MFTIndex,
                            PFILE_DIRECTORY_INFORMATION Info,
                            ULONG BufferLength)
{
    ULONG Length;
    PFILENAME_ATTRIBUTE FileName;
    PSTANDARD_INFORMATION StdInfo;

    DPRINT("NtfsGetDirectoryInformation() called\n");

    FileName = GetBestFileNameFromRecord(DeviceExt, FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(DeviceExt, FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    StdInfo = GetStandardInformationFromRecord(DeviceExt, FileRecord);
    ASSERT(StdInfo != NULL);

    Length = FileName->NameLength * sizeof (WCHAR);
    if ((sizeof(FILE_DIRECTORY_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, FileName->Name, Length);

    Info->CreationTime.QuadPart = FileName->CreationTime;
    Info->LastAccessTime.QuadPart = FileName->LastAccessTime;
    Info->LastWriteTime.QuadPart = FileName->LastWriteTime;
    Info->ChangeTime.QuadPart = FileName->ChangeTime;

    /* Convert file flags */
    NtfsFileFlagsToAttributes(FileName->FileAttributes | StdInfo->FileAttribute, &Info->FileAttributes);

    Info->EndOfFile.QuadPart = NtfsGetFileSize(DeviceExt, FileRecord, L"", 0, (PULONGLONG)&Info->AllocationSize.QuadPart);

    Info->FileIndex = MFTIndex;

    return STATUS_SUCCESS;
}


static NTSTATUS
NtfsGetFullDirectoryInformation(PDEVICE_EXTENSION DeviceExt,
                                PFILE_RECORD_HEADER FileRecord,
                                ULONGLONG MFTIndex,
                                PFILE_FULL_DIRECTORY_INFORMATION Info,
                                ULONG BufferLength)
{
    ULONG Length;
    PFILENAME_ATTRIBUTE FileName;
    PSTANDARD_INFORMATION StdInfo;

    DPRINT("NtfsGetFullDirectoryInformation() called\n");

    FileName = GetBestFileNameFromRecord(DeviceExt, FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(DeviceExt, FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    StdInfo = GetStandardInformationFromRecord(DeviceExt, FileRecord);
    ASSERT(StdInfo != NULL);

    Length = FileName->NameLength * sizeof (WCHAR);
    if ((sizeof(FILE_FULL_DIRECTORY_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_FULL_DIRECTORY_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, FileName->Name, Length);

    Info->CreationTime.QuadPart = FileName->CreationTime;
    Info->LastAccessTime.QuadPart = FileName->LastAccessTime;
    Info->LastWriteTime.QuadPart = FileName->LastWriteTime;
    Info->ChangeTime.QuadPart = FileName->ChangeTime;

    /* Convert file flags */
    NtfsFileFlagsToAttributes(FileName->FileAttributes | StdInfo->FileAttribute, &Info->FileAttributes);

    Info->EndOfFile.QuadPart = NtfsGetFileSize(DeviceExt, FileRecord, L"", 0, (PULONGLONG)&Info->AllocationSize.QuadPart);

    Info->FileIndex = MFTIndex;
    Info->EaSize = 0;

    return STATUS_SUCCESS;
}


static NTSTATUS
NtfsGetBothDirectoryInformation(PDEVICE_EXTENSION DeviceExt,
                                PFILE_RECORD_HEADER FileRecord,
                                ULONGLONG MFTIndex,
                                PFILE_BOTH_DIR_INFORMATION Info,
                                ULONG BufferLength)
{
    ULONG Length;
    PFILENAME_ATTRIBUTE FileName, ShortFileName;
    PSTANDARD_INFORMATION StdInfo;

    DPRINT("NtfsGetBothDirectoryInformation() called\n");

    FileName = GetBestFileNameFromRecord(DeviceExt, FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(DeviceExt, FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    ShortFileName = GetFileNameFromRecord(DeviceExt, FileRecord, NTFS_FILE_NAME_DOS);

    StdInfo = GetStandardInformationFromRecord(DeviceExt, FileRecord);
    ASSERT(StdInfo != NULL);

    Length = FileName->NameLength * sizeof (WCHAR);
    if ((sizeof(FILE_BOTH_DIR_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_BOTH_DIR_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, FileName->Name, Length);

    if (ShortFileName)
    {
        /* Should we upcase the filename? */
        ASSERT(ShortFileName->NameLength <= ARRAYSIZE(Info->ShortName));
        Info->ShortNameLength = ShortFileName->NameLength * sizeof(WCHAR);
        RtlCopyMemory(Info->ShortName, ShortFileName->Name, Info->ShortNameLength);
    }
    else
    {
        Info->ShortName[0] = 0;
        Info->ShortNameLength = 0;
    }

    Info->CreationTime.QuadPart = FileName->CreationTime;
    Info->LastAccessTime.QuadPart = FileName->LastAccessTime;
    Info->LastWriteTime.QuadPart = FileName->LastWriteTime;
    Info->ChangeTime.QuadPart = FileName->ChangeTime;

    /* Convert file flags */
    NtfsFileFlagsToAttributes(FileName->FileAttributes | StdInfo->FileAttribute, &Info->FileAttributes);

    Info->EndOfFile.QuadPart = NtfsGetFileSize(DeviceExt, FileRecord, L"", 0, (PULONGLONG)&Info->AllocationSize.QuadPart);

    Info->FileIndex = MFTIndex;
    Info->EaSize = 0;

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsQueryDirectory(PNTFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;
    LONG BufferLength = 0;
    PUNICODE_STRING SearchPattern = NULL;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex = 0;
    PUCHAR Buffer = NULL;
    PFILE_NAMES_INFORMATION Buffer0 = NULL;
    PNTFS_FCB Fcb;
    PNTFS_CCB Ccb;
    BOOLEAN First = FALSE;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_RECORD_HEADER FileRecord;
    ULONGLONG MFTRecord, OldMFTRecord = 0;
    UNICODE_STRING Pattern;

    DPRINT1("NtfsQueryDirectory() called\n");

    ASSERT(IrpContext);
    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;

    Ccb = (PNTFS_CCB)FileObject->FsContext2;
    Fcb = (PNTFS_FCB)FileObject->FsContext;

    /* Obtain the callers parameters */
    BufferLength = Stack->Parameters.QueryDirectory.Length;
    SearchPattern = Stack->Parameters.QueryDirectory.FileName;
    FileInformationClass = Stack->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = Stack->Parameters.QueryDirectory.FileIndex;

    if (NtfsFCBIsCompressed(Fcb))
    {
        DPRINT1("Compressed directory!\n");
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!ExAcquireResourceSharedLite(&Fcb->MainResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return STATUS_PENDING;
    }

    if (SearchPattern != NULL)
    {
        if (!Ccb->DirectorySearchPattern)
        {
            First = TRUE;
            Pattern.Length = 0;
            Pattern.MaximumLength = SearchPattern->Length + sizeof(WCHAR);
            Ccb->DirectorySearchPattern = Pattern.Buffer =
                ExAllocatePoolWithTag(NonPagedPool, Pattern.MaximumLength, TAG_NTFS);
            if (!Ccb->DirectorySearchPattern)
            {
                ExReleaseResourceLite(&Fcb->MainResource);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            memcpy(Ccb->DirectorySearchPattern, SearchPattern->Buffer, SearchPattern->Length);
            Ccb->DirectorySearchPattern[SearchPattern->Length / sizeof(WCHAR)] = 0;
        }
    }
    else if (!Ccb->DirectorySearchPattern)
    {
        First = TRUE;
        Ccb->DirectorySearchPattern = ExAllocatePoolWithTag(NonPagedPool, 2 * sizeof(WCHAR), TAG_NTFS);
        if (!Ccb->DirectorySearchPattern)
        {
            ExReleaseResourceLite(&Fcb->MainResource);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Ccb->DirectorySearchPattern[0] = L'*';
        Ccb->DirectorySearchPattern[1] = 0;
    }

    RtlInitUnicodeString(&Pattern, Ccb->DirectorySearchPattern);
    DPRINT("Search pattern '%S'\n", Ccb->DirectorySearchPattern);
    DPRINT("In: '%S'\n", Fcb->PathName);

    /* Determine directory index */
    if (Stack->Flags & SL_INDEX_SPECIFIED)
    {
        Ccb->Entry = Ccb->CurrentByteOffset.u.LowPart;
    }
    else if (First || (Stack->Flags & SL_RESTART_SCAN))
    {
        Ccb->Entry = 0;
    }

    /* Get Buffer for result */
    Buffer = NtfsGetUserBuffer(Irp, FALSE);

    DPRINT("Buffer=%p tofind=%S\n", Buffer, Ccb->DirectorySearchPattern);

    if (!ExAcquireResourceExclusiveLite(&DeviceExtension->DirResource,
                                        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        ExReleaseResourceLite(&Fcb->MainResource);
        return STATUS_PENDING;
    }

    while (Status == STATUS_SUCCESS && BufferLength > 0)
    {
        Status = NtfsFindFileAt(DeviceExtension,
                                &Pattern,
                                &Ccb->Entry,
                                &FileRecord,
                                &MFTRecord,
                                Fcb->MFTIndex);

        if (NT_SUCCESS(Status))
        {
            /* HACK: files with both a short name and a long name are present twice in the index.
             * Ignore the second entry, if it is immediately following the first one.
             */
            if (MFTRecord == OldMFTRecord)
            {
                DPRINT1("Ignoring duplicate MFT entry 0x%x\n", MFTRecord);
                Ccb->Entry++;
                ExFreePoolWithTag(FileRecord, TAG_NTFS);
                continue;
            }
            OldMFTRecord = MFTRecord;

            switch (FileInformationClass)
            {
                case FileNameInformation:
                    Status = NtfsGetNameInformation(DeviceExtension,
                                                    FileRecord,
                                                    MFTRecord,
                                                    (PFILE_NAMES_INFORMATION)Buffer,
                                                    BufferLength);
                    break;

                case FileDirectoryInformation:
                    Status = NtfsGetDirectoryInformation(DeviceExtension,
                                                         FileRecord,
                                                         MFTRecord,
                                                         (PFILE_DIRECTORY_INFORMATION)Buffer,
                                                         BufferLength);
                    break;

                case FileFullDirectoryInformation:
                    Status = NtfsGetFullDirectoryInformation(DeviceExtension,
                                                             FileRecord,
                                                             MFTRecord,
                                                             (PFILE_FULL_DIRECTORY_INFORMATION)Buffer,
                                                             BufferLength);
                    break;

                case FileBothDirectoryInformation:
                    Status = NtfsGetBothDirectoryInformation(DeviceExtension,
                                                             FileRecord,
                                                             MFTRecord,
                                                             (PFILE_BOTH_DIR_INFORMATION)Buffer,
                                                             BufferLength);
                    break;

                default:
                    Status = STATUS_INVALID_INFO_CLASS;
            }

            if (Status == STATUS_BUFFER_OVERFLOW)
            {
                if (Buffer0)
                {
                    Buffer0->NextEntryOffset = 0;
                }
                break;
            }
        }
        else
        {
            if (Buffer0)
            {
                Buffer0->NextEntryOffset = 0;
            }

            if (First)
            {
                Status = STATUS_NO_SUCH_FILE;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
            break;
        }

        Buffer0 = (PFILE_NAMES_INFORMATION)Buffer;
        Buffer0->FileIndex = FileIndex++;
        Ccb->Entry++;

        if (Stack->Flags & SL_RETURN_SINGLE_ENTRY)
        {
            break;
        }
        BufferLength -= Buffer0->NextEntryOffset;
        Buffer += Buffer0->NextEntryOffset;
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
    }

    if (Buffer0)
    {
        Buffer0->NextEntryOffset = 0;
    }

    ExReleaseResourceLite(&DeviceExtension->DirResource);
    ExReleaseResourceLite(&Fcb->MainResource);

    if (FileIndex > 0)
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS
NtfsDirectoryControl(PNTFS_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DPRINT1("NtfsDirectoryControl() called\n");

    switch (IrpContext->MinorFunction)
    {
        case IRP_MN_QUERY_DIRECTORY:
            Status = NtfsQueryDirectory(IrpContext);
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            DPRINT1("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if (Status == STATUS_PENDING && IrpContext->Flags & IRPCONTEXT_COMPLETE)
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    IrpContext->Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
