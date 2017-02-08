/* 
 * FFS File System Driver for Windows
 *
 * cleanup.c
 *
 * 2004.5.6 ~
 *
 * Lee Jae-Hong, http://www.pyrasis.com
 *
 * See License.txt
 *
 */

#include "ntifs.h"
#include "ffsdrv.h"

/* Globals */

extern PFFS_GLOBAL FFSGlobal;

/* Definitions */

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSCleanup)
#endif


__drv_mustHoldCriticalRegion
NTSTATUS
FFSCleanup(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	NTSTATUS        Status = STATUS_SUCCESS;
	PFFS_VCB        Vcb = 0;
	BOOLEAN	        VcbResourceAcquired = FALSE;
	PFILE_OBJECT    FileObject;
	PFFS_FCB        Fcb = 0;
	BOOLEAN	        FcbResourceAcquired = FALSE;
	BOOLEAN	        FcbPagingIoAcquired = FALSE;
	PFFS_CCB        Ccb;
	PIRP            Irp;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;
		
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}
		
		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
			(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		if (!IsFlagOn(Vcb->Flags, VCB_INITIALIZED))
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
		if (!ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,
				IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		VcbResourceAcquired = TRUE;

		FileObject = IrpContext->FileObject;

		Fcb = (PFFS_FCB)FileObject->FsContext;

		if (!Fcb)
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		if (Fcb->Identifier.Type == FFSVCB)
		{
			if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
				(Vcb->LockFile == FileObject))
			{
				ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
				Vcb->LockFile = NULL;

				FFSClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
			}

			Vcb->OpenHandleCount--;

			if (!Vcb->OpenHandleCount)
			{
				IoRemoveShareAccess(FileObject, &Vcb->ShareAccess);
			}

			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

/*
		if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
			 !IsFlagOn(Fcb->Flags, FCB_PAGE_FILE))
*/
		{
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
			if (!ExAcquireResourceExclusiveLite(
					&Fcb->MainResource,
					IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			FcbResourceAcquired = TRUE;
		}
		
		Ccb = (PFFS_CCB)FileObject->FsContext2;

		if (!Ccb)
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		if (IsFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
		{
			if (IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
				 IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK) &&
				 !IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
			{
				Status = FFSFlushFile(Fcb);
			}

			_SEH2_LEAVE;
		}
		
		ASSERT((Ccb->Identifier.Type == FFSCCB) &&
			(Ccb->Identifier.Size == sizeof(FFS_CCB)));		
		Irp = IrpContext->Irp;

		Fcb->OpenHandleCount--;

		if (!IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED))
		{
			Fcb->NonCachedOpenCount--;
		}

		Vcb->OpenFileHandleCount--;

		if (IsFlagOn(Fcb->Flags, FCB_DELETE_ON_CLOSE))
		{
			SetFlag(Fcb->Flags, FCB_DELETE_PENDING);

			if (IsDirectory(Fcb))
			{
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
											NULL);
			}
		}

		if (IsDirectory(Fcb))
		{
			FsRtlNotifyCleanup(
				Vcb->NotifySync,
				&Vcb->NotifyList,
				Ccb);
		}
		else
		{
			//
			// Drop any byte range locks this process may have on the file.
			//
			FsRtlFastUnlockAll(
				&Fcb->FileLockAnchor,
				FileObject,
				IoGetRequestorProcess(Irp),
				NULL);

			//
			// If there are no byte range locks owned by other processes on the
			// file the fast I/O read/write functions doesn't have to check for
			// locks so we set IsFastIoPossible to FastIoIsPossible again.
			//
			if (!FsRtlGetNextFileLock(&Fcb->FileLockAnchor, TRUE))
			{
				if (Fcb->Header.IsFastIoPossible != FastIoIsPossible)
				{
					FFSPrint((
						DBG_INFO, ": %-16.16s %-31s %s\n",
						FFSGetCurrentProcessName(),
						"FastIoIsPossible",
						Fcb->AnsiFileName.Buffer));

					Fcb->Header.IsFastIoPossible = FastIoIsPossible;
				}
			}
		}

		if (IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED) &&
			 (Fcb->NonCachedOpenCount != 0) &&
			 (Fcb->NonCachedOpenCount == Fcb->ReferenceCount) &&
			 (Fcb->SectionObject.DataSectionObject != NULL))
		{

			if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
				!IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
			{
				CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
			}

			ExAcquireResourceExclusiveLite(&(Fcb->PagingIoResource), TRUE);
			ExReleaseResourceLite(&(Fcb->PagingIoResource));

			CcPurgeCacheSection(&Fcb->SectionObject,
								NULL,
								0,
								FALSE);
		}

#if !FFS_READ_ONLY
		if (Fcb->OpenHandleCount == 0)
		{
			if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING))
			{
				BOOLEAN	 bDeleted = FALSE;

				//
				//  Have to delete this file...
				//
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
				if (!ExAcquireResourceExclusiveLite(
						 &Fcb->PagingIoResource,
						 IrpContext->IsSynchronous))
				{
					Status = STATUS_PENDING;
					_SEH2_LEAVE;
				}

				FcbPagingIoAcquired = TRUE;

				bDeleted = FFSDeleteFile(IrpContext, Vcb, Fcb);

				if (bDeleted)
				{
					if (IsDirectory(Fcb))
					{
						FFSNotifyReportChange(IrpContext, Vcb, Fcb,
												FILE_NOTIFY_CHANGE_DIR_NAME,
												FILE_ACTION_REMOVED);
					}
					else
					{
						FFSNotifyReportChange(IrpContext, Vcb, Fcb,
												FILE_NOTIFY_CHANGE_FILE_NAME,
												FILE_ACTION_REMOVED);
					}
				}

				if (FcbPagingIoAcquired)
				{
					ExReleaseResourceForThreadLite(
						&Fcb->PagingIoResource,
						ExGetCurrentResourceThread());

					FcbPagingIoAcquired = FALSE;
				}

/*
				if (bDeleted)
				{
					FFSPurgeFile(Fcb, FALSE);
				}
*/
			}
		}
#endif // !FFS_READ_ONLY

		if (!IsDirectory(Fcb) && FileObject->PrivateCacheMap)
		{
			FFSPrint((DBG_INFO, "FFSCleanup: CcUninitializeCacheMap is called for %s.\n",
								  Fcb->AnsiFileName.Buffer));

			CcUninitializeCacheMap(
					FileObject,
					(PLARGE_INTEGER)(&(Fcb->Header.FileSize)),
					NULL);
		}

		if (!Fcb->OpenHandleCount)
		{
			IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
		}

		FFSPrint((DBG_INFO, "FFSCleanup: OpenCount: %u ReferCount: %u %s\n",
			Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->AnsiFileName.Buffer));

		Status = STATUS_SUCCESS;

		if (FileObject)
		{
			SetFlag(FileObject->Flags, FO_CLEANUP_COMPLETE);
		}
	}

	_SEH2_FINALLY
	{
	   
		if (FcbPagingIoAcquired)
		{
			ExReleaseResourceForThreadLite(
				&Fcb->PagingIoResource,
				ExGetCurrentResourceThread());
		}

		if (FcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
				&Fcb->MainResource,
				ExGetCurrentResourceThread());
		}

		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			if (Status == STATUS_PENDING)
			{
				FFSQueueRequest(IrpContext);
			}
			else
			{
				IrpContext->Irp->IoStatus.Status = Status;

				FFSCompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;

	return Status;
}
