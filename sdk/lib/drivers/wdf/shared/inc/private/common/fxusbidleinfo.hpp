//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXUSBIDLEINFO_H_
#define _FXUSBIDLEINFO_H_

struct FxUsbIdleInfo : public FxStump {
public:
    FxUsbIdleInfo(
        __in FxPkgPnp* PkgPnp
        ) : m_IdleCallbackEvent(NULL), m_IdleIrp(NULL)
    {
        m_CallbackInfo.IdleCallback = _UsbIdleCallback;
        m_CallbackInfo.IdleContext = PkgPnp;
        m_EventDropped = FALSE;
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        VOID
        );

    FxCREvent* m_IdleCallbackEvent;

    FxAutoIrp m_IdleIrp;

    USB_IDLE_CALLBACK_INFO m_CallbackInfo;

    //
    // used to check if UsbSelectiveSuspendCompleted event was dropped.
    //
    BOOLEAN m_EventDropped;

private:

    __drv_maxIRQL(PASSIVE_LEVEL)
    static
    VOID
    _UsbIdleCallback(
        __in PVOID Context
        );
};

#endif // _FXUSBIDLEINFO_H_
