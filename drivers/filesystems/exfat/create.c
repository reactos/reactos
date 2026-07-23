/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     File creation, cleanup, and close operations
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

#define EXFAT_STACK_PATH_CHARACTERS 260

static BOOLEAN
ExFatDispositionCreatesFile(
    ULONG Disposition)
{
    return Disposition == FILE_CREATE ||
           Disposition == FILE_OPEN_IF ||
           Disposition == FILE_OVERWRITE_IF ||
           Disposition == FILE_SUPERSEDE;
}

static NTSTATUS
ExFatEnsureFatPath(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING FullPath,
    TCHAR** FatPath)
{
    if (!*FatPath)
        *FatPath = ExFatBuildFatPath(Vcb, FullPath);
    return *FatPath ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

/*
 * A file or directory FatFs just created/reset is fully determined: empty,
 * given attributes, stamped now. Rescanning the parent with f_stat() would
 * only re-read that.
 */
static VOID
ExFatStampNewInfo(
    FILINFO* Information,
    BYTE Attributes)
{
    DWORD Now = get_fattime();

    RtlZeroMemory(Information, sizeof(*Information));
    Information->fattrib = Attributes;
    Information->fdate = (WORD)(Now >> 16);
    Information->ftime = (WORD)Now;
    Information->crdate = Information->fdate;
    Information->crtime = Information->ftime;
}

static NTSTATUS
ExFatSetShareAccess(
    PEXFAT_FCB Fcb,
    PEXFAT_CCB Ccb,
    PFILE_OBJECT FileObject,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess)
{
    NTSTATUS Status;

    if (Fcb->OpenHandleCount)
    {
        Status = IoCheckShareAccess(DesiredAccess,
                                    ShareAccess,
                                    FileObject,
                                    &Fcb->ShareAccess,
                                    TRUE);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        IoSetShareAccess(DesiredAccess,
                         ShareAccess,
                         FileObject,
                         &Fcb->ShareAccess);
    }

    Ccb->ShareAccessSet = TRUE;
    return STATUS_SUCCESS;
}

static VOID
ExFatAttachFileObject(
    PEXFAT_FCB Fcb,
    PEXFAT_CCB Ccb,
    PFILE_OBJECT FileObject)
{
    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;
    FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
    Ccb->FileObject = FileObject;
}

static NTSTATUS
ExFatOpenVolume(
    PEXFAT_VCB Vcb,
    PFILE_OBJECT FileObject,
    PIO_STACK_LOCATION Stack,
    ACCESS_MASK DesiredAccess)
{
    PEXFAT_CCB Ccb;
    NTSTATUS Status;

    if (Vcb->ReadOnly && ExFatIsWriteAccess(DesiredAccess))
        return STATUS_MEDIA_WRITE_PROTECTED;

    Ccb = ExAllocateFromNPagedLookasideList(&ExFatGlobalData->CcbLookaside);
    if (!Ccb)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(Ccb, sizeof(*Ccb));
    Ccb->DesiredAccess = DesiredAccess;

    ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
    if (Vcb->Locked && Vcb->LockOwner != FileObject)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Failure;
    }

    ExFatReferenceFcb(Vcb->VolumeFcb);
    Status = ExFatSetShareAccess(Vcb->VolumeFcb,
                                 Ccb,
                                 FileObject,
                                 DesiredAccess,
                                 Stack->Parameters.Create.ShareAccess);
    if (!NT_SUCCESS(Status))
    {
        ExFatDereferenceFcb(Vcb->VolumeFcb);
        goto Failure;
    }

    Vcb->VolumeFcb->OpenHandleCount++;
    Vcb->OpenHandleCount++;
    ExFatAttachFileObject(Vcb->VolumeFcb, Ccb, FileObject);
    FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
    ExReleaseResourceLite(&Vcb->Resource);
    return STATUS_SUCCESS;

Failure:
    ExReleaseResourceLite(&Vcb->Resource);
    ExFreeToNPagedLookasideList(&ExFatGlobalData->CcbLookaside, Ccb);
    return Status;
}

