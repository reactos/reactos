/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/filesystems/ntfs/create.c
 * PURPOSE:     NTFS filesystem driver
 * COPYRIGHT:   Copyright (C) 2002, 2014 ReactOS Team
 *              See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Eric Kohl
 *              Pierre Schweitzer (pierre@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
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
    L"$Secure",
    L"$UpCase",
    L"$Extend",
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

    Fcb = pFileObject->FsContext;
    ASSERT(Fcb);

    if (pRelativeFileName == NULL)
        return STATUS_INVALID_PARAMETER;

    DPRINT("try related for %S\n", pRelativeFileName);

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

    DPRINT("NtfsMoonWalkID(%p, %I64x, %p)\n", DeviceExt, Id, OutPath);

    RtlZeroMemory(FullPath, sizeof(FullPath));
    MftRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
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

        FileName = GetBestFileNameFromRecord(DeviceExt, MftRecord);
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

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, MftRecord);

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

    DPRINT("NtfsOpenFileById(%p, %p, %I64x, %p)\n", DeviceExt, FileObject, MftId, FoundFCB);

    ASSERT(MftId < NTFS_FILE_FIRST_USER_FILE);
    if (MftId > 0xb) /* No entries are used yet beyond this */
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    MftRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, MftId, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, MftRecord);
        return Status;
    }

    if (!(MftRecord->Flags & FRH_IN_USE))
    {
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, MftRecord);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    FCB = NtfsGrabFCBFromTable(DeviceExt, MftIdToName[MftId]);
    if (FCB == NULL)
    {
        UNICODE_STRING Name;

        RtlInitUnicodeString(&Name, MftIdToName[MftId]);
        Status = NtfsMakeFCBFromDirEntry(DeviceExt, NULL, &Name, NULL, MftRecord, MftId, &FCB);
        if (!NT_SUCCESS(Status))
        {
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, MftRecord);
            return Status;
        }
    }

    ASSERT(FCB != NULL);

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, MftRecord);

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
             BOOLEAN CaseSensitive,
             PNTFS_FCB * FoundFCB)
{
    PNTFS_FCB ParentFcb;
    PNTFS_FCB Fcb;
    NTSTATUS Status;
    PWSTR AbsFileName = NULL;

    DPRINT("NtfsOpenFile(%p, %p, %S, %s, %p)\n",
            DeviceExt,
            FileObject,
            FileName,
            CaseSensitive ? "TRUE" : "FALSE",
            FoundFCB);

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

    //FIXME: Get canonical path name (remove .'s, ..'s and extra separators)

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
                                   FileName,
                                   CaseSensitive);
        if (ParentFcb != NULL)
        {
            NtfsReleaseFCB(DeviceExt,
                           ParentFcb);
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT("Open of '%S' failed with %lx\n", FileName, Status);

            if (AbsFileName)
                ExFreePoolWithTag(AbsFileName, TAG_NTFS);

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


/**
* @name NtfsGrantShareAccess
*
* Records a new open of Fcb against its share access, or fails it with a
* sharing violation.
*/
static
NTSTATUS
NtfsGrantShareAccess(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PNTFS_FCB Fcb,
    _In_ PFILE_OBJECT FileObject,
    _In_ PIO_STACK_LOCATION Stack)
{
    ACCESS_MASK DesiredAccess = 0;
    ULONG ShareAccess = Stack->Parameters.Create.ShareAccess;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Stack->Parameters.Create.SecurityContext != NULL)
        DesiredAccess = Stack->Parameters.Create.SecurityContext->DesiredAccess;

    if (Fcb->OpenHandleCount > 0)
    {
        Status = IoCheckShareAccess(DesiredAccess,
                                    ShareAccess,
                                    FileObject,
                                    &Fcb->ShareAccess,
                                    TRUE);
    }
    else
    {
        IoSetShareAccess(DesiredAccess,
                         ShareAccess,
                         FileObject,
                         &Fcb->ShareAccess);
    }

    if (NT_SUCCESS(Status))
    {
        Fcb->OpenHandleCount++;
        DeviceExt->OpenHandleCount++;
    }

    return Status;
}

