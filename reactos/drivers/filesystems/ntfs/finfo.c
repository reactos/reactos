/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *                   Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Retrieve the standard file information
 */
static
NTSTATUS
NtfsGetStandardInformation(PNTFS_FCB Fcb,
                           PDEVICE_OBJECT DeviceObject,
                           PFILE_STANDARD_INFORMATION StandardInfo,
                           PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT1("NtfsGetStandardInformation(%p, %p, %p, %p)\n", Fcb, DeviceObject, StandardInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    /* PRECONDITION */
    ASSERT(StandardInfo != NULL);
    ASSERT(Fcb != NULL);

    RtlZeroMemory(StandardInfo,
                  sizeof(FILE_STANDARD_INFORMATION));

    StandardInfo->AllocationSize = Fcb->RFCB.AllocationSize;
    StandardInfo->EndOfFile = Fcb->RFCB.FileSize;
    StandardInfo->NumberOfLinks = Fcb->LinkCount;
    StandardInfo->DeletePending = FALSE;
    StandardInfo->Directory = NtfsFCBIsDirectory(Fcb);

    *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetPositionInformation(PFILE_OBJECT FileObject,
                           PFILE_POSITION_INFORMATION PositionInfo,
                           PULONG BufferLength)
{
    DPRINT1("NtfsGetPositionInformation(%p, %p, %p)\n", FileObject, PositionInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    PositionInfo->CurrentByteOffset.QuadPart = FileObject->CurrentByteOffset.QuadPart;

    DPRINT("Getting position %I64x\n",
           PositionInfo->CurrentByteOffset.QuadPart);

    *BufferLength -= sizeof(FILE_POSITION_INFORMATION);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetBasicInformation(PFILE_OBJECT FileObject,
                        PNTFS_FCB Fcb,
                        PDEVICE_OBJECT DeviceObject,
                        PFILE_BASIC_INFORMATION BasicInfo,
                        PULONG BufferLength)
{
    PFILENAME_ATTRIBUTE FileName = &Fcb->Entry;

    DPRINT1("NtfsGetBasicInformation(%p, %p, %p, %p, %p)\n", FileObject, Fcb, DeviceObject, BasicInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    BasicInfo->CreationTime.QuadPart = FileName->CreationTime;
    BasicInfo->LastAccessTime.QuadPart = FileName->LastAccessTime;
    BasicInfo->LastWriteTime.QuadPart = FileName->LastWriteTime;
    BasicInfo->ChangeTime.QuadPart = FileName->ChangeTime;

    NtfsFileFlagsToAttributes(FileName->FileAttributes, &BasicInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_BASIC_INFORMATION);

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the file name information
 */
static
NTSTATUS
NtfsGetNameInformation(PFILE_OBJECT FileObject,
                       PNTFS_FCB Fcb,
                       PDEVICE_OBJECT DeviceObject,
                       PFILE_NAME_INFORMATION NameInfo,
                       PULONG BufferLength)
{
    ULONG BytesToCopy;

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT1("NtfsGetNameInformation(%p, %p, %p, %p, %p)\n", FileObject, Fcb, DeviceObject, NameInfo, BufferLength);

    ASSERT(NameInfo != NULL);
    ASSERT(Fcb != NULL);

    /* If buffer can't hold at least the file name length, bail out */
    if (*BufferLength < (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
        return STATUS_BUFFER_OVERFLOW;

    /* Save file name length, and as much file len, as buffer length allows */
    NameInfo->FileNameLength = wcslen(Fcb->PathName) * sizeof(WCHAR);

    /* Calculate amount of bytes to copy not to overflow the buffer */
    BytesToCopy = min(NameInfo->FileNameLength,
                      *BufferLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

    /* Fill in the bytes */
    RtlCopyMemory(NameInfo->FileName, Fcb->PathName, BytesToCopy);

    /* Check if we could write more but are not able to */
    if (*BufferLength < NameInfo->FileNameLength + (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    {
        /* Return number of bytes written */
        *BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* We filled up as many bytes, as needed */
    *BufferLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + NameInfo->FileNameLength);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetInternalInformation(PNTFS_FCB Fcb,
                           PFILE_INTERNAL_INFORMATION InternalInfo,
                           PULONG BufferLength)
{
    DPRINT1("NtfsGetInternalInformation(%p, %p, %p)\n", Fcb, InternalInfo, BufferLength);

    ASSERT(InternalInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    InternalInfo->IndexNumber.QuadPart = Fcb->MFTIndex;

    *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsGetNetworkOpenInformation(PNTFS_FCB Fcb,
                              PDEVICE_EXTENSION DeviceExt,
                              PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
                              PULONG BufferLength)
{
    PFILENAME_ATTRIBUTE FileName = &Fcb->Entry;

    DPRINT1("NtfsGetNetworkOpenInformation(%p, %p, %p, %p)\n", Fcb, DeviceExt, NetworkInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
        return(STATUS_BUFFER_OVERFLOW);

    NetworkInfo->CreationTime.QuadPart = FileName->CreationTime;
    NetworkInfo->LastAccessTime.QuadPart = FileName->LastAccessTime;
    NetworkInfo->LastWriteTime.QuadPart = FileName->LastWriteTime;
    NetworkInfo->ChangeTime.QuadPart = FileName->ChangeTime;

    NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;

    NtfsFileFlagsToAttributes(FileName->FileAttributes, &NetworkInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsGetSteamInformation(PNTFS_FCB Fcb,
                        PDEVICE_EXTENSION DeviceExt,
                        PFILE_STREAM_INFORMATION StreamInfo,
                        PULONG BufferLength)
{
    NTSTATUS Status;
    ULONG CurrentSize;
    PNTFS_ATTR_RECORD Attribute;
    PFILE_RECORD_HEADER FileRecord;
    PFILE_STREAM_INFORMATION CurrentInfo = StreamInfo, Previous = NULL;

    if (*BufferLength < sizeof(FILE_STREAM_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    FileRecord = ExAllocatePoolWithTag(NonPagedPool, DeviceExt->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (FileRecord == NULL)
    {
        DPRINT1("Not enough memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't find record!\n");
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        return Status;
    }

    Status = STATUS_SUCCESS;
    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
           Attribute->Type != AttributeEnd)
    {
        if (Attribute->Type == AttributeData)
        {
            CurrentSize = FIELD_OFFSET(FILE_STREAM_INFORMATION, StreamName) + Attribute->NameLength * sizeof(WCHAR);

            if (CurrentSize > *BufferLength)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            CurrentInfo->NextEntryOffset = 0;
            CurrentInfo->StreamNameLength = Attribute->NameLength * sizeof(WCHAR);
            CurrentInfo->StreamSize.QuadPart = AttributeDataLength(Attribute);
            CurrentInfo->StreamAllocationSize.QuadPart = AttributeAllocatedLength(Attribute);
            RtlMoveMemory(CurrentInfo->StreamName, (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset), CurrentInfo->StreamNameLength);

            if (Previous != NULL)
            {
                Previous->NextEntryOffset = (ULONG_PTR)CurrentInfo - (ULONG_PTR)Previous;
            }
            Previous = CurrentInfo;
            CurrentInfo = (PFILE_STREAM_INFORMATION)((ULONG_PTR)CurrentInfo + CurrentSize);
            *BufferLength -= CurrentSize;
        }

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    ExFreePoolWithTag(FileRecord, TAG_NTFS);
    return Status;
}

/*
 * FUNCTION: Retrieve the specified file information
 */
NTSTATUS
NtfsQueryInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PNTFS_FCB Fcb;
    PVOID SystemBuffer;
    ULONG BufferLength;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT1("NtfsQueryInformation(%p)\n", IrpContext);

    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    DeviceObject = IrpContext->DeviceObject;
    FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
    FileObject = IrpContext->FileObject;
    Fcb = FileObject->FsContext;

    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = Stack->Parameters.QueryFile.Length;

    if (!ExAcquireResourceSharedLite(&Fcb->MainResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    switch (FileInformationClass)
    {
        case FileStandardInformation:
            Status = NtfsGetStandardInformation(Fcb,
                                                DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FilePositionInformation:
            Status = NtfsGetPositionInformation(FileObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileBasicInformation:
            Status = NtfsGetBasicInformation(FileObject,
                                             Fcb,
                                             DeviceObject,
                                             SystemBuffer,
                                             &BufferLength);
            break;

        case FileNameInformation:
            Status = NtfsGetNameInformation(FileObject,
                                            Fcb,
                                            DeviceObject,
                                            SystemBuffer,
                                            &BufferLength);
            break;

        case FileInternalInformation:
            Status = NtfsGetInternalInformation(Fcb,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileNetworkOpenInformation:
            Status = NtfsGetNetworkOpenInformation(Fcb,
                                                   DeviceObject->DeviceExtension,
                                                   SystemBuffer,
                                                   &BufferLength);
            break;

        case FileStreamInformation:
            Status = NtfsGetSteamInformation(Fcb,
                                             DeviceObject->DeviceExtension,
                                             SystemBuffer,
                                             &BufferLength);
            break;

        case FileAlternateNameInformation:
        case FileAllInformation:
            DPRINT1("Unimplemented information class %u\n", FileInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("Unimplemented information class %u\n", FileInformationClass);
            Status = STATUS_INVALID_PARAMETER;
    }

    ExReleaseResourceLite(&Fcb->MainResource);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information =
            Stack->Parameters.QueryFile.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
