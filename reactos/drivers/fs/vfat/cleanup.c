/* $Id: cleanup.c,v 1.9 2003/01/11 15:52:54 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/cleanup.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatCleanupFile(PDEVICE_EXTENSION DeviceExt,
		PFILE_OBJECT FileObject)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
  PVFATCCB pCcb;
  PVFATFCB pFcb;
  
  DPRINT("VfatCleanupFile(DeviceExt %x, FileObject %x)\n",
	 DeviceExt, FileObject);
  
  /* FIXME: handle file/directory deletion here */
  pCcb = (PVFATCCB) (FileObject->FsContext2);
  if (pCcb == NULL)
    {
      return  STATUS_SUCCESS;
    }
  pFcb = pCcb->pFcb;

  /* Uninitialize file cache if initialized for this file object. */
  if (pFcb->RFCB.Bcb != NULL)
    {
      CcRosReleaseFileCache (FileObject, pFcb->RFCB.Bcb);
    }
  
  return STATUS_SUCCESS;
}

NTSTATUS VfatCleanup (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
   NTSTATUS Status;

   DPRINT("VfatCleanup(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

   if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, IrpContext->Flags & IRPCONTEXT_CANWAIT))
   {
     return VfatQueueRequest (IrpContext);
   }

   Status = VfatCleanupFile(IrpContext->DeviceExt, IrpContext->FileObject);

   ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

ByeBye:
   IrpContext->Irp->IoStatus.Status = Status;
   IrpContext->Irp->IoStatus.Information = 0;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

/* EOF */