/**
* @name NtfsOpenTargetDirectory
*
* Serves SL_OPEN_TARGET_DIRECTORY, the open the I/O manager does ahead of a
* rename: opens the parent of the named file, reports whether the final
* component exists, and leaves only that component in the file object's name,
* where the rename reads it.
*
* @param DeviceExt
* Volume being opened on.
*
* @param FileObject
* File object of the create, naming the rename target.
*
* @param CaseSensitive
* TRUE for POSIX semantics.
*
* @param Information
* Receives FILE_EXISTS or FILE_DOES_NOT_EXIST for the final component.
*
* @return
* STATUS_SUCCESS with the directory attached to FileObject, or an error.
*/
static
NTSTATUS
NtfsOpenTargetDirectory(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN CaseSensitive,
    _Out_ PULONG_PTR Information)
{
    WCHAR Path[MAX_PATH];
    WCHAR FinalBuffer[MAX_PATH];
    PWSTR AbsFileName = NULL;
    PWSTR LastSeparator;
    UNICODE_STRING FinalName;
    ULONGLONG ExistingMft;
    ULONG FirstEntry = 0;
    PNTFS_FCB ParentFcb = NULL;
    PNTFS_FCB Fcb;
    SIZE_T Length;
    NTSTATUS Status;

    *Information = 0;

    if (FileObject->FileName.Length == 0 || FileObject->FileName.Length >= sizeof(Path))
        return STATUS_OBJECT_NAME_INVALID;

    RtlCopyMemory(Path, FileObject->FileName.Buffer, FileObject->FileName.Length);
    Path[FileObject->FileName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    if (FileObject->RelatedFileObject)
    {
        Status = NtfsMakeAbsoluteFilename(FileObject->RelatedFileObject, Path, &AbsFileName);
        if (!NT_SUCCESS(Status))
            return Status;

        if (AbsFileName)
        {
            if (wcslen(AbsFileName) >= MAX_PATH)
            {
                ExFreePoolWithTag(AbsFileName, TAG_NTFS);
                return STATUS_NAME_TOO_LONG;
            }

            wcscpy(Path, AbsFileName);
            ExFreePoolWithTag(AbsFileName, TAG_NTFS);
        }
    }

    /* A trailing separator names nothing */
    Length = wcslen(Path);
    while (Length > 1 && Path[Length - 1] == L'\\')
        Path[--Length] = UNICODE_NULL;

    LastSeparator = wcsrchr(Path, L'\\');
    if (LastSeparator == NULL || LastSeparator[1] == UNICODE_NULL)
        return STATUS_OBJECT_NAME_INVALID;

    wcscpy(FinalBuffer, LastSeparator + 1);
    RtlInitUnicodeString(&FinalName, FinalBuffer);

    /* Cut the path down to the directory, keeping the root's separator */
    if (LastSeparator == Path)
        LastSeparator[1] = UNICODE_NULL;
    else
        *LastSeparator = UNICODE_NULL;

    Fcb = NtfsGrabFCBFromTable(DeviceExt, Path);
    if (Fcb == NULL)
    {
        Status = NtfsGetFCBForFile(DeviceExt, &ParentFcb, &Fcb, Path, CaseSensitive);
        if (ParentFcb != NULL)
            NtfsReleaseFCB(DeviceExt, ParentFcb);

        if (!NT_SUCCESS(Status))
            return (Status == STATUS_OBJECT_NAME_NOT_FOUND) ? STATUS_OBJECT_PATH_NOT_FOUND : Status;
    }

    if (!NtfsFCBIsDirectory(Fcb))
    {
        NtfsReleaseFCB(DeviceExt, Fcb);
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    Status = NtfsFindMftRecord(DeviceExt, Fcb->MFTIndex, &FinalName, &FirstEntry, FALSE, CaseSensitive, &ExistingMft);
    *Information = NT_SUCCESS(Status) ? FILE_EXISTS : FILE_DOES_NOT_EXIST;

    Status = NtfsAttachFCBToFileObject(DeviceExt, Fcb, FileObject);
    if (!NT_SUCCESS(Status))
    {
        NtfsReleaseFCB(DeviceExt, Fcb);
        return Status;
    }

    /* The final component is a suffix of the original name, so it fits */
    RtlMoveMemory(FileObject->FileName.Buffer, FinalName.Buffer, FinalName.Length);
    FileObject->FileName.Length = FinalName.Length;

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Opens a file
 */
static
NTSTATUS
NtfsCreateFile(PDEVICE_OBJECT DeviceObject,
               PNTFS_IRP_CONTEXT IrpContext);

/* A name such as \ads.txt:stream is the stream "stream" on \ads.txt.
 * Create the base file when it is missing, store the named $DATA, then open
 * that stream. *Handled stays FALSE when the name has no stream. */
static
NTSTATUS
NtfsCreateNamedStream(PDEVICE_OBJECT DeviceObject,
                      PNTFS_IRP_CONTEXT IrpContext,
                      PDEVICE_EXTENSION DeviceExt,
                      PFILE_OBJECT FileObject,
                      PIO_STACK_LOCATION Stack,
                      PIRP Irp,
                      ULONG RequestedOptions,
                      PBOOLEAN Handled)
{
    PWSTR Colon = NULL;
    ULONG NameChars;
    ULONG Index;
    WCHAR Saved;
    USHORT SavedLength;
    PNTFS_FCB BaseFcb = NULL;
    PNTFS_FCB ParentFcb = NULL;
    BOOLEAN CaseSensitive;
    NTSTATUS Status;

    *Handled = FALSE;

    if (FileObject->FileName.Buffer == NULL ||
        FileObject->FileName.Length < sizeof(WCHAR))
    {
        return STATUS_SUCCESS;
    }

    NameChars = FileObject->FileName.Length / sizeof(WCHAR);
    for (Index = 0; Index < NameChars; Index++)
    {
        if (FileObject->FileName.Buffer[Index] == L':')
            Colon = &FileObject->FileName.Buffer[Index];
    }

    if (Colon == NULL || Colon[1] == UNICODE_NULL)
        return STATUS_SUCCESS;

    if (_wcsicmp(Colon + 1, L"$DATA") == 0)
        return STATUS_SUCCESS;

    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
        *Handled = TRUE;
        return STATUS_NOT_A_DIRECTORY;
    }

    *Handled = TRUE;
    CaseSensitive = BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE);
    Saved = *Colon;
    SavedLength = FileObject->FileName.Length;
    *Colon = UNICODE_NULL;
    FileObject->FileName.Length =
        (USHORT)((PUCHAR)Colon - (PUCHAR)FileObject->FileName.Buffer);

    Status = NtfsGetFCBForFile(DeviceExt,
                               &ParentFcb,
                               &BaseFcb,
                               FileObject->FileName.Buffer,
                               CaseSensitive);
    if (!NT_SUCCESS(Status))
    {
        if (BaseFcb != NULL)
            NtfsReleaseFCB(DeviceExt, BaseFcb);
        if (ParentFcb != NULL)
            NtfsReleaseFCB(DeviceExt, ParentFcb);
        BaseFcb = NULL;
        ParentFcb = NULL;

        Status = NtfsCreateFileRecord(DeviceExt,
                                      FileObject,
                                      CaseSensitive,
                                      BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT),
                                      NULL);
        if (NT_SUCCESS(Status))
        {
            Status = NtfsGetFCBForFile(DeviceExt,
                                       &ParentFcb,
                                       &BaseFcb,
                                       FileObject->FileName.Buffer,
                                       CaseSensitive);
        }
    }

    if (NT_SUCCESS(Status) && BaseFcb != NULL)
    {
        ULONGLONG MftIndex = BaseFcb->MFTIndex;

        NtfsReleaseFCB(DeviceExt, BaseFcb);
        BaseFcb = NULL;
        if (ParentFcb != NULL)
        {
            NtfsReleaseFCB(DeviceExt, ParentFcb);
            ParentFcb = NULL;
        }

        Status = NtfsAddNamedData(DeviceExt, MftIndex, Colon + 1);
    }

    if (BaseFcb != NULL)
        NtfsReleaseFCB(DeviceExt, BaseFcb);
    if (ParentFcb != NULL)
        NtfsReleaseFCB(DeviceExt, ParentFcb);

    *Colon = Saved;
    FileObject->FileName.Length = SavedLength;

    if (!NT_SUCCESS(Status))
        return Status;

    Stack->Parameters.Create.Options = ((ULONG)FILE_OPEN << 24) | RequestedOptions;
    Status = NtfsCreateFile(DeviceObject, IrpContext);
    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information = FILE_CREATED;

    return Status;
}

