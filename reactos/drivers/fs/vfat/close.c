/* $Id: close.c,v 1.15 2002/11/11 21:49:17 hbirr Exp $
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
  pFcb = pCcb->pFcb;
  if (pFcb->Flags & FCB_IS_VOLUME)
  {
     DPRINT1("Volume\n");
     pFcb->RefCount--;
     FileObject->FsContext2 = NULL;
  }
  else if (FileObject->FileName.Buffer)
  {
    // This a FO, that was created outside from FSD.
    // Some FO's are created with IoCreateStreamFileObject() insid from FSD.
    // This FO's haven't a FileName.
    if (FileObject->DeletePending)
    {
      if (pFcb->Flags & FCB_DELETE_PENDING)
      {
        delEntry (DeviceExt, FileObject);
	pFcb->Flags &= ~FCB_UPDATE_DIRENTRY;
      }
      else
       Status = STATUS_DELETE_PENDING;
    }
    if (pFcb->Flags & FCB_UPDATE_DIRENTRY)
    {
       VfatUpdateEntry (DeviceExt, FileObject);
       pFcb->Flags &= ~FCB_UPDATE_DIRENTRY;
    }
    FileObject->FsContext2 = NULL;
    vfatReleaseFCB (DeviceExt, pFcb);
  }
  else
    FileObject->FsContext2 = NULL;

  vfatDestroyCCB(pCcb);
  
  return  Status;
}

NTSTATUS VfatClose (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Closes a file
 */
{
  NTSTATUS Status;

  DPRINT ("VfatClose(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
      DPRINT("Closing file system\n");
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

  if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
  {
     return VfatQueueRequest (IrpContext);
  }

  Status = VfatCloseFile (IrpContext->DeviceExt, IrpContext->FileObject);
  ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

ByeBye:
  IrpContext->Irp->IoStatus.Status = Status;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return (Status);
}

/* EOF */
