/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/fastio.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
CdfsAcquireForLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    PFCB Fcb = (PFCB)Context;
    ASSERT(Fcb);
    DPRINT("CdfsAcquireForLazyWrite(): Fcb %p\n", Fcb);

    if (!ExAcquireResourceExclusiveLite(&(Fcb->MainResource), Wait))
    {
        DPRINT("CdfsAcquireForLazyWrite(): ExReleaseResourceLite failed.\n");
        return FALSE;
    }
    return TRUE;
}

VOID
NTAPI
CdfsReleaseFromLazyWrite(
    _In_ PVOID Context)
{
    PFCB Fcb = (PFCB)Context;
    ASSERT(Fcb);
    DPRINT("CdfsReleaseFromLazyWrite(): Fcb %p\n", Fcb);

    ExReleaseResourceLite(&(Fcb->MainResource));
}

BOOLEAN
NTAPI
CdfsFastIoCheckIfPossible(
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
CdfsFastIoRead(
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
CdfsFastIoWrite(
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