static
NTSTATUS
NtfsCreateFile(PDEVICE_OBJECT DeviceObject,
               PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    ULONG RequestedDisposition;
    ULONG RequestedOptions;
    PNTFS_FCB Fcb = NULL;
//    PWSTR FileName;
    NTSTATUS Status;
    UNICODE_STRING FullPath = { 0, 0, NULL };
    PIRP Irp = IrpContext->Irp;

    DPRINT("NtfsCreateFile(%p, %p) called\n", DeviceObject, IrpContext);

    DeviceExt = DeviceObject->DeviceExtension;
    ASSERT(DeviceExt);
    Stack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(Stack);

    RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
    RequestedOptions = Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
//  PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
    if (RequestedOptions & FILE_DIRECTORY_FILE &&
        RequestedDisposition == FILE_SUPERSEDE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Deny create if the volume is locked */
    if (DeviceExt->Flags & VCB_VOLUME_LOCKED)
    {
        return STATUS_ACCESS_DENIED;
    }

    /* A dirty volume is readable. Creating, replacing, or deleting is not. */
    if ((DeviceExt->Flags & VCB_VOLUME_READ_ONLY) &&
        (RequestedDisposition != FILE_OPEN ||
         (RequestedOptions & FILE_DELETE_ON_CLOSE)))
    {
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    FileObject = Stack->FileObject;

    /* The I/O manager opening a rename's destination directory */
    if (Stack->Flags & SL_OPEN_TARGET_DIRECTORY)
    {
        ULONG_PTR Information;

        if (RequestedOptions & FILE_OPEN_BY_FILE_ID)
            return STATUS_INVALID_PARAMETER;

        Status = NtfsOpenTargetDirectory(DeviceExt,
                                         FileObject,
                                         BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                         &Information);
        if (!NT_SUCCESS(Status))
            return Status;

        Status = NtfsGrantShareAccess(DeviceExt, FileObject->FsContext, FileObject, Stack);
        if (!NT_SUCCESS(Status))
        {
            NtfsCloseFile(DeviceExt, FileObject);
            return Status;
        }

        Irp->IoStatus.Information = Information;
        return STATUS_SUCCESS;
    }

    if ((RequestedOptions & FILE_OPEN_BY_FILE_ID) == FILE_OPEN_BY_FILE_ID)
    {
        ULONGLONG MFTId;

        if (FileObject->FileName.Length != sizeof(ULONGLONG))
            return STATUS_INVALID_PARAMETER;

        MFTId = (*(PULONGLONG)FileObject->FileName.Buffer) & NTFS_MFT_MASK;
        if (MFTId < NTFS_FILE_FIRST_USER_FILE)
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

        DPRINT("Open by ID: %I64x -> %wZ\n", (*(PULONGLONG)FileObject->FileName.Buffer) & NTFS_MFT_MASK, &FullPath);
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

        Status = NtfsAttachFCBToFileObject(DeviceExt, DeviceExt->VolumeFcb, FileObject);
        if (!NT_SUCCESS(Status))
            return Status;

        /* The volume handle counts. Locking the volume requires this to be
         * the only open, and close drops the same count. */
        Status = NtfsGrantShareAccess(DeviceExt, DeviceExt->VolumeFcb, FileObject, Stack);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(FileObject->FsContext2);
            FileObject->FsContext2 = NULL;
            FileObject->FsContext = NULL;
            return Status;
        }

        DeviceExt->VolumeFcb->RefCount++;

        Irp->IoStatus.Information = FILE_OPENED;
        return STATUS_SUCCESS;
    }

    if (Fcb == NULL)
    {
        Status = NtfsOpenFile(DeviceExt,
                              FileObject,
                              ((RequestedOptions & FILE_OPEN_BY_FILE_ID) ? FullPath.Buffer : FileObject->FileName.Buffer),
                              BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
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

        if (RequestedDisposition == FILE_OVERWRITE ||
            RequestedDisposition == FILE_OVERWRITE_IF ||
            RequestedDisposition == FILE_SUPERSEDE)
        {
            PFILE_RECORD_HEADER fileRecord = NULL;
            PNTFS_ATTR_CONTEXT dataContext = NULL;
            ULONG DataAttributeOffset;
            LARGE_INTEGER Zero;
            Zero.QuadPart = 0;

            if (!NtfsGlobalData->EnableWriteSupport)
            {
                DPRINT1("NTFS write-support is EXPERIMENTAL and is disabled by default!\n");
                NtfsCloseFile(DeviceExt, FileObject);
                return STATUS_ACCESS_DENIED;
            }

            // TODO: check for appropriate access

            ExAcquireResourceExclusiveLite(&(Fcb->MainResource), TRUE);

            fileRecord = ExAllocateFromNPagedLookasideList(&Fcb->Vcb->FileRecLookasideList);
            if (fileRecord)
            {

                Status = ReadFileRecord(Fcb->Vcb,
                                        Fcb->MFTIndex,
                                        fileRecord);
                if (!NT_SUCCESS(Status))
                    goto DoneOverwriting;

                // find the data attribute and set it's length to 0 (TODO: Handle Alternate Data Streams)
                Status = FindAttribute(Fcb->Vcb, fileRecord, AttributeData, L"", 0, &dataContext, &DataAttributeOffset);
                if (!NT_SUCCESS(Status))
                    goto DoneOverwriting;

                Status = SetAttributeDataLength(FileObject, Fcb, dataContext, DataAttributeOffset, fileRecord, &Zero);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }

        DoneOverwriting:
            if (fileRecord)
                ExFreeToNPagedLookasideList(&Fcb->Vcb->FileRecLookasideList, fileRecord);
            if (dataContext)
                ReleaseAttributeContext(dataContext);

            ExReleaseResourceLite(&(Fcb->MainResource));

            if (!NT_SUCCESS(Status))
            {
                NtfsCloseFile(DeviceExt, FileObject);
                return Status;
            }

            if (RequestedDisposition == FILE_SUPERSEDE)
            {
                Irp->IoStatus.Information = FILE_SUPERSEDED;
            }
            else
            {
                Irp->IoStatus.Information = FILE_OVERWRITTEN;
            }
        }
    }
    else
    {
        /* HUGLY HACK: Can't create new files yet... */
        if (RequestedDisposition == FILE_CREATE ||
            RequestedDisposition == FILE_OPEN_IF ||
            RequestedDisposition == FILE_OVERWRITE_IF ||
            RequestedDisposition == FILE_SUPERSEDE)
        {
            if (!NtfsGlobalData->EnableWriteSupport)
            {
                DPRINT1("NTFS write-support is EXPERIMENTAL and is disabled by default!\n");
                NtfsCloseFile(DeviceExt, FileObject);
                return STATUS_ACCESS_DENIED;
            }

            {
                BOOLEAN Handled = FALSE;

                Status = NtfsCreateNamedStream(DeviceObject,
                                               IrpContext,
                                               DeviceExt,
                                               FileObject,
                                               Stack,
                                               Irp,
                                               RequestedOptions,
                                               &Handled);
                if (Handled)
                    return Status;
            }

            // Was the user trying to create a directory?
            if (RequestedOptions & FILE_DIRECTORY_FILE)
            {
                // Create the directory on disk
                Status = NtfsCreateDirectory(DeviceExt,
                                             FileObject,
                                             BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                             BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT),
                                             (Stack->Parameters.Create.SecurityContext != NULL &&
                                              Stack->Parameters.Create.SecurityContext->AccessState != NULL) ?
                                                 Stack->Parameters.Create.SecurityContext->AccessState->SecurityDescriptor :
                                                 NULL);
            }
            else
            {
                // Create the file record on disk
                Status = NtfsCreateFileRecord(DeviceExt,
                                              FileObject,
                                              BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                              BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT),
                                              (Stack->Parameters.Create.SecurityContext != NULL &&
                                               Stack->Parameters.Create.SecurityContext->AccessState != NULL) ?
                                                  Stack->Parameters.Create.SecurityContext->AccessState->SecurityDescriptor :
                                                  NULL);
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ERROR: Couldn't create '%wZ' (Status %lx)\n",
                        &FileObject->FileName, Status);
                return Status;
            }

            // Before we open the file/directory we just created, we need to change the disposition (upper 8 bits of ULONG)
            // from create to open, since we already created the file
            Stack->Parameters.Create.Options = (ULONG)FILE_OPEN << 24 | RequestedOptions;

            // Now we should be able to open the file using NtfsCreateFile()
            Status = NtfsCreateFile(DeviceObject, IrpContext);
            if (NT_SUCCESS(Status))
            {
                // We need to change Irp->IoStatus.Information to reflect creation
                Irp->IoStatus.Information = FILE_CREATED;
            }
            else
            {
                DPRINT1("ERROR: Created '%wZ' but could not open it (Status %lx)\n",
                        &FileObject->FileName, Status);
            }
            return Status;
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = NtfsGrantShareAccess(DeviceExt, Fcb, FileObject, Stack);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Sharing violation opening '%wZ'\n", &FileObject->FileName);
            NtfsCloseFile(DeviceExt, FileObject);
            Irp->IoStatus.Information = 0;
            return Status;
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

    if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    /* Directory indexes take their own lock. The FCB table has its spinlock,
     * so a create in one directory does not wait for a create in another. */
    Status = NtfsCreateFile(DeviceObject,
                            IrpContext);

    return Status;
}

/**
* @name NtfsCreateDirectory()
* @implemented
*
* Creates a file record for a new directory and saves it to the MFT. Adds the filename attribute of the
* created directory to the parent directory's index.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param FileObject
* Pointer to a FILE_OBJECT describing the directory to be created
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application created the folder with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @param CanWait
* Boolean indicating if the function is allowed to wait for exclusive access to the master file table.
* This will only be relevant if the MFT doesn't have any free file records and needs to be enlarged.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if unable to allocate memory for the file record.
* STATUS_CANT_WAIT if CanWait was FALSE and the function needed to resize the MFT but
* couldn't get immediate, exclusive access to it.
*/
NTSTATUS
NtfsCreateDirectory(PDEVICE_EXTENSION DeviceExt,
                    PFILE_OBJECT FileObject,
                    BOOLEAN CaseSensitive,
                    BOOLEAN CanWait,
                    PSECURITY_DESCRIPTOR ExplicitSecurity)
{

    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_RECORD NextAttribute;
    PFILENAME_ATTRIBUTE FilenameAttribute;
    PFILENAME_ATTRIBUTE DosAttribute = NULL;
    ULONGLONG ParentMftIndex;
    ULONGLONG FileMftIndex;
    PB_TREE Tree;
    PINDEX_ROOT_ATTRIBUTE NewIndexRoot;
    ULONG MaxIndexRootSize;
    ULONG RootLength;

    DPRINT("NtfsCreateFileRecord(%p, %p, %s, %s)\n",
            DeviceExt,
            FileObject,
            CaseSensitive ? "TRUE" : "FALSE",
            CanWait ? "TRUE" : "FALSE");

    // Start with an empty file record
    FileRecord = NtfsCreateEmptyFileRecord(DeviceExt);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set the directory flag
    FileRecord->Flags |= FRH_DIRECTORY;

    // find where the first attribute will be added
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);

    // add first attribute, $STANDARD_INFORMATION
    AddStandardInformation(DeviceExt, FileRecord, NextAttribute, NTFS_SECURITY_ID);

    // advance NextAttribute pointer to the next attribute
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute + (ULONG_PTR)NextAttribute->Length);

    // Add the $FILE_NAME attribute
    Status = AddFileName(FileRecord, NextAttribute, DeviceExt, FileObject, CaseSensitive, &ParentMftIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to add the $FILE_NAME attribute for '%wZ'!\n", &FileObject->FileName);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    /* The child's security id comes from $Secure. */
    NtfsAssignSecurity(DeviceExt, FileRecord, ParentMftIndex, ExplicitSecurity, TRUE);

    // save a pointer to the filename attribute
    FilenameAttribute = (PFILENAME_ATTRIBUTE)((ULONG_PTR)NextAttribute + NextAttribute->Resident.ValueOffset);

    // advance NextAttribute pointer to the next attribute
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute + (ULONG_PTR)NextAttribute->Length);

    if (FilenameAttribute->NameType == NTFS_FILE_NAME_WIN32 &&
        DeviceExt->NtfsInfo.ShortNameCreation)
    {
        Status = AddDosFileName(FileRecord,
                                NextAttribute,
                                DeviceExt,
                                FilenameAttribute,
                                ParentMftIndex,
                                CaseSensitive);
        if (NT_SUCCESS(Status))
        {
            DosAttribute = (PFILENAME_ATTRIBUTE)((ULONG_PTR)NextAttribute +
                                                 NextAttribute->Resident.ValueOffset);
            NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute +
                                               (ULONG_PTR)NextAttribute->Length);
        }
    }

    // Create an empty b-tree to represent our new index
    Status = CreateEmptyBTree(&Tree);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to create empty B-Tree!\n");
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // Calculate maximum size of index root
    MaxIndexRootSize = DeviceExt->NtfsInfo.BytesPerFileRecord
                       - ((ULONG_PTR)NextAttribute - (ULONG_PTR)FileRecord)
                       - sizeof(ULONG) * 2;

    // Create a new index record from the tree
    Status = CreateIndexRootFromBTree(DeviceExt,
                                      Tree,
                                      MaxIndexRootSize,
                                      &NewIndexRoot,
                                      &RootLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to create empty index root!\n");
        DestroyBTree(Tree);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // We're done with the B-Tree
    DestroyBTree(Tree);

    // add the $INDEX_ROOT attribute
    Status = AddIndexRoot(DeviceExt, FileRecord, NextAttribute, NewIndexRoot, RootLength, L"$I30", 4);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to add index root to new file record!\n");
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }


