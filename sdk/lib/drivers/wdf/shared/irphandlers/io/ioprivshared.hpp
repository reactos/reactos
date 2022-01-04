/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    iopriv.hpp

Abstract:

    This module defines private interfaces for the I/O package

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _IOPRIV_H_
#define _IOPRIV_H_

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#define FX_IS_USER_MODE (TRUE)
#define FX_IS_KERNEL_MODE (FALSE)
#elif ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#define FX_IS_USER_MODE (FALSE)
#define FX_IS_KERNEL_MODE (TRUE)
#endif

/*#if defined(MODE_AGNSOTIC_FXPKGIO_NOT_IN_SHARED_FOLDER)
#include <fx.hpp>
#else
// common header file for all irphandler\* files
#include "irphandlerspriv.hpp"
#endif

#if FX_IS_USER_MODE
#define PWDF_REQUEST_PARAMETERS  PVOID
#define PFN_WDF_REQUEST_CANCEL   PVOID
#define PIO_CSQ_IRP_CONTEXT      PVOID
#endif*/

extern "C" {
#include "mx.h"
}

#include "fxmin.hpp"

#include "wdfmemory.h"
#include "wdfrequest.h"
#include "wdfio.h"
#include "wdfdevice.h"
#include "wdfwmi.h"
#include "wdfchildlist.h"
#include "wdfpdo.h"
#include "wdffdo.h"
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

#include "fxsystemthread.hpp"

#include "fxcallbackspinlock.hpp"
#include "fxcallbackmutexlock.hpp"
#include "fxtransactionedlist.hpp"

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "fxirpkm.hpp"
#else
#include "fxirpum.hpp"
#endif


#include "fxpackage.hpp"
#include "fxcollection.hpp"
#include "fxdeviceinitshared.hpp"
#include "fxdevicetomxinterface.hpp"

#include "ifxmemory.hpp"
#include "fxcallback.hpp"
#include "fxrequestcontext.hpp"
#include "fxrequestcontexttypes.h"
#include "fxrequestbase.hpp"
#include "fxmemoryobject.hpp"
#include "fxmemorybufferpreallocated.hpp"
#include "fxrequestmemory.hpp"
#include "fxrequest.hpp"
#include "fxrequestbuffer.hpp"
#include "fxsyncrequest.hpp"

#include "shared/irphandlers/irphandlerspriv.hpp"
#include "fxpkgpnp.hpp"
#include "fxpkgio.hpp"
#include "fxioqueue.hpp"
#include "fxioqueuecallbacks.hpp"


//
// At this time we are unable to include wdf17.h in the share code, thus for
// now we simply cut and paste the needed structures.
//
typedef struct _WDF_IO_QUEUE_CONFIG_V1_7 {
    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

} WDF_IO_QUEUE_CONFIG_V1_7, *PWDF_IO_QUEUE_CONFIG_V1_7;

//
// At this time we are unable to include wdf19.h in the shared code, thus for
// now we simply cut and paste the needed structures.
//
//
typedef struct _WDF_IO_QUEUE_CONFIG_V1_9 {
    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

    union {
        struct {
            ULONG NumberOfPresentedRequests;

        } Parallel;

    } Settings;

} WDF_IO_QUEUE_CONFIG_V1_9, *PWDF_IO_QUEUE_CONFIG_V1_9;


//
// public headers
//
/*#include "wdfDevice.h"
#include "wdfChildList.h"
#include "wdfPdo.h"
#include "wdffdo.h"
#include "wdfQueryInterface.h"
#include "wdfMemory.h"
#include "wdfWmi.h"

#if FX_IS_KERNEL_MODE
#include "wdfrequest.h"
#endif
#include "wdfio.h"



#include "FxCallback.hpp"


#include "FxRequestBaseShared.hpp"
#include "FxRequestShared.hpp"

// FxDevice To Shared interface header
#include "FxDeviceToMxInterface.hpp"
#include "FxRequestToMxInterface.hpp"

#if defined(MODE_AGNSOTIC_FXPKGIO_NOT_IN_SHARED_FOLDER)
#include "FxRequestToMxInterfaceKM.hpp"
#endif

//
// private headers
//
#include "FxTransactionedList.hpp"
#include "FxSystemThread.hpp"

#if FX_IS_KERNEL_MODE
#include "FxFileObject.hpp"
#endif

#include "Fxirpqueue.hpp"

#include "FxCallbackSpinLock.hpp"
#include "FxCallbackMutexLock.hpp"

//#include "FxDeviceInterface.hpp"
//#include "FxQueryInterface.hpp"
//#include "FxPnpCallbacks.hpp"
#include "FxPackage.hpp"

#include "FxChildList.hpp"
#include "FxPkgPnp.hpp"
#include "FxPkgPdo.hpp"

#include "FxPkgIo.hpp"
#include "FxIoQueue.hpp"


#if !defined(MODE_AGNSOTIC_FXPKGIO_NOT_IN_SHARED_FOLDER)

// <wdf17.h>
//
// This is the structure used to configure an IoQueue and
// register callback events to it.
//
//
typedef struct _WDF_IO_QUEUE_CONFIG_V1_7 {
    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

} WDF_IO_QUEUE_CONFIG_V1_7, *PWDF_IO_QUEUE_CONFIG_V1_7;

// </wdf17.h>

//
// <wdfbugcoded.h>
//
typedef struct _WDF_QUEUE_FATAL_ERROR_DATA {
    WDFQUEUE Queue;

    WDFREQUEST Request;

    NTSTATUS Status;

} WDF_QUEUE_FATAL_ERROR_DATA, *PWDF_QUEUE_FATAL_ERROR_DATA;

// </wdfbugcoded.h>

#endif //MODE_AGNSOTIC_FXPKGIO_NOT_IN_SHARED_FOLDER*/

#endif  //_IOPRIV_H_
