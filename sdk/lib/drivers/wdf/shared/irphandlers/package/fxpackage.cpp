/*++

Copyright (c) Microsoft Corporation

Module Name:

    package.cpp

Abstract:

    This module implements the base package class.  Other packages will
    derive from this base class.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/


#include "shared/irphandlers/irphandlerspriv.hpp"

#include "fxpackage.hpp"

FxPackage::FxPackage(
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
