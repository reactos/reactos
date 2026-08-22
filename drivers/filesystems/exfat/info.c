/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     File and volume information operations
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS
ExFatQueryNameInformation(
    PEXFAT_FCB Fcb,
    PFILE_NAME_INFORMATION Information,
    ULONG Length,
    PULONG Written)
{
    ULONG HeaderLength = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
    ULONG CopyLength;

    if (Length < HeaderLength)
        return STATUS_BUFFER_TOO_SMALL;

    Information->FileNameLength = Fcb->PathName.Length;
    CopyLength = min((ULONG)Fcb->PathName.Length, Length - HeaderLength);
    if (CopyLength)
        RtlCopyMemory(Information->FileName, Fcb->PathName.Buffer, CopyLength);
    *Written = HeaderLength + CopyLength;
    return (CopyLength == Fcb->PathName.Length) ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
}

static VOID
ExFatFillBasicInformation(
    PEXFAT_FCB Fcb,
    PFILE_BASIC_INFORMATION Information)
{
    RtlZeroMemory(Information, sizeof(*Information));
    Information->CreationTime = Fcb->CreationTime;
    Information->LastAccessTime = Fcb->LastAccessTime;
    Information->LastWriteTime = Fcb->LastWriteTime;
    Information->ChangeTime = Fcb->ChangeTime;
    Information->FileAttributes = Fcb->FileAttributes;
}

static VOID
ExFatFillStandardInformation(
    PEXFAT_FCB Fcb,
    PFILE_STANDARD_INFORMATION Information)
{
    RtlZeroMemory(Information, sizeof(*Information));
    Information->AllocationSize = Fcb->Header.AllocationSize;
    Information->EndOfFile = Fcb->Header.FileSize;
    Information->NumberOfLinks = 1;
    Information->DeletePending = Fcb->DeletePending;
    Information->Directory = Fcb->IsDirectory;
}

static VOID
ExFatFillNetworkInformation(
    PEXFAT_FCB Fcb,
    PFILE_NETWORK_OPEN_INFORMATION Information)
{
    RtlZeroMemory(Information, sizeof(*Information));
    Information->CreationTime = Fcb->CreationTime;
    Information->LastAccessTime = Fcb->LastAccessTime;
    Information->LastWriteTime = Fcb->LastWriteTime;
    Information->ChangeTime = Fcb->ChangeTime;
    Information->AllocationSize = Fcb->Header.AllocationSize;
    Information->EndOfFile = Fcb->Header.FileSize;
    Information->FileAttributes = Fcb->FileAttributes;
}

