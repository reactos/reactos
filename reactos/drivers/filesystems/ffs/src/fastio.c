/* 
 * FFS File System Driver for Windows
 *
 * fastio.c
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
#if DBG
#pragma alloc_text(PAGE, FFSFastIoRead)
#pragma alloc_text(PAGE, FFSFastIoWrite)
#endif
#pragma alloc_text(PAGE, FFSFastIoCheckIfPossible)
#pragma alloc_text(PAGE, FFSFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, FFSFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, FFSFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, FFSFastIoLock)
#pragma alloc_text(PAGE, FFSFastIoUnlockSingle)
#pragma alloc_text(PAGE, FFSFastIoUnlockAll)
#pragma alloc_text(PAGE, FFSFastIoUnlockAllByKey)
#endif


BOOLEAN NTAPI
FFSFastIoCheckIfPossible(
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN ULONG                Length,
	IN BOOLEAN              Wait,
	IN ULONG                LockKey,
	IN BOOLEAN              CheckForReadOperation,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN          bPossible = FALSE;
	PFFS_FCB         Fcb;
	LARGE_INTEGER    lLength;

    PAGED_CODE();

	lLength.QuadPart = Length;

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				bPossible = FastIoIsNotPossible;
				_SEH2_LEAVE;
			}

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				bPossible = FastIoIsNotPossible;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			if (IsDirectory(Fcb))
			{
				bPossible = FALSE;
				_SEH2_LEAVE;
			}

			if (CheckForReadOperation)
			{
				bPossible = FsRtlFastCheckLockForRead(
								&Fcb->FileLockAnchor,
								FileOffset,
								&lLength,
								LockKey,
								FileObject,
								PsGetCurrentProcess());

				if (!bPossible)
					bPossible = FastIoIsQuestionable;
			}
			else
			{
				if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) ||
						IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED))
				{
					bPossible = FastIoIsNotPossible;
				}
				else
				{
					bPossible = FsRtlFastCheckLockForWrite(
									&Fcb->FileLockAnchor,
									FileOffset,
									&lLength,
									LockKey,
									FileObject,
									PsGetCurrentProcess());

					if (!bPossible)
						bPossible = FastIoIsQuestionable;
				}
			}

			FFSPrint((DBG_INFO, "FFSFastIIOCheckPossible: %s %s %s\n",
						FFSGetCurrentProcessName(),
						"FASTIO_CHECK_IF_POSSIBLE",
						Fcb->AnsiFileName.Buffer));

			FFSPrint((DBG_INFO, 
						"FFSFastIIOCheckPossible: Offset: %I64xg Length: %xh Key: %u %s %s\n",
						FileOffset->QuadPart,
						Length,
						LockKey,
						(CheckForReadOperation ? "CheckForReadOperation:" :
						 "CheckForWriteOperation:"),
						(bPossible ? "Succeeded" : "Failed")));
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			bPossible = FastIoIsNotPossible;
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;

	return bPossible;
}


#if DBG

BOOLEAN NTAPI
FFSFastIoRead(
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN ULONG                Length,
	IN BOOLEAN              Wait,
	IN ULONG                LockKey,
	OUT PVOID               Buffer,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN     Status;
	PFFS_FCB    Fcb;

    PAGED_CODE();

	Fcb = (PFFS_FCB)FileObject->FsContext;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO, "FFSFastIoRead: %s %s %s\n",
				FFSGetCurrentProcessName(),
				"FASTIO_READ",
				Fcb->AnsiFileName.Buffer));

	FFSPrint((DBG_INFO, "FFSFastIoRead: Offset: %I64xh Length: %xh Key: %u\n",
				FileOffset->QuadPart,
				Length,
				LockKey));

	Status = FsRtlCopyRead(
			FileObject, FileOffset, Length, Wait,
			LockKey, Buffer, IoStatus, DeviceObject);

	return Status;
}

#if !FFS_READ_ONLY

BOOLEAN NTAPI
FFSFastIoWrite(
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN ULONG                Length,
	IN BOOLEAN              Wait,
	IN ULONG                LockKey,
	OUT PVOID               Buffer,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN      bRet;
	PFFS_FCB     Fcb;

    PAGED_CODE();

	Fcb = (PFFS_FCB)FileObject->FsContext;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO,
				"FFSFastIoWrite: %s %s %s\n",
				FFSGetCurrentProcessName(),
				"FASTIO_WRITE",
				Fcb->AnsiFileName.Buffer));

	FFSPrint((DBG_INFO,
				"FFSFastIoWrite: Offset: %I64xh Length: %xh Key: %xh\n",
				FileOffset->QuadPart,
				Length,
				LockKey));

	if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
	{
		FFSBreakPoint();
		return FALSE;
	}

	bRet = FsRtlCopyWrite(
			FileObject, FileOffset, Length, Wait,
			LockKey, Buffer, IoStatus, DeviceObject);

	return bRet;
}

#endif // !FFS_READ_ONLY

#endif /* DBG */


