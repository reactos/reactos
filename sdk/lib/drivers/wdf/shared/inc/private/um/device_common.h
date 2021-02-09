/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    device_common.h

Abstract:

    This file contains common types for device objects that are used in
    all three components (Wudfx, Wudfhost, WudfRd).

Author:



Revision History:



--*/

#pragma once

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

#define ERROR_STRING_HW_ACCESS_NOT_ALLOWED                          \
        "Hardware access not allowed. Set the INF directive "       \
        "UmdfDirectHardwareAccess to AllowDirectHardwareAccess "    \
        "in driver's INF file to enable direct hardware access"

#define WUDF_POWER_POLICY_SETTINGS L"WudfPowerPolicySettings"

