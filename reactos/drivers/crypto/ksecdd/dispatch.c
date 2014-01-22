/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
KsecDdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    ULONG_PTR Information;
    NTSTATUS Status;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStackLocation->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:

            /* Just return success */
            Status = STATUS_SUCCESS;
            Information = 0;
            break;

        case IRP_MJ_READ:

            /* There is nothing to read */
            Status = STATUS_END_OF_FILE;
            Information = 0;
            break;

        case IRP_MJ_WRITE:

            /* Pretend to have written everything */
            Status = STATUS_SUCCESS;
            Information = IoStackLocation->Parameters.Write.Length;
            break;

        default:
            DPRINT1("Unhandled major function %lu!\n",
                    IoStackLocation->MajorFunction);
            ASSERT(FALSE);
    }

    /* Return the information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    /* Complete the request */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
