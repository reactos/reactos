/* $Id: shutdown.c,v 1.3 2001/01/14 15:28:50 ekohl Exp $
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

#if 0
   /* FIXME: block new mount requests */


   /* FIXME: Traverse list of logical volumes. For each volume: */
     {
	/* FIXME: acquire vcb resource exclusively */

	/* FIXME: Flush logical volume */

	/* FIXME: send IRP_MJ_SHUTDOWN to each volume */

	/* FIXME: wait for completion of IRP_MJ_SHUTDOWN */

	/* FIXME: release vcb resource */
     }

#endif

   Status = STATUS_SUCCESS;

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

/* EOF */
