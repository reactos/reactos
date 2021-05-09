/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/fastfat/finfo.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Herve Poussineau (reactos@poussine.freesurf.fr)
 *                   Pierre Schweitzer (pierre@reactos.org)
 *
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

#define NASSERTS_RENAME

/* GLOBALS ******************************************************************/

const char* FileInformationClassNames[] =
{
    "??????",
    "FileDirectoryInformation",
    "FileFullDirectoryInformation",
    "FileBothDirectoryInformation",
    "FileBasicInformation",
    "FileStandardInformation",
    "FileInternalInformation",
    "FileEaInformation",
    "FileAccessInformation",
    "FileNameInformation",
    "FileRenameInformation",
    "FileLinkInformation",
    "FileNamesInformation",
    "FileDispositionInformation",
    "FilePositionInformation",
    "FileFullEaInformation",
    "FileModeInformation",
    "FileAlignmentInformation",
    "FileAllInformation",
    "FileAllocationInformation",
    "FileEndOfFileInformation",
    "FileAlternateNameInformation",
    "FileStreamInformation",
    "FilePipeInformation",
    "FilePipeLocalInformation",
    "FilePipeRemoteInformation",
    "FileMailslotQueryInformation",
    "FileMailslotSetInformation",
    "FileCompressionInformation",
    "FileObjectIdInformation",
    "FileCompletionInformation",
    "FileMoveClusterInformation",
    "FileQuotaInformation",
    "FileReparsePointInformation",
    "FileNetworkOpenInformation",
    "FileAttributeTagInformation",
    "FileTrackingInformation",
    "FileIdBothDirectoryInformation",
    "FileIdFullDirectoryInformation",
    "FileValidDataLengthInformation",
    "FileShortNameInformation",
    "FileMaximumInformation"
};

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Retrieve the standard file information
 */
NTSTATUS
VfatGetStandardInformation(
    PVFATFCB FCB,
    PFILE_STANDARD_INFORMATION StandardInfo,
    PULONG BufferLength)
{
    if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    /* PRECONDITION */
    ASSERT(StandardInfo != NULL);
    ASSERT(FCB != NULL);

    if (vfatFCBIsDirectory(FCB))
    {
        StandardInfo->AllocationSize.QuadPart = 0;
        StandardInfo->EndOfFile.QuadPart = 0;
        StandardInfo->Directory = TRUE;
    }
    else
    {
        StandardInfo->AllocationSize = FCB->RFCB.AllocationSize;
        StandardInfo->EndOfFile = FCB->RFCB.FileSize;
        StandardInfo->Directory = FALSE;
    }
    StandardInfo->NumberOfLinks = 1;
    StandardInfo->DeletePending = BooleanFlagOn(FCB->Flags, FCB_DELETE_PENDING);

    *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatSetPositionInformation(
    PFILE_OBJECT FileObject,
    PFILE_POSITION_INFORMATION PositionInfo)
{
    DPRINT("FsdSetPositionInformation()\n");

    DPRINT("PositionInfo %p\n", PositionInfo);
    DPRINT("Setting position %u\n", PositionInfo->CurrentByteOffset.u.LowPart);

    FileObject->CurrentByteOffset.QuadPart =
        PositionInfo->CurrentByteOffset.QuadPart;

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetPositionInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_POSITION_INFORMATION PositionInfo,
    PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FCB);
    UNREFERENCED_PARAMETER(DeviceExt);

    DPRINT("VfatGetPositionInformation()\n");

    if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    PositionInfo->CurrentByteOffset.QuadPart =
        FileObject->CurrentByteOffset.QuadPart;

    DPRINT("Getting position %I64x\n",
           PositionInfo->CurrentByteOffset.QuadPart);

    *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatSetBasicInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_BASIC_INFORMATION BasicInfo)
{
    ULONG NotifyFilter;

    DPRINT("VfatSetBasicInformation()\n");

    ASSERT(NULL != FileObject);
    ASSERT(NULL != FCB);
    ASSERT(NULL != DeviceExt);
    ASSERT(NULL != BasicInfo);
    /* Check volume label bit */
    ASSERT(0 == (*FCB->Attributes & _A_VOLID));

    NotifyFilter = 0;

    if (BasicInfo->FileAttributes != 0)
    {
        UCHAR Attributes;

        Attributes = (BasicInfo->FileAttributes & (FILE_ATTRIBUTE_ARCHIVE |
                                                   FILE_ATTRIBUTE_SYSTEM |
                                                   FILE_ATTRIBUTE_HIDDEN |
                                                   FILE_ATTRIBUTE_DIRECTORY |
                                                   FILE_ATTRIBUTE_READONLY));

        if (vfatFCBIsDirectory(FCB))
        {
            if (BooleanFlagOn(BasicInfo->FileAttributes, FILE_ATTRIBUTE_TEMPORARY))
            {
                DPRINT("Setting temporary attribute on a directory!\n");
                return STATUS_INVALID_PARAMETER;
            }

            Attributes |= FILE_ATTRIBUTE_DIRECTORY;
        }
        else
        {
            if (BooleanFlagOn(BasicInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                DPRINT("Setting directory attribute on a file!\n");
                return STATUS_INVALID_PARAMETER;
            }
        }

        if (Attributes != *FCB->Attributes)
        {
            *FCB->Attributes = Attributes;
            DPRINT("Setting attributes 0x%02x\n", *FCB->Attributes);
            NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
        }
    }

    if (vfatVolumeIsFatX(DeviceExt))
    {
        if (BasicInfo->CreationTime.QuadPart != 0 && BasicInfo->CreationTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->CreationTime,
                                       &FCB->entry.FatX.CreationDate,
                                       &FCB->entry.FatX.CreationTime);
            NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
        }

        if (BasicInfo->LastAccessTime.QuadPart != 0 && BasicInfo->LastAccessTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->LastAccessTime,
                                       &FCB->entry.FatX.AccessDate,
                                       &FCB->entry.FatX.AccessTime);
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }

        if (BasicInfo->LastWriteTime.QuadPart != 0 && BasicInfo->LastWriteTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->LastWriteTime,
                                       &FCB->entry.FatX.UpdateDate,
                                       &FCB->entry.FatX.UpdateTime);
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }
    }
    else
    {
        if (BasicInfo->CreationTime.QuadPart != 0 && BasicInfo->CreationTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->CreationTime,
                                       &FCB->entry.Fat.CreationDate,
                                       &FCB->entry.Fat.CreationTime);
            NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
        }

        if (BasicInfo->LastAccessTime.QuadPart != 0 && BasicInfo->LastAccessTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->LastAccessTime,
                                       &FCB->entry.Fat.AccessDate,
                                       NULL);
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }

        if (BasicInfo->LastWriteTime.QuadPart != 0 && BasicInfo->LastWriteTime.QuadPart != -1)
        {
            FsdSystemTimeToDosDateTime(DeviceExt,
                                       &BasicInfo->LastWriteTime,
                                       &FCB->entry.Fat.UpdateDate,
                                       &FCB->entry.Fat.UpdateTime);
            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }
    }

    VfatUpdateEntry(DeviceExt, FCB);

    if (NotifyFilter != 0)
    {
        vfatReportChange(DeviceExt,
                         FCB,
                         NotifyFilter,
                         FILE_ACTION_MODIFIED);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VfatGetBasicInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_BASIC_INFORMATION BasicInfo,
    PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(FileObject);

    DPRINT("VfatGetBasicInformation()\n");

    if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    RtlZeroMemory(BasicInfo, sizeof(FILE_BASIC_INFORMATION));

    if (vfatVolumeIsFatX(DeviceExt))
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.FatX.CreationDate,
                                   FCB->entry.FatX.CreationTime,
                                   &BasicInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.FatX.AccessDate,
                                   FCB->entry.FatX.AccessTime,
                                   &BasicInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.FatX.UpdateDate,
                                   FCB->entry.FatX.UpdateTime,
                                   &BasicInfo->LastWriteTime);
        BasicInfo->ChangeTime = BasicInfo->LastWriteTime;
    }
    else
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.Fat.CreationDate,
                                   FCB->entry.Fat.CreationTime,
                                   &BasicInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.Fat.AccessDate,
                                   0,
                                   &BasicInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   FCB->entry.Fat.UpdateDate,
                                   FCB->entry.Fat.UpdateTime,
                                   &BasicInfo->LastWriteTime);
        BasicInfo->ChangeTime = BasicInfo->LastWriteTime;
    }

    BasicInfo->FileAttributes = *FCB->Attributes & 0x3f;
    /* Synthesize FILE_ATTRIBUTE_NORMAL */
    if (0 == (BasicInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                           FILE_ATTRIBUTE_ARCHIVE |
                                           FILE_ATTRIBUTE_SYSTEM |
                                           FILE_ATTRIBUTE_HIDDEN |
                                           FILE_ATTRIBUTE_READONLY)))
    {
        DPRINT("Synthesizing FILE_ATTRIBUTE_NORMAL\n");
        BasicInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
    }
    DPRINT("Getting attributes 0x%02x\n", BasicInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_BASIC_INFORMATION);
    return STATUS_SUCCESS;
}


