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

    FileName = GetBestFileNameFromRecord(FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(FileRecord);
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

    FileName = GetBestFileNameFromRecord(FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    StdInfo = GetStandardInformationFromRecord(FileRecord);
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

    Info->EndOfFile.QuadPart = FileName->AllocatedSize;
    Info->AllocationSize.QuadPart = ROUND_UP(FileName->AllocatedSize, DeviceExt->NtfsInfo.BytesPerCluster);

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

    FileName = GetBestFileNameFromRecord(FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    StdInfo = GetStandardInformationFromRecord(FileRecord);
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

    Info->EndOfFile.QuadPart = FileName->AllocatedSize;
    Info->AllocationSize.QuadPart = ROUND_UP(FileName->AllocatedSize, DeviceExt->NtfsInfo.BytesPerCluster);

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

    FileName = GetBestFileNameFromRecord(FileRecord);
    if (FileName == NULL)
    {
        DPRINT1("No name information for file ID: %#I64x\n", MFTIndex);
        NtfsDumpFileAttributes(FileRecord);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    ShortFileName = GetFileNameFromRecord(FileRecord, NTFS_FILE_NAME_DOS);

    StdInfo = GetStandardInformationFromRecord(FileRecord);
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

    Info->EndOfFile.QuadPart = FileName->AllocatedSize;
    Info->AllocationSize.QuadPart = ROUND_UP(FileName->AllocatedSize, DeviceExt->NtfsInfo.BytesPerCluster);

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
                DPRINT("Ignoring duplicate MFT entry 0x%x\n", MFTRecord);
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

    IrpContext->Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
