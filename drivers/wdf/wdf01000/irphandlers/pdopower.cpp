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
