/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fastio.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#define FASTIO_DEBUG_LEVEL DL_NVR


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, Ext2FastIoRead)
#pragma alloc_text(PAGE, Ext2FastIoWrite)
#pragma alloc_text(PAGE, Ext2FastIoCheckIfPossible)
#pragma alloc_text(PAGE, Ext2FastIoQueryBasicInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryStandardInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, Ext2FastIoLock)
#pragma alloc_text(PAGE, Ext2FastIoUnlockSingle)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#endif

FAST_IO_POSSIBLE
Ext2IsFastIoPossible(
    IN PEXT2_FCB Fcb
)
{
    FAST_IO_POSSIBLE IsPossible = FastIoIsNotPossible;

    if (!Fcb || !FsRtlOplockIsFastIoPossible(&Fcb->Oplock))
        return IsPossible;

    IsPossible = FastIoIsQuestionable;

    if (!FsRtlAreThereCurrentFileLocks(&Fcb->FileLockAnchor)) {
        if (!IsVcbReadOnly(Fcb->Vcb) && !FlagOn(Fcb->Vcb->Flags, VCB_VOLUME_LOCKED)) {
            IsPossible = FastIoIsPossible;
        }
    }

    return IsPossible;
}


BOOLEAN NTAPI
Ext2FastIoCheckIfPossible (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    IN BOOLEAN              CheckForReadOperation,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
)
{
    BOOLEAN          bPossible = FastIoIsNotPossible;
    PEXT2_FCB        Fcb;
    PEXT2_CCB        Ccb;
    LARGE_INTEGER    lLength;

    lLength.QuadPart = Length;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            /* do nothing if target fie was deleted */
            if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                _SEH2_LEAVE;
            }

            if (IsDirectory(Fcb)) {
                _SEH2_LEAVE;
            }

            Ccb = (PEXT2_CCB) FileObject->FsContext2;
            if (Ccb == NULL) {
                _SEH2_LEAVE;
            }

            if (CheckForReadOperation) {

                bPossible = FsRtlFastCheckLockForRead(
                                &Fcb->FileLockAnchor,
                                FileOffset,
                                &lLength,
                                LockKey,
                                FileObject,
                                PsGetCurrentProcess());

            } else {

                if (!IsVcbReadOnly(Fcb->Vcb)) {
                    bPossible = FsRtlFastCheckLockForWrite(
                                    &Fcb->FileLockAnchor,
                                    FileOffset,
                                    &lLength,
                                    LockKey,
                                    FileObject,
                                    PsGetCurrentProcess());
                }
            }

#if EXT2_DEBUG
            DEBUG(DL_INF, ("Ext2FastIIOCheckPossible: %s %s %wZ\n",
                           Ext2GetCurrentProcessName(),
                           "FASTIO_CHECK_IF_POSSIBLE",
                           &Fcb->Mcb->FullName
                          ));

            DEBUG(DL_INF, (
                      "Ext2FastIIOCheckPossible: Offset: %I64xg Length: %xh Key: %u %s %s\n",
                      FileOffset->QuadPart,
                      Length,
                      LockKey,
                      (CheckForReadOperation ? "CheckForReadOperation:" :
                       "CheckForWriteOperation:"),
                      (bPossible ? "Succeeded" : "Failed")));
#endif

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            bPossible = FastIoIsNotPossible;
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

    return bPossible;
}


