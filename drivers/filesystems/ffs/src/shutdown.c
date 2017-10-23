/* 
 * FFS File System Driver for Windows
 *
 * read.c
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
#pragma alloc_text(PAGE, FFSShutDown)
#endif


__drv_mustHoldCriticalRegion
NTSTATUS
FFSShutDown(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS                Status;

#ifndef __REACTOS__
	PKEVENT                 Event;
#endif

	PIRP                    Irp;
	PIO_STACK_LOCATION      IrpSp;

	PFFS_VCB                Vcb;
	PLIST_ENTRY             ListEntry;

	BOOLEAN                 GlobalResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		Status = STATUS_SUCCESS;

		Irp = IrpContext->Irp;

		IrpSp = IoGetCurrentIrpStackLocation(Irp);
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
		if (!ExAcquireResourceExclusiveLite(
					&FFSGlobal->Resource,
					IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		GlobalResourceAcquired = TRUE;

#ifndef __REACTOS__
		Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), FFS_POOL_TAG);
		KeInitializeEvent(Event, NotificationEvent, FALSE);
#endif

		for (ListEntry = FFSGlobal->VcbList.Flink;
				ListEntry != &(FFSGlobal->VcbList);
				ListEntry = ListEntry->Flink)
		{
			Vcb = CONTAINING_RECORD(ListEntry, FFS_VCB, Next);

			if (ExAcquireResourceExclusiveLite(
						&Vcb->MainResource,
						TRUE))
			{

				Status = FFSFlushFiles(Vcb, TRUE);
				if(!NT_SUCCESS(Status))
				{
					FFSBreakPoint();
				}

				Status = FFSFlushVolume(Vcb, TRUE);

				if(!NT_SUCCESS(Status))
				{
					FFSBreakPoint();
				}

				FFSDiskShutDown(Vcb);

				ExReleaseResourceForThreadLite(
						&Vcb->MainResource,
						ExGetCurrentResourceThread());
			}
		}

		/*
		IoUnregisterFileSystem(FFSGlobal->DeviceObject);
		*/
	}

	_SEH2_FINALLY
	{
		if (GlobalResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
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
				FFSCompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;

	return Status;
}
