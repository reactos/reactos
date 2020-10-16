//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _PNPPRIVKM_H_
#define _PNPPRIVKM_H_

// public headers
#include "wdfdmaenabler.h"

// private headers
#include "fxirpqueue.hpp"
#include "fxcallback.hpp"

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

#include "fxcallbackspinlock.hpp"
#include "fxcallbackmutexlock.hpp"
#include "fxpackage.hpp"
#include "ifxmemory.hpp"
#include "fxcallback.hpp"
#include "fxrequestcontext.hpp"
#include "fxrequestcontexttypes.h"
#include "fxrequestbase.hpp"
#include "fxrequest.hpp"
#include "fxpkgpnp.hpp"
#include "fxpkgio.hpp"
#include "fxioqueue.hpp"

#include "fxdmaenabler.hpp"
#include "fxsystemworkitem.hpp"

// #include "FxDsf.h"    // DSF support.
// #include <device_common.h>
// #include "FxTelemetry.hpp"
// __REACTOS__
typedef struct _STACK_DEVICE_CAPABILITIES {
    //
    // The capabilities as garnered from IRP_MN_QUERY_CAPABILITIES.
    //
    DEVICE_CAPABILITIES DeviceCaps;

    //
    // The lowest-power D-state that a device can be in and still generate
    // a wake signal, indexed by system state.  (PowerSystemUnspecified is
    // an unused slot in this array.)
    //
    DEVICE_WAKE_DEPTH DeepestWakeableDstate[PowerSystemHibernate+1];
} STACK_DEVICE_CAPABILITIES, *PSTACK_DEVICE_CAPABILITIES;
// end __REACTOS__


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
