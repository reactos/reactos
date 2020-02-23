#ifndef _FXREQUESTCONTEXT_H_
#define _FXREQUESTCONTEXT_H_

#include "common/fxstump.h"
#include "wdf.h"

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

//typedef struct _FxInternalIoctlParams{
//    WDFMEMORY  Argument1;
//    WDFMEMORY  Argument2;
//    WDFMEMORY  Argument4;
//}FxInternalIoctlParams,*pFxInternalIoctlParams;

class FxRequestBase;

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
    CopyParameters(
        __in FxRequestBase* Request
        )
    {
        UNREFERENCED_PARAMETER(Request);
    }

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

public:
    WDF_REQUEST_COMPLETION_PARAMS m_CompletionParams;

};

#endif //_FXREQUESTCONTEXT_H_