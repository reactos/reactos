/* $Id: finfo.c,v 1.8 2001/06/12 12:35:42 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/finfo.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatGetStandardInformation(PVFATFCB FCB,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_STANDARD_INFORMATION StandardInfo,
			   PULONG BufferLength)
/*
 * FUNCTION: Retrieve the standard file information
 */
{
  PDEVICE_EXTENSION DeviceExtension;
  unsigned long AllocSize;

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  DeviceExtension = DeviceObject->DeviceExtension;
  /* PRECONDITION */
  assert (DeviceExtension != NULL);
  assert (DeviceExtension->BytesPerCluster != 0);
  assert (StandardInfo != NULL);
  assert (FCB != NULL);

  RtlZeroMemory(StandardInfo,
		sizeof(FILE_STANDARD_INFORMATION));

  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((FCB->entry.FileSize + DeviceExtension->BytesPerCluster - 1) /
	       DeviceExtension->BytesPerCluster) *
    DeviceExtension->BytesPerCluster;

  StandardInfo->AllocationSize = RtlConvertUlongToLargeInteger (AllocSize);
  StandardInfo->EndOfFile =
    RtlConvertUlongToLargeInteger (FCB->entry.FileSize);
  StandardInfo->NumberOfLinks = 0;
  StandardInfo->DeletePending = FALSE;
  if ((FCB->entry.Attrib & 0x10) > 0)
    {
      StandardInfo->Directory = TRUE;
    }
  else
    {
      StandardInfo->Directory = FALSE;
    }
  
  *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatSetPositionInformation(PFILE_OBJECT FileObject,
			   PVFATFCB FCB,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_POSITION_INFORMATION PositionInfo)
{
  DPRINT ("FsdSetPositionInformation()\n");

  DPRINT ("PositionInfo %x\n", PositionInfo);
  DPRINT ("Setting position %d\n", PositionInfo->CurrentByteOffset.u.LowPart);
  memcpy (&FileObject->CurrentByteOffset, &PositionInfo->CurrentByteOffset,
	  sizeof (LARGE_INTEGER));

  return (STATUS_SUCCESS);
}

static NTSTATUS
VfatGetPositionInformation(PFILE_OBJECT FileObject,
			   PVFATFCB FCB,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_POSITION_INFORMATION PositionInfo,
			   PULONG BufferLength)
{
  DPRINT ("VfatGetPositionInformation()\n");

  if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  PositionInfo->CurrentByteOffset.QuadPart =
    FileObject->CurrentByteOffset.QuadPart;

  DPRINT("Getting position %I64x\n",
	 PositionInfo->CurrentByteOffset.QuadPart);

  *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatGetBasicInformation(PFILE_OBJECT FileObject,
			PVFATFCB FCB,
			PDEVICE_OBJECT DeviceObject,
			PFILE_BASIC_INFORMATION BasicInfo,
			PULONG BufferLength)
{
  DPRINT("VfatGetBasicInformation()\n");

  if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  FsdDosDateTimeToFileTime(FCB->entry.CreationDate,
			   FCB->entry.CreationTime,
			   &BasicInfo->CreationTime);
  FsdDosDateTimeToFileTime(FCB->entry.AccessDate,
			   0,
			   &BasicInfo->LastAccessTime);
  FsdDosDateTimeToFileTime(FCB->entry.UpdateDate,
			   FCB->entry.UpdateTime,
			   &BasicInfo->LastWriteTime);
  FsdDosDateTimeToFileTime(FCB->entry.UpdateDate,
			   FCB->entry.UpdateTime,
			   &BasicInfo->ChangeTime);

  BasicInfo->FileAttributes = FCB->entry.Attrib;
  DPRINT("Getting attributes %x\n", BasicInfo->FileAttributes);

  *BufferLength -= sizeof(FILE_BASIC_INFORMATION);
  return(STATUS_SUCCESS);
}


static NTSTATUS
VfatSetDispositionInformation(PFILE_OBJECT FileObject,
			      PVFATFCB FCB,
			      PDEVICE_OBJECT DeviceObject,
			      PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
  DPRINT ("FsdSetDispositionInformation()\n");

  FileObject->DeletePending = DispositionInfo->DoDeleteFile;

  return (STATUS_SUCCESS);
}

static NTSTATUS
VfatGetNameInformation(PFILE_OBJECT FileObject,
		       PVFATFCB FCB,
		       PDEVICE_OBJECT DeviceObject,
		       PFILE_NAME_INFORMATION NameInfo,
		       PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file name information
 */
{
  ULONG NameLength;

  assert (NameInfo != NULL);
  assert (FCB != NULL);

  NameLength = wcslen(FCB->PathName) * sizeof(WCHAR);
  if (*BufferLength < sizeof(FILE_NAME_INFORMATION) + NameLength)
    return STATUS_BUFFER_OVERFLOW;

  NameInfo->FileNameLength = NameLength;
  memcpy(NameInfo->FileName,
	 FCB->PathName,
	 NameLength + sizeof(WCHAR));

  *BufferLength -=
    (sizeof(FILE_NAME_INFORMATION) + NameLength + sizeof(WCHAR));

  return STATUS_SUCCESS;
}



NTSTATUS STDCALL
VfatQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  PIO_STACK_LOCATION Stack;
  FILE_INFORMATION_CLASS FileInformationClass;
  PFILE_OBJECT FileObject = NULL;
  PVFATFCB FCB = NULL;
//   PVFATCCB CCB = NULL;

  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  assert (DeviceObject != NULL);
  assert (Irp != NULL);

  /* INITIALIZATION */
  Stack = IoGetCurrentIrpStackLocation (Irp);
  FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
  FileObject = Stack->FileObject;
//   CCB = (PVFATCCB)(FileObject->FsContext2);
//   FCB = CCB->Buffer; // Should be CCB->FCB???
  FCB = ((PVFATCCB) (FileObject->FsContext2))->pFcb;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = Stack->Parameters.QueryFile.Length;
  
  switch (FileInformationClass)
    {
    case FileStandardInformation:
      RC = VfatGetStandardInformation(FCB,
				      DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FilePositionInformation:
      RC = VfatGetPositionInformation(FileObject,
				      FCB,
				      DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileBasicInformation:
      RC = VfatGetBasicInformation(FileObject,
				   FCB,
				   DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;
    case FileNameInformation:
      RC = VfatGetNameInformation(FileObject,
				  FCB,
				  DeviceObject,
				  SystemBuffer,
				  &BufferLength);
      break;
    case FileInternalInformation:
    case FileAlternateNameInformation:
    case FileAllInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = RC;
  if (NT_SUCCESS(RC))
    Irp->IoStatus.Information =
      Stack->Parameters.QueryFile.Length - BufferLength;
  else
    Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return RC;
}

NTSTATUS STDCALL
VfatSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  PIO_STACK_LOCATION Stack;
  FILE_INFORMATION_CLASS FileInformationClass;
  PFILE_OBJECT FileObject = NULL;
  PVFATFCB FCB = NULL;
//   PVFATCCB CCB = NULL;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;

  /* PRECONDITION */
  assert(DeviceObject != NULL);
  assert(Irp != NULL);

  DPRINT("VfatSetInformation(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  /* INITIALIZATION */
  Stack = IoGetCurrentIrpStackLocation (Irp);
  FileInformationClass = Stack->Parameters.SetFile.FileInformationClass;
  FileObject = Stack->FileObject;
  FCB = ((PVFATCCB) (FileObject->FsContext2))->pFcb;
  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %x\n", SystemBuffer);

  switch (FileInformationClass)
    {
    case FilePositionInformation:
      RC = VfatSetPositionInformation(FileObject,
				      FCB,
				      DeviceObject,
				      SystemBuffer);
      break;
    case FileDispositionInformation:
      RC = VfatSetDispositionInformation(FileObject,
					 FCB,
					 DeviceObject,
					 SystemBuffer);
      break;
    case FileBasicInformation:
    case FileAllocationInformation:
    case FileEndOfFileInformation:
    case FileRenameInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = RC;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return RC;
}

/* EOF */
