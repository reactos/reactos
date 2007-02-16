/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/create.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS *****************************************************************/

void
vfat8Dot3ToString (PFAT_DIR_ENTRY pEntry, PUNICODE_STRING NameU)
{
	OEM_STRING StringA;
	USHORT Length;
	CHAR  cString[12];

	RtlCopyMemory(cString, pEntry->ShortName, 11);
	cString[11] = 0;
	if (cString[0] == 0x05)
	{
		cString[0] = 0xe5;
	}

	StringA.Buffer = cString;
	for (StringA.Length = 0;
	StringA.Length < 8 && StringA.Buffer[StringA.Length] != ' ';
	StringA.Length++);
	StringA.MaximumLength = StringA.Length;

	RtlOemStringToUnicodeString(NameU, &StringA, FALSE);

	if (pEntry->lCase & VFAT_CASE_LOWER_BASE)
	{
		RtlDowncaseUnicodeString(NameU, NameU, FALSE);
	}
	if (cString[8] != ' ')
	{
		Length = NameU->Length;
		NameU->Buffer += Length / sizeof(WCHAR);
		if (!FAT_ENTRY_VOLUME(pEntry))
		{
			Length += sizeof(WCHAR);
			NameU->Buffer[0] = L'.';
			NameU->Buffer++;
		}
		NameU->Length = 0;
		NameU->MaximumLength -= Length;

		StringA.Buffer = &cString[8];
		for (StringA.Length = 0;
		StringA.Length < 3 && StringA.Buffer[StringA.Length] != ' ';
		StringA.Length++);
		StringA.MaximumLength = StringA.Length;
		RtlOemStringToUnicodeString(NameU, &StringA, FALSE);
		if (pEntry->lCase & VFAT_CASE_LOWER_EXT)
		{
			RtlDowncaseUnicodeString(NameU, NameU, FALSE);
		}
		NameU->Buffer -= Length / sizeof(WCHAR);
		NameU->Length += Length;
		NameU->MaximumLength += Length;
	}
	NameU->Buffer[NameU->Length / sizeof(WCHAR)] = 0;
	DPRINT("'%wZ'\n", NameU);
}

NTSTATUS
ReadVolumeLabel (PDEVICE_EXTENSION DeviceExt, PVPB Vpb)
/*
 * FUNCTION: Read the volume label
 */
{
	PVOID Context = NULL;
	ULONG DirIndex = 0;
	PDIR_ENTRY Entry;
	PVFATFCB pFcb;
	LARGE_INTEGER FileOffset;
	UNICODE_STRING NameU;
	ULONG SizeDirEntry;
	ULONG EntriesPerPage;
	OEM_STRING StringO;

	NameU.Buffer = Vpb->VolumeLabel;
	NameU.Length = 0;
	NameU.MaximumLength = sizeof(Vpb->VolumeLabel);
	*(Vpb->VolumeLabel) = 0;
	Vpb->VolumeLabelLength = 0;

	if (DeviceExt->Flags & VCB_IS_FATX)
	{
		SizeDirEntry = sizeof(FATX_DIR_ENTRY);
		EntriesPerPage = FATX_ENTRIES_PER_PAGE;
	}
	else
	{
		SizeDirEntry = sizeof(FAT_DIR_ENTRY);
		EntriesPerPage = FAT_ENTRIES_PER_PAGE;
	}

	ExAcquireResourceExclusiveLite (&DeviceExt->DirResource, TRUE);
	pFcb = vfatOpenRootFCB (DeviceExt);
	ExReleaseResourceLite (&DeviceExt->DirResource);

	FileOffset.QuadPart = 0;
	if (CcMapData(pFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, &Context, (PVOID*)&Entry))
	{
		while (TRUE)
		{
			if (ENTRY_VOLUME(DeviceExt, Entry))
			{
				/* copy volume label */
				if (DeviceExt->Flags & VCB_IS_FATX)
				{
					StringO.Buffer = (PCHAR)Entry->FatX.Filename;
					StringO.MaximumLength = StringO.Length = Entry->FatX.FilenameLength;
					RtlOemStringToUnicodeString(&NameU, &StringO, FALSE);
				}
				else
				{
					vfat8Dot3ToString (&Entry->Fat, &NameU);
				}
				Vpb->VolumeLabelLength = NameU.Length;
				break;
			}
			if (ENTRY_END(DeviceExt, Entry))
			{
				break;
			}
			DirIndex++;
			Entry = (PDIR_ENTRY)((ULONG_PTR)Entry + SizeDirEntry);
			if ((DirIndex % EntriesPerPage) == 0)
			{
				CcUnpinData(Context);
				FileOffset.u.LowPart += PAGE_SIZE;
				if (!CcMapData(pFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, &Context, (PVOID*)&Entry))
				{
					Context = NULL;
					break;
				}
			}
		}
		if (Context)
		{
			CcUnpinData(Context);
		}
	}
	ExAcquireResourceExclusiveLite (&DeviceExt->DirResource, TRUE);
	vfatReleaseFCB (DeviceExt, pFcb);
	ExReleaseResourceLite (&DeviceExt->DirResource);

	return STATUS_SUCCESS;
}

