/* $Id: cleanup.c,v 1.1 2001/03/07 13:44:40 ekohl Exp $
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

NTSTATUS STDCALL
VfatCleanup (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;

   DPRINT1("VfatCleanup(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

   Status = VfatCleanupFile(DeviceExtension, FileObject);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   return (Status);
}

/* EOF */