__drv_mustHoldCriticalRegion
BOOLEAN NTAPI
FFSFastIoQueryBasicInfo(
	IN PFILE_OBJECT             FileObject,
	IN BOOLEAN                  Wait,
	OUT PFILE_BASIC_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK        IoStatus,
	IN PDEVICE_OBJECT           DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_VCB    Vcb;
	PFFS_FCB    Fcb = 0;
	BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

			ASSERT(Vcb != NULL);

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			FFSPrint((DBG_INFO, 
						"FFSFastIoQueryBasicInfo: %s %s %s\n",
						FFSGetCurrentProcessName(),
						"FASTIO_QUERY_BASIC_INFO",
						Fcb->AnsiFileName.Buffer));

			if (!ExAcquireResourceSharedLite(
						&Fcb->MainResource,
						Wait))
			{
				Status = FALSE;
				_SEH2_LEAVE;
			}

			FcbMainResourceAcquired = TRUE;

			RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));

			/*
			   typedef struct _FILE_BASIC_INFORMATION {
			   LARGE_INTEGER   CreationTime;
			   LARGE_INTEGER   LastAccessTime;
			   LARGE_INTEGER   LastWriteTime;
			   LARGE_INTEGER   ChangeTime;
			   ULONG           FileAttributes;
			   } FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
			   */

			if (FS_VERSION == 1)
			{
				Buffer->CreationTime = FFSSysTime(Fcb->dinode1->di_ctime);
				Buffer->LastAccessTime = FFSSysTime(Fcb->dinode1->di_atime);
				Buffer->LastWriteTime = FFSSysTime(Fcb->dinode1->di_mtime);
				Buffer->ChangeTime = FFSSysTime(Fcb->dinode1->di_mtime);
			}
			else
			{
				Buffer->CreationTime = FFSSysTime((ULONG)Fcb->dinode2->di_ctime);
				Buffer->LastAccessTime = FFSSysTime((ULONG)Fcb->dinode2->di_atime);
				Buffer->LastWriteTime = FFSSysTime((ULONG)Fcb->dinode2->di_mtime);
				Buffer->ChangeTime = FFSSysTime((ULONG)Fcb->dinode2->di_mtime);
			}


			Buffer->FileAttributes = Fcb->FFSMcb->FileAttr;

			IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

			IoStatus->Status = STATUS_SUCCESS;

			Status =  TRUE;
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}

	_SEH2_FINALLY
	{
		if (FcbMainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread());
		}

		FsRtlExitFileSystem();
	} _SEH2_END;


	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR, 
					"FFSFastIoQueryBasicInfo: %s %s *** Status: FALSE ***\n",
					FFSGetCurrentProcessName(),
					"FASTIO_QUERY_BASIC_INFO"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR, 
					"FFSFastIoQueryBasicInfo: %s %s *** Status: %s (%#x) ***\n",
					FFSFastIoQueryBasicInfo,
					"FASTIO_QUERY_BASIC_INFO",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}

	return Status;
}


