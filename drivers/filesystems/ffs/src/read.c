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

NTSTATUS
FFSReadComplete(
	IN PFFS_IRP_CONTEXT IrpContext);

__drv_mustHoldCriticalRegion
NTSTATUS
FFSReadFile(
	IN PFFS_IRP_CONTEXT IrpContext);

__drv_mustHoldCriticalRegion
NTSTATUS
FFSReadVolume(
	IN PFFS_IRP_CONTEXT IrpContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSCompleteIrpContext)
#pragma alloc_text(PAGE, FFSCopyRead)
#pragma alloc_text(PAGE, FFSRead)
#pragma alloc_text(PAGE, FFSReadVolume)
#pragma alloc_text(PAGE, FFSv1ReadInode)
#pragma alloc_text(PAGE, FFSv2ReadInode)
#pragma alloc_text(PAGE, FFSReadFile)
#pragma alloc_text(PAGE, FFSReadComplete)

#endif


__drv_mustHoldCriticalRegion
NTSTATUS
FFSCompleteIrpContext(
	IN PFFS_IRP_CONTEXT IrpContext,
	IN NTSTATUS         Status)
{
	PIRP    Irp = NULL;
	BOOLEAN bPrint;

    PAGED_CODE();

	Irp = IrpContext->Irp;

	if (Irp != NULL)
	{
		if (NT_ERROR(Status))
		{
			Irp->IoStatus.Information = 0;
		}

		Irp->IoStatus.Status = Status;
		bPrint = !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

		FFSCompleteRequest(
				Irp, bPrint, (CCHAR)(NT_SUCCESS(Status)?
					IO_DISK_INCREMENT : IO_NO_INCREMENT));

		IrpContext->Irp = NULL;               
	}

	FFSFreeIrpContext(IrpContext);

	return Status;
}


