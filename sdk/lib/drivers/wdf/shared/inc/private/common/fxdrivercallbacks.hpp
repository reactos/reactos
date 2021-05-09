/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriverCallbacks.h

Abstract:

    This module implements the FxDriver object callbacks

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDRIVERCALLBACKS_H_
#define _FXDRIVERCALLBACKS_H_

//
// These delegates are in a seperate file
//

//
// DrvDeviceInitialize callback delegate
//
class FxDriverDeviceAdd : public FxLockedCallback {

public:
    PFN_WDF_DRIVER_DEVICE_ADD Method;

    FxDriverDeviceAdd(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDRIVER  Driver,
        __in PWDFDEVICE_INIT DeviceInit
        )
    {
        if (Method != NULL) {
            NTSTATUS status;
            KIRQL irql = 0;

            CallbackStart(&irql);
            status = Method(Driver, DeviceInit);
            CallbackEnd(irql);

            return status;
        }
        else {
            return STATUS_UNSUCCESSFUL;
        }
    }
};

//
// DrvUnload callback delegate
//
class FxDriverUnload : public FxCallback {

public:

    PFN_WDF_DRIVER_UNLOAD Method;

    FxDriverUnload(
        VOID
        ) :
        FxCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFDRIVER Driver
        )
    {
        if (Method != NULL) {

            CallbackStart();
            Method(Driver);
            CallbackEnd();
            return;
        }
        else {
            return;
        }
    }
};

#endif // _FXDRIVERCALLBACKS_H_
