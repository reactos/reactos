/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxQueryInterfaceUm.cpp

Abstract:

    This module implements the device interface object.

Author:




Environment:

    User mode only

Revision History:

--*/

#include <fxmin.hpp>

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced parameter

FxQueryInterface::FxQueryInterface(
    __in CfxDevice* Device,
    __in PWDF_QUERY_INTERFACE_CONFIG Config
    ) :
    m_Device(Device),
    m_Interface(NULL)
{
    UfxVerifierTrapNotImpl();
}

FxQueryInterface::~FxQueryInterface()
{
    UfxVerifierTrapNotImpl();
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
    UfxVerifierTrapNotImpl();
}

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
    UfxVerifierTrapNotImpl();

    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxQueryInterface::SetEmbedded(
    __in PWDF_QUERY_INTERFACE_CONFIG Config,
    __in PINTERFACE Interface
    )
/*++

Routine Description:
    Marks the structure as embedded and sets the configuration.  This is used
    for FxQueryInterface structs which are embedded in other structures because
    at contruction time the Config is not available yet.

    By marking as embedded, FxPkgPnp will not free the structure when it deletes
    the query interface chain.

Arguments:
    Config - how the interface behaves

    Interface - the interface that is exported

Return Value:
    None

  --*/
{
    UfxVerifierTrapNotImpl();
}

#pragma warning(pop)