NTSTATUS
FindFile (
	PDEVICE_EXTENSION DeviceExt,
	PVFATFCB Parent,
	PUNICODE_STRING FileToFindU,
	PVFAT_DIRENTRY_CONTEXT DirContext,
	BOOLEAN First)
/*
 * FUNCTION: Find a file
 */
{
	PWCHAR PathNameBuffer;
	USHORT PathNameBufferLength;
	NTSTATUS Status;
	PVOID Context = NULL;
	PVOID Page;
	PVFATFCB rcFcb;
	BOOLEAN Found;
	UNICODE_STRING PathNameU;
	UNICODE_STRING FileToFindUpcase;
	BOOLEAN WildCard;

	DPRINT ("FindFile(Parent %x, FileToFind '%wZ', DirIndex: %d)\n",
		Parent, FileToFindU, DirContext->DirIndex);
	DPRINT ("FindFile: Path %wZ)\n",&Parent->PathNameU);

	PathNameBufferLength = LONGNAME_MAX_LENGTH * sizeof(WCHAR);
	PathNameBuffer = ExAllocatePool(NonPagedPool, PathNameBufferLength + sizeof(WCHAR));
	if (!PathNameBuffer)
	{
		CHECKPOINT1;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	PathNameU.Buffer = PathNameBuffer;
	PathNameU.Length = 0;
	PathNameU.MaximumLength = PathNameBufferLength;

	DirContext->LongNameU.Length = 0;
	DirContext->ShortNameU.Length = 0;

	WildCard = FsRtlDoesNameContainWildCards(FileToFindU);

	if (WildCard == FALSE)
	{
		/* if there is no '*?' in the search name, than look first for an existing fcb */
		RtlCopyUnicodeString(&PathNameU, &Parent->PathNameU);
		if (!vfatFCBIsRoot(Parent))
		{
			PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = L'\\';
			PathNameU.Length += sizeof(WCHAR);
		}
		RtlAppendUnicodeStringToString(&PathNameU, FileToFindU);
		PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = 0;
		rcFcb = vfatGrabFCBFromTable(DeviceExt, &PathNameU);
		if (rcFcb)
		{
			ULONG startIndex = rcFcb->startIndex;
			if ((rcFcb->Flags & FCB_IS_FATX_ENTRY) && !vfatFCBIsRoot(Parent))
			{
				startIndex += 2;
			}
			if(startIndex >= DirContext->DirIndex)
			{
				RtlCopyUnicodeString(&DirContext->LongNameU, &rcFcb->LongNameU);
				RtlCopyUnicodeString(&DirContext->ShortNameU, &rcFcb->ShortNameU);
				RtlCopyMemory(&DirContext->DirEntry, &rcFcb->entry, sizeof(DIR_ENTRY));
				DirContext->StartIndex = rcFcb->startIndex;
				DirContext->DirIndex = rcFcb->dirIndex;
				DPRINT("FindFile: new Name %wZ, DirIndex %d (%d)\n",
					&DirContext->LongNameU, DirContext->DirIndex, DirContext->StartIndex);
				Status = STATUS_SUCCESS;
			}
			else
			{
				CHECKPOINT1;
				Status = STATUS_UNSUCCESSFUL;
			}
			vfatReleaseFCB(DeviceExt, rcFcb);
			ExFreePool(PathNameBuffer);
			return Status;
		}
	}

	/* FsRtlIsNameInExpression need the searched string to be upcase,
	* even if IgnoreCase is specified */
	Status = RtlUpcaseUnicodeString(&FileToFindUpcase, FileToFindU, TRUE);
	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		ExFreePool(PathNameBuffer);
		return Status;
	}

	while(TRUE)
	{
		Status = DeviceExt->GetNextDirEntry(&Context, &Page, Parent, DirContext, First);
		First = FALSE;
		if (Status == STATUS_NO_MORE_ENTRIES)
		{
			break;
		}
		if (ENTRY_VOLUME(DeviceExt, &DirContext->DirEntry))
		{
			DirContext->DirIndex++;
			continue;
		}
		if (WildCard)
		{
			Found = FsRtlIsNameInExpression(&FileToFindUpcase, &DirContext->LongNameU, TRUE, NULL) ||
				FsRtlIsNameInExpression(&FileToFindUpcase, &DirContext->ShortNameU, TRUE, NULL);
		}
		else
		{
			Found = FsRtlAreNamesEqual(&DirContext->LongNameU, FileToFindU, TRUE, NULL) ||
				FsRtlAreNamesEqual(&DirContext->ShortNameU, FileToFindU, TRUE, NULL);
		}

		if (Found)
		{
			if (WildCard)
			{
				RtlCopyUnicodeString(&PathNameU, &Parent->PathNameU);
				if (!vfatFCBIsRoot(Parent))
				{
					PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = L'\\';
					PathNameU.Length += sizeof(WCHAR);
				}
				RtlAppendUnicodeStringToString(&PathNameU, &DirContext->LongNameU);
				PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = 0;
				rcFcb = vfatGrabFCBFromTable(DeviceExt, &PathNameU);
				if (rcFcb != NULL)
				{
					RtlCopyMemory(&DirContext->DirEntry, &rcFcb->entry, sizeof(DIR_ENTRY));
					vfatReleaseFCB(DeviceExt, rcFcb);
				}
			}
			DPRINT("%d\n", DirContext->LongNameU.Length);
			DPRINT("FindFile: new Name %wZ, DirIndex %d\n",
				&DirContext->LongNameU, DirContext->DirIndex);

			if (Context)
			{
				CcUnpinData(Context);
			}
			RtlFreeUnicodeString(&FileToFindUpcase);
			ExFreePool(PathNameBuffer);
			return STATUS_SUCCESS;
		}
		DirContext->DirIndex++;
	}

	if (Context)
	{
		CcUnpinData(Context);
	}

	RtlFreeUnicodeString(&FileToFindUpcase);
	ExFreePool(PathNameBuffer);
	return Status;
}