static VOID
ExFatCompleteDelete(
    PEXFAT_VCB Vcb,
    PEXFAT_FCB Fcb)
{
    FRESULT Result;
    DWORD ReuseCluster = 0;

    if (Fcb->DeleteCompleted)
        return;

    ExFatAcquireFatFs(Vcb);
    if (Fcb->FatFileOpen &&
        Fcb->FatFile.obj.fs == &Vcb->FileSystem &&
        Fcb->FatFile.obj.sclust >= 2 &&
        Fcb->FatFile.obj.sclust < Vcb->FileSystem.n_fatent)
    {
        ReuseCluster = Fcb->FatFile.obj.sclust;
    }
    ExFatCloseFcbFile(Fcb);
    Result = f_unlink(Fcb->FatPath);
    if (Result == FR_OK)
    {
        /*
         * Prefer the extent just freed by an active file. This mirrors
         * FatFs's CREATE_ALWAYS truncation policy, fills holes promptly and
         * avoids advancing allocation forever on delete/create workloads.
         */
        if (ReuseCluster)
            Vcb->FileSystem.last_clst = ReuseCluster - 1;
        Fcb->DeleteCompleted = TRUE;
        Vcb->NamespaceGeneration++;
        ExFatDirIndexRemove(Vcb, &Fcb->PathName);
    }
    ExFatReleaseFatFs(Vcb);
}

