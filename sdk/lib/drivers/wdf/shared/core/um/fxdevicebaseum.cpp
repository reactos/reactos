/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceBaseUm.cpp

Abstract:

    This is the class implementation for the base device class.

Author:



Environment:

    User mode only

Revision History:



--*/

#include "coreprivshared.hpp"

extern "C" {
#include "FxDeviceBaseUm.tmh"
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
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(InterfaceSpecificData);
    UNREFERENCED_PARAMETER(TargetDevice);

    //
    // Query interface is not implemented for UMDF yet
    //
    return STATUS_NOT_IMPLEMENTED;
}

