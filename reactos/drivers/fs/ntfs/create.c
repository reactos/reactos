/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: create.c,v 1.1 2002/06/25 22:23:05 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/create.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
NtfsMakeAbsoluteFilename(PFILE_OBJECT pFileObject,
			 PWSTR pRelativeFileName,
			 PWSTR *pAbsoluteFilename)
{
  PWSTR rcName;
  PFCB Fcb;
  PCCB Ccb;

  DPRINT("try related for %S\n", pRelativeFileName);
  Ccb = pFileObject->FsContext2;
  assert(Ccb);
  Fcb = Ccb->Fcb;
  assert(Fcb);

  /* verify related object is a directory and target name
     don't start with \. */
  if (NtfsFCBIsDirectory(Fcb) == FALSE ||
      pRelativeFileName[0] == L'\\')
    {
      return(STATUS_INVALID_PARAMETER);
    }

  /* construct absolute path name */
  assert(wcslen (Fcb->PathName) + 1 + wcslen (pRelativeFileName) + 1
          <= MAX_PATH);
  rcName = ExAllocatePool(NonPagedPool, MAX_PATH * sizeof(WCHAR));
  if (!rcName)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  wcscpy(rcName, Fcb->PathName);
  if (!NtfsFCBIsRoot(Fcb))
    wcscat (rcName, L"\\");
  wcscat (rcName, pRelativeFileName);
  *pAbsoluteFilename = rcName;

  return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsOpenFile(PDEVICE_EXTENSION DeviceExt,
	     PFILE_OBJECT FileObject,
	     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
  PFCB ParentFcb;
  PFCB Fcb;
  NTSTATUS Status;
  PWSTR AbsFileName = NULL;

  DPRINT("NtfsOpenFile(%08lx, %08lx, %S)\n", DeviceExt, FileObject, FileName);

  if (FileObject->RelatedFileObject)
    {
      DPRINT("Converting relative filename to absolute filename\n");

      Status = NtfsMakeAbsoluteFilename(FileObject->RelatedFileObject,
					FileName,
					&AbsFileName);
      FileName = AbsFileName;
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      return(STATUS_UNSUCCESSFUL);
    }

  //FIXME: Get cannonical path name (remove .'s, ..'s and extra separators)

  DPRINT("PathName to open: %S\n", FileName);

  /*  try first to find an existing FCB in memory  */
  DPRINT("Checking for existing FCB in memory\n");
  Fcb = NtfsGrabFCBFromTable(DeviceExt,
			     FileName);
  if (Fcb == NULL)
    {
      DPRINT("No existing FCB found, making a new one if file exists.\n");
      Status = NtfsGetFCBForFile(DeviceExt,
				 &ParentFcb,
				 &Fcb,
				 FileName);
      if (ParentFcb != NULL)
	{
	  NtfsReleaseFCB(DeviceExt,
			 ParentFcb);
	}

      if (!NT_SUCCESS (Status))
	{
	  DPRINT("Could not make a new FCB, status: %x\n", Status);

	  if (AbsFileName)
	    ExFreePool(AbsFileName);

	  return(Status);
	}
    }

  DPRINT("Attaching FCB to fileObject\n");
  Status = NtfsAttachFCBToFileObject(DeviceExt,
				     Fcb,
				     FileObject);

  if (AbsFileName)
    ExFreePool (AbsFileName);

  return(Status);
}


static NTSTATUS
NtfsCreateFile(PDEVICE_OBJECT DeviceObject,
	       PIRP Irp)
/*
 * FUNCTION: Opens a file
 */
{
  PDEVICE_EXTENSION DeviceExt;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  ULONG RequestedDisposition;
  ULONG RequestedOptions;
  PFCB Fcb;
//  PWSTR FileName;
  NTSTATUS Status;

  DPRINT("NtfsCreateFile() called\n");

  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt);
  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack);

  RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
//  RequestedOptions =
//    Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
//  PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
//  if ((RequestedOptions & FILE_DIRECTORY_FILE)
//      && RequestedDisposition == FILE_SUPERSEDE)
//    return STATUS_INVALID_PARAMETER;

  FileObject = Stack->FileObject;

  if (RequestedDisposition == FILE_CREATE ||
      RequestedDisposition == FILE_OVERWRITE_IF ||
      RequestedDisposition == FILE_SUPERSEDE)
    {
      return(STATUS_ACCESS_DENIED);
    }

  Status = NtfsOpenFile(DeviceExt,
			FileObject,
			FileObject->FileName.Buffer);

  /*
   * If the directory containing the file to open doesn't exist then
   * fail immediately
   */
  Irp->IoStatus.Information = (NT_SUCCESS(Status)) ? FILE_OPENED : 0;
  Irp->IoStatus.Status = Status;

  return(Status);
}


NTSTATUS STDCALL
NtfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;

  if (DeviceObject == NtfsGlobalData->DeviceObject)
    {
      /* DeviceObject represents FileSystem instead of logical volume */
      DPRINT("Opening file system\n");
      Irp->IoStatus.Information = FILE_OPENED;
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

  DeviceExt = DeviceObject->DeviceExtension;

  ExAcquireResourceExclusiveLite(&DeviceExt->DirResource,
				 TRUE);
  Status = NtfsCreateFile(DeviceObject,
			  Irp);
  ExReleaseResourceLite(&DeviceExt->DirResource);

ByeBye:
  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp,
		    NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
