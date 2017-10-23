/* 
 * FFS File System Driver for Windows
 *
 * block.c
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

typedef struct _FFS_RW_CONTEXT {
	PIRP        MasterIrp;
	KEVENT      Event;
	BOOLEAN     Wait;
	LONG        Blocks;
	ULONG       Length;
} FFS_RW_CONTEXT, *PFFS_RW_CONTEXT;

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE FFSReadWriteBlockSyncCompletionRoutine;
IO_COMPLETION_ROUTINE FFSReadWriteBlockAsyncCompletionRoutine;
IO_COMPLETION_ROUTINE FFSMediaEjectControlCompletion;
#endif // _PREFAST_

NTSTATUS NTAPI
FFSReadWriteBlockSyncCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Context);

NTSTATUS NTAPI
FFSReadWriteBlockAsyncCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Context);

NTSTATUS NTAPI
FFSMediaEjectControlCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Contxt);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSLockUserBuffer)
#pragma alloc_text(PAGE, FFSGetUserBuffer)
#pragma alloc_text(PAGE, FFSReadSync)
#pragma alloc_text(PAGE, FFSReadDisk)
#pragma alloc_text(PAGE, FFSDiskIoControl)
#pragma alloc_text(PAGE, FFSReadWriteBlocks)
#pragma alloc_text(PAGE, FFSMediaEjectControl)
#pragma alloc_text(PAGE, FFSDiskShutDown)
#endif


NTSTATUS
FFSLockUserBuffer(
	IN PIRP             Irp,
	IN ULONG            Length,
	IN LOCK_OPERATION   Operation)
{
	NTSTATUS Status;

    PAGED_CODE();

	ASSERT(Irp != NULL);

	if (Irp->MdlAddress != NULL)
	{
		return STATUS_SUCCESS;
	}

	IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

	if (Irp->MdlAddress == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	_SEH2_TRY
	{
		MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);

		Status = STATUS_SUCCESS;
	}
	_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
	{
		IoFreeMdl(Irp->MdlAddress);

		Irp->MdlAddress = NULL;

		FFSBreakPoint();

		Status = STATUS_INVALID_USER_BUFFER;
	} _SEH2_END;

	return Status;
}


PVOID
FFSGetUserBuffer(
	IN PIRP Irp)
{
    PAGED_CODE();

	ASSERT(Irp != NULL);

	if (Irp->MdlAddress)
	{
#if (_WIN32_WINNT >= 0x0500)
		return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
#else
		return MmGetSystemAddressForMdl(Irp->MdlAddress);
#endif
	}
	else
	{
		return Irp->UserBuffer;
	}
}


NTSTATUS NTAPI
FFSReadWriteBlockSyncCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Context)
{
	PFFS_RW_CONTEXT pContext = (PFFS_RW_CONTEXT)Context;

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{

		pContext->MasterIrp->IoStatus = Irp->IoStatus;
	}

	IoFreeMdl(Irp->MdlAddress);
	IoFreeIrp(Irp);

	if (InterlockedDecrement(&pContext->Blocks) == 0)
	{
		pContext->MasterIrp->IoStatus.Information = 0;

		if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status))
		{
			pContext->MasterIrp->IoStatus.Information =
				pContext->Length;
		}

		KeSetEvent(&pContext->Event, 0, FALSE);
	}

	UNREFERENCED_PARAMETER(DeviceObject);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI
FFSReadWriteBlockAsyncCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
	PFFS_RW_CONTEXT pContext = (PFFS_RW_CONTEXT)Context;

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		pContext->MasterIrp->IoStatus = Irp->IoStatus;
	}

	if (InterlockedDecrement(&pContext->Blocks) == 0)
	{
		pContext->MasterIrp->IoStatus.Information = 0;

		if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status))
		{
			pContext->MasterIrp->IoStatus.Information =
				pContext->Length;
		}

		IoMarkIrpPending(pContext->MasterIrp);

		ExFreePool(pContext);
	}

	UNREFERENCED_PARAMETER(DeviceObject);

	return STATUS_SUCCESS;

}

NTSTATUS
FFSReadWriteBlocks(
	IN PFFS_IRP_CONTEXT IrpContext,
	IN PFFS_VCB         Vcb,
	IN PFFS_BDL         FFSBDL,
	IN ULONG            Length,
	IN ULONG            Count,
	IN BOOLEAN          bVerify)
{
	PMDL                Mdl;
	PIRP                Irp;
	PIRP                MasterIrp = IrpContext->Irp;
	PIO_STACK_LOCATION  IrpSp;
	NTSTATUS            Status = STATUS_SUCCESS;
	PFFS_RW_CONTEXT     pContext = NULL;
	ULONG               i;
	BOOLEAN             bBugCheck = FALSE;

    PAGED_CODE();

	ASSERT(MasterIrp);

	_SEH2_TRY
	{

		pContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(FFS_RW_CONTEXT), FFS_POOL_TAG);

		if (!pContext)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		RtlZeroMemory(pContext, sizeof(FFS_RW_CONTEXT));

		pContext->Wait = IrpContext->IsSynchronous;
		pContext->MasterIrp = MasterIrp;
		pContext->Blocks = Count;
		pContext->Length = 0;

		if (pContext->Wait)
		{
			KeInitializeEvent(&(pContext->Event), NotificationEvent, FALSE);
		}

		for (i = 0; i < Count; i++)
		{

			Irp = IoMakeAssociatedIrp(MasterIrp,
					(CCHAR)(Vcb->TargetDeviceObject->StackSize + 1));
			if (!Irp)
			{
#ifdef __REACTOS__
                ExFreePoolWithTag(pContext, FFS_POOL_TAG);
                pContext = NULL;
#endif
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			Mdl = IoAllocateMdl((PCHAR)MasterIrp->UserBuffer +
					FFSBDL[i].Offset,
					FFSBDL[i].Length,
					FALSE,
					FALSE,
					Irp);

			if (!Mdl)
			{
#ifdef __REACTOS__
                ExFreePoolWithTag(pContext, FFS_POOL_TAG);
                pContext = NULL;
#endif
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			IoBuildPartialMdl(MasterIrp->MdlAddress,
					Mdl,
					(PCHAR)MasterIrp->UserBuffer + FFSBDL[i].Offset,
					FFSBDL[i].Length);

			IoSetNextIrpStackLocation(Irp);
			IrpSp = IoGetCurrentIrpStackLocation(Irp);


			IrpSp->MajorFunction = IrpContext->MajorFunction;
			IrpSp->Parameters.Read.Length = FFSBDL[i].Length;
			IrpSp->Parameters.Read.ByteOffset.QuadPart = FFSBDL[i].Lba;

			IoSetCompletionRoutine(Irp,
					IrpContext->IsSynchronous ?
					&FFSReadWriteBlockSyncCompletionRoutine :
					&FFSReadWriteBlockAsyncCompletionRoutine,
					(PVOID) pContext,
					TRUE,
					TRUE,
					TRUE);

			IrpSp = IoGetNextIrpStackLocation(Irp);

			IrpSp->MajorFunction = IrpContext->MajorFunction;
			IrpSp->Parameters.Read.Length = FFSBDL[i].Length;
			IrpSp->Parameters.Read.ByteOffset.QuadPart = FFSBDL[i].Lba;

			if (bVerify)
			{
				SetFlag(IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME);
			}

			FFSBDL[i].Irp = Irp;
		}

		MasterIrp->AssociatedIrp.IrpCount = Count;

		if (IrpContext->IsSynchronous)
		{
			MasterIrp->AssociatedIrp.IrpCount += 1;
		}

		pContext->Length = Length;

		bBugCheck = TRUE;

		for (i = 0; i < Count; i++)
		{
			Status = IoCallDriver(Vcb->TargetDeviceObject,
					FFSBDL[i].Irp);
		}

		if (IrpContext->IsSynchronous)
		{
			KeWaitForSingleObject(&(pContext->Event),
					Executive, KernelMode, FALSE, NULL);

			KeClearEvent(&(pContext->Event));
		}
	}

	_SEH2_FINALLY
	{
		if (IrpContext->IsSynchronous)
		{
			if (MasterIrp)
				Status = MasterIrp->IoStatus.Status;

			if (pContext)
				ExFreePool(pContext);

		}
		else
		{
			IrpContext->Irp = NULL;
			Status = STATUS_PENDING;
		}

		if (_SEH2_AbnormalTermination())
		{
			if (bBugCheck)
			{
				FFSBugCheck(FFS_BUGCHK_BLOCK, 0, 0, 0);
			}

			for (i = 0; i < Count; i++)
			{
				if (FFSBDL[i].Irp != NULL)
				{
					if (FFSBDL[i].Irp->MdlAddress != NULL)
					{
						IoFreeMdl(FFSBDL[i].Irp->MdlAddress);
					}

					IoFreeIrp(FFSBDL[i].Irp);
				}
			}
		}
	} _SEH2_END;

	return Status;
}


NTSTATUS
FFSReadSync(
	IN PFFS_VCB         Vcb,
	IN ULONGLONG        Offset,
	IN ULONG            Length,
	OUT PVOID           Buffer,
	IN BOOLEAN          bVerify)
{
	KEVENT          Event;
	PIRP            Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS        Status;

    PAGED_CODE();

	ASSERT(Vcb != NULL);
	ASSERT(Vcb->TargetDeviceObject != NULL);
	ASSERT(Buffer != NULL);

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_READ,
			Vcb->TargetDeviceObject,
			Buffer,
			Length,
			(PLARGE_INTEGER)(&Offset),
			&Event,
			&IoStatus);

	if (!Irp)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (bVerify)
	{
		SetFlag(IoGetNextIrpStackLocation(Irp)->Flags, 
				SL_OVERRIDE_VERIFY_VOLUME);
	}

	Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(
				&Event,
				Suspended,
				KernelMode,
				FALSE,
				NULL);

		Status = IoStatus.Status;
	}

	return Status;
}


NTSTATUS
FFSReadDisk(
	IN PFFS_VCB    Vcb,
	IN ULONGLONG   Offset,
	IN ULONG       Size,
	IN PVOID       Buffer,
	IN BOOLEAN     bVerify)
{
	NTSTATUS    Status;
	PUCHAR      Buf;
	ULONG       Length;
	ULONGLONG   Lba;

    PAGED_CODE();

	Lba = Offset & (~((ULONGLONG)SECTOR_SIZE - 1));
	Length = (ULONG)(Size + Offset + SECTOR_SIZE - 1 - Lba) &
		(~((ULONG)SECTOR_SIZE - 1));

	Buf = ExAllocatePoolWithTag(PagedPool, Length, FFS_POOL_TAG);
	if (!Buf)
	{
		FFSPrint((DBG_ERROR, "FFSReadDisk: no enough memory.\n"));
		Status = STATUS_INSUFFICIENT_RESOURCES;

		goto errorout;
	}

	Status = FFSReadSync(Vcb, 
			Lba,
			Length,
			Buf,
			FALSE);

	if (!NT_SUCCESS(Status))
	{
		FFSPrint((DBG_ERROR, "FFSReadDisk: Read Block Device error.\n"));

		goto errorout;
	}

	RtlCopyMemory(Buffer, &Buf[Offset - Lba], Size);

errorout:

	if (Buf)
		ExFreePool(Buf);

	return Status;
}


NTSTATUS 
FFSDiskIoControl(
	IN PDEVICE_OBJECT   DeviceObject,
	IN ULONG            IoctlCode,
	IN PVOID            InputBuffer,
	IN ULONG            InputBufferSize,
	IN OUT PVOID        OutputBuffer,
	IN OUT PULONG       OutputBufferSize)
{
	ULONG           OutBufferSize = 0;
	KEVENT          Event;
	PIRP            Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS        Status;

    PAGED_CODE();

	ASSERT(DeviceObject != NULL);

	if (OutputBufferSize)
	{
		OutBufferSize = *OutputBufferSize;
	}

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(
			IoctlCode,
			DeviceObject,
			InputBuffer,
			InputBufferSize,
			OutputBuffer,
			OutBufferSize,
			FALSE,
			&Event,
			&IoStatus);

	if (Irp == NULL)
	{
		FFSPrint((DBG_ERROR, "FFSDiskIoControl: Building IRQ error!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Status = IoCallDriver(DeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
		Status = IoStatus.Status;
	}

	if (OutputBufferSize)
	{
		*OutputBufferSize = (ULONG) IoStatus.Information;
	}

	return Status;
}


NTSTATUS NTAPI
FFSMediaEjectControlCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP           Irp,
	IN PVOID          Contxt)
{
	PKEVENT Event = (PKEVENT)Contxt;

	KeSetEvent(Event, 0, FALSE);

	UNREFERENCED_PARAMETER(DeviceObject);

	return STATUS_SUCCESS;
}


__drv_mustHoldCriticalRegion
VOID
FFSMediaEjectControl(
	IN PFFS_IRP_CONTEXT IrpContext,
	IN PFFS_VCB         Vcb,
	IN BOOLEAN          bPrevent)
{
	PIRP                    Irp;
	KEVENT                  Event;
	NTSTATUS                Status;
	PREVENT_MEDIA_REMOVAL   Prevent;
	IO_STATUS_BLOCK         IoStatus;

    PAGED_CODE();

	ExAcquireResourceExclusiveLite(
			&Vcb->MainResource,
			TRUE);

	if (bPrevent != IsFlagOn(Vcb->Flags, VCB_REMOVAL_PREVENTED))
	{
		if (bPrevent)
		{
			SetFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
		}
		else
		{
			ClearFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
		}
	}

	ExReleaseResourceForThreadLite(
			&Vcb->MainResource,
			ExGetCurrentResourceThread());

	Prevent.PreventMediaRemoval = bPrevent;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_MEDIA_REMOVAL,
			Vcb->TargetDeviceObject,
			&Prevent,
			sizeof(PREVENT_MEDIA_REMOVAL),
			NULL,
			0,
			FALSE,
			NULL,
			&IoStatus);

	if (Irp != NULL)
	{
		IoSetCompletionRoutine(Irp,
				FFSMediaEjectControlCompletion,
				&Event,
				TRUE,
				TRUE,
				TRUE);

		Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

		if (Status == STATUS_PENDING)
		{
			Status = KeWaitForSingleObject(&Event,
					Executive,
					KernelMode,
					FALSE,
					NULL);
		}
	}
}


NTSTATUS
FFSDiskShutDown(
	PFFS_VCB Vcb)
{
	PIRP                Irp;
	KEVENT              Event;
	NTSTATUS            Status;
	IO_STATUS_BLOCK     IoStatus;

    PAGED_CODE();

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
			Vcb->TargetDeviceObject,
			NULL,
			0,
			NULL,
			&Event,
			&IoStatus);

	if (Irp)
	{
		Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

		if (Status == STATUS_PENDING)
		{
			KeWaitForSingleObject(&Event,
					Executive,
					KernelMode,
					FALSE,
					NULL);

			Status = IoStatus.Status;
		}
	}
	else
	{
		Status = IoStatus.Status;
	}

	return Status;
}
