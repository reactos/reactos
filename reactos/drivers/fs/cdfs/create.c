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
/* $Id: create.c,v 1.1 2002/04/15 20:39:49 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/cdfs.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

#if 0
NTSTATUS
vfatMakeAbsoluteFilename (PFILE_OBJECT pFileObject,
                          PWSTR pRelativeFileName,
                          PWSTR *pAbsoluteFilename)
{
  PWSTR  rcName;
  PVFATFCB  fcb;
  PVFATCCB  ccb;

  DPRINT ("try related for %S\n", pRelativeFileName);
  ccb = pFileObject->FsContext2;
  assert (ccb);
  fcb = ccb->pFcb;
  assert (fcb);

  /* verify related object is a directory and target name
     don't start with \. */
  if (!(fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
      || (pRelativeFileName[0] == L'\\'))
  {
    return  STATUS_INVALID_PARAMETER;
  }

  /* construct absolute path name */
  assert (wcslen (fcb->PathName) + 1 + wcslen (pRelativeFileName) + 1
          <= MAX_PATH);
  rcName = ExAllocatePool (NonPagedPool, MAX_PATH * sizeof(WCHAR));
  if (!rcName)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  wcscpy (rcName, fcb->PathName);
  if (!vfatFCBIsRoot(fcb))
    wcscat (rcName, L"\\");
  wcscat (rcName, pRelativeFileName);
  *pAbsoluteFilename = rcName;

  return  STATUS_SUCCESS;
}
#endif


static NTSTATUS
CdfsOpenFile(PDEVICE_EXTENSION DeviceExt,
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

  DPRINT("CdfsOpenFile(%08lx, %08lx, %S)\n", DeviceExt, FileObject, FileName);

  if (FileObject->RelatedFileObject)
    {
      DPRINT("Converting relative filename to absolute filename\n");
#if 0
      Status = vfatMakeAbsoluteFilename(FileObject->RelatedFileObject,
					FileName,
					&AbsFileName);
      FileName = AbsFileName;
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
#endif
      return(STATUS_UNSUCCESSFUL);
    }

  //FIXME: Get cannonical path name (remove .'s, ..'s and extra separators)

  DPRINT("PathName to open: %S\n", FileName);

  /*  try first to find an existing FCB in memory  */
  DPRINT("Checking for existing FCB in memory\n");
  Fcb = CdfsGrabFCBFromTable(DeviceExt,
			     FileName);
  if (Fcb == NULL)
    {
      DPRINT ("No existing FCB found, making a new one if file exists.\n");
      Status = CdfsGetFCBForFile(DeviceExt,
				 &ParentFcb,
				 &Fcb,
				 FileName);
      if (ParentFcb != NULL)
	{
	  CdfsReleaseFCB(DeviceExt,
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
  Status = CdfsAttachFCBToFileObject(DeviceExt,
				     Fcb,
				     FileObject);

  if (AbsFileName)
    ExFreePool (AbsFileName);

  return  Status;
}




static NTSTATUS
CdfsCreateFile(PDEVICE_OBJECT DeviceObject,
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

  DPRINT1("CdfsCreateFile() called\n");

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

  Status = CdfsOpenFile(DeviceExt,
			FileObject,
			FileObject->FileName.Buffer);

  /*
   * If the directory containing the file to open doesn't exist then
   * fail immediately
   */
  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = Status;
  return(Status);

#if 0
  /* Just skip leading backslashes... */
  while (*FileName == L'\\')
    FileName++;
CHECKPOINT1;

  Fcb = FsdSearchDirectory(DeviceExt->fss,
			   NULL,
			   FileName);
CHECKPOINT1;
  if (Fcb == NULL)
    {
      DPRINT1("FsdSearchDirectory() failed\n");
      return(STATUS_OBJECT_PATH_NOT_FOUND);
    }
CHECKPOINT1;

  FileObject->Flags = FileObject->Flags | FO_FCB_IS_VALID |
    FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  FileObject->FsContext = Fcb;
  FileObject->FsContext2 = DeviceExt->fss;

  DPRINT1("FsdOpenFile() done\n");

  return(STATUS_SUCCESS);
#endif
}


NTSTATUS STDCALL
CdfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;

  if (DeviceObject->Size == sizeof(DEVICE_OBJECT))
    {
      /* DeviceObject represents FileSystem instead of logical volume */
      DPRINT("FsdCreate called with file system\n");
      Irp->IoStatus.Information = FILE_OPENED;
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

  DeviceExt = DeviceObject->DeviceExtension;

  ExAcquireResourceExclusiveLite(&DeviceExt->DirResource,
				 TRUE);
  Status = CdfsCreateFile(DeviceObject,
			  Irp);
  ExReleaseResourceLite(&DeviceExt->DirResource);


ByeBye:
  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp,
		    NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);

  return(Status);
}

/* EOF */