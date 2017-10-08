/* 
 * FFS File System Driver for Windows
 *
 * dirctl.c
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
#pragma alloc_text(PAGE, FFSGetInfoLength)
#pragma alloc_text(PAGE, FFSProcessDirEntry)
#pragma alloc_text(PAGE, FFSQueryDirectory)
#pragma alloc_text(PAGE, FFSNotifyChangeDirectory)
#pragma alloc_text(PAGE, FFSDirectoryControl)
#pragma alloc_text(PAGE, FFSIsDirectoryEmpty)
#endif

ULONG
FFSGetInfoLength(
	IN FILE_INFORMATION_CLASS  FileInformationClass)
{
    PAGED_CODE();

	switch (FileInformationClass)
	{
		case FileDirectoryInformation:
			return sizeof(FILE_DIRECTORY_INFORMATION);
			break;

		case FileFullDirectoryInformation:
			return sizeof(FILE_FULL_DIR_INFORMATION);
			break;

		case FileBothDirectoryInformation:
			return sizeof(FILE_BOTH_DIR_INFORMATION);
			break;

		case FileNamesInformation:
			return sizeof(FILE_NAMES_INFORMATION);
			break;

		default:
			break;
	}

	return 0;
}


ULONG
FFSProcessDirEntry(
	IN PFFS_VCB                Vcb,
	IN FILE_INFORMATION_CLASS  FileInformationClass,
	IN ULONG                   in,
	IN PVOID                   Buffer,
	IN ULONG                   UsedLength,
	IN ULONG                   Length,
	IN ULONG                   FileIndex,
	IN PUNICODE_STRING         pName,
	IN BOOLEAN                 Single)
{
	FFSv1_INODE dinode1;
	FFSv2_INODE dinode2;
	PFILE_DIRECTORY_INFORMATION FDI;
	PFILE_FULL_DIR_INFORMATION FFI;
	PFILE_BOTH_DIR_INFORMATION FBI;
	PFILE_NAMES_INFORMATION FNI;

	ULONG InfoLength = 0;
	ULONG NameLength = 0;
	ULONG dwBytes = 0;

    PAGED_CODE();

	NameLength = pName->Length;

	if (!in)
	{
		FFSPrint((DBG_ERROR, "FFSPricessDirEntry: ffs_dir_entry is empty.\n"));
		return 0;
	}

	InfoLength = FFSGetInfoLength(FileInformationClass);

	if (!InfoLength || InfoLength + NameLength - sizeof(WCHAR) > Length)
	{
		FFSPrint((DBG_INFO, "FFSPricessDirEntry: Buffer is not enough.\n"));
		return 0;
	}

	if (FS_VERSION == 1)
	{
		if(!FFSv1LoadInode(Vcb, in, &dinode1))
		{
			FFSPrint((DBG_ERROR, "FFSPricessDirEntry: Loading inode %xh error.\n", in));

			FFSBreakPoint();

			return 0;
		}
	}
	else
	{
		if(!FFSv2LoadInode(Vcb, in, &dinode2))
		{
			FFSPrint((DBG_ERROR, "FFSPricessDirEntry: Loading inode %xh error.\n", in));

			FFSBreakPoint();

			return 0;
		}
	}

	switch(FileInformationClass)
	{
		case FileDirectoryInformation:
			FDI = (PFILE_DIRECTORY_INFORMATION) ((PUCHAR)Buffer + UsedLength);
			if (!Single)
				FDI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
			else
				FDI->NextEntryOffset = 0;
			FDI->FileIndex = FileIndex;

			if (FS_VERSION == 1)
			{
				FDI->CreationTime = FFSSysTime(dinode1.di_ctime);
				FDI->LastAccessTime = FFSSysTime(dinode1.di_atime);
				FDI->LastWriteTime = FFSSysTime(dinode1.di_mtime);
				FDI->ChangeTime = FFSSysTime(dinode1.di_mtime);
				FDI->EndOfFile.QuadPart = dinode1.di_size;
				FDI->AllocationSize.QuadPart = dinode1.di_size;
				FDI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || FFSIsReadOnly(dinode1.di_mode))
				{
					SetFlag(FDI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode1.di_mode & IFMT) == IFDIR)
					FDI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				FDI->FileNameLength = NameLength;
				RtlCopyMemory(FDI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 
				break;
			}
			else
			{
				FDI->CreationTime = FFSSysTime((ULONG)dinode2.di_ctime);
				FDI->LastAccessTime = FFSSysTime((ULONG)dinode2.di_atime);
				FDI->LastWriteTime = FFSSysTime((ULONG)dinode2.di_mtime);
				FDI->ChangeTime = FFSSysTime((ULONG)dinode2.di_mtime);
				FDI->EndOfFile.QuadPart = dinode2.di_size;
				FDI->AllocationSize.QuadPart = dinode2.di_size;
				FDI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || FFSIsReadOnly(dinode2.di_mode))
				{
					SetFlag(FDI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode2.di_mode & IFMT) == IFDIR)
					FDI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				FDI->FileNameLength = NameLength;
				RtlCopyMemory(FDI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 
				break;
			}

		case FileFullDirectoryInformation:
			FFI = (PFILE_FULL_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
			if (!Single)
				FFI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
			else
				FFI->NextEntryOffset = 0;
			FFI->FileIndex = FileIndex;

			if (FS_VERSION == 1)
			{
				FFI->CreationTime = FFSSysTime(dinode1.di_ctime);
				FFI->LastAccessTime = FFSSysTime(dinode1.di_atime);
				FFI->LastWriteTime = FFSSysTime(dinode1.di_mtime);
				FFI->ChangeTime = FFSSysTime(dinode1.di_mtime);
				FFI->EndOfFile.QuadPart = dinode1.di_size;
				FFI->AllocationSize.QuadPart = dinode1.di_size;
				FFI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)  || FFSIsReadOnly(dinode1.di_mode))
				{
					SetFlag(FFI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode1.di_mode & IFMT) == IFDIR)
					FFI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				FFI->FileNameLength = NameLength;
				RtlCopyMemory(FFI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

				break;
			}
			else
			{
				FFI->CreationTime = FFSSysTime((ULONG)dinode2.di_ctime);
				FFI->LastAccessTime = FFSSysTime((ULONG)dinode2.di_atime);
				FFI->LastWriteTime = FFSSysTime((ULONG)dinode2.di_mtime);
				FFI->ChangeTime = FFSSysTime((ULONG)dinode2.di_mtime);
				FFI->EndOfFile.QuadPart = dinode2.di_size;
				FFI->AllocationSize.QuadPart = dinode2.di_size;
				FFI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)  || FFSIsReadOnly(dinode2.di_mode))
				{
					SetFlag(FFI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode2.di_mode & IFMT) == IFDIR)
					FFI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				FFI->FileNameLength = NameLength;
				RtlCopyMemory(FFI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

				break;
			}

		case FileBothDirectoryInformation:
			FBI = (PFILE_BOTH_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
			if (!Single)
				FBI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
			else
				FBI->NextEntryOffset = 0;

			if (FS_VERSION == 1)
			{
				FBI->CreationTime = FFSSysTime(dinode1.di_ctime);
				FBI->LastAccessTime = FFSSysTime(dinode1.di_atime);
				FBI->LastWriteTime = FFSSysTime(dinode1.di_mtime);
				FBI->ChangeTime = FFSSysTime(dinode1.di_mtime);

				FBI->FileIndex = FileIndex;
				FBI->EndOfFile.QuadPart = dinode1.di_size;
				FBI->AllocationSize.QuadPart = dinode1.di_size;
				FBI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || FFSIsReadOnly(dinode1.di_mode))
				{
					SetFlag(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode1.di_mode & IFMT) == IFDIR)
					FBI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				FBI->FileNameLength = NameLength;
				RtlCopyMemory(FBI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

				break;
			}
			else
			{
				FBI->CreationTime = FFSSysTime((ULONG)dinode2.di_ctime);
				FBI->LastAccessTime = FFSSysTime((ULONG)dinode2.di_atime);
				FBI->LastWriteTime = FFSSysTime((ULONG)dinode2.di_mtime);
				FBI->ChangeTime = FFSSysTime((ULONG)dinode2.di_mtime);

				FBI->FileIndex = FileIndex;
				FBI->EndOfFile.QuadPart = dinode2.di_size;
				FBI->AllocationSize.QuadPart = dinode2.di_size;
				FBI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

				if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || FFSIsReadOnly(dinode2.di_mode))
				{
					SetFlag(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if ((dinode2.di_mode & IFMT) == IFDIR)
					FBI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				FBI->FileNameLength = NameLength;
				RtlCopyMemory(FBI->FileName, pName->Buffer, NameLength);
				dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

				break;
			}

		case FileNamesInformation:
			FNI = (PFILE_NAMES_INFORMATION) ((PUCHAR)Buffer + UsedLength);
			if (!Single)
				FNI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
			else
				FNI->NextEntryOffset = 0;
			FNI->FileNameLength = NameLength;
			RtlCopyMemory(FNI->FileName, pName->Buffer, NameLength);
			dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

			break;

		default:
			break;
	}

	return dwBytes;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSQueryDirectory(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT          DeviceObject;
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	PFFS_VCB                Vcb = 0;
	PFILE_OBJECT            FileObject;
	PFFS_FCB                Fcb = 0;
	PFFS_CCB                Ccb;
	PIRP                    Irp;
	PIO_STACK_LOCATION      IoStackLocation;
	FILE_INFORMATION_CLASS  FileInformationClass;
	ULONG                   Length;
	PUNICODE_STRING         FileName;
	ULONG                   FileIndex;
	BOOLEAN                 RestartScan;
	BOOLEAN                 ReturnSingleEntry;
	BOOLEAN                 IndexSpecified;
	PUCHAR                  Buffer;
	BOOLEAN                 FirstQuery;
	PFFSv1_INODE            dinode1 = NULL;
	PFFSv2_INODE            dinode2 = NULL;
	BOOLEAN                 FcbResourceAcquired = FALSE;
	ULONG                   UsedLength = 0;
	USHORT                  InodeFileNameLength;
	UNICODE_STRING          InodeFileName;
	PFFS_DIR_ENTRY          pDir = NULL;
	ULONG                   dwBytes;
	ULONG                   dwTemp = 0;
	ULONG                   dwSize = 0;
	ULONG                   dwReturn = 0;
	BOOLEAN                 bRun = TRUE;
	ULONG                   ByteOffset;

    PAGED_CODE();

	InodeFileName.Buffer = NULL;

	_SEH2_TRY
	{
		ASSERT(IrpContext);

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

		FileObject = IrpContext->FileObject;

		Fcb = (PFFS_FCB)FileObject->FsContext;

		ASSERT(Fcb);

		//
		// This request is not allowed on volumes
		//
		if (Fcb->Identifier.Type == FFSVCB)
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		if (!IsDirectory(Fcb))
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		Ccb = (PFFS_CCB)FileObject->FsContext2;

		ASSERT(Ccb);

		ASSERT((Ccb->Identifier.Type == FFSCCB) &&
				(Ccb->Identifier.Size == sizeof(FFS_CCB)));

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

#if !defined(_GNU_NTIFS_) || defined(__REACTOS__)

		FileInformationClass =
			IoStackLocation->Parameters.QueryDirectory.FileInformationClass;

		Length = IoStackLocation->Parameters.QueryDirectory.Length;

		FileName = IoStackLocation->Parameters.QueryDirectory.FileName;

		FileIndex = IoStackLocation->Parameters.QueryDirectory.FileIndex;

#else // _GNU_NTIFS_

		FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
				IoStackLocation)->Parameters.QueryDirectory.FileInformationClass;

		Length = ((PEXTENDED_IO_STACK_LOCATION)
				IoStackLocation)->Parameters.QueryDirectory.Length;

		FileName = ((PEXTENDED_IO_STACK_LOCATION)
				IoStackLocation)->Parameters.QueryDirectory.FileName;

		FileIndex = ((PEXTENDED_IO_STACK_LOCATION)
				IoStackLocation)->Parameters.QueryDirectory.FileIndex;

#endif // _GNU_NTIFS_

		RestartScan = FlagOn(IoStackLocation->Flags, SL_RESTART_SCAN);
		ReturnSingleEntry = FlagOn(IoStackLocation->Flags, SL_RETURN_SINGLE_ENTRY);
		IndexSpecified = FlagOn(IoStackLocation->Flags, SL_INDEX_SPECIFIED);
		/*
		if (!Irp->MdlAddress && Irp->UserBuffer)
		{
			ProbeForWrite(Irp->UserBuffer, Length, 1);
		}
		*/
		Buffer = FFSGetUserBuffer(Irp);

		if (Buffer == NULL)
		{
			FFSBreakPoint();
			Status = STATUS_INVALID_USER_BUFFER;
			_SEH2_LEAVE;
		}

		if (!IrpContext->IsSynchronous)
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		if (!ExAcquireResourceSharedLite(
					&Fcb->MainResource,
					IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		FcbResourceAcquired = TRUE;

		if (FileName != NULL)
		{
			if (Ccb->DirectorySearchPattern.Buffer != NULL)
			{
				FirstQuery = FALSE;
			}
			else
			{
				FirstQuery = TRUE;

				Ccb->DirectorySearchPattern.Length =
					Ccb->DirectorySearchPattern.MaximumLength =
					FileName->Length;

				Ccb->DirectorySearchPattern.Buffer =
					ExAllocatePoolWithTag(PagedPool, FileName->Length, FFS_POOL_TAG);

				if (Ccb->DirectorySearchPattern.Buffer == NULL)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
					_SEH2_LEAVE;
				}

				Status = RtlUpcaseUnicodeString(
						&(Ccb->DirectorySearchPattern),
						FileName,
						FALSE);

				if (!NT_SUCCESS(Status))
					_SEH2_LEAVE;
			}
		}
		else if (Ccb->DirectorySearchPattern.Buffer != NULL)
		{
			FirstQuery = FALSE;
			FileName = &Ccb->DirectorySearchPattern;
		}
		else
		{
			FirstQuery = TRUE;

			Ccb->DirectorySearchPattern.Length =
				Ccb->DirectorySearchPattern.MaximumLength = 2;

			Ccb->DirectorySearchPattern.Buffer =
				ExAllocatePoolWithTag(PagedPool, 2, FFS_POOL_TAG);

			if (Ccb->DirectorySearchPattern.Buffer == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			RtlCopyMemory(
					Ccb->DirectorySearchPattern.Buffer,
					L"*\0", 2);
		}

		if (!IndexSpecified)
		{
			if (RestartScan || FirstQuery)
			{
				FileIndex = Fcb->FFSMcb->DeOffset = 0;
			}
			else
			{
				FileIndex = Ccb->CurrentByteOffset;
			}
		}

		if (FS_VERSION == 1)
		{
			dinode1 = (PFFSv1_INODE)ExAllocatePoolWithTag(
					PagedPool,
					DINODE1_SIZE, FFS_POOL_TAG);

			if (dinode1 == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			RtlZeroMemory(Buffer, Length);

			if (Fcb->dinode1->di_size <= FileIndex)
			{
				Status = STATUS_NO_MORE_FILES;
				_SEH2_LEAVE;
			}
		}
		else
		{
			dinode2 = (PFFSv2_INODE)ExAllocatePoolWithTag(
					PagedPool,
					DINODE2_SIZE, FFS_POOL_TAG);

			if (dinode2 == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}

			RtlZeroMemory(Buffer, Length);

			if (Fcb->dinode2->di_size <= FileIndex)
			{
				Status = STATUS_NO_MORE_FILES;
				_SEH2_LEAVE;
			}
		}

		pDir = ExAllocatePoolWithTag(PagedPool,
				sizeof(FFS_DIR_ENTRY), FFS_POOL_TAG);
		if (!pDir)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}


		if (FS_VERSION == 1)
		{
			dwBytes = 0;
			dwSize = (ULONG)Fcb->dinode1->di_size - FileIndex -
				(sizeof(FFS_DIR_ENTRY) - FFS_NAME_LEN + 1);

			ByteOffset = FileIndex;

			dwTemp = 0;

			while (bRun && UsedLength < Length  && dwBytes < dwSize)
			{
				OEM_STRING  OemName;

				RtlZeroMemory(pDir, sizeof(FFS_DIR_ENTRY));

				Status = FFSv1ReadInode(
							NULL,
							Vcb,
							Fcb->dinode1,
							ByteOffset,
							(PVOID)pDir,
							sizeof(FFS_DIR_ENTRY),
							&dwReturn);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (!pDir->d_ino)
				{
					if (pDir->d_reclen == 0)
					{
						FFSBreakPoint();
						_SEH2_LEAVE;
					}

					goto ProcessNextEntryv1;
				}

				OemName.Buffer = pDir->d_name;
				OemName.Length = (pDir->d_namlen & 0xff);
				OemName.MaximumLength = OemName.Length;

#if 0
/*
				//
				// We could not filter the files: "." and ".."
				//

				if ((OemName.Length >) 1 && OemName.Buffer[0] == '.')
				{
					if ( OemName.Length == 2 && OemName.Buffer[1] == '.')
					{
					}
					else
					{
						goto ProcessNextEntry1;
					}
				}
*/
#endif

				InodeFileNameLength = (USHORT)
					RtlOemStringToUnicodeSize(&OemName);

				InodeFileName.Length = 0;
				InodeFileName.MaximumLength = InodeFileNameLength + 2;

				if (InodeFileNameLength <= 0)
				{
					break;
				}

				InodeFileName.Buffer = ExAllocatePoolWithTag(
						PagedPool,
						InodeFileNameLength + 2, FFS_POOL_TAG);

				if (!InodeFileName.Buffer)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
					_SEH2_LEAVE;
				}

				RtlZeroMemory(
						InodeFileName.Buffer, 
						InodeFileNameLength + 2);

				Status = FFSOEMToUnicode(&InodeFileName,
						&OemName);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (FsRtlDoesNameContainWildCards(
							&(Ccb->DirectorySearchPattern)) ?
						FsRtlIsNameInExpression(
							&(Ccb->DirectorySearchPattern),
							&InodeFileName,
							TRUE,
							NULL) :
						!RtlCompareUnicodeString(
							&(Ccb->DirectorySearchPattern),
							&InodeFileName,
							TRUE))
				{
					dwReturn = FFSProcessDirEntry(
							Vcb, FileInformationClass,
							pDir->d_ino,
							Buffer,
							UsedLength, 
							Length - UsedLength,
							(FileIndex + dwBytes),
							&InodeFileName,
							ReturnSingleEntry);

					if (dwReturn <= 0)
					{
						bRun = FALSE;
					}
					else
					{
						dwTemp = UsedLength;
						UsedLength += dwReturn;
					}
				}

				if (InodeFileName.Buffer != NULL)
				{
					ExFreePool(InodeFileName.Buffer);
					InodeFileName.Buffer = NULL;
				}

ProcessNextEntryv1:

				if (bRun)
				{
					dwBytes +=pDir->d_reclen;
					Ccb->CurrentByteOffset = FileIndex + dwBytes;
				}

				if (UsedLength && ReturnSingleEntry)
				{
					Status = STATUS_SUCCESS;
					_SEH2_LEAVE;
				}

				ByteOffset = FileIndex + dwBytes;
			}
		}
		else
		{
			dwBytes = 0;
			dwSize = (ULONG)Fcb->dinode2->di_size - FileIndex -
				(sizeof(FFS_DIR_ENTRY) - FFS_NAME_LEN + 1);

			ByteOffset = FileIndex;

			dwTemp = 0;

			while (bRun && UsedLength < Length  && dwBytes < dwSize)
			{
				OEM_STRING  OemName;

				RtlZeroMemory(pDir, sizeof(FFS_DIR_ENTRY));

				Status = FFSv2ReadInode(
							NULL,
							Vcb,
							Fcb->dinode2,
							ByteOffset,
							(PVOID)pDir,
							sizeof(FFS_DIR_ENTRY),
							&dwReturn);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (!pDir->d_ino)
				{
					if (pDir->d_reclen == 0)
					{
						FFSBreakPoint();
						_SEH2_LEAVE;
					}

					goto ProcessNextEntryv2;
				}

				OemName.Buffer = pDir->d_name;
				OemName.Length = (pDir->d_namlen & 0xff);
				OemName.MaximumLength = OemName.Length;
#if 0
/*
				//
				// We could not filter the files: "." and ".."
				//

				if ((OemName.Length >) 1 && OemName.Buffer[0] == '.')
				{
					if ( OemName.Length == 2 && OemName.Buffer[1] == '.')
					{
					}
					else
					{
						goto ProcessNextEntry2;
					}
				}
*/
#endif

				InodeFileNameLength = (USHORT)
					RtlOemStringToUnicodeSize(&OemName);

				InodeFileName.Length = 0;
				InodeFileName.MaximumLength = InodeFileNameLength + 2;

				if (InodeFileNameLength <= 0)
				{
					break;
				}

				InodeFileName.Buffer = ExAllocatePoolWithTag(
						PagedPool,
						InodeFileNameLength + 2, FFS_POOL_TAG);

				if (!InodeFileName.Buffer)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
					_SEH2_LEAVE;
				}

				RtlZeroMemory(
						InodeFileName.Buffer, 
						InodeFileNameLength + 2);

				Status = FFSOEMToUnicode(&InodeFileName,
						&OemName);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (FsRtlDoesNameContainWildCards(
							&(Ccb->DirectorySearchPattern)) ?
						FsRtlIsNameInExpression(
							&(Ccb->DirectorySearchPattern),
							&InodeFileName,
							TRUE,
							NULL) :
						!RtlCompareUnicodeString(
							&(Ccb->DirectorySearchPattern),
							&InodeFileName,
							TRUE))
				{
					dwReturn = FFSProcessDirEntry(
							Vcb, FileInformationClass,
							pDir->d_ino,
							Buffer,
							UsedLength, 
							Length - UsedLength,
							(FileIndex + dwBytes),
							&InodeFileName,
							ReturnSingleEntry);

					if (dwReturn <= 0)
					{
						bRun = FALSE;
					}
					else
					{
						dwTemp = UsedLength;
						UsedLength += dwReturn;
					}
				}

				if (InodeFileName.Buffer != NULL)
				{
					ExFreePool(InodeFileName.Buffer);
					InodeFileName.Buffer = NULL;
				}

ProcessNextEntryv2:

				if (bRun)
				{
					dwBytes +=pDir->d_reclen;
					Ccb->CurrentByteOffset = FileIndex + dwBytes;
				}

				if (UsedLength && ReturnSingleEntry)
				{
					Status = STATUS_SUCCESS;
					_SEH2_LEAVE;
				}

				ByteOffset = FileIndex + dwBytes;
			}
		}

		FileIndex += dwBytes;

		((PULONG)((PUCHAR)Buffer + dwTemp)) [0] = 0;

		if (!UsedLength)
		{
			if (FirstQuery)
			{
				Status = STATUS_NO_SUCH_FILE;
			}
			else
			{
				Status = STATUS_NO_MORE_FILES;
			}
		}
		else
		{
			Status = STATUS_SUCCESS;
		}
	}

	_SEH2_FINALLY
	{

		if (FcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (FS_VERSION == 1)
		{
			if (dinode1 != NULL)
			{
				ExFreePool(dinode1);
			}
		}
		else
		{
			if (dinode2 != NULL)
			{
				ExFreePool(dinode2);
			}
		}

		if (pDir != NULL)
		{
			ExFreePool(pDir);
			pDir = NULL;
		}

		if (InodeFileName.Buffer != NULL)
		{
			ExFreePool(InodeFileName.Buffer);
		}

		if (!IrpContext->ExceptionInProgress)
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
				IrpContext->Irp->IoStatus.Information = UsedLength;
				FFSCompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSNotifyChangeDirectory(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT      DeviceObject;
	BOOLEAN             CompleteRequest;
	NTSTATUS            Status = STATUS_UNSUCCESSFUL;
	PFFS_VCB            Vcb;
	PFILE_OBJECT        FileObject;
	PFFS_FCB            Fcb = 0;
	PIRP                Irp;
	PIO_STACK_LOCATION  IrpSp;
	ULONG               CompletionFilter;
	BOOLEAN             WatchTree;

	BOOLEAN             bFcbAcquired = FALSE;

	PUNICODE_STRING     FullName;

    PAGED_CODE();

	_SEH2_TRY
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		//
		//  Always set the wait flag in the Irp context for the original request.
		//

		SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

		DeviceObject = IrpContext->DeviceObject;

		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_DEVICE_REQUEST;
			_SEH2_LEAVE;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		FileObject = IrpContext->FileObject;

		Fcb = (PFFS_FCB)FileObject->FsContext;

		ASSERT(Fcb);

		if (Fcb->Identifier.Type == FFSVCB)
		{
			FFSBreakPoint();
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		ASSERT((Fcb->Identifier.Type == FFSFCB) &&
				(Fcb->Identifier.Size == sizeof(FFS_FCB)));

		if (!IsDirectory(Fcb))
		{
			FFSBreakPoint();
			CompleteRequest = TRUE;
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		if (ExAcquireResourceExclusiveLite(
					&Fcb->MainResource,
					TRUE))
		{
			bFcbAcquired = TRUE;
		}
		else
		{
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}

		Irp = IrpContext->Irp;

		IrpSp = IoGetCurrentIrpStackLocation(Irp);

#if !defined(_GNU_NTIFS_) || defined(__REACTOS__)

		CompletionFilter =
			IrpSp->Parameters.NotifyDirectory.CompletionFilter;

#else // _GNU_NTIFS_

		CompletionFilter = ((PEXTENDED_IO_STACK_LOCATION)
				IrpSp)->Parameters.NotifyDirectory.CompletionFilter;

#endif // _GNU_NTIFS_

		WatchTree = IsFlagOn(IrpSp->Flags, SL_WATCH_TREE);

		if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
		{
			Status = STATUS_DELETE_PENDING;
			_SEH2_LEAVE;
		}

		FullName = &Fcb->LongName;

		if (FullName->Buffer == NULL)
		{
			if (!FFSGetFullFileName(Fcb->FFSMcb, FullName))
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				_SEH2_LEAVE;
			}
		}

		FsRtlNotifyFullChangeDirectory(Vcb->NotifySync,
				&Vcb->NotifyList,
				FileObject->FsContext2,
				(PSTRING)FullName,
				WatchTree,
				FALSE,
				CompletionFilter,
				Irp,
				NULL,
				NULL);

		CompleteRequest = FALSE;

		Status = STATUS_PENDING;

		/*
		   Currently the driver is read-only but here is an example on how to use the
		   FsRtl-functions to report a change:

		   ANSI_STRING TestString;
		   USHORT      FileNamePartLength;

		   RtlInitAnsiString(&TestString, "\\ntifs.h");

		   FileNamePartLength = 7;

		   FsRtlNotifyReportChange(
		   Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
		   &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
		   &TestString,                // PSTRING      FullTargetName
		   &FileNamePartLength,        // PUSHORT      FileNamePartLength
		   FILE_NOTIFY_CHANGE_NAME     // ULONG        FilterMatch
		   );

		   or

		   ANSI_STRING TestString;

		   RtlInitAnsiString(&TestString, "\\ntifs.h");

		   FsRtlNotifyFullReportChange(
		   Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
		   &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
		   &TestString,                // PSTRING      FullTargetName
		   1,                          // USHORT       TargetNameOffset
		   NULL,                       // PSTRING      StreamName OPTIONAL
		   NULL,                       // PSTRING      NormalizedParentName OPTIONAL
		   FILE_NOTIFY_CHANGE_NAME,    // ULONG        FilterMatch
		   0,                          // ULONG        Action
		   NULL                        // PVOID        TargetContext
		   );
		   */

	}
	_SEH2_FINALLY
	{
		if (bFcbAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Fcb->MainResource,
					ExGetCurrentResourceThread());
		}

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


VOID
FFSNotifyReportChange(
	IN PFFS_IRP_CONTEXT  IrpContext,
	IN PFFS_VCB          Vcb,
	IN PFFS_FCB          Fcb,
	IN ULONG             Filter,
	IN ULONG             Action)
{
	PUNICODE_STRING FullName;
	USHORT          Offset;

	FullName = &Fcb->LongName;

	// ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	if (FullName->Buffer == NULL)
	{
		if (!FFSGetFullFileName(Fcb->FFSMcb, FullName))
		{
			/*Status = STATUS_INSUFFICIENT_RESOURCES;*/
			return;
		}
	}

	Offset = (USHORT)(FullName->Length - 
			Fcb->FFSMcb->ShortName.Length);

	FsRtlNotifyFullReportChange(Vcb->NotifySync,
			&(Vcb->NotifyList),
			(PSTRING)(FullName),
			(USHORT)Offset,
			(PSTRING)NULL,
			(PSTRING)NULL,
			(ULONG)Filter,
			(ULONG)Action,
			(PVOID)NULL);

	// ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSDirectoryControl(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS Status;

    PAGED_CODE();

	ASSERT(IrpContext);

	ASSERT((IrpContext->Identifier.Type == FFSICX) &&
			(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

	switch (IrpContext->MinorFunction)
	{
		case IRP_MN_QUERY_DIRECTORY:
			Status = FFSQueryDirectory(IrpContext);
			break;

		case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
			Status = FFSNotifyChangeDirectory(IrpContext);
			break;

		default:
			Status = STATUS_INVALID_DEVICE_REQUEST;
			FFSCompleteIrpContext(IrpContext, Status);
	}

	return Status;
}


BOOLEAN
FFSIsDirectoryEmpty(
	PFFS_VCB Vcb,
	PFFS_FCB Dcb)
{
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;

	PFFS_DIR_ENTRY          pTarget = NULL;

	ULONG                   dwBytes = 0;
	ULONG                   dwRet;

	BOOLEAN                 bRet = TRUE;

    PAGED_CODE();

	if (!IsFlagOn(Dcb->FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;

	_SEH2_TRY
	{
		pTarget = (PFFS_DIR_ENTRY)ExAllocatePoolWithTag(PagedPool,
				FFS_DIR_REC_LEN(FFS_NAME_LEN), FFS_POOL_TAG);
		if (!pTarget)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		dwBytes = 0;


		while ((LONGLONG)dwBytes < Dcb->Header.AllocationSize.QuadPart)
		{
			RtlZeroMemory(pTarget, FFS_DIR_REC_LEN(FFS_NAME_LEN));

			if (FS_VERSION == 1)
			{
				Status = FFSv1ReadInode(
						NULL,
						Vcb,
						Dcb->dinode1,
						dwBytes,
						(PVOID)pTarget,
						FFS_DIR_REC_LEN(FFS_NAME_LEN),
						&dwRet);
			}
			else
			{
				Status = FFSv2ReadInode(
						NULL,
						Vcb,
						Dcb->dinode2,
						dwBytes,
						(PVOID)pTarget,
						FFS_DIR_REC_LEN(FFS_NAME_LEN),
						&dwRet);
			}

			if (!NT_SUCCESS(Status))
			{
				FFSPrint((DBG_ERROR, "FFSRemoveEntry: Reading Directory Content error.\n"));
				_SEH2_LEAVE;
			}

			if (pTarget->d_ino)
			{
				if (pTarget->d_namlen == 1 && pTarget->d_name[0] == '.')
				{
				}
				else if (pTarget->d_namlen == 2 && pTarget->d_name[0] == '.' && 
						pTarget->d_name[1] == '.')
				{
				}
				else
				{
					bRet = FALSE;
					break;
				}
			}
			else
			{
				break;
			}

			dwBytes += pTarget->d_reclen;
		}
	}

	_SEH2_FINALLY
	{
		if (pTarget != NULL)
		{
			ExFreePool(pTarget);
		}
	} _SEH2_END;

	return bRet;
}
