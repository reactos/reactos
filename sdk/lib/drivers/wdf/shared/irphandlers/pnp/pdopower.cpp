/*++

Copyright (c) Microsoft Corporation

Module Name:

    PdoPower.cpp

Abstract:

    This module implements the Pnp package for Pdo devices.

Author:




Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

// Tracing support
#if defined(EVENT_TRACING)
extern "C" {
#include "PdoPower.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_DispatchPowerSequence(
    __inout FxPkgPnp* This,
    __in FxIrp *Irp
    )
/*++

Routine Description:
    report the power sequence for the child

Arguments:
    This - the package

    Irp - the request

Return Value:
    STATUS_NOT_SUPPORTED

  --*/
{
    return ((FxPkgPdo*) This)->CompletePowerRequest(Irp, STATUS_NOT_SUPPORTED);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_DispatchSetPower(
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
        return ((FxPkgPdo*) This)->DispatchSystemSetPower(Irp);
    }
    else {
        return ((FxPkgPdo*) This)->DispatchDeviceSetPower(Irp);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::DispatchSystemSetPower(
    __in FxIrp *Irp
    )
{
    KIRQL irql;
    MxDeviceObject deviceObject(m_Device->GetDeviceObject());

    m_SystemPowerState = (BYTE) Irp->GetParameterPowerStateSystemState();
    deviceObject.SetPowerState(SystemPowerState,
                               Irp->GetParameterPowerState());

    if (IsPowerPolicyOwner()) {
        if (m_SystemPowerState == PowerSystemWorking) {
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
            PowerPolicyProcessEvent(PwrPolS0);
            Mx::MxLowerIrql(irql);

            return CompletePowerRequest(Irp, STATUS_SUCCESS);
        }
        else {
            //
            // Power policy state machine will complete the request later
            //
            SetPendingSystemPowerIrp(Irp);
            PowerPolicyProcessEvent(PwrPolSx);
            return STATUS_PENDING;
        }
    }
    else {
        //
        // Since we are not the power policy owner, we just complete all S irps
        //
        return CompletePowerRequest(Irp, STATUS_SUCCESS);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::DispatchDeviceSetPower(
    __in FxIrp *Irp
    )
{
    if (IsPowerPolicyOwner()) {
        if (m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp == FALSE &&
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp == FALSE) {
            //
            // A power irp arrived, but we did not request it.  ASSERT and log
            // an error.
            //
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Received set device power irp 0x%p on WDFDEVICE 0x%p !devobj 0x%p, "
                "but the irp was not requested by the device (the power policy owner)",
                Irp->GetIrp(),
                m_Device->GetHandle(),
                m_Device->GetDeviceObject());

            ASSERTMSG("Received set device power irp but the irp was not "
                "requested by the device (the power policy owner)\n",
                FALSE);
        }

        //
        // We are no longer requesting a power irp because we received the one
        // we requested.
        //
        if (m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp) {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = FALSE;
        }
        else {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = FALSE;
        }
    }

    SetPendingDevicePowerIrp(Irp);

    if (Irp->GetParameterPowerStateDeviceState() == PowerDeviceD0) {
        PowerProcessEvent(PowerD0);
    }
    else {
        PowerProcessEvent(PowerDx);
    }

    return STATUS_PENDING;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_DispatchQueryPower(
    __inout FxPkgPnp* This,
    __in FxIrp *Irp
    )
/*++

Routine Description:
    Dispatches query power for system and device requests

Arguments:
    This - the package

    Irp - the query power request

Return Value:
    NTSTATUS

  --*/
{
    FxPkgPdo* pThis;
    NTSTATUS status;

    pThis = ((FxPkgPdo*) This);

    if (Irp->GetParameterPowerType() == SystemPowerState
        &&
        This->PowerPolicyIsWakeEnabled()) {

        status = pThis->PowerPolicyHandleSystemQueryPower(
            Irp->GetParameterPowerStateSystemState()
            );
    }
    else {
        status = STATUS_SUCCESS;
    }

    return pThis->CompletePowerRequest(Irp, status);
}

VOID
FxPkgPdo::PowerReleasePendingDeviceIrp(
    __in BOOLEAN IrpMustBePresent
    )
{
    MdIrp pIrp;

    pIrp = ClearPendingDevicePowerIrp();

    UNREFERENCED_PARAMETER(IrpMustBePresent);
    ASSERT(IrpMustBePresent == FALSE || pIrp != NULL);

    if (pIrp != NULL) {
        FxIrp irp(pIrp);

        CompletePowerRequest(&irp, STATUS_SUCCESS);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PowerCheckParentOverload(
    __in BOOLEAN* ParentOn
    )
/*++

Routine Description:
    This function implements the CheckParent state.  Its
    job is to determine which state we should go to next based on whether
    the parent is in D0.

Arguments:
    none

Return Value:

    VOID

--*/
{




    return (m_Device->m_ParentDevice->m_PkgPnp)->
        PowerPolicyCanChildPowerUp(ParentOn);
}

WDF_DEVICE_POWER_STATE
FxPkgPdo::PowerCheckDeviceTypeOverload(
    VOID
    )
/*++

Routine Description:
    This function implements the Check Type state.  This is a PDO.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return WdfDevStatePowerCheckParentState;
}

WDF_DEVICE_POWER_STATE
FxPkgPdo::PowerCheckDeviceTypeNPOverload(
    VOID
    )
/*++

Routine Description:
    This function implements the Check Type state.  This is a PDO.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return WdfDevStatePowerCheckParentStateNP;
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PowerEnableWakeAtBusOverload(
    VOID
    )
/*++

Routine Description:
    Arms the device at the bus level for wake.  This arming is generic since
    the bus driver can only configure the device generically.  The power policy
    owner has already armed the device for wake in a device specific fashion
    when it processed the wake irp (EvtDeviceArmDeviceForWakeFromS0/x if the ppo
    is a WDF driver).

Arguments:
    None

Return Value:
    NTSTATUS, !NT_SUCCESS if the arm failed

  --*/
{
    NTSTATUS status;

    //
    // The EnableWakeAtBus callback should not be called twice in a row without
    // an intermediate call to the DisableWakeAtBus callback.
    //
    ASSERT(m_EnableWakeAtBusInvoked == FALSE);

    status = m_DeviceEnableWakeAtBus.Invoke(
        m_Device->GetHandle(),
        (SYSTEM_POWER_STATE) m_SystemPowerState
        );

    if (NT_SUCCESS(status)) {
        m_EnableWakeAtBusInvoked = TRUE;
        PowerNotifyParentChildWakeArmed();
    }

    return status;
}

VOID
FxPkgPdo::PowerDisableWakeAtBusOverload(
    VOID
    )
/*++

Routine Description:
    Disarms the device at the bus level for wake.  This disarming is generic
    since the bus driver can only configure the device generically.  The power
    policy owner may have already disarmed the device for wake in a device
    specific fashion.  For a WDF ppo EvtDeviceDisarmDeviceForWakeFromS0/x is
    called after the bus has disarmed.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_EnableWakeAtBusInvoked) {
        m_EnableWakeAtBusInvoked = FALSE;
        PowerNotifyParentChildWakeDisarmed();

        m_DeviceDisableWakeAtBus.Invoke(m_Device->GetHandle());
    }
}

VOID
FxPkgPdo::PowerParentPowerDereference(
    VOID
    )
/*++

Routine Description:
    Releases the child power reference on the parent device.  This allows the
    parent to enter into an idle capable state.  This power reference does not
    prevent the parent from moving into Dx when the system power state changes.

Arguments:
    None

Return Value:
    None

  --*/
{

    m_Device->m_ParentDevice->m_PkgPnp->PowerPolicyChildPoweredDown();
}

VOID
FxPkgPdo::PowerNotifyParentChildWakeArmed(
    VOID
    )
/*++

Routine Description:
    Notifies the parent device that the child is armed for wake.  This will
    cause the parent to increment its count of children armed for wake.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxPowerPolicyOwnerSettings* settings;

    ASSERT(m_Device->m_ParentDevice != NULL);





    settings = m_Device->m_ParentDevice->m_PkgPnp->
        m_PowerPolicyMachine.m_Owner;
    if (settings != NULL) {
        settings->IncrementChildrenArmedForWakeCount();
    }
}

VOID
FxPkgPdo::PowerNotifyParentChildWakeDisarmed(
    VOID
    )
/*++

Routine Description:
    Notifies the parent device that the child is not armed for wake.  This will
    cause the parent to decrement its count of children armed for wake.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxPowerPolicyOwnerSettings* settings;

    ASSERT(m_Device->m_ParentDevice != NULL);





    settings = m_Device->m_ParentDevice->m_PkgPnp->
        m_PowerPolicyMachine.m_Owner;
    if (settings != NULL) {
        settings->DecrementChildrenArmedForWakeCount();
    }
}
