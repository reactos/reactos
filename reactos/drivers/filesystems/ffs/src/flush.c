/* 
 * FFS File System Driver for Windows
 *
 * flush.c
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
#pragma alloc_text(PAGE, FFSFlushFile)
#pragma alloc_text(PAGE, FFSFlushFiles)
#pragma alloc_text(PAGE, FFSFlushVolume)
#pragma alloc_text(PAGE, FFSFlush)
#endif

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE FFSFlushCompletionRoutine;
#endif // _PREFAST_

NTSTATUS NTAPI
FFSFlushCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Contxt)

{
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);


	if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST)
		Irp->IoStatus.Status = STATUS_SUCCESS;

	return STATUS_SUCCESS;
}

__drv_mustHoldCriticalRegion
NTSTATUS
FFSFlushFiles(
	IN PFFS_VCB Vcb,
	BOOLEAN     bShutDown)
{
	IO_STATUS_BLOCK    IoStatus;

	PFFS_FCB        Fcb;
	PLIST_ENTRY     ListEntry;

    PAGED_CODE();

	if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
			IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
	{
		return STATUS_SUCCESS;
	}

	FFSPrint((DBG_INFO, "Flushing Files ...\n"));

	// Flush all Fcbs in Vcb list queue.
	{
		for (ListEntry = Vcb->FcbList.Flink;
				ListEntry != &Vcb->FcbList;
				ListEntry = ListEntry->Flink)
		{
			Fcb = CONTAINING_RECORD(ListEntry, FFS_FCB, Next);

			if (ExAcquireResourceExclusiveLite(
						&Fcb->MainResource,
						TRUE))
			{
				IoStatus.Status = FFSFlushFile(Fcb);
#if 0
/*
				if (bShutDown)
					IoStatus.Status = FFSPurgeFile(Fcb, TRUE);
				else
					IoStatus.Status = FFSFlushFile(Fcb);
*/
#endif
				ExReleaseResourceForThreadLite(
						&Fcb->MainResource,
						ExGetCurrentResourceThread());
			}
		}
	}

	return IoStatus.Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSFlushVolume(
	IN PFFS_VCB Vcb,
	BOOLEAN     bShutDown)
{
	IO_STATUS_BLOCK    IoStatus;

    PAGED_CODE();

	if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
			IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
	{
		return STATUS_SUCCESS;
	}

	FFSPrint((DBG_INFO, "FFSFlushVolume: Flushing Vcb ...\n"));

	ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
	ExReleaseResourceLite(&Vcb->PagingIoResource);

	CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);

	return IoStatus.Status;       
}


NTSTATUS
FFSFlushFile(
	IN PFFS_FCB Fcb)
{
	IO_STATUS_BLOCK    IoStatus;

    PAGED_CODE();

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	if (IsDirectory(Fcb))
		return STATUS_SUCCESS;

	FFSPrint((DBG_INFO, "FFSFlushFile: Flushing File Inode=%xh %S ...\n", 
				Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer));
	/*
	{
		ULONG ResShCnt, ResExCnt; 
		ResShCnt = ExIsResourceAcquiredSharedLite(&Fcb->PagingIoResource);
		ResExCnt = ExIsResourceAcquiredExclusiveLite(&Fcb->PagingIoResource);

		FFSPrint((DBG_INFO, "FFSFlushFile: PagingIoRes: %xh:%xh\n", ResShCnt, ResExCnt));
	}
	*/
	CcFlushCache(&(Fcb->SectionObject), NULL, 0, &IoStatus);

	ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

	return IoStatus.Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSFlush(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS                Status;

	PIRP                    Irp;
	PIO_STACK_LOCATION      IrpSp;

	PFFS_VCB                Vcb = 0;
	PFFS_FCBVCB             FcbOrVcb = 0;
	PFILE_OBJECT            FileObject;

	PDEVICE_OBJECT          DeviceObject;

	BOOLEAN                 MainResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		//
		// This request is not allowed on the main device object
		//
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
				IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;

		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		FileObject = IrpContext->FileObject;

		FcbOrVcb = (PFFS_FCBVCB)FileObject->FsContext;

		ASSERT(FcbOrVcb != NULL);
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
		if (!ExAcquireResourceExclusiveLite(
					&FcbOrVcb->MainResource,
					IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		MainResourceAcquired = TRUE;

		if (FcbOrVcb->Identifier.Type == FFSVCB)
		{
			Status = FFSFlushFiles((PFFS_VCB)(FcbOrVcb), FALSE);

			if (NT_SUCCESS(Status))
			{
				_SEH2_LEAVE;
			}

			Status = FFSFlushVolume((PFFS_VCB)(FcbOrVcb), FALSE);

			if (NT_SUCCESS(Status) && IsFlagOn(Vcb->StreamObj->Flags, FO_FILE_MODIFIED))
			{
				ClearFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);
			}
		}
		else if (FcbOrVcb->Identifier.Type == FFSFCB)
		{
			Status = FFSFlushFile((PFFS_FCB)(FcbOrVcb));

			if (NT_SUCCESS(Status) && IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED))
			{
				ClearFlag(FileObject->Flags, FO_FILE_MODIFIED);
			}
		}
	}

	_SEH2_FINALLY
	{
		if (MainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FcbOrVcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
			{
				// Call the disk driver to flush the physial media.
				NTSTATUS DriverStatus;
				PIO_STACK_LOCATION NextIrpSp;

                IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
				NextIrpSp = IoGetNextIrpStackLocation(IrpContext->Irp);

				*NextIrpSp = *IrpSp;

				IoSetCompletionRoutine(IrpContext->Irp,
						FFSFlushCompletionRoutine,
						NULL,
						TRUE,
						TRUE,
						TRUE);

				DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, IrpContext->Irp);

				Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
					Status : DriverStatus;

				IrpContext->Irp = NULL;
			}

			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}
