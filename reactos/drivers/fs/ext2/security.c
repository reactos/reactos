/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/security.c
 * PURPOSE:          Security support
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <internal/string.h>

//#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS Ext2QuerySecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{   
   DPRINT("Ext2QuerySecurity(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS Ext2SetSecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{   
   DPRINT("Ext2SetSecurity(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}
