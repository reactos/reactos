/* 
 * FFS File System Driver for Windows
 *
 * fsctl.c
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

BOOLEAN
FFSIsMediaWriteProtected (
	IN PFFS_IRP_CONTEXT  IrpContext,
	IN PDEVICE_OBJECT     TargetDevice);

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, FFSSetVpbFlag)
//#pragma alloc_text(PAGE, FFSClearVpbFlag)
#pragma alloc_text(PAGE, FFSGetPartition)
#pragma alloc_text(PAGE, FFSLoadDiskLabel)
#pragma alloc_text(PAGE, FFSIsHandleCountZero)
#pragma alloc_text(PAGE, FFSLockVcb)
#pragma alloc_text(PAGE, FFSLockVolume)
#pragma alloc_text(PAGE, FFSUnlockVcb)
#pragma alloc_text(PAGE, FFSUnlockVolume)
#pragma alloc_text(PAGE, FFSAllowExtendedDasdIo)
#pragma alloc_text(PAGE, FFSUserFsRequest)
#pragma alloc_text(PAGE, FFSIsMediaWriteProtected)
#pragma alloc_text(PAGE, FFSMountVolume)
#pragma alloc_text(PAGE, FFSCheckDismount)
#pragma alloc_text(PAGE, FFSPurgeVolume)
#pragma alloc_text(PAGE, FFSPurgeFile)
#pragma alloc_text(PAGE, FFSDismountVolume)
#pragma alloc_text(PAGE, FFSIsVolumeMounted)
#pragma alloc_text(PAGE, FFSVerifyVolume)
#pragma alloc_text(PAGE, FFSFileSystemControl)
#endif


VOID
FFSSetVpbFlag(
	IN PVPB     Vpb,
	IN USHORT   Flag)
{
	KIRQL OldIrql;

	IoAcquireVpbSpinLock(&OldIrql);

	Vpb->Flags |= Flag;

	IoReleaseVpbSpinLock(OldIrql);
}


VOID
FFSClearVpbFlag(
	IN PVPB     Vpb,
	IN USHORT   Flag)
{
	KIRQL OldIrql;

	IoAcquireVpbSpinLock(&OldIrql);

	Vpb->Flags &= ~Flag;

	IoReleaseVpbSpinLock(OldIrql);
}


NTSTATUS
FFSGetPartition(
	IN PDEVICE_OBJECT DeviceObject,
	OUT ULONGLONG     *StartOffset)
{
    CHAR                  Buffer[2048];
    PIRP                  Irp;
    IO_STATUS_BLOCK       IoStatus;
    KEVENT                Event;
    NTSTATUS              Status;
    PARTITION_INFORMATION *PartInfo;

    PAGED_CODE();

    if (IsFlagOn(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE))
    {
        *StartOffset = 0;
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
			IOCTL_DISK_GET_PARTITION_INFO,
			DeviceObject,
			NULL,
			0,
			Buffer,
			2048,
			FALSE,
			&Event,
			&IoStatus);

    if (!Irp)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

    Status = IoCallDriver(DeviceObject, Irp);
    if (!NT_SUCCESS(Status))
	{
		return Status;
	}

    Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    if (!NT_SUCCESS(Status))
	{
		return Status;
	}

    PartInfo = (PARTITION_INFORMATION *)Buffer;
    *StartOffset = PartInfo->StartingOffset.QuadPart;

    return Status;
}


NTSTATUS
FFSLoadDiskLabel(
	PDEVICE_OBJECT  DeviceObject,
	IN PFFS_VCB     Vcb)
{
	NTSTATUS         Status;
	PFFS_SUPER_BLOCK FFSSb;
	PDISKLABEL       Disklabel;
	int              i;
	int              RootFS_VERSION;
	ULONGLONG        StartOffset = 0;
	ULONGLONG        FSOffset = 0;
	ULONGLONG        FSRootOffset = 0;

    PAGED_CODE();

	Disklabel = (PDISKLABEL)ExAllocatePoolWithTag(PagedPool, sizeof(DISKLABEL), FFS_POOL_TAG);
	
	FFSReadDisk(Vcb, (LABELSECTOR * SECTOR_SIZE + LABELOFFSET), sizeof(DISKLABEL), (PVOID)Disklabel, FALSE);

	if (Disklabel->d_magic == DISKMAGIC)
	{
		FFSPrint((DBG_INFO, "FFSLoadDiskLabel() Disklabel magic ok\n"));

		Status = STATUS_SUCCESS;
	}
	else
	{
			FFSPrint((DBG_INFO, "FFSLoadDiskLabel() No BSD disklabel found, trying to find BSD file system on \"normal\" partition.\n"));

			if ((FFSSb = FFSLoadSuper(Vcb, FALSE, FSOffset + SBLOCK_UFS1)) &&
				(FFSSb->fs_magic == FS_UFS1_MAGIC))
			{
				FFSPrint((DBG_VITAL, "FFSLoadDiskLabel() \"normal\" partition of FFSv1 file system is found.\n"));
/*
				if ((FFSSb->fs_fsmnt[0] == '/') && (FFSSb->fs_fsmnt[1] == '\0'))
				{
					FFSGlobal->RootPartition = i;
					FSRootOffset = FSOffset;
					RootFS_VERSION = 1;
				}
*/
				FS_VERSION = 1;
                Vcb->FSOffset[0] = 0;
                Vcb->PartitionNumber = 0;
				Vcb->ffs_super_block = FFSSb;