#ifndef NDEBUG
    NtfsDumpFileRecord(DeviceExt, FileRecord);
#endif

    // Now that we've built the file record in memory, we need to store it in the MFT.
    Status = AddNewMftEntry(FileRecord, DeviceExt, &FileMftIndex, CanWait);
    if (NT_SUCCESS(Status))
    {
        // The highest 2 bytes should be the sequence number, unless the parent happens to be root
        if (FileMftIndex == NTFS_FILE_ROOT)
            FileMftIndex = FileMftIndex + ((ULONGLONG)NTFS_FILE_ROOT << 48);
        else
            FileMftIndex = FileMftIndex + ((ULONGLONG)FileRecord->SequenceNumber << 48);

        DPRINT("New File Reference: 0x%016I64x\n", FileMftIndex);

        // Add the filename attribute to the filename-index of the parent directory
        Status = NtfsAddFilenameToDirectory(DeviceExt,
                                            ParentMftIndex,
                                            FileMftIndex,
                                            FilenameAttribute,
                                            CaseSensitive);
        if (NT_SUCCESS(Status) && DosAttribute != NULL)
        {
            Status = NtfsAddFilenameToDirectory(DeviceExt,
                                                ParentMftIndex,
                                                FileMftIndex,
                                                DosAttribute,
                                                CaseSensitive);
        }
        if (NT_SUCCESS(Status))
        {
            UNICODE_STRING FinalName;
            FinalName.Buffer = FilenameAttribute->Name;
            FinalName.Length = FilenameAttribute->NameLength * sizeof(WCHAR);
            FinalName.MaximumLength = FinalName.Length;
            NtfsAppendUsn(DeviceExt,
                          FileMftIndex,
                          ParentMftIndex,
                          USN_REASON_FILE_CREATE,
                          FilenameAttribute->FileAttributes,
                          &FinalName);
        }
    }

    ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/**
* @name NtfsCreateEmptyFileRecord
* @implemented
*
* Creates a new, empty file record, with no attributes.
*
* @param DeviceExt
* Pointer to the DEVICE_EXTENSION of the target volume the file record will be stored on.
*
* @return
* A pointer to the newly-created FILE_RECORD_HEADER if the function succeeds, NULL otherwise.
*/
PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(PDEVICE_EXTENSION DeviceExt)
{
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_RECORD NextAttribute;

    DPRINT("NtfsCreateEmptyFileRecord(%p)\n", DeviceExt);

    // allocate memory for file record
    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record!\n");
        return NULL;
    }

    RtlZeroMemory(FileRecord, DeviceExt->NtfsInfo.BytesPerFileRecord);

    FileRecord->Ntfs.Type = NRH_FILE_TYPE;

    // calculate USA offset and count
    FileRecord->Ntfs.UsaOffset = FIELD_OFFSET(FILE_RECORD_HEADER, MFTRecordNumber) + sizeof(ULONG);

    // size of USA (in ULONG's) will be 1 (for USA number) + 1 for every sector the file record uses
    FileRecord->BytesAllocated = DeviceExt->NtfsInfo.BytesPerFileRecord;
    FileRecord->Ntfs.UsaCount = (FileRecord->BytesAllocated / DeviceExt->NtfsInfo.BytesPerSector) + 1;

    // setup other file record fields
    FileRecord->SequenceNumber = 1;
    FileRecord->AttributeOffset = FileRecord->Ntfs.UsaOffset + (2 * FileRecord->Ntfs.UsaCount);
    FileRecord->AttributeOffset = ALIGN_UP_BY(FileRecord->AttributeOffset, ATTR_RECORD_ALIGNMENT);
    FileRecord->Flags = FRH_IN_USE;
    FileRecord->BytesInUse = FileRecord->AttributeOffset + sizeof(ULONG) * 2;

    // find where the first attribute will be added
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);

    // mark the (temporary) end of the file-record
    NextAttribute->Type = AttributeEnd;
    NextAttribute->Length = FILE_RECORD_END;

    return FileRecord;
}


