/* $Id: cleanup.c,v 1.11 2003/02/13 22:24:16 hbirr Exp $
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
VfatCleanupFile(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
  PVFATFCB pFcb;
  PDEVICE_EXTENSION DeviceExt = IrpContext->DeviceExt;
  PFILE_OBJECT FileObject = IrpContext->FileObject;
  
  DPRINT("VfatCleanupFile(DeviceExt %x, FileObject %x)\n",
	 DeviceExt, FileObject);
  
  /* FIXME: handle file/directory deletion here */
  pFcb = (PVFATFCB) FileObject->FsContext;
  if (pFcb)
  {
     if (!(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY) &&
        FsRtlAreThereCurrentFileLocks(&pFcb->FileLock))
     {
        /* remove all locks this process have on this file */
        FsRtlFastUnlockAll(&pFcb->FileLock,
                           FileObject,
                           IoGetRequestorProcess(IrpContext->Irp),
                           NULL);
     }

     /* Uninitialize file cache if initialized for this file object. */
     if (FileObject->PrivateCacheMap)
     {
        CcRosReleaseFileCache (FileObject);
     }
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

   Status = VfatCleanupFile(IrpContext);

   ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

ByeBye:
   IrpContext->Irp->IoStatus.Status = Status;
   IrpContext->Irp->IoStatus.Information = 0;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

/* EOF */
