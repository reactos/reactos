/* 
 * FFS File System Driver for Windows
 *
 * dipatch.c
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
#pragma alloc_text(PAGE, FFSQueueRequest)
#pragma alloc_text(PAGE, FFSDeQueueRequest)
#pragma alloc_text(PAGE, FFSDispatchRequest)
#pragma alloc_text(PAGE, FFSBuildRequest)
#endif


NTSTATUS
FFSQueueRequest(
	IN PFFS_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	// IsSynchronous means we can block (so we don't requeue it)
	IrpContext->IsSynchronous = TRUE;

	SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

	IoMarkIrpPending(IrpContext->Irp);

	ExInitializeWorkItem(
			&IrpContext->WorkQueueItem,
			FFSDeQueueRequest,
			IrpContext);

	ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

	return STATUS_PENDING;
}


VOID NTAPI
FFSDeQueueRequest(
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

			if (!IrpContext->IsTopLevel)
			{
				IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);
			}

			FFSDispatchRequest(IrpContext);
		}
		_SEH2_EXCEPT (FFSExceptionFilter(IrpContext, _SEH2_GetExceptionInformation()))
		{
			FFSExceptionHandler(IrpContext);
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		IoSetTopLevelIrp(NULL);

		FsRtlExitFileSystem();
	} _SEH2_END;
}


NTSTATUS
FFSDispatchRequest(
	IN PFFS_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	switch (IrpContext->MajorFunction)
	{
		case IRP_MJ_CREATE:
			return FFSCreate(IrpContext);

		case IRP_MJ_CLOSE:
			return FFSClose(IrpContext);

		case IRP_MJ_READ:
			return FFSRead(IrpContext);

#if !FFS_READ_ONLY
		case IRP_MJ_WRITE:
			return FFSWrite(IrpContext);
#endif // !FFS_READ_ONLY

		case IRP_MJ_FLUSH_BUFFERS:
			return FFSFlush(IrpContext);

		case IRP_MJ_QUERY_INFORMATION:
			return FFSQueryInformation(IrpContext);

		case IRP_MJ_SET_INFORMATION:
			return FFSSetInformation(IrpContext);

		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			return FFSQueryVolumeInformation(IrpContext);

#if !FFS_READ_ONLY
		case IRP_MJ_SET_VOLUME_INFORMATION:
			return FFSSetVolumeInformation(IrpContext);
#endif // !FFS_READ_ONLY

		case IRP_MJ_DIRECTORY_CONTROL:
			return FFSDirectoryControl(IrpContext);

		case IRP_MJ_FILE_SYSTEM_CONTROL:
			return FFSFileSystemControl(IrpContext);

		case IRP_MJ_DEVICE_CONTROL:
			return FFSDeviceControl(IrpContext);

		case IRP_MJ_LOCK_CONTROL:
			return FFSLockControl(IrpContext);

		case IRP_MJ_CLEANUP:
			return FFSCleanup(IrpContext);

		case IRP_MJ_SHUTDOWN:
			return FFSShutDown(IrpContext);

#if (_WIN32_WINNT >= 0x0500)
		case IRP_MJ_PNP:
			return FFSPnp(IrpContext);
#endif //(_WIN32_WINNT >= 0x0500)        
		default:
			FFSPrint((DBG_ERROR, "FFSDispatchRequest: Unexpected major function: %xh\n",
						IrpContext->MajorFunction));

			FFSCompleteIrpContext(IrpContext, STATUS_DRIVER_INTERNAL_ERROR);

			return STATUS_DRIVER_INTERNAL_ERROR;
	}
}


NTSTATUS NTAPI
FFSBuildRequest(
	PDEVICE_OBJECT   DeviceObject,
	PIRP             Irp)
{
	BOOLEAN             AtIrqlPassiveLevel = FALSE;
	BOOLEAN             IsTopLevelIrp = FALSE;
	PFFS_IRP_CONTEXT    IrpContext = NULL;
	NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
#if DBG
			FFSDbgPrintCall(DeviceObject, Irp);
#endif

			AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);

			if (AtIrqlPassiveLevel)
			{
				FsRtlEnterFileSystem();
			}

			if (!IoGetTopLevelIrp())
			{
				IsTopLevelIrp = TRUE;
				IoSetTopLevelIrp(Irp);
			}

			IrpContext = FFSAllocateIrpContext(DeviceObject, Irp);

			if (!IrpContext)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				Irp->IoStatus.Status = Status;

				FFSCompleteRequest(Irp, TRUE, IO_NO_INCREMENT);
			}
			else
			{
				if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
						!AtIrqlPassiveLevel)
				{
					FFSBreakPoint();
				}

				Status = FFSDispatchRequest(IrpContext);
			}
		}
		_SEH2_EXCEPT (FFSExceptionFilter(IrpContext, _SEH2_GetExceptionInformation()))
		{
			Status = FFSExceptionHandler(IrpContext);
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		if (IsTopLevelIrp)
		{
			IoSetTopLevelIrp(NULL);
		}

		if (AtIrqlPassiveLevel)
		{
			FsRtlExitFileSystem();
		}       
	} _SEH2_END;

	return Status;
}
