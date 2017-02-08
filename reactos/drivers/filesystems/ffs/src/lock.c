/* 
 * FFS File System Driver for Windows
 *
 * lock.c
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
#pragma alloc_text(PAGE, FFSLockControl)
#endif

__drv_mustHoldCriticalRegion
NTSTATUS
FFSLockControl(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	BOOLEAN         CompleteRequest;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;
	PFILE_OBJECT    FileObject;
	PFFS_FCB        Fcb;
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
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		FileObject = IrpContext->FileObject;

		Fcb = (PFFS_FCB)FileObject->FsContext;

		ASSERT(Fcb != NULL);

		if (Fcb->Identifier.Type == FFSVCB)
		{
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		if (FlagOn(Fcb->FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
		{
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;

		//
		// While the file has any byte range locks we set IsFastIoPossible to
		// FastIoIsQuestionable so that the FastIoCheckIfPossible function is
		// called to check the locks for any fast I/O read/write operation.
		//
		if (Fcb->Header.IsFastIoPossible != FastIoIsQuestionable)
		{

			FFSPrint((DBG_INFO,
						"FFSLockControl: %-16.16s %-31s %s\n",
						FFSGetCurrentProcessName(),
						"FastIoIsQuestionable",
						Fcb->AnsiFileName.Buffer));

			Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
		}

		//
		// FsRtlProcessFileLock acquires FileObject->FsContext->Resource while
		// modifying the file locks and calls IoCompleteRequest when it's done.
		//

		CompleteRequest = FALSE;

		Status = FsRtlProcessFileLock(
				&Fcb->FileLockAnchor,
				Irp,
				NULL);

		if (!NT_SUCCESS(Status))
		{
			FFSPrint((DBG_ERROR, 
						"FFSLockControl: %-16.16s %-31s *** Status: %s (%#x) ***\n",
						FFSGetCurrentProcessName(),
						"IRP_MJ_LOCK_CONTROL",
						FFSNtStatusToString(Status),
						Status));
		}
	}

	_SEH2_FINALLY
	{
		if (!IrpContext->ExceptionInProgress)
		{
			if (!CompleteRequest)
			{
				IrpContext->Irp = NULL;
			}

			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}
