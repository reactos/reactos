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

#include "FxMacros.hpp"

extern "C" {
#include "wdf.h"
#include "wdmsec.h"
#include "wdmguid.h"

#include "wdfdevicepri.h"
#include "wdfiotargetpri.h"
#include "wdfcx.h"
#include "wdfldr.h"

#include <FxDynamicsWrapper.h>

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

#include "FxForward.hpp"

//KMDF defines for shared type names
#include "FxTypeDefsKm.hpp"

#include "fxwmicompat.h"
#include "fxtrace.h"
#include "fxtypes.h"
#include "fxrequestcontexttypes.h"
#include "fxpool.h"
#include "FxGlobalsKM.h"
#include "FxPoolInlines.hpp"
#include "fxverifier.h"
#include "fxverifierkm.h"
#include "FxMdl.h"
#include "FxProbeAndLock.h"

#include "FxPerfTraceKm.hpp"
#include "DriverFrameworks-KernelMode-KmEvents.h"

#include <NtStrSafe.h>

#include "FxStump.hpp"

#include "FxRequestBuffer.hpp"

#include "FxTagTracker.hpp"

// internal locks
#include "FxVerifierLock.hpp"
#include "FxLock.hpp"

// base objects
#include "fxobject.hpp"
#include "FxPagedObject.hpp"
#include "FxNonPagedObject.hpp"

#include "fxhandle.h"

// external locks
#include "FxWaitLock.hpp"
#include "FxSpinLock.hpp"

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
#include "FxNPagedLookasideList.hpp"
#include "FxPagedLookasideList.hpp"
#include "FxMemoryObject.hpp"
#include "FxMemoryBuffer.hpp"
#include "FxMemoryBufferFromPool.hpp"
#include "FxMemoryBufferPreallocated.hpp"
#include "FxMemoryBufferFromLookaside.hpp"
#include "FxRequestMemory.hpp"
#include "FxRegKey.hpp"
#include "FxAutoRegistry.hpp"
#include "FxAutoString.hpp"
#include "FxString.hpp"

#include "FxValidateFunctions.hpp"
#include "FxRequestValidateFunctions.hpp"

#include "FxResource.hpp"
#include "FxRelatedDevice.hpp"
#include "FxDeviceInterface.hpp"
#include "FxQueryInterface.hpp"
#include "FxDeviceText.hpp"

#include "FxIrp.hpp"
#include "FxDriver.hpp"

// generic package interface
#include "FxPackage.hpp"
#include "FxPkgGeneral.hpp"
#include "FxDefaultIrpHandler.hpp"
#include "FxPkgPnp.hpp"
#include "FxWatchDog.hpp"

// Device support
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

// specialized irp handlers (ie packages)
#include "FxPkgFdo.hpp"
#include "FxPkgPdo.hpp"
#include "FxWmiIrpHandler.hpp"
#include "FxWmiProvider.hpp"
#include "FxWmiInstance.hpp"

// queus for read, write, (internal) IOCTL
#include "FxIoQueue.hpp"
#include "FxFileObject.hpp"
#include "FxIrpPreprocessInfo.hpp"
#include "FxIrpDynamicDispatchInfo.hpp"

#include "FxDpc.hpp"
#include "FxWorkItem.hpp"
#include "FxTimer.hpp"
#include "FxInterruptKm.hpp"

// IO targets (device lower edge interface)
#include "FxIoTarget.hpp"
#include "FxIoTargetRemote.hpp"
#include "FxIoTargetSelf.hpp"

#include "FxUsbDevice.hpp"
#include "FxUsbInterface.hpp"
#include "FxUsbPipe.hpp"

// DMA support
#include "FxDmaEnabler.hpp"
#include "FxDmaTransaction.hpp"
#include "FxCommonBuffer.hpp"

// Triage info.
#include "wdftriage.h"

#include "FxTelemetry.hpp"
#endif // _FX_H