/**
* @name NtfsCreateFileRecord()
* @implemented
*
* Creates a file record and saves it to the MFT. Adds the filename attribute of the
* created file to the parent directory's index.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param FileObject
* Pointer to a FILE_OBJECT describing the file to be created
*
* @param CanWait
* Boolean indicating if the function is allowed to wait for exclusive access to the master file table.
* This will only be relevant if the MFT doesn't have any free file records and needs to be enlarged.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if unable to allocate memory for the file record.
* STATUS_CANT_WAIT if CanWait was FALSE and the function needed to resize the MFT but
* couldn't get immediate, exclusive access to it.
*/
NTSTATUS
NtfsCreateFileRecord(PDEVICE_EXTENSION DeviceExt,
                     PFILE_OBJECT FileObject,
                     BOOLEAN CaseSensitive,
                     BOOLEAN CanWait,
                     PSECURITY_DESCRIPTOR ExplicitSecurity)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_RECORD NextAttribute;
    PFILENAME_ATTRIBUTE FilenameAttribute;
    PFILENAME_ATTRIBUTE DosAttribute = NULL;
    ULONGLONG ParentMftIndex;
    ULONGLONG FileMftIndex;

    DPRINT("NtfsCreateFileRecord(%p, %p, %s, %s)\n",
            DeviceExt,
            FileObject,
            CaseSensitive ? "TRUE" : "FALSE",
            CanWait ? "TRUE" : "FALSE");

    // allocate memory for file record
    FileRecord = NtfsCreateEmptyFileRecord(DeviceExt);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // find where the first attribute will be added
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);

    // add first attribute, $STANDARD_INFORMATION
    AddStandardInformation(DeviceExt, FileRecord, NextAttribute, NTFS_SECURITY_ID);

    // advance NextAttribute pointer to the next attribute
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute + (ULONG_PTR)NextAttribute->Length);

    // Add the $FILE_NAME attribute
    Status = AddFileName(FileRecord, NextAttribute, DeviceExt, FileObject, CaseSensitive, &ParentMftIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to add the $FILE_NAME attribute for '%wZ'!\n", &FileObject->FileName);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    /* The child's security id comes from $Secure. */
    NtfsAssignSecurity(DeviceExt, FileRecord, ParentMftIndex, ExplicitSecurity, FALSE);

    // save a pointer to the filename attribute
    FilenameAttribute = (PFILENAME_ATTRIBUTE)((ULONG_PTR)NextAttribute + NextAttribute->Resident.ValueOffset);

    // advance NextAttribute pointer to the next attribute
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute + (ULONG_PTR)NextAttribute->Length);

    /* A Win32 name that is not itself 8.3 gets a DOS alias when this volume
     * creates short names. POSIX creates do not. */
    if (FilenameAttribute->NameType == NTFS_FILE_NAME_WIN32 &&
        DeviceExt->NtfsInfo.ShortNameCreation)
    {
        Status = AddDosFileName(FileRecord,
                                NextAttribute,
                                DeviceExt,
                                FilenameAttribute,
                                ParentMftIndex,
                                CaseSensitive);
        if (NT_SUCCESS(Status))
        {
            DosAttribute = (PFILENAME_ATTRIBUTE)((ULONG_PTR)NextAttribute +
                                                 NextAttribute->Resident.ValueOffset);
            NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute +
                                               (ULONG_PTR)NextAttribute->Length);
        }
        else
        {
            DPRINT1("DOS name not stored for '%wZ' (Status %lx)\n",
                    &FileObject->FileName, Status);
        }
    }

    // add the $DATA attribute
    AddData(FileRecord, NextAttribute);