NTSTATUS
ExFatCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb = NULL;
    PEXFAT_CCB Ccb = NULL;
    UNICODE_STRING FullPath;
    WCHAR FullPathBuffer[EXFAT_STACK_PATH_CHARACTERS];
    ULONGLONG FullPathHash;
    UNICODE_STRING ParentPath;
    UNICODE_STRING LeafName;
    PEXFAT_DIR_INDEX DirIndex;
    PEXFAT_DIR_CHILD Child;
    TCHAR* FatPath = NULL;
    FIL NewFile;
    FILINFO Information;
    ACCESS_MASK DesiredAccess;
    ULONG Options;
    ULONG Disposition;
    ULONG CreateInformation = 0;
    BYTE OpenMode;
    FRESULT Result;
    NTSTATUS Status;
    BOOLEAN Exists;
    BOOLEAN IsRoot;
    BOOLEAN IsDirectory;
    BOOLEAN DirectoryRequested;
    BOOLEAN DeleteOnClose;
    BOOLEAN ShareSet = FALSE;
    BOOLEAN FcbResourceAcquired = FALSE;
    BOOLEAN NewFileOpen = FALSE;
    IO_STATUS_BLOCK CacheIoStatus;
    LARGE_INTEGER ZeroSize;

    if (DeviceObject == ExFatGlobalData->DeviceObject)
    {
        if (FileObject && FileObject->FileName.Length)
            return STATUS_OBJECT_PATH_NOT_FOUND;
        Irp->IoStatus.Information = FILE_OPENED;
        return STATUS_SUCCESS;
    }

    Vcb = DeviceObject->DeviceExtension;
    if (!Vcb->Mounted)
        return STATUS_VOLUME_DISMOUNTED;

    DesiredAccess = Stack->Parameters.Create.SecurityContext->DesiredAccess;
    Options = Stack->Parameters.Create.Options & 0x00FFFFFF;
    Disposition = (Stack->Parameters.Create.Options >> 24) & 0xFF;

    Status = ExFatBuildFullPath(FileObject,
                                &FullPath,
                                FullPathBuffer,
                                sizeof(FullPathBuffer),
                                &FullPathHash);
    if (!NT_SUCCESS(Status))
        return Status;
    if (!FullPath.Length)
    {
        Status = ExFatOpenVolume(Vcb, FileObject, Stack, DesiredAccess);
        if (NT_SUCCESS(Status))
            Irp->IoStatus.Information = FILE_OPENED;
        return Status;
    }

    Ccb = ExAllocateFromNPagedLookasideList(&ExFatGlobalData->CcbLookaside);
    if (!Ccb)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FailureWithoutLock;
    }
    RtlZeroMemory(Ccb, sizeof(*Ccb));
    Ccb->DesiredAccess = DesiredAccess;

    DeleteOnClose = !!(Options & FILE_DELETE_ON_CLOSE);
    DirectoryRequested = !!(Options & FILE_DIRECTORY_FILE);
    IsRoot = (FullPath.Length == sizeof(WCHAR) && FullPath.Buffer[0] == L'\\');

    ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
    if (Vcb->Locked && Vcb->LockOwner != FileObject)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Failure;
    }
    if (Vcb->ReadOnly &&
        (ExFatIsWriteAccess(DesiredAccess) || ExFatDispositionCreatesFile(Disposition) ||
         Disposition == FILE_OVERWRITE))
    {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto Failure;
    }

    RtlZeroMemory(&Information, sizeof(Information));
    Result = FR_OK;

    /* A live FCB answers the existence probe without a FatFs path walk. */
    Fcb = ExFatFindFcb(Vcb, &FullPath, FullPathHash);
    if (Fcb)
    {
        Exists = TRUE;
        IsDirectory = Fcb->IsDirectory;
    }
    else if (IsRoot)
    {
        Exists = TRUE;
        IsDirectory = TRUE;
        Information.fattrib = AM_DIR;
    }
    else if (ExFatLookupNegative(Vcb, &FullPath, FullPathHash, &Result))
    {
        Exists = FALSE;
        IsDirectory = FALSE;
    }
    else
    {
        ExFatSplitPath(&FullPath, &ParentPath, &LeafName);
        DirIndex = LeafName.Length ? ExFatEnsureDirIndex(Vcb, &ParentPath) : NULL;
        if (DirIndex)
        {
            /* The parent's one-sweep index answers without a FatFs scan. */
            Child = ExFatDirIndexLookup(DirIndex, &LeafName);
            Exists = (Child != NULL);
            if (Child)
            {
                Information.fattrib = Child->Attributes;
                Information.fsize = (FSIZE_t)Child->FileSize.QuadPart;
                Information.fdate = Child->ModDate;
                Information.ftime = Child->ModTime;
                Information.crdate = Child->CrtDate;
                Information.crtime = Child->CrtTime;
            }
            else
            {
                Result = FR_NO_FILE;
            }
        }
        else
        {
            Status = ExFatEnsureFatPath(Vcb, &FullPath, &FatPath);
            if (!NT_SUCCESS(Status))
                goto Failure;
            ExFatAcquireFatFs(Vcb);
            Result = f_stat(FatPath, &Information);
            ExFatReleaseFatFs(Vcb);
            Exists = (Result == FR_OK);
            if (!Exists)
            {
                if (Result != FR_NO_FILE && Result != FR_NO_PATH)
                {
                    Status = ExFatMapResult(Result);
                    goto Failure;
                }
                /* Loader search-order probes hammer the same missing names. */
                ExFatRememberNegative(Vcb, &FullPath, FullPathHash, Result);
            }
        }
        IsDirectory = Exists && !!(Information.fattrib & AM_DIR);
    }

    if (Exists && IsDirectory && (Options & FILE_NON_DIRECTORY_FILE))
    {
        Status = STATUS_FILE_IS_A_DIRECTORY;
        goto Failure;
    }
    if (Exists && !IsDirectory && DirectoryRequested)
    {
        Status = STATUS_NOT_A_DIRECTORY;
        goto Failure;
    }
    if (Exists && Disposition == FILE_CREATE)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto Failure;
    }
    if (!Exists && (Disposition == FILE_OPEN || Disposition == FILE_OVERWRITE))
    {
        Status = (Result == FR_NO_PATH) ? STATUS_OBJECT_PATH_NOT_FOUND : STATUS_OBJECT_NAME_NOT_FOUND;
        goto Failure;
    }
    if (IsRoot && Disposition != FILE_OPEN && Disposition != FILE_OPEN_IF)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto Failure;
    }

    if (Exists)
    {
        if (!Fcb)
            Fcb = ExFatCreateFcb(Vcb, &FullPath, &Information, FALSE);
        if (!Fcb)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Failure;
        }
        if (Fcb->DeletePending)
        {
            Status = STATUS_DELETE_PENDING;
            goto Failure;
        }

        Status = ExFatSetShareAccess(Fcb,
                                     Ccb,
                                     FileObject,
                                     DesiredAccess,
                                     Stack->Parameters.Create.ShareAccess);
        if (!NT_SUCCESS(Status))
            goto Failure;
        ShareSet = TRUE;
    }

    if (DirectoryRequested || IsDirectory)
    {
        if (!Exists)
        {
            Status = ExFatEnsureFatPath(Vcb, &FullPath, &FatPath);
            if (!NT_SUCCESS(Status))
                goto Failure;
            ExFatAcquireFatFs(Vcb);
            Result = f_mkdir(FatPath);
            ExFatReleaseFatFs(Vcb);
            if (Result != FR_OK)
            {
                Status = ExFatMapResult(Result);
                goto Failure;
            }
            ExFatStampNewInfo(&Information, AM_DIR);
            CreateInformation = FILE_CREATED;
        }
        else
        {
            CreateInformation = FILE_OPENED;
        }

        Status = ExFatEnsureFatPath(Vcb, &FullPath, &FatPath);
        if (!NT_SUCCESS(Status))
            goto Failure;
        ExFatAcquireFatFs(Vcb);
        Result = f_opendir(&Ccb->Directory, FatPath);
        ExFatReleaseFatFs(Vcb);
        Ccb->HandleOpen = (Result == FR_OK);
    }
    else
    {
        OpenMode = FA_READ;
        if (ExFatIsWriteAccess(DesiredAccess) || !Exists ||
            Disposition == FILE_OVERWRITE || Disposition == FILE_OVERWRITE_IF ||
            Disposition == FILE_SUPERSEDE)
        {
            OpenMode |= FA_WRITE;
        }

        switch (Disposition)
        {
            case FILE_CREATE:
                OpenMode |= FA_CREATE_NEW;
                CreateInformation = FILE_CREATED;
                break;
            case FILE_OPEN:
                OpenMode |= FA_OPEN_EXISTING;
                CreateInformation = FILE_OPENED;
                break;
            case FILE_OPEN_IF:
                OpenMode |= FA_OPEN_ALWAYS;
                CreateInformation = Exists ? FILE_OPENED : FILE_CREATED;
                break;
            case FILE_OVERWRITE:
                OpenMode |= FA_CREATE_ALWAYS;
                CreateInformation = FILE_OVERWRITTEN;
                break;
            case FILE_OVERWRITE_IF:
                OpenMode |= FA_CREATE_ALWAYS;
                CreateInformation = Exists ? FILE_OVERWRITTEN : FILE_CREATED;
                break;
            case FILE_SUPERSEDE:
                OpenMode |= FA_CREATE_ALWAYS;
                CreateInformation = Exists ? FILE_SUPERSEDED : FILE_CREATED;
                break;
            default:
                Status = STATUS_INVALID_PARAMETER;
                goto Failure;
        }

        /* f_open would refuse FA_WRITE on a read-only entry; keep parity. */
        if (Exists && (OpenMode & FA_WRITE) &&
            (Fcb->FileAttributes & FILE_ATTRIBUTE_READONLY))
        {
            Status = STATUS_ACCESS_DENIED;
            goto Failure;
        }

        if (CreateInformation == FILE_OPENED)
        {
            /*
             * Plain open of an existing file: f_stat (or the live FCB)
             * already validated it. Fcb->FatFile is opened lazily on
             * first data access.
             */
            Result = FR_OK;
        }
        else
        {
            Status = ExFatEnsureFatPath(Vcb, &FullPath, &FatPath);
            if (!NT_SUCCESS(Status))
                goto Failure;
            if (Fcb)
            {
                ZeroSize.QuadPart = 0;
                if (!MmCanFileBeTruncated(&Fcb->SectionObjectPointers, &ZeroSize))
                {
                    Status = STATUS_USER_MAPPED_FILE;
                    goto Failure;
                }

                ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
                FcbResourceAcquired = TRUE;
                if (Fcb->SectionObjectPointers.DataSectionObject)
                {
                    CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &CacheIoStatus);
                    if (!NT_SUCCESS(CacheIoStatus.Status) ||
                        !CcPurgeCacheSection(&Fcb->SectionObjectPointers, NULL, 0, FALSE))
                    {
                        Status = NT_SUCCESS(CacheIoStatus.Status) ?
                                 STATUS_USER_MAPPED_FILE : CacheIoStatus.Status;
                        ExReleaseResourceLite(&Fcb->MainResource);
                        FcbResourceAcquired = FALSE;
                        goto Failure;
                    }
                }
            }

            /*
             * Create/truncate into the FCB's FatFs handle so the first data
             * write does not reopen the same path. Truncation releases
             * clusters, so paging I/O must be drained first.
             */
            if (Fcb)
                ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
            ExFatAcquireFatFs(Vcb);
            Result = Fcb ? ExFatCloseFcbFile(Fcb) : FR_OK;
            if (Result == FR_OK)
                Result = f_open(&NewFile, FatPath, OpenMode);
            if (Result == FR_OK)
            {
                NewFileOpen = TRUE;
                ExFatStampNewInfo(&Information, AM_ARC);
                if (Fcb)
                {
                    Fcb->FatFile = NewFile;
                    Fcb->FatFileOpen = TRUE;
                    Fcb->FatFileWritable = TRUE;
                    NewFileOpen = FALSE;
                }
            }
            ExFatReleaseFatFs(Vcb);
            if (Fcb)
                ExReleaseResourceLite(&Fcb->PagingIoResource);
            if (FcbResourceAcquired)
            {
                ExReleaseResourceLite(&Fcb->MainResource);
                FcbResourceAcquired = FALSE;
            }
        }
    }

    if (Result != FR_OK)
    {
        Status = ExFatMapResult(Result);
        goto Failure;
    }
    if (!Fcb)
    {
        Fcb = ExFatCreateFcb(Vcb, &FullPath, &Information, FALSE);
        if (!Fcb)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Failure;
        }
        Status = ExFatSetShareAccess(Fcb,
                                     Ccb,
                                     FileObject,
                                     DesiredAccess,
                                     Stack->Parameters.Create.ShareAccess);
        if (!NT_SUCCESS(Status))
            goto Failure;
        ShareSet = TRUE;
    }
    else if (CreateInformation != FILE_OPENED)
    {
        ExFatUpdateFcbFromInfo(Fcb, &Information);
    }
    if (NewFileOpen)
    {
        Fcb->FatFile = NewFile;
        Fcb->FatFileOpen = TRUE;
        Fcb->FatFileWritable = TRUE;
        NewFileOpen = FALSE;
    }

    Fcb->OpenHandleCount++;
    Vcb->OpenHandleCount++;
    if (CreateInformation == FILE_CREATED)
    {
        Vcb->NamespaceGeneration++;
        ExFatSplitPath(&FullPath, &ParentPath, &LeafName);
        if (LeafName.Length)
            ExFatDirIndexInsert(Vcb, &ParentPath, &LeafName, &Information, Fcb);
    }
    if (DeleteOnClose)
        Fcb->DeletePending = TRUE;
    ExFatAttachFileObject(Fcb, Ccb, FileObject);
    if (CreateInformation != FILE_OPENED && Fcb->SectionObjectPointers.SharedCacheMap)
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
    if (!Fcb->IsDirectory)
    {
        if (Options & FILE_NO_INTERMEDIATE_BUFFERING)
            FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
        else
            FileObject->Flags |= FO_CACHE_SUPPORTED;
    }
    ExReleaseResourceLite(&Vcb->Resource);

    Irp->IoStatus.Information = CreateInformation;
    if (FatPath)
        ExFreePoolWithTag(FatPath, TAG_EXFAT_PATH);
    if (FullPath.Buffer != FullPathBuffer)
        ExFatFreeUnicodeString(&FullPath);
    return STATUS_SUCCESS;