BOOLEAN NTAPI
Ext2FastIoRead (IN PFILE_OBJECT         FileObject,
                IN PLARGE_INTEGER       FileOffset,
                IN ULONG                Length,
                IN BOOLEAN              Wait,
                IN ULONG                LockKey,
                OUT PVOID               Buffer,
                OUT PIO_STATUS_BLOCK    IoStatus,
                IN PDEVICE_OBJECT       DeviceObject)
{
    PEXT2_FCB    Fcb;
    BOOLEAN      Status = FALSE;

    Fcb = (PEXT2_FCB) FileObject->FsContext;
    if (Fcb == NULL) {
        return FALSE;
    }

    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    Status = FsRtlCopyRead (
                 FileObject, FileOffset, Length, Wait,
                 LockKey, Buffer, IoStatus, DeviceObject);

    DEBUG(DL_IO, ("Ext2FastIoRead: %wZ Offset: %I64xh Length: %xh Key: %u Status: %d\n",
                  &Fcb->Mcb->ShortName, FileOffset->QuadPart, Length, LockKey, Status));

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoWrite (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject)
{
    PEXT2_FCB   Fcb = NULL;
    BOOLEAN     Status = FALSE;
    BOOLEAN     Locked = FALSE;

    Fcb = (PEXT2_FCB) FileObject->FsContext;
    if (Fcb == NULL)
        return FALSE;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsVcbReadOnly(Fcb->Vcb)) {
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceSharedLite(Fcb->Header.Resource, Wait)) {
            _SEH2_LEAVE;
        }
        Locked = TRUE;

        if (IsWritingToEof(*FileOffset) ||
            Fcb->Header.ValidDataLength.QuadPart < FileOffset->QuadPart + Length ||
            Fcb->Header.FileSize.QuadPart < FileOffset->QuadPart + Length ) {
            Status = FALSE;
            _SEH2_LEAVE;
        }

        if (Locked) {
            ExReleaseResourceLite(Fcb->Header.Resource);
            Locked = FALSE;
        }

        Status = FsRtlCopyWrite(FileObject, FileOffset, Length, Wait,
                                LockKey, Buffer, IoStatus, DeviceObject);
        if (Status) {
            if (IoStatus)
                Length = (ULONG)IoStatus->Information;
        }

    } _SEH2_FINALLY {

        if (Locked) {
            ExReleaseResourceLite(Fcb->Header.Resource);
        }

        FsRtlExitFileSystem();
    } _SEH2_END;

    DEBUG(DL_IO, ("Ext2FastIoWrite: %wZ Offset: %I64xh Length: %xh Key: %xh Status=%d\n",
                  &Fcb->Mcb->ShortName,  FileOffset->QuadPart, Length, LockKey, Status));

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoQueryBasicInfo (
    IN PFILE_OBJECT             FileObject,
    IN BOOLEAN                  Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK        IoStatus,
    IN PDEVICE_OBJECT           DeviceObject)
{
    PEXT2_FCB   Fcb = NULL;
    PEXT2_CCB   Ccb = NULL;
    PEXT2_MCB   Mcb = NULL;
    BOOLEAN     Status = FALSE;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
            Ccb = (PEXT2_CCB) FileObject->FsContext2;
            Mcb = Fcb->Mcb;
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoQueryBasicInfo: %s %s %wZ\n",
                      Ext2GetCurrentProcessName(),
                      "FASTIO_QUERY_BASIC_INFO",
                      &Fcb->Mcb->FullName
                  ));
#endif
            if (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE)) {
                if (!ExAcquireResourceSharedLite(
                            &Fcb->MainResource,
                            Wait)) {
                    _SEH2_LEAVE;
                }
                FcbMainResourceAcquired = TRUE;
            }

            RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));

            /*
            typedef struct _FILE_BASIC_INFORMATION {
            LARGE_INTEGER   CreationTime;
            LARGE_INTEGER   LastAccessTime;
            LARGE_INTEGER   LastWriteTime;
            LARGE_INTEGER   ChangeTime;
            ULONG           FileAttributes;
            } FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
            */

            Buffer->CreationTime = Mcb->CreationTime;
            Buffer->LastAccessTime = Mcb->LastAccessTime;
            Buffer->LastWriteTime = Mcb->LastWriteTime;
            Buffer->ChangeTime = Mcb->ChangeTime;
            Buffer->FileAttributes = Mcb->FileAttr;
            if (Buffer->FileAttributes == 0) {
                Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }

            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
            IoStatus->Status = STATUS_SUCCESS;

            Status =  TRUE;

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        if (FcbMainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG

    if (Status == FALSE) {

        DEBUG(DL_ERR, ("Ext2FastIoQueryBasicInfo: %s %s Status: FALSE ***\n",
                       Ext2GetCurrentProcessName(),
                       "FASTIO_QUERY_BASIC_INFO"));

    } else if (IoStatus->Status != STATUS_SUCCESS) {

        DEBUG(DL_ERR, (
                  "Ext2FastIoQueryBasicInfo: %s %s Status: %#x ***\n",
                  Ext2FastIoQueryBasicInfo,
                  "FASTIO_QUERY_BASIC_INFO",
                  IoStatus->Status
              ));
    }
#endif

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoQueryStandardInfo (
    IN PFILE_OBJECT                 FileObject,
    IN BOOLEAN                      Wait,
    OUT PFILE_STANDARD_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK            IoStatus,
    IN PDEVICE_OBJECT               DeviceObject
)
{

    BOOLEAN     Status = FALSE;
    PEXT2_VCB   Vcb = NULL;
    PEXT2_FCB   Fcb = NULL;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB)  {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoQueryStandardInfo: %s %s %wZ\n",
                      Ext2GetCurrentProcessName(),
                      "FASTIO_QUERY_STANDARD_INFO",
                      &Fcb->Mcb->FullName ));
