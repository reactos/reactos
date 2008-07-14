/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fastio.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   14 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

/* Also used by Ext2FastIoQueryStandardInfo() */
/* Also used by Ext2FastIoQueryNetworkOpenInfo() */
VOID
Ext2FastIoQueryBasicInfoFinal (
    IN PEXT2_FCB Fcb,
    IN BOOLEAN   FcbMainResourceAcquired    );

#ifdef ALLOC_PRAGMA
#if EXT2_DEBUG
#pragma alloc_text(PAGE, Ext2FastIoRead)
#pragma alloc_text(PAGE, Ext2FastIoWrite)
#endif
#pragma alloc_text(PAGE, Ext2FastIoCheckIfPossible)
#pragma alloc_text(PAGE, Ext2FastIoQueryBasicInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryStandardInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, Ext2FastIoLock)
#pragma alloc_text(PAGE, Ext2FastIoUnlockSingle)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#endif


/* FUNCTIONS ***************************************************************/


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
        if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
            IsPossible = FastIoIsPossible;
        }
    }

    return IsPossible;
}

_SEH_WRAP_FINALLY(FsRtlExitFileSystem_PSEH_finally, FsRtlExitFileSystem);

BOOLEAN
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
    
    _SEH_TRY {

       FsRtlEnterFileSystem();

        _SEH_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                _SEH_LEAVE;
            }

            Fcb = (PEXT2_FCB) FileObject->FsContext;

            if (Fcb == NULL) {
                _SEH_LEAVE;
            }

            if (Fcb->Identifier.Type == EXT2VCB) {
                _SEH_LEAVE;
            }

            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                _SEH_LEAVE;
            }

            Ccb = (PEXT2_CCB) FileObject->FsContext2;
            if (Ccb == NULL) {
                _SEH_LEAVE;
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

                if (!(IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) ||
                      IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED))) {
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

        } 
        _SEH_HANDLE {
            bPossible = FastIoIsNotPossible;
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;
    
    return bPossible;
}


#if EXT2_DEBUG
BOOLEAN
Ext2FastIoRead (IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN ULONG                Length,
           IN BOOLEAN              Wait,
           IN ULONG                LockKey,
           OUT PVOID               Buffer,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) FileObject->FsContext;
    
    if (Fcb == NULL) {
        return Status;
    }

    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(DL_INF, ( "Ext2FastIoRead: %s %s %wZ\n",
        Ext2GetCurrentProcessName(),
        "FASTIO_READ",
        &Fcb->Mcb->FullName     ));

    DEBUG(DL_INF, ( "Ext2FastIoRead: Offset: %I64xh Length: %xh Key: %u\n",
        FileOffset->QuadPart,
        Length,
        LockKey       ));

    Status = FsRtlCopyRead (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return Status;
}

BOOLEAN
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
    BOOLEAN      Status = FALSE;
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) FileObject->FsContext;
    
    if (Fcb == NULL) {
        return Status;
    }
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(DL_INF, (
        "Ext2FastIoWrite: %s %s %wZ\n",
        Ext2GetCurrentProcessName(),
        "FASTIO_WRITE",
        &Fcb->Mcb->FullName     ));

    DEBUG(DL_INF, (
        "Ext2FastIoWrite: Offset: %I64xh Length: %xh Key: %xh\n",
        FileOffset->QuadPart,
        Length,
        LockKey       ));

    if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        return FALSE;
    }

    Status = FsRtlCopyWrite (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return Status;
}

#endif /* EXT2_DEBUG */

_SEH_DEFINE_LOCALS(Ext2FastIoQueryBasicInfoFinal)
{
    PEXT2_FCB            Fcb;
    BOOLEAN              FcbMainResourceAcquired;
};

_SEH_FINALLYFUNC(Ext2FastIoQueryBasicInfoFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2FastIoQueryBasicInfoFinal);
    Ext2FastIoQueryBasicInfoFinal(_SEH_VAR(Fcb), _SEH_VAR(FcbMainResourceAcquired));
}

VOID
Ext2FastIoQueryBasicInfoFinal (
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              FcbMainResourceAcquired
    )
{
    if (FcbMainResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }
        
    FsRtlExitFileSystem();
}

