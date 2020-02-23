#include "common/fxpkgfdo.h"
#include "common/fxdevice.h"


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
    MdDeviceObject pDevObj;
    NTSTATUS status;
    MdIrp pIrp;

    pIrp = Irp->GetIrp();
    pThis = (FxPkgFdo*) This;

    pDevObj = pThis->m_Device->GetDeviceObject();

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

VOID
FxPkgFdo::PowerReleasePendingDeviceIrp(
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

        if (irp.GetParameterPowerStateDeviceState() == PowerDeviceD0)
        {
            //
            // We catch D0 irps on the way up, so complete it
            //
            CompletePowerRequest(&irp, STATUS_SUCCESS);
        }
        else
        {
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
