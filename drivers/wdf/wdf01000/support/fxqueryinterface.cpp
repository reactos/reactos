#include "common/fxqueryinterface.h"
#include "common/fxirp.h"


_Must_inspect_result_
NTSTATUS
FxQueryInterface::_QueryForInterface(
    __in PDEVICE_OBJECT TopOfStack,
    __in const GUID* InterfaceType,
    __out PINTERFACE Interface,
    __in USHORT Size,
    __in USHORT Version,
    __in_opt PVOID InterfaceSpecificData
    )
/*++

Routine Description:
    Send an IRP_MJPNP/IRP_MN_QUERY_INTERFACE irp to a device object and its
    attached stack.

Arguments:
    TargetDevice - device to send the query to.

    InterfaceType - The type of interface to query for

    Interface - The interface to fill out

    Size - Size of Interface in bytes

    Version - Version of the interface to be queried

    InterfaceSpecificData - Addtional interface data to be queried


Return Value:
    NTSTATUS as indicated by the handler of the QI with in the device stack,
    STATUS_NOT_SUPPORTED if the QI is not handled.

  --*/
{
    PIRP pIrp;
    NTSTATUS status;

    pIrp = IoAllocateIrp(TopOfStack->StackSize, FALSE);

    if (pIrp != NULL)
    {
        FxAutoIrp irp(pIrp);

        _FormatIrp(
            pIrp,
            InterfaceType,
            Interface,
            Size,
            Version,
            InterfaceSpecificData
            );

        status = irp.SendIrpSynchronously(TopOfStack);
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID
FxQueryInterface::_FormatIrp(
    __in PIRP Irp,
    __in const GUID* InterfaceGuid,
    __out PINTERFACE Interface,
    __in USHORT InterfaceSize,
    __in USHORT InterfaceVersion,
    __in_opt PVOID InterfaceSpecificData
    )
{
    PIO_STACK_LOCATION stack;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    stack = IoGetNextIrpStackLocation(Irp);

    stack->MajorFunction = IRP_MJ_PNP;
    stack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    stack->Parameters.QueryInterface.Interface = Interface;
    stack->Parameters.QueryInterface.InterfaceSpecificData = InterfaceSpecificData;
    stack->Parameters.QueryInterface.Size = InterfaceSize;
    stack->Parameters.QueryInterface.Version = InterfaceVersion;
    stack->Parameters.QueryInterface.InterfaceType = InterfaceGuid;
}