/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2004 ReactOS Team
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
/* $Id: finfo.c,v 1.9 2004/03/08 08:51:26 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/finfo.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Retrieve the standard file information
 */
static NTSTATUS
CdfsGetStandardInformation(PFCB Fcb,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_STANDARD_INFORMATION StandardInfo,
			   PULONG BufferLength)
{
  DPRINT("CdfsGetStandardInformation() called\n");

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  /* PRECONDITION */
  assert(StandardInfo != NULL);
  assert(Fcb != NULL);

  RtlZeroMemory(StandardInfo,
		sizeof(FILE_STANDARD_INFORMATION));

  StandardInfo->AllocationSize = Fcb->RFCB.AllocationSize;
  StandardInfo->EndOfFile = Fcb->RFCB.FileSize;
  StandardInfo->NumberOfLinks = 0;
  StandardInfo->DeletePending = FALSE;
  StandardInfo->Directory = Fcb->Entry.FileFlags & 0x02 ? TRUE : FALSE;

  *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
  return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Retrieve the file position information
 */
static NTSTATUS
CdfsGetPositionInformation(PFILE_OBJECT FileObject,
			   PFILE_POSITION_INFORMATION PositionInfo,
			   PULONG BufferLength)
{
  DPRINT("CdfsGetPositionInformation() called\n");

  if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  PositionInfo->CurrentByteOffset.QuadPart =
    FileObject->CurrentByteOffset.QuadPart;

  DPRINT("Getting position %I64x\n",
	 PositionInfo->CurrentByteOffset.QuadPart);

  *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
  return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Retrieve the basic file information
 */
static NTSTATUS
CdfsGetBasicInformation(PFILE_OBJECT FileObject,
			PFCB Fcb,
			PDEVICE_OBJECT DeviceObject,
			PFILE_BASIC_INFORMATION BasicInfo,
			PULONG BufferLength)
{
  DPRINT("CdfsGetBasicInformation() called\n");

  if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&BasicInfo->CreationTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&BasicInfo->LastAccessTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&BasicInfo->LastWriteTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&BasicInfo->ChangeTime);

  CdfsFileFlagsToAttributes(Fcb,
			    &BasicInfo->FileAttributes);

  *BufferLength -= sizeof(FILE_BASIC_INFORMATION);

  return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Retrieve the file name information
 */
static NTSTATUS
CdfsGetNameInformation(PFILE_OBJECT FileObject,
		       PFCB Fcb,
		       PDEVICE_OBJECT DeviceObject,
		       PFILE_NAME_INFORMATION NameInfo,
		       PULONG BufferLength)
{
  ULONG NameLength;

  DPRINT("CdfsGetNameInformation() called\n");

  assert(NameInfo != NULL);
  assert(Fcb != NULL);

  NameLength = wcslen(Fcb->PathName) * sizeof(WCHAR);
  if (*BufferLength < sizeof(FILE_NAME_INFORMATION) + NameLength)
    return STATUS_BUFFER_OVERFLOW;

  NameInfo->FileNameLength = NameLength;
  RtlCopyMemory(NameInfo->FileName,
		Fcb->PathName,
		NameLength + sizeof(WCHAR));

  *BufferLength -=
    (sizeof(FILE_NAME_INFORMATION) + NameLength + sizeof(WCHAR));

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the internal file information
 */
static NTSTATUS
CdfsGetInternalInformation(PFCB Fcb,
			   PFILE_INTERNAL_INFORMATION InternalInfo,
			   PULONG BufferLength)
{
  DPRINT("CdfsGetInternalInformation() called\n");

  assert(InternalInfo);
  assert(Fcb);

  if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  InternalInfo->IndexNumber.QuadPart = Fcb->IndexNumber.QuadPart;

  *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);

  return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Retrieve the file network open information
 */
static NTSTATUS
CdfsGetNetworkOpenInformation(PFCB Fcb,
			      PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
			      PULONG BufferLength)
{
  assert(NetworkInfo);
  assert(Fcb);

  if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  CdfsDateTimeToFileTime(Fcb,
			 &NetworkInfo->CreationTime);
  CdfsDateTimeToFileTime(Fcb,
			 &NetworkInfo->LastAccessTime);
  CdfsDateTimeToFileTime(Fcb,
			 &NetworkInfo->LastWriteTime);
  CdfsDateTimeToFileTime(Fcb,
			 &NetworkInfo->ChangeTime);
  NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;
  NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
  CdfsFileFlagsToAttributes(Fcb,
			    &NetworkInfo->FileAttributes);

  *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);

  return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Retrieve all file information
 */
static NTSTATUS
CdfsGetAllInformation(PFILE_OBJECT FileObject,
		      PFCB Fcb,
		      PFILE_ALL_INFORMATION Info,
		      PULONG BufferLength)
{
  ULONG NameLength;

  assert(Info);
  assert(Fcb);

  NameLength = wcslen(Fcb->PathName) * sizeof(WCHAR);
  if (*BufferLength < sizeof(FILE_ALL_INFORMATION) + NameLength)
    return(STATUS_BUFFER_OVERFLOW);

  /* Basic Information */
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&Info->BasicInformation.CreationTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&Info->BasicInformation.LastAccessTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&Info->BasicInformation.LastWriteTime);
  CdfsDateTimeToFileTime(Fcb,
			 (TIME *)&Info->BasicInformation.ChangeTime);
  CdfsFileFlagsToAttributes(Fcb,
			    &Info->BasicInformation.FileAttributes);

  /* Standard Information */
  Info->StandardInformation.AllocationSize = Fcb->RFCB.AllocationSize;
  Info->StandardInformation.EndOfFile = Fcb->RFCB.FileSize;
  Info->StandardInformation.NumberOfLinks = 0;
  Info->StandardInformation.DeletePending = FALSE;
  Info->StandardInformation.Directory = Fcb->Entry.FileFlags & FILE_FLAG_DIRECTORY ? TRUE : FALSE;

  /* Internal Information */
  Info->InternalInformation.IndexNumber.QuadPart = Fcb->IndexNumber.QuadPart;

  /* EA Information */
  Info->EaInformation.EaSize = 0;

  /* Access Information */
  /* The IO-Manager adds this information */

  /* Position Information */
  Info->PositionInformation.CurrentByteOffset.QuadPart = FileObject->CurrentByteOffset.QuadPart;

  /* Mode Information */
  /* The IO-Manager adds this information */

  /* Alignment Information */
  /* The IO-Manager adds this information */

  /* Name Information */
  Info->NameInformation.FileNameLength = NameLength;
  RtlCopyMemory(Info->NameInformation.FileName,
		Fcb->PathName,
		NameLength + sizeof(WCHAR));

  *BufferLength -= (sizeof(FILE_ALL_INFORMATION) + NameLength + sizeof(WCHAR));

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the specified file information
 */
NTSTATUS STDCALL
CdfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PFCB Fcb;
  PVOID SystemBuffer;
  ULONG BufferLength;

  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("CdfsQueryInformation() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
  FileObject = Stack->FileObject;
  Fcb = FileObject->FsContext;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = Stack->Parameters.QueryFile.Length;

  switch (FileInformationClass)
    {
      case FileStandardInformation:
	Status = CdfsGetStandardInformation(Fcb,
					    DeviceObject,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FilePositionInformation:
	Status = CdfsGetPositionInformation(FileObject,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FileBasicInformation:
	Status = CdfsGetBasicInformation(FileObject,
					 Fcb,
					 DeviceObject,
					 SystemBuffer,
					 &BufferLength);
	break;

      case FileNameInformation:
	Status = CdfsGetNameInformation(FileObject,
					Fcb,
					DeviceObject,
					SystemBuffer,
					&BufferLength);
	break;

      case FileInternalInformation:
	Status = CdfsGetInternalInformation(Fcb,
					    SystemBuffer,
					    &BufferLength);
	break;

      case FileNetworkOpenInformation:
	Status = CdfsGetNetworkOpenInformation(Fcb,
					       SystemBuffer,
					       &BufferLength);
	break;

      case FileAllInformation:
	Status = CdfsGetAllInformation(FileObject,
				       Fcb,
				       SystemBuffer,
				       &BufferLength);
	break;

      case FileAlternateNameInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	DPRINT("Unimplemented information class %u\n", FileInformationClass);
	Status = STATUS_NOT_SUPPORTED;
	break;
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


/*
 * FUNCTION: Set the file position information
 */
static NTSTATUS
CdfsSetPositionInformation(PFILE_OBJECT FileObject,
			   PFILE_POSITION_INFORMATION PositionInfo)
{
  DPRINT ("CdfsSetPositionInformation()\n");

  DPRINT ("PositionInfo %x\n", PositionInfo);
  DPRINT ("Setting position %I64u\n", PositionInfo->CurrentByteOffset.QuadPart);

  FileObject->CurrentByteOffset.QuadPart =
    PositionInfo->CurrentByteOffset.QuadPart;

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Set the specified file information
 */
NTSTATUS STDCALL
CdfsSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PFCB Fcb;
  PVOID SystemBuffer;

  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("CdfsSetInformation() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileInformationClass = Stack->Parameters.SetFile.FileInformationClass;
  FileObject = Stack->FileObject;
  Fcb = FileObject->FsContext;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

  switch (FileInformationClass)
    {
      case FilePositionInformation:
	Status = CdfsSetPositionInformation(FileObject,
					    SystemBuffer);
	break;

      case FileBasicInformation:
      case FileRenameInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	Status = STATUS_NOT_SUPPORTED;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}

/* EOF */
