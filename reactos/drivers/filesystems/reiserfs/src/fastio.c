/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             fastio.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#if DBG
#pragma alloc_text(PAGE, RfsdFastIoRead)
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdFastIoWrite)
#endif // !RFSD_READ_ONLY
#endif // DBG
#pragma alloc_text(PAGE, RfsdFastIoCheckIfPossible)
#pragma alloc_text(PAGE, RfsdFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, RfsdFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, RfsdFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, RfsdFastIoLock)
#pragma alloc_text(PAGE, RfsdFastIoUnlockSingle)
#pragma alloc_text(PAGE, RfsdFastIoUnlockAll)
#pragma alloc_text(PAGE, RfsdFastIoUnlockAllByKey)
#endif

BOOLEAN NTAPI
RfsdFastIoCheckIfPossible (
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
    PRFSD_FCB        Fcb;
    LARGE_INTEGER    lLength;

    PAGED_CODE();

    lLength.QuadPart = Length;
    
    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
            
            if (IsDirectory(Fcb)) {
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

                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) ||
                    IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED)) {
                    bPossible = FastIoIsNotPossible;
                } else {
                    bPossible = FsRtlFastCheckLockForWrite(
                        &Fcb->FileLockAnchor,
                        FileOffset,
                        &lLength,
                        LockKey,
                        FileObject,
                        PsGetCurrentProcess());
                }
            }

            RfsdPrint((DBG_INFO, "RfsdFastIoCheckIfPossible: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_CHECK_IF_POSSIBLE",
                Fcb->AnsiFileName.Buffer
                ));

            RfsdPrint((DBG_INFO, 
                "RfsdFastIoCheckIfPossible: Offset: %I64xg Length: %xh Key: %u %s %s\n",
                FileOffset->QuadPart,
                Length,
                LockKey,
                (CheckForReadOperation ? "CheckForReadOperation:" :
                                         "CheckForWriteOperation:"),
                (bPossible ? "Succeeded" : "Failed")));
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            bPossible = FastIoIsNotPossible;
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;
    
    return bPossible;
}

#if DBG

BOOLEAN NTAPI
RfsdFastIoRead (IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN ULONG                Length,
           IN BOOLEAN              Wait,
           IN ULONG                LockKey,
           OUT PVOID               Buffer,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status;
    PRFSD_FCB    Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) FileObject->FsContext;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO, "RfsdFastIoRead: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "FASTIO_READ",
        Fcb->AnsiFileName.Buffer     ));

    RfsdPrint((DBG_INFO, "RfsdFastIoRead: Offset: %I64xh Length: %xh Key: %u\n",
        FileOffset->QuadPart,
        Length,
        LockKey       ));

    Status = FsRtlCopyRead (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return Status;
}

#if !RFSD_READ_ONLY

NTAPI BOOLEAN
RfsdFastIoWrite (
           IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN ULONG                Length,
           IN BOOLEAN              Wait,
           IN ULONG                LockKey,
           OUT PVOID               Buffer,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN      bRet;
    PRFSD_FCB    Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) FileObject->FsContext;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO,
        "RfsdFastIoWrite: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "FASTIO_WRITE",
        Fcb->AnsiFileName.Buffer     ));

    RfsdPrint((DBG_INFO,
        "RfsdFastIoWrite: Offset: %I64xh Length: %xh Key: %xh\n",
        FileOffset->QuadPart,
        Length,
        LockKey       ));

    if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        DbgBreak();
        return FALSE;
    }

    bRet = FsRtlCopyWrite (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return bRet;
}

#endif // !RFSD_READ_ONLY

#endif /* DBG */

