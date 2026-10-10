/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX class extension interface for host controller drivers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#ifndef WDFAPI
#error wdf.h must be included before ucxclass.h
#endif

#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <usb.h>
#include <usbioctl.h>
#include <usbdlib.h>

#include "ucxglobals.h"
#include "ucxfuncenum.h"
#include "ucxobjects.h"
#include "ucxusbdevice.h"
#include "ucxcontroller.h"
#include "ucxroothub.h"
#include "ucxendpoint.h"
#include "ucxsstreams.h"

WDF_EXTERN_C_START

typedef
_IRQL_requires_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_UCXINITIALIZEDEVICEINIT)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PWDFDEVICE_INIT DeviceInit);

/** Lets UCX hook the controller device init. Call before WdfDeviceCreate. */
_IRQL_requires_(PASSIVE_LEVEL)
_Must_inspect_result_
FORCEINLINE
NTSTATUS
NTAPI
UcxInitializeDeviceInit(
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    PFN_UCXINITIALIZEDEVICEINIT Initialize;

    Initialize = UCX_BOUND_FUNCTION(PFN_UCXINITIALIZEDEVICEINIT, UcxInitializeDeviceInitTableIndex);
    return Initialize(UcxDriverGlobals, DeviceInit);
}

WDF_EXTERN_C_END
