/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgPdoUM.cpp

Abstract:

    This module implements the Pnp package for Pdo devices.

Author:

Environment:

    User mode only

Revision History:

--*/

#include "../pnppriv.hpp"
#include <wdmguid.h>

// Tracing support
#if defined(EVENT_TRACING)
extern "C" {
#include "FxPkgPdoUM.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryResources(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryResources(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryResources(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryResources IRP.  We return
    the resources that the device is currently consuming.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(Irp);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryResourceRequirements(
        __inout FxPkgPnp* This,
        __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryResourceRequirements(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryResourceRequirements(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryResourceRequirements IRP.
    We return the set (of sets) of possible resources that we could accept
    which would allow our device to work.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(Irp);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpFilterResourceRequirements(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    Filter resource requirements for the PDO.  A chance to further muck with
    the resources assigned to the device.

Arguments:
    This - the package

    Irp - the request

Return Value:
    NTSTATUS

  --*/
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Irp);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

