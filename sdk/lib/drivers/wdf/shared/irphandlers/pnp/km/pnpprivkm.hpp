//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _PNPPRIVKM_H_
#define _PNPPRIVKM_H_

// public headers
#include "WdfDmaEnabler.h"

// private headers
#include "FxIrpQueue.hpp"
#include "FxCallback.hpp"

// <FxSystemWorkItem.hpp>
__drv_functionClass(EVT_SYSTEMWORKITEM)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_maxFunctionIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
typedef
VOID
EVT_SYSTEMWORKITEM(
    __in PVOID Parameter
    );

typedef EVT_SYSTEMWORKITEM *PFN_WDF_SYSTEMWORKITEM;

// </FxSystemWorkItem.hpp>

#include "FxCallbackSpinlock.hpp"
#include "FxCallbackMutexLock.hpp"
#include "FxPackage.hpp"
#include "IfxMemory.hpp"
#include "FxCallback.hpp"
#include "FxRequestContext.hpp"
#include "FxRequestContextTypes.h"
#include "FxRequestBase.hpp"
#include "FxRequest.hpp"
#include "FxPkgPnp.hpp"
#include "FxPkgIo.hpp"
#include "FxIoQueue.hpp"

#include "FxDmaEnabler.hpp"
#include "FxSystemWorkItem.hpp"

#include "FxDsf.h"    // DSF support.
#include <device_common.h>
#include "FxTelemetry.hpp"

_Must_inspect_result_
NTSTATUS
SendDeviceUsageNotificationWorker(
    __in MxDeviceObject* RelatedDevice,
    __inout FxIrp* RelatedIrp,
    __in FxIrp* OriginalIrp,
    __in BOOLEAN Revert
    );

IO_WORKITEM_ROUTINE
_DeviceUsageNotificationWorkItem;

struct FxUsageWorkitemParameters {

    FxUsageWorkitemParameters(
        VOID
        )
    {
        RelatedDevice = NULL;
        RelatedIrp = NULL;
        OriginalIrp = NULL;
        Status = STATUS_UNSUCCESSFUL;
    }

    _In_ MxDeviceObject* RelatedDevice;

    _In_ FxIrp* RelatedIrp;

    _In_ FxIrp* OriginalIrp;

    _In_ BOOLEAN Revert;

    _In_ FxCREvent Event;

    _Out_ NTSTATUS Status;
};


#endif // _PNPPRIVKM_H_