static
NTSTATUS
VfatOpenFile (
	PDEVICE_EXTENSION DeviceExt,
        PUNICODE_STRING PathNameU,
	PFILE_OBJECT FileObject,
	PVFATFCB* ParentFcb )
/*
 * FUNCTION: Opens a file
 */
{
	PVFATFCB Fcb;
	NTSTATUS Status;

	DPRINT ("VfatOpenFile(%08lx, '%wZ', %08lx, %08lx)\n", DeviceExt, PathNameU, FileObject, ParentFcb);

	if (FileObject->RelatedFileObject)
	{
		DPRINT ("'%wZ'\n", &FileObject->RelatedFileObject->FileName);

		*ParentFcb = FileObject->RelatedFileObject->FsContext;
		(*ParentFcb)->RefCount++;
	}
	else
	{
		*ParentFcb = NULL;
	}

	if (!DeviceExt->FatInfo.FixedMedia)
	{
		Status = VfatBlockDeviceIoControl (DeviceExt->StorageDevice,
			IOCTL_DISK_CHECK_VERIFY,
			NULL,
			0,
			NULL,
			0,
			FALSE);

		if (Status == STATUS_VERIFY_REQUIRED)

		{
			PDEVICE_OBJECT DeviceToVerify;

			DPRINT ("Media change detected!\n");
			DPRINT ("Device %p\n", DeviceExt->StorageDevice);

			DeviceToVerify = IoGetDeviceToVerify (PsGetCurrentThread ());

			IoSetDeviceToVerify (PsGetCurrentThread (),
				NULL);
			Status = IoVerifyVolume (DeviceExt->StorageDevice,
				FALSE);
		}
		if (!NT_SUCCESS(Status))
		{
			DPRINT ("Status %lx\n", Status);
			*ParentFcb = NULL;
			return Status;
		}
	}

	if (*ParentFcb)
	{
		(*ParentFcb)->RefCount++;
	}

	/*  try first to find an existing FCB in memory  */
	DPRINT ("Checking for existing FCB in memory\n");

	Status = vfatGetFCBForFile (DeviceExt, ParentFcb, &Fcb, PathNameU);
	if (!NT_SUCCESS (Status))
	{
		DPRINT ("Could not make a new FCB, status: %x\n", Status);
		return  Status;
	}
	if (Fcb->Flags & FCB_DELETE_PENDING)
	{
		vfatReleaseFCB (DeviceExt, Fcb);
		return STATUS_DELETE_PENDING;
	}
	DPRINT ("Attaching FCB to fileObject\n");
	Status = vfatAttachFCBToFileObject (DeviceExt, Fcb, FileObject);
	if (!NT_SUCCESS(Status))
	{
		vfatReleaseFCB (DeviceExt, Fcb);
	}
	return  Status;
}

