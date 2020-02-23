#include "common/fxdefaultirphandler.h"
#include "common/fxdevice.h"

FxDefaultIrpHandler::FxDefaultIrpHandler(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPackage(FxDriverGlobals, Device, FX_TYPE_DEFAULT_IRP_HANDLER)
{
}

_Must_inspect_result_
NTSTATUS
FxDefaultIrpHandler::Dispatch(
    __in MdIrp Irp
    )
{
    UCHAR major, minor;
    FxIrp fxIrp(Irp);

    major = fxIrp.GetMajorFunction();
    minor = fxIrp.GetMinorFunction();

    if (FxDevice::_RequiresRemLock(major, minor) == FxDeviceRemLockRequired)
    {
        if (major == IRP_MJ_POWER)
        {
            //
            // Required to be called before the power irp is completed
            //
            fxIrp.StartNextPowerIrp();
        }

        fxIrp.SetStatus(STATUS_INVALID_DEVICE_REQUEST);
        fxIrp.SetInformation(0);

        fxIrp.CompleteRequest(IO_NO_INCREMENT);

        //
        // If we are in this function, m_Device is still valid, so we can use it
        // to get the WDM remove lock and not have to get it out of the
        // device object from current irp stack location.
        //
        Mx::MxReleaseRemoveLock(m_Device->GetRemoveLock(), Irp);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (m_Device->IsFilter())
    {
        fxIrp.SkipCurrentIrpStackLocation();
        return fxIrp.CallDriver(m_Device->GetAttachedDevice());
    }
    else
    {
        fxIrp.SetStatus(STATUS_INVALID_DEVICE_REQUEST);
        fxIrp.SetInformation(0);

        fxIrp.CompleteRequest(IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}