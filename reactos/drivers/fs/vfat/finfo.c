/* $Id: finfo.c,v 1.12 2002/03/18 22:37:12 hbirr Exp $
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

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  DeviceExtension = DeviceObject->DeviceExtension;
  /* PRECONDITION */
  assert (DeviceExtension != NULL);
  assert (DeviceExtension->FatInfo.BytesPerCluster != 0);
  assert (StandardInfo != NULL);
  assert (FCB != NULL);

  RtlZeroMemory(StandardInfo,
		sizeof(FILE_STANDARD_INFORMATION));

  StandardInfo->AllocationSize = FCB->RFCB.AllocationSize;
  StandardInfo->EndOfFile = FCB->RFCB.FileSize;
  StandardInfo->NumberOfLinks = 0;
  StandardInfo->DeletePending = FCB->Flags & FCB_DELETE_PENDING ? TRUE : FALSE;
  StandardInfo->Directory = FCB->entry.Attrib & 0x10 ? TRUE : FALSE;

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
  KIRQL oldIrql;
  VFATFCB tmpFcb;
  WCHAR star[2];
  ULONG Index;
  NTSTATUS Status = STATUS_SUCCESS;
  int count;

  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

  DPRINT ("FsdSetDispositionInformation()\n");

  assert (DeviceExt != NULL);
  assert (DeviceExt->FatInfo.BytesPerCluster != 0);
  assert (FCB != NULL);

  if (!wcscmp(FCB->PathName, L"\\") || !wcscmp(FCB->ObjectName, L"..")
    || !wcscmp(FCB->ObjectName, L"."))
  {
    // we cannot delete a '.', '..' or the root directory
    return STATUS_ACCESS_DENIED;
  }
  if (DispositionInfo->DoDeleteFile)
  {
    KeAcquireSpinLock (&DeviceExt->FcbListLock, &oldIrql);
    count = FCB->RefCount;
    if (FCB->RefCount > 1)
      Status = STATUS_ACCESS_DENIED;
    else
    {
      FCB->Flags |= FCB_DELETE_PENDING;
      FileObject->DeletePending = TRUE;
    }
    KeReleaseSpinLock(&DeviceExt->FcbListLock, oldIrql);
    DPRINT("RefCount:%d\n", count);
    if (NT_SUCCESS(Status) && vfatFCBIsDirectory(DeviceExt, FCB))
    {
      memset (&tmpFcb, 0, sizeof(VFATFCB));
      tmpFcb.ObjectName = tmpFcb.PathName;
      star[0] = L'*';
      star[1] = 0;
      // skip '.' and '..', start by 2
      Index = 2;
      Status = FindFile (DeviceExt, &tmpFcb, FCB, star, &Index, NULL);
      if (NT_SUCCESS(Status))
      {
        DPRINT1("found: \'%S\'\n", tmpFcb.PathName);
        Status = STATUS_DIRECTORY_NOT_EMPTY;
        FCB->Flags &= ~FCB_DELETE_PENDING;
        FileObject->DeletePending = FALSE;
      }
      else
      {
        Status = STATUS_SUCCESS;
      }
    }
  }
  else
    FileObject->DeletePending = FALSE;
  return Status;
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

static NTSTATUS
VfatGetInternalInformation(PVFATFCB Fcb,
			   PFILE_INTERNAL_INFORMATION InternalInfo,
			   PULONG BufferLength)
{
  assert (InternalInfo);
  assert (Fcb);

  if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;
  // FIXME: get a real index, that can be used in a create operation
  InternalInfo->IndexNumber.QuadPart = 0;
  *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);
  return STATUS_SUCCESS;
}



NTSTATUS VfatQueryInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;

  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  assert (IrpContext);

  /* INITIALIZATION */
  FileInformationClass = IrpContext->Stack->Parameters.QueryFile.FileInformationClass;
  FCB = ((PVFATCCB) IrpContext->FileObject->FsContext2)->pFcb;

  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;
  BufferLength = IrpContext->Stack->Parameters.QueryFile.Length;

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     if (!ExAcquireResourceSharedLite(&FCB->MainResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
     {
        return VfatQueueRequest (IrpContext);
     }
  }


  switch (FileInformationClass)
    {
    case FileStandardInformation:
      RC = VfatGetStandardInformation(FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FilePositionInformation:
      RC = VfatGetPositionInformation(IrpContext->FileObject,
				      FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileBasicInformation:
      RC = VfatGetBasicInformation(IrpContext->FileObject,
				   FCB,
				   IrpContext->DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;
    case FileNameInformation:
      RC = VfatGetNameInformation(IrpContext->FileObject,
				  FCB,
				  IrpContext->DeviceObject,
				  SystemBuffer,
				  &BufferLength);
      break;
    case FileInternalInformation:
      RC = VfatGetInternalInformation(FCB,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileAlternateNameInformation:
    case FileAllInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }
  IrpContext->Irp->IoStatus.Status = RC;
  if (NT_SUCCESS(RC))
    IrpContext->Irp->IoStatus.Information =
      IrpContext->Stack->Parameters.QueryFile.Length - BufferLength;
  else
    IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}

NTSTATUS VfatSetInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;

  /* PRECONDITION */
  assert(IrpContext);

  DPRINT("VfatSetInformation(IrpContext %x)\n", IrpContext);

  /* INITIALIZATION */
  FileInformationClass = IrpContext->Stack->Parameters.SetFile.FileInformationClass;
  FCB = ((PVFATCCB) IrpContext->FileObject->FsContext2)->pFcb;
  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;

  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %x\n", SystemBuffer);

  if (FCB->Flags & FCB_IS_PAGE_FILE)
  {
     if (!ExAcquireResourceExclusiveLite(&FCB->PagingIoResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
     {
        return VfatQueueRequest (IrpContext);
     }
  }
  else
  {
     if (!ExAcquireResourceExclusiveLite(&FCB->MainResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
     {
        return VfatQueueRequest (IrpContext);
     }
  }

  switch (FileInformationClass)
    {
    case FilePositionInformation:
      RC = VfatSetPositionInformation(IrpContext->FileObject,
				      FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer);
      break;
    case FileDispositionInformation:
      RC = VfatSetDispositionInformation(IrpContext->FileObject,
					 FCB,
					 IrpContext->DeviceObject,
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

  if (FCB->Flags & FCB_IS_PAGE_FILE)
  {
     ExReleaseResourceLite(&FCB->PagingIoResource);
  }
  else
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }

  IrpContext->Irp->IoStatus.Status = RC;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}

/* EOF */
