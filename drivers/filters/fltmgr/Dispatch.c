/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Dispatch.c
* PURPOSE:         Contains dispatch handler routines
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"
#include "fltmgr_shared.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

NTSTATUS
HandleLoadUnloadIoctl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
);

NTSTATUS
HandleFindFirstIoctl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
);


/* sdsfds *******************************************************************/

NTSTATUS
FltpDeviceControlHandler(_In_ PDEVICE_OBJECT DeviceObject,
                         _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION StackPtr;
    ULONG ControlCode;
    NTSTATUS Status;

    StackPtr = IoGetCurrentIrpStackLocation(Irp);
    FLT_ASSERT(StackPtr->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    ControlCode = StackPtr->Parameters.DeviceIoControl.IoControlCode;
    switch (ControlCode)
    {
        case IOCTL_FILTER_LOAD:
            Status = HandleLoadUnloadIoctl(DeviceObject, Irp);
            break;

        case IOCTL_FILTER_UNLOAD:
            Status = HandleLoadUnloadIoctl(DeviceObject, Irp);
            break;

        case IOCTL_FILTER_CREATE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_ATTATCH:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_DETATCH:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_SEND_MESSAGE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_GET_MESSAGE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_REPLY_MESSAGE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_FIND_FIRST:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_FIND_NEXT:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_FILTER_GET_INFO:
            Status = STATUS_NOT_SUPPORTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}

NTSTATUS
FltpDispatchHandler(_In_ PDEVICE_OBJECT DeviceObject,
                    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION StackPtr;
    StackPtr = IoGetCurrentIrpStackLocation(Irp);
    UNREFERENCED_PARAMETER(StackPtr);

    // implement me

    return STATUS_SUCCESS;
}


/* INTERNAL FUNCTIONS ******************************************************/

NTSTATUS
HandleLoadUnloadIoctl(_In_ PDEVICE_OBJECT DeviceObject,
                      _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION StackPtr;
    UNICODE_STRING Name;
    PFILTER_NAME FilterName;
    ULONG BufferLength;
    ULONG ControlCode;
    NTSTATUS Status;

    /* Get the IOCTL data from the stack pointer */
    StackPtr = IoGetCurrentIrpStackLocation(Irp);
    BufferLength = StackPtr->Parameters.DeviceIoControl.InputBufferLength;
    ControlCode = StackPtr->Parameters.DeviceIoControl.IoControlCode;

    FLT_ASSERT(ControlCode == IOCTL_FILTER_LOAD || ControlCode == IOCTL_FILTER_UNLOAD);

    /* Make sure the buffer is valid */
    if (BufferLength < sizeof(FILTER_NAME))
        return STATUS_INVALID_PARAMETER;

    /* Convert the file name buffer into a string */
    FilterName = (PFILTER_NAME)Irp->AssociatedIrp.SystemBuffer;
    Name.Length = FilterName->Length;
    Name.MaximumLength = FilterName->Length;
    Name.Buffer = (PWCH)((PCHAR)FilterName + FIELD_OFFSET(FILTER_NAME, FilterName[0]));

    /* Forward the request to our Flt routines */
    if (ControlCode == IOCTL_FILTER_LOAD)
    {
        Status = FltLoadFilter(&Name);
    }
    else
    {
        Status = FltUnloadFilter(&Name);
    }

    return Status;
}
