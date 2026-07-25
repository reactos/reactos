/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     File and raw-volume reads and writes
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

static FRESULT
ExFatReadFileData(
    PEXFAT_FCB Fcb,
    PIRP Irp,
    PVOID Buffer,
    ULONGLONG ByteOffset,
    ULONG Length,
    PUINT BytesRead)
{
    UINT Bytes = 0;
    FRESULT Result;

    *BytesRead = 0;
    Result = ExFatEnsureFcbFile(Fcb, FALSE);
    if (Result != FR_OK)
        return Result;

    if (ByteOffset >= f_size(&Fcb->FatFile))
        return FR_OK;
    if (Length > f_size(&Fcb->FatFile) - ByteOffset)
        Length = (ULONG)(f_size(&Fcb->FatFile) - ByteOffset);

    if (!Buffer)
    {
        Buffer = ExFatGetDirectIoBuffer(Irp);
        if (!Buffer)
            return FR_NOT_ENOUGH_CORE;
    }
    Result = f_lseek(&Fcb->FatFile, (FSIZE_t)ByteOffset);
    if (Result != FR_OK)
        return Result;

    /* The window is keyed on the MDL's address, which is where FatFs writes. */
    NT_ASSERT(!Irp || !Irp->MdlAddress ||
              Buffer == MmGetMdlVirtualAddress(Irp->MdlAddress));
    ExFatBeginDataRead(Fcb->Vcb, Irp ? Irp->MdlAddress : NULL, Length);
    Result = f_read(&Fcb->FatFile, Buffer, Length, &Bytes);
    ExFatEndDataRead(Fcb->Vcb);
    *BytesRead += Bytes;
    return Result;
}

static FRESULT
ExFatWriteFileData(
    PEXFAT_FCB Fcb,
    PIRP Irp,
    PVOID Buffer,
    ULONGLONG ByteOffset,
    ULONG Length,
    BOOLEAN CanExtend,
    PUINT BytesWritten)
{
    UINT Bytes = 0;
    FRESULT Result;

    *BytesWritten = 0;
    Result = ExFatEnsureFcbFile(Fcb, TRUE);
    if (Result != FR_OK)
        return Result;

    if (CanExtend)
    {
        if ((FSIZE_t)ByteOffset > f_size(&Fcb->FatFile))
        {
            Result = ExFatZeroFileRange(Fcb,
                                        f_size(&Fcb->FatFile),
                                        (FSIZE_t)ByteOffset);
            if (Result != FR_OK)
                return Result;
        }
    }

    if (!Buffer)
    {
        Buffer = ExFatGetUserBuffer(Irp, FALSE);
        if (!Buffer)
            return FR_NOT_ENOUGH_CORE;
    }
    Result = f_lseek(&Fcb->FatFile, (FSIZE_t)ByteOffset);
    if (Result != FR_OK)
        return Result;

    Result = f_write(&Fcb->FatFile, Buffer, Length, &Bytes);
    *BytesWritten += Bytes;
    /* FatFs signals a full volume as a short transfer with FR_OK. */
    if (Result == FR_OK && Bytes < Length)
        Result = EXFAT_FR_DISK_FULL;
    return Result;
}

