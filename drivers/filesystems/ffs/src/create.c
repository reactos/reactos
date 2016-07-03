/* 
 * FFS File System Driver for Windows
 *
 * create.c
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
#pragma alloc_text(PAGE, FFSv1LookupFileName)
#pragma alloc_text(PAGE, FFSv2LookupFileName)
#pragma alloc_text(PAGE, FFSSearchFcbList)
#pragma alloc_text(PAGE, FFSv1ScanDir)
#pragma alloc_text(PAGE, FFSv2ScanDir)
#pragma alloc_text(PAGE, FFSCreateFile)
#pragma alloc_text(PAGE, FFSCreateVolume)
#pragma alloc_text(PAGE, FFSCreate)
#pragma alloc_text(PAGE, FFSCreateInode)
#pragma alloc_text(PAGE, FFSSupersedeOrOverWriteFile)
#endif


NTSTATUS
FFSv1LookupFileName(
	IN PFFS_VCB            Vcb,
	IN PUNICODE_STRING     FullFileName,
	IN PFFS_MCB            ParentMcb,
	OUT PFFS_MCB*          FFSMcb,
	IN OUT PFFSv1_INODE    dinode1)
{
	NTSTATUS        Status;
	UNICODE_STRING  FileName;
	PFFS_MCB        Mcb = 0;

	FFS_DIR_ENTRY   ffs_dir;
	int             i = 0;
	BOOLEAN         bRun = TRUE;
	BOOLEAN         bParent = FALSE;
	FFSv1_INODE     in;
	ULONG           off = 0;

    PAGED_CODE();

	Status = STATUS_OBJECT_NAME_NOT_FOUND;

	*FFSMcb = NULL;

	if (ParentMcb)
	{
		bParent = TRUE;
	}
	else if (FullFileName->Buffer[0] == L'\\')
	{
		ParentMcb = Vcb->McbTree;
	}
	else
	{
		return STATUS_OBJECT_PATH_NOT_FOUND;
	}

	RtlZeroMemory(&ffs_dir, sizeof(FFS_DIR_ENTRY));

	if (FullFileName->Length == 0)
	{
		return Status;
	}

	if (FullFileName->Length == 2 && FullFileName->Buffer[0] == L'\\')
	{
		if (!FFSv1LoadInode(Vcb, ParentMcb->Inode, dinode1))
		{
			return Status;      
		}

		*FFSMcb = Vcb->McbTree;

		return STATUS_SUCCESS;
	}

	while (bRun && i < FullFileName->Length / 2)
	{
		int Length;
		ULONG FileAttr = FILE_ATTRIBUTE_NORMAL;

		if (bParent)
		{
			bParent = FALSE;
		}
		else
		{
			while(i < FullFileName->Length / 2 && FullFileName->Buffer[i] == L'\\') i++;
		}

		Length = i;

		while(i < FullFileName->Length / 2 && (FullFileName->Buffer[i] != L'\\')) i++;

		if (i - Length > 0)
		{
			FileName = *FullFileName;
			FileName.Buffer += Length;
			FileName.Length = (USHORT)((i - Length) * 2);

			Mcb = FFSSearchMcb(Vcb, ParentMcb, &FileName);

			if (Mcb)
			{
				ParentMcb = Mcb;

				Status = STATUS_SUCCESS;

				if (!IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
				{
					if (i < FullFileName->Length / 2)
					{
						Status = STATUS_OBJECT_PATH_NOT_FOUND;
					}

					break;
				}
			}
			else
			{
				if (!FFSv1LoadInode(Vcb, ParentMcb->Inode, &in))
				{
					Status = STATUS_OBJECT_NAME_NOT_FOUND;
					break;
				}

				if ((in.di_mode & IFMT) != IFDIR)
				{
					if (i < FullFileName->Length / 2)
					{
						Status =  STATUS_OBJECT_NAME_NOT_FOUND;
					}
					break;
				}

				Status = FFSv1ScanDir(
						Vcb,
						ParentMcb,
						&FileName,
						&off,
						&in,
						&ffs_dir);

				if (!NT_SUCCESS(Status))
				{
					bRun = FALSE;
					/*
					   if (i >= FullFileName->Length/2)
					   {
					 *FFSMcb = ParentMcb;
					 }
					 */
				}
				else
				{
#if 0
					if (IsFlagOn(SUPER_BLOCK->s_feature_incompat, 
								FFS_FEATURE_INCOMPAT_FILETYPE))
					{
						if (ffs_dir.d_type == FFS_FT_DIR)
							SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
					}
					else
#endif
					{
						if (!FFSv1LoadInode(Vcb, ffs_dir.d_ino, &in))
						{
							Status = STATUS_OBJECT_NAME_NOT_FOUND;
							break;
						}

						if ((in.di_mode & IFMT) == IFDIR)
						{
							SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
						}
					}

					SetFlag(ParentMcb->Flags, MCB_IN_USE);
					Mcb = FFSAllocateMcb(Vcb, &FileName, FileAttr);
					ClearFlag(ParentMcb->Flags, MCB_IN_USE);

					if (!Mcb)
					{
						Status = STATUS_OBJECT_NAME_NOT_FOUND;
						break;
					}

					Mcb->Inode = ffs_dir.d_ino;
					Mcb->DeOffset = off;
					FFSAddMcbNode(Vcb, ParentMcb, Mcb);
					ParentMcb = Mcb;
				}
			}
		}
		else
		{
			break;
		}
	}

	if (NT_SUCCESS(Status))
	{
		*FFSMcb = Mcb;
		if (dinode1)
		{
			if (!FFSv1LoadInode(Vcb, Mcb->Inode, dinode1))
			{
				FFSPrint((DBG_ERROR, "FFSv1LookupFileName: error loading Inode %xh\n",
							Mcb->Inode));
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}

	return Status;
}


NTSTATUS
FFSv2LookupFileName(
	IN PFFS_VCB            Vcb,
	IN PUNICODE_STRING     FullFileName,
	IN PFFS_MCB            ParentMcb,
	OUT PFFS_MCB*          FFSMcb,
	IN OUT PFFSv2_INODE    dinode2)
{
	NTSTATUS        Status;
	UNICODE_STRING  FileName;
	PFFS_MCB        Mcb = 0;

	FFS_DIR_ENTRY   ffs_dir;
	int             i = 0;
	BOOLEAN         bRun = TRUE;
	BOOLEAN         bParent = FALSE;
	FFSv2_INODE     in;
	ULONG           off = 0;

    PAGED_CODE();

	Status = STATUS_OBJECT_NAME_NOT_FOUND;

	*FFSMcb = NULL;

	if (ParentMcb)
	{
		bParent = TRUE;
	}
	else if (FullFileName->Buffer[0] == L'\\')
	{
		ParentMcb = Vcb->McbTree;
	}
	else
	{
		return STATUS_OBJECT_PATH_NOT_FOUND;
	}

	RtlZeroMemory(&ffs_dir, sizeof(FFS_DIR_ENTRY));

	if (FullFileName->Length == 0)
	{
		return Status;
	}

	if (FullFileName->Length == 2 && FullFileName->Buffer[0] == L'\\')
	{
		if (!FFSv2LoadInode(Vcb, ParentMcb->Inode, dinode2))
		{
			return Status;      
		}

		*FFSMcb = Vcb->McbTree;

		return STATUS_SUCCESS;
	}

	while (bRun && i < FullFileName->Length / 2)
	{
		int Length;
		ULONG FileAttr = FILE_ATTRIBUTE_NORMAL;

		if (bParent)
		{
			bParent = FALSE;
		}
		else
		{
			while(i < FullFileName->Length / 2 && FullFileName->Buffer[i] == L'\\') i++;
		}

		Length = i;

		while(i < FullFileName->Length / 2 && (FullFileName->Buffer[i] != L'\\')) i++;

		if (i - Length > 0)
		{
			FileName = *FullFileName;
			FileName.Buffer += Length;
			FileName.Length = (USHORT)((i - Length) * 2);

			Mcb = FFSSearchMcb(Vcb, ParentMcb, &FileName);

			if (Mcb)
			{
				ParentMcb = Mcb;

				Status = STATUS_SUCCESS;

				if (!IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
				{
					if (i < FullFileName->Length / 2)
					{
						Status = STATUS_OBJECT_PATH_NOT_FOUND;
					}

					break;
				}
			}
			else
			{
				if (!FFSv2LoadInode(Vcb, ParentMcb->Inode, &in))
				{
					Status = STATUS_OBJECT_NAME_NOT_FOUND;
					break;
				}

				if ((in.di_mode & IFMT) != IFDIR)
				{
					if (i < FullFileName->Length / 2)
					{
						Status =  STATUS_OBJECT_NAME_NOT_FOUND;
					}
					break;
				}

				Status = FFSv2ScanDir(
						Vcb,
						ParentMcb,
						&FileName,
						&off,
						&in,
						&ffs_dir);

				if (!NT_SUCCESS(Status))
				{
					bRun = FALSE;
					/*
					   if (i >= FullFileName->Length/2)
					   {
					 *FFSMcb = ParentMcb;
					 }
					 */
				}
				else
				{
#if 0
					if (IsFlagOn(SUPER_BLOCK->s_feature_incompat, 
								FFS_FEATURE_INCOMPAT_FILETYPE))
					{
						if (ffs_dir.d_type == FFS_FT_DIR)
							SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
					}
					else
#endif
					{
						if (!FFSv2LoadInode(Vcb, ffs_dir.d_ino, &in))
						{
							Status = STATUS_OBJECT_NAME_NOT_FOUND;
							break;
						}

						if ((in.di_mode & IFMT) == IFDIR)
						{
							SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
						}
					}

					SetFlag(ParentMcb->Flags, MCB_IN_USE);
					Mcb = FFSAllocateMcb(Vcb, &FileName, FileAttr);
					ClearFlag(ParentMcb->Flags, MCB_IN_USE);

					if (!Mcb)
					{
						Status = STATUS_OBJECT_NAME_NOT_FOUND;
						break;
					}

					Mcb->Inode = ffs_dir.d_ino;
					Mcb->DeOffset = off;
					FFSAddMcbNode(Vcb, ParentMcb, Mcb);
					ParentMcb = Mcb;
				}
			}
		}
		else
		{
			break;
		}
	}

	if (NT_SUCCESS(Status))
	{
		*FFSMcb = Mcb;
		if (dinode2)
		{
			if (!FFSv2LoadInode(Vcb, Mcb->Inode, dinode2))
			{
				FFSPrint((DBG_ERROR, "FFSv2LookupFileName: error loading Inode %xh\n",
							Mcb->Inode));
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}

	return Status;
}


NTSTATUS
FFSv1ScanDir(
	IN PFFS_VCB            Vcb,
	IN PFFS_MCB            ParentMcb,
	IN PUNICODE_STRING     FileName,
	IN OUT PULONG          Index,
	IN PFFSv1_INODE        dinode1,
	IN OUT PFFS_DIR_ENTRY  ffs_dir)
{
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	USHORT                  InodeFileNameLength;
	UNICODE_STRING          InodeFileName;

	PFFS_DIR_ENTRY          pDir = NULL;
	ULONG                   dwBytes = 0;
	BOOLEAN                 bFound = FALSE;
	LONGLONG                Offset = 0;
#ifndef __REACTOS__
    ULONG                   inode = ParentMcb->Inode;
#endif
	ULONG                   dwRet;

    PAGED_CODE();

	_SEH2_TRY
	{

		pDir = (PFFS_DIR_ENTRY)ExAllocatePoolWithTag(PagedPool,
				sizeof(FFS_DIR_ENTRY), FFS_POOL_TAG);
		if (!pDir)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		InodeFileName.Buffer = ExAllocatePoolWithTag(
				PagedPool,
				(FFS_NAME_LEN + 1) * 2, FFS_POOL_TAG);

		if (!InodeFileName.Buffer)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		dwBytes = 0;

		while (!bFound && dwBytes < dinode1->di_size)
		{
			RtlZeroMemory(pDir, sizeof(FFS_DIR_ENTRY));

			// Reading the DCB contents
			Status = FFSv1ReadInode(
					NULL,
					Vcb,
					dinode1,
					dwBytes,
					(PVOID)pDir,
					sizeof(FFS_DIR_ENTRY),
					&dwRet);


			if (!NT_SUCCESS(Status))
			{
				FFSPrint((DBG_ERROR, "FFSv1ScanDir: Reading Directory Content error.\n"));
				_SEH2_LEAVE;
			}

			if (pDir->d_ino /*&& (pDir->d_ino < INODES_COUNT)*/)
			{
				OEM_STRING OemName;
				OemName.Buffer = pDir->d_name;
				OemName.Length = (pDir->d_namlen & 0xff);
				OemName.MaximumLength = OemName.Length;

				InodeFileNameLength = (USHORT)
					RtlOemStringToCountedUnicodeSize(&OemName);

				InodeFileName.Length = 0;
				InodeFileName.MaximumLength = (FFS_NAME_LEN + 1) * 2;

				RtlZeroMemory(InodeFileName.Buffer,
						InodeFileNameLength + 2);

				Status = FFSOEMToUnicode(
						&InodeFileName,
						&OemName);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (!RtlCompareUnicodeString(
							FileName,
							&InodeFileName,
							TRUE))
				{
					bFound = TRUE;
					*Index = dwBytes;
					RtlCopyMemory(ffs_dir, pDir, pDir->d_reclen > sizeof(FFS_DIR_ENTRY)
							? sizeof(FFS_DIR_ENTRY) : pDir->d_reclen);
					Status = STATUS_SUCCESS;

					FFSPrint((DBG_INFO, "FFSv1ScanDir: Found: Name=%S Inode=%xh\n", InodeFileName.Buffer, pDir->d_ino));
				}

				dwBytes +=pDir->d_reclen;
				Offset = (LONGLONG)dwBytes;
			}
			else
			{
				if (pDir->d_ino == 0)
				{
					if (pDir->d_reclen == 0)
					{
						FFSBreakPoint();
						break;
					}
					else
					{
						dwBytes +=pDir->d_reclen;
						Offset = (LONGLONG)dwBytes;
					}
				}
				else
				{
					break;
				}
			}
		}

		if (!bFound)
		{
			Status = STATUS_NO_SUCH_FILE;
		}
	}

	_SEH2_FINALLY
	{
		if (InodeFileName.Buffer != NULL)
		{
			ExFreePool(InodeFileName.Buffer);
		}

		if (pDir)
			ExFreePool(pDir);
	} _SEH2_END;

	return Status;
}


NTSTATUS
FFSv2ScanDir(
	IN PFFS_VCB            Vcb,
	IN PFFS_MCB            ParentMcb,
	IN PUNICODE_STRING     FileName,
	IN OUT PULONG          Index,
	IN PFFSv2_INODE        dinode2,
	IN OUT PFFS_DIR_ENTRY  ffs_dir)
{
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	USHORT                  InodeFileNameLength;
	UNICODE_STRING          InodeFileName;

	PFFS_DIR_ENTRY          pDir = NULL;
	ULONG                   dwBytes = 0;
	BOOLEAN                 bFound = FALSE;
	LONGLONG                Offset = 0;
#ifndef __REACTOS__
    ULONG                   inode = ParentMcb->Inode;
#endif
	ULONG                   dwRet;

    PAGED_CODE();

	_SEH2_TRY
	{

		pDir = (PFFS_DIR_ENTRY)ExAllocatePoolWithTag(PagedPool,
				sizeof(FFS_DIR_ENTRY), FFS_POOL_TAG);
		if (!pDir)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		InodeFileName.Buffer = ExAllocatePoolWithTag(
				PagedPool,
				(FFS_NAME_LEN + 1) * 2, FFS_POOL_TAG);

		if (!InodeFileName.Buffer)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		dwBytes = 0;

		while (!bFound && dwBytes < dinode2->di_size)
		{
			RtlZeroMemory(pDir, sizeof(FFS_DIR_ENTRY));

			// Reading the DCB contents
			Status = FFSv2ReadInode(
					NULL,
					Vcb,
					dinode2,
					dwBytes,
					(PVOID)pDir,
					sizeof(FFS_DIR_ENTRY),
					&dwRet);


			if (!NT_SUCCESS(Status))
			{
				FFSPrint((DBG_ERROR, "FFSv2ScanDir: Reading Directory Content error.\n"));
				_SEH2_LEAVE;
			}

			if (pDir->d_ino /*&& (pDir->d_ino < INODES_COUNT)*/)
			{
				OEM_STRING OemName;
				OemName.Buffer = pDir->d_name;
				OemName.Length = (pDir->d_namlen & 0xff);
				OemName.MaximumLength = OemName.Length;

				InodeFileNameLength = (USHORT)
					RtlOemStringToCountedUnicodeSize(&OemName);

				InodeFileName.Length = 0;
				InodeFileName.MaximumLength = (FFS_NAME_LEN + 1) * 2;

				RtlZeroMemory(InodeFileName.Buffer,
						InodeFileNameLength + 2);

				Status = FFSOEMToUnicode(
						&InodeFileName,
						&OemName);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}

				if (!RtlCompareUnicodeString(
							FileName,
							&InodeFileName,
							TRUE))
				{
					bFound = TRUE;
					*Index = dwBytes;
					RtlCopyMemory(ffs_dir, pDir, pDir->d_reclen > sizeof(FFS_DIR_ENTRY)
							? sizeof(FFS_DIR_ENTRY) : pDir->d_reclen);
					Status = STATUS_SUCCESS;

					FFSPrint((DBG_VITAL, "FFSv2ScanDir: Found: Name=%S Inode=%xh\n", InodeFileName.Buffer, pDir->d_ino));
				}

				dwBytes +=pDir->d_reclen;
				Offset = (LONGLONG)dwBytes;
			}
			else
			{
				if (pDir->d_ino == 0)
				{
					if (pDir->d_reclen == 0)
					{
						FFSBreakPoint();
						break;
					}
					else
					{
						dwBytes +=pDir->d_reclen;
						Offset = (LONGLONG)dwBytes;
					}
				}
				else
				{
					break;
				}
			}
		}

		if (!bFound)
		{
			Status = STATUS_NO_SUCH_FILE;
		}
	}

	_SEH2_FINALLY
	{
		if (InodeFileName.Buffer != NULL)
		{
			ExFreePool(InodeFileName.Buffer);
		}

		if (pDir)
			ExFreePool(pDir);
	} _SEH2_END;

	return Status;
}


/*
PFFS_FCB
FFSSearchFcbList(
	IN PFFS_VCB     Vcb,
	IN ULONG        inode)
{
	BOOLEAN             bFound = FALSE;
	PLIST_ENTRY         Link;
	PFFS_FCB            TmpFcb;

	Link = Vcb->FcbList.Flink;

	while (!bFound && Link != &Vcb->FcbList)
	{
		TmpFcb = CONTAINING_RECORD(Link, FFS_FCB, Next);

		if (TmpFcb && TmpFcb->Identifier.Type == FCB)
		{
#if DBG
			FFSPrint((DBG_INFO, "FFSSearchFcbList: [%s,%xh]\n", 
				TmpFcb->AnsiFileName.Buffer, TmpFcb->Inode));
#endif          
			if (TmpFcb->Inode == inode)
			{
				FFSPrint((DBG_INFO, "FFSSearchMcb: Found FCB for %xh.\n", inode));
				bFound = TRUE;
			}
		}
		Link = Link->Flink;
	}

	if (bFound)
		return TmpFcb;
	else
		return NULL;

}
*/

__drv_mustHoldCriticalRegion
NTSTATUS
FFSCreateFile(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	NTSTATUS            Status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION  IrpSp;
	PFFS_FCB            Fcb = NULL;
	PFFS_MCB            FFSMcb = NULL;

	PFFS_FCB            ParentFcb = NULL;
	PFFS_MCB            ParentMcb = NULL;

	BOOLEAN             bParentFcbCreated = FALSE;

	PFFS_CCB            Ccb = NULL;
	PFFSv1_INODE        dinode1 = 0;
	PFFSv2_INODE        dinode2 = 0;
	BOOLEAN             VcbResourceAcquired = FALSE;
#ifndef __REACTOS__
    BOOLEAN             bDir = FALSE;
#endif
	BOOLEAN             bFcbAllocated = FALSE;
	BOOLEAN             bCreated = FALSE;
	UNICODE_STRING      FileName;
	PIRP                Irp;

	ULONG               Options;
	ULONG               CreateDisposition;

	BOOLEAN             OpenDirectory;
	BOOLEAN             OpenTargetDirectory;
	BOOLEAN             CreateDirectory;
	BOOLEAN             SequentialOnly;
	BOOLEAN             NoIntermediateBuffering;
	BOOLEAN             IsPagingFile;
	BOOLEAN             DirectoryFile;
	BOOLEAN             NonDirectoryFile;
	BOOLEAN             NoEaKnowledge;
	BOOLEAN             DeleteOnClose;
	BOOLEAN             TemporaryFile;
	BOOLEAN             CaseSensitive;

	ACCESS_MASK         DesiredAccess;
	ULONG               ShareAccess;

    PAGED_CODE();

	Irp = IrpContext->Irp;
	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	Options  = IrpSp->Parameters.Create.Options;

	DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
	OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

	NonDirectoryFile = IsFlagOn(Options, FILE_NON_DIRECTORY_FILE);
	SequentialOnly = IsFlagOn(Options, FILE_SEQUENTIAL_ONLY);
	NoIntermediateBuffering = IsFlagOn(Options, FILE_NO_INTERMEDIATE_BUFFERING);
	NoEaKnowledge = IsFlagOn(Options, FILE_NO_EA_KNOWLEDGE);
	DeleteOnClose = IsFlagOn(Options, FILE_DELETE_ON_CLOSE);

	CaseSensitive = IsFlagOn(IrpSp->Flags, SL_CASE_SENSITIVE);

	TemporaryFile = IsFlagOn(IrpSp->Parameters.Create.FileAttributes,
			FILE_ATTRIBUTE_TEMPORARY);

	CreateDisposition = (Options >> 24) & 0x000000ff;

	IsPagingFile = IsFlagOn(IrpSp->Flags, SL_OPEN_PAGING_FILE);

	CreateDirectory = (BOOLEAN)(DirectoryFile &&
			((CreateDisposition == FILE_CREATE) ||
			 (CreateDisposition == FILE_OPEN_IF)));

	OpenDirectory   = (BOOLEAN)(DirectoryFile &&
			((CreateDisposition == FILE_OPEN) ||
			 (CreateDisposition == FILE_OPEN_IF)));

	DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
	ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

	FileName.Buffer = NULL;

	_SEH2_TRY
	{
		ExAcquireResourceExclusiveLite(
				&Vcb->MainResource, TRUE);

		VcbResourceAcquired = TRUE;

		if (Irp->Overlay.AllocationSize.HighPart) 
		{
			Status = STATUS_INVALID_PARAMETER;
			_SEH2_LEAVE;
		}

		if (FS_VERSION == 1)
		{
			if (!(dinode1 = ExAllocatePoolWithTag(
							PagedPool, DINODE1_SIZE, 'EInd')))
			{
				_SEH2_LEAVE;
			}

			RtlZeroMemory(dinode1, sizeof(DINODE1_SIZE));
		}
		else
		{
			if (!(dinode2 = ExAllocatePoolWithTag(
							PagedPool, DINODE2_SIZE, 'EInd')))
			{
				_SEH2_LEAVE;
			}

			RtlZeroMemory(dinode2, sizeof(DINODE2_SIZE));
		}


		FileName.MaximumLength = IrpSp->FileObject->FileName.MaximumLength;
		FileName.Length = IrpSp->FileObject->FileName.Length;

		FileName.Buffer = ExAllocatePoolWithTag(PagedPool, FileName.MaximumLength, FFS_POOL_TAG);
		if (!FileName.Buffer)
		{   
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}

		RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);
		RtlCopyMemory(FileName.Buffer, IrpSp->FileObject->FileName.Buffer, FileName.Length);

		if (IrpSp->FileObject->RelatedFileObject)
		{
			ParentFcb = (PFFS_FCB)(IrpSp->FileObject->RelatedFileObject->FsContext);
		}

		if ((FileName.Length > sizeof(WCHAR)) &&
				(FileName.Buffer[1] == L'\\') &&
				(FileName.Buffer[0] == L'\\')) {

			FileName.Length -= sizeof(WCHAR);

			RtlMoveMemory(&FileName.Buffer[0],
					&FileName.Buffer[1],
					FileName.Length);

			//
			//  Bad Name if there are still beginning backslashes.
			//

			if ((FileName.Length > sizeof(WCHAR)) &&
					(FileName.Buffer[1] == L'\\') &&
					(FileName.Buffer[0] == L'\\'))
			{

				Status = STATUS_OBJECT_NAME_INVALID;

				_SEH2_LEAVE;
			}
		}

		if (IsFlagOn(Options, FILE_OPEN_BY_FILE_ID))
		{
			Status = STATUS_NOT_IMPLEMENTED;
			_SEH2_LEAVE;
		}

		FFSPrint((DBG_INFO, "FFSCreateFile: %S (NameLen=%xh) Paging=%xh Option: %xh.\n",
					FileName.Buffer, FileName.Length, IsPagingFile, IrpSp->Parameters.Create.Options));

		if (ParentFcb)
		{
			ParentMcb = ParentFcb->FFSMcb;
		}

		if (FS_VERSION == 1)
		{
			Status = FFSv1LookupFileName(
						Vcb,
						&FileName,
						ParentMcb,
						&FFSMcb,
						dinode1);
		}
		else
		{
			Status = FFSv2LookupFileName(
						Vcb,
						&FileName,
						ParentMcb,
						&FFSMcb,
						dinode2);
		}

		if (!NT_SUCCESS(Status))
		{
			UNICODE_STRING  PathName;
			UNICODE_STRING  RealName;
			UNICODE_STRING  RemainName;
#ifndef __REACTOS__
			LONG            i = 0;
#endif

			PathName = FileName;

			FFSPrint((DBG_INFO, "FFSCreateFile: File %S will be created.\n", PathName.Buffer));

			FFSMcb = NULL;

			if (PathName.Buffer[PathName.Length / 2 - 1] == L'\\')
			{
				if (DirectoryFile)
				{
					PathName.Length -= 2;
					PathName.Buffer[PathName.Length / 2] = 0;
				}
				else
				{
					Status = STATUS_NOT_A_DIRECTORY;
					_SEH2_LEAVE;
				}
			}

			if (!ParentMcb)
			{
				if (PathName.Buffer[0] != L'\\')
				{
					Status = STATUS_OBJECT_PATH_NOT_FOUND;
					_SEH2_LEAVE;
				}
				else
				{
					ParentMcb = Vcb->McbTree;
				}
			}

Dissecting:

			FsRtlDissectName(PathName, &RealName, &RemainName);

			if (((RemainName.Length != 0) && (RemainName.Buffer[0] == L'\\')) ||
					(RealName.Length >= 256 * sizeof(WCHAR)))
			{
				Status = STATUS_OBJECT_NAME_INVALID;
				_SEH2_LEAVE;
			}

			if (RemainName.Length != 0)
			{
				PFFS_MCB   RetMcb;

				if (FS_VERSION == 1)
				{
					Status = FFSv1LookupFileName(
								Vcb,
								&RealName,
								ParentMcb,
								&RetMcb,
								dinode1);
				}
				else
				{
					Status = FFSv2LookupFileName(
								Vcb,
								&RealName,
								ParentMcb,
								&RetMcb,
								dinode2);
				}

				if (!NT_SUCCESS(Status))
				{
					Status = STATUS_OBJECT_PATH_NOT_FOUND;
					_SEH2_LEAVE;
				}

				ParentMcb = RetMcb;
				PathName  = RemainName;

				goto Dissecting;
			}

			if (FsRtlDoesNameContainWildCards(&RealName))
			{
				Status = STATUS_OBJECT_NAME_INVALID;
				_SEH2_LEAVE;
			}

			ParentFcb = ParentMcb->FFSFcb;

			if (!ParentFcb)
			{
				if (FS_VERSION == 1)
				{
					PFFSv1_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, 
							DINODE1_SIZE, FFS_POOL_TAG);
					if (!pTmpInode)
					{
						Status = STATUS_INSUFFICIENT_RESOURCES;
						_SEH2_LEAVE;
					}

					if(!FFSv1LoadInode(Vcb, ParentMcb->Inode, pTmpInode))
					{
						Status = STATUS_OBJECT_PATH_NOT_FOUND;
						_SEH2_LEAVE;
					}

					ParentFcb = FFSv1AllocateFcb(Vcb, ParentMcb, pTmpInode);

					if (!ParentFcb)
					{
						ExFreePool(pTmpInode);
						Status = STATUS_INSUFFICIENT_RESOURCES;
						_SEH2_LEAVE;
					}
				}
				else
				{
					PFFSv2_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, 
							DINODE2_SIZE, FFS_POOL_TAG);
					if (!pTmpInode)
					{
						Status = STATUS_INSUFFICIENT_RESOURCES;
						_SEH2_LEAVE;
					}

					if(!FFSv2LoadInode(Vcb, ParentMcb->Inode, pTmpInode))
					{
						Status = STATUS_OBJECT_PATH_NOT_FOUND;
						_SEH2_LEAVE;
					}

					ParentFcb = FFSv2AllocateFcb(Vcb, ParentMcb, pTmpInode);

					if (!ParentFcb)
					{
						ExFreePool(pTmpInode);
						Status = STATUS_INSUFFICIENT_RESOURCES;
						_SEH2_LEAVE;
					}
				}

				bParentFcbCreated = TRUE;
				ParentFcb->ReferenceCount++;
			}

			// We need to create a new one ?
			if ((CreateDisposition == FILE_CREATE) ||
					(CreateDisposition == FILE_OPEN_IF) ||
					(CreateDisposition == FILE_OVERWRITE_IF))
			{
#if FFS_READ_ONLY
				Status = STATUS_MEDIA_WRITE_PROTECTED;
				_SEH2_LEAVE;
#else // !FFS_READ_ONLY
				if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
				{
					Status = STATUS_MEDIA_WRITE_PROTECTED;
					_SEH2_LEAVE;
				}

				if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
				{
					IoSetHardErrorOrVerifyDevice(IrpContext->Irp,
							Vcb->Vpb->RealDevice);
					SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

					FFSRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
				}

				if (DirectoryFile)
				{
					if (TemporaryFile)
					{
						Status = STATUS_INVALID_PARAMETER;
						_SEH2_LEAVE;
					}
				}

				if (!ParentFcb)
				{
					Status = STATUS_OBJECT_PATH_NOT_FOUND;
					_SEH2_LEAVE;
				}

				if (DirectoryFile)
				{
					if (ParentFcb->FFSMcb->Inode == FFS_ROOT_INO) 
					{
						if ((RealName.Length == 0x10) && 
								memcmp(RealName.Buffer, L"Recycled\0", 0x10) == 0)
						{
							SetFlag(IrpSp->Parameters.Create.FileAttributes,
									FILE_ATTRIBUTE_READONLY);
						}
					}

					Status = FFSCreateInode(
								IrpContext,
								Vcb, 
								ParentFcb,
								DT_DIR,
								IrpSp->Parameters.Create.FileAttributes,
								&RealName);
				}
				else
				{
					Status = FFSCreateInode(
								IrpContext,
								Vcb,
								ParentFcb,
								DT_REG,
								IrpSp->Parameters.Create.FileAttributes,
								&RealName);
				}

				if (NT_SUCCESS(Status))
				{
					bCreated = TRUE;

					Irp->IoStatus.Information = FILE_CREATED;

					if (FS_VERSION == 1)
					{
						Status = FFSv1LookupFileName(
									Vcb,
									&RealName,
									ParentMcb,
									&FFSMcb,
									dinode1);
					}
					else
					{
						Status = FFSv2LookupFileName(
									Vcb,
									&RealName,
									ParentMcb,
									&FFSMcb,
									dinode2);
					}

					if (NT_SUCCESS(Status))
					{
						if (DirectoryFile)
						{
							FFSNotifyReportChange(
								IrpContext,
								Vcb,
								ParentFcb,
								FILE_NOTIFY_CHANGE_DIR_NAME,
								FILE_ACTION_ADDED);
						}
						else
						{
							FFSNotifyReportChange(
								IrpContext,
								Vcb,
								ParentFcb,
								FILE_NOTIFY_CHANGE_FILE_NAME,
								FILE_ACTION_ADDED);
						}
					}
					else
					{
						FFSBreakPoint();
					}
				}
				else
				{
					FFSBreakPoint();
				}
#endif // !FFS_READ_ONLY
			}
			else if (OpenTargetDirectory)
			{
				if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
				{
					Status = STATUS_MEDIA_WRITE_PROTECTED;
					_SEH2_LEAVE;
				}

				if (!ParentFcb)
				{
					Status = STATUS_OBJECT_PATH_NOT_FOUND;
					_SEH2_LEAVE;
				}

				RtlZeroMemory(IrpSp->FileObject->FileName.Buffer,
						IrpSp->FileObject->FileName.MaximumLength);
				IrpSp->FileObject->FileName.Length = RealName.Length;

				RtlCopyMemory(IrpSp->FileObject->FileName.Buffer,
						RealName.Buffer,
						RealName.Length);

				Fcb = ParentFcb;

				Irp->IoStatus.Information = FILE_DOES_NOT_EXIST;
				Status = STATUS_SUCCESS;
			}
			else
			{
				Status = STATUS_OBJECT_NAME_NOT_FOUND;
				_SEH2_LEAVE;
			}
		}
		else // File / Dir already exists.
		{
			if (OpenTargetDirectory)
			{
				if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
				{
					Status = STATUS_MEDIA_WRITE_PROTECTED;
					_SEH2_LEAVE;
				}

				Irp->IoStatus.Information = FILE_EXISTS;
				Status = STATUS_SUCCESS;

				RtlZeroMemory(IrpSp->FileObject->FileName.Buffer,
						IrpSp->FileObject->FileName.MaximumLength);
				IrpSp->FileObject->FileName.Length = FFSMcb->ShortName.Length;

				RtlCopyMemory(IrpSp->FileObject->FileName.Buffer,
						FFSMcb->ShortName.Buffer,
						FFSMcb->ShortName.Length);

				//Let Mcb pointer to it's parent
				FFSMcb = FFSMcb->Parent;

				goto Openit;
			}

			// We can not create if one exists
			if (CreateDisposition == FILE_CREATE)
			{
				Irp->IoStatus.Information = FILE_EXISTS;
				Status = STATUS_OBJECT_NAME_COLLISION;
				_SEH2_LEAVE;
			}

			if(IsFlagOn(FFSMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
			{
				if ((CreateDisposition != FILE_OPEN) &&
						(CreateDisposition != FILE_OPEN_IF))
				{

					Status = STATUS_OBJECT_NAME_COLLISION;
					_SEH2_LEAVE;
				}

				if (NonDirectoryFile) 
				{
					Status = STATUS_FILE_IS_A_DIRECTORY;
					_SEH2_LEAVE;
				}

				if (FFSMcb->Inode == FFS_ROOT_INO)
				{
					if (DeleteOnClose)
					{
						Status = STATUS_CANNOT_DELETE;
						_SEH2_LEAVE;
					}

					if (OpenTargetDirectory)
					{
						Status = STATUS_INVALID_PARAMETER;
						_SEH2_LEAVE;
					}
				}
			}

			Irp->IoStatus.Information = FILE_OPENED;
		}

Openit:

		if (FFSMcb)
		{
			Fcb = FFSMcb->FFSFcb;

			if (!Fcb)
			{
				if (FS_VERSION == 1)
				{
					Fcb = FFSv1AllocateFcb(Vcb, FFSMcb, dinode1);
					bFcbAllocated = TRUE;
				}
				else
				{
					Fcb = FFSv2AllocateFcb(Vcb, FFSMcb, dinode2);
					bFcbAllocated = TRUE;
				}
			}
		}

		if (Fcb)
		{
			if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
			{
				Status = STATUS_FILE_DELETED;
				_SEH2_LEAVE;
			}

			if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
			{
				Status = STATUS_DELETE_PENDING;
				_SEH2_LEAVE;
			}

			if (bCreated)
			{
#if !FFS_READ_ONLY
				//
				//  This file is just created.
				//

				if (DirectoryFile)
				{
					UNICODE_STRING EntryName;
					USHORT  NameBuf[6];

					RtlZeroMemory(&NameBuf, 6 * sizeof(USHORT));

					EntryName.Length = EntryName.MaximumLength = 2;
					EntryName.Buffer = &NameBuf[0];
					NameBuf[0] = (USHORT)'.';

					FFSAddEntry(IrpContext, Vcb, Fcb,
							DT_DIR,
							Fcb->FFSMcb->Inode,
							&EntryName);

					if (FS_VERSION == 1)
					{
						FFSv1SaveInode(IrpContext, Vcb,
								Fcb->FFSMcb->Inode,
								Fcb->dinode1);
					}
					else
					{
						FFSv2SaveInode(IrpContext, Vcb,
								Fcb->FFSMcb->Inode,
								Fcb->dinode2);
					}

					EntryName.Length = EntryName.MaximumLength = 4;
					EntryName.Buffer = &NameBuf[0];
					NameBuf[0] = NameBuf[1] = (USHORT)'.';

					FFSAddEntry(IrpContext, Vcb, Fcb,
							DT_DIR,
							Fcb->FFSMcb->Parent->Inode,
							&EntryName);

					if (FS_VERSION == 1)
					{
						FFSv1SaveInode(IrpContext, Vcb,
								Fcb->FFSMcb->Inode,
								Fcb->dinode1);
					}
					else
					{
						FFSv2SaveInode(IrpContext, Vcb,
								Fcb->FFSMcb->Inode,
								Fcb->dinode2);
					}
				}
				else
				{
					if (!FFSExpandFile(
								IrpContext, Vcb, Fcb,
								&(Irp->Overlay.AllocationSize)))
					{
						Status = STATUS_INSUFFICIENT_RESOURCES;
						_SEH2_LEAVE;
					}
				}
#endif // !FFS_READ_ONLY
			}
			else
			{
				//
				//  This file alreayd exists.
				//

				if (DeleteOnClose)
				{
					if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
					{
						Status = STATUS_MEDIA_WRITE_PROTECTED;
						_SEH2_LEAVE;
					}

					if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
					{
						Status = STATUS_MEDIA_WRITE_PROTECTED;

						IoSetHardErrorOrVerifyDevice(IrpContext->Irp,
								Vcb->Vpb->RealDevice);

						SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

						FFSRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
					}

					SetFlag(Fcb->Flags, FCB_DELETE_ON_CLOSE);
				}
				else
				{
					//
					// Just to Open file (Open/OverWrite ...)
					//

					if ((!IsDirectory(Fcb)) && (IsFlagOn(IrpSp->FileObject->Flags,
									FO_NO_INTERMEDIATE_BUFFERING)))
					{
						Fcb->Header.IsFastIoPossible = FastIoIsPossible;

						if (IsFlagOn(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED) &&
								(Fcb->SectionObject.DataSectionObject != NULL))
						{
							if (Fcb->NonCachedOpenCount == Fcb->OpenHandleCount)
							{
								/* IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) */

								if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
								{
									CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
									ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
								}

								CcPurgeCacheSection(&Fcb->SectionObject,
										NULL,
										0,
										FALSE);
							}
						}
					}
				}
			}

			if (!IsDirectory(Fcb))
			{
				if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
				{
					if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile)
					{
						DesiredAccess |= DELETE;
					}
					else if (((CreateDisposition == FILE_OVERWRITE) ||
								(CreateDisposition == FILE_OVERWRITE_IF)) && !IsPagingFile)
					{
						DesiredAccess |= (FILE_WRITE_DATA | FILE_WRITE_EA |
								FILE_WRITE_ATTRIBUTES);
					}
				}
			}

			if (Fcb->OpenHandleCount > 0) 
			{
				Status = IoCheckShareAccess(DesiredAccess,
						ShareAccess,
						IrpSp->FileObject,
						&(Fcb->ShareAccess),
						TRUE);

				if (!NT_SUCCESS(Status))
				{
					_SEH2_LEAVE;
				}
			} 
			else 
			{
				IoSetShareAccess(DesiredAccess,
						ShareAccess,
						IrpSp->FileObject,
						&(Fcb->ShareAccess));
			}

			Ccb = FFSAllocateCcb();

			Fcb->OpenHandleCount++;
			Fcb->ReferenceCount++;

			if (IsFlagOn(IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
			{
				Fcb->NonCachedOpenCount++;
			}

			Vcb->OpenFileHandleCount++;
			Vcb->ReferenceCount++;

			IrpSp->FileObject->FsContext = (void*)Fcb;
			IrpSp->FileObject->FsContext2 = (void*) Ccb;
			IrpSp->FileObject->PrivateCacheMap = NULL;
			IrpSp->FileObject->SectionObjectPointer = &(Fcb->SectionObject);
			IrpSp->FileObject->Vpb = Vcb->Vpb;

			Status = STATUS_SUCCESS;

			FFSPrint((DBG_INFO, "FFSCreateFile: %s OpenCount: %u ReferCount: %u\n",
						Fcb->AnsiFileName.Buffer, Fcb->OpenHandleCount, Fcb->ReferenceCount));

			if (!IsDirectory(Fcb) && !NoIntermediateBuffering)
			{
				IrpSp->FileObject->Flags |= FO_CACHE_SUPPORTED;
			}

			if (!bCreated && !IsDirectory(Fcb))
			{
				if (DeleteOnClose || 
						IsFlagOn(DesiredAccess, FILE_WRITE_DATA) || 
						(CreateDisposition == FILE_OVERWRITE) ||
						(CreateDisposition == FILE_OVERWRITE_IF))
				{
					if (!MmFlushImageSection(&Fcb->SectionObject,
								MmFlushForWrite))
					{

						Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
							STATUS_SHARING_VIOLATION;
						_SEH2_LEAVE;
					}
				}

				if ((CreateDisposition == FILE_SUPERSEDE) ||
						(CreateDisposition == FILE_OVERWRITE) ||
						(CreateDisposition == FILE_OVERWRITE_IF))
				{
#if FFS_READ_ONLY
					Status = STATUS_MEDIA_WRITE_PROTECTED;
					_SEH2_LEAVE;
#else // !FFS_READ_ONLY
					BOOLEAN bRet;

					if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
					{
						Status = STATUS_MEDIA_WRITE_PROTECTED;
						_SEH2_LEAVE;
					}

					if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED))
					{
						IoSetHardErrorOrVerifyDevice(IrpContext->Irp,
								Vcb->Vpb->RealDevice);
						SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

						FFSRaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
					}

					Status = FFSSupersedeOrOverWriteFile(IrpContext,
							Vcb,
							Fcb,
							CreateDisposition);

					if (NT_SUCCESS(Status))
					{
						_SEH2_LEAVE;
					}

					bRet = FFSExpandFile(IrpContext,
							Vcb,
							Fcb,
							&(Irp->Overlay.AllocationSize));

					if (!bRet)
					{
						Status = STATUS_DISK_FULL;
						_SEH2_LEAVE;
					}

					FFSNotifyReportChange(
							IrpContext,
							Vcb,
							Fcb,
							FILE_NOTIFY_CHANGE_LAST_WRITE |
							FILE_NOTIFY_CHANGE_ATTRIBUTES |
							FILE_NOTIFY_CHANGE_SIZE,
							FILE_ACTION_MODIFIED);


					if (CreateDisposition == FILE_SUPERSEDE)
					{
						Irp->IoStatus.Information = FILE_SUPERSEDED;
					}
					else
					{
						Irp->IoStatus.Information = FILE_OVERWRITTEN;
					}
#endif // !FFS_READ_ONLY
				}
			}
		}
	}

	_SEH2_FINALLY
	{
		if (FileName.Buffer)
			ExFreePool(FileName.Buffer);

		if (bParentFcbCreated)
		{
			ParentFcb->ReferenceCount--;
		}

		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
					ExGetCurrentResourceThread());
		}

		if (!bFcbAllocated)
		{
			if (FS_VERSION == 1)
			{
				if (dinode1)
					ExFreePool(dinode1);
			}
			else
			{
				if (dinode2)
					ExFreePool(dinode2);
			}
		}
		else
		{
			if (FS_VERSION == 1)
			{
				if (!Fcb && dinode1)
					ExFreePool(dinode1);
			}
			else
			{
				if (!Fcb && dinode2)
					ExFreePool(dinode2);
			}
		}
	} _SEH2_END;

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSCreateVolume(
	PFFS_IRP_CONTEXT IrpContext,
	PFFS_VCB         Vcb)
{
	PIO_STACK_LOCATION  IrpSp;
	PIRP                Irp;

	NTSTATUS            Status;

	ACCESS_MASK         DesiredAccess;
	ULONG               ShareAccess;

	ULONG               Options;
	BOOLEAN             DirectoryFile;
	BOOLEAN             OpenTargetDirectory;

	ULONG               CreateDisposition;

    PAGED_CODE();

	Irp = IrpContext->Irp;
	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	Options  = IrpSp->Parameters.Create.Options;

	DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
	OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

	CreateDisposition = (Options >> 24) & 0x000000ff;

	DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
	ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

	if (DirectoryFile)
	{
		return STATUS_NOT_A_DIRECTORY;
	}

	if (OpenTargetDirectory)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if ((CreateDisposition != FILE_OPEN) && 
			(CreateDisposition != FILE_OPEN_IF)) 
	{
		return STATUS_ACCESS_DENIED;
	}

	Status = STATUS_SUCCESS;

	if (Vcb->OpenHandleCount > 0) 
	{
		Status = IoCheckShareAccess(DesiredAccess, ShareAccess,
				IrpSp->FileObject,
				&(Vcb->ShareAccess), TRUE);

		if (!NT_SUCCESS(Status)) 
		{
			goto errorout;
		}
	} 
	else 
	{
		IoSetShareAccess(DesiredAccess, ShareAccess,
				IrpSp->FileObject,
				&(Vcb->ShareAccess));
	}

	if (FlagOn(DesiredAccess, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA))
	{
		ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
		FFSFlushFiles(Vcb, FALSE);
		FFSFlushVolume(Vcb, FALSE);
		ExReleaseResourceLite(&Vcb->MainResource);
	}

	{
		PFFS_CCB Ccb = FFSAllocateCcb();

		if (Ccb == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			goto errorout;
		}

		IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
		IrpSp->FileObject->FsContext  = Vcb;
		IrpSp->FileObject->FsContext2 = Ccb;

		Vcb->ReferenceCount++;
		Vcb->OpenHandleCount++;

		Irp->IoStatus.Information = FILE_OPENED;
	}

errorout:

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSCreate(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT      DeviceObject;
	PIRP                Irp;
	PIO_STACK_LOCATION  IrpSp;
	PFFS_VCB            Vcb = 0;
	NTSTATUS            Status = STATUS_OBJECT_NAME_NOT_FOUND;
	PFFS_FCBVCB         Xcb = NULL;

    PAGED_CODE();

	DeviceObject = IrpContext->DeviceObject;

	Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

#if 0
	/* 드라이버가 로드 되었는지 검사할 때 마운트가 되어 있지 않은
	   경우도 있기 때문에 멈추면 안됨. */
	ASSERT(IsMounted(Vcb));
#endif	

	Irp = IrpContext->Irp;

	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	Xcb = (PFFS_FCBVCB)(IrpSp->FileObject->FsContext);

	if (DeviceObject == FFSGlobal->DeviceObject)
	{
		FFSPrint((DBG_INFO, "FFSCreate: Create on main device object.\n"));

		Status = STATUS_SUCCESS;

		Irp->IoStatus.Information = FILE_OPENED;

		FFSUnpinRepinnedBcbs(IrpContext);

		FFSCompleteIrpContext(IrpContext, Status);        

		return Status;

	}

	_SEH2_TRY
	{
		if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
		{
			Status = STATUS_ACCESS_DENIED;

			if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
			{
				Status = STATUS_VOLUME_DISMOUNTED;
			}

			_SEH2_LEAVE;
		}

		if (((IrpSp->FileObject->FileName.Length == 0) &&
					(IrpSp->FileObject->RelatedFileObject == NULL)) || 
				(Xcb && Xcb->Identifier.Type == FFSVCB))
		{
			Status = FFSCreateVolume(IrpContext, Vcb);
		}
		else
		{
			Status = FFSCreateFile(IrpContext, Vcb);
		}
	}

	_SEH2_FINALLY
	{

		if (!IrpContext->ExceptionInProgress)
		{
			FFSUnpinRepinnedBcbs(IrpContext);

			FFSCompleteIrpContext(IrpContext, Status);
		}
	} _SEH2_END;

	return Status;
}

#if !FFS_READ_ONLY

__drv_mustHoldCriticalRegion
NTSTATUS
FFSCreateInode(
	PFFS_IRP_CONTEXT    IrpContext,
	PFFS_VCB            Vcb,
	PFFS_FCB            ParentFcb,
	ULONG               Type,
	ULONG               FileAttr,
	PUNICODE_STRING     FileName)
{
	NTSTATUS    Status;
	ULONG       Inode;
	ULONG       Group;

	FFSv1_INODE dinode1;

    PAGED_CODE();

	RtlZeroMemory(&dinode1, DINODE1_SIZE);

	Group = (ParentFcb->FFSMcb->Inode - 1) / BLOCKS_PER_GROUP;

	FFSPrint((DBG_INFO,
				"FFSCreateInode: %S in %S(Inode=%xh)\n",
				FileName->Buffer, 
				ParentFcb->FFSMcb->ShortName.Buffer, 
				ParentFcb->FFSMcb->Inode));

	Status = FFSNewInode(IrpContext, Vcb, Group,Type, &Inode);

	if (!NT_SUCCESS(Status))
	{
		goto errorout;
	}

	Status = FFSAddEntry(IrpContext, Vcb, ParentFcb, Type, Inode, FileName);

	if (!NT_SUCCESS(Status))
	{
		FFSBreakPoint();
		FFSFreeInode(IrpContext, Vcb, Inode, Type);

		goto errorout;
	}

	FFSv1SaveInode(IrpContext, Vcb, ParentFcb->FFSMcb->Inode, ParentFcb->dinode1);

	dinode1.di_ctime = ParentFcb->dinode1->di_mtime;
	dinode1.di_mode =  0x1FF;

	if (IsFlagOn(FileAttr, FILE_ATTRIBUTE_READONLY))
	{
		FFSSetReadOnly(dinode1.di_mode);
	}

	if (Type == DT_DIR)
	{
		SetFlag(dinode1.di_mode, IFDIR);

		dinode1.di_nlink = 2;
	}
	else
	{
		dinode1.di_nlink = 1;
		SetFlag(dinode1.di_mode, IFLNK);
	}

	FFSv1SaveInode(IrpContext, Vcb, Inode, &dinode1);

	FFSPrint((DBG_INFO, "FFSCreateInode: New Inode = %xh (Type=%xh)\n", Inode, Type));

errorout:

	return Status;
}


__drv_mustHoldCriticalRegion
NTSTATUS
FFSSupersedeOrOverWriteFile(
	IN PFFS_IRP_CONTEXT IrpContext,
	IN PFFS_VCB         Vcb,
	IN PFFS_FCB         Fcb,
	IN ULONG            Disposition)
{
	LARGE_INTEGER   CurrentTime;
	LARGE_INTEGER   AllocationSize;
	NTSTATUS        Status = STATUS_SUCCESS;

	BOOLEAN         bRet = FALSE;

    PAGED_CODE();

	KeQuerySystemTime(&CurrentTime);

	AllocationSize.QuadPart = (LONGLONG)0;

	if (!MmCanFileBeTruncated(&(Fcb->SectionObject), &(AllocationSize))) 
	{
		Status = STATUS_USER_MAPPED_FILE;

		return Status;
	}

	bRet = FFSTruncateFile(IrpContext, Vcb, Fcb, &AllocationSize);

	if (bRet)
	{
		Fcb->Header.AllocationSize.QuadPart = 
			Fcb->Header.FileSize.QuadPart = (LONGLONG)0;

		Fcb->dinode1->di_size = 0;

		if (Disposition == FILE_SUPERSEDE)
			Fcb->dinode1->di_ctime = FFSInodeTime(CurrentTime);

		Fcb->dinode1->di_atime =
			Fcb->dinode1->di_mtime = FFSInodeTime(CurrentTime);
	}
	else
	{
		if (Fcb->dinode1->di_size > Fcb->Header.AllocationSize.LowPart)
			Fcb->dinode1->di_size = Fcb->Header.AllocationSize.LowPart;

		Fcb->Header.FileSize.QuadPart = (LONGLONG)Fcb->dinode1->di_size;

		Status = STATUS_UNSUCCESSFUL;
	}

	FFSv1SaveInode(IrpContext, Vcb, Fcb->FFSMcb->Inode, Fcb->dinode1);

	return Status;
}

#endif // !FFS_READ_ONLY