#ifndef NDEBUG
    // dump file record in memory (for debugging)
    NtfsDumpFileRecord(DeviceExt, FileRecord);
#endif

    // Now that we've built the file record in memory, we need to store it in the MFT.
    Status = AddNewMftEntry(FileRecord, DeviceExt, &FileMftIndex, CanWait);
    if (NT_SUCCESS(Status))
    {
        // The highest 2 bytes should be the sequence number, unless the parent happens to be root
        if (FileMftIndex == NTFS_FILE_ROOT)
            FileMftIndex = FileMftIndex + ((ULONGLONG)NTFS_FILE_ROOT << 48);
        else
            FileMftIndex = FileMftIndex + ((ULONGLONG)FileRecord->SequenceNumber << 48);

        DPRINT("New File Reference: 0x%016I64x\n", FileMftIndex);

        // Add the filename attribute to the filename-index of the parent directory
        Status = NtfsAddFilenameToDirectory(DeviceExt,
                                            ParentMftIndex,
                                            FileMftIndex,
                                            FilenameAttribute,
                                            CaseSensitive);
        if (NT_SUCCESS(Status) && DosAttribute != NULL)
        {
            Status = NtfsAddFilenameToDirectory(DeviceExt,
                                                ParentMftIndex,
                                                FileMftIndex,
                                                DosAttribute,
                                                CaseSensitive);
        }
        if (NT_SUCCESS(Status))
        {
            UNICODE_STRING FinalName;
            FinalName.Buffer = FilenameAttribute->Name;
            FinalName.Length = FilenameAttribute->NameLength * sizeof(WCHAR);
            FinalName.MaximumLength = FinalName.Length;
            NtfsAppendUsn(DeviceExt,
                          FileMftIndex,
                          ParentMftIndex,
                          USN_REASON_FILE_CREATE,
                          FilenameAttribute->FileAttributes,
                          &FinalName);
        }
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/* EOF */