Failure:
    if (NewFileOpen)
    {
        ExFatAcquireFatFs(Vcb);
        f_close(&NewFile);
        ExFatReleaseFatFs(Vcb);
    }
    if (Ccb && Ccb->HandleOpen)
    {
        ExFatAcquireFatFs(Vcb);
        f_closedir(&Ccb->Directory);
        ExFatReleaseFatFs(Vcb);
    }
    if (ShareSet && Fcb)
        IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
    if (Fcb)
        ExFatDereferenceFcb(Fcb);
    ExReleaseResourceLite(&Vcb->Resource);

FailureWithoutLock:
    if (Ccb)
        ExFreeToNPagedLookasideList(&ExFatGlobalData->CcbLookaside, Ccb);
    if (FatPath)
        ExFreePoolWithTag(FatPath, TAG_EXFAT_PATH);
    if (FullPath.Buffer != FullPathBuffer)
        ExFatFreeUnicodeString(&FullPath);
    return Status;
}

NTSTATUS
ExFatCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject)
        return STATUS_SUCCESS;
    Vcb = DeviceObject->DeviceExtension;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || !Ccb || Ccb->CleanedUp)
        return STATUS_SUCCESS;

    if (!Fcb->IsDirectory && !Fcb->IsVolume && FileObject->PrivateCacheMap)
        CcUninitializeCacheMap(FileObject, NULL, NULL);

    ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
    if (Ccb->ShareAccessSet)
    {
        IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
        Ccb->ShareAccessSet = FALSE;
    }
    if (Fcb->OpenHandleCount)
        Fcb->OpenHandleCount--;
    if (Vcb->OpenHandleCount)
        Vcb->OpenHandleCount--;
    if (Vcb->LockOwner == FileObject)
    {
        Vcb->Locked = FALSE;
        Vcb->LockOwner = NULL;
    }
    if (FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
    {
        FsRtlFastUnlockAll(&Fcb->FileLock,
                           FileObject,
                           IoGetRequestorProcess(Irp),
                           NULL);
    }
    if (Fcb->DeletePending && !Fcb->IsVolume && Fcb->OpenHandleCount == 0)
        ExFatCompleteDelete(Vcb, Fcb);
    Ccb->CleanedUp = TRUE;
    FileObject->Flags |= FO_CLEANUP_COMPLETE;
    ExReleaseResourceLite(&Vcb->Resource);
    return STATUS_SUCCESS;
}

