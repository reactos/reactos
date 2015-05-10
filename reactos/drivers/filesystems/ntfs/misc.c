/*
 *  ReactOS kernel
 *  Copyright (C) 2008, 2014 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/misc.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Used with IRP to set them to TopLevelIrp field
 * ARGUMENTS:
 *           Irp = The IRP to set
 * RETURNS: TRUE if top level was null, else FALSE
 */
BOOLEAN
NtfsIsIrpTopLevel(PIRP Irp)
{
    BOOLEAN ReturnCode = FALSE;

    TRACE_(NTFS, "NtfsIsIrpTopLevel()\n");

    if (IoGetTopLevelIrp() == NULL)
    {
        IoSetTopLevelIrp(Irp);
        ReturnCode = TRUE;
    }

    return ReturnCode;
}

/*
 * FUNCTION: Allocate and fill an NTFS_IRP_CONTEXT struct in order to use it for IRP
 * ARGUMENTS:
 *           DeviceObject = Used to fill in struct 
 *           Irp = The IRP that need IRP_CONTEXT struct
 * RETURNS: NULL or PNTFS_IRP_CONTEXT
 */
PNTFS_IRP_CONTEXT
NtfsAllocateIrpContext(PDEVICE_OBJECT DeviceObject,
                       PIRP Irp)
{
    PNTFS_IRP_CONTEXT IrpContext;

    TRACE_(NTFS, "NtfsAllocateIrpContext()\n");

    IrpContext = (PNTFS_IRP_CONTEXT)ExAllocatePoolWithTag(NonPagedPool,
                                                          sizeof(NTFS_IRP_CONTEXT),
                                                          'PRIN');
    if (IrpContext == NULL)
        return NULL;

    RtlZeroMemory(IrpContext, sizeof(NTFS_IRP_CONTEXT));

    IrpContext->Identifier.Type = NTFS_TYPE_IRP_CONTEST;
    IrpContext->Identifier.Size = sizeof(NTFS_IRP_CONTEXT);
    IrpContext->Irp = Irp;
    IrpContext->DeviceObject = DeviceObject;
    IrpContext->Stack = IoGetCurrentIrpStackLocation(Irp);
    IrpContext->MajorFunction = IrpContext->Stack->MajorFunction;
    IrpContext->MinorFunction = IrpContext->Stack->MinorFunction;
    IrpContext->FileObject = IrpContext->Stack->FileObject;
    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
    IrpContext->PriorityBoost = IO_NO_INCREMENT;
    IrpContext->Flags = IRPCONTEXT_COMPLETE;

    if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN ||
        (IrpContext->MajorFunction != IRP_MJ_CLEANUP &&
         IrpContext->MajorFunction != IRP_MJ_CLOSE &&
         IoIsOperationSynchronous(Irp)))
    {
        IrpContext->Flags |= IRPCONTEXT_CANWAIT;
    }

    return IrpContext;
}

VOID
NtfsFileFlagsToAttributes(ULONG NtfsAttributes,
                          PULONG FileAttributes)
{
    *FileAttributes = NtfsAttributes;
    if ((NtfsAttributes & NTFS_FILE_TYPE_DIRECTORY) == NTFS_FILE_TYPE_DIRECTORY)
    {
        *FileAttributes = NtfsAttributes & ~NTFS_FILE_TYPE_DIRECTORY;
        *FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    if (NtfsAttributes == 0)
        *FileAttributes = FILE_ATTRIBUTE_NORMAL;
}

PVOID
NtfsGetUserBuffer(PIRP Irp)
{
    if (Irp->MdlAddress != NULL)
    {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
    }
    else
    {
        return Irp->UserBuffer;
    }
}

/* EOF */
