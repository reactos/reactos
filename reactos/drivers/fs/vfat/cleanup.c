/* $Id: cleanup.c,v 1.4 2002/05/15 18:05:00 ekohl Exp $
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
   DPRINT("VfatCleanupFile(DeviceExt %x, FileObject %x)\n",
	 DeviceExt, FileObject);

   /* FIXME: handle file/directory deletion here */

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
