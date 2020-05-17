#ifndef _FXPACKAGE_H_
#define _FXPACKAGE_H_

#include "common/fxnonpagedobject.h"
#include "common/fxirp.h"

class FxPackage : public FxNonPagedObject
{
public:

    FxPackage(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device,
        __in WDFTYPE Type
        ) :
        // By passing 0, we are indicating we are an internal object which will not
        // represented as a WDFHANDLE
        FxNonPagedObject(Type, 0, FxDriverGlobals)
    {
        m_Device = Device;
    }

    virtual
    NTSTATUS
    NTAPI
    Dispatch(
        __in MdIrp Irp
        ) = 0;

    __inline
    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }

    DECLARE_INTERNAL_NEW_OPERATOR();
};

#endif // _FXPACKAGE_H_