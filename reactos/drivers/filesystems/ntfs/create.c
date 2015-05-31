/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2014 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/create.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

static PCWSTR MftIdToName[] = {
    L"$MFT",
    L"$MFTMirr",
    L"$LogFile",
    L"$Volume",
    L"AttrDef",
    L".",
    L"$Bitmap",
    L"$Boot",
    L"$BadClus",
    L"$Quota",
    L"$UpCase",
    L"$Extended",
};

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
NtfsMakeAbsoluteFilename(PFILE_OBJECT pFileObject,
                         PWSTR pRelativeFileName,
                         PWSTR *pAbsoluteFilename)
{
    PWSTR rcName;
    PNTFS_FCB Fcb;

    DPRINT("try related for %S\n", pRelativeFileName);
    Fcb = pFileObject->FsContext;
    ASSERT(Fcb);

    if (Fcb->Flags & FCB_IS_VOLUME)
    {
        /* This is likely to be an opening by ID, return ourselves */
        if (pRelativeFileName[0] == L'\\')
        {
            *pAbsoluteFilename = NULL;
            return STATUS_SUCCESS;
        }

        return STATUS_INVALID_PARAMETER;
    }

    /* verify related object is a directory and target name
       don't start with \. */
    if (NtfsFCBIsDirectory(Fcb) == FALSE ||
        pRelativeFileName[0] == L'\\')
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* construct absolute path name */
    ASSERT(wcslen (Fcb->PathName) + 1 + wcslen (pRelativeFileName) + 1 <= MAX_PATH);
    rcName = ExAllocatePoolWithTag(NonPagedPool, MAX_PATH * sizeof(WCHAR), TAG_NTFS);
    if (!rcName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wcscpy(rcName, Fcb->PathName);
    if (!NtfsFCBIsRoot(Fcb))
        wcscat (rcName, L"\\");
    wcscat (rcName, pRelativeFileName);
    *pAbsoluteFilename = rcName;

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsMoonWalkID(PDEVICE_EXTENSION DeviceExt,
               ULONGLONG Id,
               PUNICODE_STRING OutPath)
{
    NTSTATUS Status;
    PFILE_RECORD_HEADER MftRecord;
    PFILENAME_ATTRIBUTE FileName;
    WCHAR FullPath[MAX_PATH];
    ULONG WritePosition = MAX_PATH - 1;

    DPRINT1("NtfsMoonWalkID(%p, %I64x, %p)\n", DeviceExt, Id, OutPath);

    RtlZeroMemory(FullPath, sizeof(FullPath));
    MftRecord = ExAllocatePoolWithTag(NonPagedPool,
                                      DeviceExt->NtfsInfo.BytesPerFileRecord,
                                      TAG_NTFS);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (TRUE)
    {
        Status = ReadFileRecord(DeviceExt, Id, MftRecord);
        if (!NT_SUCCESS(Status))
            break;

        ASSERT(MftRecord->Ntfs.Type == NRH_FILE_TYPE);
        if (!(MftRecord->Flags & FRH_IN_USE))
        {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            break;
        }

        FileName = GetBestFileNameFromRecord(MftRecord);
        if (FileName == NULL)
        {
            DPRINT1("$FILE_NAME attribute not found for %I64x\n", Id);
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            break;
        }

        WritePosition -= FileName->NameLength;
        ASSERT(WritePosition < MAX_PATH);
        RtlCopyMemory(FullPath + WritePosition, FileName->Name, FileName->NameLength * sizeof(WCHAR));
        WritePosition -= 1;
        ASSERT(WritePosition < MAX_PATH);
        FullPath[WritePosition] = L'\\';

        Id = FileName->DirectoryFileReferenceNumber & NTFS_MFT_MASK;
        if (Id == NTFS_FILE_ROOT)
            break;
    }

    ExFreePoolWithTag(MftRecord, TAG_NTFS);

    if (!NT_SUCCESS(Status))
        return Status;

    OutPath->Length = (MAX_PATH - WritePosition - 1) * sizeof(WCHAR);
    OutPath->MaximumLength = (MAX_PATH - WritePosition) * sizeof(WCHAR);
    OutPath->Buffer = ExAllocatePoolWithTag(NonPagedPool, OutPath->MaximumLength, TAG_NTFS);
    if (OutPath->Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(OutPath->Buffer, FullPath + WritePosition, OutPath->MaximumLength);

    return Status;
}

static
NTSTATUS
NtfsOpenFileById(PDEVICE_EXTENSION DeviceExt,
                 PFILE_OBJECT FileObject,
                 ULONGLONG MftId,
                 PNTFS_FCB * FoundFCB)
{
    NTSTATUS Status;
    PNTFS_FCB FCB;
    PFILE_RECORD_HEADER MftRecord;

    DPRINT1("NtfsOpenFileById(%p, %p, %I64x, %p)\n", DeviceExt, FileObject, MftId, FoundFCB);

    ASSERT(MftId < 0x10);
    if (MftId > 0xb) /* No entries are used yet beyond this */
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    MftRecord = ExAllocatePoolWithTag(NonPagedPool,
                                      DeviceExt->NtfsInfo.BytesPerFileRecord,
                                      TAG_NTFS);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, MftId, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return Status;
    }

    if (!(MftRecord->Flags & FRH_IN_USE))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    FCB = NtfsGrabFCBFromTable(DeviceExt, MftIdToName[MftId]);
    if (FCB == NULL)
    {
        UNICODE_STRING Name;

        RtlInitUnicodeString(&Name, MftIdToName[MftId]);
        Status = NtfsMakeFCBFromDirEntry(DeviceExt, NULL, &Name, MftRecord, MftId, &FCB);
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(MftRecord, TAG_NTFS);
            return Status;
        }
    }

    ASSERT(FCB != NULL);

    ExFreePoolWithTag(MftRecord, TAG_NTFS);

    Status = NtfsAttachFCBToFileObject(DeviceExt,
                                       FCB,
                                       FileObject);
    *FoundFCB = FCB;

    return Status;
}

/*
 * FUNCTION: Opens a file
 */
static
NTSTATUS
NtfsOpenFile(PDEVICE_EXTENSION DeviceExt,
             PFILE_OBJECT FileObject,
             PWSTR FileName,
             PNTFS_FCB * FoundFCB)
{
    PNTFS_FCB ParentFcb;
    PNTFS_FCB Fcb;
    NTSTATUS Status;
    PWSTR AbsFileName = NULL;

    DPRINT1("NtfsOpenFile(%p, %p, %S, %p)\n", DeviceExt, FileObject, FileName, FoundFCB);

    *FoundFCB = NULL;

    if (FileObject->RelatedFileObject)
    {
        DPRINT("Converting relative filename to absolute filename\n");

        Status = NtfsMakeAbsoluteFilename(FileObject->RelatedFileObject,
                                          FileName,
                                          &AbsFileName);
        if (AbsFileName) FileName = AbsFileName;
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    //FIXME: Get cannonical path name (remove .'s, ..'s and extra separators)

    DPRINT("PathName to open: %S\n", FileName);

    /*  try first to find an existing FCB in memory  */
    DPRINT("Checking for existing FCB in memory\n");
    Fcb = NtfsGrabFCBFromTable(DeviceExt,
                               FileName);
    if (Fcb == NULL)
    {
        DPRINT("No existing FCB found, making a new one if file exists.\n");
        Status = NtfsGetFCBForFile(DeviceExt,
                                   &ParentFcb,
                                   &Fcb,
                                   FileName);
        if (ParentFcb != NULL)
        {
            NtfsReleaseFCB(DeviceExt,
                           ParentFcb);
        }

        if (!NT_SUCCESS (Status))
        {
            DPRINT("Could not make a new FCB, status: %x\n", Status);

            if (AbsFileName)
                ExFreePool(AbsFileName);

            return Status;
        }
    }

    DPRINT("Attaching FCB to fileObject\n");
    Status = NtfsAttachFCBToFileObject(DeviceExt,
                                       Fcb,
                                       FileObject);

    if (AbsFileName)
        ExFreePool(AbsFileName);

    *FoundFCB = Fcb;

    return Status;
}


/*
 * FUNCTION: Opens a file
 */
static
NTSTATUS
NtfsCreateFile(PDEVICE_OBJECT DeviceObject,
               PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    ULONG RequestedDisposition;
    ULONG RequestedOptions;
    PNTFS_FCB Fcb = NULL;
//    PWSTR FileName;
    NTSTATUS Status;
    UNICODE_STRING FullPath;

    DPRINT1("NtfsCreateFile(%p, %p) called\n", DeviceObject, Irp);

    DeviceExt = DeviceObject->DeviceExtension;
    ASSERT(DeviceExt);
    Stack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT(Stack);

    RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
    RequestedOptions = Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
//  PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
    if (RequestedOptions & FILE_DIRECTORY_FILE &&
        RequestedDisposition == FILE_SUPERSEDE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FileObject = Stack->FileObject;

    if (RequestedDisposition == FILE_CREATE ||
        RequestedDisposition == FILE_OVERWRITE_IF ||
        RequestedDisposition == FILE_SUPERSEDE)
    {
        return STATUS_ACCESS_DENIED;
    }

    if ((RequestedOptions & FILE_OPEN_BY_FILE_ID) == FILE_OPEN_BY_FILE_ID)
    {
        ULONGLONG MFTId;

        if (FileObject->FileName.Length != sizeof(ULONGLONG))
            return STATUS_INVALID_PARAMETER;

        MFTId = (*(PULONGLONG)FileObject->FileName.Buffer) & NTFS_MFT_MASK;
        if (MFTId < 0x10)
        {
            Status = NtfsOpenFileById(DeviceExt, FileObject, MFTId, &Fcb);
        }
        else
        {
            Status = NtfsMoonWalkID(DeviceExt, MFTId, &FullPath);
        }

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        DPRINT1("Open by ID: %I64x -> %wZ\n", (*(PULONGLONG)FileObject->FileName.Buffer) & NTFS_MFT_MASK, &FullPath);
    }

    /* This a open operation for the volume itself */
    if (FileObject->FileName.Length == 0 &&
        (FileObject->RelatedFileObject == NULL || FileObject->RelatedFileObject->FsContext2 != NULL))
    {
        if (RequestedDisposition != FILE_OPEN &&
            RequestedDisposition != FILE_OPEN_IF)
        {
            return STATUS_ACCESS_DENIED;
        }

        if (RequestedOptions & FILE_DIRECTORY_FILE)
        {
            return STATUS_NOT_A_DIRECTORY;
        }

        NtfsAttachFCBToFileObject(DeviceExt, DeviceExt->VolumeFcb, FileObject);
        DeviceExt->VolumeFcb->RefCount++;

        Irp->IoStatus.Information = FILE_OPENED;
        return STATUS_SUCCESS;
    }

    if (Fcb == NULL)
    {
        Status = NtfsOpenFile(DeviceExt,
                              FileObject,
                              ((RequestedOptions & FILE_OPEN_BY_FILE_ID) ? FullPath.Buffer : FileObject->FileName.Buffer),
                              &Fcb);

        if (RequestedOptions & FILE_OPEN_BY_FILE_ID)
        {
            ExFreePoolWithTag(FullPath.Buffer, TAG_NTFS);
        }
    }

    if (NT_SUCCESS(Status))
    {
        if (RequestedDisposition == FILE_CREATE)
        {
            Irp->IoStatus.Information = FILE_EXISTS;
            NtfsCloseFile(DeviceExt, FileObject);
            return STATUS_OBJECT_NAME_COLLISION;
        }

        if (RequestedOptions & FILE_NON_DIRECTORY_FILE &&
            NtfsFCBIsDirectory(Fcb))
        {
            NtfsCloseFile(DeviceExt, FileObject);
            return STATUS_FILE_IS_A_DIRECTORY;
        }

        if (RequestedOptions & FILE_DIRECTORY_FILE &&
            !NtfsFCBIsDirectory(Fcb))
        {
            NtfsCloseFile(DeviceExt, FileObject);
            return STATUS_NOT_A_DIRECTORY;
        }

        /*
         * If it is a reparse point & FILE_OPEN_REPARSE_POINT, then allow opening it
         * as a normal file.
         * Otherwise, attempt to read reparse data and hand them to the Io manager
         * with status reparse to force a reparse.
         */
        if (NtfsFCBIsReparsePoint(Fcb) &&
            ((RequestedOptions & FILE_OPEN_REPARSE_POINT) != FILE_OPEN_REPARSE_POINT))
        {
            PREPARSE_DATA_BUFFER ReparseData = NULL;

            Status = NtfsReadFCBAttribute(DeviceExt, Fcb,
                                          AttributeReparsePoint, L"", 0,
                                          (PVOID *)&Irp->Tail.Overlay.AuxiliaryBuffer);
            if (NT_SUCCESS(Status))
            {
                ReparseData = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;
                if (ReparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                {
                    Status = STATUS_REPARSE;
                }
                else
                {
                    Status = STATUS_NOT_IMPLEMENTED;
                    ExFreePoolWithTag(ReparseData, TAG_NTFS);
                }
            }

            Irp->IoStatus.Information = ((Status == STATUS_REPARSE) ? ReparseData->ReparseTag : 0);

            NtfsCloseFile(DeviceExt, FileObject);
            return Status;
        }

        /* HUGLY HACK: remain RO so far... */
        if (RequestedDisposition == FILE_OVERWRITE ||
            RequestedDisposition == FILE_OVERWRITE_IF ||
            RequestedDisposition == FILE_SUPERSEDE)
        {
            DPRINT1("Denying write request on NTFS volume\n");
            NtfsCloseFile(DeviceExt, FileObject);
            return STATUS_ACCESS_DENIED;
        }
    }
    else
    {
        /* HUGLY HACK: remain RO so far... */
        if (RequestedDisposition == FILE_CREATE ||
            RequestedDisposition == FILE_OPEN_IF ||
            RequestedDisposition == FILE_OVERWRITE_IF ||
            RequestedDisposition == FILE_SUPERSEDE)
        {
            DPRINT1("Denying write request on NTFS volume\n");
            return STATUS_ACCESS_DENIED;
        }
    }

    /*
     * If the directory containing the file to open doesn't exist then
     * fail immediately
     */
    Irp->IoStatus.Information = (NT_SUCCESS(Status)) ? FILE_OPENED : 0;

    return Status;
}


NTSTATUS
NtfsCreate(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    DeviceObject = IrpContext->DeviceObject;
    if (DeviceObject == NtfsGlobalData->DeviceObject)
    {
        /* DeviceObject represents FileSystem instead of logical volume */
        DPRINT("Opening file system\n");
        IrpContext->Irp->IoStatus.Information = FILE_OPENED;
        return STATUS_SUCCESS;
    }

    DeviceExt = DeviceObject->DeviceExtension;

    if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    ExAcquireResourceExclusiveLite(&DeviceExt->DirResource,
                                   TRUE);
    Status = NtfsCreateFile(DeviceObject,
                            IrpContext->Irp);
    ExReleaseResourceLite(&DeviceExt->DirResource);

    return Status;
}

/* EOF */
