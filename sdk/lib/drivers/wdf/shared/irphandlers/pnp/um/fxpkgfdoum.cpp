/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgFdoUm.cpp

Abstract:

    This module implements the pnp/power package for the driver
    framework.

Author:




Environment:

    User mode only

Revision History:



--*/

#include "../pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>


#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "FxPkgFdoUm.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PnpFilterResourceRequirements(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp FilterResourceRequirements IRP.

Arguments:

    Device - a pointer to the FxDevice

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
FxPkgFdo::PnpQueryCapabilities(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryCapabilities IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    HandleQueryCapabilities(Irp);

    //
    // Set a completion routine on the IRP
    //
    Irp->CopyCurrentIrpStackLocationToNext();
    Irp->SetCompletionRoutine(
        _PnpQueryCapabilitiesCompletionRoutine,
        this
        );

    //
    // Send the IRP down the stack
    //
    Irp->CallDriver(m_Device->GetAttachedDevice());

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryCapabilitiesCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    NTSTATUS status;
    FxPkgFdo* pThis;
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxPkgFdo*) Context;
    status = irp.GetStatus();

    //
    // Now that the IRP has returned to us, we modify what the bus driver
    // set up.
    //
    if (NT_SUCCESS(status)) {
        pThis->HandleQueryCapabilitiesCompletion(&irp);
    }

    pThis->CompletePnpRequest(&irp, status);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryPnpDeviceState(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryPnpDeviceState IRP.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    FxPkgFdo* pThis;

    pThis = (FxPkgFdo*) This;

    //
    // Set a completion routine on the IRP
    //
    Irp->CopyCurrentIrpStackLocationToNext();
    Irp->SetCompletionRoutine(
        FxPkgFdo::_PnpQueryPnpDeviceStateCompletionRoutine,
        pThis
        );

    //
    // Send the IRP down the stack
    //
    Irp->CallDriver(pThis->m_Device->GetAttachedDevice());

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryPnpDeviceStateCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    NTSTATUS status;
    FxPkgFdo* pThis;
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxPkgFdo*) Context;
    status = irp.GetStatus();

    if (status == STATUS_NOT_SUPPORTED) {
        //
        // Morph into a successful code so that we process the request
        //
        status = STATUS_SUCCESS;
        irp.SetStatus(status);
    }

    if (NT_SUCCESS(status)) {
        pThis->HandleQueryPnpDeviceStateCompletion(&irp);
    }
    else {
        DoTraceLevelMessage(
            pThis->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Lower stack returned error for query pnp device state, %!STATUS!",
            status);
    }

    //
    // Since we already sent the request down the stack, we must complete it
    // now.
    //
    pThis->CompletePnpRequest(&irp, status);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::Initialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
/*++











Routine Description:

    After creating a FxPkgFdo, the driver writer will initialize it by passing
    a set of driver callbacks that allow the driver writer to customize the
    behavior when handling certain IRPs.

    This is the place to do any initialization that might fail.

Arguments:

    Device - a pointer to the FxDevice

    DispatchTable - a driver supplied table of callbacks

Returns:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pGlobals;
    NTSTATUS status;

    pGlobals = GetDriverGlobals();

    status = FxPkgPnp::Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = AllocateEnumInfo();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::AskParentToRemoveAndReenumerate(
    VOID
    )
/*++

Routine Description:
    This routine asks the PDO to ask its parent bus driver to Surprise-Remove
    and re-enumerate the PDO.  This will be done only at the point of
    catastrophic software failure, and occasionally after catastrophic hardware
    failure.

Arguments:
    None

Return Value:
    status

  --*/
{
    HRESULT hr;
    IWudfDeviceStack2* pDevStack = m_Device->GetDeviceStack2();

    hr = pDevStack->ReenumerateSelf();
    if (SUCCEEDED(hr)) {
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

