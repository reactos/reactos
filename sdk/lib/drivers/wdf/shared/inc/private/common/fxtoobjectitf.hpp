/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxToObjectItf.hpp

Abstract:

    This file contains the funcionality exposed by framework to object
    (Framework to Object Interface)

Author:

Revision History:

--*/

#ifndef _FXTOOBJECTITF_H
#define _FXTOOBJECTITF_H

extern "C" {
////////////////////////////////////////////////
//To be implemented by respective frameworks
////////////////////////////////////////////////

class FxToObjectItf
{
public:
    static
    VOID
    FxAddToDisposeList(
        __in CfxDeviceBase* DeviceBase,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject * ObjectToAdd
        );

    static
    VOID
    FxAddToDriverDisposeList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject * ObjectToAdd
        );





    static
    FxObject *
    FxGetDriverAsDefaultParent(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject * Object
        );
};
////////////////////////////////////////////////
} //extern "C"

#endif //_FXTOOBJECTITF_H
