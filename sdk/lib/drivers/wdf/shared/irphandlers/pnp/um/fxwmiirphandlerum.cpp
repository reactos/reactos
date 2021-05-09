/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiIrpHandlerUm.cpp

Abstract:

    This module implements the wmi irp handler for the driver frameworks.

Author:




Environment:

    User mode only

Revision History:

--*/

#include "fxmin.hpp"
#include "FxWmiIrpHandler.hpp"

class  FxWmiIrpHandler;

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::PostCreateDeviceInitialize(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}


VOID
FxWmiIrpHandler::Deregister(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::AddPowerPolicyProviderAndInstance(
    __in    PWDF_WMI_PROVIDER_CONFIG /* ProviderConfig */,
    __in    FxWmiInstanceInternalCallbacks* /* InstanceCallbacks */,
    __inout FxWmiInstanceInternal** /* Instance */
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::Register(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxWmiIrpHandler::Cleanup(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
FxWmiIrpHandler::ResetStateForPdoRestart(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}