#ifdef __REACTOS__
                ExFreePoolWithTag(Disklabel, FFS_POOL_TAG);
#endif
				Status = STATUS_SUCCESS;
                return Status;
			}
			else if ((FFSSb = FFSLoadSuper(Vcb, FALSE, FSOffset + SBLOCK_UFS2)) &&
					(FFSSb->fs_magic == FS_UFS2_MAGIC))
			{
				FFSPrint((DBG_VITAL, "FFSLoadDiskLabel() \"normal\" partition of FFSv2 file system is found.\n"));
/*
				if ((FFSSb->fs_fsmnt[0] == '/') && (FFSSb->fs_fsmnt[1] == '\0'))
				{
					FFSGlobal->RootPartition = i;
					FSRootOffset = FSOffset;
					RootFS_VERSION = 2;
				}
*/
				FS_VERSION = 2;
                Vcb->FSOffset[0] = 0;
                Vcb->PartitionNumber = 0;
				Vcb->ffs_super_block = FFSSb;
#ifdef __REACTOS__
                ExFreePoolWithTag(Disklabel, FFS_POOL_TAG);
#endif
				Status = STATUS_SUCCESS;
                return Status;
			}
            else
            {
                FFSPrint((DBG_INFO, "FFSLoadDiskLabel() No BSD file system was found on the \"normal\" partition.\n"));
#ifdef __REACTOS__
                ExFreePoolWithTag(Disklabel, FFS_POOL_TAG);
#endif
                Status = STATUS_UNRECOGNIZED_VOLUME;
                return Status;
            }
	}

	Status = FFSGetPartition(DeviceObject, &StartOffset);
	if (!NT_SUCCESS(Status))
	{
		FFSPrint((DBG_ERROR, "FFSLoadDiskLabel() Slice info failed, Status %u\n", Status));
#ifdef __REACTOS__
        ExFreePoolWithTag(Disklabel, FFS_POOL_TAG);
#endif
		return Status;
	}

    Vcb->PartitionNumber = FFSGlobal->PartitionNumber;

	FFSPrint((DBG_INFO, "FFSLoadDiskLabel() Selected BSD Label : %d, StartOffset : %x\n", Vcb->PartitionNumber, StartOffset));

	for (i = 0; i < MAXPARTITIONS; i++)
	{
		if (Disklabel->d_partitions[i].p_fstype == FS_BSDFFS)
		{
			/* Important */
			FSOffset = Disklabel->d_partitions[i].p_offset;
			FSOffset = FSOffset * SECTOR_SIZE;
			//FSOffset = FSOffset - StartOffset;
			Vcb->FSOffset[i] = FSOffset;
			/* Important */

			FFSPrint((DBG_INFO, "FFSLoadDiskLabel() Label %d, FS_BSDFFS, %x\n", i, Vcb->FSOffset[i]));

			if ((FFSSb = FFSLoadSuper(Vcb, FALSE, FSOffset + SBLOCK_UFS1)) &&
				(FFSSb->fs_magic == FS_UFS1_MAGIC))
			{
				FFSPrint((DBG_VITAL, "FFSLoadDiskLabel() Label %d of FFSv1 file system is found.\n", i));

				if ((FFSSb->fs_fsmnt[0] == '/') && (FFSSb->fs_fsmnt[1] == '\0'))
				{
					Vcb->RootPartition = i;
					FSRootOffset = FSOffset;
					RootFS_VERSION = 1;
				}

				FS_VERSION = 1;
				
				if (i == (int)Vcb->PartitionNumber)
				{
					Vcb->ffs_super_block = FFSSb;
				}

				Status = STATUS_SUCCESS;
			}
			else if ((FFSSb = FFSLoadSuper(Vcb, FALSE, FSOffset + SBLOCK_UFS2)) &&
					(FFSSb->fs_magic == FS_UFS2_MAGIC))
			{
				FFSPrint((DBG_VITAL, "FFSLoadDiskLabel() Label %d of FFSv2 file system is found.\n", i));

				if ((FFSSb->fs_fsmnt[0] == '/') && (FFSSb->fs_fsmnt[1] == '\0'))
				{
					Vcb->RootPartition = i;
					FSRootOffset = FSOffset;
					RootFS_VERSION = 2;
				}

				FS_VERSION = 2;

				if (i == (int)Vcb->PartitionNumber)
				{
					Vcb->ffs_super_block = FFSSb;
				}

				Status = STATUS_SUCCESS;
			}
		}
#if 0
		else if (i == (int)FFSGlobal->PartitionNumber)
		{
			/* 선택된 BSD 파티션이 디스크에 없을 경우 Root 파티션으로 대체 */
			if (RootFS_VERSION == 1)
			{
				FFSSb = FFSLoadSuper(Vcb, FALSE, FSRootOffset + SBLOCK_UFS1);
				Vcb->ffs_super_block = FFSSb;
				FFSGlobal->PartitionNumber = FFSGlobal->RootPartition;
			}
			else
			{
				FFSSb = FFSLoadSuper(Vcb, FALSE, FSRootOffset + SBLOCK_UFS2);
				Vcb->ffs_super_block = FFSSb;
				FFSGlobal->PartitionNumber = FFSGlobal->RootPartition;
			}
		}
#endif
	}

	if (Vcb->ffs_super_block == NULL)
		Status = STATUS_UNRECOGNIZED_VOLUME;

