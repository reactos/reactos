#include "common/fxpkgpdo.h"
#include "common/fxpkgpnp.h"


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