static
NTSTATUS
VfatSetDispositionInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
    DPRINT("FsdSetDispositionInformation(<%wZ>, Delete %u)\n", &FCB->PathNameU, DispositionInfo->DeleteFile);

    ASSERT(DeviceExt != NULL);
    ASSERT(DeviceExt->FatInfo.BytesPerCluster != 0);
    ASSERT(FCB != NULL);

    if (!DispositionInfo->DeleteFile)
    {
        /* undelete the file */
        FCB->Flags &= ~FCB_DELETE_PENDING;
        FileObject->DeletePending = FALSE;
        return STATUS_SUCCESS;
    }

    if (BooleanFlagOn(FCB->Flags, FCB_DELETE_PENDING))
    {
        /* stream already marked for deletion. just update the file object */
        FileObject->DeletePending = TRUE;
        return STATUS_SUCCESS;
    }

    if (vfatFCBIsReadOnly(FCB))
    {
        return STATUS_CANNOT_DELETE;
    }

    if (vfatFCBIsRoot(FCB) || IsDotOrDotDot(&FCB->LongNameU))
    {
        /* we cannot delete a '.', '..' or the root directory */
        return STATUS_ACCESS_DENIED;
    }

    if (!MmFlushImageSection (FileObject->SectionObjectPointer, MmFlushForDelete))
    {
        /* can't delete a file if its mapped into a process */

        DPRINT("MmFlushImageSection returned FALSE\n");
        return STATUS_CANNOT_DELETE;
    }

    if (vfatFCBIsDirectory(FCB) && !VfatIsDirectoryEmpty(DeviceExt, FCB))
    {
        /* can't delete a non-empty directory */

        return STATUS_DIRECTORY_NOT_EMPTY;
    }

    /* all good */
    FCB->Flags |= FCB_DELETE_PENDING;
    FileObject->DeletePending = TRUE;

    return STATUS_SUCCESS;
}

