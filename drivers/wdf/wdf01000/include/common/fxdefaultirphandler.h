#ifndef _FXDEFAULTIRPHANDLER_H_
#define _FXDEFAULTIRPHANDLER_H_

#include "common/fxpackage.h"

class FxDefaultIrpHandler : public FxPackage {
public:
    FxDefaultIrpHandler(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice* Device
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    NTAPI
    Dispatch(
        __in MdIrp Irp
        );
};

#endif // _FXDEFAULTIRPHANDLER_H_
