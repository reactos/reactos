/* $Id: shutdown.c,v 1.2 2001/01/12 21:00:08 dwelch Exp $
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
