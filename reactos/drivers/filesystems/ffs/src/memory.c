/* 
 * FFS File System Driver for Windows
 *
 * memory.c
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
#pragma alloc_text(PAGE, FFSAllocateIrpContext)
#pragma alloc_text(PAGE, FFSFreeIrpContext)
#pragma alloc_text(PAGE, FFSv1AllocateFcb)
#pragma alloc_text(PAGE, FFSv2AllocateFcb)
#pragma alloc_text(PAGE, FFSFreeFcb)
#pragma alloc_text(PAGE, FFSAllocateMcb)
#pragma alloc_text(PAGE, FFSFreeMcb)
#pragma alloc_text(PAGE, FFSSearchMcbTree)
#pragma alloc_text(PAGE, FFSSearchMcb)
#pragma alloc_text(PAGE, FFSGetFullFileName)
#pragma alloc_text(PAGE, FFSRefreshMcb)
#pragma alloc_text(PAGE, FFSAddMcbNode)
#pragma alloc_text(PAGE, FFSDeleteMcbNode)
#pragma alloc_text(PAGE, FFSFreeMcbTree)
#pragma alloc_text(PAGE, FFSCheckBitmapConsistency)
#pragma alloc_text(PAGE, FFSCheckSetBlock)
#pragma alloc_text(PAGE, FFSInitializeVcb)
#pragma alloc_text(PAGE, FFSFreeCcb)
#pragma alloc_text(PAGE, FFSAllocateCcb)
#pragma alloc_text(PAGE, FFSFreeVcb)
#pragma alloc_text(PAGE, FFSCreateFcbFromMcb)
#pragma alloc_text(PAGE, FFSSyncUninitializeCacheMap)
#endif


__drv_mustHoldCriticalRegion
PFFS_IRP_CONTEXT
FFSAllocateIrpContext(
	IN PDEVICE_OBJECT   DeviceObject,
	IN PIRP             Irp)
{
	PIO_STACK_LOCATION  IoStackLocation;
	PFFS_IRP_CONTEXT    IrpContext;

    PAGED_CODE();

	ASSERT(DeviceObject != NULL);
	ASSERT(Irp != NULL);

	IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->LAResource,
			TRUE);

	IrpContext = (PFFS_IRP_CONTEXT)(ExAllocateFromNPagedLookasideList(&(FFSGlobal->FFSIrpContextLookasideList)));

	ExReleaseResourceForThreadLite(
			&FFSGlobal->LAResource,
			ExGetCurrentResourceThread());

	if (IrpContext == NULL)
	{
		IrpContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(FFS_IRP_CONTEXT), FFS_POOL_TAG);

		//
		//  Zero out the irp context and indicate that it is from pool and
		//  not region allocated
		//

		RtlZeroMemory(IrpContext, sizeof(FFS_IRP_CONTEXT));

		SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL);
	}
	else
	{
		//
		//  Zero out the irp context and indicate that it is from zone and
		//  not pool allocated
		//

		RtlZeroMemory(IrpContext, sizeof(FFS_IRP_CONTEXT));
	}

	if (!IrpContext)
	{
		return NULL;
	}

	IrpContext->Identifier.Type = FFSICX;
	IrpContext->Identifier.Size = sizeof(FFS_IRP_CONTEXT);

	IrpContext->Irp = Irp;

	IrpContext->MajorFunction = IoStackLocation->MajorFunction;
	IrpContext->MinorFunction = IoStackLocation->MinorFunction;

	IrpContext->DeviceObject = DeviceObject;

	IrpContext->FileObject = IoStackLocation->FileObject;

	if (IrpContext->FileObject != NULL)
	{
		IrpContext->RealDevice = IrpContext->FileObject->DeviceObject;
	}
	else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)
	{
		if (IoStackLocation->Parameters.MountVolume.Vpb)
		{
			IrpContext->RealDevice = 
				IoStackLocation->Parameters.MountVolume.Vpb->RealDevice;
		}
	}

	if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
			IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
			IrpContext->MajorFunction == IRP_MJ_SHUTDOWN)
	{
		IrpContext->IsSynchronous = TRUE;
	}
	else if (IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
			IrpContext->MajorFunction == IRP_MJ_CLOSE)
	{
		IrpContext->IsSynchronous = FALSE;
	}
#if (_WIN32_WINNT >= 0x0500)
	else if (IrpContext->MajorFunction == IRP_MJ_PNP)
	{
		if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL)
		{
			IrpContext->IsSynchronous = TRUE;
		}
		else
		{
			IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
		}
	}
#endif //(_WIN32_WINNT >= 0x0500)
	else
	{
		IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
	}

#if 0    
	//
	// Temporary workaround for a bug in close that makes it reference a
	// fileobject when it is no longer valid.
	//
	if (IrpContext->MajorFunction == IRP_MJ_CLOSE)
	{
		IrpContext->IsSynchronous = TRUE;
	}
#endif

	IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);

	IrpContext->ExceptionInProgress = FALSE;

	return IrpContext;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeIrpContext(
	IN PFFS_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

	ASSERT(IrpContext != NULL);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	FFSUnpinRepinnedBcbs(IrpContext);

	//  Return the Irp context record to the region or to pool depending on
	//  its flag

	if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL))
	{
		IrpContext->Identifier.Type = 0;
		IrpContext->Identifier.Size = 0;

		ExFreePool(IrpContext);
	}
	else
	{
		IrpContext->Identifier.Type = 0;
		IrpContext->Identifier.Size = 0;

		ExAcquireResourceExclusiveLite(
				&FFSGlobal->LAResource,
				TRUE);

		ExFreeToNPagedLookasideList(&(FFSGlobal->FFSIrpContextLookasideList), IrpContext);

		ExReleaseResourceForThreadLite(
				&FFSGlobal->LAResource,
				ExGetCurrentResourceThread());

	}
}


VOID
FFSRepinBcb(
	IN PFFS_IRP_CONTEXT IrpContext,
	IN PBCB             Bcb)
{
	PFFS_REPINNED_BCBS Repinned;
	ULONG i;

	Repinned = &IrpContext->Repinned;


	return;

	while (Repinned)
	{
		for (i = 0; i < FFS_REPINNED_BCBS_ARRAY_SIZE; i += 1)
		{
			if (Repinned->Bcb[i] == Bcb)
			{
				return;
			}
		}

		Repinned = Repinned->Next;
	}

	while (TRUE)
	{
		for (i = 0; i < FFS_REPINNED_BCBS_ARRAY_SIZE; i += 1)
		{
			if (Repinned->Bcb[i] == Bcb)
			{
				return;
			}

			if (Repinned->Bcb[i] == NULL)
			{
				Repinned->Bcb[i] = Bcb;
				CcRepinBcb(Bcb);

				return;
			}
		}

		if (Repinned->Next == NULL)
		{
			Repinned->Next = ExAllocatePoolWithTag(PagedPool, sizeof(FFS_REPINNED_BCBS), FFS_POOL_TAG);
			RtlZeroMemory(Repinned->Next, sizeof(FFS_REPINNED_BCBS));
		}

		Repinned = Repinned->Next;
	}
}


VOID
FFSUnpinRepinnedBcbs(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	IO_STATUS_BLOCK    RaiseIosb;
	PFFS_REPINNED_BCBS Repinned;
	BOOLEAN            WriteThroughToDisk;
	PFILE_OBJECT       FileObject = NULL;
	BOOLEAN            ForceVerify = FALSE;
	ULONG              i;

	Repinned = &IrpContext->Repinned;
	RaiseIosb.Status = STATUS_SUCCESS;

	WriteThroughToDisk = (BOOLEAN)(IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH) ||
			IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY));

	while (Repinned != NULL)
	{
		for (i = 0; i < FFS_REPINNED_BCBS_ARRAY_SIZE; i += 1)
		{
			if (Repinned->Bcb[i] != NULL)
			{
				IO_STATUS_BLOCK Iosb;

				ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

				if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY))
				{
					FileObject = CcGetFileObjectFromBcb(Repinned->Bcb[i]);
				}

				ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

				CcUnpinRepinnedBcb(Repinned->Bcb[i],
						WriteThroughToDisk,
						&Iosb);

				ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

				if (!NT_SUCCESS(Iosb.Status))
				{
					if (RaiseIosb.Status == STATUS_SUCCESS)
					{
						RaiseIosb = Iosb;
					}

					if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY) &&
							(IrpContext->MajorFunction != IRP_MJ_CLEANUP) &&
							(IrpContext->MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
							(IrpContext->MajorFunction != IRP_MJ_SET_INFORMATION))
					{

						CcPurgeCacheSection(FileObject->SectionObjectPointer,
								NULL,
								0,
								FALSE);

						ForceVerify = TRUE;
					}
				}

				Repinned->Bcb[i] = NULL;

			}
			else
			{
				break;
			}
		}

		if (Repinned != &IrpContext->Repinned)
		{
			PFFS_REPINNED_BCBS Saved;

			Saved = Repinned->Next;
			ExFreePool(Repinned);
			Repinned = Saved;

		}
		else
		{
			Repinned = Repinned->Next;
			IrpContext->Repinned.Next = NULL;
		}
	}

	if (!NT_SUCCESS(RaiseIosb.Status))
	{
		FFSBreakPoint();

		if (ForceVerify && FileObject)
		{
			SetFlag(FileObject->DeviceObject->Flags, DO_VERIFY_VOLUME);
			IoSetHardErrorOrVerifyDevice(IrpContext->Irp,
					FileObject->DeviceObject);
		}

		IrpContext->Irp->IoStatus = RaiseIosb;
		FFSNormalizeAndRaiseStatus(IrpContext, RaiseIosb.Status);
	}

	return;
}


__drv_mustHoldCriticalRegion
PFFS_FCB
FFSv1AllocateFcb(
	IN PFFS_VCB           Vcb,
	IN PFFS_MCB           FFSMcb,
	IN PFFSv1_INODE       dinode1)
{
	PFFS_FCB Fcb;

    PAGED_CODE();

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->LAResource,
			TRUE);

	Fcb = (PFFS_FCB)(ExAllocateFromNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList)));

	ExReleaseResourceForThreadLite(
			&FFSGlobal->LAResource,
			ExGetCurrentResourceThread());

	if (Fcb == NULL)
	{
		Fcb = (PFFS_FCB)ExAllocatePoolWithTag(NonPagedPool, sizeof(FFS_FCB), FFS_POOL_TAG);

		RtlZeroMemory(Fcb, sizeof(FFS_FCB));

		SetFlag(Fcb->Flags, FCB_FROM_POOL);
	}
	else
	{
		RtlZeroMemory(Fcb, sizeof(FFS_FCB));
	}

	if (!Fcb)
	{
		return NULL;
	}

	Fcb->Identifier.Type = FFSFCB;
	Fcb->Identifier.Size = sizeof(FFS_FCB);

	FsRtlInitializeFileLock(
			&Fcb->FileLockAnchor,
			NULL,
			NULL);

	Fcb->OpenHandleCount = 0;
	Fcb->ReferenceCount = 0;

	Fcb->Vcb = Vcb;

#if DBG    

	Fcb->AnsiFileName.MaximumLength = (USHORT)
		RtlxUnicodeStringToOemSize(&(FFSMcb->ShortName)) + 1;

	Fcb->AnsiFileName.Buffer = (PUCHAR) 
		ExAllocatePoolWithTag(PagedPool, Fcb->AnsiFileName.MaximumLength, FFS_POOL_TAG);

	if (!Fcb->AnsiFileName.Buffer)
	{
		goto errorout;
	}

	RtlZeroMemory(Fcb->AnsiFileName.Buffer, Fcb->AnsiFileName.MaximumLength);

	FFSUnicodeToOEM(&(Fcb->AnsiFileName),
			&(FFSMcb->ShortName));

#endif

	FFSMcb->FileAttr = FILE_ATTRIBUTE_NORMAL;

	if ((dinode1->di_mode & IFMT) == IFDIR)
	{
		SetFlag(FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
	}

	if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) || 
			FFSIsReadOnly(dinode1->di_mode))
	{
		SetFlag(FFSMcb->FileAttr, FILE_ATTRIBUTE_READONLY);
	}

	Fcb->dinode1 = dinode1;

	Fcb->FFSMcb = FFSMcb;
	FFSMcb->FFSFcb = Fcb;

	RtlZeroMemory(&Fcb->Header, sizeof(FSRTL_COMMON_FCB_HEADER));

	Fcb->Header.NodeTypeCode = (USHORT)FFSFCB;
	Fcb->Header.NodeByteSize = sizeof(FFS_FCB);
	Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
	Fcb->Header.Resource = &(Fcb->MainResource);
	Fcb->Header.PagingIoResource = &(Fcb->PagingIoResource);

	{
		ULONG Totalblocks = (Fcb->dinode1->di_blocks);
		Fcb->Header.AllocationSize.QuadPart = 
			(((LONGLONG)FFSDataBlocks(Vcb, Totalblocks)) << BLOCK_BITS);
	}

	Fcb->Header.FileSize.QuadPart = (LONGLONG)(Fcb->dinode1->di_size);
	Fcb->Header.ValidDataLength.QuadPart = (LONGLONG)(0x7fffffffffffffff);

	Fcb->SectionObject.DataSectionObject = NULL;
	Fcb->SectionObject.SharedCacheMap = NULL;
	Fcb->SectionObject.ImageSectionObject = NULL;

	ExInitializeResourceLite(&(Fcb->MainResource));
	ExInitializeResourceLite(&(Fcb->PagingIoResource));

	InsertTailList(&Vcb->FcbList, &Fcb->Next);

#if DBG

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->CountResource,
			TRUE);

	FFSGlobal->FcbAllocated++;

	ExReleaseResourceForThreadLite(
			&FFSGlobal->CountResource,
			ExGetCurrentResourceThread());
#endif

	return Fcb;

#if DBG
errorout:
#endif

	if (Fcb)
	{

#if DBG
		if (Fcb->AnsiFileName.Buffer)
			ExFreePool(Fcb->AnsiFileName.Buffer);
#endif

		if (FlagOn(Fcb->Flags, FCB_FROM_POOL))
		{
			ExFreePool(Fcb);
		}
		else
		{
			ExAcquireResourceExclusiveLite(
					&FFSGlobal->LAResource,
					TRUE);

			ExFreeToNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList), Fcb);

			ExReleaseResourceForThreadLite(
					&FFSGlobal->LAResource,
					ExGetCurrentResourceThread());
		}

	}

	return NULL;
}


__drv_mustHoldCriticalRegion
PFFS_FCB
FFSv2AllocateFcb(
	IN PFFS_VCB           Vcb,
	IN PFFS_MCB           FFSMcb,
	IN PFFSv2_INODE       dinode2)
{
	PFFS_FCB Fcb;

    PAGED_CODE();

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->LAResource,
			TRUE);

	Fcb = (PFFS_FCB)(ExAllocateFromNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList)));

	ExReleaseResourceForThreadLite(
			&FFSGlobal->LAResource,
			ExGetCurrentResourceThread());

	if (Fcb == NULL)
	{
		Fcb = (PFFS_FCB)ExAllocatePoolWithTag(NonPagedPool, sizeof(FFS_FCB), FFS_POOL_TAG);

		RtlZeroMemory(Fcb, sizeof(FFS_FCB));

		SetFlag(Fcb->Flags, FCB_FROM_POOL);
	}
	else
	{
		RtlZeroMemory(Fcb, sizeof(FFS_FCB));
	}

	if (!Fcb)
	{
		return NULL;
	}

	Fcb->Identifier.Type = FFSFCB;
	Fcb->Identifier.Size = sizeof(FFS_FCB);

	FsRtlInitializeFileLock(
			&Fcb->FileLockAnchor,
			NULL,
			NULL);

	Fcb->OpenHandleCount = 0;
	Fcb->ReferenceCount = 0;

	Fcb->Vcb = Vcb;

#if DBG    

	Fcb->AnsiFileName.MaximumLength = (USHORT)
		RtlxUnicodeStringToOemSize(&(FFSMcb->ShortName)) + 1;

	Fcb->AnsiFileName.Buffer = (PUCHAR) 
		ExAllocatePoolWithTag(PagedPool, Fcb->AnsiFileName.MaximumLength, FFS_POOL_TAG);

	if (!Fcb->AnsiFileName.Buffer)
	{
		goto errorout;
	}

	RtlZeroMemory(Fcb->AnsiFileName.Buffer, Fcb->AnsiFileName.MaximumLength);

	FFSUnicodeToOEM(&(Fcb->AnsiFileName),
			&(FFSMcb->ShortName));

#endif

	FFSMcb->FileAttr = FILE_ATTRIBUTE_NORMAL;

	if ((dinode2->di_mode & IFMT) == IFDIR)
	{
		SetFlag(FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
	}

	if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) || 
			FFSIsReadOnly(dinode2->di_mode))
	{
		SetFlag(FFSMcb->FileAttr, FILE_ATTRIBUTE_READONLY);
	}

	Fcb->dinode2 = dinode2;

	Fcb->FFSMcb = FFSMcb;
	FFSMcb->FFSFcb = Fcb;

	RtlZeroMemory(&Fcb->Header, sizeof(FSRTL_COMMON_FCB_HEADER));

	Fcb->Header.NodeTypeCode = (USHORT)FFSFCB;
	Fcb->Header.NodeByteSize = sizeof(FFS_FCB);
	Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
	Fcb->Header.Resource = &(Fcb->MainResource);
	Fcb->Header.PagingIoResource = &(Fcb->PagingIoResource);

	{
		ULONG Totalblocks = (ULONG)(Fcb->dinode2->di_blocks);
		Fcb->Header.AllocationSize.QuadPart = 
			(((LONGLONG)FFSDataBlocks(Vcb, Totalblocks)) << BLOCK_BITS);
	}

	Fcb->Header.FileSize.QuadPart = (LONGLONG)(Fcb->dinode2->di_size);
	Fcb->Header.ValidDataLength.QuadPart = (LONGLONG)(0x7fffffffffffffff);

	Fcb->SectionObject.DataSectionObject = NULL;
	Fcb->SectionObject.SharedCacheMap = NULL;
	Fcb->SectionObject.ImageSectionObject = NULL;

	ExInitializeResourceLite(&(Fcb->MainResource));
	ExInitializeResourceLite(&(Fcb->PagingIoResource));

	InsertTailList(&Vcb->FcbList, &Fcb->Next);

#if DBG

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->CountResource,
			TRUE);

	FFSGlobal->FcbAllocated++;

	ExReleaseResourceForThreadLite(
			&FFSGlobal->CountResource,
			ExGetCurrentResourceThread());
#endif

	return Fcb;

#if DBG
errorout:
#endif

	if (Fcb)
	{

#if DBG
		if (Fcb->AnsiFileName.Buffer)
			ExFreePool(Fcb->AnsiFileName.Buffer);
#endif

		if (FlagOn(Fcb->Flags, FCB_FROM_POOL))
		{
			ExFreePool(Fcb);
		}
		else
		{
			ExAcquireResourceExclusiveLite(
					&FFSGlobal->LAResource,
					TRUE);

			ExFreeToNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList), Fcb);

			ExReleaseResourceForThreadLite(
					&FFSGlobal->LAResource,
					ExGetCurrentResourceThread());
		}

	}

	return NULL;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeFcb(
	IN PFFS_FCB Fcb)
{
	PFFS_VCB       Vcb;

    PAGED_CODE();

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	Vcb = Fcb->Vcb;

	FsRtlUninitializeFileLock(&Fcb->FileLockAnchor);

	ExDeleteResourceLite(&Fcb->MainResource);

	ExDeleteResourceLite(&Fcb->PagingIoResource);

	Fcb->FFSMcb->FFSFcb = NULL;

	if(IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
	{
		if (Fcb->FFSMcb)
		{
			FFSDeleteMcbNode(Vcb, Fcb->FFSMcb->Parent, Fcb->FFSMcb);
			FFSFreeMcb(Fcb->FFSMcb);
		}
	}

	if (Fcb->LongName.Buffer)
	{
		ExFreePool(Fcb->LongName.Buffer);
		Fcb->LongName.Buffer = NULL;
	}

#if DBG    
	ExFreePool(Fcb->AnsiFileName.Buffer);
#endif

	if (FS_VERSION == 1)
	{
		ExFreePool(Fcb->dinode1);
	}
	else
	{
		ExFreePool(Fcb->dinode2);
	}

	Fcb->Header.NodeTypeCode = (SHORT)0xCCCC;
	Fcb->Header.NodeByteSize = (SHORT)0xC0C0;

	if (FlagOn(Fcb->Flags, FCB_FROM_POOL))
	{
		ExFreePool(Fcb);
	}
	else
	{
		ExAcquireResourceExclusiveLite(
				&FFSGlobal->LAResource,
				TRUE);

		ExFreeToNPagedLookasideList(&(FFSGlobal->FFSFcbLookasideList), Fcb);

		ExReleaseResourceForThreadLite(
				&FFSGlobal->LAResource,
				ExGetCurrentResourceThread());
	}

#if DBG

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->CountResource,
			TRUE);

	FFSGlobal->FcbAllocated--;

	ExReleaseResourceForThreadLite(
			&FFSGlobal->CountResource,
			ExGetCurrentResourceThread());
#endif

}


__drv_mustHoldCriticalRegion
PFFS_CCB
FFSAllocateCcb(
	VOID)
{
	PFFS_CCB Ccb;

    PAGED_CODE();

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->LAResource,
			TRUE);

	Ccb = (PFFS_CCB)(ExAllocateFromNPagedLookasideList(&(FFSGlobal->FFSCcbLookasideList)));

	ExReleaseResourceForThreadLite(
			&FFSGlobal->LAResource,
			ExGetCurrentResourceThread());

	if (Ccb == NULL)
	{
		Ccb = (PFFS_CCB)ExAllocatePoolWithTag(NonPagedPool, sizeof(FFS_CCB), FFS_POOL_TAG);

		RtlZeroMemory(Ccb, sizeof(FFS_CCB));

		SetFlag(Ccb->Flags, CCB_FROM_POOL);
	}
	else
	{
		RtlZeroMemory(Ccb, sizeof(FFS_CCB));
	}

	if (!Ccb)
	{
		return NULL;
	}

	Ccb->Identifier.Type = FFSCCB;
	Ccb->Identifier.Size = sizeof(FFS_CCB);

	Ccb->CurrentByteOffset = 0;

	Ccb->DirectorySearchPattern.Length = 0;
	Ccb->DirectorySearchPattern.MaximumLength = 0;
	Ccb->DirectorySearchPattern.Buffer = 0;

	return Ccb;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeCcb(
	IN PFFS_CCB Ccb)
{
    PAGED_CODE();

	ASSERT(Ccb != NULL);

	ASSERT((Ccb->Identifier.Type == FFSCCB) &&
			(Ccb->Identifier.Size == sizeof(FFS_CCB)));

	if (Ccb->DirectorySearchPattern.Buffer != NULL)
	{
		ExFreePool(Ccb->DirectorySearchPattern.Buffer);
	}

	if (FlagOn(Ccb->Flags, CCB_FROM_POOL))
	{
		ExFreePool(Ccb);
	}
	else
	{
		ExAcquireResourceExclusiveLite(
				&FFSGlobal->LAResource,
				TRUE);

		ExFreeToNPagedLookasideList(&(FFSGlobal->FFSCcbLookasideList), Ccb);

		ExReleaseResourceForThreadLite(
				&FFSGlobal->LAResource,
				ExGetCurrentResourceThread());
	}
}


__drv_mustHoldCriticalRegion
PFFS_MCB
FFSAllocateMcb(
	PFFS_VCB        Vcb,
	PUNICODE_STRING FileName,
	ULONG           FileAttr)
{
	PFFS_MCB    Mcb = NULL;
	PLIST_ENTRY List = NULL;

	ULONG       Extra = 0;

    PAGED_CODE();

#define MCB_NUM_SHIFT   0x04

	if (FFSGlobal->McbAllocated > (FFSGlobal->MaxDepth << MCB_NUM_SHIFT))
		Extra = FFSGlobal->McbAllocated - 
			(FFSGlobal->MaxDepth << MCB_NUM_SHIFT) +
			FFSGlobal->MaxDepth;

	FFSPrint((DBG_INFO,
				"FFSAllocateMcb: CurrDepth=%xh/%xh/%xh FileName=%S\n", 
				FFSGlobal->McbAllocated,
				FFSGlobal->MaxDepth << MCB_NUM_SHIFT,
				FFSGlobal->FcbAllocated,
				FileName->Buffer));

	List = Vcb->McbList.Flink;

	while ((List != &(Vcb->McbList)) && (Extra > 0))
	{
		Mcb = CONTAINING_RECORD(List, FFS_MCB, Link);
		List = List->Flink;

		if ((Mcb->Inode != 2) && (Mcb->Child == NULL) &&
				(Mcb->FFSFcb == NULL) && (!IsMcbUsed(Mcb)))
		{
			FFSPrint((DBG_INFO, "FFSAllocateMcb: Mcb %S will be freed.\n",
						Mcb->ShortName.Buffer));

			if (FFSDeleteMcbNode(Vcb, Vcb->McbTree, Mcb))
			{
				FFSFreeMcb(Mcb);

				Extra--;
			}
		}
	}

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->LAResource,
			TRUE);

	Mcb = (PFFS_MCB)(ExAllocateFromPagedLookasideList(
				&(FFSGlobal->FFSMcbLookasideList)));

	ExReleaseResourceForThreadLite(
			&FFSGlobal->LAResource,
			ExGetCurrentResourceThread());

	if (Mcb == NULL)
	{
		Mcb = (PFFS_MCB)ExAllocatePoolWithTag(PagedPool, sizeof(FFS_MCB), FFS_POOL_TAG);

		RtlZeroMemory(Mcb, sizeof(FFS_MCB));

		SetFlag(Mcb->Flags, MCB_FROM_POOL);
	}
	else
	{
		RtlZeroMemory(Mcb, sizeof(FFS_MCB));
	}

	if (!Mcb)
	{
		return NULL;
	}

	Mcb->Identifier.Type = FFSMCB;
	Mcb->Identifier.Size = sizeof(FFS_MCB);

	if (FileName && FileName->Length)
	{
		Mcb->ShortName.Length = FileName->Length;
		Mcb->ShortName.MaximumLength = Mcb->ShortName.Length + 2;

		Mcb->ShortName.Buffer = ExAllocatePoolWithTag(PagedPool, Mcb->ShortName.MaximumLength, FFS_POOL_TAG);

		if (!Mcb->ShortName.Buffer)
			goto errorout;

		RtlZeroMemory(Mcb->ShortName.Buffer, Mcb->ShortName.MaximumLength);
		RtlCopyMemory(Mcb->ShortName.Buffer, FileName->Buffer, Mcb->ShortName.Length);
	} 

	Mcb->FileAttr = FileAttr;

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->CountResource,
			TRUE);

	FFSGlobal->McbAllocated++;

	ExReleaseResourceForThreadLite(
			&FFSGlobal->CountResource,
			ExGetCurrentResourceThread());

	return Mcb;

errorout:

	if (Mcb)
	{
		if (Mcb->ShortName.Buffer)
			ExFreePool(Mcb->ShortName.Buffer);

		if (FlagOn(Mcb->Flags, MCB_FROM_POOL))
		{
			ExFreePool(Mcb);
		}
		else
		{
			ExAcquireResourceExclusiveLite(
					&FFSGlobal->LAResource,
					TRUE);

			ExFreeToPagedLookasideList(&(FFSGlobal->FFSMcbLookasideList), Mcb);

			ExReleaseResourceForThreadLite(
					&FFSGlobal->LAResource,
					ExGetCurrentResourceThread());
		}
	}

	return NULL;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeMcb(
	IN PFFS_MCB Mcb)
{
#ifndef __REACTOS__
    PFFS_MCB   Parent = Mcb->Parent;
#endif
    PAGED_CODE();

	ASSERT(Mcb != NULL);

	ASSERT((Mcb->Identifier.Type == FFSMCB) &&
			(Mcb->Identifier.Size == sizeof(FFS_MCB)));

	FFSPrint((DBG_INFO, "FFSFreeMcb: Mcb %S will be freed.\n", Mcb->ShortName.Buffer));

	if (Mcb->ShortName.Buffer)
		ExFreePool(Mcb->ShortName.Buffer);

	if (FlagOn(Mcb->Flags, MCB_FROM_POOL))
	{
		ExFreePool(Mcb);
	}
	else
	{
		ExAcquireResourceExclusiveLite(
				&FFSGlobal->LAResource,
				TRUE);

		ExFreeToPagedLookasideList(&(FFSGlobal->FFSMcbLookasideList), Mcb);

		ExReleaseResourceForThreadLite(
				&FFSGlobal->LAResource,
				ExGetCurrentResourceThread());
	}

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->CountResource,
			TRUE);

	FFSGlobal->McbAllocated--;

	ExReleaseResourceForThreadLite(
			&FFSGlobal->CountResource,
			ExGetCurrentResourceThread());
}


__drv_mustHoldCriticalRegion
PFFS_FCB
FFSCreateFcbFromMcb(
	PFFS_VCB Vcb,
	PFFS_MCB Mcb)
{
	PFFS_FCB       Fcb = NULL;
	FFSv1_INODE    dinode1;
	FFSv2_INODE    dinode2;

    PAGED_CODE();

	if (Mcb->FFSFcb)
		return Mcb->FFSFcb;

	if (FS_VERSION == 1)
	{
		if (FFSv1LoadInode(Vcb, Mcb->Inode, &dinode1))
		{
			PFFSv1_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, DINODE1_SIZE, FFS_POOL_TAG);
			if (!pTmpInode)
			{
				goto errorout;
			}

			RtlCopyMemory(pTmpInode, &dinode1, DINODE1_SIZE);
			Fcb = FFSv1AllocateFcb(Vcb, Mcb, pTmpInode);
			if (!Fcb)
			{
				ExFreePool(pTmpInode);
			}
		}
	}
	else
	{
		if (FFSv2LoadInode(Vcb, Mcb->Inode, &dinode2))
		{
			PFFSv2_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, DINODE2_SIZE, FFS_POOL_TAG);
			if (!pTmpInode)
			{
				goto errorout;
			}

			RtlCopyMemory(pTmpInode, &dinode2, DINODE2_SIZE);
			Fcb = FFSv2AllocateFcb(Vcb, Mcb, pTmpInode);
			if (!Fcb)
			{
				ExFreePool(pTmpInode);
			}
		}
	}

errorout:

	return Fcb;
}


BOOLEAN
FFSGetFullFileName(
	PFFS_MCB        Mcb,
	PUNICODE_STRING FileName)
{
	USHORT          Length = 0;
	PFFS_MCB        TmpMcb = Mcb;
	PUNICODE_STRING FileNames[256];
	SHORT           Count = 0 , i = 0, j = 0;

    PAGED_CODE();

	while(TmpMcb && Count < 256)
	{
		if (TmpMcb->Inode == FFS_ROOT_INO)
			break;

		FileNames[Count++] = &TmpMcb->ShortName;
		Length += (2 + TmpMcb->ShortName.Length);

		TmpMcb = TmpMcb->Parent;
	}

	if (Count >= 256)
		return FALSE;

	if (Count == 0)
		Length = 2;

	FileName->Length = Length;
	FileName->MaximumLength = Length + 2;
	FileName->Buffer = ExAllocatePoolWithTag(PagedPool, Length + 2, FFS_POOL_TAG);

	if (!FileName->Buffer)
	{
		return FALSE;
	}

	RtlZeroMemory(FileName->Buffer, FileName->MaximumLength);

	if (Count == 0)
	{
		FileName->Buffer[0] = L'\\';
		return  TRUE;
	}

	for (i = Count - 1; i >= 0 && j < (SHORT)(FileName->MaximumLength); i--)
	{
		FileName->Buffer[j++] = L'\\';

		RtlCopyMemory(&(FileName->Buffer[j]),
				FileNames[i]->Buffer,
				FileNames[i]->Length);

		j += FileNames[i]->Length / 2;
	}

	return TRUE;
}


PFFS_MCB
FFSSearchMcbTree(
	PFFS_VCB Vcb,
	PFFS_MCB FFSMcb,
	ULONG    Inode)
{
	PFFS_MCB    Mcb = NULL;
	PLIST_ENTRY List = NULL;
	BOOLEAN     bFind = FALSE;

    PAGED_CODE();

	List = Vcb->McbList.Flink;

	while ((!bFind) && (List != &(Vcb->McbList)))
	{
		Mcb = CONTAINING_RECORD(List, FFS_MCB, Link);
		List = List->Flink;

		if (Mcb->Inode == Inode)
		{
			bFind = TRUE;
			break;
		}
	}

	if (bFind)
	{
		ASSERT(Mcb != NULL);
		FFSRefreshMcb(Vcb, Mcb);
	}
	else
	{
		Mcb = NULL;
	}

	return Mcb;
}


PFFS_MCB
FFSSearchMcb(
	PFFS_VCB        Vcb,
	PFFS_MCB        Parent,
	PUNICODE_STRING FileName)
{
	PFFS_MCB TmpMcb = Parent->Child;

    PAGED_CODE();

	while (TmpMcb)
	{
		if (!RtlCompareUnicodeString(
					&(TmpMcb->ShortName),
					FileName, TRUE))
			break;

		TmpMcb = TmpMcb->Next;
	}

	if (TmpMcb)
	{
		FFSRefreshMcb(Vcb, TmpMcb);
	}

	return TmpMcb;
}


VOID
FFSRefreshMcb(
	PFFS_VCB Vcb,
	PFFS_MCB Mcb)
{
    PAGED_CODE();

	ASSERT (IsFlagOn(Mcb->Flags, MCB_IN_TREE));

	RemoveEntryList(&(Mcb->Link));
	InsertTailList(&(Vcb->McbList), &(Mcb->Link));
}


VOID
FFSAddMcbNode(
	PFFS_VCB Vcb,
	PFFS_MCB Parent,
	PFFS_MCB Child)
{
	PFFS_MCB TmpMcb = Parent->Child;

    PAGED_CODE();

	if(IsFlagOn(Child->Flags, MCB_IN_TREE))
	{
		FFSBreakPoint();
		FFSPrint((DBG_ERROR, "FFSAddMcbNode: Child Mcb is alreay in the tree.\n"));
		return;
	}

	if (TmpMcb)
	{
		ASSERT(TmpMcb->Parent == Parent);

		while (TmpMcb->Next)
		{
			TmpMcb = TmpMcb->Next;
			ASSERT(TmpMcb->Parent == Parent);
		}

		TmpMcb->Next = Child;
		Child->Parent = Parent;
		Child->Next = NULL;
	}
	else
	{
		Parent->Child = Child;
		Child->Parent = Parent;
		Child->Next = NULL;
	}

	InsertTailList(&(Vcb->McbList), &(Child->Link));
	SetFlag(Child->Flags, MCB_IN_TREE);
}


BOOLEAN
FFSDeleteMcbNode(
	PFFS_VCB Vcb,
	PFFS_MCB McbTree,
	PFFS_MCB FFSMcb)
{
	PFFS_MCB   TmpMcb;

    PAGED_CODE();

	if(!IsFlagOn(FFSMcb->Flags, MCB_IN_TREE))
	{
		return TRUE;
	}

	if (FFSMcb->Parent)
	{
		if (FFSMcb->Parent->Child == FFSMcb)
		{
			FFSMcb->Parent->Child = FFSMcb->Next;
		}
		else
		{
			TmpMcb = FFSMcb->Parent->Child;

			while (TmpMcb && TmpMcb->Next != FFSMcb)
				TmpMcb = TmpMcb->Next;

			if (TmpMcb)
			{
				TmpMcb->Next = FFSMcb->Next;
			}
			else
			{
				// error
				return FALSE;
			}
		}
	}
	else if (FFSMcb->Child)
	{
		return FALSE;
	}

	RemoveEntryList(&(FFSMcb->Link));
	ClearFlag(FFSMcb->Flags, MCB_IN_TREE);

	return TRUE;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeMcbTree(
	PFFS_MCB McbTree)
{
    PAGED_CODE();

	if (!McbTree)
		return;

	if (McbTree->Child)
	{
		FFSFreeMcbTree(McbTree->Child);
	}

	if (McbTree->Next)
	{
		PFFS_MCB   Current;
		PFFS_MCB   Next;

		Current = McbTree->Next;

		while (Current)
		{
			Next = Current->Next;

			if (Current->Child)
			{
				FFSFreeMcbTree(Current->Child);
			}

			FFSFreeMcb(Current);
			Current = Next;
		}
	}

	FFSFreeMcb(McbTree);
}


BOOLEAN
FFSCheckSetBlock(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb,
	ULONG            Block)
{
    PAGED_CODE();
#if 0
	ULONG           Group, dwBlk, Length;

	RTL_BITMAP      BlockBitmap;
	PVOID           BitmapCache;
	PBCB            BitmapBcb;

	LARGE_INTEGER   Offset;

	BOOLEAN         bModified = FALSE;


	//Group = (Block - FFS_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;

	dwBlk = (Block - FFS_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;


	Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
	Offset.QuadPart = Offset.QuadPart * Vcb->ffs_group_desc[Group].bg_block_bitmap;

	if (Group == Vcb->ffs_groups - 1)
	{
		Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

		/* s_blocks_count is integer multiple of s_blocks_per_group */
		if (Length == 0)
			Length = BLOCKS_PER_GROUP;
	}
	else
	{
		Length = BLOCKS_PER_GROUP;
	}

	if (dwBlk >= Length)
		return FALSE;

	if (!CcPinRead(Vcb->StreamObj,
				&Offset,
				Vcb->BlockSize,
				PIN_WAIT,
				&BitmapBcb,
				&BitmapCache))
	{
		FFSPrint((DBG_ERROR, "FFSDeleteBlock: PinReading error ...\n"));
		return FALSE;
	}

	RtlInitializeBitMap(&BlockBitmap,
			BitmapCache,
			Length);

	if (RtlCheckBit(&BlockBitmap, dwBlk) == 0)
	{
		FFSBreakPoint();
		RtlSetBits(&BlockBitmap, dwBlk, 1);
		bModified = TRUE;
	}

	if (bModified)
	{
		CcSetDirtyPinnedData(BitmapBcb, NULL);

		FFSRepinBcb(IrpContext, BitmapBcb);

		FFSAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);
	}

	{
		CcUnpinData(BitmapBcb);
		BitmapBcb = NULL;
		BitmapCache = NULL;

		RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
	}

	return (!bModified);
