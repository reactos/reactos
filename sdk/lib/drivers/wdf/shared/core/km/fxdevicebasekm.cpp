/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceBaseKm.cpp

Abstract:

    This is the class implementation for the base device class.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxDeviceBaseKm.tmh"
}

_Must_inspect_result_
NTSTATUS
FxDeviceBase::QueryForInterface(
    __in const GUID* InterfaceType,
    __out PINTERFACE Interface,
    __in USHORT Size,
    __in USHORT Version,
    __in PVOID InterfaceSpecificData,
    __in_opt MdDeviceObject TargetDevice
    )
/*++

Routine Description:
    Send an IRP_MJPNP/IRP_MN_QUERY_INTERFACE irp to a device object and its
    attached stack.

Arguments:
    InterfaceType - The type of interface to query for

    Interface - The interface to fill out

    Size - Size of Interface in bytes

    Version - Version of the interface to be queried

    InterfaceSpecificData - Addtional interface data to be queried

    TargetDevice - device in the stack to send the query to.  If NULL, the top
                   of the stack will receive the query.

Return Value:
    NTSTATUS as indicated by the handler of the QI with in the device stack,
    STATUS_NOT_SUPPORTED if the QI is not handled.

  --*/
{
    PDEVICE_OBJECT pTopOfStack;
    NTSTATUS status;

    //
    // PnP rules dictate you send the QI through the entire stack and not just
    // the stack below you...but we let the caller override this.  There are
    // some stacks which are not PnP reentrant, so sending a QI from a lower
    // filter might cause the stack to stop responding if the FDO is synchronously
    // sending a PnP irp down the stack already.
    //
    if (TargetDevice == NULL) {
        pTopOfStack = GetAttachedDeviceReference();
    }
    else {
        //
        // To make the exit logic simpler below, just add our own reference.
        //
        Mx::MxReferenceObject(TargetDevice);
        pTopOfStack = TargetDevice;
    }

    status = FxQueryInterface::_QueryForInterface(
        pTopOfStack,
        InterfaceType,
        Interface,
        Size,
        Version,
        InterfaceSpecificData
        );

    Mx::MxDereferenceObject(pTopOfStack);

    return status;
}

