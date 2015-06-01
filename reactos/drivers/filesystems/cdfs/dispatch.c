/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/cdfs/dispatch.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

static LONG QueueCount = 0;

/* FUNCTIONS ****************************************************************/

static WORKER_THREAD_ROUTINE CdfsDoRequest;

static
NTSTATUS
CdfsQueueRequest(PCDFS_IRP_CONTEXT IrpContext)
{
    InterlockedIncrement(&QueueCount);
    DPRINT("CdfsQueueRequest(IrpContext %p), %d\n", IrpContext, QueueCount);

    ASSERT(!(IrpContext->Flags & IRPCONTEXT_QUEUE) &&
           (IrpContext->Flags & IRPCONTEXT_COMPLETE));
    IrpContext->Flags |= IRPCONTEXT_CANWAIT;
    IoMarkIrpPending(IrpContext->Irp);
    ExInitializeWorkItem(&IrpContext->WorkQueueItem, CdfsDoRequest, IrpContext);
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

    return STATUS_PENDING;
}

static
NTSTATUS
CdfsDispatch(PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp = IrpContext->Irp;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DPRINT("CdfsDispatch()\n");

    FsRtlEnterFileSystem();

    CdfsIsIrpTopLevel(Irp);

    switch (IrpContext->MajorFunction)
    {
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            Status = CdfsQueryVolumeInformation(IrpContext);
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION:
            Status = CdfsSetVolumeInformation(IrpContext);
            break;

        case IRP_MJ_QUERY_INFORMATION:
            Status = CdfsQueryInformation(IrpContext);
            break;

        case IRP_MJ_SET_INFORMATION:
            Status = CdfsSetInformation(IrpContext);
            break;

        case IRP_MJ_DIRECTORY_CONTROL:
//            Status = CdfsDirectoryControl(IrpContext);
            break;

        case IRP_MJ_READ:
//            Status = CdfsRead(IrpContext);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            Status = CdfsDeviceControl(IrpContext);
            break;

        case IRP_MJ_WRITE:
//            Status = CdfsWrite(IrpContext);
            break;

        case IRP_MJ_CLOSE:
//            Status = CdfsClose(IrpContext);
            break;

        case IRP_MJ_CREATE:
//            Status = CdfsCreate(IrpContext);
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            Status = CdfsFileSystemControl(IrpContext);
            break;
    }

    ASSERT((!(IrpContext->Flags & IRPCONTEXT_COMPLETE) && !(IrpContext->Flags & IRPCONTEXT_QUEUE)) ||
           ((IrpContext->Flags & IRPCONTEXT_COMPLETE) && !(IrpContext->Flags & IRPCONTEXT_QUEUE)) ||
           (!(IrpContext->Flags & IRPCONTEXT_COMPLETE) && (IrpContext->Flags & IRPCONTEXT_QUEUE)));

    if (IrpContext->Flags & IRPCONTEXT_COMPLETE)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IrpContext->PriorityBoost);
    }

    if (IrpContext->Flags & IRPCONTEXT_QUEUE)
    {
        /* Reset our status flags before queueing the IRP */
        IrpContext->Flags |= IRPCONTEXT_COMPLETE;
        IrpContext->Flags &= ~IRPCONTEXT_QUEUE;
        Status = CdfsQueueRequest(IrpContext);
    }
    else
    {
        ExFreeToNPagedLookasideList(&CdfsGlobalData->IrpContextLookasideList, IrpContext);
    }

    IoSetTopLevelIrp(NULL);
    FsRtlExitFileSystem();

    return Status;
}

static
VOID
NTAPI
CdfsDoRequest(PVOID IrpContext)
{
    InterlockedDecrement(&QueueCount);
    DPRINT("CdfsDoRequest(IrpContext %p), MajorFunction %x, %d\n",
           IrpContext, ((PCDFS_IRP_CONTEXT)IrpContext)->MajorFunction, QueueCount);
    CdfsDispatch((PCDFS_IRP_CONTEXT)IrpContext);
}

/*
 * FUNCTION: This function manages IRP for various major functions
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           Irp = IRP to be passed to internal functions
 * RETURNS: Status of I/O Request
 */
NTSTATUS
NTAPI
CdfsFsdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PCDFS_IRP_CONTEXT IrpContext = NULL;
    NTSTATUS Status;

    DPRINT("CdfsFsdDispatch()\n");

    IrpContext = CdfsAllocateIrpContext(DeviceObject, Irp);
    if (IrpContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        Status = CdfsDispatch(IrpContext);
    }

    return Status;
}
