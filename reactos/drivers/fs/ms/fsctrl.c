/* $Id: fsctrl.c,v 1.1 2001/05/05 15:11:57 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/fsctrl.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "msfs.h"

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MsfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   NTSTATUS Status;
   
   DPRINT1("MsfsFileSystemControl(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   Fcb = FileObject->FsContext;
   Mailslot = Fcb->Mailslot;
   
   DPRINT1("Mailslot name: %wZ\n", &Mailslot->Name);
   
   switch (IoStack->Parameters.FileSystemControl.IoControlCode)
     {
#if 0
      case FSCTL_WAIT_PIPE:
	break;
	
      case FSCTL_LISTEN:
	break;
	
      case FSCTL_SET_STATE:
	break;
	
      case FSCTL_GET_STATE:
	  {
	     
	     
	     break;
	  }
	
#endif
     default:
	Status = STATUS_NOT_IMPLEMENTED;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
