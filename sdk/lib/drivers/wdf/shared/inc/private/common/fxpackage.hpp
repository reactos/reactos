/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxPackage.hpp

Abstract:

    This is the definition of the FxPackage object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPACKAGE_H_
#define _FXPACKAGE_H_

class FxPackage : public FxNonPagedObject
{
public:

    FxPackage(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device,
        __in WDFTYPE Type
        );

    virtual
    NTSTATUS
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