#endif
	return FALSE;
}


BOOLEAN
FFSCheckBitmapConsistency(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
    PAGED_CODE();
#if 0
	ULONG i, j, InodeBlocks;

	for (i = 0; i < Vcb->ffs_groups; i++)
	{
		FFSCheckSetBlock(IrpContext, Vcb, Vcb->ffs_group_desc[i].bg_block_bitmap);
		FFSCheckSetBlock(IrpContext, Vcb, Vcb->ffs_group_desc[i].bg_inode_bitmap);


		if (i == Vcb->ffs_groups - 1)
		{
			InodeBlocks = ((INODES_COUNT % INODES_PER_GROUP) * sizeof(FFS_INODE) + Vcb->BlockSize - 1) / (Vcb->BlockSize);
		}
		else
		{
			InodeBlocks = (INODES_PER_GROUP * sizeof(FFS_INODE) + Vcb->BlockSize - 1) / (Vcb->BlockSize);
		}

		for (j = 0; j < InodeBlocks; j++)
			FFSCheckSetBlock(IrpContext, Vcb, Vcb->ffs_group_desc[i].bg_inode_table + j);
	}

	return TRUE;
#endif
	return FALSE;
}


VOID
FFSInsertVcb(
	PFFS_VCB Vcb)
{
	InsertTailList(&(FFSGlobal->VcbList), &Vcb->Next);
}


