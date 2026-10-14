/*
 * PROJECT:     ReactOS NTFS driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Byte range locks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
NtfsLockControl(
    _In_ PNTFS_IRP_CONTEXT IrpContext)
{
    PNTFS_FCB Fcb;

    DPRINT("NtfsLockControl(%p)\n", IrpContext);

    if (IrpContext->DeviceObject == NtfsGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    /* Only the data of regular files can be locked */
    if (!NtfsIsRegularFileObject(IrpContext->FileObject))
        return STATUS_INVALID_PARAMETER;

    Fcb = (PNTFS_FCB)IrpContext->FileObject->FsContext;

    /* FsRtl completes the IRP, or keeps it until a conflicting lock goes away */
    IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;

    return FsRtlProcessFileLock(&Fcb->FileLock, IrpContext->Irp, NULL);
}

/* EOF */