BOOLEAN
Ext2FastIoQueryBasicInfo (
              IN PFILE_OBJECT             FileObject,
              IN BOOLEAN                  Wait,
              OUT PFILE_BASIC_INFORMATION Buffer,
              OUT PIO_STATUS_BLOCK        IoStatus,
              IN PDEVICE_OBJECT           DeviceObject)
{
    PEXT2_FCB   Fcb = NULL;
    BOOLEAN     Status = FALSE;
    
    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            _SEH_DECLARE_LOCALS(Ext2FastIoQueryBasicInfoFinal);
            _SEH_VAR(Fcb) = NULL;
            _SEH_VAR(FcbMainResourceAcquired) = FALSE;

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            _SEH_VAR(Fcb) = Fcb;
            
            if (Fcb == NULL) {
                _SEH_LEAVE;
            }
            
            if (Fcb->Identifier.Type == EXT2VCB) {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
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
                    Status = FALSE;
                    _SEH_LEAVE;
                }
                _SEH_VAR(FcbMainResourceAcquired) = TRUE;
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

            if (IsRoot(Fcb)) {
                Buffer->CreationTime =
                Buffer->LastAccessTime =
                Buffer->LastWriteTime =
                Buffer->ChangeTime = Ext2NtTime(0);
            } else {
                Buffer->CreationTime = Fcb->Mcb->CreationTime;
                Buffer->LastAccessTime = Fcb->Mcb->LastAccessTime;
                Buffer->LastWriteTime = Fcb->Mcb->LastWriteTime;
                Buffer->ChangeTime = Fcb->Mcb->ChangeTime;
            }

            Buffer->FileAttributes = Fcb->Mcb->FileAttr;
            if (Buffer->FileAttributes == 0) {
                Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
            }
            
            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;
        }
        _SEH_HANDLE {

            IoStatus->Status = _SEH_GetExceptionCode();
            Status = TRUE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(Ext2FastIoQueryBasicInfoFinal_PSEH)
    _SEH_END;
    
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

_SEH_DEFINE_LOCALS(Ext2FastIoQueryStandardInfoFinal)
{
    PEXT2_FCB            Fcb;
    BOOLEAN              FcbMainResourceAcquired;
};

/* Use Ext2FastIoQueryBasicInfo() PSEH final function */
_SEH_FINALLYFUNC(Ext2FastIoQueryStandardInfoFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2FastIoQueryStandardInfoFinal);
    Ext2FastIoQueryBasicInfoFinal(_SEH_VAR(Fcb), _SEH_VAR(FcbMainResourceAcquired));
}