static NTSTATUS
ExFatReadVolume(
    PEXFAT_VCB Vcb,
    PIRP Irp,
    PVOID Buffer,
    ULONG Length,
    LARGE_INTEGER Offset)
{
    ULONGLONG VolumeLength = Vcb->SectorCount * Vcb->BytesPerSector;
    NTSTATUS Status;

    if (Offset.QuadPart < 0 || (ULONGLONG)Offset.QuadPart >= VolumeLength)
        return STATUS_END_OF_FILE;
    if (((ULONGLONG)Offset.QuadPart % Vcb->BytesPerSector) != 0 ||
        (Length % Vcb->BytesPerSector) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (Length > VolumeLength - Offset.QuadPart)
        Length = (ULONG)(VolumeLength - Offset.QuadPart);

    if (!Length)
        return STATUS_SUCCESS;
    ExFatAcquireFatFs(Vcb);
    Status = ExFatFlushSectorCache(Vcb);
    ExFatReleaseFatFs(Vcb);
    if (!NT_SUCCESS(Status))
        return Status;
    if (!NT_SUCCESS(ExFatReadWriteDevice(Vcb->StorageDevice,
                                         IRP_MJ_READ,
                                         Buffer,
                                         Length,
                                         &Offset,
                                         FALSE)))
    {
        return STATUS_IO_DEVICE_ERROR;
    }

    Irp->IoStatus.Information = Length;
    return STATUS_SUCCESS;
}

NTSTATUS
ExFatRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;
    LARGE_INTEGER Offset = Stack->Parameters.Read.ByteOffset;
    ULONG Length = Stack->Parameters.Read.Length;
    UINT BytesRead = 0;
    PVOID Buffer;
    FRESULT Result;
    NTSTATUS Status;
    BOOLEAN PagingIo = !!(Irp->Flags & IRP_PAGING_IO);
    BOOLEAN NoCache;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (Stack->MinorFunction & IRP_MN_COMPLETE)
    {
        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        return STATUS_SUCCESS;
    }

    Vcb = DeviceObject->DeviceExtension;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || (!PagingIo && !Ccb))
        return STATUS_INVALID_HANDLE;
    if (Fcb->IsDirectory)
        return STATUS_INVALID_DEVICE_REQUEST;
    if (!Length)
        return STATUS_SUCCESS;

    if (Offset.LowPart == FILE_USE_FILE_POINTER_POSITION && Offset.HighPart == -1)
        Offset = FileObject->CurrentByteOffset;
    if (Offset.QuadPart < 0)
        return STATUS_INVALID_PARAMETER;

    NoCache = PagingIo ||
              !!(Irp->Flags & IRP_NOCACHE) ||
              !!(FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING);

    if (!(Stack->MinorFunction & IRP_MN_MDL))
    {
        Status = ExFatLockUserBuffer(Irp, Length, IoWriteAccess);
        if (!NT_SUCCESS(Status))
            return Status;
        if (!NoCache || PagingIo || Fcb->IsVolume)
        {
            Buffer = ExFatGetUserBuffer(Irp, PagingIo);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Buffer = NULL;
        }
    }
    else
    {
        Buffer = NULL;
    }

    if (Fcb->IsVolume)
    {
        Status = ExFatReadVolume(Vcb, Irp, Buffer, Length, Offset);
    }
    else
    {
        if (!PagingIo && !(Ccb->DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)))
            return STATUS_ACCESS_DENIED;
        if ((ULONGLONG)Offset.QuadPart >= (ULONGLONG)Fcb->Header.FileSize.QuadPart)
            return STATUS_END_OF_FILE;
        if (Length > (ULONGLONG)Fcb->Header.FileSize.QuadPart - Offset.QuadPart)
            Length = (ULONG)((ULONGLONG)Fcb->Header.FileSize.QuadPart - Offset.QuadPart);
        if (!(Irp->Flags & IRP_PAGING_IO) &&
            !FsRtlCheckLockForReadAccess(&Fcb->FileLock, Irp))
        {
            return STATUS_FILE_LOCK_CONFLICT;
        }

        if (!NoCache)
        {
            ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                if (!FileObject->PrivateCacheMap)
                {
                    CcInitializeCacheMap(FileObject,
                                         (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                         FALSE,
                                         &ExFatGlobalData->CacheManagerCallbacks,
                                         Fcb);
                    CcSetReadAheadGranularity(FileObject, EXFAT_READ_AHEAD_GRANULARITY);
                }

                if (Stack->MinorFunction & IRP_MN_MDL)
                {
                    CcMdlRead(FileObject,
                              &Offset,
                              Length,
                              &Irp->MdlAddress,
                              &Irp->IoStatus);
                    Status = Irp->IoStatus.Status;
                }
                else if (!CcCopyRead(FileObject,
                                     &Offset,
                                     Length,
                                     TRUE,
                                     Buffer,
                                     &Irp->IoStatus))
                {
                    Status = STATUS_CANT_WAIT;
                }
                else
                {
                    Status = Irp->IoStatus.Status;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            ExReleaseResourceLite(&Fcb->MainResource);
        }
        else
        {
            ExAcquireResourceSharedLite(PagingIo ? &Fcb->PagingIoResource : &Fcb->MainResource,
                                        TRUE);
            ExFatAcquireFatFs(Vcb);
            Result = ExFatReadFileData(Fcb,
                                       PagingIo ? NULL : Irp,
                                       Buffer,
                                       (ULONGLONG)Offset.QuadPart,
                                       Length,
                                       &BytesRead);
            ExFatReleaseFatFs(Vcb);
            ExReleaseResourceLite(PagingIo ? &Fcb->PagingIoResource : &Fcb->MainResource);

            Status = ExFatMapResult(Result);
            if (NT_SUCCESS(Status))
                Irp->IoStatus.Information = BytesRead;
        }
    }

    if (NT_SUCCESS(Status) && !PagingIo && (FileObject->Flags & FO_SYNCHRONOUS_IO))
        FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Irp->IoStatus.Information;
    return Status;
}

