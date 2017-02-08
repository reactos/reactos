/* 
 * FFS File System Driver for Windows
 *
 * misc.c
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


extern PFFS_GLOBAL FFSGlobal;

#if (_WIN32_WINNT >= 0x0500)

/* Globals */

extern PFFS_GLOBAL FFSGlobal;

/* Definitions */

#define DBG_PNP DBG_USER

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE FFSPnpCompletionRoutine;
#endif // _PREFAST_

NTSTATUS NTAPI
FFSPnpCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Contxt);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSPnp)
#pragma alloc_text(PAGE, FFSPnpQueryRemove)
#pragma alloc_text(PAGE, FFSPnpRemove)
#pragma alloc_text(PAGE, FFSPnpCancelRemove)
#pragma alloc_text(PAGE, FFSPnpSurpriseRemove)
#endif



NTSTATUS NTAPI
FFSPnpCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Contxt)
{
	PKEVENT Event = (PKEVENT) Contxt;

	KeSetEvent(Event, 0, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Contxt);
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPnp(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS            Status = STATUS_INVALID_PARAMETER;

	PIRP                Irp;
	PIO_STACK_LOCATION  IrpSp;
	PFFS_VCB            Vcb = 0;
	PDEVICE_OBJECT      DeviceObject;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		if (!((Vcb->Identifier.Type == FFSVCB) &&
			(Vcb->Identifier.Size == sizeof(FFS_VCB))))
		{
			_SEH2_LEAVE; // Status = STATUS_INVALID_PARAMETER
		}

		Irp = IrpContext->Irp;
		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

		switch (IrpSp->MinorFunction)
		{

			case IRP_MN_QUERY_REMOVE_DEVICE:

				FFSPrint((DBG_PNP, "FFSPnp: FFSPnpQueryRemove...\n"));
				Status = FFSPnpQueryRemove(IrpContext, Vcb);

				break;

			case IRP_MN_REMOVE_DEVICE:

				FFSPrint((DBG_PNP, "FFSPnp: FFSPnpRemove...\n"));
				Status = FFSPnpRemove(IrpContext, Vcb);
				break;

			case IRP_MN_CANCEL_REMOVE_DEVICE:

				FFSPrint((DBG_PNP, "FFSPnp: FFSPnpCancelRemove...\n"));
				Status = FFSPnpCancelRemove(IrpContext, Vcb);
				break;

			case IRP_MN_SURPRISE_REMOVAL:

				FFSPrint((DBG_PNP, "FFSPnp: FFSPnpSupriseRemove...\n"));
				Status = FFSPnpSurpriseRemove(IrpContext, Vcb);
				break;

			default:
				break;
		}
	}

	_SEH2_FINALLY
	{
		if (!IrpContext->ExceptionInProgress)
		{
			Irp = IrpContext->Irp;

			if (Irp)
			{
				//
				// Here we need pass the IRP to the disk driver.
				//

				IoSkipCurrentIrpStackLocation(Irp);

				Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

				IrpContext->Irp = NULL;
			}

			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPnpQueryRemove(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	NTSTATUS Status;
	KEVENT   Event;
	BOOLEAN  bDeleted = FALSE;
	BOOLEAN  VcbAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY {

		FFSPrint((DBG_PNP, "FFSPnpQueryRemove by FFSPnp ...\n"));

		FFSPrint((DBG_PNP, "FFSPnpQueryRemove: FFSFlushVolume ...\n"));

#if (_WIN32_WINNT >= 0x0500)
		CcWaitForCurrentLazyWriterActivity();
#endif

		ExAcquireResourceExclusiveLite(
			&Vcb->MainResource, TRUE);
		VcbAcquired = TRUE;

		FFSFlushFiles(Vcb, FALSE);

		FFSFlushVolume(Vcb, FALSE);

		FFSPrint((DBG_PNP, "FFSPnpQueryRemove: FFSLockVcb: Vcb=%xh FileObject=%xh ...\n", Vcb, IrpContext->FileObject));
		Status = FFSLockVcb(Vcb, IrpContext->FileObject);

		FFSPrint((DBG_PNP, "FFSPnpQueryRemove: FFSPurgeVolume ...\n"));
		FFSPurgeVolume(Vcb, TRUE);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());
		VcbAcquired = FALSE;

		IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		IoSetCompletionRoutine(IrpContext->Irp,
				FFSPnpCompletionRoutine,
				&Event,
				TRUE,
				TRUE,
				TRUE);

		FFSPrint((DBG_PNP, "FFSPnpQueryRemove: Call lower level driver...\n"));
		Status = IoCallDriver(Vcb->TargetDeviceObject, 
				IrpContext->Irp);

		if (Status == STATUS_PENDING)
		{
			KeWaitForSingleObject(&Event,
					Executive,
					KernelMode,
					FALSE,
					NULL);

			Status = IrpContext->Irp->IoStatus.Status;
		}

		if (NT_SUCCESS(Status))
		{
			FFSPrint((DBG_PNP, "FFSPnpQueryRemove: FFSCheckDismount ...\n"));
			bDeleted = FFSCheckDismount(IrpContext, Vcb, TRUE);
			FFSPrint((DBG_PNP, "FFSPnpQueryRemove: FFSFlushVolume bDelted=%xh ...\n", bDeleted));
		}

		ASSERT(!(NT_SUCCESS(Status) && !bDeleted));
	}

	_SEH2_FINALLY
	{
		if (VcbAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		FFSCompleteRequest(
				IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status) ?
					IO_DISK_INCREMENT : IO_NO_INCREMENT));

		IrpContext->Irp = NULL;
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPnpRemove(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	NTSTATUS Status;
	KEVENT   Event;
	BOOLEAN  bDeleted;

    PAGED_CODE();

	_SEH2_TRY
	{

		FFSPrint((DBG_PNP, "FFSPnpRemove by FFSPnp ...\n"));
#if (_WIN32_WINNT >= 0x0500)
		CcWaitForCurrentLazyWriterActivity();
#endif
		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,  TRUE);

		Status = FFSLockVcb(Vcb, IrpContext->FileObject);

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());

		//
		// Setup the Irp. We'll send it to the lower disk driver.
		//

		IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		IoSetCompletionRoutine(IrpContext->Irp,
				FFSPnpCompletionRoutine,
				&Event,
				TRUE,
				TRUE,
				TRUE);

		Status = IoCallDriver(Vcb->TargetDeviceObject, 
				IrpContext->Irp);

		if (Status == STATUS_PENDING)
		{

			KeWaitForSingleObject(&Event,
					Executive,
					KernelMode,
					FALSE,
					NULL);

			Status = IrpContext->Irp->IoStatus.Status;
		}

		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource, TRUE);

		FFSPurgeVolume(Vcb, FALSE);

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());

		bDeleted = FFSCheckDismount(IrpContext, Vcb, TRUE);
	}

	_SEH2_FINALLY
	{
		FFSCompleteRequest(
				IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status)?
					IO_DISK_INCREMENT : IO_NO_INCREMENT));

		IrpContext->Irp = NULL;
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPnpSurpriseRemove(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	NTSTATUS Status;
	KEVENT   Event;
	BOOLEAN  bDeleted;

    PAGED_CODE();

	_SEH2_TRY
	{

		FFSPrint((DBG_PNP, "FFSPnpSupriseRemove by FFSPnp ...\n"));
#if (_WIN32_WINNT >= 0x0500)
		CcWaitForCurrentLazyWriterActivity();
#endif
		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,  TRUE);

		Status = FFSLockVcb(Vcb, IrpContext->FileObject);

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());

		//
		// Setup the Irp. We'll send it to the lower disk driver.
		//

		IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		IoSetCompletionRoutine(IrpContext->Irp,
				FFSPnpCompletionRoutine,
				&Event,
				TRUE,
				TRUE,
				TRUE);

		Status = IoCallDriver(Vcb->TargetDeviceObject, 
					IrpContext->Irp);

		if (Status == STATUS_PENDING)
		{

			KeWaitForSingleObject(&Event,
					Executive,
					KernelMode,
					FALSE,
					NULL);

			Status = IrpContext->Irp->IoStatus.Status;
		}

		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource, TRUE);

		FFSPurgeVolume(Vcb, FALSE);

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());

		bDeleted = FFSCheckDismount(IrpContext, Vcb, TRUE);
	}

	_SEH2_FINALLY
	{
		FFSCompleteRequest(
				IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status)?
					IO_DISK_INCREMENT : IO_NO_INCREMENT));

		IrpContext->Irp = NULL;
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPnpCancelRemove(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	NTSTATUS Status;

    PAGED_CODE();

	FFSPrint((DBG_PNP, "FFSPnpCancelRemove by FFSPnp ...\n"));

	ExAcquireResourceExclusiveLite(
			&Vcb->MainResource, TRUE);

	Status = FFSUnlockVcb(Vcb, IrpContext->FileObject);

	ExReleaseResourceForThreadLite(
			&Vcb->MainResource,
			ExGetCurrentResourceThread());

	IoSkipCurrentIrpStackLocation(IrpContext->Irp);

	Status = IoCallDriver(Vcb->TargetDeviceObject, IrpContext->Irp);

	IrpContext->Irp = NULL;

	return Status;
}


#endif //(_WIN32_WINNT >= 0x0500)