__drv_mustHoldCriticalRegion
BOOLEAN NTAPI
FFSFastIoQueryStandardInfo(
	IN PFILE_OBJECT                 FileObject,
	IN BOOLEAN                      Wait,
	OUT PFILE_STANDARD_INFORMATION  Buffer,
	OUT PIO_STATUS_BLOCK            IoStatus,
	IN PDEVICE_OBJECT               DeviceObject)
{

	BOOLEAN     Status = FALSE;
	PFFS_VCB    Vcb;
	PFFS_FCB    Fcb = 0;
	BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

			ASSERT(Vcb != NULL);

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			FFSPrint((DBG_INFO,
						"FFSFastIoQueryStandardInfo: %s %s %s\n",
						FFSGetCurrentProcessName(),
						"FASTIO_QUERY_STANDARD_INFO",
						Fcb->AnsiFileName.Buffer));

			if (!ExAcquireResourceSharedLite(
						&Fcb->MainResource,
						Wait))
			{
				Status = FALSE;
				_SEH2_LEAVE;
			}

			FcbMainResourceAcquired = TRUE;

			RtlZeroMemory(Buffer, sizeof(FILE_STANDARD_INFORMATION));

			/*
			   typedef struct _FILE_STANDARD_INFORMATION {
			   LARGE_INTEGER   AllocationSize;
			   LARGE_INTEGER   EndOfFile;
			   ULONG           NumberOfLinks;
			   BOOLEAN         DeletePending;
			   BOOLEAN         Directory;
			   } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
			   */

			if (FS_VERSION == 1)
			{
				Buffer->AllocationSize.QuadPart =
					(LONGLONG)(Fcb->dinode1->di_size);

				Buffer->EndOfFile.QuadPart =
					(LONGLONG)(Fcb->dinode1->di_size);

				Buffer->NumberOfLinks = Fcb->dinode1->di_nlink;
			}
			else
			{
				Buffer->AllocationSize.QuadPart =
					(LONGLONG)(Fcb->dinode2->di_size);

				Buffer->EndOfFile.QuadPart =
					(LONGLONG)(Fcb->dinode2->di_size);

				Buffer->NumberOfLinks = Fcb->dinode2->di_nlink;
			}


			if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
			{
				Buffer->DeletePending = FALSE;
			}
			else
			{
				Buffer->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
			}

			if (FlagOn(Fcb->FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
			{
				Buffer->Directory = TRUE;
			}
			else
			{
				Buffer->Directory = FALSE;
			}

			IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

			IoStatus->Status = STATUS_SUCCESS;

			Status =  TRUE;
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}

	_SEH2_FINALLY
	{
		if (FcbMainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread()
					);
		}

		FsRtlExitFileSystem();
	} _SEH2_END;

#if DBG
	if (Status == FALSE)
	{
		FFSPrint((DBG_INFO,
					"FFSFastIoQueryStandardInfo: %s %s *** Status: FALSE ***\n",
					FFSGetCurrentProcessName(),
					"FASTIO_QUERY_STANDARD_INFO"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_INFO,
					"FFSFastIoQueryStandardInfo: %s %s *** Status: %s (%#x) ***\n",
					FFSGetCurrentProcessName(),
					"FASTIO_QUERY_STANDARD_INFO",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}
#endif

	return Status;
}


__drv_mustHoldCriticalRegion
BOOLEAN NTAPI
FFSFastIoQueryNetworkOpenInfo(
	IN PFILE_OBJECT                     FileObject,
	IN BOOLEAN                          Wait,
	OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
	OUT PIO_STATUS_BLOCK                IoStatus,
	IN PDEVICE_OBJECT                   DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_VCB    Vcb;
	PFFS_FCB    Fcb = 0;
	BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

			ASSERT(Vcb != NULL);

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			FFSPrint((DBG_INFO, 
						"%-16.16s %-31s %s\n",
						FFSGetCurrentProcessName(),
						"FASTIO_QUERY_NETWORK_OPEN_INFO",
						Fcb->AnsiFileName.Buffer));

			if (!ExAcquireResourceSharedLite(
						&Fcb->MainResource,
						Wait))
			{
				Status = FALSE;
				_SEH2_LEAVE;
			}

			FcbMainResourceAcquired = TRUE;

			RtlZeroMemory(Buffer, sizeof(FILE_NETWORK_OPEN_INFORMATION));

			/*
			typedef struct _FILE_NETWORK_OPEN_INFORMATION {
				LARGE_INTEGER   CreationTime;
				LARGE_INTEGER   LastAccessTime;
				LARGE_INTEGER   LastWriteTime;
				LARGE_INTEGER   ChangeTime;
				LARGE_INTEGER   AllocationSize;
				LARGE_INTEGER   EndOfFile;
				ULONG           FileAttributes;
			} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;
			*/

			if (FS_VERSION == 1)
			{
				Buffer->CreationTime = FFSSysTime(Fcb->dinode1->di_ctime);
				Buffer->LastAccessTime = FFSSysTime(Fcb->dinode1->di_atime);
				Buffer->LastWriteTime = FFSSysTime(Fcb->dinode1->di_mtime);
				Buffer->ChangeTime = FFSSysTime(Fcb->dinode1->di_mtime);
				Buffer->FileAttributes = Fcb->FFSMcb->FileAttr;
				Buffer->AllocationSize.QuadPart =
					(LONGLONG)(Fcb->dinode1->di_size);
				Buffer->EndOfFile.QuadPart =
					(LONGLONG)(Fcb->dinode1->di_size);
			}
			else
			{
				Buffer->CreationTime = FFSSysTime((ULONG)Fcb->dinode2->di_ctime);
				Buffer->LastAccessTime = FFSSysTime((ULONG)Fcb->dinode2->di_atime);
				Buffer->LastWriteTime = FFSSysTime((ULONG)Fcb->dinode2->di_mtime);
				Buffer->ChangeTime = FFSSysTime((ULONG)Fcb->dinode2->di_mtime);
				Buffer->FileAttributes = Fcb->FFSMcb->FileAttr;
				Buffer->AllocationSize.QuadPart =
					(LONGLONG)(Fcb->dinode2->di_size);
				Buffer->EndOfFile.QuadPart =
					(LONGLONG)(Fcb->dinode2->di_size);
			}

			Buffer->FileAttributes = Fcb->FFSMcb->FileAttr;

			IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

			IoStatus->Status = STATUS_SUCCESS;

			Status = TRUE;
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		if (FcbMainResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread()
					);
		}

		FsRtlExitFileSystem();
	} _SEH2_END;



	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR,
					"%s %s *** Status: FALSE ***\n",
					FFSGetCurrentProcessName(),
					"FASTIO_QUERY_NETWORK_OPEN_INFO"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR,
					"%s %s *** Status: %s (%#x) ***\n",
					FFSGetCurrentProcessName(),
					"FASTIO_QUERY_NETWORK_OPEN_INFO",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}

	return Status;
}