BOOLEAN
Ext2FastIoQueryStandardInfo (
                IN PFILE_OBJECT                 FileObject,
                IN BOOLEAN                      Wait,
                OUT PFILE_STANDARD_INFORMATION  Buffer,
                OUT PIO_STATUS_BLOCK            IoStatus,
                IN PDEVICE_OBJECT               DeviceObject
                )
{
    
    BOOLEAN     Status = FALSE;
    PEXT2_VCB   Vcb;
    PEXT2_FCB   Fcb;

    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            _SEH_DECLARE_LOCALS(Ext2FastIoQueryStandardInfoFinal);
            _SEH_VAR(FcbMainResourceAcquired) = FALSE;

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            _SEH_VAR(Fcb) = Fcb;
            
            if (Fcb == NULL) {
                _SEH_LEAVE;
            }
            
            if (Fcb->Identifier.Type == EXT2VCB)  {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
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
                    Status = FALSE;
                    _SEH_LEAVE;
                }
                _SEH_VAR(FcbMainResourceAcquired) = TRUE;
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

            Buffer->NumberOfLinks = Fcb->Inode->i_links_count;
            
            if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
                Buffer->DeletePending = FALSE;
            } else {
                Buffer->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
            }
            
            if (IsDirectory(Fcb)) {
                Buffer->Directory = TRUE;
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

        }
        _SEH_HANDLE {

            IoStatus->Status = _SEH_GetExceptionCode();
            Status = TRUE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(Ext2FastIoQueryStandardInfoFinal_PSEH)
    _SEH_END;

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

BOOLEAN
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

    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            if (Fcb == NULL) {
                _SEH_LEAVE;
            }
            
            if (Fcb->Identifier.Type == EXT2VCB) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
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
                Status = FALSE;
                _SEH_LEAVE;
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

        }
        _SEH_HANDLE {
            IoStatus->Status = _SEH_GetExceptionCode();
            Status = FALSE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;

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

BOOLEAN
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

    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            if (Fcb == NULL) {
                _SEH_LEAVE;
            }

            if (Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {

                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
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
                Status = FALSE;
                _SEH_LEAVE;
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

        }
        _SEH_HANDLE {
            IoStatus->Status = _SEH_GetExceptionCode();
            Status = FALSE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;

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

BOOLEAN
Ext2FastIoUnlockAll (
            IN PFILE_OBJECT         FileObject,
            IN PEPROCESS            Process,
            OUT PIO_STATUS_BLOCK    IoStatus,
            IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;

            if (Fcb == NULL) {
                _SEH_LEAVE;
            }

            if (Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
 
            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
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
                Status = FALSE;
                _SEH_LEAVE;
            }
            
            IoStatus->Status = FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                Process,
                NULL        );
            
            IoStatus->Information = 0;
            Status =  TRUE;

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

        }
        _SEH_HANDLE {
            IoStatus->Status = _SEH_GetExceptionCode();
            Status = FALSE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;

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

BOOLEAN
Ext2FastIoUnlockAllByKey (
             IN PFILE_OBJECT         FileObject,
             IN PEPROCESS            Process,
             IN ULONG                Key,
             OUT PIO_STATUS_BLOCK    IoStatus,
             IN PDEVICE_OBJECT       DeviceObject
             )
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    _SEH_TRY {

        FsRtlEnterFileSystem();

        _SEH_TRY {

            if (IsExt2FsDevice(DeviceObject)) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;

            if (Fcb == NULL) {
                _SEH_LEAVE;
            }

            if (Fcb->Identifier.Type == EXT2VCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

            if (IsDirectory(Fcb)) {

                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH_LEAVE;
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
                Status = FALSE;
                _SEH_LEAVE;
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

        }
        _SEH_HANDLE {
            IoStatus->Status = _SEH_GetExceptionCode();
            Status = FALSE;
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;

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

_SEH_DEFINE_LOCALS(Ext2FastIoQueryNetworkOpenInfoFinal)
{
    PEXT2_FCB            Fcb;
    BOOLEAN              FcbMainResourceAcquired;
};

/* Use Ext2FastIoQueryBasicInfo() PSEH final function */
_SEH_FINALLYFUNC(Ext2FastIoQueryNetworkOpenInfoFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2FastIoQueryNetworkOpenInfoFinal);
    Ext2FastIoQueryBasicInfoFinal(_SEH_VAR(Fcb), _SEH_VAR(FcbMainResourceAcquired));
}

BOOLEAN
Ext2FastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT         FileObject,
    IN BOOLEAN              Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION PFNOI,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     bResult = FALSE;

    PEXT2_FCB   Fcb = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2FastIoQueryNetworkOpenInfoFinal);
        _SEH_VAR(Fcb) = NULL;
        _SEH_VAR(FcbMainResourceAcquired) = FALSE;

        FsRtlEnterFileSystem();

        if (IsExt2FsDevice(DeviceObject)) {
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }
            
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        _SEH_VAR(Fcb) = Fcb;
            
        if (Fcb == NULL) {
            _SEH_LEAVE;
        }
            
        if (Fcb->Identifier.Type == EXT2VCB) {
            DbgBreak();
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }
            
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

#if EXT2_DEBUG
        DEBUG(DL_INF, ( 
                "%-31s %wZ\n",
                "FASTIO_QUERY_NETWORK_OPEN_INFO",
                &Fcb->Mcb->FullName
                ));
#endif

        if (FileObject->FsContext2) {
            _SEH_LEAVE;
        }

        if (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE)) {

            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait
                )) {
                _SEH_LEAVE;
            }
            
            _SEH_VAR(FcbMainResourceAcquired) = TRUE;
        }

        if (IsDirectory(Fcb)) {
            PFNOI->AllocationSize.QuadPart = 0;
            PFNOI->EndOfFile.QuadPart = 0;
        } else {
            PFNOI->AllocationSize = Fcb->Header.AllocationSize;
            PFNOI->EndOfFile      = Fcb->Header.FileSize;
        }

        PFNOI->FileAttributes = Fcb->Mcb->FileAttr;
        if (PFNOI->FileAttributes == 0) {
            PFNOI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        if (IsRoot(Fcb)) {
            PFNOI->CreationTime =
            PFNOI->LastAccessTime =
            PFNOI->LastWriteTime =
            PFNOI->ChangeTime = Ext2NtTime(0);
        } else {
            PFNOI->CreationTime   = Fcb->Mcb->CreationTime;
            PFNOI->LastAccessTime = Fcb->Mcb->LastAccessTime;
            PFNOI->LastWriteTime  = Fcb->Mcb->LastWriteTime;
            PFNOI->ChangeTime     = Fcb->Mcb->ChangeTime;
        }

        bResult = TRUE;

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

    }
    _SEH_FINALLY(Ext2FastIoQueryNetworkOpenInfoFinal_PSEH)
    _SEH_END;

    return bResult;
}
