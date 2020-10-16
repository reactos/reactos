/*++

Copyright (c) Microsoft Corporation

Module Name:

    FdoPower.cpp

Abstract:

    This module implements the pnp/power package for the driver
    framework.  This, specifically, is the power code.

Author:




Environment:

    Both kernel and user mode

Revision History:




--*/

#include "pnppriv.hpp"




#if defined(EVENT_TRACING)
extern "C" {
#include "FdoPower.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PowerPassDown(
    __inout FxPkgPnp* This,
    __in FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked when a Power Irp we don't handle comes into the
    driver.

Arguemnts:

    This - the package

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    FxPkgFdo* pThis;
    NTSTATUS status;
    MdIrp pIrp;

    pIrp = Irp->GetIrp();
    pThis = (FxPkgFdo*) This;

    //
    // FDOs don't handle this IRP, so simply pass it down.
    //
    Irp->StartNextPowerIrp();
    Irp->CopyCurrentIrpStackLocationToNext();

    status = Irp->PoCallDriver(pThis->m_Device->GetAttachedDevice());

    Mx::MxReleaseRemoveLock(pThis->m_Device->GetRemoveLock(),
                            pIrp);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_DispatchSetPower(
    __inout FxPkgPnp* This,
    __in FxIrp *Irp
    )
/*++

Routine Description:

    This method is invoked when a SetPower IRP enters the driver.

Arguemnts:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    if (Irp->GetParameterPowerType() == SystemPowerState) {
        return ((FxPkgFdo*) This)->DispatchSystemSetPower(Irp);
    }
    else {
        return ((FxPkgFdo*) This)->DispatchDeviceSetPower(Irp);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_DispatchQueryPower(
    __inout FxPkgPnp* This,
    __in FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked when a QueryPower IRP enters the driver.

Arguemnts:

    This - The package

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    if (Irp->GetParameterPowerType() == SystemPowerState) {
        return ((FxPkgFdo*) This)->DispatchSystemQueryPower(Irp);
    }
    else {
        return ((FxPkgFdo*) This)->DispatchDeviceQueryPower(Irp);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_SystemPowerS0Completion(
    __in MdDeviceObject DeviceObject,
    __in MdIrp OriginalIrp,
    __in PVOID Context
    )
{
    FxPkgPnp* pPkgPnp;
    KIRQL irql;
    FxIrp irp(OriginalIrp);

    pPkgPnp = (FxPkgPnp*) Context;

    //
    // Ideally we would like to complete the S0 irp before we start
    // processing the event in the state machine so that the D0 irp
    // comes after the S0 is moving up the stack...
    //
    // ... BUT ...
    //
    // ... by allowing the S0 irp to go up the stack first, we must then
    // handle pnp requests from the current power policy state (because
    // the S0 irp could be the last S irp in the system and when completed,
    // the pnp lock is released).  So, we process the event first so
    // that we can move into a state where we can handle pnp events in
    // the power policy state machine.
    //
    // We mitigate the situation a little bit by forcing the processing of the
    // event to occur on the power policy thread rather then in the current
    // context.
    //
    Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
    pPkgPnp->PowerPolicyProcessEvent(PwrPolS0);
    Mx::MxLowerIrql(irql);

    irp.StartNextPowerIrp();

    //
    // Let the irp continue on its way
    //
    if (irp.PendingReturned()) {
        irp.MarkIrpPending();
    }

    Mx::MxReleaseRemoveLock((&FxDevice::_GetFxWdmExtension(DeviceObject)->IoRemoveLock),
                            OriginalIrp);

    return irp.GetStatus();
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_SystemPowerSxCompletion(
    __in MdDeviceObject DeviceObject,
    __in MdIrp OriginalIrp,
    __in PVOID Context
    )
{
    FxPkgFdo *pThis;
    FxIrp irp(OriginalIrp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxPkgFdo*) Context;

    ASSERT(pThis->IsPowerPolicyOwner());
    ASSERT(OriginalIrp == pThis->GetPendingSystemPowerIrp());

    pThis->PowerPolicyProcessEvent(PwrPolSx);

    //
    // Power policy will complete the system irp
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::DispatchSystemSetPower(
    __in FxIrp *Irp
    )
{
    NTSTATUS status;
    MxDeviceObject deviceObject(m_Device->GetDeviceObject());

    m_SystemPowerState = (BYTE) Irp->GetParameterPowerStateSystemState();
    deviceObject.SetPowerState(SystemPowerState,
                               Irp->GetParameterPowerState());

    if (IsPowerPolicyOwner()) {
        //
        // If we are going to S0, we just notify the power policy state machine
        // and then let the request go (per the fast resume spec).  Otherwise,
        // send the request down and on the way up, send the Dx request.
        //
        if (m_SystemPowerState == PowerSystemWorking) {
            //
            // Post the event into the state machine when the irp is going up
            // the stack.  See the comment in _SystemPowerS0Completion for more
            // detail as to why.
            //
            Irp->CopyCurrentIrpStackLocationToNext();
            Irp->SetCompletionRoutineEx(deviceObject.GetObject(),
                                        _SystemPowerS0Completion,
                                        this);

            return Irp->PoCallDriver(m_Device->GetAttachedDevice());
        }
        else {
            //
            // Stash away the irp for the power policy state machine.  We will
            // post the event to the power policy state machine when the S irp
            // completes back to this driver.
            //
            SetPendingSystemPowerIrp(Irp);

            Irp->CopyCurrentIrpStackLocationToNext();
            Irp->SetCompletionRoutineEx(deviceObject.GetObject(),
                                        _SystemPowerSxCompletion,
                                        this);

            Irp->PoCallDriver(m_Device->GetAttachedDevice());

            status = STATUS_PENDING;
        }
    }
    else {
        //
        // We don't do anything with S irps if we are not the power policy
        // owner.
        //
        // This will release the remove lock as well.
        //
        status = _PowerPassDown(this, Irp);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::DispatchDeviceSetPower(
    __in FxIrp *Irp
    )

{
    NTSTATUS status;

    if (IsPowerPolicyOwner()) {
        if (m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp == FALSE &&
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp == FALSE) {
            //
            // A power irp arrived, but we did not request it.  log and bugcheck
            //
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Received set device power irp 0x%p on WDFDEVICE 0x%p !devobj 0x%p, "
                "but the irp was not requested by the device (the power policy owner)",
                Irp->GetIrp(), m_Device->GetHandle(),
                m_Device->GetDeviceObject());

            FxVerifierBugCheck(GetDriverGlobals(),  // globals
                   WDF_POWER_MULTIPLE_PPO, // specific type
                   (ULONG_PTR)m_Device->GetDeviceObject(), //parm 2
                   (ULONG_PTR)Irp->GetIrp());  // parm 3

            /* NOTREACHED */
        }

        //
        // We are no longer requesting a power irp because we received the one
        // we requested.
        //
        if (m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp) {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = FALSE;
        } else {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = FALSE;
        }
    }

    //
    // Determine if we are raising or lowering the device power state.
    //
    if (Irp->GetParameterPowerStateDeviceState() == PowerDeviceD0) {
        status = RaiseDevicePower(Irp);
    }
    else {
        status = LowerDevicePower(Irp);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::RaiseDevicePower(
    __in FxIrp *Irp
    )
{
    Irp->MarkIrpPending();
    Irp->CopyCurrentIrpStackLocationToNext();
    Irp->SetCompletionRoutineEx(m_Device->GetDeviceObject(),
                                RaiseDevicePowerCompletion,
                                this);

    Irp->PoCallDriver(m_Device->GetAttachedDevice());

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::RaiseDevicePowerCompletion(
    __in MdDeviceObject DeviceObject,
    __in MdIrp OriginalIrp,
    __in PVOID Context
    )
{
    FxPkgFdo* pThis;
    FxIrp irp(OriginalIrp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxPkgFdo*) Context;

    //
    // We can safely cache away the device power irp in our fdo package
    // storage because we know we can only get one device power irp at
    // a time.
    //
    pThis->SetPendingDevicePowerIrp(&irp);

    //
    // Kick off the power state machine.
    //
    pThis->PowerProcessEvent(PowerD0);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::LowerDevicePower(
    __in FxIrp *Irp
    )
{
    SetPendingDevicePowerIrp(Irp);

    //
    // Kick off the power state machine.
    //
    PowerProcessEvent(PowerDx);

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::DispatchSystemQueryPower(
    __in FxIrp *Irp
    )
{
    if (PowerPolicyIsWakeEnabled()) {
        NTSTATUS status;

        status = PowerPolicyHandleSystemQueryPower(
            Irp->GetParameterPowerStateSystemState()
            );

        Irp->SetStatus(status);

        if (!NT_SUCCESS(status)) {
            return CompletePowerRequest(Irp, status);
        }
    }

    //
    // Passing down the irp because one of the following
    // a) We don't care b/c we don't control the power policy
    // b) we are not enabled for arming for wake from Sx
    // c) we can wake from the queried S state
    //
    return _PowerPassDown(this, Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::DispatchDeviceQueryPower(
    __in FxIrp *Irp
    )
{
    //
    // Either the framework is the power policy owner and we wouldn't be sending
    // a device query power or we are a subordinate will do what the power
    // policy owner wants 100% of the time.
    //
    Irp->SetStatus(STATUS_SUCCESS);

    //
    // This will release the remove lock
    //
    return _PowerPassDown(this, Irp);
}

VOID
FxPkgFdo::PowerReleasePendingDeviceIrp(
    __in BOOLEAN IrpMustBePresent
    )
{
    MdIrp pIrp;

    pIrp = ClearPendingDevicePowerIrp();

    UNREFERENCED_PARAMETER(IrpMustBePresent);
    ASSERT(IrpMustBePresent == FALSE || pIrp != NULL);

    if (pIrp != NULL) {
        FxIrp irp(pIrp);

        if (irp.GetParameterPowerStateDeviceState() == PowerDeviceD0) {
            //
            // We catch D0 irps on the way up, so complete it
            //
            CompletePowerRequest(&irp, STATUS_SUCCESS);
        }
        else {
            irp.SetStatus(STATUS_SUCCESS);

            //
            // We catch Dx irps on the way down, so send it on its way
            //
            // This will also release the remove lock
            //
            (void) _PowerPassDown(this, &irp);
        }
    }
}

WDF_DEVICE_POWER_STATE
FxPkgFdo::PowerCheckDeviceTypeOverload(
    VOID
    )
/*++

Routine Description:
    This function implements the Check Type state.  This is FDO code,
    so the answer is reductionistly simple.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return WdfDevStatePowerWaking;
}

WDF_DEVICE_POWER_STATE
FxPkgFdo::PowerCheckDeviceTypeNPOverload(
    VOID
    )
/*++

Routine Description:
    This function implements the Check Type state.  This is FDO code,
    so the answer is reductionistly simple.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return WdfDevStatePowerWakingNP;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PowerCheckParentOverload(
    __out BOOLEAN* ParentOn
    )
/*++

Routine Description:
    This function implements the CheckParent state.  Its
    job is to determine which state we should go to next based on whether
    the parent is in D0.  But since this is the FDO code, we can't know
    that.  So just assume that the PDO will guarantee it.

Arguments:
    none

Return Value:

    new power state

--*/
{
    ASSERT(!"This state shouldn't be reachable for an FDO.");
    *ParentOn = TRUE;
    return STATUS_SUCCESS;
}

VOID
FxPkgFdo::PowerParentPowerDereference(
    VOID
    )
/*++

Routine Description:
    This virtual function is a nop for an FDO.  PDOs implement this function

Arguments:
    None

Return Value:
    None

  --*/
{
    DO_NOTHING();
}