BOOLEAN NTAPI
FFSFastIoLock(
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN PLARGE_INTEGER       Length,
	IN PEPROCESS            Process,
	IN ULONG                Key,
	IN BOOLEAN              FailImmediately,
	IN BOOLEAN              ExclusiveLock,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_FCB    Fcb;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			if (IsDirectory(Fcb))
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			FFSPrint((DBG_INFO,
						"FFSFastIoLock: %s %s %s\n",
						FFSGetCurrentProcessName(),
						"FASTIO_LOCK",
						Fcb->AnsiFileName.Buffer));

			FFSPrint((DBG_INFO,
						"FFSFastIoLock: Offset: %I64xh Length: %I64xh Key: %u %s%s\n",
						FileOffset->QuadPart,
						Length->QuadPart,
						Key,
						(FailImmediately ? "FailImmediately " : ""),
						(ExclusiveLock ? "ExclusiveLock " : "")));

			if (Fcb->Header.IsFastIoPossible != FastIoIsQuestionable)
			{
				FFSPrint((DBG_INFO,
							"FFSFastIoLock: %s %s %s\n",
							(PUCHAR) Process + ProcessNameOffset,
							"FastIoIsQuestionable",
							Fcb->AnsiFileName.Buffer));

				Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
			}
#ifdef _MSC_VER
#pragma prefast( suppress: 28159, "bug in prefast" )
#endif
			Status = FsRtlFastLock(
						&Fcb->FileLockAnchor,
						FileObject,
						FileOffset,
						Length,
						Process,
						Key,
						FailImmediately,
						ExclusiveLock,
						IoStatus,
						NULL,
						FALSE);
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}

	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;

#if DBG 
	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR, 
					"FFSFastIoLock: %s %s *** Status: FALSE ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_LOCK"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoLock: %s %s *** Status: %s (%#x) ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_LOCK",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}
#endif

	return Status;
}


