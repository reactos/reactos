/* 
 * FFS File System Driver for Windows
 *
 * devctl.c
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

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE FFSDeviceControlCompletion;
#endif // _PREFAST_

NTSTATUS NTAPI
FFSDeviceControlCompletion(
	IN PDEVICE_OBJECT   DeviceObject,
	IN PIRP             Irp,
	IN PVOID            Context);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSDeviceControl)
#pragma alloc_text(PAGE, FFSDeviceControlNormal)
#if FFS_UNLOAD
#pragma alloc_text(PAGE, FFSPrepareToUnload)
#endif
#endif


NTSTATUS NTAPI
FFSDeviceControlCompletion(
	IN PDEVICE_OBJECT   DeviceObject,
	IN PIRP             Irp,
	IN PVOID            Context)
{
	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}

	return STATUS_SUCCESS;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSDeviceControlNormal(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	BOOLEAN         CompleteRequest = TRUE;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;

	PFFS_VCB        Vcb;

	PIRP            Irp;
	PIO_STACK_LOCATION IrpSp;
	PIO_STACK_LOCATION NextIrpSp;

	PDEVICE_OBJECT  TargetDeviceObject;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		CompleteRequest = TRUE;

		DeviceObject = IrpContext->DeviceObject;

		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;

			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;
		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		Vcb = (PFFS_VCB)IrpSp->FileObject->FsContext;

		if (!((Vcb) && (Vcb->Identifier.Type == FFSVCB) &&
					(Vcb->Identifier.Size == sizeof(FFS_VCB))
		    )
		  )
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		TargetDeviceObject = Vcb->TargetDeviceObject;

		//
		// Pass on the IOCTL to the driver below
		//

		CompleteRequest = FALSE;

		NextIrpSp = IoGetNextIrpStackLocation(Irp);
		*NextIrpSp = *IrpSp;

		IoSetCompletionRoutine(
				Irp,
				FFSDeviceControlCompletion,
				NULL,
				FALSE,
				TRUE,
				TRUE);

		Status = IoCallDriver(TargetDeviceObject, Irp);
	}

	_SEH2_FINALLY
	{
		if (!IrpContext->ExceptionInProgress)
		{
			if (IrpContext)
			{
				if (!CompleteRequest)
				{
					IrpContext->Irp = NULL;
				}

				FFSCompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;

	return Status;
}


#if FFS_UNLOAD

__drv_mustHoldCriticalRegion
NTSTATUS
FFSPrepareToUnload(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;
	BOOLEAN         GlobalDataResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		if (DeviceObject != FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		ExAcquireResourceExclusiveLite(
				&FFSGlobal->Resource,
				TRUE);

		GlobalDataResourceAcquired = TRUE;

		if (FlagOn(FFSGlobal->Flags, FFS_UNLOAD_PENDING))
		{
			FFSPrint((DBG_ERROR, "FFSPrepareUnload:  Already ready to unload.\n"));

			Status = STATUS_ACCESS_DENIED;

			_SEH2_LEAVE;
		}

		{
			PFFS_VCB                Vcb;
			PLIST_ENTRY             ListEntry;

			ListEntry = FFSGlobal->VcbList.Flink;

			while (ListEntry != &(FFSGlobal->VcbList))
			{
				Vcb = CONTAINING_RECORD(ListEntry, FFS_VCB, Next);
				ListEntry = ListEntry->Flink;

				if (Vcb && (!Vcb->ReferenceCount) &&
						IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
				{
					FFSRemoveVcb(Vcb);
					FFSClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

					FFSFreeVcb(Vcb);
				}
			}
		}

		if (!IsListEmpty(&(FFSGlobal->VcbList)))
		{

			FFSPrint((DBG_ERROR, "FFSPrepareUnload:  Mounted volumes exists.\n"));

			Status = STATUS_ACCESS_DENIED;

			_SEH2_LEAVE;
		}

		IoUnregisterFileSystem(FFSGlobal->DeviceObject);

#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed to unload" )
#endif
		FFSGlobal->DriverObject->DriverUnload = DriverUnload;

		SetFlag(FFSGlobal->Flags, FFS_UNLOAD_PENDING);

		FFSPrint((DBG_INFO, "FFSPrepareToUnload: Driver is ready to unload.\n"));

		Status = STATUS_SUCCESS;
	}
	_SEH2_FINALLY
	{
		if (GlobalDataResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}

#endif


__drv_mustHoldCriticalRegion
NTSTATUS
FFSDeviceControl(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIRP                Irp;
	PIO_STACK_LOCATION  IoStackLocation;
	ULONG               IoControlCode;
	NTSTATUS            Status;

    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	Irp = IrpContext->Irp;

	IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	IoControlCode =
		IoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (IoControlCode)
	{
#if FFS_UNLOAD
		case IOCTL_PREPARE_TO_UNLOAD:
			Status = FFSPrepareToUnload(IrpContext);
			break;
#endif

		case IOCTL_SELECT_BSD_PARTITION:
			Status = FFSSelectBSDPartition(IrpContext);
			break;

		default:
			Status = FFSDeviceControlNormal(IrpContext);
	}

	return Status;
}
