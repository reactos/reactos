/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             cleanup.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   12 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2CleanupFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PEXT2_VCB Vcb,
    IN PEXT2_FCB Fcb,
    IN BOOLEAN VcbResourceAcquired,
    IN BOOLEAN FcbResourceAcquired,
    IN BOOLEAN FcbPagingIoResourceAcquired    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2Cleanup)
#endif


/* FUNCTIONS ***************************************************************/


_SEH_DEFINE_LOCALS(Ext2CleanupFinal)
{
    PEXT2_IRP_CONTEXT       IrpContext;
    PNTSTATUS               pStatus;
    PEXT2_VCB               Vcb;
    PEXT2_FCB               Fcb;
    BOOLEAN                 VcbResourceAcquired;
    BOOLEAN                 FcbResourceAcquired;
    BOOLEAN                 FcbPagingIoResourceAcquired;
};

_SEH_FINALLYFUNC(Ext2CleanupFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2CleanupFinal);
    Ext2CleanupFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus), _SEH_VAR(Vcb),
                     _SEH_VAR(Fcb), _SEH_VAR(VcbResourceAcquired),
                     _SEH_VAR(FcbResourceAcquired),
                     _SEH_VAR(FcbPagingIoResourceAcquired));
}

VOID
Ext2CleanupFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              VcbResourceAcquired,
    IN BOOLEAN              FcbResourceAcquired,
    IN BOOLEAN              FcbPagingIoResourceAcquired
    )
{
    if (FcbPagingIoResourceAcquired) {
        ExReleaseResourceLite(&Fcb->PagingIoResource);
    }

    if (FcbResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }
        
    if (VcbResourceAcquired) {
        ExReleaseResourceLite(&Vcb->MainResource);
    }
        
    if (!IrpContext->ExceptionInProgress) {
        if (*pStatus == STATUS_PENDING) {
            Ext2QueueRequest(IrpContext);
        } else {
            IrpContext->Irp->IoStatus.Status = *pStatus;
            Ext2CompleteIrpContext(IrpContext, *pStatus);
        }
    }
}