BOOLEAN NTAPI
FFSFastIoUnlockSingle(
	IN PFILE_OBJECT         FileObject,
	IN PLARGE_INTEGER       FileOffset,
	IN PLARGE_INTEGER       Length,
	IN PEPROCESS            Process,
	IN ULONG                Key,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_FCB    Fcb;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			if (IsDirectory(Fcb))
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			FFSPrint((DBG_INFO,
						"FFSFastIoUnlockSingle: %s %s %s\n",
						(PUCHAR) Process + ProcessNameOffset,
						"FASTIO_UNLOCK_SINGLE",
						Fcb->AnsiFileName.Buffer));

			FFSPrint((DBG_INFO,
						"FFSFastIoUnlockSingle: Offset: %I64xh Length: %I64xh Key: %u\n",
						FileOffset->QuadPart,
						Length->QuadPart,
						Key));

			IoStatus->Status = FsRtlFastUnlockSingle(
									&Fcb->FileLockAnchor,
									FileObject,
									FileOffset,
									Length,
									Process,
									Key,
									NULL,
									FALSE);                      

			IoStatus->Information = 0;

			Status =  TRUE;
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;

#if DBG 
	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockSingle: %s %s *** Status: FALSE ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_SINGLE"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockSingle: %s %s *** Status: %s (%#x) ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_SINGLE",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}
#endif  
	return Status;
}


BOOLEAN NTAPI
FFSFastIoUnlockAll(
	IN PFILE_OBJECT         FileObject,
	IN PEPROCESS            Process,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_FCB    Fcb;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			if (IsDirectory(Fcb))
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			FFSPrint((DBG_INFO,
						"FFSFastIoUnlockSingle: %s %s %s\n",
						(PUCHAR) Process + ProcessNameOffset,
						"FASTIO_UNLOCK_ALL",
						Fcb->AnsiFileName.Buffer));

			IoStatus->Status = FsRtlFastUnlockAll(
									&Fcb->FileLockAnchor,
									FileObject,
									Process,
									NULL);

			IoStatus->Information = 0;

			Status = TRUE;
		}
		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}
	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;

#if DBG 
	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockSingle: %s %s *** Status: FALSE ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_ALL"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockSingle: %s %s *** Status: %s (%#x) ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_ALL",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}
#endif  
	return Status;
}


BOOLEAN NTAPI
FFSFastIoUnlockAllByKey(
	IN PFILE_OBJECT         FileObject,
#ifndef __REACTOS__
	IN PEPROCESS            Process,
#else
	IN PVOID                Process,
#endif
	IN ULONG                Key,
	OUT PIO_STATUS_BLOCK    IoStatus,
	IN PDEVICE_OBJECT       DeviceObject)
{
	BOOLEAN     Status = FALSE;
	PFFS_FCB    Fcb;

    PAGED_CODE();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
            FsRtlEnterFileSystem();

			if (DeviceObject == FFSGlobal->DeviceObject)
			{
				IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			Fcb = (PFFS_FCB)FileObject->FsContext;

			ASSERT(Fcb != NULL);

			if (Fcb->Identifier.Type == FFSVCB)
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			ASSERT((Fcb->Identifier.Type == FFSFCB) &&
					(Fcb->Identifier.Size == sizeof(FFS_FCB)));

			if (IsDirectory(Fcb))
			{
				FFSBreakPoint();
				IoStatus->Status = STATUS_INVALID_PARAMETER;
				Status = TRUE;
				_SEH2_LEAVE;
			}

			FFSPrint((DBG_INFO,
						"FFSFastIoUnlockAllByKey: %s %s %s\n",
						(PUCHAR) Process + ProcessNameOffset,
						"FASTIO_UNLOCK_ALL_BY_KEY",
						Fcb->AnsiFileName.Buffer));

			FFSPrint((DBG_INFO,
						"FFSFastIoUnlockAllByKey: Key: %u\n",
						Key));

			IoStatus->Status = FsRtlFastUnlockAllByKey(
									&Fcb->FileLockAnchor,
									FileObject,
									Process,
									Key,
									NULL);  

			IoStatus->Information = 0;

			Status =  TRUE;
		}

		_SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
		{
			IoStatus->Status = _SEH2_GetExceptionCode();
			Status = TRUE;
		} _SEH2_END;
	}

	_SEH2_FINALLY
	{
		FsRtlExitFileSystem();
	} _SEH2_END;
#if DBG 
	if (Status == FALSE)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockAllByKey: %s %s *** Status: FALSE ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_ALL_BY_KEY"));
	}
	else if (IoStatus->Status != STATUS_SUCCESS)
	{
		FFSPrint((DBG_ERROR,
					"FFSFastIoUnlockAllByKey: %s %s *** Status: %s (%#x) ***\n",
					(PUCHAR) Process + ProcessNameOffset,
					"FASTIO_UNLOCK_ALL_BY_KEY",
					FFSNtStatusToString(IoStatus->Status),
					IoStatus->Status));
	}
#endif  
	return Status;
}
