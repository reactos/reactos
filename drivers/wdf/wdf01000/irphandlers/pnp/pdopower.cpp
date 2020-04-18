#include "common/fxpkgpdo.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"


VOID
FxPkgPdo::PowerReleasePendingDeviceIrp(
    __in BOOLEAN IrpMustBePresent
    )
{
    MdIrp pIrp;

    pIrp = ClearPendingDevicePowerIrp();

    UNREFERENCED_PARAMETER(IrpMustBePresent);
    ASSERT(IrpMustBePresent == FALSE || pIrp != NULL);

    if (pIrp != NULL)
    {
        FxIrp irp(pIrp);
        
        CompletePowerRequest(&irp, STATUS_SUCCESS);
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}
