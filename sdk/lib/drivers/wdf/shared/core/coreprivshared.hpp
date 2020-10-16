/*++

Copyright (c) Microsoft Corporation

Module Name:

    corepriv.hpp

Abstract:

    This is the main driver framework.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#pragma once

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#define FX_IS_USER_MODE (TRUE)
#define FX_IS_KERNEL_MODE (FALSE)
#elif ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#define FX_IS_USER_MODE (FALSE)
#define FX_IS_KERNEL_MODE (TRUE)
#endif

/*
extern "C" {
#include <ntddk.h>
#include "wdf.h"
}

#define WDF_REGISTRY_BASE_PATH L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf"

*/

extern "C" {
#include "mx.h"
}

#include "fxmin.hpp"

#include "wdfmemory.h"
#include "wdfrequest.h"
#include "wdfdevice.h"
// #include "wdfdevicepri.h"
// #include "wdfiotargetpri.h"
#include "wdfwmi.h"
#include "wdfchildlist.h"
#include "wdfpdo.h"
#include "wdffdo.h"
#include "wdfiotarget.h"
#include "wdfcontrol.h"
#include "wdfcx.h"
#include "wdfio.h"
#include "wdfqueryinterface.h"
// #include "wdftriage.h"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#include "fxirpum.hpp"
#else
#include "fxirpkm.hpp"
#endif

// <FxSystemWorkItem.hpp>
typedef
VOID
(*PFN_WDF_SYSTEMWORKITEM) (
    IN PVOID             Parameter
    );

#include "fxirpqueue.hpp"


// </FxSystemWorkItem.hpp>


#include "fxprobeandlock.h"
#include "fxpackage.hpp"
#include "fxcollection.hpp"
#include "fxdeviceinitshared.hpp"

#include "ifxmemory.hpp"
#include "fxcallback.hpp"
#include "fxrequestcontext.hpp"
#include "fxrequestcontexttypes.h"
#include "fxrequestbase.hpp"
#include "fxmemoryobject.hpp"
#include "fxmemorybuffer.hpp"

#include "fxmemorybufferfrompool.hpp"

#include "fxmemorybufferpreallocated.hpp"

#include "fxtransactionedlist.hpp"

//
// MERGE temp: We may not need these include files here,
// temporarily including them to verify they compile in shared code
//
#include "fxrequestvalidatefunctions.hpp"
#include "fxrequestcallbacks.hpp"

// support
#include "stringutil.hpp"
#include "fxautostring.hpp"
#include "fxstring.hpp"
#include "fxdevicetext.hpp"
#include "fxcallback.hpp"
#include "fxdisposelist.hpp"
#include "fxsystemthread.hpp"

#include "fxirppreprocessinfo.hpp"
#include "fxpnpcallbacks.hpp"

// device init
#include "fxcxdeviceinit.hpp"
#include "fxcxdeviceinfo.hpp"
#include "fxdeviceinit.hpp"

#include "fxdevicetomxinterface.hpp"

// request
#include "fxrequestmemory.hpp"
#include "fxrequest.hpp"
#include "fxrequestbuffer.hpp"
#include "fxsyncrequest.hpp"

// io target
#include "fxiotarget.hpp"
#include "fxiotargetself.hpp"

#include "fxsystemworkitem.hpp"
#include "fxcallbackmutexlock.hpp"
#include "fxdriver.hpp"

#include "fxdeviceinterface.hpp"
#include "fxqueryinterface.hpp"

#include "fxcallbackspinlock.hpp"
#include "fxdefaultirphandler.hpp"
#include "fxwmiirphandler.hpp"

// packages
#include "fxpkgio.hpp"
#include "fxpkgpnp.hpp"
#include "fxpkgfdo.hpp"
#include "fxpkgpdo.hpp"
#include "fxpkggeneral.hpp"
#include "fxfileobject.hpp"
#include "fxioqueue.hpp"
#include "fxdevice.hpp"
#include "fxtelemetry.hpp"

#include "fxchildlist.hpp"

#include "fxlookasidelist.hpp"

/*#if FX_IS_KERNEL_MODE
#include "wdfrequest.h"
#endif*/

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V1_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callbacks
    //
    PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

    PFN_WDF_DRIVER_UNLOAD    EvtDriverUnload;

    //
    // Combination of WDF_DRIVER_INIT_FLAGS values
    //
    ULONG DriverInitFlags;

} WDF_DRIVER_CONFIG_V1_0, *PWDF_DRIVER_CONFIG_V1_0;

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V1_1 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callbacks
    //
    PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

    PFN_WDF_DRIVER_UNLOAD    EvtDriverUnload;

    //
    // Combination of WDF_DRIVER_INIT_FLAGS values
    //
    ULONG DriverInitFlags;

} WDF_DRIVER_CONFIG_V1_1, *PWDF_DRIVER_CONFIG_V1_1;


typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Indicates whether the device can wake itself up while the machine is in
    // S0.
    //
    WDF_POWER_POLICY_S0_IDLE_CAPABILITIES IdleCaps;

    //
    // The low power state in which the device will be placed when it is idled
    // out while the machine is in S0.
    //
    DEVICE_POWER_STATE DxState;

    //
    // Amount of time the device must be idle before idling out.  Timeout is in
    // milliseconds.
    //
    ULONG IdleTimeout;

    //
    // Inidcates whether a user can control the idle policy of the device.
    // By default, a user is allowed to change the policy.
    //
    WDF_POWER_POLICY_S0_IDLE_USER_CONTROL UserControlOfIdleSettings;

    //
    // If WdfTrue, idling out while the machine is in S0 will be enabled.
    //
    // If WdfFalse, idling out will be disabled.
    //
    // If WdfUseDefault, the idling out will be enabled.  If
    // UserControlOfIdleSettings is set to IdleAllowUserControl, the user's
    // settings will override the default.
    //
    WDF_TRI_STATE Enabled;

    //
    // This field is applicable only when IdleCaps == IdleCannotWakeFromS0
    // If WdfTrue,device is powered up on System Wake even if device is idle
    // If WdfFalse, device is not powered up on system wake if it is idle
    // If WdfUseDefault, the behavior is same as WdfFalse
    //
    WDF_TRI_STATE PowerUpIdleDeviceOnSystemWake;

} WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9, *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9;

typedef struct _WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Indicates whether the device can wake itself up while the machine is in
    // S0.
    //
    WDF_POWER_POLICY_S0_IDLE_CAPABILITIES IdleCaps;

    //
    // The low power state in which the device will be placed when it is idled
    // out while the machine is in S0.
    //
    DEVICE_POWER_STATE DxState;

    //
    // Amount of time the device must be idle before idling out.  Timeout is in
    // milliseconds.
    //
    ULONG IdleTimeout;

    //
    // Inidcates whether a user can control the idle policy of the device.
    // By default, a user is allowed to change the policy.
    //
    WDF_POWER_POLICY_S0_IDLE_USER_CONTROL UserControlOfIdleSettings;

    //
    // If WdfTrue, idling out while the machine is in S0 will be enabled.
    //
    // If WdfFalse, idling out will be disabled.
    //
    // If WdfUseDefault, the idling out will be enabled.  If
    // UserControlOfIdleSettings is set to IdleAllowUserControl, the user's
    // settings will override the default.
    //
    WDF_TRI_STATE Enabled;

} WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7, *PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7;

typedef struct _WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // The low power state in which the device will be placed when it is armed
    // for wake from Sx.
    //
    DEVICE_POWER_STATE DxState;

    //
    // Inidcates whether a user can control the idle policy of the device.
    // By default, a user is allowed to change the policy.
    //
    WDF_POWER_POLICY_SX_WAKE_USER_CONTROL UserControlOfWakeSettings;

    //
    // If WdfTrue, arming the device for wake while the machine is in Sx is
    // enabled.
    //
    // If WdfFalse, arming the device for wake while the machine is in Sx is
    // disabled.
    //
    // If WdfUseDefault, arming will be enabled.  If UserControlOfWakeSettings
    // is set to WakeAllowUserControl, the user's settings will override the
    // default.
    //
    WDF_TRI_STATE Enabled;

} WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5, *PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5;

//
// Versioning of structures for wdftimer.h
//
typedef struct _WDF_TIMER_CONFIG_V1_7 {
    ULONG         Size;

    PFN_WDF_TIMER EvtTimerFunc;

    LONG          Period;

    //
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN       AutomaticSerialization;

} WDF_TIMER_CONFIG_V1_7, *PWDF_TIMER_CONFIG_V1_7;

typedef struct _WDF_TIMER_CONFIG_V1_11 {
    ULONG Size;

    PFN_WDF_TIMER EvtTimerFunc;

    ULONG Period;

    //
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN AutomaticSerialization;

    //
    // Optional tolerance for the timer in milliseconds.
    //
    ULONG TolerableDelay;

} WDF_TIMER_CONFIG_V1_11, *PWDF_TIMER_CONFIG_V1_11;

