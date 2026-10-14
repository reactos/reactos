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
 * FILE:             drivers/filesystem/ntfs/fastio.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
NtfsAcqLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    PNTFS_FCB Fcb = (PNTFS_FCB)Context;

    ASSERT(Fcb->Identifier.Type == NTFS_TYPE_FCB);

    /* The paging writes below take it shared as well, size changes exclude us */
    if (!ExAcquireResourceSharedLite(&Fcb->PagingIoResource, Wait))
        return FALSE;

    /* Let Cc know it doesn't have to acquire the file again for the flush */
    if (IoGetTopLevelIrp() == NULL)
        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}


VOID
NTAPI
NtfsRelLazyWrite(
    _In_ PVOID Context)
{
    PNTFS_FCB Fcb = (PNTFS_FCB)Context;

    ASSERT(Fcb->Identifier.Type == NTFS_TYPE_FCB);

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
        IoSetTopLevelIrp(NULL);

    ExReleaseResourceLite(&Fcb->PagingIoResource);
}


BOOLEAN
NTAPI
NtfsAcqReadAhead(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    PNTFS_FCB Fcb = (PNTFS_FCB)Context;

    ASSERT(Fcb->Identifier.Type == NTFS_TYPE_FCB);

    /* Keep size changes out while we read ahead */
    if (!ExAcquireResourceSharedLite(&Fcb->MainResource, Wait))
        return FALSE;

    if (IoGetTopLevelIrp() == NULL)
        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}


VOID
NTAPI
NtfsRelReadAhead(
    _In_ PVOID Context)
{
    PNTFS_FCB Fcb = (PNTFS_FCB)Context;

    ASSERT(Fcb->Identifier.Type == NTFS_TYPE_FCB);

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
        IoSetTopLevelIrp(NULL);

    ExReleaseResourceLite(&Fcb->MainResource);
}

BOOLEAN
NTAPI
NtfsFastIoCheckIfPossible(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ BOOLEAN CheckForReadOperation,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* Deny FastIo */
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(CheckForReadOperation);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);
    return FALSE;
}

BOOLEAN
NTAPI
NtfsFastIoRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    DBG_UNREFERENCED_PARAMETER(FileObject);
    DBG_UNREFERENCED_PARAMETER(FileOffset);
    DBG_UNREFERENCED_PARAMETER(Length);
    DBG_UNREFERENCED_PARAMETER(Wait);
    DBG_UNREFERENCED_PARAMETER(LockKey);
    DBG_UNREFERENCED_PARAMETER(Buffer);
    DBG_UNREFERENCED_PARAMETER(IoStatus);
    DBG_UNREFERENCED_PARAMETER(DeviceObject);
    return FALSE;
}

BOOLEAN
NTAPI
NtfsFastIoWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    DBG_UNREFERENCED_PARAMETER(FileObject);
    DBG_UNREFERENCED_PARAMETER(FileOffset);
    DBG_UNREFERENCED_PARAMETER(Length);
    DBG_UNREFERENCED_PARAMETER(Wait);
    DBG_UNREFERENCED_PARAMETER(LockKey);
    DBG_UNREFERENCED_PARAMETER(Buffer);
    DBG_UNREFERENCED_PARAMETER(IoStatus);
    DBG_UNREFERENCED_PARAMETER(DeviceObject);
    return FALSE;
}

/* EOF */