static NTSTATUS
vfatPrepareTargetForRename(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB * ParentFCB,
    IN PUNICODE_STRING NewName,
    IN BOOLEAN ReplaceIfExists,
    IN PUNICODE_STRING ParentName,
    OUT PBOOLEAN Deleted)
{
    NTSTATUS Status;
    PVFATFCB TargetFcb;

    DPRINT("vfatPrepareTargetForRename(%p, %p, %wZ, %d, %wZ, %p)\n", DeviceExt, ParentFCB, NewName, ReplaceIfExists, ParentName);

    *Deleted = FALSE;
    /* Try to open target */
    Status = vfatGetFCBForFile(DeviceExt, ParentFCB, &TargetFcb, NewName);
    /* If it exists */
    if (NT_SUCCESS(Status))
    {
        DPRINT("Target file %wZ exists. FCB Flags %08x\n", NewName, TargetFcb->Flags);
        /* Check whether we are allowed to replace */
        if (ReplaceIfExists)
        {
            /* If that's a directory or a read-only file, we're not allowed */
            if (vfatFCBIsDirectory(TargetFcb) || vfatFCBIsReadOnly(TargetFcb))
            {
                DPRINT("And this is a readonly file!\n");
                vfatReleaseFCB(DeviceExt, *ParentFCB);
                *ParentFCB = NULL;
                vfatReleaseFCB(DeviceExt, TargetFcb);
                return STATUS_OBJECT_NAME_COLLISION;
            }


            /* If we still have a file object, close it. */
            if (TargetFcb->FileObject)
            {
                if (!MmFlushImageSection(TargetFcb->FileObject->SectionObjectPointer, MmFlushForDelete))
                {
                    DPRINT("MmFlushImageSection failed.\n");
                    vfatReleaseFCB(DeviceExt, *ParentFCB);
                    *ParentFCB = NULL;
                    vfatReleaseFCB(DeviceExt, TargetFcb);
                    return STATUS_ACCESS_DENIED;
                }

                TargetFcb->FileObject->DeletePending = TRUE;
                VfatCloseFile(DeviceExt, TargetFcb->FileObject);
            }

            /* If we are here, ensure the file isn't open by anyone! */
            if (TargetFcb->OpenHandleCount != 0)
            {
                DPRINT("There are still open handles for this file.\n");
                vfatReleaseFCB(DeviceExt, *ParentFCB);
                *ParentFCB = NULL;
                vfatReleaseFCB(DeviceExt, TargetFcb);
                return STATUS_ACCESS_DENIED;
            }

            /* Effectively delete old file to allow renaming */
            DPRINT("Effectively deleting the file.\n");
            VfatDelEntry(DeviceExt, TargetFcb, NULL);
            vfatReleaseFCB(DeviceExt, TargetFcb);
            *Deleted = TRUE;
            return STATUS_SUCCESS;
        }
        else
        {
            vfatReleaseFCB(DeviceExt, *ParentFCB);
            *ParentFCB = NULL;
            vfatReleaseFCB(DeviceExt, TargetFcb);
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }
    else if (*ParentFCB != NULL)
    {
        return STATUS_SUCCESS;
    }

    /* Failure */
    return Status;
}

static
BOOLEAN
IsThereAChildOpened(PVFATFCB FCB)
{
    PLIST_ENTRY Entry;
    PVFATFCB VolFCB;

    for (Entry = FCB->ParentListHead.Flink; Entry != &FCB->ParentListHead; Entry = Entry->Flink)
    {
        VolFCB = CONTAINING_RECORD(Entry, VFATFCB, ParentListEntry);
        if (VolFCB->OpenHandleCount != 0)
        {
            ASSERT(VolFCB->parentFcb == FCB);
            DPRINT1("At least one children file opened! %wZ (%u, %u)\n", &VolFCB->PathNameU, VolFCB->RefCount, VolFCB->OpenHandleCount);
            return TRUE;
        }

        if (vfatFCBIsDirectory(VolFCB) && !IsListEmpty(&VolFCB->ParentListHead))
        {
            if (IsThereAChildOpened(VolFCB))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static
VOID
VfatRenameChildFCB(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB FCB)
{
    PLIST_ENTRY Entry;
    PVFATFCB Child;

    if (IsListEmpty(&FCB->ParentListHead))
        return;

    for (Entry = FCB->ParentListHead.Flink; Entry != &FCB->ParentListHead; Entry = Entry->Flink)
    {
        NTSTATUS Status;

        Child = CONTAINING_RECORD(Entry, VFATFCB, ParentListEntry);
        DPRINT("Found %wZ with still %lu references (parent: %lu)!\n", &Child->PathNameU, Child->RefCount, FCB->RefCount);

        Status = vfatSetFCBNewDirName(DeviceExt, Child, FCB);
        if (!NT_SUCCESS(Status))
            continue;

        if (vfatFCBIsDirectory(Child))
        {
            VfatRenameChildFCB(DeviceExt, Child);
        }
    }
}

/*
 * FUNCTION: Set the file name information
 */
static
NTSTATUS
VfatSetRenameInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_RENAME_INFORMATION RenameInfo,
    PFILE_OBJECT TargetFileObject)
{
#ifdef NASSERTS_RENAME
#pragma push_macro("ASSERT")
#undef ASSERT
#define ASSERT(x) ((VOID) 0)
#endif
    NTSTATUS Status;
    UNICODE_STRING NewName;
    UNICODE_STRING SourcePath;
    UNICODE_STRING SourceFile;
    UNICODE_STRING NewPath;
    UNICODE_STRING NewFile;
    PFILE_OBJECT RootFileObject;
    PVFATFCB RootFCB;
    UNICODE_STRING RenameInfoString;
    PVFATFCB ParentFCB;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE TargetHandle;
    BOOLEAN DeletedTarget;
    ULONG OldReferences, NewReferences;
    PVFATFCB OldParent;

    DPRINT("VfatSetRenameInfo(%p, %p, %p, %p, %p)\n", FileObject, FCB, DeviceExt, RenameInfo, TargetFileObject);

    /* Disallow renaming root */
    if (vfatFCBIsRoot(FCB))
    {
        return STATUS_INVALID_PARAMETER;
    }

    OldReferences = FCB->parentFcb->RefCount;
#ifdef NASSERTS_RENAME
    UNREFERENCED_PARAMETER(OldReferences);
#endif

    /* If we are performing relative opening for rename, get FO for getting FCB and path name */
    if (RenameInfo->RootDirectory != NULL)
    {
        /* We cannot tolerate relative opening with a full path */
        if (RenameInfo->FileName[0] == L'\\')
        {
            return STATUS_OBJECT_NAME_INVALID;
        }

        Status = ObReferenceObjectByHandle(RenameInfo->RootDirectory,
                                           FILE_READ_DATA,
                                           *IoFileObjectType,
                                           ExGetPreviousMode(),
                                           (PVOID *)&RootFileObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        RootFCB = RootFileObject->FsContext;
    }

    RtlInitEmptyUnicodeString(&NewName, NULL, 0);
    ParentFCB = NULL;

    if (TargetFileObject == NULL)
    {
        /* If we don't have target file object, construct paths thanks to relative FCB, if any, and with
         * information supplied by the user
         */

        /* First, setup a string we'll work on */
        RenameInfoString.Length = RenameInfo->FileNameLength;
        RenameInfoString.MaximumLength = RenameInfo->FileNameLength;
        RenameInfoString.Buffer = RenameInfo->FileName;

        /* Check whether we have FQN */
        if (RenameInfoString.Length > 6 * sizeof(WCHAR))
        {
            if (RenameInfoString.Buffer[0] == L'\\' && RenameInfoString.Buffer[1] == L'?' &&
                RenameInfoString.Buffer[2] == L'?' && RenameInfoString.Buffer[3] == L'\\' &&
                RenameInfoString.Buffer[5] == L':' && (RenameInfoString.Buffer[4] >= L'A' &&
                RenameInfoString.Buffer[4] <= L'Z'))
            {
                /* If so, open its target directory */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &RenameInfoString,
                                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                           NULL, NULL);

                Status = IoCreateFile(&TargetHandle,
                                      FILE_WRITE_DATA | SYNCHRONIZE,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL, 0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      FILE_OPEN,
                                      FILE_OPEN_FOR_BACKUP_INTENT,
                                      NULL, 0,
                                      CreateFileTypeNone,
                                      NULL,
                                      IO_FORCE_ACCESS_CHECK | IO_OPEN_TARGET_DIRECTORY);
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                /* Get its FO to get the FCB */
                Status = ObReferenceObjectByHandle(TargetHandle,
                                                   FILE_WRITE_DATA,
                                                   *IoFileObjectType,
                                                   KernelMode,
                                                   (PVOID *)&TargetFileObject,
                                                   NULL);
                if (!NT_SUCCESS(Status))
                {
                    ZwClose(TargetHandle);
                    goto Cleanup;
                }

                /* Are we working on the same volume? */
                if (IoGetRelatedDeviceObject(TargetFileObject) != IoGetRelatedDeviceObject(FileObject))
                {
                    ObDereferenceObject(TargetFileObject);
                    ZwClose(TargetHandle);
                    TargetFileObject = NULL;
                    Status = STATUS_NOT_SAME_DEVICE;
                    goto Cleanup;
                }
            }
        }

        NewName.Length = 0;
        NewName.MaximumLength = RenameInfo->FileNameLength;
        if (RenameInfo->RootDirectory != NULL)
        {
            NewName.MaximumLength += sizeof(WCHAR) + RootFCB->PathNameU.Length;
        }
        else if (RenameInfo->FileName[0] != L'\\')
        {
            /* We don't have full path, and we don't have root directory:
             * => we move inside the same directory
             */
            NewName.MaximumLength += sizeof(WCHAR) + FCB->DirNameU.Length;
        }
        else if (TargetFileObject != NULL)
        {
            /* We had a FQN:
             * => we need to use its correct path
             */
            NewName.MaximumLength += sizeof(WCHAR) + ((PVFATFCB)TargetFileObject->FsContext)->PathNameU.Length;
        }

        NewName.Buffer = ExAllocatePoolWithTag(NonPagedPool, NewName.MaximumLength, TAG_NAME);
        if (NewName.Buffer == NULL)
        {
            if (TargetFileObject != NULL)
            {
                ObDereferenceObject(TargetFileObject);
                ZwClose(TargetHandle);
                TargetFileObject = NULL;
            }
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (RenameInfo->RootDirectory != NULL)
        {
            /* Here, copy first absolute and then append relative */ 
            RtlCopyUnicodeString(&NewName, &RootFCB->PathNameU);
            NewName.Buffer[NewName.Length / sizeof(WCHAR)] = L'\\';
            NewName.Length += sizeof(WCHAR);
            RtlAppendUnicodeStringToString(&NewName, &RenameInfoString);
        }
        else if (RenameInfo->FileName[0] != L'\\')
        {
            /* Here, copy first work directory and then append filename */
            RtlCopyUnicodeString(&NewName, &FCB->DirNameU);
            NewName.Buffer[NewName.Length / sizeof(WCHAR)] = L'\\';
            NewName.Length += sizeof(WCHAR);
            RtlAppendUnicodeStringToString(&NewName, &RenameInfoString);
        }
        else if (TargetFileObject != NULL)
        {
            /* Here, copy first path name and then append filename */
            RtlCopyUnicodeString(&NewName, &((PVFATFCB)TargetFileObject->FsContext)->PathNameU);
            NewName.Buffer[NewName.Length / sizeof(WCHAR)] = L'\\';
            NewName.Length += sizeof(WCHAR);
            RtlAppendUnicodeStringToString(&NewName, &RenameInfoString);
        }
        else
        {
            /* Here we should have full path, so simply copy it */
            RtlCopyUnicodeString(&NewName, &RenameInfoString);
        }

        /* Do we have to cleanup some stuff? */
        if (TargetFileObject != NULL)
        {
            ObDereferenceObject(TargetFileObject);
            ZwClose(TargetHandle);
            TargetFileObject = NULL;
        }
    }
    else
    {
        /* At that point, we shouldn't care about whether we are relative opening
         * Target FO FCB should already have full path
         */

        /* Before constructing string, just make a sanity check (just to be sure!) */
        if (IoGetRelatedDeviceObject(TargetFileObject) != IoGetRelatedDeviceObject(FileObject))
        {
            Status = STATUS_NOT_SAME_DEVICE;
            goto Cleanup;
        }

        NewName.Length = 0;
        NewName.MaximumLength = TargetFileObject->FileName.Length + ((PVFATFCB)TargetFileObject->FsContext)->PathNameU.Length + sizeof(WCHAR);
        NewName.Buffer = ExAllocatePoolWithTag(NonPagedPool, NewName.MaximumLength, TAG_NAME);
        if (NewName.Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyUnicodeString(&NewName, &((PVFATFCB)TargetFileObject->FsContext)->PathNameU);
        /* If \, it's already backslash terminated, don't add it */
        if (!vfatFCBIsRoot(TargetFileObject->FsContext))
        {
            NewName.Buffer[NewName.Length / sizeof(WCHAR)] = L'\\';
            NewName.Length += sizeof(WCHAR);
        }
        RtlAppendUnicodeStringToString(&NewName, &TargetFileObject->FileName);
    }

    /* Explode our paths to get path & filename */
    vfatSplitPathName(&FCB->PathNameU, &SourcePath, &SourceFile);
    DPRINT("Old dir: %wZ, Old file: %wZ\n", &SourcePath, &SourceFile);
    vfatSplitPathName(&NewName, &NewPath, &NewFile);
    DPRINT("New dir: %wZ, New file: %wZ\n", &NewPath, &NewFile);

    if (IsDotOrDotDot(&NewFile))
    {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Cleanup;
    }

    if (vfatFCBIsDirectory(FCB) && !IsListEmpty(&FCB->ParentListHead))
    {
        if (IsThereAChildOpened(FCB))
        {
            Status = STATUS_ACCESS_DENIED;
            ASSERT(OldReferences == FCB->parentFcb->RefCount);
            goto Cleanup;
        }
    }

    /* Are we working in place? */
    if (FsRtlAreNamesEqual(&SourcePath, &NewPath, TRUE, NULL))
    {
        if (FsRtlAreNamesEqual(&SourceFile, &NewFile, FALSE, NULL))
        {
            Status = STATUS_SUCCESS;
            ASSERT(OldReferences == FCB->parentFcb->RefCount);
            goto Cleanup;
        }

        if (FsRtlAreNamesEqual(&SourceFile, &NewFile, TRUE, NULL))
        {
            vfatReportChange(DeviceExt,
                             FCB,
                             (vfatFCBIsDirectory(FCB) ?
                              FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                             FILE_ACTION_RENAMED_OLD_NAME);
            Status = vfatRenameEntry(DeviceExt, FCB, &NewFile, TRUE);
            if (NT_SUCCESS(Status))
            {
                vfatReportChange(DeviceExt,
                                 FCB,
                                 (vfatFCBIsDirectory(FCB) ?
                                  FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                                 FILE_ACTION_RENAMED_NEW_NAME);
            }
        }
        else
        {
            /* Try to find target */
            ParentFCB = FCB->parentFcb;
            vfatGrabFCB(DeviceExt, ParentFCB);
            Status = vfatPrepareTargetForRename(DeviceExt,
                                                &ParentFCB,
                                                &NewFile,
                                                RenameInfo->ReplaceIfExists,
                                                &NewPath,
                                                &DeletedTarget);
            if (!NT_SUCCESS(Status))
            {
                ASSERT(OldReferences == FCB->parentFcb->RefCount - 1);
                ASSERT(OldReferences == ParentFCB->RefCount - 1);
                goto Cleanup;
            }

            vfatReportChange(DeviceExt,
                             FCB,
                             (vfatFCBIsDirectory(FCB) ?
                              FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                             (DeletedTarget ? FILE_ACTION_REMOVED : FILE_ACTION_RENAMED_OLD_NAME));
            Status = vfatRenameEntry(DeviceExt, FCB, &NewFile, FALSE);
            if (NT_SUCCESS(Status))
            {
                if (DeletedTarget)
                {
                    vfatReportChange(DeviceExt,
                                     FCB,
                                     FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE
                                     | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_EA,
                                     FILE_ACTION_MODIFIED);
                }
                else
                {
                    vfatReportChange(DeviceExt,
                                     FCB,
                                     (vfatFCBIsDirectory(FCB) ?
                                      FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                                     FILE_ACTION_RENAMED_NEW_NAME);
                }
            }
        }

        ASSERT(OldReferences == FCB->parentFcb->RefCount - 1); // extra grab
        ASSERT(OldReferences == ParentFCB->RefCount - 1); // extra grab
    }
    else
    {

        /* Try to find target */
        ParentFCB = NULL;
        OldParent = FCB->parentFcb;
#ifdef NASSERTS_RENAME
        UNREFERENCED_PARAMETER(OldParent);
#endif
        Status = vfatPrepareTargetForRename(DeviceExt,
                                            &ParentFCB,
                                            &NewName,
                                            RenameInfo->ReplaceIfExists,
                                            &NewPath,
                                            &DeletedTarget);
        if (!NT_SUCCESS(Status))
        {
            ASSERT(OldReferences == FCB->parentFcb->RefCount);
            goto Cleanup;
        }

        NewReferences = ParentFCB->RefCount;
#ifdef NASSERTS_RENAME
        UNREFERENCED_PARAMETER(NewReferences);
#endif

        vfatReportChange(DeviceExt,
                         FCB,
                         (vfatFCBIsDirectory(FCB) ?
                          FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                         FILE_ACTION_REMOVED);
        Status = VfatMoveEntry(DeviceExt, FCB, &NewFile, ParentFCB);
        if (NT_SUCCESS(Status))
        {
            if (DeletedTarget)
            {
                vfatReportChange(DeviceExt,
                                 FCB,
                                 FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE
                                 | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_EA,
                                 FILE_ACTION_MODIFIED);
            }
            else
            {
                vfatReportChange(DeviceExt,
                                 FCB,
                                 (vfatFCBIsDirectory(FCB) ?
                                  FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                                 FILE_ACTION_ADDED);
            }
        }
    }

    if (NT_SUCCESS(Status) && vfatFCBIsDirectory(FCB))
    {
        VfatRenameChildFCB(DeviceExt, FCB);
    }

    ASSERT(OldReferences == OldParent->RefCount + 1); // removed file
    ASSERT(NewReferences == ParentFCB->RefCount - 1); // new file
Cleanup:
    if (ParentFCB != NULL) vfatReleaseFCB(DeviceExt, ParentFCB);
    if (NewName.Buffer != NULL) ExFreePoolWithTag(NewName.Buffer, TAG_NAME);
    if (RenameInfo->RootDirectory != NULL) ObDereferenceObject(RootFileObject);

    return Status;
#ifdef NASSERTS_RENAME
#pragma pop_macro("ASSERT")
#endif
}

/*
 * FUNCTION: Retrieve the file name information
 */
static
NTSTATUS
VfatGetNameInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB FCB,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_NAME_INFORMATION NameInfo,
    PULONG BufferLength)
{
    ULONG BytesToCopy;

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(DeviceExt);

    ASSERT(NameInfo != NULL);
    ASSERT(FCB != NULL);

    /* If buffer can't hold at least the file name length, bail out */
    if (*BufferLength < (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
        return STATUS_BUFFER_OVERFLOW;

    /* Save file name length, and as much file len, as buffer length allows */
    NameInfo->FileNameLength = FCB->PathNameU.Length;

    /* Calculate amount of bytes to copy not to overflow the buffer */
    BytesToCopy = min(FCB->PathNameU.Length,
                      *BufferLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

    /* Fill in the bytes */
    RtlCopyMemory(NameInfo->FileName, FCB->PathNameU.Buffer, BytesToCopy);

    /* Check if we could write more but are not able to */
    if (*BufferLength < FCB->PathNameU.Length + (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    {
        /* Return number of bytes written */
        *BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* We filled up as many bytes, as needed */
    *BufferLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + FCB->PathNameU.Length);

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetInternalInformation(
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_INTERNAL_INFORMATION InternalInfo,
    PULONG BufferLength)
{
    ASSERT(InternalInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    InternalInfo->IndexNumber.QuadPart = (LONGLONG)vfatDirEntryGetFirstCluster(DeviceExt, &Fcb->entry) * DeviceExt->FatInfo.BytesPerCluster;

    *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);
    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the file network open information
 */
static
NTSTATUS
VfatGetNetworkOpenInformation(
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
    PULONG BufferLength)
{
    ASSERT(NetworkInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
        return(STATUS_BUFFER_OVERFLOW);

    if (vfatVolumeIsFatX(DeviceExt))
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.FatX.CreationDate,
                                   Fcb->entry.FatX.CreationTime,
                                   &NetworkInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.FatX.AccessDate,
                                   Fcb->entry.FatX.AccessTime,
                                   &NetworkInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.FatX.UpdateDate,
                                   Fcb->entry.FatX.UpdateTime,
                                   &NetworkInfo->LastWriteTime);
        NetworkInfo->ChangeTime.QuadPart = NetworkInfo->LastWriteTime.QuadPart;
    }
    else
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.Fat.CreationDate,
                                   Fcb->entry.Fat.CreationTime,
                                   &NetworkInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.Fat.AccessDate,
                                   0,
                                   &NetworkInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   Fcb->entry.Fat.UpdateDate,
                                   Fcb->entry.Fat.UpdateTime,
                                   &NetworkInfo->LastWriteTime);
        NetworkInfo->ChangeTime.QuadPart = NetworkInfo->LastWriteTime.QuadPart;
    }

    if (vfatFCBIsDirectory(Fcb))
    {
        NetworkInfo->EndOfFile.QuadPart = 0L;
        NetworkInfo->AllocationSize.QuadPart = 0L;
    }
    else
    {
        NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;
        NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    }

    NetworkInfo->FileAttributes = *Fcb->Attributes & 0x3f;
    /* Synthesize FILE_ATTRIBUTE_NORMAL */
    if (0 == (NetworkInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                             FILE_ATTRIBUTE_ARCHIVE |
                                             FILE_ATTRIBUTE_SYSTEM |
                                             FILE_ATTRIBUTE_HIDDEN |
                                             FILE_ATTRIBUTE_READONLY)))
    {
        DPRINT("Synthesizing FILE_ATTRIBUTE_NORMAL\n");
        NetworkInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
    }

    *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    return STATUS_SUCCESS;
}


static
NTSTATUS
VfatGetEaInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_EA_INFORMATION Info,
    PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Fcb);

    /* FIXME - use SEH to access the buffer! */
    Info->EaSize = 0;
    *BufferLength -= sizeof(*Info);
    if (DeviceExt->FatInfo.FatType == FAT12 ||
        DeviceExt->FatInfo.FatType == FAT16)
    {
        /* FIXME */
        DPRINT1("VFAT: FileEaInformation not implemented!\n");
    }
    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the all file information
 */
static
NTSTATUS
VfatGetAllInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_ALL_INFORMATION Info,
    PULONG BufferLength)
{
    NTSTATUS Status;

    ASSERT(Info);
    ASSERT(Fcb);

    if (*BufferLength < FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName))
        return STATUS_BUFFER_OVERFLOW;

    *BufferLength -= (sizeof(FILE_ACCESS_INFORMATION) + sizeof(FILE_MODE_INFORMATION) + sizeof(FILE_ALIGNMENT_INFORMATION));

    /* Basic Information */
    Status = VfatGetBasicInformation(FileObject, Fcb, DeviceExt, &Info->BasicInformation, BufferLength);
    if (!NT_SUCCESS(Status)) return Status;
    /* Standard Information */
    Status = VfatGetStandardInformation(Fcb, &Info->StandardInformation, BufferLength);
    if (!NT_SUCCESS(Status)) return Status;
    /* Internal Information */
    Status = VfatGetInternalInformation(Fcb, DeviceExt, &Info->InternalInformation, BufferLength);
    if (!NT_SUCCESS(Status)) return Status;
    /* EA Information */
    Status = VfatGetEaInformation(FileObject, Fcb, DeviceExt, &Info->EaInformation, BufferLength);
    if (!NT_SUCCESS(Status)) return Status;
    /* Position Information */
    Status = VfatGetPositionInformation(FileObject, Fcb, DeviceExt, &Info->PositionInformation, BufferLength);
    if (!NT_SUCCESS(Status)) return Status;
    /* Name Information */
    Status = VfatGetNameInformation(FileObject, Fcb, DeviceExt, &Info->NameInformation, BufferLength);

    return Status;
}

static
VOID
UpdateFileSize(
    PFILE_OBJECT FileObject,
    PVFATFCB Fcb,
    ULONG Size,
    ULONG ClusterSize,
    BOOLEAN IsFatX)
{
    if (Size > 0)
    {
        Fcb->RFCB.AllocationSize.QuadPart = ROUND_UP_64(Size, ClusterSize);
    }
    else
    {
        Fcb->RFCB.AllocationSize.QuadPart = (LONGLONG)0;
    }
    if (!vfatFCBIsDirectory(Fcb))
    {
        if (IsFatX)
            Fcb->entry.FatX.FileSize = Size;
        else
            Fcb->entry.Fat.FileSize = Size;
    }
    Fcb->RFCB.FileSize.QuadPart = Size;
    Fcb->RFCB.ValidDataLength.QuadPart = Size;

    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
}

NTSTATUS
VfatSetAllocationSizeInformation(
    PFILE_OBJECT FileObject,
    PVFATFCB Fcb,
    PDEVICE_EXTENSION DeviceExt,
    PLARGE_INTEGER AllocationSize)
{
    ULONG OldSize;
    ULONG Cluster, FirstCluster;
    NTSTATUS Status;

    ULONG ClusterSize = DeviceExt->FatInfo.BytesPerCluster;
    ULONG NewSize = AllocationSize->u.LowPart;
    ULONG NCluster;
    BOOLEAN AllocSizeChanged = FALSE, IsFatX = vfatVolumeIsFatX(DeviceExt);

    DPRINT("VfatSetAllocationSizeInformation(File <%wZ>, AllocationSize %d %u)\n",
           &Fcb->PathNameU, AllocationSize->HighPart, AllocationSize->LowPart);

    if (IsFatX)
        OldSize = Fcb->entry.FatX.FileSize;
    else
        OldSize = Fcb->entry.Fat.FileSize;

    if (AllocationSize->u.HighPart > 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (OldSize == NewSize)
    {
        return STATUS_SUCCESS;
    }

    FirstCluster = vfatDirEntryGetFirstCluster(DeviceExt, &Fcb->entry);

    if (NewSize > Fcb->RFCB.AllocationSize.u.LowPart)
    {
        AllocSizeChanged = TRUE;
        if (FirstCluster == 0)
        {
            Fcb->LastCluster = Fcb->LastOffset = 0;
            Status = NextCluster(DeviceExt, FirstCluster, &FirstCluster, TRUE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NextCluster failed. Status = %x\n", Status);
                return Status;
            }

            if (FirstCluster == 0xffffffff)
            {
                return STATUS_DISK_FULL;
            }

            Status = OffsetToCluster(DeviceExt, FirstCluster,
                                     ROUND_DOWN(NewSize - 1, ClusterSize),
                                     &NCluster, TRUE);
            if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
            {
                /* disk is full */
                NCluster = Cluster = FirstCluster;
                Status = STATUS_SUCCESS;
                while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
                {
                    Status = NextCluster(DeviceExt, FirstCluster, &NCluster, FALSE);
                    WriteCluster(DeviceExt, Cluster, 0);
                    Cluster = NCluster;
                }
                return STATUS_DISK_FULL;
            }

            if (IsFatX)
            {
                Fcb->entry.FatX.FirstCluster = FirstCluster;
            }
            else
            {
                if (DeviceExt->FatInfo.FatType == FAT32)
                {
                    Fcb->entry.Fat.FirstCluster = (unsigned short)(FirstCluster & 0x0000FFFF);
                    Fcb->entry.Fat.FirstClusterHigh = FirstCluster >> 16;
                }
                else
                {
                    ASSERT((FirstCluster >> 16) == 0);
                    Fcb->entry.Fat.FirstCluster = (unsigned short)(FirstCluster & 0x0000FFFF);
                }
            }
        }
        else
        {
            if (Fcb->LastCluster > 0)
            {
                if (Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize == Fcb->LastOffset)
                {
                    Cluster = Fcb->LastCluster;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = OffsetToCluster(DeviceExt, Fcb->LastCluster,
                                             Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize - Fcb->LastOffset,
                                             &Cluster, FALSE);
                }
            }
            else
            {
                Status = OffsetToCluster(DeviceExt, FirstCluster,
                                         Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize,
                                         &Cluster, FALSE);
            }

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            Fcb->LastCluster = Cluster;
            Fcb->LastOffset = Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize;

            /* FIXME: Check status */
            /* Cluster points now to the last cluster within the chain */
            Status = OffsetToCluster(DeviceExt, Cluster,
                                     ROUND_DOWN(NewSize - 1, ClusterSize) - Fcb->LastOffset,
                                     &NCluster, TRUE);
            if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
            {
                /* disk is full */
                NCluster = Cluster;
                Status = NextCluster(DeviceExt, FirstCluster, &NCluster, FALSE);
                WriteCluster(DeviceExt, Cluster, 0xffffffff);
                Cluster = NCluster;
                while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
                {
                    Status = NextCluster(DeviceExt, FirstCluster, &NCluster, FALSE);
                    WriteCluster(DeviceExt, Cluster, 0);
                    Cluster = NCluster;
                }
                return STATUS_DISK_FULL;
            }
        }
        UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize, vfatVolumeIsFatX(DeviceExt));
    }
    else if (NewSize + ClusterSize <= Fcb->RFCB.AllocationSize.u.LowPart)
    {
        DPRINT("Check for the ability to set file size\n");
        if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer,
                                  (PLARGE_INTEGER)AllocationSize))
        {
            DPRINT("Couldn't set file size!\n");
            return STATUS_USER_MAPPED_FILE;
        }
        DPRINT("Can set file size\n");

        AllocSizeChanged = TRUE;
        /* FIXME: Use the cached cluster/offset better way. */
        Fcb->LastCluster = Fcb->LastOffset = 0;
        UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize, vfatVolumeIsFatX(DeviceExt));
        if (NewSize > 0)
        {
            Status = OffsetToCluster(DeviceExt, FirstCluster,
                                     ROUND_DOWN(NewSize - 1, ClusterSize),
                                     &Cluster, FALSE);

            NCluster = Cluster;
            Status = NextCluster(DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster(DeviceExt, Cluster, 0xffffffff);
            Cluster = NCluster;
        }
        else
        {
            if (IsFatX)
            {
                Fcb->entry.FatX.FirstCluster = 0;
            }
            else
            {
                if (DeviceExt->FatInfo.FatType == FAT32)
                {
                    Fcb->entry.Fat.FirstCluster = 0;
                    Fcb->entry.Fat.FirstClusterHigh = 0;
                }
                else
                {
                    Fcb->entry.Fat.FirstCluster = 0;
                }
            }

            NCluster = Cluster = FirstCluster;
            Status = STATUS_SUCCESS;
        }

        while (NT_SUCCESS(Status) && 0xffffffff != Cluster && Cluster > 1)
        {
            Status = NextCluster(DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster(DeviceExt, Cluster, 0);
            Cluster = NCluster;
        }

        if (DeviceExt->FatInfo.FatType == FAT32)
        {
            FAT32UpdateFreeClustersCount(DeviceExt);
        }
    }
    else
    {
        UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize, vfatVolumeIsFatX(DeviceExt));
    }

    /* Update the on-disk directory entry */
    Fcb->Flags |= FCB_IS_DIRTY;
    if (AllocSizeChanged)
    {
        VfatUpdateEntry(DeviceExt, Fcb);

        vfatReportChange(DeviceExt, Fcb, FILE_NOTIFY_CHANGE_SIZE, FILE_ACTION_MODIFIED);
    }
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Retrieve the specified file information
 */
NTSTATUS
VfatQueryInformation(
    PVFAT_IRP_CONTEXT IrpContext)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    PVFATFCB FCB;

    NTSTATUS Status = STATUS_SUCCESS;
    PVOID SystemBuffer;
    ULONG BufferLength;

    /* PRECONDITION */
    ASSERT(IrpContext);

    /* INITIALIZATION */
    FileInformationClass = IrpContext->Stack->Parameters.QueryFile.FileInformationClass;
    FCB = (PVFATFCB) IrpContext->FileObject->FsContext;

    DPRINT("VfatQueryInformation is called for '%s'\n",
           FileInformationClass >= FileMaximumInformation - 1 ? "????" : FileInformationClassNames[FileInformationClass]);

    if (FCB == NULL)
    {
        DPRINT1("IRP_MJ_QUERY_INFORMATION without FCB!\n");
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;
    BufferLength = IrpContext->Stack->Parameters.QueryFile.Length;

    if (!BooleanFlagOn(FCB->Flags, FCB_IS_PAGE_FILE))
    {
        if (!ExAcquireResourceSharedLite(&FCB->MainResource,
                                         BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
        {
            return VfatMarkIrpContextForQueue(IrpContext);
        }
    }

    switch (FileInformationClass)
    {
        case FileStandardInformation:
            Status = VfatGetStandardInformation(FCB,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FilePositionInformation:
            Status = VfatGetPositionInformation(IrpContext->FileObject,
                                                FCB,
                                                IrpContext->DeviceExt,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileBasicInformation:
            Status = VfatGetBasicInformation(IrpContext->FileObject,
                                             FCB,
                                             IrpContext->DeviceExt,
                                             SystemBuffer,
                                             &BufferLength);
            break;

        case FileNameInformation:
            Status = VfatGetNameInformation(IrpContext->FileObject,
                                            FCB,
                                            IrpContext->DeviceExt,
                                            SystemBuffer,
                                            &BufferLength);
            break;

        case FileInternalInformation:
            Status = VfatGetInternalInformation(FCB,
                                                IrpContext->DeviceExt,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileNetworkOpenInformation:
            Status = VfatGetNetworkOpenInformation(FCB,
                                                   IrpContext->DeviceExt,
                                                   SystemBuffer,
                                                   &BufferLength);
            break;

        case FileAllInformation:
            Status = VfatGetAllInformation(IrpContext->FileObject,
                                           FCB,
                                           IrpContext->DeviceExt,
                                           SystemBuffer,
                                           &BufferLength);
            break;

        case FileEaInformation:
            Status = VfatGetEaInformation(IrpContext->FileObject,
                                          FCB,
                                          IrpContext->DeviceExt,
                                          SystemBuffer,
                                          &BufferLength);
            break;

        case FileAlternateNameInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
    }

    if (!BooleanFlagOn(FCB->Flags, FCB_IS_PAGE_FILE))
    {
        ExReleaseResourceLite(&FCB->MainResource);
    }

    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
        IrpContext->Irp->IoStatus.Information =
            IrpContext->Stack->Parameters.QueryFile.Length - BufferLength;
    else
        IrpContext->Irp->IoStatus.Information = 0;

    return Status;
}

/*
 * FUNCTION: Retrieve the specified file information
 */
NTSTATUS
VfatSetInformation(
    PVFAT_IRP_CONTEXT IrpContext)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    PVFATFCB FCB;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID SystemBuffer;
    BOOLEAN LockDir;

    /* PRECONDITION */
    ASSERT(IrpContext);

    DPRINT("VfatSetInformation(IrpContext %p)\n", IrpContext);

    /* INITIALIZATION */
    FileInformationClass =
        IrpContext->Stack->Parameters.SetFile.FileInformationClass;
    FCB = (PVFATFCB) IrpContext->FileObject->FsContext;
    SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;

    DPRINT("VfatSetInformation is called for '%s'\n",
           FileInformationClass >= FileMaximumInformation - 1 ? "????" : FileInformationClassNames[ FileInformationClass]);

    DPRINT("FileInformationClass %d\n", FileInformationClass);
    DPRINT("SystemBuffer %p\n", SystemBuffer);

    if (FCB == NULL)
    {
        DPRINT1("IRP_MJ_SET_INFORMATION without FCB!\n");
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* Special: We should call MmCanFileBeTruncated here to determine if changing
       the file size would be allowed.  If not, we bail with the right error.
       We must do this before acquiring the lock. */
    if (FileInformationClass == FileEndOfFileInformation)
    {
        DPRINT("Check for the ability to set file size\n");
        if (!MmCanFileBeTruncated(IrpContext->FileObject->SectionObjectPointer,
                                  (PLARGE_INTEGER)SystemBuffer))
        {
            DPRINT("Couldn't set file size!\n");
            IrpContext->Irp->IoStatus.Information = 0;
            return STATUS_USER_MAPPED_FILE;
        }
        DPRINT("Can set file size\n");
    }

    LockDir = FALSE;
    if (FileInformationClass == FileRenameInformation || FileInformationClass == FileAllocationInformation ||
        FileInformationClass == FileEndOfFileInformation || FileInformationClass == FileBasicInformation)
    {
        LockDir = TRUE;
    }

    if (LockDir)
    {
        if (!ExAcquireResourceExclusiveLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource,
                                            BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
        {
            return VfatMarkIrpContextForQueue(IrpContext);
        }
    }

    if (!BooleanFlagOn(FCB->Flags, FCB_IS_PAGE_FILE))
    {
        if (!ExAcquireResourceExclusiveLite(&FCB->MainResource,
                                            BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
        {
            if (LockDir)
            {
                ExReleaseResourceLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource);
            }

            return VfatMarkIrpContextForQueue(IrpContext);
        }
    }

    switch (FileInformationClass)
    {
        case FilePositionInformation:
            Status = VfatSetPositionInformation(IrpContext->FileObject,
                                                SystemBuffer);
            break;

        case FileDispositionInformation:
            Status = VfatSetDispositionInformation(IrpContext->FileObject,
                                                   FCB,
                                                   IrpContext->DeviceExt,
                                                   SystemBuffer);
            break;

        case FileAllocationInformation:
        case FileEndOfFileInformation:
            Status = VfatSetAllocationSizeInformation(IrpContext->FileObject,
                                                      FCB,
                                                      IrpContext->DeviceExt,
                                                      (PLARGE_INTEGER)SystemBuffer);
            break;

        case FileBasicInformation:
            Status = VfatSetBasicInformation(IrpContext->FileObject,
                                             FCB,
                                             IrpContext->DeviceExt,
                                             SystemBuffer);
            break;

        case FileRenameInformation:
            Status = VfatSetRenameInformation(IrpContext->FileObject,
                                              FCB,
                                              IrpContext->DeviceExt,
                                              SystemBuffer,
                                              IrpContext->Stack->Parameters.SetFile.FileObject);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    if (!BooleanFlagOn(FCB->Flags, FCB_IS_PAGE_FILE))
    {
        ExReleaseResourceLite(&FCB->MainResource);
    }

    if (LockDir)
    {
        ExReleaseResourceLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource);
    }

    IrpContext->Irp->IoStatus.Information = 0;
    return Status;
}

/* EOF */