static NTSTATUS
VfatCreateFile ( PDEVICE_OBJECT DeviceObject, PIRP Irp )
/*
 * FUNCTION: Create or open a file
 */
{
	PIO_STACK_LOCATION Stack;
	PFILE_OBJECT FileObject;
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_EXTENSION DeviceExt;
	ULONG RequestedDisposition, RequestedOptions;
	PVFATCCB pCcb;
	PVFATFCB pFcb = NULL;
	PVFATFCB ParentFcb = NULL;
	PWCHAR c, last;
	BOOLEAN PagingFileCreate = FALSE;
	BOOLEAN Dots;
	UNICODE_STRING FileNameU;
        UNICODE_STRING PathNameU;

	/* Unpack the various parameters. */
	Stack = IoGetCurrentIrpStackLocation (Irp);
	RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
	RequestedOptions =
		Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
	PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
	FileObject = Stack->FileObject;
	DeviceExt = DeviceObject->DeviceExtension;

	/* Check their validity. */
	if (RequestedOptions & FILE_DIRECTORY_FILE &&
		RequestedDisposition == FILE_SUPERSEDE)
	{
		return(STATUS_INVALID_PARAMETER);
	}

        if (RequestedOptions & FILE_DIRECTORY_FILE &&
            RequestedOptions & FILE_NON_DIRECTORY_FILE)
        {
		return(STATUS_INVALID_PARAMETER);
        }

	/* This a open operation for the volume itself */
	if (FileObject->FileName.Length == 0 &&
		FileObject->RelatedFileObject == NULL)
	{
		if (RequestedDisposition == FILE_CREATE ||
			RequestedDisposition == FILE_OVERWRITE_IF ||
			RequestedDisposition == FILE_SUPERSEDE)
		{
			return(STATUS_ACCESS_DENIED);
		}
		if (RequestedOptions & FILE_DIRECTORY_FILE)
		{
			return(STATUS_NOT_A_DIRECTORY);
		}
		pFcb = DeviceExt->VolumeFcb;
		pCcb = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
		if (pCcb == NULL)
		{
			return (STATUS_INSUFFICIENT_RESOURCES);
		}
		RtlZeroMemory(pCcb, sizeof(VFATCCB));
		FileObject->SectionObjectPointer = &pFcb->SectionObjectPointers;
		FileObject->FsContext = pFcb;
		FileObject->FsContext2 = pCcb;
		pFcb->RefCount++;

		Irp->IoStatus.Information = FILE_OPENED;
		return(STATUS_SUCCESS);
	}

	/*
	 * Check for illegal characters and illegale dot sequences in the file name
	 */
        PathNameU = FileObject->FileName;
	c = PathNameU.Buffer + PathNameU.Length / sizeof(WCHAR);
	last = c - 1;
	Dots = TRUE;
	while (c-- > PathNameU.Buffer)
	{
		if (*c == L'\\' || c == PathNameU.Buffer)
		{
			if (Dots && last > c)
			{
				return(STATUS_OBJECT_NAME_INVALID);
			}
			last = c - 1;
			Dots = TRUE;
		}
		else if (*c != L'.')
		{
			Dots = FALSE;
		}

		if (*c != '\\' && vfatIsLongIllegal(*c))
		{
			return(STATUS_OBJECT_NAME_INVALID);
		}
	}
        if (FileObject->RelatedFileObject && PathNameU.Buffer[0] == L'\\')
        {
            return(STATUS_OBJECT_NAME_INVALID);
        }
        if (PathNameU.Length > sizeof(WCHAR) && PathNameU.Buffer[PathNameU.Length/sizeof(WCHAR)-1] == L'\\')
        {
            PathNameU.Length -= sizeof(WCHAR);
        }

	/* Try opening the file. */
	Status = VfatOpenFile (DeviceExt, &PathNameU, FileObject, &ParentFcb);

	/*
	 * If the directory containing the file to open doesn't exist then
	 * fail immediately
	 */
	if (Status == STATUS_OBJECT_PATH_NOT_FOUND ||
            Status == STATUS_INVALID_PARAMETER ||
	    Status == STATUS_DELETE_PENDING)
	{
		if (ParentFcb)
		{
			vfatReleaseFCB (DeviceExt, ParentFcb);
		}
		return(Status);
	}
        if (!NT_SUCCESS(Status) && ParentFcb == NULL)
        {
                DPRINT1("VfatOpenFile faild for '%wZ', status %x\n", &PathNameU, Status);
                return Status;
        }

	/*
	 * If the file open failed then create the required file
	 */
	if (!NT_SUCCESS (Status))
	{
		if (RequestedDisposition == FILE_CREATE ||
		    RequestedDisposition == FILE_OPEN_IF ||
		    RequestedDisposition == FILE_OVERWRITE_IF ||
		    RequestedDisposition == FILE_SUPERSEDE)
		{
			ULONG Attributes;
			Attributes = Stack->Parameters.Create.FileAttributes;

			vfatSplitPathName(&PathNameU, NULL, &FileNameU);
			Status = VfatAddEntry (DeviceExt, &FileNameU, &pFcb, ParentFcb, RequestedOptions,
				(UCHAR)(Attributes & FILE_ATTRIBUTE_VALID_FLAGS));
			vfatReleaseFCB (DeviceExt, ParentFcb);
			if (NT_SUCCESS (Status))
			{
				Status = vfatAttachFCBToFileObject (DeviceExt, pFcb, FileObject);
				if ( !NT_SUCCESS(Status) )
				{
					vfatReleaseFCB (DeviceExt, pFcb);
					return Status;
				}

				Irp->IoStatus.Information = FILE_CREATED;
				VfatSetAllocationSizeInformation(FileObject,
					pFcb,
					DeviceExt,
					&Irp->Overlay.AllocationSize);
				VfatSetExtendedAttributes(FileObject,
					Irp->AssociatedIrp.SystemBuffer,
					Stack->Parameters.Create.EaLength);

				if (PagingFileCreate)
				{
					pFcb->Flags |= FCB_IS_PAGE_FILE;
				}
			}
			else
			{
				return(Status);
			}
		}
		else
		{
			if (ParentFcb)
			{
				vfatReleaseFCB (DeviceExt, ParentFcb);
			}
			return(Status);
		}
	}
	else
	{
		if (ParentFcb)
		{
			vfatReleaseFCB (DeviceExt, ParentFcb);
		}
		/* Otherwise fail if the caller wanted to create a new file  */
		if (RequestedDisposition == FILE_CREATE)
		{
			Irp->IoStatus.Information = FILE_EXISTS;
			VfatCloseFile (DeviceExt, FileObject);
			return(STATUS_OBJECT_NAME_COLLISION);
		}

		pFcb = FileObject->FsContext;

		if (pFcb->OpenHandleCount != 0)
		{
			Status = IoCheckShareAccess(Stack->Parameters.Create.SecurityContext->DesiredAccess,
				Stack->Parameters.Create.ShareAccess,
				FileObject,
				&pFcb->FCBShareAccess,
				FALSE);
			if (!NT_SUCCESS(Status))
			{
				VfatCloseFile (DeviceExt, FileObject);
				return(Status);
			}
		}

		/*
		 * Check the file has the requested attributes
		 */
		if (RequestedOptions & FILE_NON_DIRECTORY_FILE &&
			*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			VfatCloseFile (DeviceExt, FileObject);
			return(STATUS_FILE_IS_A_DIRECTORY);
		}
		if (RequestedOptions & FILE_DIRECTORY_FILE &&
			!(*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			VfatCloseFile (DeviceExt, FileObject);
			return(STATUS_NOT_A_DIRECTORY);
		}
#ifndef USE_ROS_CC_AND_FS
		if (!(*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (Stack->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA || 
				RequestedDisposition == FILE_OVERWRITE ||
				RequestedDisposition == FILE_OVERWRITE_IF)
			{
				if (!MmFlushImageSection(&pFcb->SectionObjectPointers, MmFlushForWrite))
				{
					DPRINT1("%wZ\n", &pFcb->PathNameU);
					DPRINT1("%d %d %d\n", Stack->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA, 
							RequestedDisposition == FILE_OVERWRITE, RequestedDisposition == FILE_OVERWRITE_IF);
					VfatCloseFile (DeviceExt, FileObject);
					return STATUS_SHARING_VIOLATION;
				}
			}
		}
#endif
		if (PagingFileCreate)
		{
			/* FIXME:
			 *   Do more checking for page files. It is possible,
			 *   that the file was opened and closed previously
			 *   as a normal cached file. In this case, the cache
			 *   manager has referenced the fileobject and the fcb
			 *   is held in memory. Try to remove the fileobject
			 *   from cache manager and use the fcb.
			 */
			if (pFcb->RefCount > 1)
			{
				if(!(pFcb->Flags & FCB_IS_PAGE_FILE))
				{
					VfatCloseFile(DeviceExt, FileObject);
					return(STATUS_INVALID_PARAMETER);
				}
			}
			else
			{
				pFcb->Flags |= FCB_IS_PAGE_FILE;
			}
		}
		else
		{
			if (pFcb->Flags & FCB_IS_PAGE_FILE)
			{
				VfatCloseFile(DeviceExt, FileObject);
				return(STATUS_INVALID_PARAMETER);
			}
		}


		if (RequestedDisposition == FILE_OVERWRITE ||
		    RequestedDisposition == FILE_OVERWRITE_IF ||
		    RequestedDisposition == FILE_SUPERSEDE)
		{
                        ExAcquireResourceExclusiveLite(&(pFcb->MainResource), TRUE);
			Status = VfatSetAllocationSizeInformation (FileObject,
				                                   pFcb,
				                                   DeviceExt,
				                                   &Irp->Overlay.AllocationSize);
                        ExReleaseResourceLite(&(pFcb->MainResource));
			if (!NT_SUCCESS (Status))
			{
				VfatCloseFile (DeviceExt, FileObject);
				return(Status);
			}
		}

		if (RequestedDisposition == FILE_SUPERSEDE)
		{
			Irp->IoStatus.Information = FILE_SUPERSEDED;
		}
		else if (RequestedDisposition == FILE_OVERWRITE || 
		         RequestedDisposition == FILE_OVERWRITE_IF)
		{
			Irp->IoStatus.Information = FILE_OVERWRITTEN;
		}
		else
		{
			Irp->IoStatus.Information = FILE_OPENED;
		}
	}

	if (pFcb->OpenHandleCount == 0)
	{
		IoSetShareAccess(Stack->Parameters.Create.SecurityContext->DesiredAccess,
			Stack->Parameters.Create.ShareAccess,
			FileObject,
			&pFcb->FCBShareAccess);
	}
	else
	{
		IoUpdateShareAccess(
			FileObject,
			&pFcb->FCBShareAccess
			);

	}

	pFcb->OpenHandleCount++;

	/* FIXME : test write access if requested */

	return(Status);
}


NTSTATUS
VfatCreate (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Create or open a file
 */
{
	NTSTATUS Status;

	ASSERT(IrpContext);

	if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
	{
		/* DeviceObject represents FileSystem instead of logical volume */
		DPRINT ("FsdCreate called with file system\n");
		IrpContext->Irp->IoStatus.Information = FILE_OPENED;
		IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest (IrpContext->Irp, IO_DISK_INCREMENT);
		VfatFreeIrpContext(IrpContext);
		return(STATUS_SUCCESS);
	}

	if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
	{
		return(VfatQueueRequest (IrpContext));
	}

	IrpContext->Irp->IoStatus.Information = 0;
	ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, TRUE);
	Status = VfatCreateFile (IrpContext->DeviceObject, IrpContext->Irp);
	ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

	IrpContext->Irp->IoStatus.Status = Status;
	IoCompleteRequest (IrpContext->Irp,
		(CCHAR)(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
	VfatFreeIrpContext(IrpContext);
	return(Status);
}

/* EOF */
