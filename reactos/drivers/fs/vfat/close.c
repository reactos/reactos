/* $Id: close.c,v 1.17 2003/02/13 22:24:16 hbirr Exp $
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
  pFcb = (PVFATFCB) (FileObject->FsContext);

  if (pFcb == NULL)
  {
     return STATUS_SUCCESS;
  }

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
      }
      else
      {
        Status = STATUS_DELETE_PENDING;
      }
    }
    vfatReleaseFCB (DeviceExt, pFcb);
  }
    
  FileObject->FsContext2 = NULL;
  FileObject->FsContext = NULL;
  FileObject->SectionObjectPointers = NULL;

  if (pCcb)
  {
    vfatDestroyCCB(pCcb);
  }
  
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
#if 0
  /* There occurs a dead look at the call to CcRosDeleteFileCache/ObDereferenceObject/VfatClose 
     in CmLazyCloseThreadMain if VfatClose is execute asynchronous in a worker thread. */
  if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
#else
  if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, TRUE))
#endif
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