VOID
FFSRemoveVcb(
	PFFS_VCB Vcb)
{
	RemoveEntryList(&Vcb->Next);
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSInitializeVcb(
	IN PFFS_IRP_CONTEXT IrpContext, 
	IN PFFS_VCB         Vcb, 
	IN PFFS_SUPER_BLOCK FFSSb,
	IN PDEVICE_OBJECT   TargetDevice,
	IN PDEVICE_OBJECT   VolumeDevice,
	IN PVPB             Vpb)
{
	BOOLEAN                     VcbResourceInitialized = FALSE;
	USHORT                      VolumeLabelLength;
	ULONG                       IoctlSize;
	BOOLEAN                     NotifySyncInitialized = FALSE;
	LONGLONG                    DiskSize;
	LONGLONG                    PartSize;
	NTSTATUS                    Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING              RootNode;
	USHORT                      Buffer[2];
	ULONG                       ChangeCount;

    PAGED_CODE();

	_SEH2_TRY
	{
		if (!Vpb)
		{
			Status = STATUS_DEVICE_NOT_READY;
			_SEH2_LEAVE;
		}

		Buffer[0] = L'\\';
		Buffer[1] = 0;

		RootNode.Buffer = Buffer;
		RootNode.MaximumLength = RootNode.Length = 2;

		Vcb->McbTree = FFSAllocateMcb(Vcb, &RootNode,
				FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL);

		if (!Vcb->McbTree)
		{
			_SEH2_LEAVE;
		}

#if FFS_READ_ONLY
		SetFlag(Vcb->Flags, VCB_READ_ONLY);
#endif // FFS_READ_ONLY

		if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA))
		{
			SetFlag(Vcb->Flags, VCB_REMOVABLE_MEDIA);
		}

		if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_FLOPPY_DISKETTE))
		{
			SetFlag(Vcb->Flags, VCB_FLOPPY_DISK);
		}
