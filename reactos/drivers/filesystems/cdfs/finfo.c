/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2004 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/finfo.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
* FUNCTION: Retrieve the standard file information
*/
static NTSTATUS
CdfsGetStandardInformation(PFCB Fcb,
                           PDEVICE_OBJECT DeviceObject,
                           PFILE_STANDARD_INFORMATION StandardInfo,
                           PULONG BufferLength)
{
    DPRINT("CdfsGetStandardInformation() called\n");

    UNREFERENCED_PARAMETER(DeviceObject);

    if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    /* PRECONDITION */
    ASSERT(StandardInfo != NULL);
    ASSERT(Fcb != NULL);

    RtlZeroMemory(StandardInfo,
        sizeof(FILE_STANDARD_INFORMATION));

    if (CdfsFCBIsDirectory(Fcb))
    {
        StandardInfo->AllocationSize.QuadPart = 0LL;
        StandardInfo->EndOfFile.QuadPart = 0LL;
        StandardInfo->Directory = TRUE;
    }
    else
    {
        StandardInfo->AllocationSize = Fcb->RFCB.AllocationSize;
        StandardInfo->EndOfFile = Fcb->RFCB.FileSize;
        StandardInfo->Directory = FALSE;
    }
    StandardInfo->NumberOfLinks = 0;
    StandardInfo->DeletePending = FALSE;

    *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Retrieve the file position information
*/
static NTSTATUS
CdfsGetPositionInformation(PFILE_OBJECT FileObject,
                           PFILE_POSITION_INFORMATION PositionInfo,
                           PULONG BufferLength)
{
    DPRINT("CdfsGetPositionInformation() called\n");

    if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    PositionInfo->CurrentByteOffset.QuadPart =
        FileObject->CurrentByteOffset.QuadPart;

    DPRINT("Getting position %I64x\n",
        PositionInfo->CurrentByteOffset.QuadPart);

    *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Retrieve the basic file information
*/
static NTSTATUS
CdfsGetBasicInformation(PFILE_OBJECT FileObject,
                        PFCB Fcb,
                        PDEVICE_OBJECT DeviceObject,
                        PFILE_BASIC_INFORMATION BasicInfo,
                        PULONG BufferLength)
{
    DPRINT("CdfsGetBasicInformation() called\n");

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(DeviceObject);

    if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    CdfsDateTimeToSystemTime(Fcb,
        &BasicInfo->CreationTime);
    CdfsDateTimeToSystemTime(Fcb,
        &BasicInfo->LastAccessTime);
    CdfsDateTimeToSystemTime(Fcb,
        &BasicInfo->LastWriteTime);
    CdfsDateTimeToSystemTime(Fcb,
        &BasicInfo->ChangeTime);

    CdfsFileFlagsToAttributes(Fcb,
        &BasicInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_BASIC_INFORMATION);

    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Retrieve the file name information
*/
static NTSTATUS
CdfsGetNameInformation(PFILE_OBJECT FileObject,
                       PFCB Fcb,
                       PDEVICE_OBJECT DeviceObject,
                       PFILE_NAME_INFORMATION NameInfo,
                       PULONG BufferLength)
{
    ULONG NameLength;
    ULONG BytesToCopy;

    DPRINT("CdfsGetNameInformation() called\n");

    ASSERT(NameInfo != NULL);
    ASSERT(Fcb != NULL);

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(DeviceObject);

    /* If buffer can't hold at least the file name length, bail out */
    if (*BufferLength < (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
        return STATUS_BUFFER_OVERFLOW;

    /* Calculate file name length in bytes */
    NameLength = Fcb->PathName.Length;
    NameInfo->FileNameLength = NameLength;

    /* Calculate amount of bytes to copy not to overflow the buffer */
    BytesToCopy = min(NameLength,
        *BufferLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

    /* Fill in the bytes */
    RtlCopyMemory(NameInfo->FileName, Fcb->PathName.Buffer, BytesToCopy);

    /* Check if we could write more but are not able to */
    if (*BufferLength < NameLength + FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    {
        /* Return number of bytes written */
        *BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* We filled up as many bytes, as needed */
    *BufferLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + NameLength);

    return STATUS_SUCCESS;
}


/*
* FUNCTION: Retrieve the internal file information
*/
static NTSTATUS
CdfsGetInternalInformation(PFCB Fcb,
                           PFILE_INTERNAL_INFORMATION InternalInfo,
                           PULONG BufferLength)
{
    DPRINT("CdfsGetInternalInformation() called\n");

    ASSERT(InternalInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
        return(STATUS_BUFFER_OVERFLOW);

    InternalInfo->IndexNumber.QuadPart = Fcb->IndexNumber.QuadPart;

    *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);

    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Retrieve the file network open information
*/
static NTSTATUS
CdfsGetNetworkOpenInformation(PFCB Fcb,
                              PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
                              PULONG BufferLength)
{
    ASSERT(NetworkInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
        return(STATUS_BUFFER_OVERFLOW);

    CdfsDateTimeToSystemTime(Fcb,
        &NetworkInfo->CreationTime);
    CdfsDateTimeToSystemTime(Fcb,
        &NetworkInfo->LastAccessTime);
    CdfsDateTimeToSystemTime(Fcb,
        &NetworkInfo->LastWriteTime);
    CdfsDateTimeToSystemTime(Fcb,
        &NetworkInfo->ChangeTime);
    if (CdfsFCBIsDirectory(Fcb))
    {
        NetworkInfo->AllocationSize.QuadPart = 0LL;
        NetworkInfo->EndOfFile.QuadPart = 0LL;
    }
    else
    {
        NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;
        NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    }
    CdfsFileFlagsToAttributes(Fcb,
        &NetworkInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);

    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Retrieve all file information
*/
static NTSTATUS
CdfsGetAllInformation(PFILE_OBJECT FileObject,
                      PFCB Fcb,
                      PFILE_ALL_INFORMATION Info,
                      PULONG BufferLength)
{
    ULONG NameLength;

    ASSERT(Info);
    ASSERT(Fcb);

    NameLength = Fcb->PathName.Length;
    if (*BufferLength < sizeof(FILE_ALL_INFORMATION) + NameLength)
        return(STATUS_BUFFER_OVERFLOW);

    /* Basic Information */
    CdfsDateTimeToSystemTime(Fcb,
        &Info->BasicInformation.CreationTime);
    CdfsDateTimeToSystemTime(Fcb,
        &Info->BasicInformation.LastAccessTime);
    CdfsDateTimeToSystemTime(Fcb,
        &Info->BasicInformation.LastWriteTime);
    CdfsDateTimeToSystemTime(Fcb,
        &Info->BasicInformation.ChangeTime);
    CdfsFileFlagsToAttributes(Fcb,
        &Info->BasicInformation.FileAttributes);

    /* Standard Information */
    if (CdfsFCBIsDirectory(Fcb))
    {
        Info->StandardInformation.AllocationSize.QuadPart = 0LL;
        Info->StandardInformation.EndOfFile.QuadPart = 0LL;
        Info->StandardInformation.Directory = TRUE;
    }
    else
    {
        Info->StandardInformation.AllocationSize = Fcb->RFCB.AllocationSize;
        Info->StandardInformation.EndOfFile = Fcb->RFCB.FileSize;
        Info->StandardInformation.Directory = FALSE;
    }
    Info->StandardInformation.NumberOfLinks = 0;
    Info->StandardInformation.DeletePending = FALSE;

    /* Internal Information */
    Info->InternalInformation.IndexNumber.QuadPart = Fcb->IndexNumber.QuadPart;

    /* EA Information */
    Info->EaInformation.EaSize = 0;

    /* Access Information */
    /* The IO-Manager adds this information */

    /* Position Information */
    Info->PositionInformation.CurrentByteOffset.QuadPart = FileObject->CurrentByteOffset.QuadPart;

    /* Mode Information */
    /* The IO-Manager adds this information */

    /* Alignment Information */
    /* The IO-Manager adds this information */

    /* Name Information */
    Info->NameInformation.FileNameLength = NameLength;
    RtlCopyMemory(Info->NameInformation.FileName,
        Fcb->PathName.Buffer,
        NameLength + sizeof(WCHAR));

    *BufferLength -= (sizeof(FILE_ALL_INFORMATION) + NameLength + sizeof(WCHAR));

    return STATUS_SUCCESS;
}


/*
* FUNCTION: Retrieve the specified file information
*/
NTSTATUS NTAPI
CdfsQueryInformation(
    PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    FILE_INFORMATION_CLASS FileInformationClass;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PFCB Fcb;
    PVOID SystemBuffer;
    ULONG BufferLength;

    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CdfsQueryInformation() called\n");

    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;
    Stack = IrpContext->Stack;
    FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
    FileObject = Stack->FileObject;
    Fcb = FileObject->FsContext;

    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = Stack->Parameters.QueryFile.Length;

    switch (FileInformationClass)
    {
    case FileStandardInformation:
        Status = CdfsGetStandardInformation(Fcb,
            DeviceObject,
            SystemBuffer,
            &BufferLength);
        break;

    case FilePositionInformation:
        Status = CdfsGetPositionInformation(FileObject,
            SystemBuffer,
            &BufferLength);
        break;

    case FileBasicInformation:
        Status = CdfsGetBasicInformation(FileObject,
            Fcb,
            DeviceObject,
            SystemBuffer,
            &BufferLength);
        break;

    case FileNameInformation:
        Status = CdfsGetNameInformation(FileObject,
            Fcb,
            DeviceObject,
            SystemBuffer,
            &BufferLength);
        break;

    case FileInternalInformation:
        Status = CdfsGetInternalInformation(Fcb,
            SystemBuffer,
            &BufferLength);
        break;

    case FileNetworkOpenInformation:
        Status = CdfsGetNetworkOpenInformation(Fcb,
            SystemBuffer,
            &BufferLength);
        break;

    case FileAllInformation:
        Status = CdfsGetAllInformation(FileObject,
            Fcb,
            SystemBuffer,
            &BufferLength);
        break;

    case FileAlternateNameInformation:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        DPRINT("Unimplemented information class %x\n", FileInformationClass);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    Irp->IoStatus.Status = Status;
    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
        Irp->IoStatus.Information =
        Stack->Parameters.QueryFile.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return(Status);
}


/*
* FUNCTION: Set the file position information
*/
static NTSTATUS
CdfsSetPositionInformation(PFILE_OBJECT FileObject,
                           PFILE_POSITION_INFORMATION PositionInfo)
{
    DPRINT ("CdfsSetPositionInformation()\n");

    DPRINT ("PositionInfo %p\n", PositionInfo);
    DPRINT ("Setting position %I64d\n", PositionInfo->CurrentByteOffset.QuadPart);

    FileObject->CurrentByteOffset.QuadPart =
        PositionInfo->CurrentByteOffset.QuadPart;

    return STATUS_SUCCESS;
}


/*
* FUNCTION: Set the specified file information
*/
NTSTATUS NTAPI
CdfsSetInformation(
    PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    FILE_INFORMATION_CLASS FileInformationClass;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PVOID SystemBuffer;

    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CdfsSetInformation() called\n");

    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    FileInformationClass = Stack->Parameters.SetFile.FileInformationClass;
    FileObject = Stack->FileObject;

    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

    switch (FileInformationClass)
    {
    case FilePositionInformation:
        Status = CdfsSetPositionInformation(FileObject,
            SystemBuffer);
        break;

    case FileBasicInformation:
    case FileRenameInformation:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
