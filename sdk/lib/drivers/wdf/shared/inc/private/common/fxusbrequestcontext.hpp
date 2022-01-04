/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbRequestContext.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXUSBREQUESTCONTEXT_H_
#define _FXUSBREQUESTCONTEXT_H_

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include <umusb.h>
#endif

struct FxUsbRequestContext : public FxRequestContext {
    FxUsbRequestContext(
        __in FX_REQUEST_CONTEXT_TYPE Type
        ) :
        FxRequestContext(Type)
    {
        InitUsbParameters();
        SetUsbType(WdfUsbRequestTypeNoFormat);
    }

    virtual
    USBD_STATUS
    GetUsbdStatus(
        VOID
        ) = 0;

    virtual
    VOID
    CopyParameters(
        __in FxRequestBase* Request
        )
    {
        m_UsbParameters.UsbdStatus = GetUsbdStatus();
        FxRequestContext::CopyParameters(Request); // __super call
    }

    VOID
    SetUsbType(
        __in WDF_USB_REQUEST_TYPE Type
        )
    {
        //
        // The completion params are set every time we set the type
        //
        m_CompletionParams.Type = WdfRequestTypeUsb;
        m_CompletionParams.Parameters.Usb.Completion = &m_UsbParameters;

        m_UsbParameters.Type = Type;
    }

    VOID
    __inline
    InitUsbParameters(
        VOID
        )
    {
        RtlZeroMemory(&m_UsbParameters, sizeof(m_UsbParameters));
    }

public:
    WDF_USB_REQUEST_COMPLETION_PARAMS m_UsbParameters;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // UMURB we send to the WUDF USB Dispatcher. The dispatcher
    // extracts the encoded data and passes it to WinUsb APIs.
    //
    UMURB m_UmUrb;
#endif
};

#endif // _FXUSBREQUESTCONTEXT_H_