BOOLEAN 
FFSCopyRead(
	IN PFILE_OBJECT       FileObject,
	IN PLARGE_INTEGER     FileOffset,
	IN ULONG              Length,
	IN BOOLEAN            Wait,
	OUT PVOID             Buffer,
	OUT PIO_STATUS_BLOCK  IoStatus)
{
	BOOLEAN bRet;

    PAGED_CODE();

	bRet =  CcCopyRead(FileObject,
				FileOffset,
				Length,
				Wait,
				Buffer,
				IoStatus);

	if (bRet)
	{
		ASSERT(NT_SUCCESS(IoStatus->Status));
	}

	return bRet;
/*
	   PVOID Bcb = NULL;
	   PVOID Buf = NULL;

	   if (CcMapData(FileObject,
			FileOffset,
			Length,
			Wait,
			&Bcb,
			&Buf))
	   {
			RtlCopyMemory(Buffer,  Buf, Length);
			IoStatus->Status = STATUS_SUCCESS;
			IoStatus->Information = Length;
			CcUnpinData(Bcb);
			return TRUE;
		   
	   }
	   else
	   {
			// IoStatus->Status = STATUS_
			return FALSE;
	   }
*/
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSReadVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS            Status = STATUS_UNSUCCESSFUL;

	PFFS_VCB            Vcb = 0;
	PFFS_CCB            Ccb;
	PFFS_FCBVCB         FcbOrVcb;
	PFILE_OBJECT        FileObject;

	PDEVICE_OBJECT      DeviceObject;

	PIRP                Irp;
	PIO_STACK_LOCATION  IoStackLocation;

	ULONG               Length;
	LARGE_INTEGER       ByteOffset;

	BOOLEAN             PagingIo;
	BOOLEAN             Nocache;
	BOOLEAN             SynchronousIo;
	BOOLEAN             MainResourceAcquired = FALSE;
	BOOLEAN             PagingIoResourceAcquired = FALSE;

	PUCHAR              Buffer = NULL;
	PFFS_BDL            ffs_bdl = NULL;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		FileObject = IrpContext->FileObject;

		FcbOrVcb = (PFFS_FCBVCB)FileObject->FsContext;

		ASSERT(FcbOrVcb);

		if (!(FcbOrVcb->Identifier.Type == FFSVCB && (PVOID)FcbOrVcb == (PVOID)Vcb))
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		Ccb = (PFFS_CCB)FileObject->FsContext2;

		Irp = IrpContext->Irp;

		Irp->IoStatus.Information = 0;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		Length = IoStackLocation->Parameters.Read.Length;
		ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

		PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
		Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
		SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

		if (Length == 0)
		{
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		if (Ccb != NULL)
		{
			if(!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO))
			{
				if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart)
				{
					Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
				}
			}

			{
				FFS_BDL BlockArray;

				if ((ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
						(Length & (SECTOR_SIZE - 1)))
				{
					Status = STATUS_INVALID_PARAMETER;
					_SEH2_LEAVE;
				}

				Status = FFSLockUserBuffer(
							IrpContext->Irp,
							Length,
							IoReadAccess);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				BlockArray.Irp = NULL;
				BlockArray.Lba = ByteOffset.QuadPart;;
				BlockArray.Offset = 0;
				BlockArray.Length = Length;

				Status = FFSReadWriteBlocks(IrpContext,
							Vcb,
							&BlockArray,
							Length,
							1,
							FALSE);
				Irp = IrpContext->Irp;

				_SEH2_LEAVE;
			}
		}

		if (Nocache &&
				((ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
				  (Length & (SECTOR_SIZE - 1))))
		{
			FFSBreakPoint();

			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
		{
			ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		if (!PagingIo)
		{
			if (!ExAcquireResourceSharedLite(
						&Vcb->MainResource,
						IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			MainResourceAcquired = TRUE;
		}
		else
		{
			if (!ExAcquireResourceSharedLite(
						&Vcb->PagingIoResource,
						IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			PagingIoResourceAcquired = TRUE;
		}


		if (ByteOffset.QuadPart >=
				Vcb->PartitionInformation.PartitionLength.QuadPart)
		{
			Irp->IoStatus.Information = 0;
			Status = STATUS_END_OF_FILE;
			_SEH2_LEAVE;
		}

		if (!Nocache)
		{
			if ((ByteOffset.QuadPart + Length) >
					Vcb->PartitionInformation.PartitionLength.QuadPart)
			{
				Length = (ULONG)(
						Vcb->PartitionInformation.PartitionLength.QuadPart -
						ByteOffset.QuadPart);
				Length &= ~((ULONG)SECTOR_SIZE - 1);
			}

			if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL))
			{
				CcMdlRead(
						Vcb->StreamObj,
						&ByteOffset,
						Length,
						&Irp->MdlAddress,
						&Irp->IoStatus);

				Status = Irp->IoStatus.Status;
			}
			else
			{
				Buffer = FFSGetUserBuffer(Irp);

				if (Buffer == NULL)
				{
					FFSBreakPoint();
					Status = STATUS_INVALID_USER_BUFFER;
					_SEH2_LEAVE;
				}

				if (!CcCopyRead(
							Vcb->StreamObj,
							(PLARGE_INTEGER)&ByteOffset,
							Length,
							IrpContext->IsSynchronous,
							Buffer,
							&Irp->IoStatus))
				{
					Status = STATUS_PENDING;
					_SEH2_LEAVE;
				}

				Status = Irp->IoStatus.Status;
			}
		}
		else
		{
			if ((ByteOffset.QuadPart + Length) >
					Vcb->PartitionInformation.PartitionLength.QuadPart
			)
			{
				Length = (ULONG)(
						Vcb->PartitionInformation.PartitionLength.QuadPart -
						ByteOffset.QuadPart);

				Length &= ~((ULONG)SECTOR_SIZE - 1);
			}

			Status = FFSLockUserBuffer(
					IrpContext->Irp,
					Length,
					IoWriteAccess);

			if (!NT_SUCCESS(Status))
			{
				_SEH2_LEAVE;
			}

#if DBG
			Buffer = FFSGetUserBuffer(Irp);
#endif
			ffs_bdl = ExAllocatePoolWithTag(PagedPool, sizeof(FFS_BDL), FFS_POOL_TAG);

			if (!ffs_bdl)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			ffs_bdl->Irp = NULL;
			ffs_bdl->Lba = ByteOffset.QuadPart;
			ffs_bdl->Length = Length;
			ffs_bdl->Offset = 0;

			Status = FFSReadWriteBlocks(IrpContext,
						Vcb,
						ffs_bdl,
						Length,
						1,
						FALSE);

			if (NT_SUCCESS(Status))
			{
				Irp->IoStatus.Information = Length;
			}

			Irp = IrpContext->Irp;

			if (!Irp)
				_SEH2_LEAVE;
		}
	}

	_SEH2_FINALLY
	{
		if (PagingIoResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->PagingIoResource,
					ExGetCurrentResourceThread());
		}

		if (MainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (ffs_bdl)
			ExFreePool(ffs_bdl);

		if (!IrpContext->ExceptionInProgress)
		{
			if (IrpContext->Irp)
			{
				if (Status == STATUS_PENDING &&
						!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED))
				{
					Status = FFSLockUserBuffer(
								IrpContext->Irp,
								Length,
								IoWriteAccess);

					if (NT_SUCCESS(Status))
					{
						Status = FFSQueueRequest(IrpContext);
					}
					else
					{
						FFSCompleteIrpContext(IrpContext, Status);
					}
				}
				else
				{
					if (NT_SUCCESS(Status))
					{
						if (SynchronousIo && !PagingIo)
						{
							IrpContext->FileObject->CurrentByteOffset.QuadPart =
								ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
						}

						if (!PagingIo)
						{
							IrpContext->FileObject->Flags &= ~FO_FILE_FAST_IO_READ;
						}
					}

					FFSCompleteIrpContext(IrpContext, Status);
				}
			}
			else
			{
				FFSFreeIrpContext(IrpContext);
			}
		}
	} _SEH2_END;

	return Status;
}


NTSTATUS
FFSv1ReadInode(
	IN PFFS_IRP_CONTEXT     IrpContext,
	IN PFFS_VCB             Vcb,
	IN PFFSv1_INODE         dinode1,
	IN ULONGLONG            offset,
	IN PVOID                Buffer,
	IN ULONG                size,
	OUT PULONG              dwRet)
{
	PFFS_BDL    ffs_bdl = NULL;
	ULONG       blocks, i;
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK IoStatus;

	ULONG       Totalblocks;
	LONGLONG    AllocSize;

    PAGED_CODE();

	if (dwRet)
	{
		*dwRet = 0;
	}

	Totalblocks = (dinode1->di_blocks);
	AllocSize = ((LONGLONG)(FFSDataBlocks(Vcb, Totalblocks)) << BLOCK_BITS);

	if ((LONGLONG)offset >= AllocSize)
	{
		FFSPrint((DBG_ERROR, "FFSv1ReadInode: beyond the file range.\n"));
		return STATUS_SUCCESS;
	}

	if ((LONGLONG)offset + size > AllocSize)
	{
		size = (ULONG)(AllocSize - offset);
	}

	blocks = FFSv1BuildBDL(IrpContext, Vcb, dinode1, offset, size, &ffs_bdl);

	if (blocks <= 0)
	{
		FFSBreakPoint();
		goto errorout;
	}

	if (IrpContext)
	{
		// assume offset is aligned.
		Status = FFSReadWriteBlocks(IrpContext, Vcb, ffs_bdl, size, blocks, FALSE);
	}
	else
	{
		for(i = 0; i < blocks; i++)
		{
			IoStatus.Information = 0;

#if DBG
			KdPrint(("FFSv1ReadInode() i : %d, Lba : %x, Length : %x, Offset : %x\n", 
				i, ffs_bdl[i].Lba, ffs_bdl[i].Length, ffs_bdl[i].Offset));
#endif

			FFSCopyRead(
					Vcb->StreamObj, 
					(PLARGE_INTEGER)(&(ffs_bdl[i].Lba)), 
					ffs_bdl[i].Length,
					PIN_WAIT,
					(PVOID)((PUCHAR)Buffer + ffs_bdl[i].Offset), 
					&IoStatus);

			Status = IoStatus.Status;
		}
	}

errorout:

	if (ffs_bdl)
		ExFreePool(ffs_bdl);

	if (NT_SUCCESS(Status))
	{
		if (dwRet) *dwRet = size;
	}

	return Status;
}


NTSTATUS
FFSv2ReadInode(
	IN PFFS_IRP_CONTEXT     IrpContext,
	IN PFFS_VCB             Vcb,
	IN PFFSv2_INODE         dinode2,
	IN ULONGLONG            offset,
	IN PVOID                Buffer,
	IN ULONG                size,
	OUT PULONG              dwRet)
{
	PFFS_BDL    ffs_bdl = NULL;
	ULONG       blocks, i;
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK IoStatus;

	ULONG       Totalblocks;
	LONGLONG    AllocSize;

    PAGED_CODE();

	if (dwRet)
	{
		*dwRet = 0;
	}

	Totalblocks = (ULONG)(dinode2->di_blocks);
	AllocSize = ((LONGLONG)(FFSDataBlocks(Vcb, Totalblocks)) << BLOCK_BITS);

	if ((LONGLONG)offset >= AllocSize)
	{
		FFSPrint((DBG_ERROR, "FFSv2ReadInode: beyond the file range.\n"));
		return STATUS_SUCCESS;
	}

	if ((LONGLONG)offset + size > AllocSize)
	{
		size = (ULONG)(AllocSize - offset);
	}

	blocks = FFSv2BuildBDL(IrpContext, Vcb, dinode2, offset, size, &ffs_bdl);

	if (blocks <= 0)
	{
		FFSBreakPoint();
		goto errorout;
	}

	if (IrpContext)
	{
		// assume offset is aligned.
		Status = FFSReadWriteBlocks(IrpContext, Vcb, ffs_bdl, size, blocks, FALSE);
	}
	else
	{
		for(i = 0; i < blocks; i++)
		{
			IoStatus.Information = 0;

#if 0
			KdPrint(("FFSv2ReadInode() i : %d, Lba : %x, Length : %x, Offset : %x\n", 
				i, ffs_bdl[i].Lba, ffs_bdl[i].Length, ffs_bdl[i].Offset));
#endif

			FFSCopyRead(
					Vcb->StreamObj, 
					(PLARGE_INTEGER)(&(ffs_bdl[i].Lba)), 
					ffs_bdl[i].Length,
					PIN_WAIT,
					(PVOID)((PUCHAR)Buffer + ffs_bdl[i].Offset), 
					&IoStatus);

			Status = IoStatus.Status;
		}
	}

errorout:

	if (ffs_bdl)
		ExFreePool(ffs_bdl);

	if (NT_SUCCESS(Status))
	{
		if (dwRet) *dwRet = size;
	}

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSReadFile(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS            Status = STATUS_UNSUCCESSFUL;

	PFFS_VCB            Vcb;
	PFFS_FCB            Fcb = 0;
	PFFS_CCB            Ccb;
	PFILE_OBJECT        FileObject;
	PFILE_OBJECT        CacheObject;

	PDEVICE_OBJECT      DeviceObject;

	PIRP                Irp;
	PIO_STACK_LOCATION  IoStackLocation;

	ULONG               Length;
	ULONG               ReturnedLength;
	LARGE_INTEGER       ByteOffset;

	BOOLEAN             PagingIo;
	BOOLEAN             Nocache;
	BOOLEAN             SynchronousIo;
	BOOLEAN             MainResourceAcquired = FALSE;
	BOOLEAN             PagingIoResourceAcquired = FALSE;

	PUCHAR              Buffer;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		FileObject = IrpContext->FileObject;

		Fcb = (PFFS_FCB)FileObject->FsContext;

		ASSERT(Fcb);

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		Ccb = (PFFS_CCB)FileObject->FsContext2;

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		Length = IoStackLocation->Parameters.Read.Length;
		ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

		PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
		Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
		SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

#if 0
/*
		if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
		{
			Status = STATUS_FILE_DELETED;
			_SEH2_LEAVE;
		}

		if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING))
		{
			Status = STATUS_DELETE_PENDING;
			_SEH2_LEAVE;
		}
*/
#endif

		if (Length == 0)
		{
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		if (Nocache &&
				(ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
				 Length & (SECTOR_SIZE - 1)))
		{
			Status = STATUS_INVALID_PARAMETER;
			FFSBreakPoint();
			_SEH2_LEAVE;
		}

		if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
		{
			ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
			Status = STATUS_PENDING;
			FFSBreakPoint();
			_SEH2_LEAVE;
		}

		if (!PagingIo)
		{
			if (!ExAcquireResourceSharedLite(
						&Fcb->MainResource,
						IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			MainResourceAcquired = TRUE;

			if (!FsRtlCheckLockForReadAccess(
						&Fcb->FileLockAnchor,
						Irp))
			{
				Status = STATUS_FILE_LOCK_CONFLICT;
				_SEH2_LEAVE;
			}
		}
		else
		{
			if (!ExAcquireResourceSharedLite(
						&Fcb->PagingIoResource,
						IrpContext->IsSynchronous))
			{
				Status = STATUS_PENDING;
				_SEH2_LEAVE;
			}

			PagingIoResourceAcquired = TRUE;
		}

		if (!Nocache)
		{
			if ((ByteOffset.QuadPart + (LONGLONG)Length) >
					Fcb->Header.FileSize.QuadPart)
			{
				if (ByteOffset.QuadPart >= (Fcb->Header.FileSize.QuadPart))
				{
					Irp->IoStatus.Information = 0;
					Status = STATUS_END_OF_FILE;
					_SEH2_LEAVE;
				}

				Length =
					(ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);

			}

			ReturnedLength = Length;

			if (IsDirectory(Fcb))
			{
				_SEH2_LEAVE;
			}

			{
				if (FileObject->PrivateCacheMap == NULL)
				{
					CcInitializeCacheMap(
							FileObject,
							(PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
							FALSE,
							&FFSGlobal->CacheManagerCallbacks,
							Fcb);
				}

				CacheObject = FileObject;
			}

			if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL))
			{
				CcMdlRead(
						CacheObject,
						(&ByteOffset),
						Length,
						&Irp->MdlAddress,
						&Irp->IoStatus);

				Status = Irp->IoStatus.Status;
			}
			else
			{
				Buffer = FFSGetUserBuffer(Irp);

				if (Buffer == NULL)
				{
					Status = STATUS_INVALID_USER_BUFFER;
					FFSBreakPoint();
					_SEH2_LEAVE;
				}

				if (!CcCopyRead(
							CacheObject,
							(PLARGE_INTEGER)&ByteOffset,
							Length,
							IrpContext->IsSynchronous,
							Buffer,
							&Irp->IoStatus))
				{
					Status = STATUS_PENDING;
					FFSBreakPoint();
					_SEH2_LEAVE;
				}

				Status = Irp->IoStatus.Status;
			}
		}
		else
		{
			/* ByteOffset과 AllocationSize 모두 0이 아닐때 */
			if (ByteOffset.QuadPart && Fcb->Header.AllocationSize.QuadPart)
			{
				if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->Header.AllocationSize.QuadPart)
				{

					if (ByteOffset.QuadPart >= Fcb->Header.AllocationSize.QuadPart)
					{
						Irp->IoStatus.Information = 0;
						Status = STATUS_END_OF_FILE;
						FFSBreakPoint();
						_SEH2_LEAVE;
					}

					Length =
						(ULONG)(Fcb->Header.AllocationSize.QuadPart- ByteOffset.QuadPart);
				}
			}

			ReturnedLength = Length;

			Status = FFSLockUserBuffer(
					IrpContext->Irp,
					Length,
					IoWriteAccess);

			if (!NT_SUCCESS(Status))
			{
				_SEH2_LEAVE;
			}

			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = Length;

			if (FS_VERSION == 1)
			{
				Status = 
					FFSv1ReadInode(
							IrpContext,
							Vcb,
							Fcb->dinode1,
							ByteOffset.QuadPart,
							NULL,
							Length,
							&ReturnedLength);
			}
			else
			{
				Status = 
					FFSv2ReadInode(
							IrpContext,
							Vcb,
							Fcb->dinode2,
							ByteOffset.QuadPart,
							NULL,
							Length,
							&ReturnedLength);
			}

			Irp = IrpContext->Irp;

		}
	}

	_SEH2_FINALLY
	{
		if (PagingIoResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->PagingIoResource,
					ExGetCurrentResourceThread());
		}

		if (MainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			if (IrpContext->Irp)
			{
				if (Status == STATUS_PENDING)
				{
					Status = FFSLockUserBuffer(
							IrpContext->Irp,
							Length,
							IoWriteAccess);

					if (NT_SUCCESS(Status))
					{
						Status = FFSQueueRequest(IrpContext);
					}
					else
					{
						FFSCompleteIrpContext(IrpContext, Status);
					}
				}
				else
				{
					if (NT_SUCCESS(Status))
					{
						if (SynchronousIo && !PagingIo)
						{
							IrpContext->FileObject->CurrentByteOffset.QuadPart =
								ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
						}

						if (!PagingIo)
						{
							IrpContext->FileObject->Flags &= ~FO_FILE_FAST_IO_READ;
						}
					}

					FFSCompleteIrpContext(IrpContext, Status);
				}
			}
			else
			{
				FFSFreeIrpContext(IrpContext);
			}
		}
	} _SEH2_END;

	return Status;

}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSReadComplete(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;
	PFILE_OBJECT    FileObject;
	PIRP            Irp;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		FileObject = IrpContext->FileObject;

		Irp = IrpContext->Irp;

		CcMdlReadComplete(FileObject, Irp->MdlAddress);

		Irp->MdlAddress = NULL;

		Status = STATUS_SUCCESS;
	}

	_SEH2_FINALLY
	{
		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSRead(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS            Status;
	PFFS_VCB            Vcb;
	PFFS_FCBVCB         FcbOrVcb;
	PDEVICE_OBJECT      DeviceObject;
	PFILE_OBJECT        FileObject;
	BOOLEAN             bCompleteRequest;

    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));
	_SEH2_TRY
	{

		if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE))
		{
			Status = FFSReadComplete(IrpContext);
			bCompleteRequest = FALSE;
		}
		else
		{
			DeviceObject = IrpContext->DeviceObject;

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				bCompleteRequest = TRUE;
				_SEH2_LEAVE;
			}

			Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

			if (Vcb->Identifier.Type != FFSVCB ||
					Vcb->Identifier.Size != sizeof(FFS_VCB))
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				bCompleteRequest = TRUE;

				_SEH2_LEAVE;
			}

			if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
			{
				Status = STATUS_TOO_LATE;
				bCompleteRequest = TRUE;
				_SEH2_LEAVE;
			}

			FileObject = IrpContext->FileObject;

			FcbOrVcb = (PFFS_FCBVCB)FileObject->FsContext;

			if (FcbOrVcb->Identifier.Type == FFSVCB)
			{
				Status = FFSReadVolume(IrpContext);
				bCompleteRequest = FALSE;
			}
			else if (FcbOrVcb->Identifier.Type == FFSFCB)
			{
				Status = FFSReadFile(IrpContext);
				bCompleteRequest = FALSE;
			}
			else
			{
				FFSPrint((DBG_ERROR, "FFSRead: INVALID PARAMETER ... \n"));
				FFSBreakPoint();

				Status = STATUS_INVALID_PARAMETER;
				bCompleteRequest = TRUE;
			}
		}
	}

	_SEH2_FINALLY
	{
		if (bCompleteRequest)
		{
			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}
