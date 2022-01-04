//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXDEFAULTIRPHANDLER_HPP_
#define _FXDEFAULTIRPHANDLER_HPP_

class FxDefaultIrpHandler : public FxPackage {
public:
    FxDefaultIrpHandler(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice* Device
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    Dispatch(
        __in MdIrp Irp
        );
};

#endif // _FXDEFAULTIRPHANDLER_HPP_
