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
#include "fxmacros.hpp"

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
// #include "wdfdevicepri.h"
// #include "wdfiotargetpri.h"
#include "wdfdriver.h"

#include "wdfmemory.h"

#include "wdfrequest.h"
#include "wdfwmi.h"
#include "wdfchildlist.h"
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

// #pragma warning(disable:4200)  // suppress nameless struct/union warning
// #pragma warning(disable:4201)  // suppress nameless struct/union warning
// #pragma warning(disable:4214)  // suppress bit field types other than int warning
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

// #include "cobbled.hpp"

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

#include "fxforward.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "hostfxutil.h"
#include "wdfplatform.h"
#include "wdfplatformimpl.h"
#include "debug.h"
#include "devreg.h"
#include "wudfx_namespace_on.h"
#include "wudfx.h"
#include "wudfx_namespace_off.h"
#include "driverframeworks-usermode-umevents.h"
#endif

#include "fxtypedefs.hpp"

#if defined(EVENT_TRACING)
#include "fxwmicompat.h"
#include "fxtrace.h"
#else
#include "dbgtrace.h"
#endif  // EVENT_TRACING

#include "fxtypes.h"
#include "fxrequestcontexttypes.h"
#include "fxpool.h"

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "fxglobalskm.h"
#include "fxperftracekm.hpp"
// #include "DriverFrameworks-KernelMode-KmEvents.h"
#else
#include "fxglobalsum.h"
#endif
#include "fxpoolinlines.hpp"
#include "fxverifier.h"
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "fxverifierkm.h"
#else
#include "fxverifierum.h"
#include "device_common.h"
#endif
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "fxmdl.h"
#endif

#include "fxprobeandlock.h"
//#include "FxPerfTraceKm.hpp"
//#include <NtStrSafe.h>

#include "fxstump.hpp"
#include "fxrequestbuffer.hpp"
#include "fxtagtracker.hpp"

// internal locks
#include "fxverifierlock.hpp"
#include "fxlock.hpp"

// base objects
#include "fxobject.hpp"
#include "fxpagedobject.hpp"
#include "fxnonpagedobject.hpp"

#include "fxhandle.h"

// external locks
#include "fxwaitlock.hpp"
//#include "FxSpinLock.hpp"

// utitilty classes and functions
#include "fxtransactionedlist.hpp"
#include "fxrelateddevicelist.hpp"
#include "fxdisposelist.hpp"
#include "fxcollection.hpp"
#include "stringutil.hpp"

// abstract classes
#include "ifxhascallbacks.hpp"

// callback delegation and locking
#include "fxsystemthread.hpp"
#include "fxsystemworkitem.hpp"
#include "fxcallbacklock.hpp"
#include "fxcallbackspinlock.hpp"
#include "fxcallbackmutexlock.hpp"
#include "fxcallback.hpp"
#include "fxsystemthread.hpp"

#include "ifxmemory.hpp"
#include "fxlookasidelist.hpp"
//#include "fxnpagedlookasidelist.hpp"
//#include "fxpagedlookasidelist.hpp"
#include "fxmemoryobject.hpp"
#include "fxmemorybuffer.hpp"
#include "fxmemorybufferfrompool.hpp"
#include "fxmemorybufferpreallocated.hpp"
//#include "fxmemorybufferfromlookaside.hpp"
#include "fxrequestmemory.hpp"
#include "fxregkey.hpp"
#include "fxautoregistry.hpp"
#include "fxautostring.hpp"
#include "fxstring.hpp"

#include "fxvalidatefunctions.hpp"

#include "fxresource.hpp"
#include "fxrelateddevice.hpp"
#include "fxdeviceinterface.hpp"
#include "fxqueryinterface.hpp"
#include "fxdevicetext.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "fxirpum.hpp"
#include "fxinterruptthreadpoolum.hpp"
#else
#include "fxirpkm.hpp"
#endif
#include "fxdriver.hpp"

// generic package interface
#include "fxpackage.hpp"
#include "fxpkggeneral.hpp"
#include "fxdefaultirphandler.hpp"
//#include "fxpoxkm.hpp"
#include "fxpkgpnp.hpp"
#include "fxwatchdog.hpp"

// Device supportfx
#include "fxchildlist.hpp"
#include "fxcxdeviceinfo.hpp"

#include "fxdevice.hpp"

#include "fxpkgio.hpp"

#include "fxdevicetomxinterface.hpp"

#include "fxirpqueue.hpp"
#include "fxrequestcontext.hpp"
#include "fxrequestcallbacks.hpp"
#include "fxrequestbase.hpp"
#include "fxrequest.hpp"
#include "fxsyncrequest.hpp"

#include "fxrequestvalidatefunctions.hpp"

// specialized irp handlers (ie packages)
#include "fxpkgfdo.hpp"
#include "fxpkgpdo.hpp"

//#include "fxwmiirphandler.hpp"
//#include "fxwmiprovider.hpp"
//#include "fxwmiinstance.hpp"

// queus for read, write, (internal) IOCTL
#include "fxioqueue.hpp"
#include "fxfileobject.hpp"
#include "fxirppreprocessinfo.hpp"
#include "fxirpdynamicdispatchinfo.hpp"

//#include "fxdpc.hpp"
#include "fxworkitem.hpp"
#include "fxtimer.hpp"
#if FX_CORE_MODE==FX_CORE_USER_MODE
#include "fxinterruptum.hpp"
#include "fxmessagedispatchum.hpp"
#else
#include "fxinterruptkm.hpp"
#endif

// IO targets (device lower edge interface)
#include "fxiotarget.hpp"
#include "fxiotargetremote.hpp"
#include "fxiotargetself.hpp"

#include "fxusbdevice.hpp"
#include "fxusbinterface.hpp"
#include "fxusbpipe.hpp"

// DMA support
//#include "fxdmaenabler.hpp"
//#include "fxdmatransaction.hpp"
//#include "fxcommonbuffer.hpp"

// Triage info.
// #include "wdftriage.h"


#include "fxpkgioshared.hpp"

#if FX_CORE_MODE==FX_CORE_USER_MODE
#include "ufxverifier.h"
#endif


#endif // _FXMIN_HPP