#endif
            Vcb = Fcb->Vcb;

            if (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE)) {
                if (!ExAcquireResourceSharedLite(
                            &Fcb->MainResource,
                            Wait        )) {
                    _SEH2_LEAVE;
                }
                FcbMainResourceAcquired = TRUE;
            }

            RtlZeroMemory(Buffer, sizeof(FILE_STANDARD_INFORMATION));

            /*
            typedef struct _FILE_STANDARD_INFORMATION {
            LARGE_INTEGER   AllocationSize;
            LARGE_INTEGER   EndOfFile;
            ULONG           NumberOfLinks;
            BOOLEAN         DeletePending;
            BOOLEAN         Directory;
            } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
            */

            Buffer->NumberOfLinks = Fcb->Inode->i_nlink;
            Buffer->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);

            if (IsDirectory(Fcb)) {
                Buffer->Directory = IsDirectory(Fcb);
                Buffer->AllocationSize.QuadPart = 0;
                Buffer->EndOfFile.QuadPart = 0;
            } else {
                Buffer->Directory = FALSE;
                Buffer->AllocationSize = Fcb->Header.AllocationSize;
                Buffer->EndOfFile = Fcb->Header.FileSize;
            }

            IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);
            IoStatus->Status = STATUS_SUCCESS;
#if EXT2_DEBUG
            DEBUG(DL_INF, ( "Ext2FastIoQueryStandInfo: AllocatieonSize = %I64xh FileSize = %I64xh\n",
                            Buffer->AllocationSize.QuadPart, Buffer->EndOfFile.QuadPart));
#endif
            Status =  TRUE;

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        if (FcbMainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG
    if (Status == FALSE) {
        DEBUG(DL_INF, (
                  "Ext2FastIoQueryStandardInfo: %s %s Status: FALSE ***\n",
                  Ext2GetCurrentProcessName(),
                  "FASTIO_QUERY_STANDARD_INFO"            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        DEBUG(DL_INF, (
                  "Ext2FastIoQueryStandardInfo: %s %s Status: %#x ***\n",
                  Ext2GetCurrentProcessName(),
                  "FASTIO_QUERY_STANDARD_INFO",
                  IoStatus->Status            ));
    }
#endif

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoLock (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately,
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoLock: %s %s %wZ\n",
                      Ext2GetCurrentProcessName(),
                      "FASTIO_LOCK",
                      &Fcb->Mcb->FullName        ));

            DEBUG(DL_INF, (
                      "Ext2FastIoLock: Offset: %I64xh Length: %I64xh Key: %u %s%s\n",
                      FileOffset->QuadPart,
                      Length->QuadPart,
                      Key,
                      (FailImmediately ? "FailImmediately " : ""),
                      (ExclusiveLock ? "ExclusiveLock " : "") ));
#endif

            if (!FsRtlOplockIsFastIoPossible(&Fcb->Oplock)) {
                _SEH2_LEAVE;
            }

            Status = FsRtlFastLock(
                         &Fcb->FileLockAnchor,
                         FileObject,
                         FileOffset,
                         Length,
                         Process,
                         Key,
                         FailImmediately,
                         ExclusiveLock,
                         IoStatus,
                         NULL,
                         FALSE);

            if (Status) {
                Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
            }

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG
    if (Status == FALSE) {
        DEBUG(DL_ERR, (
                  "Ext2FastIoLock: %s %s *** Status: FALSE ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_LOCK"
              ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        DEBUG(DL_ERR, (
                  "Ext2FastIoLock: %s %s *** Status: %s (%#x) ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_LOCK",
                  Ext2NtStatusToString(IoStatus->Status),
                  IoStatus->Status
              ));
    }
#endif

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoUnlockSingle (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoUnlockSingle: %s %s %wZ\n",
                      (PUCHAR) Process + ProcessNameOffset,
                      "FASTIO_UNLOCK_SINGLE",
                      &Fcb->Mcb->FullName        ));

            DEBUG(DL_INF, (
                      "Ext2FastIoUnlockSingle: Offset: %I64xh Length: %I64xh Key: %u\n",
                      FileOffset->QuadPart,
                      Length->QuadPart,
                      Key     ));
#endif

            if (!FsRtlOplockIsFastIoPossible(&Fcb->Oplock)) {
                _SEH2_LEAVE;
            }

            IoStatus->Status = FsRtlFastUnlockSingle(
                                   &Fcb->FileLockAnchor,
                                   FileObject,
                                   FileOffset,
                                   Length,
                                   Process,
                                   Key,
                                   NULL,
                                   FALSE);

            IoStatus->Information = 0;
            Status =  TRUE;

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG
    if (Status == FALSE) {

        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockSingle: %s %s *** Status: FALSE ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_SINGLE"          ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockSingle: %s %s *** Status: %s (%#x) ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_SINGLE",
                  Ext2NtStatusToString(IoStatus->Status),
                  IoStatus->Status            ));
    }
#endif

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoUnlockAll (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoUnlockSingle: %s %s %wZ\n",
                      (PUCHAR) Process + ProcessNameOffset,
                      "FASTIO_UNLOCK_ALL",
                      &Fcb->Mcb->FullName
                  ));