#ifdef __REACTOS__
    ExFreePoolWithTag(Disklabel, FFS_POOL_TAG);
#endif

	return Status;
}


BOOLEAN
FFSIsHandleCountZero(
	IN PFFS_VCB Vcb)
{
	PFFS_FCB   Fcb;
	PLIST_ENTRY List;

    PAGED_CODE();

	for(List = Vcb->FcbList.Flink;
			List != &Vcb->FcbList;
			List = List->Flink)
	{
		Fcb = CONTAINING_RECORD(List, FFS_FCB, Next);

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		FFSPrint((DBG_INFO, "FFSIsHandleCountZero: Inode:%xh File:%S OpenHandleCount=%xh\n",
					Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer, Fcb->OpenHandleCount));

		if (Fcb->OpenHandleCount)
		{
			return FALSE;
		}
	}

	return TRUE;
}


NTSTATUS
FFSLockVcb(
	IN PFFS_VCB     Vcb,
	IN PFILE_OBJECT FileObject)
{
	NTSTATUS           Status;

    PAGED_CODE();

	_SEH2_TRY
	{
		if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
		{
			FFSPrint((DBG_INFO, "FFSLockVolume: Volume is already locked.\n"));

			Status = STATUS_ACCESS_DENIED;

			_SEH2_LEAVE;
		}

		if (Vcb->OpenFileHandleCount > (ULONG)(FileObject ? 1 : 0))
		{
			FFSPrint((DBG_INFO, "FFSLockVcb: There are still opened files.\n"));

			Status = STATUS_ACCESS_DENIED;

			_SEH2_LEAVE;
		}

		if (!FFSIsHandleCountZero(Vcb))
		{
			FFSPrint((DBG_INFO, "FFSLockVcb: Thare are still opened files.\n"));

			Status = STATUS_ACCESS_DENIED;

			_SEH2_LEAVE;
		}

		SetFlag(Vcb->Flags, VCB_VOLUME_LOCKED);

		FFSSetVpbFlag(Vcb->Vpb, VPB_LOCKED);

		Vcb->LockFile = FileObject;

		FFSPrint((DBG_INFO, "FFSLockVcb: Volume locked.\n"));

		Status = STATUS_SUCCESS;
	}

	_SEH2_FINALLY
	{
		// Nothing
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSLockVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIO_STACK_LOCATION IrpSp;
	PDEVICE_OBJECT     DeviceObject;
	PFFS_VCB           Vcb = 0;
	NTSTATUS           Status;
	BOOLEAN VcbResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		Status = STATUS_UNSUCCESSFUL;

		//
		// This request is not allowed on the main device object
		//
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

#if (_WIN32_WINNT >= 0x0500)
		CcWaitForCurrentLazyWriterActivity();
#endif
		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,
				TRUE);

		VcbResourceAcquired = TRUE;

		Status = FFSLockVcb(Vcb, IrpSp->FileObject);        
	}

	_SEH2_FINALLY
	{
		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


NTSTATUS
FFSUnlockVcb(
	IN PFFS_VCB     Vcb,
	IN PFILE_OBJECT FileObject)
{
	NTSTATUS        Status;

    PAGED_CODE();

	_SEH2_TRY
	{
		if (FileObject && FileObject->FsContext != Vcb)
		{
			Status = STATUS_NOT_LOCKED;
			_SEH2_LEAVE;
		}

		if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
		{
			FFSPrint((DBG_ERROR, ": FFSUnlockVcb: Volume is not locked.\n"));
			Status = STATUS_NOT_LOCKED;
			_SEH2_LEAVE;
		}

		if (Vcb->LockFile == FileObject)
		{
			ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);

			FFSClearVpbFlag(Vcb->Vpb, VPB_LOCKED);

			FFSPrint((DBG_INFO, "FFSUnlockVcb: Volume unlocked.\n"));

			Status = STATUS_SUCCESS;
		}
		else
		{
			Status = STATUS_NOT_LOCKED;
		}
	}

	_SEH2_FINALLY
	{
		// Nothing
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSUnlockVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIO_STACK_LOCATION IrpSp;
	PDEVICE_OBJECT     DeviceObject;
	NTSTATUS           Status;
	PFFS_VCB           Vcb = 0;
	BOOLEAN            VcbResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		//
		// This request is not allowed on the main device object
		//
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,
				TRUE);

		VcbResourceAcquired = TRUE;

		Status = FFSUnlockVcb(Vcb, IrpSp->FileObject);
	}
	_SEH2_FINALLY
	{
		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSInvalidateVolumes(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS            Status;
	PIRP                Irp;
	PIO_STACK_LOCATION  IrpSp;

	HANDLE              Handle;

	PLIST_ENTRY         ListEntry;

	PFILE_OBJECT        FileObject;
	PDEVICE_OBJECT      DeviceObject;
	BOOLEAN             GlobalResourceAcquired = FALSE;

	LUID Privilege = {SE_TCB_PRIVILEGE, 0};

	_SEH2_TRY
	{
		Irp   = IrpContext->Irp;
		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		if (!SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode))
		{
			Status = STATUS_PRIVILEGE_NOT_HELD;
			_SEH2_LEAVE;
		}

		if (
#if !defined(_GNU_NTIFS_) || defined(__REACTOS__)
				IrpSp->Parameters.FileSystemControl.InputBufferLength
#else
				((PEXTENDED_IO_STACK_LOCATION)(IrpSp))->
				Parameters.FileSystemControl.InputBufferLength
#endif
				!= sizeof(HANDLE))
		{
			Status = STATUS_INVALID_PARAMETER;

			_SEH2_LEAVE;
		}

		Handle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;

		Status = ObReferenceObjectByHandle(Handle,
				0,
				*IoFileObjectType,
				KernelMode,
				(void **)&FileObject,
				NULL);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}
		else
		{
			ObDereferenceObject(FileObject);
			DeviceObject = FileObject->DeviceObject;
		}

		FFSPrint((DBG_INFO, "FFSInvalidateVolumes: FileObject=%xh ...\n", FileObject));

		ExAcquireResourceExclusiveLite(
				&FFSGlobal->Resource,
				TRUE);

		GlobalResourceAcquired = TRUE;

		ListEntry = FFSGlobal->VcbList.Flink;

		while (ListEntry != &FFSGlobal->VcbList)
		{
			PFFS_VCB Vcb;

			Vcb = CONTAINING_RECORD(ListEntry, FFS_VCB, Next);

			ListEntry = ListEntry->Flink;

			FFSPrint((DBG_INFO, "FFSInvalidateVolumes: Vcb=%xh Vcb->Vpb=%xh...\n", Vcb, Vcb->Vpb));
			if (Vcb->Vpb && (Vcb->Vpb->RealDevice == DeviceObject))
			{
				ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
				FFSPrint((DBG_INFO, "FFSInvalidateVolumes: FFSPurgeVolume...\n"));
				FFSPurgeVolume(Vcb, FALSE);
				ClearFlag(Vcb->Flags, VCB_MOUNTED);
				ExReleaseResourceLite(&Vcb->MainResource);

				//
				// Vcb is still attached on the list ......
				//

				if (ListEntry->Blink == &Vcb->Next)
				{
					FFSPrint((DBG_INFO, "FFSInvalidateVolumes: FFSCheckDismount...\n"));
					FFSCheckDismount(IrpContext, Vcb, FALSE);
				}
			}
		}
	}

	_SEH2_FINALLY
	{
		if (GlobalResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
					ExGetCurrentResourceThread());
		}

		FFSCompleteIrpContext(IrpContext, Status);
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSAllowExtendedDasdIo(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIO_STACK_LOCATION IrpSp;
	PFFS_VCB Vcb;
	PFFS_CCB Ccb;

    PAGED_CODE();

	IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

	Vcb = (PFFS_VCB)IrpSp->FileObject->FsContext;
	Ccb = (PFFS_CCB)IrpSp->FileObject->FsContext2;

	ASSERT(Vcb != NULL);

	ASSERT((Vcb->Identifier.Type == FFSVCB) &&
			(Vcb->Identifier.Size == sizeof(FFS_VCB)));

	ASSERT(IsMounted(Vcb));

	if (Ccb)
	{
		SetFlag(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO);

		FFSCompleteIrpContext(IrpContext, STATUS_SUCCESS);

		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_INVALID_PARAMETER;
	}
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSUserFsRequest(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIRP                Irp;
	PIO_STACK_LOCATION  IoStackLocation;
	ULONG               FsControlCode;
	NTSTATUS            Status;

    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	Irp = IrpContext->Irp;

	IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

#if !defined(_GNU_NTIFS_) || defined(__REACTOS__)
	FsControlCode =
		IoStackLocation->Parameters.FileSystemControl.FsControlCode;
#else
	FsControlCode = ((PEXTENDED_IO_STACK_LOCATION)
			IoStackLocation)->Parameters.FileSystemControl.FsControlCode;
#endif

	switch (FsControlCode)
	{
		case FSCTL_LOCK_VOLUME:
			Status = FFSLockVolume(IrpContext);
			break;

		case FSCTL_UNLOCK_VOLUME:
			Status = FFSUnlockVolume(IrpContext);
			break;

		case FSCTL_DISMOUNT_VOLUME:
			Status = FFSDismountVolume(IrpContext);
			break;

		case FSCTL_IS_VOLUME_MOUNTED:
			Status = FFSIsVolumeMounted(IrpContext);
			break;

		case FSCTL_INVALIDATE_VOLUMES:
			Status = FFSInvalidateVolumes(IrpContext);
			break;

#if (_WIN32_WINNT >= 0x0500)
		case FSCTL_ALLOW_EXTENDED_DASD_IO:
			Status = FFSAllowExtendedDasdIo(IrpContext);
			break;
#endif //(_WIN32_WINNT >= 0x0500)

		default:

			FFSPrint((DBG_ERROR, "FFSUserFsRequest: Invalid User Request: %xh.\n", FsControlCode));
			Status = STATUS_INVALID_DEVICE_REQUEST;

			FFSCompleteIrpContext(IrpContext,  Status);
	}

	return Status;
}


BOOLEAN
FFSIsMediaWriteProtected(
	IN PFFS_IRP_CONTEXT   IrpContext,
	IN PDEVICE_OBJECT     TargetDevice)
{
	PIRP            Irp;
	KEVENT          Event;
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_IS_WRITABLE,
			TargetDevice,
			NULL,
			0,
			NULL,
			0,
			FALSE,
			&Event,
			&IoStatus);

	if (Irp == NULL)
	{
		return FALSE;
	}

	SetFlag(IoGetNextIrpStackLocation(Irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);

	Status = IoCallDriver(TargetDevice, Irp);

	if (Status == STATUS_PENDING)
	{

		(VOID) KeWaitForSingleObject(&Event,
					      Executive,
					      KernelMode,
					      FALSE,
					      (PLARGE_INTEGER)NULL);

		Status = IoStatus.Status;
	}

	return (BOOLEAN)(Status == STATUS_MEDIA_WRITE_PROTECTED);
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSMountVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT              MainDeviceObject;
	BOOLEAN                     GlobalDataResourceAcquired = FALSE;
	PIRP                        Irp;
	PIO_STACK_LOCATION          IoStackLocation;
	PDEVICE_OBJECT              TargetDeviceObject;
	NTSTATUS                    Status = STATUS_UNRECOGNIZED_VOLUME;
	PDEVICE_OBJECT              VolumeDeviceObject = NULL;
	PFFS_VCB                    Vcb;
	PFFS_SUPER_BLOCK            FFSSb = NULL;
	ULONG                       dwBytes;
	DISK_GEOMETRY               DiskGeometry;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		MainDeviceObject = IrpContext->DeviceObject;

		//
		//  Make sure we can wait.
		//

		SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

		//
		// This request is only allowed on the main device object
		//
		if (MainDeviceObject != FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		ExAcquireResourceExclusiveLite(
				&(FFSGlobal->Resource),
				TRUE);

		GlobalDataResourceAcquired = TRUE;

		if (FlagOn(FFSGlobal->Flags, FFS_UNLOAD_PENDING))
		{
			Status = STATUS_UNRECOGNIZED_VOLUME;
			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		TargetDeviceObject =
			IoStackLocation->Parameters.MountVolume.DeviceObject;

		dwBytes = sizeof(DISK_GEOMETRY);
		Status = FFSDiskIoControl(
					TargetDeviceObject,
					IOCTL_DISK_GET_DRIVE_GEOMETRY,
					NULL,
					0,
					&DiskGeometry,
					&dwBytes);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}

		Status = IoCreateDevice(
					MainDeviceObject->DriverObject,
					sizeof(FFS_VCB),
					NULL,
					FILE_DEVICE_DISK_FILE_SYSTEM,
					0,
					FALSE,
					&VolumeDeviceObject);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}

		VolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);

		if (TargetDeviceObject->AlignmentRequirement > 
				VolumeDeviceObject->AlignmentRequirement)
		{

			VolumeDeviceObject->AlignmentRequirement = 
				TargetDeviceObject->AlignmentRequirement;
		}

		(IoStackLocation->Parameters.MountVolume.Vpb)->DeviceObject =
			VolumeDeviceObject;

		Vcb = (PFFS_VCB)VolumeDeviceObject->DeviceExtension;

		RtlZeroMemory(Vcb, sizeof(FFS_VCB));

		Vcb->Identifier.Type = FFSVCB;
		Vcb->Identifier.Size = sizeof(FFS_VCB);

		Vcb->TargetDeviceObject = TargetDeviceObject;
		Vcb->DiskGeometry = DiskGeometry;

		Status = FFSLoadDiskLabel(TargetDeviceObject, Vcb);

		if (!NT_SUCCESS(Status))
		{
			_SEH2_LEAVE;
		}


		FFSSb = Vcb->ffs_super_block;

		Vcb->BlockSize = FFSSb->fs_bsize;
		Vcb->SectorBits = FFSLog2(SECTOR_SIZE);

		Status = FFSInitializeVcb(IrpContext, Vcb, FFSSb, TargetDeviceObject, 
				VolumeDeviceObject, IoStackLocation->Parameters.MountVolume.Vpb);

		if (NT_SUCCESS(Status))
		{
			if (FFSIsMediaWriteProtected(IrpContext, TargetDeviceObject))
			{
				SetFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
			}
			else
			{
				ClearFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
			}

			SetFlag(Vcb->Flags, VCB_MOUNTED);

			FFSInsertVcb(Vcb);

			ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);
		}
	}

	_SEH2_FINALLY
	{
		if (GlobalDataResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
					ExGetCurrentResourceThread());
		}

		if (!NT_SUCCESS(Status))
		{
			if (FFSSb)
			{
				ExFreePool(FFSSb);
			}

			if (VolumeDeviceObject)
			{
				IoDeleteDevice(VolumeDeviceObject);
			}
		}

		if (!IrpContext->ExceptionInProgress)
		{
			if (NT_SUCCESS(Status))
			{
				ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);
			}

			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSVerifyVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT          DeviceObject;
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	PFFS_SUPER_BLOCK        FFSSb = NULL;
	PFFS_VCB                Vcb = 0;
	BOOLEAN                 VcbResourceAcquired = FALSE;
	BOOLEAN                 GlobalResourceAcquired = FALSE;
	PIRP                    Irp;
	PIO_STACK_LOCATION      IoStackLocation;
	ULONG                   ChangeCount;
	ULONG                   dwReturn;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

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

		ExAcquireResourceExclusiveLite(
				&FFSGlobal->Resource,
				TRUE);

		GlobalResourceAcquired = TRUE;

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,
				TRUE);

		VcbResourceAcquired = TRUE;

		if (!FlagOn(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME))
		{
			Status = STATUS_SUCCESS;
			_SEH2_LEAVE;
		}

		if (!IsMounted(Vcb))
		{
			Status = STATUS_WRONG_VOLUME;
			_SEH2_LEAVE;
		}

		dwReturn = sizeof(ULONG);
		Status = FFSDiskIoControl(
				Vcb->TargetDeviceObject,
				IOCTL_DISK_CHECK_VERIFY,
				NULL,
				0,
				&ChangeCount,
				&dwReturn);

		if (ChangeCount != Vcb->ChangeCount)
		{
			Status = STATUS_WRONG_VOLUME;
			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		if (((((FFSSb = FFSLoadSuper(Vcb, TRUE, SBLOCK_UFS1)) != NULL) && (FFSSb->fs_magic == FS_UFS1_MAGIC)) ||
			 (((FFSSb = FFSLoadSuper(Vcb, TRUE, SBLOCK_UFS2)) != NULL) && (FFSSb->fs_magic == FS_UFS2_MAGIC))) &&
				(memcmp(FFSSb->fs_id, SUPER_BLOCK->fs_id, 8) == 0) &&
				(memcmp(FFSSb->fs_volname, SUPER_BLOCK->fs_volname, 16) == 0))
		{
			ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);

			if (FFSIsMediaWriteProtected(IrpContext, Vcb->TargetDeviceObject))
			{
				SetFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
			}
			else
			{
				ClearFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
			}

			FFSPrint((DBG_INFO, "FFSVerifyVolume: Volume verify succeeded.\n"));

			Status = STATUS_SUCCESS;
		}
		else
		{
			Status = STATUS_WRONG_VOLUME;

			FFSPurgeVolume(Vcb, FALSE);

			SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);

			ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);

			FFSPrint((DBG_INFO, "FFSVerifyVolume: Volume verify failed.\n"));
		}
	}

	_SEH2_FINALLY
	{
		if (FFSSb)
			ExFreePool(FFSSb);

		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (GlobalResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSIsVolumeMounted(
	IN PFFS_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	return FFSVerifyVolume(IrpContext);
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSDismountVolume(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT  DeviceObject;
	NTSTATUS        Status = STATUS_UNSUCCESSFUL;
	PFFS_VCB        Vcb = 0;
	BOOLEAN         VcbResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

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

		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource,
				TRUE);

		VcbResourceAcquired = TRUE;

		if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
		{
			Status = STATUS_VOLUME_DISMOUNTED;
			_SEH2_LEAVE;
		}

		/*        
		if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
		{
			FFSPrint((DBG_ERROR, "FFSDismount: Volume is not locked.\n"));

			Status = STATUS_ACCESS_DENIED;
			_SEH2_LEAVE;
		}
		*/

		FFSFlushFiles(Vcb, FALSE);

		FFSFlushVolume(Vcb, FALSE);

		FFSPurgeVolume(Vcb, TRUE);

		ExReleaseResourceForThreadLite(
				&Vcb->MainResource,
				ExGetCurrentResourceThread());

		VcbResourceAcquired = FALSE;

		FFSCheckDismount(IrpContext, Vcb, TRUE);

		FFSPrint((DBG_INFO, "FFSDismount: Volume dismount pending.\n"));

		Status = STATUS_SUCCESS;
	}
	_SEH2_FINALLY
	{
		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
BOOLEAN
FFSCheckDismount(
	IN PFFS_IRP_CONTEXT  IrpContext,
	IN PFFS_VCB          Vcb,
	IN BOOLEAN           bForce)
{
	KIRQL   Irql;
	PVPB    Vpb = Vcb->Vpb;
	BOOLEAN bDeleted = FALSE;
	ULONG   UnCleanCount = 0;

    PAGED_CODE();

	ExAcquireResourceExclusiveLite(
			&FFSGlobal->Resource, TRUE);

	ExAcquireResourceExclusiveLite(
			&Vcb->MainResource, TRUE);

	if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
			(IrpContext->RealDevice == Vcb->RealDevice))
	{
		UnCleanCount = 3;
	}
	else
	{
		UnCleanCount = 2;
	}

	IoAcquireVpbSpinLock(&Irql);

	if ((Vpb->ReferenceCount == UnCleanCount) || bForce)
	{

		if ((Vpb->ReferenceCount != UnCleanCount) && bForce)
		{
			FFSBreakPoint();
		}

		ClearFlag(Vpb->Flags, VPB_MOUNTED);
		ClearFlag(Vpb->Flags, VPB_LOCKED);

		if ((Vcb->RealDevice != Vpb->RealDevice) &&
#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed in file system drivers" )
#endif
				(Vcb->RealDevice->Vpb == Vpb))
		{
			SetFlag(Vcb->RealDevice->Flags, DO_DEVICE_INITIALIZING);
			SetFlag(Vpb->Flags, VPB_PERSISTENT);
		}

		FFSRemoveVcb(Vcb);

		ClearFlag(Vpb->Flags, VPB_MOUNTED);
		SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);

		Vpb->DeviceObject = NULL;

		bDeleted = TRUE;
	}

#if 0

	else if ((Vpb->RealDevice->Vpb == Vpb) && bForce)
	{
		PVPB NewVpb;

#define TAG_VPB                         ' bpV'

		NewVpb = ExAllocatePoolWithTag(NonPagedPoolMustSucceed, 
				sizeof(VPB), TAG_VPB);

		NewVpb->Type = IO_TYPE_VPB;
		NewVpb->Size = sizeof(VPB);
		NewVpb->RealDevice = Vcb->Vpb->RealDevice;

		NewVpb->RealDevice->Vpb = NewVpb;

		NewVpb->Flags = FlagOn(Vcb->Vpb->Flags, VPB_REMOVE_PENDING);

		NewVpb = NULL;

		ClearFlag(Vcb->Flags, VCB_MOUNTED);
		ClearFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
	}

#endif

	IoReleaseVpbSpinLock(Irql);

	ExReleaseResourceForThreadLite(
			&Vcb->MainResource,
			ExGetCurrentResourceThread());

	ExReleaseResourceForThreadLite(
			&FFSGlobal->Resource,
			ExGetCurrentResourceThread());

	if (bDeleted)
	{
#if 0
		FFSBreakPoint(); /* XP에서 브레이크 포인트 발생 */
#endif

		FFSFreeVcb(Vcb);
	}

	return bDeleted;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPurgeVolume(
	IN PFFS_VCB Vcb,
	IN BOOLEAN  FlushBeforePurge)
{
	PFFS_FCB        Fcb;
	LIST_ENTRY      FcbList;
	PLIST_ENTRY     ListEntry;
	PFCB_LIST_ENTRY FcbListEntry;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
				IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
		{
			FlushBeforePurge = FALSE;
		}

		FcbListEntry= NULL;
		InitializeListHead(&FcbList);

		for (ListEntry = Vcb->FcbList.Flink;
				ListEntry != &Vcb->FcbList;
				ListEntry = ListEntry->Flink)
		{
			Fcb = CONTAINING_RECORD(ListEntry, FFS_FCB, Next);

			Fcb->ReferenceCount++;

			FFSPrint((DBG_INFO, "FFSPurgeVolume: %s refercount=%xh\n", Fcb->AnsiFileName.Buffer, Fcb->ReferenceCount));

			FcbListEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FCB_LIST_ENTRY), FFS_POOL_TAG);

			if (FcbListEntry)
			{
				FcbListEntry->Fcb = Fcb;

				InsertTailList(&FcbList, &FcbListEntry->Next);
			}
			else
			{
				FFSPrint((DBG_ERROR, "FFSPurgeVolume: Error allocating FcbListEntry ...\n"));
			}
		}

		while (!IsListEmpty(&FcbList))
		{
			ListEntry = RemoveHeadList(&FcbList);

			FcbListEntry = CONTAINING_RECORD(ListEntry, FCB_LIST_ENTRY, Next);

			Fcb = FcbListEntry->Fcb;

			if (ExAcquireResourceExclusiveLite(
						&Fcb->MainResource,
						TRUE))
			{
				FFSPurgeFile(Fcb, FlushBeforePurge);

				if (!Fcb->OpenHandleCount && Fcb->ReferenceCount == 1)
				{
					RemoveEntryList(&Fcb->Next);
					FFSFreeFcb(Fcb);
				}
				else
				{
					ExReleaseResourceForThreadLite(
						&Fcb->MainResource,
						ExGetCurrentResourceThread());
				}
			}

			ExFreePool(FcbListEntry);
		}

		if (FlushBeforePurge)
		{
			ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
			ExReleaseResourceLite(&Vcb->PagingIoResource);

			CcFlushCache(&Vcb->SectionObject, NULL, 0, NULL);
		}

		if (Vcb->SectionObject.ImageSectionObject)
		{
			MmFlushImageSection(&Vcb->SectionObject, MmFlushForWrite);
		}

		if (Vcb->SectionObject.DataSectionObject)
		{
			CcPurgeCacheSection(&Vcb->SectionObject, NULL, 0, FALSE);
		}

		FFSPrint((DBG_INFO, "FFSPurgeVolume: Volume flushed and purged.\n"));
	}
	_SEH2_FINALLY
	{
		// Nothing
	} _SEH2_END;

	return STATUS_SUCCESS;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSPurgeFile(
	IN PFFS_FCB Fcb,
	IN BOOLEAN  FlushBeforePurge)
{
	IO_STATUS_BLOCK    IoStatus;

    PAGED_CODE();

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));


	if(!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) && FlushBeforePurge &&
			!IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED))
	{

		FFSPrint((DBG_INFO, "FFSPurgeFile: CcFlushCache on %s.\n", 
					Fcb->AnsiFileName.Buffer));

		ExAcquireSharedStarveExclusive(&Fcb->PagingIoResource, TRUE);
		ExReleaseResourceLite(&Fcb->PagingIoResource);

		CcFlushCache(&Fcb->SectionObject, NULL, 0, &IoStatus);

		ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
	}

	if (Fcb->SectionObject.ImageSectionObject)
	{

		FFSPrint((DBG_INFO, "FFSPurgeFile: MmFlushImageSection on %s.\n", 
					Fcb->AnsiFileName.Buffer));

		MmFlushImageSection(&Fcb->SectionObject, MmFlushForWrite);
	}

	if (Fcb->SectionObject.DataSectionObject)
	{

		FFSPrint((DBG_INFO, "FFSPurgeFile: CcPurgeCacheSection on %s.\n",
					Fcb->AnsiFileName.Buffer));

		CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);
	}

	return STATUS_SUCCESS;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSSelectBSDPartition(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PIRP                Irp;
	NTSTATUS            Status;
	PFFS_BSD_PARTITION  BSDPartition;

	_SEH2_TRY
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		Irp = IrpContext->Irp;

		BSDPartition = (PFFS_BSD_PARTITION)Irp->AssociatedIrp.SystemBuffer;
		FFSGlobal->PartitionNumber = BSDPartition->Number;

		Status = STATUS_SUCCESS;
	}
	_SEH2_FINALLY
	{
		if (!IrpContext->ExceptionInProgress)
		{
			FFSCompleteIrpContext(IrpContext,  Status);
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSFileSystemControl(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS    Status;

    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	switch (IrpContext->MinorFunction)
	{
		case IRP_MN_USER_FS_REQUEST:
			Status = FFSUserFsRequest(IrpContext);
			break;

		case IRP_MN_MOUNT_VOLUME:
			Status = FFSMountVolume(IrpContext);
			break;

		case IRP_MN_VERIFY_VOLUME:
			Status = FFSVerifyVolume(IrpContext);
			break;

		default:

			FFSPrint((DBG_ERROR, "FFSFilsSystemControl: Invalid Device Request.\n"));
			Status = STATUS_INVALID_DEVICE_REQUEST;
			FFSCompleteIrpContext(IrpContext,  Status);
	}

	return Status;
}
