/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxRequestContext.hpp

Abstract:

    Defines the base structure for contexts associated with FxRequest

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXREQUESTCONTEXT_H_
#define _FXREQUESTCONTEXT_H_

typedef UCHAR FX_REQUEST_CONTEXT_TYPE;

//
// IO_STACK_LOCATION::Parameters.Others.Argument3 is taken up by the IOCTL
// value, so there are only 3
//
#define FX_REQUEST_NUM_OTHER_PARAMS (3)

//
// Here are all the derivations off of FxRequestContext
//
// FxIoContext
// FxInternalIoctlOthersContext
// FxUsbRequestContext
//   +FxUsbDeviceVendorContext
//   +FxUsbDeviceStatusContext
//   +FxUsbDeviceFeatureContext
//   +FxUsbPipeTransferContext
//   +FxUsbUrbContext
//   +FxUsbPipeRequestContext
//   +FxUsbPipeControlTransferContext
//

typedef struct _FxInternalIoctlParams{
    WDFMEMORY  Argument1;
    WDFMEMORY  Argument2;
    WDFMEMORY  Argument4;
}FxInternalIoctlParams,*pFxInternalIoctlParams;

struct FxRequestContext : public FxStump {

    FxRequestContext(
        __in FX_REQUEST_CONTEXT_TYPE Type
        );

    virtual
    ~FxRequestContext(
        VOID
        );

    virtual
    VOID
    Dispose(
        VOID
        )
    {}

    virtual
    VOID
    StoreAndReferenceMemory(
        __in FxRequestBuffer* Buffer
        );

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

    __inline
    BOOLEAN
    IsType(
        __in FX_REQUEST_CONTEXT_TYPE Type
        )
    {
        return m_RequestType == Type;
    }

    virtual
    VOID
    CopyParameters(
        __in FxRequestBase* Request
        )
    {
        UNREFERENCED_PARAMETER(Request);
    }

    VOID
    FormatWriteParams(
        __in_opt IFxMemory* WriteMemory,
        __in_opt PWDFMEMORY_OFFSET WriteOffsets
        );

    VOID
    FormatReadParams(
        __in_opt IFxMemory* ReadMemory,
        __in_opt PWDFMEMORY_OFFSET ReadOffsets
        );

    VOID
    FormatOtherParams(
        __in FxInternalIoctlParams *InternalIoctlParams
        );

protected:
    static
    VOID
    _StoreAndReferenceMemoryWorker(
        __in PVOID Tag,
        __deref_out_opt IFxMemory** PPMemory,
        __in FxRequestBuffer* Buffer
        );

    VOID
    __inline
    InitCompletionParams(
        VOID
        )
    {
        WDF_REQUEST_COMPLETION_PARAMS_INIT(&m_CompletionParams);
        m_CompletionParams.Type = WdfRequestTypeNoFormat;
    }

public:
    WDF_REQUEST_COMPLETION_PARAMS m_CompletionParams;

    //
    // Memory associated with the context that will be released when the
    // FxRequestBase has been completed by the target.
    //
    IFxMemory* m_RequestMemory;

    //
    // RTTI replacement
    //
    FX_REQUEST_CONTEXT_TYPE m_RequestType;
};

#endif // _FXREQUESTCONTEXT_H_
