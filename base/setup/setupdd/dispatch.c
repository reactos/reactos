/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Device Driver Helper
 * FILE:            base/setup/setupdd/dispatch.c
 * PURPOSE:         Dispatching interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "setupdd.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI
CompleteRequest(IN PIRP      Irp,
                IN NTSTATUS  Status,
                IN ULONG_PTR Information)
{
    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS NTAPI
CompleteRequestNoInfo(IN PIRP     Irp,
                      IN NTSTATUS Status)
{
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS NTAPI
DispatchHelper(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
#define HANDLE_CTRL_CODE(Code)  \
    case Code :                 \
    {                           \
        DPRINT1("SpDd: " #Code ", Stack->FileObject = 0x%p\n", Stack->FileObject);          \
        if (Stack->FileObject)                                                              \
        {                                                                                   \
            DPRINT1("Stack->FileObject->FileName = %wZ\n", &Stack->FileObject->FileName);   \
        }                                                                                   \
        break;                                                                              \
    }

    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG MajorFunction      = Stack->MajorFunction;

    /* Just display all the IRP codes for now... */
    switch (MajorFunction)
    {
        HANDLE_CTRL_CODE(IRP_MJ_CREATE);
        HANDLE_CTRL_CODE(IRP_MJ_CREATE_NAMED_PIPE);
        HANDLE_CTRL_CODE(IRP_MJ_CLOSE);
        HANDLE_CTRL_CODE(IRP_MJ_READ);
        HANDLE_CTRL_CODE(IRP_MJ_WRITE);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_SET_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_EA);
        HANDLE_CTRL_CODE(IRP_MJ_SET_EA);
        HANDLE_CTRL_CODE(IRP_MJ_FLUSH_BUFFERS);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_VOLUME_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_SET_VOLUME_INFORMATION);
        HANDLE_CTRL_CODE(IRP_MJ_DIRECTORY_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_FILE_SYSTEM_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_DEVICE_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_INTERNAL_DEVICE_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_SHUTDOWN);
        HANDLE_CTRL_CODE(IRP_MJ_LOCK_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_CLEANUP);
        HANDLE_CTRL_CODE(IRP_MJ_CREATE_MAILSLOT);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_SECURITY);
        HANDLE_CTRL_CODE(IRP_MJ_SET_SECURITY);
        HANDLE_CTRL_CODE(IRP_MJ_POWER);
        HANDLE_CTRL_CODE(IRP_MJ_SYSTEM_CONTROL);
        HANDLE_CTRL_CODE(IRP_MJ_DEVICE_CHANGE);
        HANDLE_CTRL_CODE(IRP_MJ_QUERY_QUOTA);
        HANDLE_CTRL_CODE(IRP_MJ_SET_QUOTA);
        HANDLE_CTRL_CODE(IRP_MJ_PNP);
        // case IRP_MJ_PNP_POWER:
        // case IRP_MJ_MAXIMUM_FUNCTION:

        default:
        {
            DPRINT1("Unknown Major %lu\n", MajorFunction);
            break;
        }
    }

    return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS NTAPI
SpDdDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    DPRINT1("SpDdDispatch\n");
    return DispatchHelper(DeviceObject, Irp);
}

/* EOF */
