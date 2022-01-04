/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Fx.hpp

Abstract:

    This is the main driver framework include file.

Author:
    WDF team

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FX_H
#define _FX_H

extern "C" {
#include "mx.h"
}

#include "fxmacros.hpp"

extern "C" {
#include "wdf.h"
// #include "wdmsec.h"
#include "wdmguid.h"

// #include "wdfdevicepri.h"
// #include "wdfiotargetpri.h"
#include "wdfcx.h"
#include "wdfldr.h"

#include <fxdynamicswrapper.h>

#include "wdf10.h"
#include "wdf11.h"
#include "wdf15.h"
#include "wdf17.h"
#include "wdf19.h"
#include "wdf111.h"
#include "wdf113.h"
#include "wdf115.h"
}

#define KMDF_ONLY_CODE_PATH_ASSERT()

// Integer overflow functions
#include "ntintsafe.h"

#include "fxforward.hpp"

//KMDF defines for shared type names
#include "fxtypedefskm.hpp"

// #include "fxwmicompat.h"
// #include "fxtrace.h"
#if defined(EVENT_TRACING)
#include "fxwmicompat.h"
#include "fxtrace.h"
#else
#include "dbgtrace.h"
#endif  // EVENT_TRACING
#include "fxtypes.h"
#include "fxrequestcontexttypes.h"
#include "fxpool.h"
#include "fxglobalskm.h"
#include "fxpoolinlines.hpp"
#include "fxverifier.h"
#include "fxverifierkm.h"
#include "fxmdl.h"
#include "fxprobeandlock.h"

#include "fxperftracekm.hpp"
// #include "DriverFrameworks-KernelMode-KmEvents.h"

#include <ntstrsafe.h>

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
#include "fxspinlock.hpp"

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
#include "fxnpagedlookasidelist.hpp"
#include "fxpagedlookasidelist.hpp"
#include "fxmemoryobject.hpp"
#include "fxmemorybuffer.hpp"
#include "fxmemorybufferfrompool.hpp"
#include "fxmemorybufferpreallocated.hpp"
#include "fxmemorybufferfromlookaside.hpp"
#include "fxrequestmemory.hpp"
#include "fxregkey.hpp"
#include "fxautoregistry.hpp"
#include "fxautostring.hpp"
#include "fxstring.hpp"

#include "fxvalidatefunctions.hpp"
#include "fxrequestvalidatefunctions.hpp"

#include "fxresource.hpp"
#include "fxrelateddevice.hpp"
#include "fxdeviceinterface.hpp"
#include "fxqueryinterface.hpp"
#include "fxdevicetext.hpp"

#include "fxirp.hpp"
#include "fxdriver.hpp"

// generic package interface
#include "fxpackage.hpp"
#include "fxpkggeneral.hpp"
#include "fxdefaultirphandler.hpp"
#include "fxpkgpnp.hpp"
#include "fxwatchdog.hpp"

// Device support
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

// specialized irp handlers (ie packages)
#include "fxpkgfdo.hpp"
#include "fxpkgpdo.hpp"
#include "fxwmiirphandler.hpp"
#include "fxwmiprovider.hpp"
#include "fxwmiinstance.hpp"

// queus for read, write, (internal) IOCTL
#include "fxioqueue.hpp"
#include "fxfileobject.hpp"
#include "fxirppreprocessinfo.hpp"
#include "fxirpdynamicdispatchinfo.hpp"

#include "fxdpc.hpp"
#include "fxworkitem.hpp"
#include "fxtimer.hpp"
#include "fxinterruptkm.hpp"

// IO targets (device lower edge interface)
#include "fxiotarget.hpp"
#include "fxiotargetremote.hpp"
#include "fxiotargetself.hpp"

#include "fxusbdevice.hpp"
#include "fxusbinterface.hpp"
#include "fxusbpipe.hpp"

// DMA support
#include "fxdmaenabler.hpp"
#include "fxdmatransaction.hpp"
#include "fxcommonbuffer.hpp"

// Triage info.
// #include "wdftriage.h"

#include "fxtelemetry.hpp"
#endif // _FX_H
