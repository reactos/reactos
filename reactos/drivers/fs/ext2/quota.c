/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/quota.c
 * PURPOSE:          Quota support
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

NTSTATUS Ext2QueryQuota(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   NTSTATUS Status;
   
   Status = STATUS_NOT_IMPLEMENTED;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS Ext2SetQuota(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   NTSTATUS Status;
   
   Status = STATUS_NOT_IMPLEMENTED;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}
