/* $Id: close.c,v 1.7 2001/05/10 04:02:21 rex Exp $
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

  DPRINT ("VfatCloseFile(DeviceExt %x, FileObject %x)\n",
	  DeviceExt, FileObject);

  /* FIXME : update entry in directory? */
  pCcb = (PVFATCCB) (FileObject->FsContext2);

  DPRINT ("pCcb %x\n", pCcb);
  if (pCcb == NULL)
  {
    return  STATUS_SUCCESS;
  }

  pFcb = pCcb->pFcb;
  vfatReleaseFCB (DeviceExt, pFcb);
  ExFreePool (pCcb);

  return  STATUS_SUCCESS;
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

  Status = VfatCloseFile (DeviceExtension, FileObject);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest (Irp, IO_NO_INCREMENT);
  return (Status);
}

/* EOF */
