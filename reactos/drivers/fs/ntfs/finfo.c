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
/* $Id: finfo.c,v 1.1 2002/06/25 22:23:05 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/dirctl.c
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
NtfsGetStandardInformation(PFCB Fcb,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_STANDARD_INFORMATION StandardInfo,
			   PULONG BufferLength)
/*
 * FUNCTION: Retrieve the standard file information
 */
{
  DPRINT("NtfsGetStandardInformation() called\n");

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  /* PRECONDITION */
  assert(StandardInfo != NULL);
  assert(Fcb != NULL);

  RtlZeroMemory(StandardInfo,
		sizeof(FILE_STANDARD_INFORMATION));

  StandardInfo->AllocationSize = Fcb->RFCB.AllocationSize;
  StandardInfo->EndOfFile = Fcb->RFCB.FileSize;
  StandardInfo->NumberOfLinks = 0;
  StandardInfo->DeletePending = FALSE;
  StandardInfo->Directory = NtfsFCBIsDirectory(Fcb);

  *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
  return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsGetPositionInformation(PFILE_OBJECT FileObject,
			   PFILE_POSITION_INFORMATION PositionInfo,
			   PULONG BufferLength)
{
  DPRINT("NtfsGetPositionInformation() called\n");

  if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  PositionInfo->CurrentByteOffset.QuadPart = 
    0;
//    FileObject->CurrentByteOffset.QuadPart;

  DPRINT("Getting position %I64x\n",
	 PositionInfo->CurrentByteOffset.QuadPart);

  *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
  return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsGetBasicInformation(PFILE_OBJECT FileObject,
			PFCB Fcb,
			PDEVICE_OBJECT DeviceObject,
			PFILE_BASIC_INFORMATION BasicInfo,
			PULONG BufferLength)
{
  DPRINT("NtfsGetBasicInformation() called\n");

  if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

#if 0
  CdfsDateTimeToFileTime(Fcb,
			 &BasicInfo->CreationTime);
  CdfsDateTimeToFileTime(Fcb,
			 &BasicInfo->LastAccessTime);
  CdfsDateTimeToFileTime(Fcb,
			 &BasicInfo->LastWriteTime);
  CdfsDateTimeToFileTime(Fcb,
			 &BasicInfo->ChangeTime);

  CdfsFileFlagsToAttributes(Fcb,
			    &BasicInfo->FileAttributes);
#endif

  *BufferLength -= sizeof(FILE_BASIC_INFORMATION);

  return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsGetNameInformation(PFILE_OBJECT FileObject,
		       PFCB Fcb,
		       PDEVICE_OBJECT DeviceObject,
		       PFILE_NAME_INFORMATION NameInfo,
		       PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file name information
 */
{
  ULONG NameLength;

  DPRINT("NtfsGetNameInformation() called\n");

  assert(NameInfo != NULL);
  assert(Fcb != NULL);

//  NameLength = wcslen(Fcb->PathName) * sizeof(WCHAR);
  NameLength = 2;
  if (*BufferLength < sizeof(FILE_NAME_INFORMATION) + NameLength)
    return(STATUS_BUFFER_OVERFLOW);

  NameInfo->FileNameLength = NameLength;
//  memcpy(NameInfo->FileName,
//	 Fcb->PathName,
//	 NameLength + sizeof(WCHAR));
  wcscpy(NameInfo->FileName, L"\\");

  *BufferLength -=
    (sizeof(FILE_NAME_INFORMATION) + NameLength + sizeof(WCHAR));

  return(STATUS_SUCCESS);
}


static NTSTATUS
NtfsGetInternalInformation(PFCB Fcb,
			   PFILE_INTERNAL_INFORMATION InternalInfo,
			   PULONG BufferLength)
{
  DPRINT("NtfsGetInternalInformation() called\n");

  assert(InternalInfo);
  assert(Fcb);

  if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  /* FIXME: get a real index, that can be used in a create operation */
  InternalInfo->IndexNumber.QuadPart = 0;

  *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PFCB Fcb;
  PVOID SystemBuffer;
  ULONG BufferLength;

  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("NtfsQueryInformation() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
  FileObject = Stack->FileObject;
  Fcb = FileObject->FsContext;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = Stack->Parameters.QueryFile.Length;

  switch (FileInformationClass)
    {
      case FileStandardInformation:
	Status = NtfsGetStandardInformation(Fcb,
					    DeviceObject,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FilePositionInformation:
	Status = NtfsGetPositionInformation(FileObject,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FileBasicInformation:
	Status = NtfsGetBasicInformation(FileObject,
					 Fcb,
					 DeviceObject,
					 SystemBuffer,
					 &BufferLength);
	break;

      case FileNameInformation:
	Status = NtfsGetNameInformation(FileObject,
					Fcb,
					DeviceObject,
					SystemBuffer,
					&BufferLength);
	break;

      case FileInternalInformation:
	Status = NtfsGetInternalInformation(Fcb,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FileAlternateNameInformation:
      case FileAllInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	DPRINT("Unimplemented information class %u\n", FileInformationClass);
	Status = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = Status;
  if (NT_SUCCESS(Status))
    Irp->IoStatus.Information =
      Stack->Parameters.QueryFile.Length - BufferLength;
  else
    Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