#endif

            if (!FsRtlOplockIsFastIoPossible(&Fcb->Oplock)) {
                _SEH2_LEAVE;
            }

            IoStatus->Status = FsRtlFastUnlockAll(
                                   &Fcb->FileLockAnchor,
                                   FileObject,
                                   Process,
                                   NULL        );

            IoStatus->Information = 0;
            Status =  TRUE;

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG
    if (Status == FALSE) {

        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockSingle: %s %s *** Status: FALSE ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_ALL"
              ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockSingle: %s %s *** Status: %s (%#x) ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_ALL",
                  Ext2NtStatusToString(IoStatus->Status),
                  IoStatus->Status
              ));
    }
#endif

    return Status;
}

BOOLEAN NTAPI
Ext2FastIoUnlockAllByKey (
    IN PFILE_OBJECT         FileObject,
#ifdef __REACTOS__
    IN PVOID                Process,
#else
    IN PEPROCESS            Process,
#endif
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        _SEH2_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

#if EXT2_DEBUG
            DEBUG(DL_INF, (
                      "Ext2FastIoUnlockAllByKey: %s %s %wZ\n",
                      (PUCHAR) Process + ProcessNameOffset,
                      "FASTIO_UNLOCK_ALL_BY_KEY",
                      &Fcb->Mcb->FullName
                  ));

            DEBUG(DL_INF, (
                      "Ext2FastIoUnlockAllByKey: Key: %u\n",
                      Key
                  ));
#endif

            if (!FsRtlOplockIsFastIoPossible(&Fcb->Oplock)) {
                _SEH2_LEAVE;
            }

            IoStatus->Status = FsRtlFastUnlockAllByKey(
                                   &Fcb->FileLockAnchor,
                                   FileObject,
                                   Process,
                                   Key,
                                   NULL
                               );

            IoStatus->Information = 0;
            Status =  TRUE;

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if EXT2_DEBUG
    if (Status == FALSE) {

        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockAllByKey: %s %s *** Status: FALSE ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_ALL_BY_KEY"
              ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {

        DEBUG(DL_ERR, (
                  "Ext2FastIoUnlockAllByKey: %s %s *** Status: %s (%#x) ***\n",
                  (PUCHAR) Process + ProcessNameOffset,
                  "FASTIO_UNLOCK_ALL_BY_KEY",
                  Ext2NtStatusToString(IoStatus->Status),
                  IoStatus->Status
              ));
    }
#endif

    return Status;
}


BOOLEAN NTAPI
Ext2FastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT         FileObject,
    IN BOOLEAN              Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION PFNOI,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
)
{
    PEXT2_FCB   Fcb = NULL;
    PEXT2_CCB   Ccb = NULL;
    PEXT2_MCB   Mcb = NULL;

    BOOLEAN     bResult = FALSE;
    BOOLEAN FcbResourceAcquired = FALSE;

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        if (IsExt2FsDevice(DeviceObject)) {
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Fcb = (PEXT2_FCB) FileObject->FsContext;
        if (Fcb == NULL || Fcb->Identifier.Type == EXT2VCB) {
            DbgBreak();
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        Mcb = Fcb->Mcb;

#if EXT2_DEBUG
        DEBUG(DL_INF, (
                  "%-31s %wZ\n",
                  "FASTIO_QUERY_NETWORK_OPEN_INFO",
                  &Fcb->Mcb->FullName
              ));
#endif

        if (!Ccb) {
            _SEH2_LEAVE;
        }

        if (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE)) {

            if (!ExAcquireResourceSharedLite(
                        &Fcb->MainResource,
                        Wait
                    )) {
                _SEH2_LEAVE;
            }

            FcbResourceAcquired = TRUE;
        }

        if (IsDirectory(Fcb)) {
            PFNOI->AllocationSize.QuadPart = 0;
            PFNOI->EndOfFile.QuadPart = 0;
        } else {
            PFNOI->AllocationSize = Fcb->Header.AllocationSize;
            PFNOI->EndOfFile      = Fcb->Header.FileSize;
        }

        PFNOI->FileAttributes = Mcb->FileAttr;
        if (PFNOI->FileAttributes == 0) {
            PFNOI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        PFNOI->CreationTime   = Mcb->CreationTime;
        PFNOI->LastAccessTime = Mcb->LastAccessTime;
        PFNOI->LastWriteTime  = Mcb->LastWriteTime;
        PFNOI->ChangeTime     = Mcb->ChangeTime;

        bResult = TRUE;

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        FsRtlExitFileSystem();
    } _SEH2_END;

    return bResult;
}


VOID NTAPI
Ext2AcquireForCreateSection (
    IN PFILE_OBJECT FileObject
)

{
    PEXT2_FCB Fcb = FileObject->FsContext;

    if (Fcb->Header.Resource != NULL) {
        ExAcquireResourceExclusiveLite(Fcb->Header.Resource, TRUE);
    }

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2AcquireForCreateSection:  Fcb=%p\n", Fcb));
}

VOID NTAPI
Ext2ReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
)
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2ReleaseForCreateSection:  Fcb=%p\n", Fcb));

    if (Fcb->Header.Resource != NULL) {
        ExReleaseResourceLite(Fcb->Header.Resource);
    }
}