NTSTATUS
ExFatClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject)
        return STATUS_SUCCESS;
    Vcb = DeviceObject->DeviceExtension;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || !Ccb)
        return STATUS_SUCCESS;

    ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
    if (!Ccb->CleanedUp)
    {
        if (Ccb->ShareAccessSet)
            IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
        if (Fcb->OpenHandleCount)
            Fcb->OpenHandleCount--;
        if (Vcb->OpenHandleCount)
            Vcb->OpenHandleCount--;
    }

    if (Ccb->HandleOpen)
    {
        ExFatAcquireFatFs(Vcb);
        f_closedir(&Ccb->Directory);
        ExFatReleaseFatFs(Vcb);
        Ccb->HandleOpen = FALSE;
    }

    if (Fcb->DeletePending && !Fcb->DeleteCompleted &&
        !Fcb->IsVolume && Fcb->OpenHandleCount == 0)
        ExFatCompleteDelete(Vcb, Fcb);

    RtlFreeUnicodeString(&Ccb->SearchPattern);
    FileObject->FsContext = NULL;
    FileObject->FsContext2 = NULL;
    FileObject->SectionObjectPointer = NULL;
    ExFreeToNPagedLookasideList(&ExFatGlobalData->CcbLookaside, Ccb);
    ExFatDereferenceFcb(Fcb);
    ExReleaseResourceLite(&Vcb->Resource);
    return STATUS_SUCCESS;
}
