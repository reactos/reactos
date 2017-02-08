/* 
 * FFS File System Driver for Windows
 *
 * close.c
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
#pragma alloc_text(PAGE, FFSClose)
#pragma alloc_text(PAGE, FFSQueueCloseRequest)
#pragma alloc_text(PAGE, FFSDeQueueCloseRequest)
#endif


__drv_mustHoldCriticalRegion
NTSTATUS
FFSClose(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	NTSTATUS        Status = STATUS_SUCCESS;
	PFFS_VCB        Vcb = 0;
	BOOLEAN         VcbResourceAcquired = FALSE;
	PFILE_OBJECT    FileObject;
	PFFS_FCB        Fcb = 0;
	BOOLEAN         FcbResourceAcquired = FALSE;
	PFFS_CCB        Ccb;
	BOOLEAN         FreeVcb = FALSE;

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

		Vcb = (PFFS_VCB) DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		if (!ExAcquireResourceExclusiveLite(
					&Vcb->MainResource,
					IrpContext->IsSynchronous))
		{
			FFSPrint((DBG_INFO, "FFSClose: PENDING ... Vcb: %xh/%xh\n",
						Vcb->OpenFileHandleCount, Vcb->ReferenceCount));

			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		VcbResourceAcquired = TRUE;

		FileObject = IrpContext->FileObject;

		if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE))
		{
			Fcb = IrpContext->Fcb;
			Ccb = IrpContext->Ccb;
		}
		else
		{
			Fcb = (PFFS_FCB)FileObject->FsContext;

			if (!Fcb)
			{
				Status = STATUS_SUCCESS;
				_SEH2_LEAVE;
			}

			ASSERT(Fcb != NULL);

			Ccb = (PFFS_CCB)FileObject->FsContext2;
		}

		if (Fcb->Identifier.Type == FFSVCB)
		{
			Vcb->ReferenceCount--;

			if (!Vcb->ReferenceCount && FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
			{
				FreeVcb = TRUE;
			}

			if (Ccb)
			{
				FFSFreeCcb(Ccb);
				if (FileObject)
				{
					FileObject->FsContext2 = Ccb = NULL;
				}
			}

			Status = STATUS_SUCCESS;

			_SEH2_LEAVE;
		}

		if (Fcb->Identifier.Type != FFSFCB || Fcb->Identifier.Size != sizeof(FFS_FCB))
		{
#if DBG
			FFSPrint((DBG_ERROR, "FFSClose: Strange IRP_MJ_CLOSE by system!\n"));
			ExAcquireResourceExclusiveLite(
					&FFSGlobal->CountResource,
					TRUE);

			FFSGlobal->IRPCloseCount++;

			ExReleaseResourceForThreadLite(
					&FFSGlobal->CountResource,
					ExGetCurrentResourceThread());
#endif
			_SEH2_LEAVE;
		}

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		/*        
			  if ((!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) && 
			  (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE)))
			  */
		{
			if (!ExAcquireResourceExclusiveLite(
						&Fcb->MainResource,
						IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			FcbResourceAcquired = TRUE;
		}

		if (!Ccb)
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		ASSERT((Ccb->Identifier.Type == FFSCCB) &&
				(Ccb->Identifier.Size == sizeof(FFS_CCB)));

		Fcb->ReferenceCount--;
		Vcb->ReferenceCount--;

		if (!Vcb->ReferenceCount && IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
		{
			FreeVcb = TRUE;
		}

		FFSPrint((DBG_INFO, "FFSClose: OpenHandleCount: %u ReferenceCount: %u %s\n",
					Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->AnsiFileName.Buffer));

		if (Ccb)
		{
			FFSFreeCcb(Ccb);

			if (FileObject)
			{
				FileObject->FsContext2 = Ccb = NULL;
			}
		}

		if (!Fcb->ReferenceCount)
		{
			//
			// Remove Fcb from Vcb->FcbList ...
			//

			RemoveEntryList(&Fcb->Next);

			FFSFreeFcb(Fcb);

			FcbResourceAcquired = FALSE;
		}

		Status = STATUS_SUCCESS;
	}

	_SEH2_FINALLY
	{
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
				FFSQueueCloseRequest(IrpContext);
#if 0
/*
				Status = STATUS_SUCCESS;

				if (IrpContext->Irp != NULL)
				{
					IrpContext->Irp->IoStatus.Status = Status;

					FFSCompleteRequest(
							IrpContext->Irp,
							(BOOLEAN)!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED),
							(CCHAR)
							(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));

					IrpContext->Irp = NULL;
				}
*/
#endif
			}
			else
			{
				FFSCompleteIrpContext(IrpContext, Status);

				if (FreeVcb)
				{
					ExAcquireResourceExclusiveLite(
							&FFSGlobal->Resource, TRUE);

					FFSClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

					FFSRemoveVcb(Vcb);

					ExReleaseResourceForThreadLite(
							&FFSGlobal->Resource,
							ExGetCurrentResourceThread());

					FFSFreeVcb(Vcb);
				}
			}
		}
	} _SEH2_END;

	return Status;
}


VOID
FFSQueueCloseRequest(
	IN PFFS_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	if (!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE))
	{
		SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE);

		IrpContext->Fcb = (PFFS_FCB)IrpContext->FileObject->FsContext;
		IrpContext->Ccb = (PFFS_CCB)IrpContext->FileObject->FsContext2;

		IrpContext->FileObject = NULL;
	}

	// IsSynchronous means we can block (so we don't requeue it)
	IrpContext->IsSynchronous = TRUE;

	ExInitializeWorkItem(
			&IrpContext->WorkQueueItem,
			FFSDeQueueCloseRequest,
			IrpContext);

	ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
}


VOID NTAPI
FFSDeQueueCloseRequest(
	IN PVOID Context)
{
	PFFS_IRP_CONTEXT IrpContext;

    PAGED_CODE();

	IrpContext = (PFFS_IRP_CONTEXT) Context;

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			FsRtlEnterFileSystem();
			FFSClose(IrpContext);
		}
		_SEH2_EXCEPT (FFSExceptionFilter(IrpContext, _SEH2_GetExceptionInformation()))
		{
			FFSExceptionHandler(IrpContext);
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;
}