BOOLEAN NTAPI
RfsdFastIoQueryBasicInfo (IN PFILE_OBJECT             FileObject,
              IN BOOLEAN                  Wait,
              OUT PFILE_BASIC_INFORMATION Buffer,
              OUT PIO_STATUS_BLOCK        IoStatus,
              IN PDEVICE_OBJECT           DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PRFSD_FCB   Fcb = 0;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

            RfsdPrint((DBG_INFO, 
                "RfsdFastIoQueryBasicInfo: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_QUERY_BASIC_INFO",
                Fcb->AnsiFileName.Buffer
                ));
            
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait)) {
                Status = FALSE;
                _SEH2_LEAVE;
            }
            
            FcbMainResourceAcquired = TRUE;
            
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

            Buffer->CreationTime = RfsdSysTime(Fcb->Inode->i_ctime);
            Buffer->LastAccessTime = RfsdSysTime(Fcb->Inode->i_atime);
            Buffer->LastWriteTime = RfsdSysTime(Fcb->Inode->i_mtime);
            Buffer->ChangeTime = RfsdSysTime(Fcb->Inode->i_mtime);
            
            
            Buffer->FileAttributes = Fcb->RfsdMcb->FileAttr;
            
            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
            
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {

        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        FsRtlExitFileSystem();
    } _SEH2_END;
    
    
    if (Status == FALSE) {

        RfsdPrint((DBG_ERROR, 
            "RfsdFastIoQueryBasicInfo: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_QUERY_BASIC_INFO"
            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {

        RfsdPrint((DBG_ERROR, 
            "RfsdFastIoQueryBasicInfo: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_QUERY_BASIC_INFO",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }
    
    return Status;
}

BOOLEAN NTAPI
RfsdFastIoQueryStandardInfo (
                IN PFILE_OBJECT                 FileObject,
                IN BOOLEAN                      Wait,
                OUT PFILE_STANDARD_INFORMATION  Buffer,
                OUT PIO_STATUS_BLOCK            IoStatus,
                IN PDEVICE_OBJECT               DeviceObject
                )
{
    BOOLEAN     Status = FALSE;
    PRFSD_VCB   Vcb;
    PRFSD_FCB   Fcb = 0;
    BOOLEAN     FcbMainResourceAcquired = FALSE;
    LONGLONG    FileSize;
    LONGLONG    AllocationSize;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {

                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB)  {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

            RfsdPrint((DBG_INFO,
                "RfsdFastIoQueryStandardInfo: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_QUERY_STANDARD_INFO",
                Fcb->AnsiFileName.Buffer ));

            Vcb = Fcb->Vcb;
            
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait        )) {
                Status = FALSE;
                _SEH2_LEAVE;
            }
            
            FcbMainResourceAcquired = TRUE;
            
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

            FileSize  = (LONGLONG) Fcb->Inode->i_size;

#if DISABLED
            if (S_ISREG(Fcb->Inode->i_mode))
                FileSize |= ((LONGLONG)(Fcb->Inode->i_size_high) << 32);
#endif

            AllocationSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);		// TODO: This is incorrect for file tails...

            Buffer->AllocationSize.QuadPart = AllocationSize;
            Buffer->EndOfFile.QuadPart = FileSize;
            Buffer->NumberOfLinks = Fcb->Inode->i_links_count;
            
            if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
                Buffer->DeletePending = FALSE;
            } else {
                Buffer->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
            }
            
            if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
                Buffer->Directory = TRUE;
            } else {
                Buffer->Directory = FALSE;
            }
            
            IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);
            
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {
        if (FcbMainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        FsRtlExitFileSystem();
    } _SEH2_END;

#if DBG
    if (Status == FALSE) {
        RfsdPrint((DBG_INFO,
            "RfsdFastIoQueryStandardInfo: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO"            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        RfsdPrint((DBG_INFO,
            "RfsdFastIoQueryStandardInfo: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status            ));
    }
#endif
    
    return Status;
}

BOOLEAN NTAPI
RfsdFastIoLock (
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
    PRFSD_FCB   Fcb;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
            
            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }

            RfsdPrint((DBG_INFO,
                "RfsdFastIoLock: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_LOCK",
                Fcb->AnsiFileName.Buffer        ));

            RfsdPrint((DBG_INFO,
                "RfsdFastIoLock: Offset: %I64xh Length: %I64xh Key: %u %s%s\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key,
                (FailImmediately ? "FailImmediately " : ""),
                (ExclusiveLock ? "ExclusiveLock " : "") ));
            
            if (Fcb->Header.IsFastIoPossible != FastIoIsQuestionable) {
                RfsdPrint((DBG_INFO,
                    "RfsdFastIoLock: %s %s %s\n",
                    RfsdGetCurrentProcessName(),
                    "FastIoIsQuestionable",
                    Fcb->AnsiFileName.Buffer        ));

                Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
            }
            
#ifdef _MSC_VER
#pragma prefast( suppress: 28159, "bug in prefast" )
#endif
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
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {
        FsRtlExitFileSystem();
    } _SEH2_END;

#if DBG 
    if (Status == FALSE) {
        RfsdPrint((DBG_ERROR, 
            "RfsdFastIoLock: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_LOCK"
            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        RfsdPrint((DBG_ERROR,
            "RfsdFastIoLock: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_LOCK",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }
#endif
    
    return Status;
}

BOOLEAN NTAPI
RfsdFastIoUnlockSingle (
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
    PRFSD_FCB   Fcb;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
            
            if (IsDirectory(Fcb)) {

                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }

            RfsdPrint((DBG_INFO,
                "RfsdFastIoUnlockSingle: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_UNLOCK_SINGLE",
                Fcb->AnsiFileName.Buffer        ));

            RfsdPrint((DBG_INFO,
                "RfsdFastIoUnlockSingle: Offset: %I64xh Length: %I64xh Key: %u\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key     ));
            
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
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if DBG 
    if (Status == FALSE) {

        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockSingle: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_SINGLE"          ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockSingle: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_SINGLE",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status            ));
    }
#endif  

    return Status;
}

BOOLEAN NTAPI
RfsdFastIoUnlockAll (
            IN PFILE_OBJECT         FileObject,
            IN PEPROCESS            Process,
            OUT PIO_STATUS_BLOCK    IoStatus,
            IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PRFSD_FCB   Fcb;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
            
            if (IsDirectory(Fcb)) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }

            RfsdPrint((DBG_INFO,
                "RfsdFastIoUnlockAll: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_UNLOCK_ALL",
                Fcb->AnsiFileName.Buffer
                ));
            
            IoStatus->Status = FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                Process,
                NULL        );
            
            IoStatus->Information = 0;
            
            Status =  TRUE;
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;

#if DBG 
    if (Status == FALSE) {

        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockAll: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_ALL"
            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {
        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockAll: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_ALL",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }
#endif  

    return Status;
}

BOOLEAN NTAPI
RfsdFastIoUnlockAllByKey (
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
    PRFSD_FCB   Fcb;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            Fcb = (PRFSD_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == RFSDVCB) {
                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }
            
            ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
                (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
            
            if (IsDirectory(Fcb)) {

                DbgBreak();
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                _SEH2_LEAVE;
            }

            RfsdPrint((DBG_INFO,
                "RfsdFastIoUnlockAllByKey: %s %s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_UNLOCK_ALL_BY_KEY",
                Fcb->AnsiFileName.Buffer
                ));

            RfsdPrint((DBG_INFO,
                "RfsdFastIoUnlockAllByKey: Key: %u\n",
                Key
                ));
            
            IoStatus->Status = FsRtlFastUnlockAllByKey(
                &Fcb->FileLockAnchor,
                FileObject,
                Process,
                Key,
                NULL
                );  
            
            IoStatus->Information = 0;
            
            Status =  TRUE;

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

            IoStatus->Status = _SEH2_GetExceptionCode();
            Status = TRUE;
        } _SEH2_END;

    } _SEH2_FINALLY {
        FsRtlExitFileSystem();
    } _SEH2_END;

#if DBG 
    if (Status == FALSE) {

        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockAllByKey: %s %s *** Status: FALSE ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_ALL_BY_KEY"
            ));
    } else if (IoStatus->Status != STATUS_SUCCESS) {

        RfsdPrint((DBG_ERROR,
            "RfsdFastIoUnlockAllByKey: %s %s *** Status: %s (%#x) ***\n",
            RfsdGetCurrentProcessName(),
            "FASTIO_UNLOCK_ALL_BY_KEY",
            RfsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }
#endif  

    return Status;
}

BOOLEAN NTAPI
RfsdFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT         FileObject,
    IN BOOLEAN              Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION PFNOI,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     bResult = FALSE;
    PRFSD_FCB   Fcb = NULL;
    BOOLEAN FcbResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        FsRtlEnterFileSystem();

        if (DeviceObject == RfsdGlobal->DeviceObject) {
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
            
        Fcb = (PRFSD_FCB) FileObject->FsContext;
            
        ASSERT(Fcb != NULL);
            
        if (Fcb->Identifier.Type == RFSDVCB) {
            DbgBreak();
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
            
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
               (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        RfsdPrint((DBG_INFO, 
                "%-16.16s %-31s %s\n",
                RfsdGetCurrentProcessName(),
                "FASTIO_QUERY_NETWORK_OPEN_INFO",
                Fcb->AnsiFileName.Buffer
                ));

        if (FileObject->FsContext2) {
            _SEH2_LEAVE;
        }

        if (!FlagOn(Fcb->Flags, FCB_PAGE_FILE)) {

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
            PFNOI->EndOfFile.QuadPart      = 0;
        } else {
            PFNOI->AllocationSize = Fcb->Header.AllocationSize;
            PFNOI->EndOfFile      = Fcb->Header.FileSize;
        }

        PFNOI->FileAttributes = Fcb->RfsdMcb->FileAttr;
        if (PFNOI->FileAttributes == 0) {
            PFNOI->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        PFNOI->CreationTime   = RfsdSysTime(Fcb->Inode->i_ctime);
        PFNOI->LastAccessTime = RfsdSysTime(Fcb->Inode->i_atime);
        PFNOI->LastWriteTime  = RfsdSysTime(Fcb->Inode->i_mtime);
        PFNOI->ChangeTime     = RfsdSysTime(Fcb->Inode->i_mtime);

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
