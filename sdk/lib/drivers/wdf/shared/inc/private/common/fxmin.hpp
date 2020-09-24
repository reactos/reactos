/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxMin.hpp

Abstract:

    This is the minimal version of driver framework include file
    that is needed to build FxObject






Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXMIN_HPP
#define _FXMIN_HPP

extern "C" {
#include "mx.h"
}
#include "FxMacros.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
//
// Undef METHOD_FROM_CTL_CODE so that it is prevented from being used
// inadvertently in mode-agnostic code. Irp->GetParameterIoctlCodeBufferMethod
// should be used instead.
//
#ifdef METHOD_FROM_CTL_CODE
#undef METHOD_FROM_CTL_CODE
#endif
#endif // FX_CORE_USER_MODE

typedef struct _WDF_BIND_INFO *PWDF_BIND_INFO;

extern "C" {

///////////////////
// Basic definitions (abridged version of wdf.h)

#ifdef __cplusplus
  #define WDF_EXTERN_C       extern "C"
  #define WDF_EXTERN_C_START extern "C" {
  #define WDF_EXTERN_C_END   }
#else
  #define WDF_EXTERN_C
  #define WDF_EXTERN_C_START
  #define WDF_EXTERN_C_END
#endif

WDF_EXTERN_C_START

typedef VOID (*WDFFUNC) (VOID);
extern const WDFFUNC *WdfFunctions;

#include "wdftypes.h"
#include "wdfglobals.h"
#include "wdffuncenum.h"
#include "wdfstatus.h"
#include "wdfassert.h"
#include "wdfverifier.h"

// generic object
#include "wdfobject.h"

#include "wdfcore.h"
#include "wdfdevice.h"
#include "wdfdevicepri.h"
#include "wdfiotargetpri.h"
#include "wdfdriver.h"

#include "wdfmemory.h"

#include "wdfrequest.h"
#include "wdfwmi.h"
#include "wdfChildList.h"
#include "wdfpdo.h"
#include "wdffdo.h"
#include "wdfiotarget.h"
#include "wdfcontrol.h"
#include "wdfcx.h"
#include "wdfio.h"
#include "wdfqueryinterface.h"
#include "wdfworkitem.h"

#include "wdfcollection.h"

#include "wdffileobject.h"
#include "wdfinterrupt.h"
#include "wdfregistry.h"
#include "wdfresource.h"
#include "wdfstring.h"
#include "wdfsync.h"
#include "wdftimer.h"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "wdfhid.h"
#endif

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <usb.h>
#include <usbspec.h>
#include "wdfusb.h"
#include <initguid.h>
#include <usbdlib.h>
#include <usbbusif.h>

#include "wdfbugcodes.h"

WDF_EXTERN_C_END

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#define KMDF_ONLY_CODE_PATH_ASSERT()  FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
#define UMDF_ONLY_CODE_PATH_ASSERT()
#else
#define KMDF_ONLY_CODE_PATH_ASSERT()
#define UMDF_ONLY_CODE_PATH_ASSERT()  ASSERTMSG("Not implemented for KMDF", FALSE);
#endif

//
// Since C does not have strong type checking we must invent our own
//
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0 {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // String representation of the context's type name, i.e. "DEVICE_CONTEXT"
    //
    PCHAR ContextName;

    //
    // The size of the context in bytes.  This will be the size of the context
    // associated with the handle unless
    // WDF_OBJECT_ATTRIBUTES::ContextSizeOverride is specified.
    //
    size_t ContextSize;

} WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0, *PWDF_OBJECT_CONTEXT_TYPE_INFO_V1_0;

typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0 *PWDF_OBJECT_CONTEXT_TYPE_INFO_V1_0;

#include "cobbled.hpp"

}  //extern "C"

#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <wmistr.h>
#include <evntrace.h>

#if FX_CORE_MODE==FX_CORE_USER_MODE
typedef enum _TRACE_INFORMATION_CLASS {
    TraceIdClass,
    TraceHandleClass,
    TraceEnableFlagsClass,
    TraceEnableLevelClass,
    GlobalLoggerHandleClass,
    EventLoggerHandleClass,
    AllLoggerHandlesClass,
    TraceHandleByNameClass,
    LoggerEventsLostClass,
    TraceSessionSettingsClass,
    MaxTraceInformationClass
} TRACE_INFORMATION_CLASS;

#include <wmium.h>
#endif  // FX_CORE_MODE==FX_CORE_USER_MODE

#include "FxForward.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "HostFxUtil.h"
#include "wdfplatform.h"
#include "wdfplatformimpl.h"
#include "debug.h"
#include "devreg.h"
#include "wudfx_namespace_on.h"
#include "wudfx.h"
#include "wudfx_namespace_off.h"
#include "DriverFrameworks-UserMode-UmEvents.h"
#endif

#include "fxtypedefs.hpp"

#if defined(EVENT_TRACING)
#include "fxwmicompat.h"
#include "FxTrace.h"
#else
#include "DbgTrace.h"
#endif  // EVENT_TRACING