NTSTATUS
ExFatQueryInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_FCB Fcb;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG Length = Stack->Parameters.QueryFile.Length;
    ULONG Written = 0;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject || !Buffer)
        return STATUS_INVALID_DEVICE_REQUEST;
    Fcb = FileObject->FsContext;
    if (!Fcb)
        return STATUS_INVALID_HANDLE;

    ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);
    RtlZeroMemory(Buffer, Length);

    switch (Stack->Parameters.QueryFile.FileInformationClass)
    {
        case FileBasicInformation:
            if (Length < sizeof(FILE_BASIC_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ExFatFillBasicInformation(Fcb, Buffer);
                Written = sizeof(FILE_BASIC_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileStandardInformation:
            if (Length < sizeof(FILE_STANDARD_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ExFatFillStandardInformation(Fcb, Buffer);
                Written = sizeof(FILE_STANDARD_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileInternalInformation:
            if (Length < sizeof(FILE_INTERNAL_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ((PFILE_INTERNAL_INFORMATION)Buffer)->IndexNumber.QuadPart = Fcb->IndexNumber;
                Written = sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileEaInformation:
            if (Length < sizeof(FILE_EA_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ((PFILE_EA_INFORMATION)Buffer)->EaSize = 0;
                Written = sizeof(FILE_EA_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FilePositionInformation:
            if (Length < sizeof(FILE_POSITION_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ((PFILE_POSITION_INFORMATION)Buffer)->CurrentByteOffset = FileObject->CurrentByteOffset;
                Written = sizeof(FILE_POSITION_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileNameInformation:
#if (NTDDI_VERSION >= NTDDI_VISTA)
        case FileNormalizedNameInformation:
#endif
            Status = ExFatQueryNameInformation(Fcb, Buffer, Length, &Written);
            break;

        case FileNetworkOpenInformation:
            if (Length < sizeof(FILE_NETWORK_OPEN_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ExFatFillNetworkInformation(Fcb, Buffer);
                Written = sizeof(FILE_NETWORK_OPEN_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileAttributeTagInformation:
            if (Length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ((PFILE_ATTRIBUTE_TAG_INFORMATION)Buffer)->FileAttributes = Fcb->FileAttributes;
                ((PFILE_ATTRIBUTE_TAG_INFORMATION)Buffer)->ReparseTag = 0;
                Written = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileStreamInformation:
        {
            static const WCHAR StreamName[] = L"::$DATA";
            PFILE_STREAM_INFORMATION Stream = Buffer;
            ULONG NameLength = sizeof(StreamName) - sizeof(UNICODE_NULL);
            ULONG Required = FIELD_OFFSET(FILE_STREAM_INFORMATION, StreamName) + NameLength;

            if (Fcb->IsDirectory)
            {
                Status = STATUS_SUCCESS;
            }
            else if (Length < Required)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                Stream->NextEntryOffset = 0;
                Stream->StreamNameLength = NameLength;
                Stream->StreamSize = Fcb->Header.FileSize;
                Stream->StreamAllocationSize = Fcb->Header.AllocationSize;
                RtlCopyMemory(Stream->StreamName, StreamName, NameLength);
                Written = Required;
                Status = STATUS_SUCCESS;
            }
            break;
        }

        case FileAllInformation:
        {
            PFILE_ALL_INFORMATION All = Buffer;
            ULONG FixedLength = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName);
            ULONG NameCopy;

            if (Length < FixedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            ExFatFillBasicInformation(Fcb, &All->BasicInformation);
            ExFatFillStandardInformation(Fcb, &All->StandardInformation);
            All->InternalInformation.IndexNumber.QuadPart = Fcb->IndexNumber;
            All->EaInformation.EaSize = 0;
            All->AccessInformation.AccessFlags = 0;
            All->PositionInformation.CurrentByteOffset = FileObject->CurrentByteOffset;
            All->ModeInformation.Mode = 0;
            All->AlignmentInformation.AlignmentRequirement = Fcb->Vcb->DeviceObject->AlignmentRequirement;
            All->NameInformation.FileNameLength = Fcb->PathName.Length;
            NameCopy = min((ULONG)Fcb->PathName.Length, Length - FixedLength);
            RtlCopyMemory(All->NameInformation.FileName, Fcb->PathName.Buffer, NameCopy);
            Written = FixedLength + NameCopy;
            Status = (NameCopy == Fcb->PathName.Length) ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
            break;
        }

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    ExReleaseResourceLite(&Fcb->MainResource);
    Irp->IoStatus.Information = Written;
    return Status;
}

static NTSTATUS
ExFatSetBasicInformation(
    PEXFAT_FCB Fcb,
    PFILE_BASIC_INFORMATION Information)
{
    FILINFO FatInformation;
    BYTE Attributes;
    FRESULT Result = FR_OK;

    RtlZeroMemory(&FatInformation, sizeof(FatInformation));
    ExFatAcquireFatFs(Fcb->Vcb);
    if (Information->FileAttributes)
    {
        Attributes = ExFatNtAttributesToFat(Information->FileAttributes);
        Result = f_chmod(Fcb->FatPath, Attributes, AM_RDO | AM_HID | AM_SYS | AM_ARC);
    }
    if (Result == FR_OK && Information->LastWriteTime.QuadPart)
    {
        ExFatSystemTimeToFatTime(&Information->LastWriteTime,
                                 &FatInformation.fdate,
                                 &FatInformation.ftime);
        if (Information->CreationTime.QuadPart)
        {
            ExFatSystemTimeToFatTime(&Information->CreationTime,
                                     &FatInformation.crdate,
                                     &FatInformation.crtime);
        }
        Result = f_utime(Fcb->FatPath, &FatInformation);
    }
    ExFatReleaseFatFs(Fcb->Vcb);

    if (Result == FR_OK)
    {
        if (Information->FileAttributes)
        {
            /* Mirror what f_chmod actually wrote, through the shared mapping. */
            Fcb->FileAttributes =
                ExFatFatAttributesToNt(Attributes |
                                       (Fcb->IsDirectory ? AM_DIR : 0));
        }
        if (Information->CreationTime.QuadPart)
            Fcb->CreationTime = Information->CreationTime;
        if (Information->LastAccessTime.QuadPart)
            Fcb->LastAccessTime = Information->LastAccessTime;
        if (Information->LastWriteTime.QuadPart)
            Fcb->LastWriteTime = Information->LastWriteTime;
        if (Information->ChangeTime.QuadPart)
            Fcb->ChangeTime = Information->ChangeTime;
    }
    return ExFatMapResult(Result);
}

static NTSTATUS
ExFatSetEndOfFile(
    PEXFAT_FCB Fcb,
    PEXFAT_CCB Ccb,
    PLARGE_INTEGER EndOfFile)
{
    FRESULT Result;
    LARGE_INTEGER OldFileSize;
    IO_STATUS_BLOCK IoStatus;

    if (EndOfFile->QuadPart < 0)
        return STATUS_INVALID_PARAMETER;
    if (!(Ccb->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
        return STATUS_ACCESS_DENIED;
    if (EndOfFile->QuadPart < Fcb->Header.FileSize.QuadPart &&
        !MmCanFileBeTruncated(&Fcb->SectionObjectPointers, EndOfFile))
    {
        return STATUS_USER_MAPPED_FILE;
    }

    OldFileSize = Fcb->Header.FileSize;
    if (Fcb->SectionObjectPointers.DataSectionObject)
    {
        CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &IoStatus);
        if (!NT_SUCCESS(IoStatus.Status))
            return IoStatus.Status;
    }

    /* No paging I/O may be in flight while clusters are released. */
    ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
    ExFatAcquireFatFs(Fcb->Vcb);
    Result = ExFatEnsureFcbFile(Fcb, TRUE);
    if (Result == FR_OK)
    {
        if ((FSIZE_t)EndOfFile->QuadPart > f_size(&Fcb->FatFile))
        {
            Result = ExFatZeroFileRange(Fcb,
                                        f_size(&Fcb->FatFile),
                                        (FSIZE_t)EndOfFile->QuadPart);
        }
        else
        {
            Result = f_lseek(&Fcb->FatFile, (FSIZE_t)EndOfFile->QuadPart);
        }
    }
    if (Result == FR_OK)
        Result = f_truncate(&Fcb->FatFile);
    if (Result == FR_OK)
        Result = f_sync(&Fcb->FatFile);
    if (Result == FR_OK)
    {
        Fcb->Header.FileSize = *EndOfFile;
        Fcb->Header.ValidDataLength = *EndOfFile;
        Fcb->Header.AllocationSize.QuadPart = ExFatRoundUp(EndOfFile->QuadPart,
                                                           Fcb->Vcb->BytesPerCluster);
    }
    ExFatReleaseFatFs(Fcb->Vcb);
    ExReleaseResourceLite(&Fcb->PagingIoResource);

    if (Result == FR_OK)
    {
        /* The size report belongs here, next to the size change. The
         * matching index refresh does not: it needs Vcb->Resource, which
         * must never be taken under an FCB resource, so ExFatSetInformation
         * performs it once this FCB's resources are released. */
        ExFatReportChange(Fcb->Vcb,
                          &Fcb->PathName,
                          FILE_NOTIFY_CHANGE_SIZE,
                          FILE_ACTION_MODIFIED);
    }

    if (Result == FR_OK && Fcb->SectionObjectPointers.SharedCacheMap)
    {
        CcSetFileSizes(Ccb->FileObject,
                       (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
        if (EndOfFile->QuadPart < OldFileSize.QuadPart)
        {
            CcPurgeCacheSection(&Fcb->SectionObjectPointers,
                                EndOfFile,
                                0,
                                FALSE);
        }
    }
    return ExFatMapResult(Result);
}

/*
 * Renaming a directory does not move its children's directory entries, so
 * open child FatFs handles stay valid; only the cached path strings of
 * descendant FCBs go stale and are rewritten afterwards. The renamed FCB's
 * own FatFs handle must be closed first: an open FIL caches the directory
 * entry location that f_rename moves.
 */
static VOID
ExFatRenameDescendants(
    PEXFAT_VCB Vcb,
    PEXFAT_FCB Fcb,
    PUNICODE_STRING OldPath)
{
    PLIST_ENTRY Entry;
    PEXFAT_FCB WalkFcb;
    UNICODE_STRING NewChildPath;
    ULONG SuffixLength;

    for (Entry = Vcb->FcbListHead.Flink;
         Entry != &Vcb->FcbListHead;
         Entry = Entry->Flink)
    {
        WalkFcb = CONTAINING_RECORD(Entry, EXFAT_FCB, ListEntry);
        if (WalkFcb == Fcb || WalkFcb->DeleteCompleted)
            continue;
        if (WalkFcb->PathName.Length <= OldPath->Length ||
            WalkFcb->PathName.Buffer[OldPath->Length / sizeof(WCHAR)] != L'\\' ||
            !RtlPrefixUnicodeString(OldPath, &WalkFcb->PathName, TRUE))
        {
            continue;
        }

        SuffixLength = WalkFcb->PathName.Length - OldPath->Length;
        NewChildPath.Length = Fcb->PathName.Length + (USHORT)SuffixLength;
        NewChildPath.MaximumLength = NewChildPath.Length;
        NewChildPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                    NewChildPath.Length,
                                                    TAG_EXFAT_PATH);
        if (!NewChildPath.Buffer)
        {
            /* Unrenameable stale path: drop the FCB from future lookups. */
            WalkFcb->DeleteCompleted = TRUE;
            continue;
        }
        RtlCopyMemory(NewChildPath.Buffer,
                      Fcb->PathName.Buffer,
                      Fcb->PathName.Length);
        RtlCopyMemory((PUCHAR)NewChildPath.Buffer + Fcb->PathName.Length,
                      (PUCHAR)WalkFcb->PathName.Buffer + OldPath->Length,
                      SuffixLength);
        if (!NT_SUCCESS(ExFatSetFcbPath(WalkFcb, &NewChildPath)))
            WalkFcb->DeleteCompleted = TRUE;
        ExFreePoolWithTag(NewChildPath.Buffer, TAG_EXFAT_PATH);
    }
}

static NTSTATUS
ExFatSetRenameInformation(
    PEXFAT_FCB Fcb,
    PIO_STACK_LOCATION Stack,
    PFILE_RENAME_INFORMATION RenameInfo,
    ULONG Length)
{
    PEXFAT_VCB Vcb = Fcb->Vcb;
    PFILE_OBJECT TargetFileObject = Stack->Parameters.SetFile.FileObject;
    BOOLEAN ReplaceIfExists = Stack->Parameters.SetFile.ReplaceIfExists;
    PEXFAT_FCB TargetFcb = NULL;
    UNICODE_STRING NewParent;
    UNICODE_STRING NewLeaf;
    UNICODE_STRING NewPath;
    UNICODE_STRING OldPath;
    TCHAR* NewFatPath = NULL;
    FILINFO Information;
    FRESULT Result;
    NTSTATUS Status;
    ULONG NameFilter;
    ULONG Index;
    ULONGLONG NewPathHash;
    BOOLEAN SamePathExactly;
    BOOLEAN SamePathAnyCase;
    USHORT PrefixLength;
    BOOLEAN UnlinkTarget = FALSE;
    BOOLEAN RootParent;

    OldPath.Buffer = NULL;
    if (Length < sizeof(FILE_RENAME_INFORMATION) ||
        !RenameInfo->FileNameLength ||
        (RenameInfo->FileNameLength & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (Fcb->IsVolume || ExFatIsRootPath(&Fcb->PathName))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (Fcb->DeletePending)
        return STATUS_DELETE_PENDING;

    if (TargetFileObject)
    {
        TargetFcb = TargetFileObject->FsContext;
        if (!TargetFcb || !TargetFcb->IsDirectory || TargetFcb->Vcb != Vcb)
            return STATUS_INVALID_PARAMETER;
        NewParent = TargetFcb->PathName;
        NewLeaf = TargetFileObject->FileName;
        TargetFcb = NULL;
    }
    else
    {
        ExFatSplitPath(&Fcb->PathName, &NewParent, &NewLeaf);
        NewLeaf.Buffer = RenameInfo->FileName;
        NewLeaf.Length = (USHORT)RenameInfo->FileNameLength;
        NewLeaf.MaximumLength = NewLeaf.Length;
    }
    if (!NewLeaf.Length)
        return STATUS_OBJECT_NAME_INVALID;
    for (Index = 0; Index < NewLeaf.Length / sizeof(WCHAR); Index++)
    {
        if (NewLeaf.Buffer[Index] == L'\\' || NewLeaf.Buffer[Index] == L'/')
            return STATUS_OBJECT_NAME_INVALID;
    }

    RootParent = ExFatIsRootPath(&NewParent);
    NewPath.Length = (RootParent ? sizeof(WCHAR) : NewParent.Length + sizeof(WCHAR)) +
                     NewLeaf.Length;
    NewPath.MaximumLength = NewPath.Length;
    NewPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                           NewPath.Length,
                                           TAG_EXFAT_PATH);
    if (!NewPath.Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    PrefixLength = RootParent ? 0 : NewParent.Length;
    RtlCopyMemory(NewPath.Buffer, NewParent.Buffer, PrefixLength);
    NewPath.Buffer[PrefixLength / sizeof(WCHAR)] = L'\\';
    RtlCopyMemory((PUCHAR)NewPath.Buffer + PrefixLength + sizeof(WCHAR),
                  NewLeaf.Buffer,
                  NewLeaf.Length);

    NewPathHash = ExFatNormalizePath(&NewPath);
    SamePathExactly = RtlEqualUnicodeString(&NewPath, &Fcb->PathName, FALSE);
    SamePathAnyCase = RtlEqualUnicodeString(&NewPath, &Fcb->PathName, TRUE);
    if (SamePathExactly)
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    NewFatPath = ExFatBuildFatPath(Vcb, &NewPath);
    if (!NewFatPath)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!SamePathAnyCase)
    {
        TargetFcb = ExFatFindFcb(Vcb, &NewPath, NewPathHash);
        if (TargetFcb == Fcb)
        {
            ExFatDereferenceFcb(TargetFcb);
            TargetFcb = NULL;
        }
        if (TargetFcb)
        {
            if (!ReplaceIfExists)
            {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto Cleanup;
            }
            if (TargetFcb->IsDirectory ||
                TargetFcb->OpenHandleCount != 0 ||
                TargetFcb->DeletePending)
            {
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }
            if (TargetFcb->FileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                /* Replacing a target deletes it, so honour the same rule. */
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }
            if (TargetFcb->SectionObjectPointers.DataSectionObject &&
                !MmFlushImageSection(&TargetFcb->SectionObjectPointers,
                                     MmFlushForDelete))
            {
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }
            UnlinkTarget = TRUE;
        }
        else
        {
            Status = ExFatProbePath(Vcb, &NewPath, NewPathHash, NewFatPath,
                                    &Information, &Result);
            if (!NT_SUCCESS(Status))
                goto Cleanup;
            if (Result == FR_OK)
            {
                if (!ReplaceIfExists)
                {
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto Cleanup;
                }
                if ((Information.fattrib & (AM_DIR | AM_RDO)) != 0)
                {
                    Status = STATUS_ACCESS_DENIED;
                    goto Cleanup;
                }
                UnlinkTarget = TRUE;
            }
        }
    }

    /*
     * The FatFs lock is held from the close through the FCB path swap:
     * a concurrent paging write would otherwise reopen the file by its
     * stale path between the rename and the path update.
     */
    ExFatAcquireFatFs(Vcb);
    ExFatCloseFcbFile(Fcb);
    Result = FR_OK;
    if (UnlinkTarget)
        Result = f_unlink(NewFatPath);
    if (Result == FR_OK)
        Result = f_rename(Fcb->FatPath, NewFatPath);
    if (Result != FR_OK)
    {
        ExFatReleaseFatFs(Vcb);
        Status = (Result == FR_EXIST && SamePathAnyCase) ? STATUS_SUCCESS
                                                         : ExFatMapResult(Result);
        goto Cleanup;
    }

    Vcb->NamespaceGeneration++;
    if (UnlinkTarget && TargetFcb)
        TargetFcb->DeleteCompleted = TRUE;
    ExFatDirIndexRemove(Vcb, &Fcb->PathName);
    ExFatDirIndexRemove(Vcb, &NewPath);
    if (Fcb->IsDirectory)
        ExFatDropDirIndexes(Vcb);

    OldPath = Fcb->PathName;
    Fcb->PathName.Buffer = NULL;
    Fcb->PathName.Length = 0;
    Fcb->PathName.MaximumLength = 0;
    Status = ExFatSetFcbPath(Fcb, &NewPath);
    if (!NT_SUCCESS(Status))
    {
        /* The on-disk rename happened; drop the stale FCB from lookups. */
        Fcb->PathName = OldPath;
        OldPath.Buffer = NULL;
        Fcb->DeleteCompleted = TRUE;
        ExFatReleaseFatFs(Vcb);
        goto Cleanup;
    }

    if (Fcb->IsDirectory)
        ExFatRenameDescendants(Vcb, Fcb, &OldPath);

    /* ExFatDirIndexAppend takes the live values from the FCB when one is
     * supplied, so only the attribute byte has to be handed over here. */
    RtlZeroMemory(&Information, sizeof(Information));
    Information.fattrib = ExFatNtAttributesToFat(Fcb->FileAttributes) |
                          (Fcb->IsDirectory ? AM_DIR : 0);
    ExFatDirIndexInsert(Vcb, &NewParent, &NewLeaf, &Information, Fcb);
    ExFatReleaseFatFs(Vcb);

    NameFilter = ExFatNameChangeFilter(Fcb->IsDirectory);
    ExFatReportChange(Vcb, &OldPath, NameFilter, FILE_ACTION_RENAMED_OLD_NAME);
    ExFatReportChange(Vcb, &Fcb->PathName, NameFilter, FILE_ACTION_RENAMED_NEW_NAME);
    Status = STATUS_SUCCESS;

Cleanup:
    if (OldPath.Buffer)
        ExFreePoolWithTag(OldPath.Buffer, TAG_EXFAT_PATH);
    if (TargetFcb)
        ExFatDereferenceFcb(TargetFcb);
    if (NewFatPath)
        ExFreePoolWithTag(NewFatPath, TAG_EXFAT_PATH);
    ExFreePoolWithTag(NewPath.Buffer, TAG_EXFAT_PATH);
    return Status;
}

NTSTATUS
ExFatSetInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG Length = Stack->Parameters.SetFile.Length;
    FILE_INFORMATION_CLASS InfoClass;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject || !Buffer)
        return STATUS_INVALID_DEVICE_REQUEST;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || !Ccb)
        return STATUS_INVALID_HANDLE;
    if (Fcb->Vcb->ReadOnly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    InfoClass = Stack->Parameters.SetFile.FileInformationClass;

    /*
     * Renaming walks the FCB list and the directory indexes, so it needs the
     * volume before the file. Everything else takes the file alone; the one
     * piece of shared state an attribute change touches is refreshed after
     * the file is released, so the volume is never held across FatFs I/O.
     */
    if (InfoClass == FileRenameInformation)
        ExAcquireResourceExclusiveLite(&Fcb->Vcb->Resource, TRUE);
    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
    switch (InfoClass)
    {
        case FilePositionInformation:
            if (Length < sizeof(FILE_POSITION_INFORMATION))
                Status = STATUS_INFO_LENGTH_MISMATCH;
            else if (((PFILE_POSITION_INFORMATION)Buffer)->CurrentByteOffset.QuadPart < 0)
                Status = STATUS_INVALID_PARAMETER;
            else
            {
                FileObject->CurrentByteOffset = ((PFILE_POSITION_INFORMATION)Buffer)->CurrentByteOffset;
                Status = STATUS_SUCCESS;
            }
            break;

        case FileBasicInformation:
            if (Length < sizeof(FILE_BASIC_INFORMATION))
                Status = STATUS_INFO_LENGTH_MISMATCH;
            else if (Fcb->IsVolume)
                Status = STATUS_INVALID_DEVICE_REQUEST;
            else
            {
                Status = ExFatSetBasicInformation(Fcb, Buffer);
                if (NT_SUCCESS(Status))
                {
                    ExFatReportChange(Fcb->Vcb,
                                      &Fcb->PathName,
                                      FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                          FILE_NOTIFY_CHANGE_CREATION |
                                          FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE,
                                      FILE_ACTION_MODIFIED);
                }
            }
            break;

        case FileEndOfFileInformation:
            if (Length < sizeof(FILE_END_OF_FILE_INFORMATION))
                Status = STATUS_INFO_LENGTH_MISMATCH;
            else if (Fcb->IsDirectory || Fcb->IsVolume)
                Status = STATUS_INVALID_DEVICE_REQUEST;
            else if (Stack->Parameters.SetFile.AdvanceOnly)
            {
                /* Lazy-writer valid-data advance; f_write sizes eagerly. */
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = ExFatSetEndOfFile(Fcb,
                                           Ccb,
                                           &((PFILE_END_OF_FILE_INFORMATION)Buffer)->EndOfFile);
            }
            break;

        case FileRenameInformation:
            Status = ExFatSetRenameInformation(Fcb,
                                               Stack,
                                               Buffer,
                                               Length);
            break;

        case FileAllocationInformation:
            if (Length < sizeof(FILE_ALLOCATION_INFORMATION))
                Status = STATUS_INFO_LENGTH_MISMATCH;
            else if (((PFILE_ALLOCATION_INFORMATION)Buffer)->AllocationSize.QuadPart <
                     Fcb->Header.FileSize.QuadPart)
            {
                LARGE_INTEGER NewSize = ((PFILE_ALLOCATION_INFORMATION)Buffer)->AllocationSize;
                Status = ExFatSetEndOfFile(Fcb, Ccb, &NewSize);
            }
            else
                Status = STATUS_SUCCESS;
            break;

        case FileDispositionInformation:
            if (Length < sizeof(FILE_DISPOSITION_INFORMATION))
                Status = STATUS_INFO_LENGTH_MISMATCH;
            else if (Fcb->IsVolume)
                Status = STATUS_CANNOT_DELETE;
            else if (((PFILE_DISPOSITION_INFORMATION)Buffer)->DeleteFile &&
                     ((Fcb->FileAttributes & FILE_ATTRIBUTE_READONLY) ||
                      !MmFlushImageSection(&Fcb->SectionObjectPointers, MmFlushForDelete)))
                Status = STATUS_CANNOT_DELETE;
            else
            {
                Fcb->DeletePending = ((PFILE_DISPOSITION_INFORMATION)Buffer)->DeleteFile;
                Status = STATUS_SUCCESS;
            }
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }
    ExReleaseResourceLite(&Fcb->MainResource);
    if (InfoClass == FileRenameInformation)
    {
        ExReleaseResourceLite(&Fcb->Vcb->Resource);
    }
    else if (NT_SUCCESS(Status) &&
             (InfoClass == FileBasicInformation ||
              InfoClass == FileEndOfFileInformation ||
              InfoClass == FileAllocationInformation))
    {
        /* Publish the new attributes/size to the parent's index. */
        ExFatCommitFcbMetadata(Fcb);
    }
    return Status;
}

static NTSTATUS
ExFatQueryFreeClusters(
    PEXFAT_VCB Vcb,
    DWORD* FreeClusters)
{
    TCHAR DrivePath[3];
    FATFS* FileSystem;
    FRESULT Result;

    ExFatBuildDrivePath(Vcb, DrivePath);
    ExFatAcquireFatFs(Vcb);
    Result = f_getfree(DrivePath, FreeClusters, &FileSystem);
    ExFatReleaseFatFs(Vcb);
    return ExFatMapResult(Result);
}

static NTSTATUS
ExFatQueryVolumeLabel(
    PEXFAT_VCB Vcb,
    PFILE_FS_VOLUME_INFORMATION Information,
    ULONG Length,
    PULONG Written)
{
    ULONG HeaderLength = FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel);
    ULONG CopyLength;

    if (Length < HeaderLength)
        return STATUS_BUFFER_TOO_SMALL;

    /* The VPB copy is maintained at mount and set-label time. */
    RtlZeroMemory(Information, HeaderLength);
    Information->VolumeSerialNumber = Vcb->SerialNumber;
    Information->SupportsObjects = FALSE;
    Information->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;
    CopyLength = min((ULONG)Vcb->Vpb->VolumeLabelLength, Length - HeaderLength);
    RtlCopyMemory(Information->VolumeLabel, Vcb->Vpb->VolumeLabel, CopyLength);
    *Written = HeaderLength + CopyLength;
    return (CopyLength == Information->VolumeLabelLength) ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
}

NTSTATUS
ExFatQueryVolumeInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    static const WCHAR FileSystemName[] = L"exFAT";
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PEXFAT_VCB Vcb;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG Length = Stack->Parameters.QueryVolume.Length;
    ULONG Written = 0;
    DWORD FreeClusters;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !Buffer)
        return STATUS_INVALID_DEVICE_REQUEST;
    Vcb = DeviceObject->DeviceExtension;
    RtlZeroMemory(Buffer, Length);

    ExAcquireResourceSharedLite(&Vcb->Resource, TRUE);
    switch (Stack->Parameters.QueryVolume.FsInformationClass)
    {
        case FileFsVolumeInformation:
            Status = ExFatQueryVolumeLabel(Vcb, Buffer, Length, &Written);
            break;

        case FileFsSizeInformation:
            if (Length < sizeof(FILE_FS_SIZE_INFORMATION))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            Status = ExFatQueryFreeClusters(Vcb, &FreeClusters);
            if (!NT_SUCCESS(Status))
                break;
            ((PFILE_FS_SIZE_INFORMATION)Buffer)->TotalAllocationUnits.QuadPart = Vcb->FileSystem.n_fatent - 2;
            ((PFILE_FS_SIZE_INFORMATION)Buffer)->AvailableAllocationUnits.QuadPart = FreeClusters;
            ((PFILE_FS_SIZE_INFORMATION)Buffer)->SectorsPerAllocationUnit = Vcb->FileSystem.csize;
            ((PFILE_FS_SIZE_INFORMATION)Buffer)->BytesPerSector = Vcb->BytesPerSector;
            Written = sizeof(FILE_FS_SIZE_INFORMATION);
            break;

        case FileFsFullSizeInformation:
            if (Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            Status = ExFatQueryFreeClusters(Vcb, &FreeClusters);
            if (!NT_SUCCESS(Status))
                break;
            ((PFILE_FS_FULL_SIZE_INFORMATION)Buffer)->TotalAllocationUnits.QuadPart = Vcb->FileSystem.n_fatent - 2;
            ((PFILE_FS_FULL_SIZE_INFORMATION)Buffer)->CallerAvailableAllocationUnits.QuadPart = FreeClusters;
            ((PFILE_FS_FULL_SIZE_INFORMATION)Buffer)->ActualAvailableAllocationUnits.QuadPart = FreeClusters;
            ((PFILE_FS_FULL_SIZE_INFORMATION)Buffer)->SectorsPerAllocationUnit = Vcb->FileSystem.csize;
            ((PFILE_FS_FULL_SIZE_INFORMATION)Buffer)->BytesPerSector = Vcb->BytesPerSector;
            Written = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            break;

        case FileFsDeviceInformation:
            if (Length < sizeof(FILE_FS_DEVICE_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                ((PFILE_FS_DEVICE_INFORMATION)Buffer)->DeviceType = FILE_DEVICE_DISK;
                ((PFILE_FS_DEVICE_INFORMATION)Buffer)->Characteristics = Vcb->StorageDevice->Characteristics;
                Written = sizeof(FILE_FS_DEVICE_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;

        case FileFsAttributeInformation:
        {
            PFILE_FS_ATTRIBUTE_INFORMATION Attributes = Buffer;
            ULONG NameLength = sizeof(FileSystemName) - sizeof(UNICODE_NULL);
            ULONG Required = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) + NameLength;

            if (Length < Required)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            Attributes->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK;
            Attributes->MaximumComponentNameLength = FF_MAX_LFN;
            Attributes->FileSystemNameLength = NameLength;
            RtlCopyMemory(Attributes->FileSystemName, FileSystemName, NameLength);
            Written = Required;
            Status = STATUS_SUCCESS;
            break;
        }

#if (NTDDI_VERSION >= NTDDI_WIN8)
        case FileFsSectorSizeInformation:
            if (Length < sizeof(FILE_FS_SECTOR_SIZE_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                PFILE_FS_SECTOR_SIZE_INFORMATION Sector = Buffer;
                Sector->LogicalBytesPerSector = Vcb->BytesPerSector;
                Sector->PhysicalBytesPerSectorForAtomicity = Vcb->BytesPerSector;
                Sector->PhysicalBytesPerSectorForPerformance = Vcb->BytesPerSector;
                Sector->FileSystemEffectivePhysicalBytesPerSectorForAtomicity = Vcb->BytesPerSector;
                Sector->Flags = 0;
                Sector->ByteOffsetForSectorAlignment = 0;
                Sector->ByteOffsetForPartitionAlignment = 0;
                Written = sizeof(FILE_FS_SECTOR_SIZE_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            break;
#endif

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }
    ExReleaseResourceLite(&Vcb->Resource);
    Irp->IoStatus.Information = Written;
    return Status;
}

NTSTATUS
ExFatSetVolumeInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PEXFAT_VCB Vcb;
    PFILE_FS_LABEL_INFORMATION Information;
    UNICODE_STRING Label;
    TCHAR* FatLabel;
    FRESULT Result;

    if (DeviceObject == ExFatGlobalData->DeviceObject ||
        Stack->Parameters.SetVolume.FsInformationClass != FileFsLabelInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }
    Vcb = DeviceObject->DeviceExtension;
    if (Vcb->ReadOnly)
        return STATUS_MEDIA_WRITE_PROTECTED;
    if (Stack->Parameters.SetVolume.Length < FIELD_OFFSET(FILE_FS_LABEL_INFORMATION, VolumeLabel))
        return STATUS_INFO_LENGTH_MISMATCH;

    Information = Irp->AssociatedIrp.SystemBuffer;
    if (!Information || Information->VolumeLabelLength > Stack->Parameters.SetVolume.Length -
                                                       FIELD_OFFSET(FILE_FS_LABEL_INFORMATION, VolumeLabel) ||
        Information->VolumeLabelLength > MAXUSHORT)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Label.Buffer = Information->VolumeLabel;
    Label.Length = (USHORT)Information->VolumeLabelLength;
    Label.MaximumLength = Label.Length;
    FatLabel = ExFatBuildFatPath(Vcb, &Label);
    if (!FatLabel)
        return STATUS_INSUFFICIENT_RESOURCES;
    if (!Label.Length)
        FatLabel[2] = ANSI_NULL;

    ExFatAcquireFatFs(Vcb);
    Result = f_setlabel(FatLabel);
    ExFatReleaseFatFs(Vcb);
    ExFreePoolWithTag(FatLabel, TAG_EXFAT_PATH);

    if (Result == FR_OK)
    {
        /* Queries read the VPB copy under Vcb->Resource shared. */
        ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
        Vcb->Vpb->VolumeLabelLength = min(Label.Length,
                                          (USHORT)sizeof(Vcb->Vpb->VolumeLabel));
        RtlCopyMemory(Vcb->Vpb->VolumeLabel,
                      Label.Buffer,
                      Vcb->Vpb->VolumeLabelLength);
        ExReleaseResourceLite(&Vcb->Resource);
    }
    return ExFatMapResult(Result);
}