static NTSTATUS
ExFatWriteVolume(
    PEXFAT_VCB Vcb,
    PFILE_OBJECT FileObject,
    PIRP Irp,
    PVOID Buffer,
    ULONG Length,
    LARGE_INTEGER Offset)
{
    ULONGLONG VolumeLength = Vcb->SectorCount * Vcb->BytesPerSector;

    if (Vcb->ReadOnly)
        return STATUS_MEDIA_WRITE_PROTECTED;
    if (!Vcb->Locked || Vcb->LockOwner != FileObject)
        return STATUS_ACCESS_DENIED;
    if (((ULONGLONG)Offset.QuadPart % Vcb->BytesPerSector) != 0 ||
        (Length % Vcb->BytesPerSector) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (Offset.QuadPart < 0 ||
        (ULONGLONG)Offset.QuadPart > VolumeLength ||
        Length > VolumeLength - Offset.QuadPart)
    {
        return STATUS_END_OF_FILE;
    }

    if (!Length)
        return STATUS_SUCCESS;
    ExFatAcquireFatFs(Vcb);
    if (!NT_SUCCESS(ExFatRawWriteDevice(Vcb, Buffer, Length, &Offset)))
    {
        ExFatReleaseFatFs(Vcb);
        return STATUS_IO_DEVICE_ERROR;
    }
    ExFatReleaseFatFs(Vcb);

    Irp->IoStatus.Information = Length;
    return STATUS_SUCCESS;
}

NTSTATUS
ExFatWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;
    LARGE_INTEGER Offset = Stack->Parameters.Write.ByteOffset;
    ULONG Length = Stack->Parameters.Write.Length;
    UINT BytesWritten = 0;
    PVOID Buffer;
    FRESULT Result;
    NTSTATUS Status;
    BOOLEAN PagingIo = !!(Irp->Flags & IRP_PAGING_IO);
    BOOLEAN NoCache;
    LARGE_INTEGER EndOffset;
    LARGE_INTEGER OldValidDataLength;
    IO_STATUS_BLOCK FlushStatus;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !FileObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (Stack->MinorFunction & IRP_MN_COMPLETE)
    {
        CcMdlWriteComplete(FileObject,
                           &Stack->Parameters.Write.ByteOffset,
                           Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        return STATUS_SUCCESS;
    }

    Vcb = DeviceObject->DeviceExtension;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || (!PagingIo && !Ccb))
        return STATUS_INVALID_HANDLE;
    if (Fcb->IsDirectory)
        return STATUS_INVALID_DEVICE_REQUEST;
    if (Vcb->ReadOnly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (Offset.LowPart == FILE_WRITE_TO_END_OF_FILE && Offset.HighPart == -1)
        Offset = Fcb->Header.FileSize;
    else if (Offset.LowPart == FILE_USE_FILE_POINTER_POSITION && Offset.HighPart == -1)
        Offset = FileObject->CurrentByteOffset;
    if (Offset.QuadPart < 0 || (ULONGLONG)Offset.QuadPart + Length < (ULONGLONG)Offset.QuadPart)
        return STATUS_INVALID_PARAMETER;
    if (!Length)
        return STATUS_SUCCESS;

    EndOffset.QuadPart = Offset.QuadPart + Length;
    NoCache = PagingIo ||
              !!(Irp->Flags & IRP_NOCACHE) ||
              !!(FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING);

    if (!(Stack->MinorFunction & IRP_MN_MDL))
    {
        Status = ExFatLockUserBuffer(Irp, Length, IoReadAccess);
        if (!NT_SUCCESS(Status))
            return Status;
        Buffer = ExFatGetUserBuffer(Irp, PagingIo);
        if (!Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Buffer = NULL;
    }

    if (Fcb->IsVolume)
    {
        Status = ExFatWriteVolume(Vcb, FileObject, Irp, Buffer, Length, Offset);
    }
    else
    {
        if (!PagingIo && !(Ccb->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
            return STATUS_ACCESS_DENIED;
        if (!PagingIo && !FsRtlCheckLockForWriteAccess(&Fcb->FileLock, Irp))
            return STATUS_FILE_LOCK_CONFLICT;
        if (PagingIo && (ULONGLONG)Offset.QuadPart >= (ULONGLONG)Fcb->Header.FileSize.QuadPart)
            return STATUS_SUCCESS;
        if (PagingIo && Length > (ULONGLONG)Fcb->Header.FileSize.QuadPart - Offset.QuadPart)
            Length = (ULONG)((ULONGLONG)Fcb->Header.FileSize.QuadPart - Offset.QuadPart);

        if (!Length)
            return STATUS_SUCCESS;
        EndOffset.QuadPart = Offset.QuadPart + Length;

        if (!NoCache)
        {
            ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
            Status = STATUS_SUCCESS;
            OldValidDataLength = Fcb->Header.ValidDataLength;

            _SEH2_TRY
            {
                if (!FileObject->PrivateCacheMap)
                {
                    CcInitializeCacheMap(FileObject,
                                         (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                         FALSE,
                                         &ExFatGlobalData->CacheManagerCallbacks,
                                         Fcb);
                    CcSetReadAheadGranularity(FileObject, EXFAT_READ_AHEAD_GRANULARITY);
                }

                if (EndOffset.QuadPart > Fcb->Header.FileSize.QuadPart)
                {
                    ExFatAcquireFatFs(Vcb);
                    Result = ExFatEnsureFcbFile(Fcb, TRUE);
                    if (Result == FR_OK)
                        Result = f_lseek(&Fcb->FatFile, (FSIZE_t)EndOffset.QuadPart);
                    if (Result == FR_OK)
                    {
                        Fcb->Header.FileSize.QuadPart = f_size(&Fcb->FatFile);
                        Fcb->Header.AllocationSize.QuadPart = ExFatRoundUp(f_size(&Fcb->FatFile),
                                                                          Vcb->BytesPerCluster);
                    }
                    ExFatReleaseFatFs(Vcb);
                    Status = ExFatMapResult(Result);
                    if (!NT_SUCCESS(Status))
                        _SEH2_LEAVE;
                    CcSetFileSizes(FileObject,
                                   (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
                }

                if (Offset.QuadPart > OldValidDataLength.QuadPart &&
                    !CcZeroData(FileObject,
                                &OldValidDataLength,
                                &Offset,
                                TRUE))
                {
                    Status = STATUS_CANT_WAIT;
                    _SEH2_LEAVE;
                }

                if (Stack->MinorFunction & IRP_MN_MDL)
                {
                    CcPrepareMdlWrite(FileObject,
                                      &Offset,
                                      Length,
                                      &Irp->MdlAddress,
                                      &Irp->IoStatus);
                    Status = Irp->IoStatus.Status;
                }
                else if (!CcCopyWrite(FileObject,
                                      &Offset,
                                      Length,
                                      TRUE,
                                      Buffer))
                {
                    Status = STATUS_CANT_WAIT;
                }
                else
                {
                    Irp->IoStatus.Information = Length;
                }

                /* Cc updated itself during CcCopyWrite; only track our VDL. */
                if (NT_SUCCESS(Status) &&
                    EndOffset.QuadPart > Fcb->Header.ValidDataLength.QuadPart)
                {
                    Fcb->Header.ValidDataLength = EndOffset;
                }

                if (NT_SUCCESS(Status) &&
                    ((Stack->Flags & SL_WRITE_THROUGH) ||
                     (FileObject->Flags & FO_WRITE_THROUGH)))
                {
                    if (Offset.QuadPart > OldValidDataLength.QuadPart)
                    {
                        CcFlushCache(&Fcb->SectionObjectPointers,
                                     NULL,
                                     0,
                                     &FlushStatus);
                    }
                    else
                    {
                        CcFlushCache(&Fcb->SectionObjectPointers,
                                     &Offset,
                                     Length,
                                     &FlushStatus);
                    }
                    Status = FlushStatus.Status;
                    if (NT_SUCCESS(Status))
                    {
                        ExFatAcquireFatFs(Vcb);
                        Result = ExFatEnsureFcbFile(Fcb, TRUE);
                        if (Result == FR_OK)
                            Result = f_sync(&Fcb->FatFile);
                        if (Result == FR_OK && !NT_SUCCESS(ExFatFlushSectorCache(Vcb)))
                            Result = FR_DISK_ERR;
                        ExFatReleaseFatFs(Vcb);
                        Status = ExFatMapResult(Result);
                    }
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            if (NT_SUCCESS(Status))
                FileObject->Flags |= FO_FILE_MODIFIED;
            ExReleaseResourceLite(&Fcb->MainResource);
        }
        else
        {
            if (PagingIo)
                /* Cc flush already owns this resource shared. */
                ExAcquireResourceSharedLite(&Fcb->PagingIoResource, TRUE);
            else
                ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
            if (!PagingIo && Fcb->SectionObjectPointers.DataSectionObject)
            {
                CcFlushCache(&Fcb->SectionObjectPointers,
                             &Offset,
                             Length,
                             &FlushStatus);
                if (!NT_SUCCESS(FlushStatus.Status) ||
                    !CcPurgeCacheSection(&Fcb->SectionObjectPointers,
                                         &Offset,
                                         Length,
                                         FALSE))
                {
                    Status = NT_SUCCESS(FlushStatus.Status) ?
                             STATUS_USER_MAPPED_FILE : FlushStatus.Status;
                    ExReleaseResourceLite(&Fcb->MainResource);
                    return Status;
                }
            }

            ExFatAcquireFatFs(Vcb);
            Result = ExFatWriteFileData(Fcb,
                                        Irp,
                                        Buffer,
                                        (ULONGLONG)Offset.QuadPart,
                                        Length,
                                        !PagingIo,
                                        &BytesWritten);
            if (Result == FR_OK &&
                ((Stack->Flags & SL_WRITE_THROUGH) || (FileObject->Flags & FO_WRITE_THROUGH)))
            {
                Result = f_sync(&Fcb->FatFile);
                if (Result == FR_OK && !NT_SUCCESS(ExFatFlushSectorCache(Vcb)))
                    Result = FR_DISK_ERR;
            }
            if (Result == FR_OK && !PagingIo)
            {
                Fcb->Header.FileSize.QuadPart = f_size(&Fcb->FatFile);
                Fcb->Header.ValidDataLength = Fcb->Header.FileSize;
                Fcb->Header.AllocationSize.QuadPart = ExFatRoundUp(f_size(&Fcb->FatFile),
                                                                   Vcb->BytesPerCluster);
            }
            ExFatReleaseFatFs(Vcb);

            Status = ExFatMapResult(Result);
            if (NT_SUCCESS(Status))
            {
                Irp->IoStatus.Information = BytesWritten;
                FileObject->Flags |= FO_FILE_MODIFIED;
                if (!PagingIo && Fcb->SectionObjectPointers.SharedCacheMap)
                {
                    CcSetFileSizes(FileObject,
                                   (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
                }
            }
            ExReleaseResourceLite(PagingIo ? &Fcb->PagingIoResource : &Fcb->MainResource);
            /* Volume lock precedes FCB locks, so refresh the index here. */
            if (NT_SUCCESS(Status) && !PagingIo)
                ExFatCommitFcbMetadata(Fcb);
        }
    }

    if (NT_SUCCESS(Status) && !PagingIo && (FileObject->Flags & FO_SYNCHRONOUS_IO))
        FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Irp->IoStatus.Information;
    return Status;
}
