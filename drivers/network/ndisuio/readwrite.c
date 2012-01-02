/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        readwrite.c
 * PURPOSE:     Handles IRP_MJ_READ and IRP_MJ_WRITE
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
NduDispatchRead(PDEVICE_OBJECT DeviceObject,
                PIRP Irp)
{
    ASSERT(DeviceObject == GlobalDeviceObject);
    
    /* FIXME: Not implemented */
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
NduDispatchWrite(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp)
{
    ASSERT(DeviceObject == GlobalDeviceObject);

    /* FIXME: Not implemented */
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    
    return STATUS_NOT_IMPLEMENTED;
}