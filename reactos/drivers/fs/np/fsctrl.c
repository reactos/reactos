/* $Id: fsctrl.c,v 1.2 2001/05/01 11:09:01 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/fsctrl.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PNPFS_PIPE Pipe;
   PNPFS_FCB Fcb;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   Fcb = FileObject->FsContext;
   Pipe = Fcb->Pipe;
   
   switch (IoStack->Parameters.FileSystemControl.IoControlCode)
     {
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
	
      default:
     }
}


/* EOF */
