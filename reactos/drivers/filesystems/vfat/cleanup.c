/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/cleanup.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatCleanupFile(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
  PVFATFCB pFcb;
  PFILE_OBJECT FileObject = IrpContext->FileObject;

  DPRINT("VfatCleanupFile(DeviceExt %x, FileObject %x)\n",
	 IrpContext->DeviceExt, FileObject);

  /* FIXME: handle file/directory deletion here */
  pFcb = (PVFATFCB) FileObject->FsContext;
  if (pFcb)
    {
      if (pFcb->Flags & FCB_IS_VOLUME)
        {
          pFcb->OpenHandleCount--;

          if (pFcb->OpenHandleCount != 0)
            {
              IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
            }
        }
      else
        {
          if(!ExAcquireResourceExclusiveLite (&pFcb->MainResource,
                                              (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
            {
              return STATUS_PENDING;
            }
          if(!ExAcquireResourceExclusiveLite (&pFcb->PagingIoResource,
                                              (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
            {
              ExReleaseResourceLite (&pFcb->MainResource);
              return STATUS_PENDING;
            }

          pFcb->OpenHandleCount--;

          if (!(*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&
              FsRtlAreThereCurrentFileLocks(&pFcb->FileLock))
            {
              /* remove all locks this process have on this file */
              FsRtlFastUnlockAll(&pFcb->FileLock,
                                 FileObject,
                                 IoGetRequestorProcess(IrpContext->Irp),
                                 NULL);
            }

     if (pFcb->Flags & FCB_IS_DIRTY)
       {
	 VfatUpdateEntry (pFcb);
       }

     if (pFcb->Flags & FCB_DELETE_PENDING &&
         pFcb->OpenHandleCount == 1)
       {
	 PFILE_OBJECT tmpFileObject;
	 tmpFileObject = pFcb->FileObject;
	 if (tmpFileObject != NULL)
	   {
	     pFcb->FileObject = NULL;
#ifdef USE_ROS_CC_AND_FS
             CcRosReleaseFileCache(tmpFileObject);
#else
             CcUninitializeCacheMap(tmpFileObject, NULL, NULL);
#endif
             ObDereferenceObject(tmpFileObject);
           }

#if 0
         /* FIXME:
	  *  CcPurgeCacheSection is unimplemented.
	  */
         CcPurgeCacheSection(FileObject->SectionObjectPointer, NULL, 0, FALSE);
#endif
       }
     /* Uninitialize file cache if. */
#ifdef USE_ROS_CC_AND_FS
     CcRosReleaseFileCache (FileObject);
#else
          if (FileObject->SectionObjectPointer->SharedCacheMap)
	    {
              CcUninitializeCacheMap (FileObject, &pFcb->RFCB.FileSize, NULL);
	    }
#endif
          if (pFcb->OpenHandleCount != 0)
            {
              IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
            }

          FileObject->Flags |= FO_CLEANUP_COMPLETE;

          ExReleaseResourceLite (&pFcb->PagingIoResource);
          ExReleaseResourceLite (&pFcb->MainResource);
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

   DPRINT("VfatCleanup(DeviceObject %x, Irp %x)\n", IrpContext->DeviceObject, IrpContext->Irp);

   if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
     {
       Status = STATUS_SUCCESS;
       goto ByeBye;
     }

   if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource,
                                        (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
     {
       return VfatQueueRequest (IrpContext);
     }

   Status = VfatCleanupFile(IrpContext);

   ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

   if (Status == STATUS_PENDING)
   {
      return VfatQueueRequest(IrpContext);
   }

ByeBye:
   IrpContext->Irp->IoStatus.Status = Status;
   IrpContext->Irp->IoStatus.Information = 0;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

/* EOF */