#include "FxTypes.h"
#include "fxrequestcontexttypes.h"
#include "FxPool.h"

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "FxGlobalsKm.h"
#include "FxPerfTraceKm.hpp"
#include "DriverFrameworks-KernelMode-KmEvents.h"
#else
#include "FxGlobalsUm.h"
#endif
#include "FxPoolInlines.hpp"
#include "fxverifier.h"
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "FxVerifierKm.h"
#else
#include "FxVerifierUm.h"
#include "device_common.h"
#endif
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "FxMdl.h"
#endif

#include "FxProbeAndLock.h"
//#include "FxPerfTraceKm.hpp"
//#include <NtStrSafe.h>

#include "FxStump.hpp"
#include "FxRequestBuffer.hpp"
#include "FxTagTracker.hpp"

// internal locks
#include "FxVerifierLock.hpp"
#include "FxLock.hpp"

// base objects
#include "FxObject.hpp"
#include "FxPagedObject.hpp"
#include "FxNonPagedObject.hpp"

#include "fxhandle.h"

// external locks
#include "FxWaitLock.hpp"
//#include "FxSpinLock.hpp"

// utitilty classes and functions
#include "FxTransactionedList.hpp"
#include "FxRelatedDeviceList.hpp"
#include "FxDisposeList.hpp"
#include "FxCollection.hpp"
#include "StringUtil.hpp"

// abstract classes
#include "IFxHasCallbacks.hpp"

// callback delegation and locking
#include "FxSystemThread.hpp"
#include "FxSystemWorkItem.hpp"
#include "FxCallbackLock.hpp"
#include "FxCallbackSpinLock.hpp"
#include "FxCallbackMutexLock.hpp"
#include "FxCallback.hpp"
#include "FxSystemThread.hpp"

#include "IFxMemory.hpp"
#include "FxLookasideList.hpp"
//#include "FxNPagedLookasideList.hpp"
//#include "FxPagedLookasideList.hpp"
#include "FxMemoryObject.hpp"
#include "FxMemoryBuffer.hpp"
#include "FxMemoryBufferFromPool.hpp"
#include "FxMemoryBufferPreallocated.hpp"
//#include "FxMemoryBufferFromLookaside.hpp"
#include "FxRequestMemory.hpp"
#include "FxRegKey.hpp"
#include "FxAutoRegistry.hpp"
#include "FxAutoString.hpp"
#include "FxString.hpp"

#include "FxValidateFunctions.hpp"

#include "FxResource.hpp"
#include "FxRelatedDevice.hpp"
#include "FxDeviceInterface.hpp"
#include "FxQueryInterface.hpp"
#include "FxDeviceText.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "FxIrpUm.hpp"
#include "FxInterruptThreadpoolUm.hpp"
#else
#include "FxIrpKm.hpp"
#endif
#include "FxDriver.hpp"

// generic package interface
#include "FxPackage.hpp"
#include "FxPkgGeneral.hpp"
#include "FxDefaultIrpHandler.hpp"
//#include "FxPoxKm.hpp"
#include "FxPkgPnp.hpp"
#include "FxWatchDog.hpp"

// Device supportfx
#include "FxChildList.hpp"
#include "FxCxDeviceInfo.hpp"

#include "FxDevice.hpp"

#include "FxPkgIo.hpp"

#include "FxDeviceToMxInterface.hpp"

#include "FxIrpQueue.hpp"
#include "FxRequestContext.hpp"
#include "FxRequestCallbacks.hpp"
#include "FxRequestBase.hpp"
#include "FxRequest.hpp"
#include "FxSyncRequest.hpp"

#include "FxRequestValidateFunctions.hpp"

// specialized irp handlers (ie packages)
#include "FxPkgFdo.hpp"
#include "FxPkgPdo.hpp"

//#include "FxWmiIrpHandler.hpp"
//#include "FxWmiProvider.hpp"
//#include "FxWmiInstance.hpp"

// queus for read, write, (internal) IOCTL
#include "FxIoQueue.hpp"
#include "FxFileObject.hpp"
#include "FxIrpPreprocessInfo.hpp"
#include "FxIrpDynamicDispatchInfo.hpp"

//#include "FxDpc.hpp"
#include "FxWorkItem.hpp"
#include "FxTimer.hpp"
#if FX_CORE_MODE==FX_CORE_USER_MODE
#include "FxInterruptUm.hpp"
#include "FxMessageDispatchUm.hpp"
#else
#include "FxInterruptKm.hpp"
#endif

// IO targets (device lower edge interface)
#include "FxIoTarget.hpp"
#include "FxIoTargetRemote.hpp"
#include "FxIoTargetSelf.hpp"

#include "FxUsbDevice.hpp"
#include "FxUsbInterface.hpp"
#include "FxUsbPipe.hpp"

// DMA support
//#include "FxDmaEnabler.hpp"
//#include "FxDmaTransaction.hpp"
//#include "FxCommonBuffer.hpp"

// Triage info.
#include "wdftriage.h"


#include "FxPkgIoShared.hpp"

#if FX_CORE_MODE==FX_CORE_USER_MODE
#include "UfxVerifier.h"
#endif


#endif // _FXMIN_HPP
