/* $Id: shutdown.c,v 1.1 2000/12/29 13:45:01 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/shutdown.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;

   DPRINT("VfatShutdown(DeviceObject %x, Irp %x)\n",DeviceObject, Irp);

   /* FIXME: Shut down the file system */

   Status = STATUS_SUCCESS;

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

/* EOF */
