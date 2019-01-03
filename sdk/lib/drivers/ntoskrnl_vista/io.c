/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            lib/drivers/ntoskrnl_vista/io.c
 * PURPOSE:         Io functions of Vista+
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntdef.h>
#include <ntifs.h>

NTKERNELAPI
NTSTATUS
NTAPI
IoGetIrpExtraCreateParameter(IN PIRP Irp,
                             OUT PECP_LIST *ExtraCreateParameter)
{
    /* Check we have a create operation */
    if (!BooleanFlagOn(Irp->Flags, IRP_CREATE_OPERATION))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If so, return user buffer */
    *ExtraCreateParameter = Irp->UserBuffer;
    return STATUS_SUCCESS;
}
