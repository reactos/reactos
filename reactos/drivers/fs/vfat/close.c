/* $Id: close.c,v 1.8 2001/08/14 20:47:30 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/close.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatCloseFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
  PVFATFCB pFcb;
  PVFATCCB pCcb;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT ("VfatCloseFile(DeviceExt %x, FileObject %x)\n",
	  DeviceExt, FileObject);

  /* FIXME : update entry in directory? */
  pCcb = (PVFATCCB) (FileObject->FsContext2);

  DPRINT ("pCcb %x\n", pCcb);
  if (pCcb == NULL)
  {
    return  STATUS_SUCCESS;
  }
  if (FileObject->FileName.Buffer)
  {
    // This a FO, that was created outside from FSD.
    // Some FO's are created with IoCreateStreamFileObject() insid from FSD.
    // This FO's haven't a FileName.
    pFcb = pCcb->pFcb;
    if (FileObject->DeletePending)
    {
      if (pFcb->Flags & FCB_DELETE_PENDING)
      {
        delEntry (DeviceExt, FileObject);
      }
      else
       Status = STATUS_DELETE_PENDING;
    }
    FileObject->FsContext2 = NULL;
    vfatReleaseFCB (DeviceExt, pFcb);
  }
  else
    FileObject->FsContext2 = NULL;

  ExFreePool (pCcb);

  return  Status;
}

NTSTATUS STDCALL
VfatClose (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Closes a file
 */
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  NTSTATUS Status;

  DPRINT ("VfatClose(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  ExAcquireResourceExclusiveLite (&DeviceExtension->DirResource, TRUE);
  Status = VfatCloseFile (DeviceExtension, FileObject);
  ExReleaseResourceLite (&DeviceExtension->DirResource);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest (Irp, IO_NO_INCREMENT);
  return (Status);
}

/* EOF */
