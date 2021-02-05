/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceCallbacks.h

Abstract:

    This module implements the FxDevice object callbacks

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDEVICECALLBACKS_H_
#define _FXDEVICECALLBACKS_H_

//
// These delegates are in a seperate file
//

//
// DrvDeviceInitialize callback delegate
//
class FxIoInCallerContext : public FxCallback {

public:
    PFN_WDF_IO_IN_CALLER_CONTEXT m_Method;

    FxIoInCallerContext(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) : FxCallback(FxDriverGlobals), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device,
        __in WDFREQUEST Request
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device, Request);
            CallbackEnd();
        }
    }
};



#endif // _FXDEVICECALLBACKS_H_