#if 0
		if (IsFlagOn(FFSGlobal->Flags, FFS_SUPPORT_WRITING))
		{
			if (IsFlagOn(FFSGlobal->Flags, FFS_SUPPORT_WRITING))
			{
				ClearFlag(Vcb->Flags, VCB_READ_ONLY);
			}
			else
			{
				SetFlag(Vcb->Flags, VCB_READ_ONLY);
			}
		}
		else
#endif
		{
			SetFlag(Vcb->Flags, VCB_READ_ONLY);
		}

		ExInitializeResourceLite(&Vcb->MainResource);
		ExInitializeResourceLite(&Vcb->PagingIoResource);

		ExInitializeResourceLite(&Vcb->McbResource);

		VcbResourceInitialized = TRUE;

		Vcb->McbTree->Inode = FFS_ROOT_INO;

		Vcb->Vpb = Vpb;

		Vcb->RealDevice = Vpb->RealDevice;
		Vpb->DeviceObject = VolumeDevice;

		{
			UNICODE_STRING      LabelName;
			OEM_STRING          OemName;

			LabelName.MaximumLength = 16 * 2;
			LabelName.Length    = 0;
			LabelName.Buffer    = Vcb->Vpb->VolumeLabel;

			RtlZeroMemory(LabelName.Buffer, LabelName.MaximumLength);

			VolumeLabelLength = 16;

			while((VolumeLabelLength > 0) &&
					((FFSSb->fs_volname[VolumeLabelLength-1] == 0x00) ||
					 (FFSSb->fs_volname[VolumeLabelLength-1] == 0x20)))
			{
				VolumeLabelLength--;
			}

			OemName.Buffer = FFSSb->fs_volname;
			OemName.MaximumLength = 16;
			OemName.Length = VolumeLabelLength;

			Status = FFSOEMToUnicode(&LabelName,
					&OemName);

			if (!NT_SUCCESS(Status))
			{
				_SEH2_LEAVE;
			}

			Vpb->VolumeLabelLength = LabelName.Length;
		}

		Vpb->SerialNumber = ((ULONG*)FFSSb->fs_id)[0] + ((ULONG*)FFSSb->fs_id)[1];

		Vcb->StreamObj = IoCreateStreamFileObject(NULL, Vcb->Vpb->RealDevice);

		if (Vcb->StreamObj)
		{
			Vcb->StreamObj->SectionObjectPointer = &(Vcb->SectionObject);
			Vcb->StreamObj->Vpb = Vcb->Vpb;
			Vcb->StreamObj->ReadAccess = TRUE;
			if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
			{
				Vcb->StreamObj->WriteAccess = TRUE;
				Vcb->StreamObj->DeleteAccess = TRUE;
			}
			else
			{
				Vcb->StreamObj->WriteAccess = TRUE;
				Vcb->StreamObj->DeleteAccess = TRUE;
			}
			Vcb->StreamObj->FsContext = (PVOID) Vcb;
			Vcb->StreamObj->FsContext2 = NULL;
			Vcb->StreamObj->Vpb = Vcb->Vpb;

			SetFlag(Vcb->StreamObj->Flags, FO_NO_INTERMEDIATE_BUFFERING);
		}
		else
		{
			_SEH2_LEAVE;
		}

		InitializeListHead(&Vcb->FcbList);

		InitializeListHead(&Vcb->NotifyList);

		FsRtlNotifyInitializeSync(&Vcb->NotifySync);

		NotifySyncInitialized = TRUE;

		Vcb->DeviceObject = VolumeDevice;

		Vcb->TargetDeviceObject = TargetDevice;

		Vcb->OpenFileHandleCount = 0;

		Vcb->ReferenceCount = 0;

		Vcb->ffs_super_block = FFSSb;

		Vcb->Header.NodeTypeCode = (USHORT) FFSVCB;
		Vcb->Header.NodeByteSize = sizeof(FFS_VCB);
		Vcb->Header.IsFastIoPossible = FastIoIsNotPossible;
		Vcb->Header.Resource = &(Vcb->MainResource);
		Vcb->Header.PagingIoResource = &(Vcb->PagingIoResource);

		Vcb->Vpb->SerialNumber = 'PS';

		DiskSize =
			Vcb->DiskGeometry.Cylinders.QuadPart *
			Vcb->DiskGeometry.TracksPerCylinder *
			Vcb->DiskGeometry.SectorsPerTrack *
			Vcb->DiskGeometry.BytesPerSector;

		IoctlSize = sizeof(PARTITION_INFORMATION);

		Status = FFSDiskIoControl(
					TargetDevice,
					IOCTL_DISK_GET_PARTITION_INFO,
					NULL,
					0,
					&Vcb->PartitionInformation,
					&IoctlSize);

		PartSize = Vcb->PartitionInformation.PartitionLength.QuadPart;

		if (!NT_SUCCESS(Status))
		{
			Vcb->PartitionInformation.StartingOffset.QuadPart = 0;

			Vcb->PartitionInformation.PartitionLength.QuadPart =
				DiskSize;

			PartSize = DiskSize;

			Status = STATUS_SUCCESS;
		}

		IoctlSize = sizeof(ULONG);
		Status = FFSDiskIoControl(
					TargetDevice,
					IOCTL_DISK_CHECK_VERIFY,
					NULL,
					0,
					&ChangeCount,
					&IoctlSize);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}

		Vcb->ChangeCount = ChangeCount;

		Vcb->Header.AllocationSize.QuadPart =
			Vcb->Header.FileSize.QuadPart = PartSize;

		Vcb->Header.ValidDataLength.QuadPart = 
			(LONGLONG)(0x7fffffffffffffff);
		/*
		Vcb->Header.AllocationSize.QuadPart = (LONGLONG)(ffs_super_block->s_blocks_count - ffs_super_block->s_free_blocks_count)
			* (FFS_MIN_BLOCK << ffs_super_block->s_log_block_size);
			Vcb->Header.FileSize.QuadPart = Vcb->Header.AllocationSize.QuadPart;
			Vcb->Header.ValidDataLength.QuadPart = Vcb->Header.AllocationSize.QuadPart;
		*/

		{
			CC_FILE_SIZES FileSizes;

			FileSizes.AllocationSize.QuadPart =
				FileSizes.FileSize.QuadPart =
				Vcb->Header.AllocationSize.QuadPart;

			FileSizes.ValidDataLength.QuadPart= (LONGLONG)(0x7fffffffffffffff);

			CcInitializeCacheMap(Vcb->StreamObj,
					&FileSizes,
					TRUE,
					&(FFSGlobal->CacheManagerNoOpCallbacks),
					Vcb);
		}