NTSTATUS NTAPI
Ext2AcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
)

{
    BOOLEAN ResourceAcquired = FALSE;

    PEXT2_FCB Fcb = FileObject->FsContext;

    *ResourceToRelease = Fcb->Header.Resource;
    ResourceAcquired = ExAcquireResourceExclusiveLite(*ResourceToRelease, FALSE);
    if (!ResourceAcquired) {
        *ResourceToRelease = NULL;
    }

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2AcquireFileForModWrite:  Fcb=%p Acquired=%d\n",
                             Fcb, ResourceAcquired));

    return (ResourceAcquired ? STATUS_SUCCESS : STATUS_CANT_WAIT);
}

NTSTATUS NTAPI
Ext2ReleaseFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
)
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2ReleaseFileForModWrite: Fcb=%p\n", Fcb));

    if (ResourceToRelease != NULL) {
        ASSERT(ResourceToRelease == Fcb->Header.Resource);
        ExReleaseResourceLite(ResourceToRelease);
    } else {
        DbgBreak();
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
Ext2AcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
)
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    if (Fcb->Header.Resource != NULL) {
        ExAcquireResourceExclusiveLite(Fcb->Header.Resource, TRUE);
    }

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2AcquireFileForCcFlush: Fcb=%p\n", Fcb));

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
Ext2ReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
)
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(FASTIO_DEBUG_LEVEL, ("Ext2ReleaseFileForCcFlush: Fcb=%p\n", Fcb));

    if (Fcb->Header.Resource != NULL) {
        ExReleaseResourceLite(Fcb->Header.Resource);
    }

    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
Ext2PreAcquireForCreateSection(
    IN PFS_FILTER_CALLBACK_DATA cd,
    OUT PVOID *cc
    )
{
    PEXT2_FCB Fcb = (PEXT2_FCB)cd->FileObject->FsContext;
    NTSTATUS        status;

    ASSERT(cd->Operation == FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION);
    ExAcquireResourceExclusiveLite(Fcb->Header.Resource, TRUE);
    if (cd->Parameters.AcquireForSectionSynchronization.SyncType != SyncTypeCreateSection) {
        status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
    } else if (Fcb->ShareAccess.Writers == 0) {
        status = STATUS_FILE_LOCKED_WITH_ONLY_READERS;
    } else {
        status = STATUS_FILE_LOCKED_WITH_WRITERS;
    }

    return status;
}