NTSTATUS
Ext2Cleanup (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    PEXT2_CCB       Ccb;
    PIRP            Irp;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2CleanupFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(VcbResourceAcquired) = FALSE;
        _SEH_VAR(FcbResourceAcquired) = FALSE;
        _SEH_VAR(FcbPagingIoResourceAcquired) = FALSE;

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject))  {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        Irp = IrpContext->Irp;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        _SEH_VAR(Vcb) = Vcb;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!IsFlagOn(Vcb->Flags, VCB_INITIALIZED)) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        _SEH_VAR(Fcb) = Fcb;
        if (!Fcb || (Fcb->Identifier.Type != EXT2VCB &&
                     Fcb->Identifier.Type != EXT2FCB)) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        if (IsFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE)) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        _SEH_VAR(VcbResourceAcquired) = 
            ExAcquireResourceExclusiveLite(
                 &Vcb->MainResource,
                 IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                 );

        if (Fcb->Identifier.Type == EXT2VCB) {

            if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
                (Vcb->LockFile == FileObject) ) {
                ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
                Vcb->LockFile = NULL;

                Ext2ClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
            }

            Ext2DerefXcb(&Vcb->OpenHandleCount);

            if (!Vcb->OpenHandleCount) {
                IoRemoveShareAccess(FileObject, &Vcb->ShareAccess);
            }

            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        if (IsFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE)) {
            if (IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
                IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK) &&
               !IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED) ) {
                Status = Ext2FlushFile(IrpContext, Fcb, Ccb);
            }
            _SEH_LEAVE;
        }

        if (Ccb == NULL) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        if (IsDirectory(Fcb)) {
            if (IsFlagOn(Fcb->Flags, FCB_DELETE_ON_CLOSE))  {
                SetLongFlag(Fcb->Flags, FCB_DELETE_PENDING);

                FsRtlNotifyFullChangeDirectory(
                                            Vcb->NotifySync,
                                            &Vcb->NotifyList,
                                            Fcb,
                                            NULL,
                                            FALSE,
                                            FALSE,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL );
            }

            FsRtlNotifyCleanup(Vcb->NotifySync, &Vcb->NotifyList, Ccb);

        }

        ExReleaseResourceLite(&Vcb->MainResource);
        _SEH_VAR(VcbResourceAcquired) = FALSE;

        _SEH_VAR(FcbResourceAcquired) = 
            ExAcquireResourceExclusiveLite(
                 &Fcb->MainResource,
                 IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                 );

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));        

        Ext2DerefXcb(&Vcb->OpenFileHandleCount);
        Ext2DerefXcb(&Fcb->OpenHandleCount);

        if (IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED)) {
            Fcb->Mcb->FileAttr |= FILE_ATTRIBUTE_ARCHIVE;
        }

        if (!IsDirectory(Fcb)) {

            if ( IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) && 
                !IsFlagOn(Ccb->Flags, CCB_LAST_WRITE_UPDATED)) {

                LARGE_INTEGER   SysTime;
                KeQuerySystemTime(&SysTime);

                Fcb->Inode->i_atime = 
                Fcb->Inode->i_mtime = Ext2LinuxTime(SysTime);
                Fcb->Mcb->LastAccessTime = 
                Fcb->Mcb->LastWriteTime = Ext2NtTime(Fcb->Inode->i_atime);

                Ext2SaveInode( IrpContext, Vcb,  
                               Fcb->Mcb->iNo,
                               Fcb->Inode );

                Ext2NotifyReportChange(
                        IrpContext,
                        Vcb,
                        Fcb->Mcb,
                        FILE_NOTIFY_CHANGE_ATTRIBUTES |
                        FILE_NOTIFY_CHANGE_LAST_WRITE |
                        FILE_NOTIFY_CHANGE_LAST_ACCESS,
                        FILE_ACTION_MODIFIED );
            }

            FsRtlCheckOplock( &Fcb->Oplock,
                              Irp,
                              IrpContext,
                              NULL,
                              NULL );

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

            if (!IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED)) {
                Fcb->NonCachedOpenCount--;
            }

            if (IsFlagOn(Fcb->Flags, FCB_DELETE_ON_CLOSE))  {
                SetLongFlag(Fcb->Flags, FCB_DELETE_PENDING);
            }

            //
            // Drop any byte range locks this process may have on the file.
            //

            FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                IoGetRequestorProcess(Irp),
                NULL  );

            //
            // If there are no byte range locks owned by other processes on the
            // file the fast I/O read/write functions doesn't have to check for
            // locks so we set IsFastIoPossible to FastIoIsPossible again.
            //
            if (!FsRtlGetNextFileLock(&Fcb->FileLockAnchor, TRUE)) {
                if (Fcb->Header.IsFastIoPossible != FastIoIsPossible) {
                    DEBUG(DL_INF, (": %-16.16s %-31s %wZ\n",
                        Ext2GetCurrentProcessName(),
                        "FastIoIsPossible",
                        &Fcb->Mcb->FullName
                        ));

                    Fcb->Header.IsFastIoPossible = FastIoIsPossible;
                }
            }

            if (Fcb->OpenHandleCount == 0 && 
                (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE) || 
                 IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_WRITE)) ) {

                LARGE_INTEGER Size;
        
                ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
                _SEH_VAR(FcbPagingIoResourceAcquired) = TRUE;

                Size.QuadPart = CEILING_ALIGNED(ULONGLONG, 
                                    (ULONGLONG)Fcb->Mcb->FileSize.QuadPart,
                                    (ULONGLONG)BLOCK_SIZE);
                if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_WRITE)) {
                    Fcb->RealSize = Fcb->Header.AllocationSize;
                }
                if (Size.QuadPart < Fcb->RealSize.QuadPart) {
                    Fcb->Mcb->FileSize = Fcb->RealSize;
                    if (!IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                        Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &Size);
                        Fcb->Mcb->FileSize = Fcb->Header.FileSize;
                        Fcb->Header.AllocationSize = Size;
                        if (CcIsFileCached(FileObject)) {
                            CcSetFileSizes(FileObject, 
                                    (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                        }
                    }
                }
                ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE|FCB_ALLOC_IN_WRITE);
                Fcb->RealSize.QuadPart = 0;
                ExReleaseResourceLite(&Fcb->PagingIoResource);
                _SEH_VAR(FcbPagingIoResourceAcquired) = FALSE;
            }
        }

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {

            if (Fcb->OpenHandleCount == 0 || Ccb->SymLink) {

                PEXT2_MCB   Mcb = Fcb->Mcb;

                //
                // Ext2DeleteFile will acquire these lock inside
                //

                if (_SEH_VAR(FcbResourceAcquired)) {
                    ExReleaseResourceLite(&Fcb->MainResource);
                    _SEH_VAR(FcbResourceAcquired) = FALSE;
                }

                //
                //  Have to delete this file...
                //

                if (Ccb->SymLink) {
                    Mcb = Ccb->SymLink;
                }

                Status = Ext2DeleteFile(IrpContext, Vcb, Mcb);

                if (NT_SUCCESS(Status)) {
                    if (IsMcbDirectory(Mcb)) {
                        Ext2NotifyReportChange( IrpContext, Vcb, Mcb,
                                                FILE_NOTIFY_CHANGE_DIR_NAME,
                                                FILE_ACTION_REMOVED );
                    } else {
                        Ext2NotifyReportChange( IrpContext, Vcb, Mcb,
                                                FILE_NOTIFY_CHANGE_FILE_NAME,
                                                FILE_ACTION_REMOVED );
                    }
                }

                if (Ccb->SymLink) {
                    ClearFlag(Fcb->Flags, FCB_DELETE_PENDING);
                    ClearFlag(Fcb->Flags, FCB_DELETE_ON_CLOSE);
                    ClearFlag(Fcb->Flags, FCB_FILE_DELETED);
                    FileObject->DeletePending = FALSE;
                }

                //
                // re-acquire the main resource lock 
                //

                _SEH_VAR(FcbResourceAcquired) = 
                    ExAcquireResourceExclusiveLite(
                         &Fcb->MainResource,
                         IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                         );

                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, 
                            (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                }
            }
        }

        if (!IsDirectory(Fcb)) {

            if ( IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED) &&
                 (Fcb->NonCachedOpenCount + 1 == Fcb->ReferenceCount) &&
                 (Fcb->SectionObject.DataSectionObject != NULL)) {

                if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
                    !IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED) ) {
                    CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
                }

                if (ExAcquireResourceExclusiveLite(&(Fcb->PagingIoResource), TRUE)) {
                    ExReleaseResourceLite(&(Fcb->PagingIoResource));
                }

                CcPurgeCacheSection( &Fcb->SectionObject,
                                     NULL,
                                     0,
                                     FALSE );
            }

            CcUninitializeCacheMap(FileObject, NULL, NULL);
        }

        if (!Fcb->OpenHandleCount) {
            IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
        }

        DEBUG(DL_INF, ( "Ext2Cleanup: OpenCount=%u ReferCount=%u NonCahcedCount=%xh %wZ\n",
            Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount, &Fcb->Mcb->FullName));

        Status = STATUS_SUCCESS;

        if (FileObject) {
            SetFlag(FileObject->Flags, FO_CLEANUP_COMPLETE);
        }

    }
    _SEH_FINALLY(Ext2CleanupFinal_PSEH)
    _SEH_END;
    
    return Status;
}