#if 0 //	LoadGroup XXX
		if (!FFSLoadGroup(Vcb))
		{
			Status = STATUS_UNSUCCESSFUL;
			_SEH2_LEAVE;
		}
#endif
		FsRtlInitializeLargeMcb(&(Vcb->DirtyMcbs), PagedPool);
		InitializeListHead(&(Vcb->McbList));

		if (IsFlagOn(FFSGlobal->Flags, FFS_CHECKING_BITMAP))
		{
			FFSCheckBitmapConsistency(IrpContext, Vcb);
		}

		{
			ULONG   dwData[FFS_BLOCK_TYPES] = {NDADDR, 1, 1, 1};
			ULONG   dwMeta[FFS_BLOCK_TYPES] = {0, 0, 0, 0};
			ULONG   i;

			if (FS_VERSION == 1)
			{
				for (i = 0; i < FFS_BLOCK_TYPES; i++)
				{
					dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

					if (i > 0)
					{
						dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
					}

					Vcb->dwData[i] = dwData[i];
					Vcb->dwMeta[i] = dwMeta[i];
				}
			}
			else
			{
				for (i = 0; i < FFS_BLOCK_TYPES; i++)
				{
					dwData[i] = dwData[i] << ((BLOCK_BITS - 3) * i);

					if (i > 0)
					{
						dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 3));
					}

					Vcb->dwData[i] = dwData[i];
					Vcb->dwMeta[i] = dwMeta[i];
				}
			}
		}

		SetFlag(Vcb->Flags, VCB_INITIALIZED);
	}

	_SEH2_FINALLY
	{
		if (!NT_SUCCESS(Status))
		{
			if (NotifySyncInitialized)
			{
				FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
			}

			if (Vcb->ffs_super_block)
			{
				ExFreePool(Vcb->ffs_super_block);
				Vcb->ffs_super_block = NULL;
			}

			if (VcbResourceInitialized)
			{
				ExDeleteResourceLite(&Vcb->MainResource);
				ExDeleteResourceLite(&Vcb->PagingIoResource);
			}
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
VOID
FFSFreeVcb(
	IN PFFS_VCB Vcb)
{
    PAGED_CODE();

	ASSERT(Vcb != NULL);

	ASSERT((Vcb->Identifier.Type == FFSVCB) &&
			(Vcb->Identifier.Size == sizeof(FFS_VCB)));

	FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

	if (Vcb->StreamObj)
	{
		if (IsFlagOn(Vcb->StreamObj->Flags, FO_FILE_MODIFIED))
		{
			IO_STATUS_BLOCK IoStatus;

			CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);
			ClearFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);
		}

		if (Vcb->StreamObj->PrivateCacheMap)
			FFSSyncUninitializeCacheMap(Vcb->StreamObj);

		ObDereferenceObject(Vcb->StreamObj);
		Vcb->StreamObj = NULL;
	}

#if DBG
	if (FsRtlNumberOfRunsInLargeMcb(&(Vcb->DirtyMcbs)) != 0)
	{
		LONGLONG            DirtyVba;
		LONGLONG            DirtyLba;
		LONGLONG            DirtyLength;
		int                 i;

		for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
		{
			FFSPrint((DBG_INFO, "DirtyVba = %I64xh\n", DirtyVba));
			FFSPrint((DBG_INFO, "DirtyLba = %I64xh\n", DirtyLba));
			FFSPrint((DBG_INFO, "DirtyLen = %I64xh\n\n", DirtyLength));
		}

		FFSBreakPoint();
	}
#endif

	FsRtlUninitializeLargeMcb(&(Vcb->DirtyMcbs));

	FFSFreeMcbTree(Vcb->McbTree);

	if (Vcb->ffs_super_block)
	{
		ExFreePool(Vcb->ffs_super_block);
		Vcb->ffs_super_block = NULL;
	}

	ExDeleteResourceLite(&Vcb->McbResource);

	ExDeleteResourceLite(&Vcb->PagingIoResource);

	ExDeleteResourceLite(&Vcb->MainResource);

	IoDeleteDevice(Vcb->DeviceObject);
}


VOID
FFSSyncUninitializeCacheMap(
	IN PFILE_OBJECT FileObject)
{
	CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
	NTSTATUS WaitStatus;
	LARGE_INTEGER FFSLargeZero = {0, 0};

    PAGED_CODE();

	KeInitializeEvent(&UninitializeCompleteEvent.Event,
			SynchronizationEvent,
			FALSE);

	CcUninitializeCacheMap(FileObject,
			&FFSLargeZero,
			&UninitializeCompleteEvent);

	WaitStatus = KeWaitForSingleObject(&UninitializeCompleteEvent.Event,
			Executive,
			KernelMode,
			FALSE,
			NULL);

	ASSERT (NT_SUCCESS(WaitStatus));
}
