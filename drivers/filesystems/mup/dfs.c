/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/mup/dfs.c
 * PURPOSE:          Multi UNC Provider
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "mup.h"

#define NDEBUG
#include <debug.h>

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT, DfsDriverEntry)
#endif

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
DfsVolumePassThrough(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DfsFsdFileSystemControl(PDEVICE_OBJECT DeviceObject,
                        PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DfsFsdCreate(PDEVICE_OBJECT DeviceObject,
             PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DfsFsdCleanup(PDEVICE_OBJECT DeviceObject,
              PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DfsFsdClose(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
DfsUnload(PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
}

CODE_SEG("INIT")
NTSTATUS
DfsDriverEntry(PDRIVER_OBJECT DriverObject,
               PUNICODE_STRING RegistryPath)
{
    /* We don't support DFS yet, so
     * fail to make sure it remains disabled
     */
    DPRINT("DfsDriverEntry not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

